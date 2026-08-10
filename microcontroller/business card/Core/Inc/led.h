/**
 * @file led.h
 * @brief Public interface for the LED framebuffer and WS2812 driver.
 *
 * Created on: 25 July 2026
 * Author: Jaxon Teague
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

/*
 * At a 3 MHz SPI clock, 128 zero bytes hold MOSI low for about
 * 341 µs, exceeding the device's reset/latch minimum.
 */
#define LED_RESET_BYTES    128U

/**
 * @brief Initialize the framebuffer with all pixels off.
 */
void LED_Init(void);

/**
 * @brief Store one pixel in the framebuffer without transmitting it.
 *
 * @param x Column in the range 0 to LED_COLUMNS - 1.
 * @param y Row in the range 0 to LED_ROWS - 1.
 * @param r Red intensity from 0 to 255.
 * @param g Green intensity from 0 to 255.
 * @param b Blue intensity from 0 to 255.
 * @param brightness Per-pixel brightness scale from 0 to 255.
 */
void LED_DrawPixel(uint8_t x,
               uint8_t y,
               uint8_t r,
               uint8_t g,
               uint8_t b,
               uint8_t brightness);


/**
 * @brief Encode the framebuffer and start a non-blocking SPI DMA transfer.
 *
 * Converts the stored RGB values into the WS2812 SPI waveform format and
 * starts a DMA transfer. The physical LEDs update after the reset/latch period.
 *
 * @return true if DMA accepted the frame, otherwise false.
 */
bool LED_Transmit(void);

/**
 * @brief Check whether the driver can accept another frame.
 *
 * @return true when DMA is idle and the reset/latch interval has elapsed.
 */
bool LED_IsReady(void);


/**
 * @brief Set every framebuffer pixel to black without transmitting it.
 */
void LED_Clear(void);


/**
 * @brief Fill the framebuffer with one colour without transmitting it.
 *
 * @param r Red intensity from 0 to 255.
 * @param g Green intensity from 0 to 255.
 * @param b Blue intensity from 0 to 255.
 * @param brightness Per-pixel brightness scale from 0 to 255.
 */
void LED_Fill(uint8_t r,
              uint8_t g,
              uint8_t b,
              uint8_t brightness);

#endif /* INC_LED_H_ */
