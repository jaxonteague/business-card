/**
 * @file led.c
 * @brief Implements the framebuffer and DMA-driven WS2812 SPI encoder.
 *
 * Created on: 25 July 2026
 * Author: Jaxon Teague
 */

#include "led.h"
#include "spi.h"

#include <stdbool.h>
#include <string.h>

/* One logical pixel before brightness scaling and wire-format encoding. */
typedef struct
{
    uint8_t r;           /* Unscaled red intensity. */
    uint8_t g;           /* Unscaled green intensity. */
    uint8_t b;           /* Unscaled blue intensity. */
    uint8_t brightness;  /* Per-pixel scale applied during transmission. */

} LED_t;

/* Framebuffer stored in physical WS2812 chain order. */
static LED_t led[LED_COUNT];

/*
 * DMA source buffer containing the complete encoded WS2812 frame.
 *
 * WS2812 bit encoding:
 *
 *   logical 0 -> 1000
 *   logical 1 -> 1110
 *
 * Buffer size is LED_COUNT x 24 LED bits x
 * LED_SPI_BITS_PER_LED_BIT / 8. LED_RESET_BYTES zero
 * bytes are appended to provide the low latch interval.
 */
static uint8_t spi_buffer[
    (LED_COUNT * 24U * LED_SPI_BITS_PER_LED_BIT) / 8U +
    LED_RESET_BYTES
];


/* Next SPI bit to write into spi_buffer while constructing a frame. */
static uint16_t encode_position;

/* Set while DMA owns spi_buffer; changed by both main and interrupt contexts. */
static volatile bool led_transmitting = false;

/* First HAL tick at which the WS2812 reset interval has fully elapsed. */
static volatile uint32_t led_next_transmit_time = 0U;

/**
 * @brief Convert matrix coordinates to the physical serpentine chain index.
 *
 * Even rows run left-to-right and odd rows run right-to-left on the PCB.
 *
 * @param x Zero-based column measured from the left.
 * @param y Zero-based row measured from the top.
 * @return Zero-based LED index in physical chain order.
 */
static uint16_t XY_To_Chain_Index(uint8_t x, uint8_t y)
{
    /* Even rows follow coordinate order; odd rows reverse the column index. */
    if((y & 1U) == 0U)
    {
        return (y * LED_COLUMNS) + x;
    }

    return (y * LED_COLUMNS) +
           (LED_COLUMNS - 1U - x);
}

/**
 * @brief Append one byte to the SPI buffer using four SPI bits per WS2812 bit.
 *
 * @param value Byte to encode, most-significant bit first.
 */
static void EncodeByte(uint8_t value)
{
    /* WS2812 sends the most-significant bit of each colour byte first. */
    for(int8_t bit = 7; bit >= 0; bit--)
    {
        /* Four-bit SPI symbol representing the current logical WS2812 bit. */
        uint8_t encoded;

        if(value & (1U << bit))
        {
            encoded = LED_SPI_ONE;
        }
        else
        {
            encoded = LED_SPI_ZERO;
        }

        /* Pack the symbol into spi_buffer without assuming byte alignment. */
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

/**
 * @brief Initialize every framebuffer pixel to off at full brightness scale.
 */
void LED_Init(void)
{
    /* Start with every pixel off at full per-pixel scale. */
    for(uint16_t i = 0; i < LED_COUNT; i++)
    {
        led[i].r = 0;
        led[i].g = 0;
        led[i].b = 0;
        led[i].brightness = 255;
    }

}

/**
 * @brief Store one pixel in the framebuffer without starting transmission.
 *
 * @param x Zero-based matrix column.
 * @param y Zero-based matrix row.
 * @param red Unscaled red intensity.
 * @param green Unscaled green intensity.
 * @param blue Unscaled blue intensity.
 * @param brightness Per-pixel brightness scale from 0 to 255.
 */
void LED_DrawPixel(uint8_t x,
                   uint8_t y,
                   uint8_t red,
                   uint8_t green,
                   uint8_t blue,
                   uint8_t brightness)
{
    if((x >= LED_COLUMNS) || (y >= LED_ROWS))
    {
        return;
    }

    /* Translate logical coordinates so callers need not know the PCB routing. */
    uint16_t led_index = XY_To_Chain_Index(x, y);

    led[led_index].r = red;
    led[led_index].g = green;
    led[led_index].b = blue;
    led[led_index].brightness = brightness;
}

/**
 * @brief Report whether a new frame can safely reuse the SPI buffer.
 *
 * @return true when DMA is idle and the reset/latch interval has elapsed.
 */
bool LED_IsReady(void)
{
    if(led_transmitting)
    {
        return false;
    }

    /* Signed tick comparison remains correct across wrap for short delays. */
    return ((int32_t)(HAL_GetTick() - led_next_transmit_time) >= 0);
}

/**
 * @brief Encode the framebuffer and begin a non-blocking SPI DMA transfer.
 *
 * @return true if DMA accepted the frame, otherwise false.
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
    memset(spi_buffer, 0, sizeof(spi_buffer));

    encode_position = 0U;

    for(uint16_t i = 0U; i < LED_COUNT; i++)
    {
        /*
         * Apply brightness only while encoding. The framebuffer retains the
         * original colour values, avoiding cumulative rounding loss.
         */
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
/**
 * @brief Set every framebuffer pixel to black without transmitting it.
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

/**
 * @brief Fill the framebuffer with one colour and brightness value.
 *
 * @param r Red intensity applied to every pixel.
 * @param g Green intensity applied to every pixel.
 * @param b Blue intensity applied to every pixel.
 * @param brightness Brightness scale applied to every pixel.
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

/*
 * Handle an SPI transmission error by stopping in the debugger.
 */
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    /*
     * Development-time trap. Production firmware should record the error,
     * release the driver state, and recover or reset instead.
     */
    volatile uint32_t err = HAL_SPI_GetError(hspi);

    __BKPT(0);
}
