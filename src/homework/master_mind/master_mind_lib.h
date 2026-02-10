/**
 * @file master_mind.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #ifndef __MASTER_MIND_H__
 #define __MASTER_MIND_H__

#include "main.h"
#include <stdint.h>
#include <stdbool.h>

/********************************************************************************/
/* Macros used to define the geometry and location of objects                   */
/********************************************************************************/


// =====================================================
// Screen geometry
// =====================================================
#define LCD_W               320
#define LCD_H               240
#define LCD_MARGIN          8

// =====================================================
// Area heights
// =====================================================
#define LCD_TEXT_H          32
#define LCD_ROW_H           56

// =====================================================
// Box geometry
// =====================================================
#define TILE_W               70
#define TILE_H               56

/********************************************************************************/
/* Data Types for Master Mind Game                                              */
/********************************************************************************/

/**
 * @brief 
 * This enumerate type is used to define the purpose of each of the rows printed
 * out to the screen.
 */
typedef enum
{
    LCD_TILE_ROW_CYPHER = 0,    // Secret code
    LCD_TILE_ROW_NUM_0_3,       // Input numbers 0–3
    LCD_TILE_ROW_NUM_4_7,       // Input numbers 4–7
    LCD_TILE_ROW_COUNT
} lcd_row_t;

/* Defines the 4 columns used to render boxes */
typedef uint8_t lcd_col_t; // Valid range: 0–3

/* Struct used to define the rectangle for a tile */
typedef struct
{
    int16_t cx;   // center X pixel
    int16_t cy;   // center Y pixel
    int16_t w;    // width in pixels
    int16_t h;    // height in pixels
} lcd_rect_t;

/* Defines a box on the LCD screen */
typedef struct{
    lcd_row_t row;  // Which row the tile is in
    lcd_col_t col;  // Which column the tile is in  
    uint8_t number; // Valid values: 0–7 (ignored if type == EMPTY)
    uint16_t color_fg; // Foreground color
    uint16_t color_bg; // Background color
} lcd_tile_t;

/* Defines the type of command to send to the LCD */
typedef enum {
    LCD_CMD_DRAW_TILE,               // Set content of a single box
    LCD_CMD_DRAW_TILE_INVERTED,      // Highlight exactly one box
    LCD_CMD_PRINT_MESSAGE,          // Print status/instruction text
    LCD_CMD_CLEAR_SCREEN,           // Optional: clear whole screen
} lcd_command_t;


/* Message format sent to the LCD task */
typedef struct {
    lcd_command_t command;
    union {
        lcd_tile_t tile;                  // For UP
        char message[32];               // For PRINT_MESSAGE    
    } payload;
} lcd_msg_t;


// =====================================================
// Internal area Y positions (not part of public API)
// =====================================================
static inline int lcd_area_text_top(void)
{
    return LCD_MARGIN;
}

static inline int lcd_area_code_top(void)
{
    return lcd_area_text_top() + LCD_TEXT_H + LCD_MARGIN;
}

static inline int lcd_area_num0_3_top(void)
{
    return lcd_area_code_top() + LCD_ROW_H + LCD_MARGIN;
}

static inline int lcd_area_num4_7_top(void)
{
    return lcd_area_num0_3_top() + LCD_ROW_H + LCD_MARGIN;
}

// =====================================================
// Box geometry helpers (public API)
// =====================================================
static inline int lcd_tile_left_x(lcd_col_t col)
{
    return LCD_MARGIN + col * (TILE_W + LCD_MARGIN);
}

static inline int lcd_tile_center_x(lcd_col_t col)
{
    return lcd_tile_left_x(col) + (TILE_W / 2);
}

static inline int lcd_tile_top_y(lcd_row_t row)
{
    switch (row)
    {
        case LCD_TILE_ROW_CYPHER:
            return lcd_area_code_top();

        case LCD_TILE_ROW_NUM_0_3:
            return lcd_area_num0_3_top();

        case LCD_TILE_ROW_NUM_4_7:
        default:
            return lcd_area_num4_7_top();
    }
}

static inline int lcd_tile_center_y(lcd_row_t row)
{
    return lcd_tile_top_y(row) + (TILE_H / 2);
}

// =====================================================
// Tile dimensions (center-based drawing helpers)
// =====================================================
static inline int lcd_tile_width(void)
{
    return TILE_W;
}

static inline int lcd_tile_height(void)
{
    return TILE_H;
}

// =====================================================
// Text area helpers
// =====================================================
static inline int lcd_text_area_width(void)
{
    return LCD_W - 2 * LCD_MARGIN;
}

static inline int lcd_text_area_center_x(void)
{
    return LCD_MARGIN + lcd_text_area_width() / 2;
}

static inline int lcd_text_area_center_y(void)
{
    return lcd_area_text_top() + (LCD_TEXT_H / 2);
}

static inline int lcd_text_area_height(void)
{
    return LCD_TEXT_H;
}


/**
 * @brief 
 * This function will calculate the rectangle coordinates and dimensions for a given tile
 * @param r 
 * @param row 
 * @param col 
 * @return true 
 * @return false 
 */
static inline bool lcd_tile_rect(lcd_rect_t *r, lcd_row_t row, uint8_t col)
{
    if(r == NULL)
    {
        return false;
    }

    if (row >= LCD_TILE_ROW_COUNT || col >= 4)
    {
        return false;
    }

    r->cx = lcd_tile_center_x(col);
    r->cy = lcd_tile_center_y(row);
    r->w  = lcd_tile_width();
    r->h  = lcd_tile_height();

    return true;
}

/********************************************************************************/
/* Function Prototypes                                                          */
/********************************************************************************/
bool master_mind_handle_msg(lcd_msg_t* msg);

#endif // __MASTER_MIND_H__
