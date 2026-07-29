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
#define GAME_UPDATE_PERIOD_MS    16U


/* Bird appearance and initial location. */
#define PLAYER_START_X                 0U
#define PLAYER_START_Y                 3U
#define BIRD_RED                       0U
#define BIRD_GREEN                     255U
#define BIRD_BLUE                      0U
#define BIRD_BRIGHTNESS                100U

/*
 * Fixed-point physics values use 1/16 pixel units. Negative velocity moves
 * upward; positive velocity and gravity move downward.
 */
#define BIRD_FLAP_VELOCITY             (-8)
#define BIRD_GRAVITY                   1
#define BIRD_TERMINAL_VELOCITY         6

/* Dim purple breathing background. */
#define BACKGROUND_RED                 160U
#define BACKGROUND_GREEN               0U
#define BACKGROUND_BLUE                255U
#define BACKGROUND_BRIGHTNESS_MIN      2U
#define BACKGROUND_BRIGHTNESS_MAX      12U
#define BACKGROUND_BREATHING_DIVIDER   8U

#endif /* INC_CONFIG_H_ */
