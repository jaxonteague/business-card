/*
 * game.c
 *
 * Main game logic
 *
 *  Created on: 26 July 2026
 *      Author: Jaxon Teague
 */


#include "game.h"
#include "led.h"
#include "config.h"
#include "stm32c0xx_hal.h" //need this for HAL_GetTick()

#include <stdio.h>
#include <stdbool.h>

/*
 * Private function prototypes.
 */
static void Update_Game_State(void);
static void Render_Game(void);

/*
 * Game state variables.
 *
 * These will expand as the game is developed.
 */
static uint32_t last_update_time;

typedef struct
{
    /*
     * Player position.
     */
    uint8_t player_x;
    uint8_t player_y;

    /*
     * Player movement direction.
     */
    int8_t player_dx;
    int8_t player_dy;

    /*
     * Animation colour.
     */
    uint8_t hue;

    /*
     * Current score.
     */
    uint16_t score;

    /*
     * Game running.
     */
    bool active;

    int8_t brightness_step;
    uint8_t background_brightness;
    uint8_t breathing_counter;
    uint8_t colour_phase;
    uint8_t player_move_counter;

} Game_State_t;


/*
 * Current game instance.
 */
static Game_State_t game;


/*
 * Initialise game state.
 */
void Game_Init(void)
{
    /*
     * Initialise game state.
     */
	game.player_x = 0;
	game.player_y = 0;
	game.player_dx = 1;
	game.player_dy = 1;
	game.hue = 0;

	game.background_brightness = 1;
	game.brightness_step = 1;
	game.breathing_counter = 0;
	game.colour_phase = 0;
	game.player_move_counter = 0;

    /*
     * Render and display the initial game state before
     * the first scheduled update.
     */
    Render_Game();

    while(!LED_Transmit())
    {
    }

    last_update_time = HAL_GetTick();
}


static void Update_Game_State(void)
{
	/*
	 * Move player every 4 frames.
	 */
	game.player_move_counter++;

	if(game.player_move_counter >= 4)
	{
	    game.player_move_counter = 0;

	    game.player_x += game.player_dx;
	    game.player_y += game.player_dy;

	    if(game.player_x == 0 ||
	       game.player_x == (LED_COLUMNS - 1))
	    {
	        game.player_dx = -game.player_dx;
	    }

	    if(game.player_y == 0 ||
	       game.player_y == (LED_ROWS - 1))
	    {
	        game.player_dy = -game.player_dy;
	    }
	}

    /*
     * Slow breathing effect.
     *
     * Update brightness once every 8 frames.
     * At 16 ms per frame, one full breath takes
     * approximately 4.9 seconds.
     */
    game.breathing_counter++;

    if(game.breathing_counter >= 8)
    {
        game.breathing_counter = 0;

        if(game.brightness_step > 0)
        {
            if(game.background_brightness >= 20)
            {
                game.background_brightness = 20;
                game.brightness_step = -1;
            }
            else
            {
                game.background_brightness++;
            }
        }
        else
        {
            if(game.background_brightness <= 1)
            {
                game.background_brightness = 1;
                game.brightness_step = 1;
            }
            else
            {
                game.background_brightness--;
            }
        }
    }

    /*
     * Slowly rotate through the colour spectrum.
     */
    game.colour_phase++;
}


static void Get_Background_Colour(
    uint8_t phase,
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue
)
{
    uint8_t section;
    uint8_t offset;

    section = phase / 43;
    offset = (phase % 43) * 6;

    switch(section)
    {
        case 0:
            *red = 255;
            *green = offset;
            *blue = 0;
            break;

        case 1:
            *red = 255 - offset;
            *green = 255;
            *blue = 0;
            break;

        case 2:
            *red = 0;
            *green = 255;
            *blue = offset;
            break;

        case 3:
            *red = 0;
            *green = 255 - offset;
            *blue = 255;
            break;

        case 4:
            *red = offset;
            *green = 0;
            *blue = 255;
            break;

        default:
            *red = 255;
            *green = 0;
            *blue = 255 - offset;
            break;
    }
}

/*
 * Render current game state.
 */
static void Render_Game(void)
{
    uint8_t x;
    uint8_t y;
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    LED_Clear();

    Get_Background_Colour(
        game.colour_phase,
        &red,
        &green,
        &blue
    );

    /*
     * Slowly changing colour with breathing brightness.
     */
    for(y = 0; y < LED_ROWS; y++)
    {
        for(x = 0; x < LED_COLUMNS; x++)
        {
            LED_DrawPixel(
                x,
                y,
                red,
                green,
                blue,
                game.background_brightness
            );
        }
    }

    LED_DrawPixel(
        game.player_x,
        game.player_y,
        255 - red,
        255 - green,
        255 - blue,
        31
    );
}


/*
 * Main game task.
 */
void Game_Task(void)
{
    uint32_t current_time = HAL_GetTick();

    if((current_time - last_update_time) < GAME_UPDATE_PERIOD_MS)
    {
        return;
    }

    Render_Game();

    if(LED_Transmit())
    {
        last_update_time = current_time;
        Update_Game_State();
    }
}
