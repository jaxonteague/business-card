/*
 * config.h
 *
 *  Game configuration settings.
 *
 *  Created on: 26 July 2026
 *      Author: Jaxon Teague
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


/*
 * Game update period.
 *
 * Units:
 * milliseconds
 *
 * 16ms gives approximately 60 FPS.
 */
#define GAME_UPDATE_PERIOD_MS    20U//16U


#define LED_DEFAULT_BRIGHTNESS   255U
#define GAME_WIDTH               16U
#define GAME_HEIGHT              8U
#define PLAYER_START_X           0U
#define PLAYER_START_Y           0U

#endif /* INC_CONFIG_H_ */
