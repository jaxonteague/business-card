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
 * Game state variables.
 *
 * These will expand as the game is developed.
 */
static uint32_t last_update_time;

/*
 * Current game state.
 *
 * Contains all variables required to
 * update and render the game.
 */
typedef struct
{
    /*
     * Player position.
     *
     * Uses LED matrix coordinates.
     */
    uint8_t player_x;
    uint8_t player_y;


    /*
     * Current score.
     */
    uint16_t score;


    /*
     * Game status.
     *
     * false:
     *   Game not running
     *
     * true:
     *   Game active
     */
    bool active;


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
	LED_Clear();

	/*
     * Set initial player position.
     */
    game.player_x = 0;
    game.player_y = 0;


    /*
     * Reset score.
     */
    game.score = 0;


    /*
     * Start game.
     */
    game.active = true;
}


/*
 * Update game logic.
 *
 * Moves the player:
 * - Left to right across a row
 * - Then moves down one row
 * - Repeats from the top
 */
static void Update_Game_State(void)
{
    /*
     * Move right one pixel.
     */
    game.player_x++;


    /*
     * End of row reached.
     */
    if(game.player_x >= LED_COLUMNS)
    {
        game.player_x = 0;

        /*
         * Move to next row.
         */
        game.player_y++;


        /*
         * End of display reached.
         *
         * Start again at top.
         */
        if(game.player_y >= LED_ROWS)
        {
            game.player_y = 0;
        }
    }

    //for debugging
    //printf("x=%d y=%d\r\n", game.player_x, game.player_y);
}


/*
 * Render current game state.
 */
static void Render_Game(void)
{
    LED_Clear();

    LED_DrawPixel(
        game.player_x,
        game.player_y,
        0,
        255,
        0,
        6
    );
}
//static void Render_Game(void)//test
//{
//    LED_Clear();
//
//    for(uint8_t y = 0; y < LED_ROWS; y++)
//    {
//        for(uint8_t x = 0; x < LED_COLUMNS; x++)
//        {
//            LED_DrawPixel(
//                x,
//                y,
//                0,
//                255,
//                0,
//                6
//            );
//        }
//    }
//}


/*
 * Main game task.
 */
/*
 * Main game task.
 */
void Game_Task(void)
{
    static volatile uint32_t task_calls = 0;
    static volatile uint32_t game_updates = 0;
    static volatile uint32_t transmit_failures = 0;

    task_calls++;

    uint32_t current_time = HAL_GetTick();

    /*
     * Wait until the next game update.
     */
    if((current_time - last_update_time) >= GAME_UPDATE_PERIOD_MS)
    {
        game_updates++;

        last_update_time = current_time;

        Update_Game_State();

        Render_Game();

        /*
         * Send rendered frame to LEDs.
         */
        if(!LED_Transmit())
        {
            transmit_failures++;
        }
    }
}
