/*
 * led.h
 *
 *LED framebuffer and WS2812 driver interface
 *
 *  Created on: 25 July 2026
 *      Author: Jaxon Teague
 */

#ifndef INC_LED_H_
#define INC_LED_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * LED matrix dimensions.
 *
 * Physical arrangement:
 *
 * 8 rows
 * 16 columns
 *
 * Coordinate system:
 *
 * x = column (0-15)
 * y = row    (0-7)
 */
#define LED_ROWS           8U
#define LED_COLUMNS        16U
#define LED_COUNT          (LED_ROWS * LED_COLUMNS)

/*
 * WS2812 SPI encoding.
 *
 * SPI clock = 3 MHz
 *
 * 1 SPI bit = 333 ns
 *
 * WS2812 '0' = 1000
 * WS2812 '1' = 1110
 */
#define LED_SPI_BITS_PER_LED_BIT    4U

#define LED_SPI_ZERO                0b1000
#define LED_SPI_ONE                 0b1110

/*
 * WS2812 minimum reset time.
 *
 * The XL-1615RGBC-WS2812 requires the data line to remain
 * LOW for at least 200 µs between frames.
 *
 * Using the 1 ms SysTick, a delay of one tick provides
 * ample timing margin.
 */
#define LED_RESET_DELAY_MS    1U

#define LED_RESET_BYTES    128U

/*
 * Initialise LED driver.
 *
 * Clears the framebuffer.
 * Does not transmit data.
 */
void LED_Init(void);

/*
 * Draw a pixel into the LED framebuffer.
 *
 * The framebuffer uses an X/Y coordinate
 * system matching the physical LED matrix.
 *
 * Coordinates:
 *
 * x:
 *   Column number (0-15)
 *
 * y:
 *   Row number (0-7)
 *
 * Colour:
 *
 * r:
 *   Red intensity (0-255)
 *
 * g:
 *   Green intensity (0-255)
 *
 * b:
 *   Blue intensity (0-255)
 *
 * brightness:
 *   Per-pixel brightness scaling (0-255)
 *
 * The pixel is not updated on the physical
 * LEDs until LED_Transmit() is called.
 */
void LED_DrawPixel(uint8_t x,
               uint8_t y,
               uint8_t r,
               uint8_t g,
               uint8_t b,
               uint8_t brightness);


/*
 * Transmit the LED framebuffer.
 *
 * Converts the stored RGB values into the
 * WS2812 SPI waveform format and starts a
 * DMA transfer.
 *
 * The physical LEDs update after the
 * WS2812 reset/latch period.
 */
bool LED_Transmit(void);


/*
 * Clear the entire LED framebuffer.
 *
 * Sets all pixels to black.
 *
 * Does not transmit data.
 */
void LED_Clear(void);


/*
 * Fill the entire LED framebuffer
 * with one colour.
 *
 * Does not transmit data.
 */
void LED_Fill(uint8_t r,
              uint8_t g,
              uint8_t b,
              uint8_t brightness);

#endif
