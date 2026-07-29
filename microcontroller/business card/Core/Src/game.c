/*
 * game.c
 *
 * Bird movement and display rendering.
 *
 *  Created on: 26 July 2026
 *      Author: Jaxon Teague
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
    int16_t bird_position;
    int16_t bird_velocity;

    int8_t brightness_step;
    uint8_t background_brightness;
    uint8_t breathing_counter;
} Game_State_t;

static Game_State_t game;

/* Set by the EXTI callback and consumed by Game_Task(). */
static volatile bool flap_requested;

/* SysTick value at which the latest frame was accepted for transmission. */
static uint32_t last_update_time;

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
        /* DMA should be idle at startup; wait if the driver is not ready yet. */
    }

    last_update_time = HAL_GetTick();
}

void Game_ButtonPressed(void)
{
    /*
     * Keep interrupt work short. A boolean is sufficient because multiple
     * presses before the next 16 ms game update should produce one flap.
     */
    flap_requested = true;
}

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

    /* Positive velocity moves down the matrix; gravity accelerates downward. */
    game.bird_velocity += BIRD_GRAVITY;

    if(game.bird_velocity > BIRD_TERMINAL_VELOCITY)
    {
        game.bird_velocity = BIRD_TERMINAL_VELOCITY;
    }

    game.bird_position += game.bird_velocity;

    /*
     * Clamp at the display boundaries for this early game stage. Collision
     * and game-over handling can replace these clamps when pipes are added.
     */
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

static void Render_Game(void)
{
    /*
     * Fill uses logical RGB values plus a low brightness scale, producing a
     * dim purple background without repeated per-pixel drawing calls.
     */
    LED_Fill(BACKGROUND_RED,
             BACKGROUND_GREEN,
             BACKGROUND_BLUE,
             game.background_brightness);

    /* Round the fixed-point position to the nearest physical LED row. */
    uint8_t bird_y =
        (uint8_t)((game.bird_position + (POSITION_SCALE / 2)) / POSITION_SCALE);

    LED_DrawPixel(PLAYER_START_X,
                  bird_y,
                  BIRD_RED,
                  BIRD_GREEN,
                  BIRD_BLUE,
                  BIRD_BRIGHTNESS);
}

void Game_Task(void)
{
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
        last_update_time = current_time;
    }
}
