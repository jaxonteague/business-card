/**
 * @file game.c
 * @brief Implements bird physics, background animation, and frame rendering.
 *
 * Created on: 26 July 2026
 * Author: Jaxon Teague
 */

#include "game.h"
#include "led.h"
#include "config.h"
#include "stm32c0xx_hal.h" /* HAL_GetTick() */

#include <stdbool.h>

/*
 * The bird position and velocity use four fractional bits. This produces
 * smoother motion than changing directly between the matrix's eight rows.
 */
#define POSITION_SCALE             16
#define BIRD_MIN_POSITION          0
#define BIRD_MAX_POSITION          ((int16_t)((LED_ROWS - 1U) * POSITION_SCALE))

static void Update_Game_State(void);
static void Update_Background_Breathing(void);
static void Render_Game(void);

typedef struct
{
    int16_t bird_position;          /* Vertical position in fixed-point units. */
    int16_t bird_velocity;          /* Signed vertical speed per game update. */

    int8_t brightness_step;         /* +1 while brightening, -1 while dimming. */
    uint8_t background_brightness;  /* Current background brightness scale. */
    uint8_t breathing_counter;      /* Divides the game rate for a slower fade. */
} Game_State_t;

/* Complete mutable state for the current game session. */
static Game_State_t game;

/* Interrupt-to-main-loop flag; volatile because the EXTI callback writes it. */
static volatile bool flap_requested;

/* HAL tick when the most recent frame was accepted by the LED driver. */
static uint32_t last_update_time;

/**
 * @brief Reset game state, render the initial frame, and start LED output.
 */
void Game_Init(void)
{
    game.bird_position = PLAYER_START_Y * POSITION_SCALE;
    game.bird_velocity = 0;
    game.background_brightness = BACKGROUND_BRIGHTNESS_MIN;
    game.brightness_step = 1;
    game.breathing_counter = 0;
    flap_requested = false;

    Render_Game();

    while(!LED_Transmit())
    {
        /* Startup must display the initial frame before normal updates begin. */
    }

    last_update_time = HAL_GetTick();
}

/**
 * @brief Queue one flap for processing during the next game update.
 *
 * This function is safe to call from the GPIO interrupt callback because it
 * only sets a flag; physics and rendering remain in the main-loop context.
 */
void Game_ButtonPressed(void)
{
    /*
     * A boolean is sufficient because multiple presses before the next 16 ms
     * game update should produce one flap.
     */
    flap_requested = true;
}

/**
 * @brief Apply queued input, advance bird physics, and update the background.
 */
static void Update_Game_State(void)
{
    /*
     * The interrupt only sets this flag; all physics remains in the main loop.
     * Clear it after consuming the pending flap request.
     */
    if(flap_requested)
    {
        flap_requested = false;
        game.bird_velocity = BIRD_FLAP_VELOCITY;
    }

    /* Positive velocity is downward; gravity therefore increases velocity. */
    game.bird_velocity += BIRD_GRAVITY;

    if(game.bird_velocity > BIRD_TERMINAL_VELOCITY)
    {
        game.bird_velocity = BIRD_TERMINAL_VELOCITY;
    }

    game.bird_position += game.bird_velocity;

    /* Clamp the fixed-point position so rendering cannot leave the matrix. */
    if(game.bird_position < BIRD_MIN_POSITION)
    {
        game.bird_position = BIRD_MIN_POSITION;
        game.bird_velocity = 0;
    }
    else if(game.bird_position > BIRD_MAX_POSITION)
    {
        game.bird_position = BIRD_MAX_POSITION;
        game.bird_velocity = 0;
    }

    Update_Background_Breathing();
}

/**
 * @brief Advance the background brightness breathing effect by one game tick.
 */
static void Update_Background_Breathing(void)
{
    /*
     * Change brightness less often than the physics update so the purple
     * background fades slowly without affecting the bird brightness.
     */
    game.breathing_counter++;

    if(game.breathing_counter < BACKGROUND_BREATHING_DIVIDER)
    {
        return;
    }

    game.breathing_counter = 0;

    /* Reverse direction at each limit to produce a triangular fade waveform. */
    if(game.brightness_step > 0)
    {
        if(game.background_brightness >= BACKGROUND_BRIGHTNESS_MAX)
        {
            game.background_brightness = BACKGROUND_BRIGHTNESS_MAX;
            game.brightness_step = -1;
        }
        else
        {
            game.background_brightness++;
        }
    }
    else
    {
        if(game.background_brightness <= BACKGROUND_BRIGHTNESS_MIN)
        {
            game.background_brightness = BACKGROUND_BRIGHTNESS_MIN;
            game.brightness_step = 1;
        }
        else
        {
            game.background_brightness--;
        }
    }
}

/**
 * @brief Draw the background and bird into the LED framebuffer.
 */
static void Render_Game(void)
{
    /* Apply brightness separately so the configured RGB hue remains intact. */
    LED_Fill(BACKGROUND_RED,
             BACKGROUND_GREEN,
             BACKGROUND_BLUE,
             game.background_brightness);

    /* Adding half the scale rounds fixed-point position to the nearest row. */
    uint8_t bird_y =
        (uint8_t)((game.bird_position + (POSITION_SCALE / 2)) / POSITION_SCALE);

    LED_DrawPixel(PLAYER_START_X,
                  bird_y,
                  BIRD_RED,
                  BIRD_GREEN,
                  BIRD_BLUE,
                  BIRD_BRIGHTNESS);
}

/**
 * @brief Run one non-blocking, frame-timed game update when the LEDs are ready.
 */
void Game_Task(void)
{
    /* Snapshot the tick so every timing decision in this pass is consistent. */
    uint32_t current_time = HAL_GetTick();

    /* Unsigned subtraction remains correct when the 32-bit HAL tick wraps. */
    if((current_time - last_update_time) < GAME_UPDATE_PERIOD_MS)
    {
        return;
    }

    /*
     * Do not advance physics while the previous framebuffer is still being
     * transmitted. This keeps motion tied to frames visible on the LEDs.
     */
    if(!LED_IsReady())
    {
        return;
    }

    Update_Game_State();
    Render_Game();

    if(LED_Transmit())
    {
        /* Advance the schedule only after the driver accepts the new frame. */
        last_update_time = current_time;
    }
}
