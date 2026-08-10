/**
 * @file game.c
 * @brief Implements bird physics, pipes, scoring, lives, and rendering.
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

#define DIGIT_WIDTH                5U
#define DIGIT_HEIGHT               7U
#define DIGIT_SPACING              1U
#define BACKGROUND_COLOUR_COUNT     2U

typedef enum
{
    GAME_MODE_PLAYING,
    GAME_MODE_RECOVERING,
    GAME_MODE_GAME_OVER
} Game_Mode_t;

typedef enum
{
    PIPE_ACTIVE,
    PIPE_PASSED,
    PIPE_IGNORED
} Pipe_Status_t;

typedef struct
{
    int16_t bird_position;          /* Vertical position in fixed-point units. */
    int16_t bird_velocity;          /* Signed vertical speed per game update. */
    uint8_t gravity_accumulator;    /* Produces fractional average gravity. */

    int8_t pipe_x;                  /* Left edge of the pipe in matrix columns. */
    uint8_t pipe_gap_y;             /* Top row of the passable pipe opening. */
    uint8_t pipe_move_accumulator;  /* Accumulates fractional pipe movement. */
    Pipe_Status_t pipe_status;      /* Active, successfully passed, or ignored. */

    uint8_t lives;                  /* Remaining collisions before game over. */
    uint16_t score;                 /* Number of pipes successfully passed. */
    uint8_t difficulty_level;       /* Cached pipe-speed and background level. */
    uint16_t random_state;          /* Pseudorandom state used for pipe gaps. */
    Game_Mode_t mode;               /* Current game state and rendered screen. */
    uint32_t state_change_time;      /* HAL tick when the current mode began. */
} Game_State_t;

/*
 * Five-by-seven digit rows stored as five active bits. The font permits two
 * large digits to fit side-by-side on the 16-column display.
 */
static const uint8_t digit_font[10][DIGIT_HEIGHT] =
{
    {0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU}, /* 0 */
    {0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU}, /* 1 */
    {0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU}, /* 2 */
    {0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU}, /* 3 */
    {0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U}, /* 4 */
    {0x1FU, 0x10U, 0x10U, 0x1EU, 0x01U, 0x01U, 0x1EU}, /* 5 */
    {0x0EU, 0x10U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU}, /* 6 */
    {0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U}, /* 7 */
    {0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU}, /* 8 */
    {0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x01U, 0x0EU}  /* 9 */
};

/*
 * Background colours alternate between blue and purple as the speed level
 * increases. Both remain distinct from the red pipes and all bird colours.
 */
static const uint8_t background_colours[BACKGROUND_COLOUR_COUNT][3] =
{
    {
        BACKGROUND_BLUE_RED,
        BACKGROUND_BLUE_GREEN,
        BACKGROUND_BLUE_BLUE
    }, /* Even levels: blue */
    {
        BACKGROUND_PURPLE_RED,
        BACKGROUND_PURPLE_GREEN,
        BACKGROUND_PURPLE_BLUE
    }  /* Odd levels: deep purple */
};

/* Complete mutable state for the current game session. */
static Game_State_t game;

/* Interrupt-to-main-loop flag; volatile because the EXTI callback writes it. */
static volatile bool flap_requested;

/* HAL tick when the most recent frame was accepted by the LED driver. */
static uint32_t last_update_time;

static void Start_New_Game(void);
static void Reset_Round(void);
static uint8_t Next_Pipe_Gap(void);
static bool Update_Playing(uint32_t current_time);
static bool Update_Recovering(uint32_t current_time);
static bool Update_Game_Over(uint32_t current_time);
static void Update_Game_State(uint32_t current_time);
static void Update_Pipe(void);
static bool Bird_Hit_Pipe(void);
static void Lose_Life(uint32_t current_time);
static void Render_Game(void);
static void Render_Playing(void);
static void Render_Background(void);
static void Render_Pipe(void);
static void Render_Large_Number(uint16_t value);
static void Render_Digit(uint8_t digit, uint8_t origin_x);

/**
 * @brief Reset game state, render the initial frame, and start LED output.
 */
void Game_Init(void)
{
    Start_New_Game();
    Render_Game();

    while(!LED_Transmit())
    {
        /* DMA should be idle at startup; wait if the driver is not ready yet. */
    }

    last_update_time = HAL_GetTick();
}

