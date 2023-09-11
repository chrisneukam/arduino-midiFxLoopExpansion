# arduino-midiFxLoopExpansion
[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

MIDI driven true bypass FX loop expansion for guitar amplifiers based on Arduino.

## Overview
The `arduino-midiFxLoopExpansion` is a MIDI controlled expansion for the effects loop of a guitar amplifier. By the combination of the ATmega microcontroller and the controlled hardware up to 4 guitar effects can be switched into the signal path. Each of the 4 effect paths supports true bypass.

With the right housing the midiFxLoopExpansion is ready to rock!

### Features
- 4 individual effect paths
- true-bypass
- MIDI-in and MIDI-through
- MIDI-learn for up to 16 storable presets
- programming of the effect channels via two buttons (high/low bank)
- LED display for the switched effect channels
- power supply via standard 9V power supply for guitar effects

### Tech Stack
- Arduino promini compatible board with ATmega 328p microcontroller (5V, 16 MHz)
- [KiCad](https://www.kicad.org/) v7.0.7 used to create the circuit diagrams
- [BlackBoard Circuit Designer](https://github.com/mpue/blackboard#blackboard-circuit-designer) used for creating the breadboard design
- [Arduino IDE](https://www.arduino.cc/en/software) used for developing the Arduino sketch

### Required developer knowledge
- Basics of circuit technology to understand electronic circuitry
- Basics of software development in C/C++ for the development of the Arduino code
- soldering skills to build the hardware circuit

## Product realization
This chapter roughly describes the procedure to implement the project. Detailed instructions will follow.

### Softare deployment to the Arduino promini
First you have to load the Arduino sketch in the Arduino IDE, then the software will be flashed to the Arduino promini.

### Hardware soldering
The electronic circuit is optimized for a bread board of size 160mm x 45mm. The following pictures show the layout. Accordingly, the circuit can be soldered.

#### front view
![midiFxLoopExpansion_160x45 front](blackboard/midiFxLoopExpansion_160x45.png "midiFxLoopExpansion_160x45 front")

#### back view
![midiFxLoopExpansion_160x45 back](blackboard/midiFxLoopExpansion_160x45_back.png "midiFxLoopExpansion_160x45 back")

#### schematics
![hustlin_erd](kicad/midiFxLoopExpansion.png)

## List of components
|#  |Reference         |Qty|Value      |Footprint|
|---|------------------|---|-----------|---------|
|1  |C1, C4, C5        |3  |100n       |         |
|2  |C2, C3            |2  |220u       |         |
|3  |D1                |1  |1N5818     |         |
|4  |D2                |1  |1N4148     |         |
|5  |D3                |1  |LED-Red    |         |
|6  |D4, D5, D6, D7    |4  |LED-Blue   |         |
|7  |D8, D9, D10, D11  |4  |1N4004     |         |
|8  |IC1               |1  |ARDUPROMINI|         |
|9  |K1, K2, K3, K4    |4  |FRT5       |         |
|10 |P1                |1  |POWER_CONN |         |
|11 |P2                |1  |MIDI Input |         |
|12 |P3                |1  |MIDI Output|         |
|13 |Q1, Q2, Q3, Q4    |4  |BC547      |TO-92    |
|14 |R1, R2, R4        |3  |220        |         |
|15 |R3                |1  |1K         |         |
|16 |R5                |1  |100        |         |
|17 |R6                |1  |180        |         |
|18 |R7                |1  |150        |         |
|19 |R8, R18, R21, R24 |4  |2.2K       |         |
|20 |R9, R20, R23, R27 |4  |1M         |         |
|21 |R10, R11, R12     |3  |100K       |         |
|22 |R13               |1  |51         |         |
|23 |R14, R15, R16, R17|4  |91         |         |
|24 |R19, R22, R26, R28|4  |2.2M       |         |
|25 |R25               |1  |270        |         |
|26 |U1                |1  |PC900V     |         |

See also [midiFxLoopExpansion.csv](kicad/midiFxLoopExpansion.csv).

## License
`arduino-midiFxLoopExpansion` is free and open-source software licensed under the Apache 2.0 License.
