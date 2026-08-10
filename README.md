# LED Business Card

An electronic business card built around an STM32C031 microcontroller and an
8-by-16 addressable RGB LED matrix. The repository contains the KiCad hardware
design, manufacturing files, and STM32 firmware for a button-controlled game.

## Game

The firmware runs a compact side-scrolling game on the LED matrix:

- Press the button to flap and guide the bird through the pipe gaps.
- The bird is clipped to the top and bottom rows without losing a life.
- The player starts with three lives, and the bird changes colour after each
  collision.
- A collided pipe flashes, is ignored for scoring, and the game continues.
- Each pipe passed successfully adds one point.
- Pipe speed increases across ten difficulty levels. The dim background changes
  between blue and purple to indicate the current level.
- When all lives are lost, the score is shown in large white digits for at least
  three seconds. A new button press then starts another game.

All pipes are two LEDs wide. LED brightness is deliberately limited to reduce
power consumption and eye strain.

## Firmware

The firmware targets an **STM32C031G6UX** and was generated with STM32CubeMX for
use in STM32CubeIDE. Application code is separated from generated peripheral
initialization:

- `Core/Src/game.c` contains game state, physics, collision detection, scoring,
  difficulty progression, and rendering.
- `Core/Inc/config.h` contains the adjustable gameplay, timing, colour, and
  brightness settings.
- `Core/Src/led.c` drives the LED matrix by encoding WS2812 data for SPI DMA.
- `Core/Src/main.c` initializes the application, services the game task, and
  sleeps between interrupts to reduce processor power consumption.

The LED framebuffer is transmitted without blocking normal game updates. The
display uses serpentine row mapping to match the physical PCB routing.

### Building

1. Open STM32CubeIDE.
2. Import `microcontroller/business card` as an existing project.
3. Select the Debug configuration and build the project.
4. Program the board with an ST-LINK-compatible debugger.

The CubeMX configuration is stored in
`microcontroller/business card/business card.ioc`. Application changes should
remain inside CubeMX user-code regions or in application-owned files so code
regeneration does not overwrite them.

## Hardware

The root KiCad files contain the schematic and PCB layout. Manufacturing outputs
are under `jlcpcb/`, including Gerbers, drill files, bill of materials, and
component placement data.

## Repository layout

```text
.
|-- business card.kicad_pcb       PCB layout
|-- business card.kicad_sch       Main schematic
|-- datasheets/                   Component documentation
|-- jlcpcb/                       PCB manufacturing outputs
`-- microcontroller/business card STM32CubeIDE firmware project
```
