# Business Card - LED Matrix Game

I designed this electronic business card as a compact demonstration of embedded
systems development, PCB design, and product-focused engineering. Rather than a
static printed card, it combines custom hardware with an interactive game on an
8-by-16 RGB LED matrix.

## Project overview

The card is built around an STM32C031 microcontroller and a custom PCB designed
in KiCad. Its firmware runs a small side-scrolling game controlled by a physical
button: the player guides a bird through moving pipes, earns points for successful
passes, and receives immediate visual feedback after a collision.

The project took the design from schematic capture and PCB layout through
manufacturing outputs and embedded firmware. This repository records both the
hardware and software sides of that process.

## Engineering highlights

- Designed the schematic and PCB for a dense 128-pixel addressable LED display.
- Developed a framebuffer-based LED driver that converts RGB pixel data into the
  WS2812 signalling format and transmits it using SPI with DMA.
- Implemented non-blocking game logic with fixed-point bird physics, collision
  detection, procedural pipe gaps, scoring, lives, and game-over states.
- Created a ten-level difficulty system that gradually increases pipe speed and
  changes the background colour to communicate progression.
- Added a compact 5-by-7 numeric font so the final score can be rendered directly
  on the constrained 16-by-8 display.
- Used interrupt-driven input and cooperative frame updates to keep interrupt
  handlers short and gameplay responsive.
- Put the Cortex-M0+ core into sleep between interrupts to reduce processor power
  consumption while retaining timer, button, and DMA responsiveness.
- Tuned LED intensity for practical power consumption and comfortable viewing,
  including a minimum non-zero background brightness.

## Design decisions

The display is physically wired in a serpentine arrangement, so the LED driver
maps logical `(x, y)` coordinates to physical chain positions. This keeps the
game and rendering code independent of PCB routing.

Game motion uses fixed-point arithmetic rather than floating point. That provides
smoother movement than whole-row updates while remaining appropriate for a small
Cortex-M0+ microcontroller. Fractional accumulators also provide fine control of
gravity and pipe speed without adding floating-point overhead.

Rendering and game state are separated from the hardware driver. Adjustable
timing, physics, colour, and brightness values are centralized in a configuration
header, while implementation-specific state remains private to the game module.
CubeMX-generated peripheral code is kept separate from application-owned logic.

## Gameplay details

The player starts with three lives. Hitting a pipe flashes the obstacle, changes
the bird's colour, and prevents that pipe from being counted toward the score.
Touching the top or bottom of the display only clips the bird to the boundary and
does not cost a life. After the final collision, the score remains visible for at
least three seconds and a fresh button press starts a new game.

## Technologies and skills demonstrated

- Embedded C
- STM32CubeMX and STM32CubeIDE
- STM32 HAL, GPIO interrupts, SPI, DMA, and low-power wait-for-interrupt operation
- Real-time state machines and fixed-point arithmetic
- Addressable RGB LED control and framebuffer rendering
- KiCad schematic capture and PCB layout
- Design-for-manufacture outputs including Gerbers, BOM, and placement data
- Hardware-aware debugging, power management, and iterative user-experience tuning

## Repository contents

The `pcb` directory contains the KiCad schematic, PCB design, custom footprint,
and JLCPCB manufacturing outputs. The STM32 firmware is in
`microcontroller/business card`.
