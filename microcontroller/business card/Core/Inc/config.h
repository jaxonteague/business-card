/**
 * @file config.h
 * @brief Compile-time settings for game timing, physics, and appearance.
 *
 * Created on: 26 July 2026
 * Author: Jaxon Teague
 */

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_


/* Frame period in milliseconds; 16 ms produces approximately 60 FPS. */
#define GAME_UPDATE_PERIOD_MS    16U


/* Bird start coordinates and RGB brightness values. */
#define PLAYER_START_X                 0U
#define PLAYER_START_Y                 3U
#define BIRD_RED                       0U
#define BIRD_GREEN                     255U
#define BIRD_BLUE                      0U
#define BIRD_BRIGHTNESS                100U

/* Physics uses 1/16-pixel units; negative is up and positive is down. */
#define BIRD_FLAP_VELOCITY             (-8)
#define BIRD_GRAVITY                   1
#define BIRD_TERMINAL_VELOCITY         6

/* Background RGB colour, brightness limits, and animation rate divider. */
#define BACKGROUND_RED                 160U
#define BACKGROUND_GREEN               0U
#define BACKGROUND_BLUE                255U
#define BACKGROUND_BRIGHTNESS_MIN      2U
#define BACKGROUND_BRIGHTNESS_MAX      12U
#define BACKGROUND_BREATHING_DIVIDER   8U

#endif /* INC_CONFIG_H_ */
