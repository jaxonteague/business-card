/*
 * game.h
 *
 *  Game logic interface
 *
 *  Created on: 26 July 2026
 *      Author: Jaxon Teague
 */

#ifndef INC_GAME_H_
#define INC_GAME_H_

/*
 * Initialise game variables.
 *
 * Called once during startup.
 */
void Game_Init(void);


/*
 * Main game processing function.
 *
 * Called continuously from main loop.
 *
 * Handles:
 * - Game timing
 * - Game state updates
 * - Rendering
 */
void Game_Task(void);

/*
 * Record an active-low button press.
 *
 * Safe to call from HAL_GPIO_EXTI_Falling_Callback(); the actual physics
 * update is deferred until Game_Task() runs in the main loop.
 */
void Game_ButtonPressed(void);

#endif /* INC_GAME_H_ */
