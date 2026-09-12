# Business Card - Floppy Bird

I wanted my business card to do more than sit in someone's wallet, so I built
one with a custom PCB, 128 RGB LEDs, and a playable game called **Floppy Bird**.
It brings together PCB design, embedded C, real-time game logic, all displayed on
a 16x8 custom led mattrix display.

## What it does

Floppy Bird is controlled with a single physical button. Pressing it makes the
bird flap while two-pixel-wide pipes move across the display. Passing a pipe
earns a point, and the game gradually speeds up across ten difficulty levels.

The player gets three lives. Once all three lives are gone, the final score
appears.

## What I built

- A custom KiCad schematic and PCB for an STM32C031 and 8x16 addressable RGB
  LED matrix.
- A framebuffer-based LED driver that converts RGB values into WS2812 data and
  sends each frame using SPI with DMA.
- Non-blocking game logic covering bird physics, random pipe gaps, collisions,
  lives, scoring, difficulty progression, and game-over handling.
- Interrupt-driven button input with the actual game update kept in the main
  loop, keeping the interrupt handler short.
- Low-power idle time using the Cortex-M0+ wait-for-interrupt instruction.
- Production files including Gerbers, a bill of materials, and component
  placement data.

## Hardware

At the centre of the card is an STM32C031, a small Cortex-M0+ microcontroller
that handles the game, button input, and LED data. The display is made from 128
individually addressable RGB LEDs arranged as 16 columns by 8 rows. A single
physical button provides the game input, keeping the interface simple and making
the card immediately playable.

The card is powered by two AAA batteries. A boost converter raises the battery
voltage to 5 V for the LED matrix, while an LDO steps that rail down to 3.3 V for
the microcontroller. Because the STM32 and LEDs operate at different logic
levels, a level converter shifts the 3.3 V SPI data signal up to 5 V before it
reaches the LEDs. The button input is debounced in hardware with an RC filter so
a single press produces a clean, reliable transition.

The LEDs share one serial data chain and are updated from an SPI peripheral using
DMA. This lets the microcontroller transfer a complete frame in the background
instead of spending all of its time manually generating the LED waveform.

Power consumption was an important part of the design because 128 RGB LEDs can
draw a lot of current at full output. The firmware keeps brightness deliberately
low, and the hardware includes local decoupling for the LED supply.

While the led matrix is cool, it is quite power hungry. Drawing 3/4W, the display
will run for 4-6 hours from 2x AAA batteries.

PCB is 2 layer double sided. The matrix is connected in alternate directions so
as to simpliy routing and avoid unnecessary trace lengths.

## A few interesting details

The LEDs are routed in a serpentine pattern on the PCB. The driver translates
normal `(x, y)` coordinates into the physical LED order, so the game code does
not need to know how the traces are laid out.

The bird uses fixed-point physics instead of floating point. This makes its
movement smoother than jumping directly between the eight rows while staying
lightweight for the Cortex-M0+ microcontroller. Fractional accumulators give
fine control over gravity and pipe speed without floating-point overhead.

The firmware is split into small modules: game logic and rendering are separate
from the LED driver, and adjustable gameplay values live in one configuration
header.

The leds are very bright, so to same battery and eyesight, they are set to very
low brightness.

## Tools and technologies

- Embedded C
- STM32C031 microcontroller
- STM32CubeMX and STM32CubeIDE
- GPIO interrupts, SPI, DMA, and low-power operation
- WS2812-compatible RGB LEDs
- Fixed-point arithmetic and real-time state machines
- KiCad schematic capture and PCB layout
- Design-for-manufacture files for JLCPCB

## Repository contents

The `pcb` folder contains the KiCad design and manufacturing files. The STM32
firmware is in `microcontroller/business card`.
