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
#define BIRD_BRIGHTNESS                20U

/* Physics uses 1/16-pixel units; negative is up and positive is down. */
#define BIRD_FLAP_VELOCITY             (-7)
#define BIRD_GRAVITY                   1
#define BIRD_GRAVITY_NUMERATOR         3U
#define BIRD_GRAVITY_DENOMINATOR       4U
#define BIRD_TERMINAL_VELOCITY         6

/* Player lives and collision/game-over timing. */
#define PLAYER_STARTING_LIVES          3U
#define PIPE_HIT_FLASH_MS              600U
#define PIPE_HIT_FLASH_PERIOD_MS       100U
#define GAME_OVER_SCORE_DELAY_MS       3000U

/* Bird colours change with the number of remaining lives. */
#define BIRD_TWO_LIVES_RED             255U
#define BIRD_TWO_LIVES_GREEN           255U
#define BIRD_TWO_LIVES_BLUE            255U
#define BIRD_ONE_LIFE_RED              255U
#define BIRD_ONE_LIFE_GREEN            255U
#define BIRD_ONE_LIFE_BLUE             0U

/* Pipe geometry and movement rate. */
#define PIPE_WIDTH                     2U
#define PIPE_GAP_HEIGHT                3U
#define PIPE_MOVE_RATE_SCALE           40U
#define PIPE_MOVE_RATE_START           5U
#define PIPE_MOVE_RATE_PER_LEVEL       1U
#define PIPE_SPEEDUP_SCORE_INTERVAL    5U
#define PIPE_DIFFICULTY_LEVEL_COUNT    10U
#define PIPE_MAX_DIFFICULTY_LEVEL      (PIPE_DIFFICULTY_LEVEL_COUNT - 1U)
#define PIPE_RED                       255U
#define PIPE_GREEN                     0U
#define PIPE_BLUE                      0U
#define PIPE_BRIGHTNESS                6U
#define PIPE_HIT_BRIGHTNESS            12U

/* Fixed background palette entries use the minimum non-zero brightness. */
#define BACKGROUND_BLUE_RED            0U
#define BACKGROUND_BLUE_GREEN          0U
#define BACKGROUND_BLUE_BLUE           255U
#define BACKGROUND_PURPLE_RED          255U
#define BACKGROUND_PURPLE_GREEN        0U
#define BACKGROUND_PURPLE_BLUE         255U
#define BACKGROUND_BRIGHTNESS          1U

/* Large game-over score appearance. */
#define DIGIT_RED                      255U
#define DIGIT_GREEN                    255U
#define DIGIT_BLUE                     255U
#define DIGIT_BRIGHTNESS               4U

#endif /* INC_CONFIG_H_ */
