/*
  Copyright [2016, 2023] Christian Neukam

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/
/*
  MIDI driven true bypass FX loop expansion for guitar amplifiers based on
  Arduino.
  
  Supports four effect loops with bank selection
  
  EEPROM memory layout (byte aligned):
    16 slots in total -> 16 bytes in EEPROM
    slot layout:
    
           XX         YY           ZZZZ
    high bank | low bank
          FX preset      | MIDI program
 */
#include <EEPROM.h>

/*****************************************************************/
/* DEFINES & CONSTANTS                                           */
/*****************************************************************/
/* led pins: */
const int PIN_LED_FX1 = 9;
const int PIN_LED_FX2 = 8;
const int PIN_LED_FX3 = 7;
const int PIN_LED_FX4 = 6;
const int PIN_LED_LRN = 5;

/* toggle pins: */
const int PIN_BUTTON_LRN = 4;
const int PIN_BUTTON_BKL = 3;
const int PIN_BUTTON_BKH = 2;

/* relais pins: */
const int PIN_RELAIS_FX1 = 13;
const int PIN_RELAIS_FX2 = 12;
const int PIN_RELAIS_FX3 = 11;
const int PIN_RELAIS_FX4 = 10;

/* debounce delay in ms */
const unsigned long DEBOUNCEDELAY = 50;

/* FX bank offset in bits */
const unsigned int BANKOFFSET = 2;

/* EEPROM constants */
const unsigned int EEPROM_ADDRESS      = 0;
const unsigned int EEPROM_NUMBYTES     = 16;
const unsigned int EEPROM_FXPRG_OFFSET = 4;

/* debug flags */
const int EEPROM_INIT = 0;

/*****************************************************************/
/* GLOBALS                                                       */
/*****************************************************************/
/* button flags: */
int g_button_flags      = 0;
int g_button_flags_prev = 0;

/* FX switch states: */
byte g_fx_state = 0;

/* midi learn state: */
byte g_midilearn_state = 0;

/* debounce interval: */
unsigned long g_lastDebounceTime = 0;

/* EEPROM memory */
byte eeprom_memory[EEPROM_NUMBYTES] = {0};

/*****************************************************************/
/* MAIN FUNCTIONS                                                */
/*****************************************************************/
/* the setup routine runs once when you press reset: */
void setup(
        void
) {
  /* set MIDI baud rate. */
  Serial.begin(31250);
  
  /* initialize the digital pins as an output. */
  pinMode(PIN_LED_FX1, OUTPUT);
  pinMode(PIN_LED_FX2, OUTPUT);
  pinMode(PIN_LED_FX3, OUTPUT);
  pinMode(PIN_LED_FX4, OUTPUT);
  pinMode(PIN_LED_LRN, OUTPUT);
  
  /* intitialize the digital pins as an input. */
  pinMode(PIN_BUTTON_LRN, INPUT);
  pinMode(PIN_BUTTON_BKL, INPUT);
  pinMode(PIN_BUTTON_BKH, INPUT);

  /* initialize the digital pins as an output. */
  pinMode(PIN_RELAIS_FX1, OUTPUT);
  pinMode(PIN_RELAIS_FX2, OUTPUT);
  pinMode(PIN_RELAIS_FX3, OUTPUT);
  pinMode(PIN_RELAIS_FX4, OUTPUT);

  /* send led test sequence */
  startUp();
  
  /* disable all FX loops as start-up config. */
  
  /* reset EEPROM on Arduino */
  if (EEPROM_INIT == 1) {
    resetEEPROM();
    delay(500);
  }
  
  /* read EEPROM into buffer */
  readEEPROM(eeprom_memory);
}

/* the loop routine runs over and over again forever: */
void loop(
        void
) {  
  /* select FX via buttons */
  manualSelect(&g_fx_state,
               &g_midilearn_state);
  
  /* select FX via MIDI, skip if MIDI learn is active */
  if (g_midilearn_state == 0) {
    midiSelect(&g_fx_state,
               eeprom_memory,
               EEPROM_NUMBYTES);
  }
  
  /* MIDI learn */
  midiLearn(&g_midilearn_state,
            g_fx_state,
            eeprom_memory,
            EEPROM_NUMBYTES);
}

