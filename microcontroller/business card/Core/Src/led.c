/*
 * led.c
 *
 *  Created on: 25 July 2026
 *      Author: Jaxon Teague
 */

#include "led.h"
#include "spi.h"

#include <stdbool.h>
#include <string.h>

//#define RESET_BYTES 32U

/*
 * CubeMX generated peripheral handles.
 */
extern SPI_HandleTypeDef hspi1;

/*
 * LED framebuffer.
 *
 * Stores the desired display state.
 *
 * Data is stored internally in physical
 * WS2812 chain order, but accessed using
 * display X/Y coordinates.
 */
typedef struct
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t brightness;

} LED_t;

static LED_t led[LED_COUNT];

/*
 * SPI encoded buffer.
 *
 * WS2812 bit encoding:
 *
 * 0 = 100
 * 1 = 110
 *
 * Buffer size:
 *
 * LED_COUNT
 * × 24 bits
 * × 3 SPI bits
 * ÷ 8
 */
static uint8_t spi_buffer[
    (LED_COUNT * 24U * LED_SPI_BITS_PER_LED_BIT) / 8U +
    LED_RESET_BYTES
];


/*
 * Current bit position while encoding.
 */
static uint16_t encode_position;

/*
 * Earliest time that another LED transmission may begin.
 *
 * Uses the 1 ms SysTick timer.
 */
static volatile bool led_transmitting = false;
static volatile uint32_t led_next_transmit_time = 0U;

/*
 * Convert display coordinates into WS2812 chain index.
 *
 * Coordinates:
 *
 * x = column (0 = left)
 * y = row    (0 = top)
 *
 * PCB routing:
 *
 * y=0 left  -> right
 * y=1 right -> left
 * y=2 left  -> right
 * y=3 right -> left
 */
static uint16_t XY_To_Chain_Index(uint8_t x, uint8_t y)
{
    if((y & 1U) == 0U)
    {
        return (y * LED_COLUMNS) + x;
    }

    return (y * LED_COLUMNS) +
           (LED_COLUMNS - 1U - x);
}

/*
 * Encode one byte into WS2812 SPI format.
 */
static void EncodeByte(uint8_t value)
{
    for(int8_t bit = 7; bit >= 0; bit--)
    {
        uint8_t encoded;

        if(value & (1U << bit))
        {
            encoded = LED_SPI_ONE;
        }
        else
        {
            encoded = LED_SPI_ZERO;
        }

        for(int8_t i = (LED_SPI_BITS_PER_LED_BIT - 1); i >= 0; i--)
        {
            if(encoded & (1U << i))
            {
                spi_buffer[encode_position / 8U] |=
                    (1U << (7U - (encode_position % 8U)));
            }

            encode_position++;
        }
    }
}

/*
 * Initialise LED driver.
 */
void LED_Init(void)
{
    for(uint16_t i = 0; i < LED_COUNT; i++)
    {
        led[i].r = 0;
        led[i].g = 0;
        led[i].b = 0;
        led[i].brightness = 255;
    }

}

/*
 * Draw a pixel into the LED framebuffer.
 *
 * Converts the display X/Y coordinate into
 * the physical WS2812 chain index and stores
 * the colour information.
 *
 * This function does not transmit data.
 * Call LED_Transmit() to update the LEDs.
 */
void LED_DrawPixel(uint8_t x,
                   uint8_t y,
                   uint8_t red,
                   uint8_t green,
                   uint8_t blue,
                   uint8_t brightness)
{
    uint16_t led_index;

    if((x >= LED_COLUMNS) || (y >= LED_ROWS))
    {
        return;
    }

    if((y & 1U) == 0U)
    {
        /* Even row: left to right */
        led_index = ((uint16_t)y * LED_COLUMNS) + x;
    }
    else
    {
        /* Odd row: right to left */
        led_index = ((uint16_t)y * LED_COLUMNS)
                  + (LED_COLUMNS - 1U - x);
    }

    led[led_index].r = red;
    led[led_index].g = green;
    led[led_index].b = blue;
    led[led_index].brightness = brightness;
}

/*
 * Return true when no DMA transfer is active and the
 * WS2812 reset delay has elapsed.
 */
bool LED_IsReady(void)
{
    if(led_transmitting)
    {
        return false;
    }

    return ((int32_t)(HAL_GetTick() - led_next_transmit_time) >= 0);
}

/*
 * Transmit the LED framebuffer.
 *
 * Converts the stored RGB values into the
 * WS2812 SPI waveform format and starts
 * a DMA transfer.
 *
 * The physical LEDs are updated after the
 * WS2812 reset/latch period.
 */
bool LED_Transmit(void)
{
    if(!LED_IsReady())
    {
        return false;
    }

    /*
     * Reserve the SPI buffer before clearing or encoding it.
     *
     * This prevents another call from modifying the buffer
     * while DMA is transmitting the current frame.
     */
    led_transmitting = true;

    /*
     * Clear previous encoded data, including reset bytes.
     */
    for(uint16_t i = 0U; i < sizeof(spi_buffer); i++)
    {
        spi_buffer[i] = 0U;
    }

    encode_position = 0U;

    for(uint16_t i = 0U; i < LED_COUNT; i++)
    {
        uint8_t g =
            ((uint16_t)led[i].g * led[i].brightness) / 255U;

        uint8_t r =
            ((uint16_t)led[i].r * led[i].brightness) / 255U;

        uint8_t b =
            ((uint16_t)led[i].b * led[i].brightness) / 255U;

        EncodeByte(g);
        EncodeByte(r);
        EncodeByte(b);
    }

    HAL_StatusTypeDef status =
        HAL_SPI_Transmit_DMA(&hspi1,
                             spi_buffer,
                             sizeof(spi_buffer));

    if(status != HAL_OK)
    {
        /*
         * DMA did not start, so release the driver.
         */
        led_transmitting = false;
        return false;
    }

    return true;
}
/*
 * Clear the LED framebuffer.
 */
void LED_Clear(void)
{
    for(uint16_t i = 0; i < LED_COUNT; i++)
    {
        led[i].r = 0;
        led[i].g = 0;
        led[i].b = 0;
        led[i].brightness = 0;
    }
}

/*
 * Fill the LED framebuffer with one colour.
 */
void LED_Fill(uint8_t r,
              uint8_t g,
              uint8_t b,
              uint8_t brightness)
{
    for(uint16_t i = 0; i < LED_COUNT; i++)
    {
        led[i].r = r;
        led[i].g = g;
        led[i].b = b;
        led[i].brightness = brightness;
    }
}


/*
 * SPI DMA transmission complete callback.
 */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if(hspi == &hspi1)
    {
        /*
         * The encoded buffer is no longer being read by DMA.
         */
        led_transmitting = false;

        /*
         * Earliest time at which another frame may start.
         */
        led_next_transmit_time =
            HAL_GetTick() + LED_RESET_DELAY_MS;
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    volatile uint32_t err = HAL_SPI_GetError(hspi);

    __BKPT(0);
}