/**
 * @brief Queue one flap or game restart for processing in the main loop.
 *
 * This function is safe to call from the GPIO interrupt callback because it
 * only sets a flag; game state and rendering remain in the main-loop context.
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
 * @brief Initialize a new game with the configured lives and a score of zero.
 */
static void Start_New_Game(void)
{
    game.lives = PLAYER_STARTING_LIVES;
    game.score = 0U;
    game.difficulty_level = 0U;
    game.random_state = (uint16_t)(HAL_GetTick() ^ 0xA5C3U);
    game.mode = GAME_MODE_PLAYING;
    game.state_change_time = HAL_GetTick();

    Reset_Round();
}

/**
 * @brief Reset the bird and pipe while preserving lives and score.
 */
static void Reset_Round(void)
{
    game.bird_position = PLAYER_START_Y * POSITION_SCALE;
    game.bird_velocity = 0;
    /* Prime the accumulator so gravity is applied on the first update. */
    game.gravity_accumulator =
        BIRD_GRAVITY_DENOMINATOR - BIRD_GRAVITY_NUMERATOR;
    game.pipe_x = LED_COLUMNS;
    game.pipe_gap_y = Next_Pipe_Gap();
    game.pipe_move_accumulator = 0U;
    game.pipe_status = PIPE_ACTIVE;
    flap_requested = false;
}

/**
 * @brief Generate a valid pipe-gap row using a compact xorshift sequence.
 *
 * @return Top row of a gap that leaves at least one pipe LED above and below.
 */
static uint8_t Next_Pipe_Gap(void)
{
    uint16_t value = game.random_state;

    value ^= (uint16_t)(value << 7U);
    value ^= (uint16_t)(value >> 9U);
    value ^= (uint16_t)(value << 8U);
    game.random_state = value;

    return (uint8_t)(1U +
        (value % (LED_ROWS - PIPE_GAP_HEIGHT - 1U)));
}

/**
 * @brief Advance active gameplay by one frame.
 *
 * @param current_time Current HAL tick used for collision timing.
 * @return true because active gameplay always produces a new frame.
 */
static bool Update_Playing(uint32_t current_time)
{
    Update_Game_State(current_time);
    return true;
}

/**
 * @brief Continue gameplay while temporarily flashing the ignored hit pipe.
 *
 * @param current_time Current HAL tick used to time the pipe flash.
 * @return true because recovery continues to produce gameplay frames.
 */
static bool Update_Recovering(uint32_t current_time)
{
    if((current_time - game.state_change_time) >= PIPE_HIT_FLASH_MS)
    {
        game.mode = GAME_MODE_PLAYING;
    }

    Update_Game_State(current_time);
    return true;
}

/**
 * @brief Hold the score for three seconds, then wait for a fresh button press.
 *
 * @param current_time Current HAL tick used to enforce the score hold.
 * @return true when a new game starts and its initial frame must be rendered.
 */
static bool Update_Game_Over(uint32_t current_time)
{
    if((current_time - game.state_change_time) < GAME_OVER_SCORE_DELAY_MS)
    {
        /* Discard early presses so restarting requires a press after the hold. */
        flap_requested = false;
        return false;
    }

    if(!flap_requested)
    {
        return false;
    }

    Start_New_Game();
    return true;
}

/**
 * @brief Apply input, advance physics and pipes, and detect collisions.
 *
 * @param current_time Current HAL tick used to timestamp a lost life.
 */
static void Update_Game_State(uint32_t current_time)
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

    /*
     * Apply gravity at an average rate of 3/4 units per frame. This is about
     * 10 percent weaker than the previous 33/40-unit average.
     */
    game.gravity_accumulator += BIRD_GRAVITY_NUMERATOR;

    if(game.gravity_accumulator >= BIRD_GRAVITY_DENOMINATOR)
    {
        game.gravity_accumulator -= BIRD_GRAVITY_DENOMINATOR;
        game.bird_velocity += BIRD_GRAVITY;
    }

    if(game.bird_velocity > BIRD_TERMINAL_VELOCITY)
    {
        game.bird_velocity = BIRD_TERMINAL_VELOCITY;
    }

    game.bird_position += game.bird_velocity;

    /* Clip to the display edges without treating either boundary as a hit. */
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

    Update_Pipe();

    if(Bird_Hit_Pipe())
    {
        Lose_Life(current_time);
    }
}

/**
 * @brief Move the pipe periodically, award points, and recycle passed pipes.
 */