/*****************************************************************/
/* STATIC FUNCTIONS                                              */
/*****************************************************************/
/* switch FX channels: */
void fxSwitch(
        const byte          fx_state
) {
  byte bank        = 0;
  byte flags       = 0;
  const int led[4] = {PIN_LED_FX1, PIN_LED_FX2, PIN_LED_FX3, PIN_LED_FX4};
  const int rel[4] = {PIN_RELAIS_FX1, PIN_RELAIS_FX2, PIN_RELAIS_FX3, PIN_RELAIS_FX4};
  
  for (bank = 0; bank < BANKOFFSET; bank++) {
    if (bank == 0) {
      flags = 0x03 & fx_state;
    } else {
      flags = (0x0C & fx_state) >> BANKOFFSET;
    }

    switch (flags) {
      case 0:
        digitalWrite(led[bank * BANKOFFSET],     LOW);
        digitalWrite(led[bank * BANKOFFSET + 1], LOW);
        digitalWrite(rel[bank * BANKOFFSET],     HIGH);
        digitalWrite(rel[bank * BANKOFFSET + 1], HIGH);
        break;
      case 1:
        digitalWrite(led[bank * BANKOFFSET],     HIGH);
        digitalWrite(led[bank * BANKOFFSET + 1], LOW);
        digitalWrite(rel[bank * BANKOFFSET],     LOW);
        digitalWrite(rel[bank * BANKOFFSET + 1], HIGH);
        break;
      case 2:
        digitalWrite(led[bank * BANKOFFSET],     LOW);
        digitalWrite(led[bank * BANKOFFSET + 1], HIGH);
        digitalWrite(rel[bank * BANKOFFSET],     HIGH);
        digitalWrite(rel[bank * BANKOFFSET + 1], LOW);
        break;
      case 3:
        digitalWrite(led[bank * BANKOFFSET],     HIGH);
        digitalWrite(led[bank * BANKOFFSET + 1], HIGH);
        digitalWrite(rel[bank * BANKOFFSET],     LOW);
        digitalWrite(rel[bank * BANKOFFSET + 1], LOW);
        break;
      default:
        break;
    }
  }
}

/* select FX via buttons: */
void manualSelect(
        byte*               fx_state,
        byte*               midi_learn
) {
  int button_state    = 0;
  int input_flags     = 0;
  byte midi_learn_int = *midi_learn;
  byte fx_state_int   = *fx_state;
  byte fxLow_state    = 0x03 & fx_state_int;
  byte fxHigh_state   = 0x03 & (fx_state_int >> BANKOFFSET);

  /* read low FX bank state. */
  button_state = digitalRead(PIN_BUTTON_BKL);
  if (button_state == HIGH) {
    input_flags |= 1; 
  }

  /* read high FX bank state. */
  button_state = digitalRead(PIN_BUTTON_BKH);
  if (button_state == HIGH) {
    input_flags |= (1 << 1); 
  }
  
  /* read midi learn button state. */
  button_state = digitalRead(PIN_BUTTON_LRN);
  if (button_state == HIGH) {
    input_flags |= (1 << 2); 
  }
  
  /*  change due to noise or button pressing. */
  if (g_button_flags_prev != input_flags) {
    g_lastDebounceTime = millis();
  }
  
  /* debounce input. */
  if ((millis() - g_lastDebounceTime) > DEBOUNCEDELAY) {
  
    /* the input has changed. */
    if (input_flags != g_button_flags) {
      g_button_flags = input_flags;

      /* the user gives an input */
      if (input_flags > 0) {
        switch (g_button_flags) {
          case 1:  /* low FX bank selected */
            fxLow_state++;
            fxLow_state    = 0x03 & fxLow_state;
            midi_learn_int = 0;
            break;
          case 2:  /* high FX bank selected */
            fxHigh_state++;
            fxHigh_state   = 0x03 & fxHigh_state;
            midi_learn_int = 0;
            break;
          case 4:  /* MIDI learn toggled */
            midi_learn_int = !midi_learn_int;
            break;
          default:
            break;
        }
  
        /* select FX */
        fx_state_int = 0x0f & ((fxHigh_state << BANKOFFSET) | fxLow_state);
        fxSwitch(fx_state_int);
        
        /* update return values */
        *fx_state   = fx_state_int;
        *midi_learn = midi_learn_int;
      }
    }
  }
  
  /* save current button flags */
  g_button_flags_prev = input_flags;
}

/* select FX via MIDI: */
void midiSelect(
        byte*               fx_state,
        const byte*         prg_table,
        const int           prg_table_length
) {
  byte midi_prg = 0x00;
  
  /* read midi data */
  if (midiRead(&midi_prg, prg_table_length) > 0) {
    
    /* the received MIDI program must be in a valid range */
    if (midi_prg >= 0 && midi_prg < prg_table_length) {

      /* select associated FX preset */
      *fx_state = 0x0f & (prg_table[midi_prg] >> EEPROM_FXPRG_OFFSET);
      fxSwitch(*fx_state);
    }
  }
}

