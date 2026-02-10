/**
 * @file master_mind.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "main.h"
#include "drivers.h"
#include "master_mind_lib.h"

/**
 * @brief 
 * This function will parse an LCD message and perform the appropriate action
 * @param msg 
 * @return true 
 * @return false 
 */
bool master_mind_handle_msg(lcd_msg_t* msg) {

    int location;
    int width = 0;

    // Write message on top left of the screen
    switch (msg->command) {
        
        case (LCD_CMD_PRINT_MESSAGE):

            // Run a for loop for each character in the message and print it to the screen
                for (int i = 0; i < 32; i++) {
                    if (msg->payload.message[i] == '\0') {
                        break;
                    }

                    // Convert character to ascii and get info from font info
                    location = ((int)msg->payload.message[i]) - Consolas_20ptFontInfo.start_char;
                    FONT_CHAR_INFO info = Consolas_20ptDescriptors[location];

                    // Draw the character to the screen
                    lcd_draw_image(
                        width, 
                        0, 
                        info.width, 
                        info.height, 
                        Consolas_20ptBitmaps + info.offset, 
                        LCD_COLOR_WHITE, 
                        LCD_COLOR_BLACK, 
                        false
                    );

                    // Update width for next character
                    width += info.width;
                }

            return true;

        case (LCD_CMD_DRAW_TILE):
            //Draw a tile at the specified location
            lcd_draw_rectangle(
                lcd_tile_left_x(msg->payload.tile.col), 
                lcd_tile_top_y(msg->payload.tile.row), 
                TILE_W, 
                TILE_H, 
                msg->payload.tile.color_bg, 
                false
            );
    
            // Draw the number on top of the tile
            lcd_draw_image(
                lcd_tile_center_x(msg->payload.tile.col),
                lcd_tile_center_y(msg->payload.tile.row),
                FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].width,
                FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].height,
                FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].offset,
                msg->payload.tile.color_fg,
                msg->payload.tile.color_bg,
                true 
            );

            return true;

        case (LCD_CMD_DRAW_TILE_INVERTED):
            //Draw a tile at the specified location
            lcd_draw_rectangle(
                lcd_tile_left_x(msg->payload.tile.col), 
                lcd_tile_top_y(msg->payload.tile.row), 
                TILE_W, 
                TILE_H, 
                msg->payload.tile.color_fg, 
                false
            );

            // Draw the number on top of the tile
            lcd_draw_image(
                lcd_tile_center_x(msg->payload.tile.col),
                lcd_tile_center_y(msg->payload.tile.row),
                FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].width,
                FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].height,
                FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[msg->payload.tile.number].offset,
                msg->payload.tile.color_bg,
                msg->payload.tile.color_fg,
                true 
            );

            return true;


        case (LCD_CMD_CLEAR_SCREEN):
            // Clear the screen with the specified color
            lcd_clear_screen(msg->payload.tile.color_bg);

            return true;
        
        default:
            // Invalid command
            return false;
        
    }



    return false;
}