static void Update_Pipe(void)
{
    uint8_t move_rate = PIPE_MOVE_RATE_START +
        (game.difficulty_level * PIPE_MOVE_RATE_PER_LEVEL);
    game.pipe_move_accumulator += move_rate;

    if(game.pipe_move_accumulator < PIPE_MOVE_RATE_SCALE)
    {
        return;
    }

    game.pipe_move_accumulator -= PIPE_MOVE_RATE_SCALE;
    game.pipe_x--;

    /* Award one point when the complete pipe has moved behind the bird. */
    if((game.pipe_status == PIPE_ACTIVE) &&
       ((game.pipe_x + (int8_t)PIPE_WIDTH - 1) <
        (int8_t)PLAYER_START_X))
    {
        game.score++;
        game.pipe_status = PIPE_PASSED;

        if(((game.score % PIPE_SPEEDUP_SCORE_INTERVAL) == 0U) &&
           (game.difficulty_level < PIPE_MAX_DIFFICULTY_LEVEL))
        {
            game.difficulty_level++;
        }
    }

    if(game.pipe_x < -(int8_t)PIPE_WIDTH)
    {
        game.pipe_x = LED_COLUMNS;
        game.pipe_gap_y = Next_Pipe_Gap();
        game.pipe_status = PIPE_ACTIVE;
    }
}

/**
 * @brief Check whether the bird overlaps a solid row of the current pipe.
 *
 * @return true if the bird has hit the current pipe.
 */
static bool Bird_Hit_Pipe(void)
{
    if(game.pipe_status == PIPE_IGNORED)
    {
        return false;
    }

    int8_t pipe_right = game.pipe_x + (int8_t)PIPE_WIDTH - 1;

    if(((int8_t)PLAYER_START_X < game.pipe_x) ||
       ((int8_t)PLAYER_START_X > pipe_right))
    {
        return false;
    }

    uint8_t bird_y =
        (uint8_t)((game.bird_position + (POSITION_SCALE / 2)) / POSITION_SCALE);

    return (bird_y < game.pipe_gap_y) ||
           (bird_y >= (game.pipe_gap_y + PIPE_GAP_HEIGHT));
}

/**
 * @brief Consume one life and ignore the pipe responsible for the collision.
 *
 * @param current_time Current HAL tick used to time the collision indication.
 */
static void Lose_Life(uint32_t current_time)
{
    if(game.lives > 0U)
    {
        game.lives--;
    }

    game.state_change_time = current_time;

    if(game.lives == 0U)
    {
        game.mode = GAME_MODE_GAME_OVER;
        flap_requested = false;
    }
    else
    {
        /* This pipe can neither hit again nor award a point after the collision. */
        game.pipe_status = PIPE_IGNORED;
        game.mode = GAME_MODE_RECOVERING;
    }
}

/**
 * @brief Render the screen associated with the current game mode.
 */
static void Render_Game(void)
{
    switch(game.mode)
    {
        case GAME_MODE_PLAYING:
            Render_Playing();
            break;

        case GAME_MODE_RECOVERING:
            Render_Playing();
            break;

        case GAME_MODE_GAME_OVER:
            Render_Large_Number(game.score);
            break;
    }
}

/**
 * @brief Draw the gameplay background, pipe, and bird into the framebuffer.
 */
static void Render_Playing(void)
{
    Render_Background();

    Render_Pipe();

    /* Round the clamped fixed-point position to the nearest display row. */
    uint8_t bird_y =
        (uint8_t)((game.bird_position + (POSITION_SCALE / 2)) / POSITION_SCALE);

    /* Select a high-contrast bird colour for the remaining life count. */
    uint8_t bird_red = BIRD_RED;
    uint8_t bird_green = BIRD_GREEN;
    uint8_t bird_blue = BIRD_BLUE;

    if(game.lives == 2U)
    {
        bird_red = BIRD_TWO_LIVES_RED;
        bird_green = BIRD_TWO_LIVES_GREEN;
        bird_blue = BIRD_TWO_LIVES_BLUE;
    }
    else if(game.lives == 1U)
    {
        bird_red = BIRD_ONE_LIFE_RED;
        bird_green = BIRD_ONE_LIFE_GREEN;
        bird_blue = BIRD_ONE_LIFE_BLUE;
    }

    LED_DrawPixel(PLAYER_START_X,
                  bird_y,
                  bird_red,
                  bird_green,
                  bird_blue,
                  BIRD_BRIGHTNESS);
}