/* MIDI learn: */
void midiLearn(
        byte*               midi_learn,
        const byte          fx_state,
        byte*               prg_table,
        const int           prg_table_length
) {
  int i             = 0;
  int led_toggle    = LOW;
  byte midi_prg     = 0x00;
  byte eeprom_value = 0x00;

  if (*midi_learn == 1) {
    /* midi learn is active. */
    digitalWrite(PIN_LED_LRN, HIGH);
    
    /* read midi data */
    if (midiRead(&midi_prg, prg_table_length) > 0) {

      /* the received MIDI program must be in a valid range */
      if (midi_prg >= 0 && midi_prg < prg_table_length) {
  
        /* create table entry */
        eeprom_value = midi_prg | (0xf0 & (fx_state << EEPROM_FXPRG_OFFSET));

        /* store FX preset to its MIDI program */
        if (eeprom_value != prg_table[midi_prg]) {
          prg_table[midi_prg] = eeprom_value;
          
          /* write patch to EEPROM */
          writeEEPROM(midi_prg,
                      eeprom_value);
          
          /* blink LED to indicate write process */
          for (i = 0; i < 11; i++) {
            digitalWrite(PIN_LED_LRN, led_toggle);
            led_toggle = !led_toggle;
            delay(150);
          }
        }
        
        /* reset MIDI learn state */
        *midi_learn = 0;
      }
    }
  } else {
    digitalWrite(PIN_LED_LRN, LOW);
  }
}

/* MIDI read: */
int midiRead(
        byte*               program,
        const int           prg_table_length
) {
  int retVal = -1;
  byte event = 0x00;
  *program   = 0x00;

  if (Serial.available() > 0) {
    event = Serial.read();

    /* short delay to refresh serial input */
    delay(5);
    
    /* only check for program changes */
    if (0xC0 == event) {
      event = Serial.read();
      
      /* check if received program is valid */
      if (event >= 0 && event < prg_table_length) {
        *program = event;
        retVal   = 1;
      }

      return retVal;
    }
  }
  
  return retVal;
}

/* EEPROM read: */
void readEEPROM(
        byte*               buffer
) {
  byte i = 0;

  for (i = 0; i < EEPROM_NUMBYTES; i++) {
    buffer[i] = EEPROM.read(i + EEPROM_ADDRESS); 
  }
}

/* EEPROM reset: */
void resetEEPROM(
        void
) {
  byte i = 0;
 
  for (i = 0; i < EEPROM_NUMBYTES; i++) {
    EEPROM.write(i + EEPROM_ADDRESS, i);
  }
}

/* EEPROM write: */
void writeEEPROM(
        const int           address,
        const byte          value
) {
  if (address >= 0 && address < EEPROM_NUMBYTES) {
    EEPROM.write(address + EEPROM_ADDRESS, value);
  }
}

/* the start-up testing routine: */
void startUp(
        void
) {
  int i;
  int j;
  int state[5]     = {LOW, LOW, LOW, LOW, LOW};
  const int led[5] = {PIN_LED_FX1, PIN_LED_FX2, PIN_LED_FX3, PIN_LED_FX4, PIN_LED_LRN};
  const int rel[4] = {PIN_RELAIS_FX1, PIN_RELAIS_FX2, PIN_RELAIS_FX3, PIN_RELAIS_FX4};
  
  for (j = 0; j < 4; j++) {
    digitalWrite(rel[j], HIGH);
  }
  
  for (i = 0; i < 5; i++) {
    for (j = 0; j < 5; j++) {
      if (i == j) {
        state[j] = HIGH;
      } else {
        state[j] = LOW;
      }
      digitalWrite(led[j], state[j]);
    }
    delay(100);
  }
  
  for (i = 3; i >= 0; i--) {
    for (j = 0; j < 5; j++) {
      if (i == j) {
        state[j] = HIGH;
      } else {
        state[j] = LOW;
      }
      digitalWrite(led[j], state[j]);
    }
    delay(100);
  }
  
  for (j = 0; j < 5; j++) {
    digitalWrite(led[j], LOW);
  }
  
  delay(100);
  
  for (j = 0; j < 5; j++) {
    digitalWrite(led[j], HIGH);
  }
  for (j = 0; j < 4; j++) {
    digitalWrite(rel[j], LOW);
  }
  
  delay(1000);
  
  for (j = 0; j < 5; j++) {
    digitalWrite(led[j], LOW);
  }
  for (j = 0; j < 4; j++) {
    digitalWrite(rel[j], HIGH);
  }
  
  delay(100);
}
