/**
 * @file game.h
 * @brief Public interface for game initialization, input, and frame updates.
 *
 * Created on: 26 July 2026
 * Author: Jaxon Teague
 */

#ifndef INC_GAME_H_
#define INC_GAME_H_

/**
 * @brief Initialize game state and display the initial frame.
 *
 * Call once after the LED and HAL peripherals have been initialized.
 */
void Game_Init(void);

/**
 * @brief Process frame timing, game physics, and rendering without blocking.
 *
 * Call continuously from the main loop.
 */
void Game_Task(void);

/**
 * @brief Record an active-low button press for the next game update.
 *
 * Safe to call from HAL_GPIO_EXTI_Falling_Callback(); the actual physics
 * update is deferred until Game_Task() runs in the main loop.
 */
void Game_ButtonPressed(void);

#endif /* INC_GAME_H_ */