/**
 * @brief Fill the matrix with the fixed RGB colour for the current level.
 */
static void Render_Background(void)
{
    uint8_t colour = game.difficulty_level % BACKGROUND_COLOUR_COUNT;

    LED_Fill(background_colours[colour][0],
             background_colours[colour][1],
             background_colours[colour][2],
             BACKGROUND_BRIGHTNESS);
}

/**
 * @brief Draw the visible columns of the pipe around its passable gap.
 */
static void Render_Pipe(void)
{
    uint8_t pipe_brightness = PIPE_BRIGHTNESS;

    if(game.mode == GAME_MODE_RECOVERING)
    {
        /* Alternate between bright red and off while the hit is acknowledged. */
        uint32_t flash_time = HAL_GetTick() - game.state_change_time;

        if(((flash_time / PIPE_HIT_FLASH_PERIOD_MS) & 1U) != 0U)
        {
            pipe_brightness = PIPE_HIT_BRIGHTNESS;
        }
        else
        {
            pipe_brightness = 0U;
        }
    }

    for(uint8_t offset = 0U; offset < PIPE_WIDTH; offset++)
    {
        int8_t x = game.pipe_x + (int8_t)offset;

        if((x < 0) || (x >= (int8_t)LED_COLUMNS))
        {
            continue;
        }

        for(uint8_t y = 0U; y < LED_ROWS; y++)
        {
            bool inside_gap = (y >= game.pipe_gap_y) &&
                              (y < (game.pipe_gap_y + PIPE_GAP_HEIGHT));

            if(!inside_gap)
            {
                LED_DrawPixel((uint8_t)x,
                              y,
                              PIPE_RED,
                              PIPE_GREEN,
                              PIPE_BLUE,
                              pipe_brightness);
            }
        }
    }
}

/**
 * @brief Draw a centered one- or two-digit value using the five-by-seven font.
 *
 * Values above 99 display their final two digits because only two large digits
 * fit on the matrix.
 *
 * @param value Number to display.
 */
static void Render_Large_Number(uint16_t value)
{
    LED_Clear();

    if(value < 10U)
    {
        uint8_t origin_x = (LED_COLUMNS - DIGIT_WIDTH) / 2U;
        Render_Digit((uint8_t)value, origin_x);
    }
    else
    {
        uint8_t display_value = (uint8_t)(value % 100U);
        uint8_t total_width = (2U * DIGIT_WIDTH) + DIGIT_SPACING;
        uint8_t origin_x = (LED_COLUMNS - total_width) / 2U;

        Render_Digit(display_value / 10U, origin_x);
        Render_Digit(display_value % 10U,
                     origin_x + DIGIT_WIDTH + DIGIT_SPACING);
    }
}

/**
 * @brief Draw one five-by-seven digit at a specified horizontal origin.
 *
 * @param digit Numeric digit from 0 to 9.
 * @param origin_x Leftmost destination column.
 */
static void Render_Digit(uint8_t digit, uint8_t origin_x)
{
    for(uint8_t y = 0U; y < DIGIT_HEIGHT; y++)
    {
        uint8_t row_bits = digit_font[digit][y];

        for(uint8_t x = 0U; x < DIGIT_WIDTH; x++)
        {
            if((row_bits & (1U << (DIGIT_WIDTH - 1U - x))) != 0U)
            {
                LED_DrawPixel(origin_x + x,
                              y,
                              DIGIT_RED,
                              DIGIT_GREEN,
                              DIGIT_BLUE,
                              DIGIT_BRIGHTNESS);
            }
        }
    }
}

/**
 * @brief Run one non-blocking, frame-timed game update when the LEDs are ready.
 */
void Game_Task(void)
{
    uint32_t current_time = HAL_GetTick();
    bool frame_changed;

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

    switch(game.mode)
    {
        case GAME_MODE_PLAYING:
            frame_changed = Update_Playing(current_time);
            break;

        case GAME_MODE_RECOVERING:
            frame_changed = Update_Recovering(current_time);
            break;

        case GAME_MODE_GAME_OVER:
            frame_changed = Update_Game_Over(current_time);
            break;

        default:
            frame_changed = false;
            break;
    }

    if(!frame_changed)
    {
        return;
    }

    Render_Game();

    if(LED_Transmit())
    {
        last_update_time = current_time;
    }
}
