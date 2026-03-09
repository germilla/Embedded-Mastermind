/**
 * @file task_lcd.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_lcd.h"
 #include "lcd-fonts.h"

#if defined(ECE353_FREERTOS)

/* FreeRTOS Queue for LCD messages */
static QueueHandle_t Queue_Requests = NULL;

/* LCD Task */
void task_lcd(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    lcd_msg_request_t lcd_request;
    lcd_msg_response_t response;
    bool status = false;
    int location;
    int width;

    printf("Starting LCD Task\n\r");

    while(1)
    {
        xQueueReceive(Queue_Requests, &lcd_request, portMAX_DELAY);

        switch (lcd_request.msg.command) {

            case (LCD_CMD_PRINT_MESSAGE):
                width = 0;

                // Run a for loop for each character in the message and print it to the screen
                for (int i = 0; i < 32; i++) {
                    if (lcd_request.msg.payload.message[i] == '\0') {
                        break;
                    }

                    // Convert character to ascii and get info from font info
                    location = ((int)lcd_request.msg.payload.message[i]) - Consolas_20ptFontInfo.start_char;
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

                status = true;
                break;

            case (LCD_CMD_DRAW_TILE):
                //Draw a tile at the specified location
                lcd_draw_rectangle(
                    lcd_tile_left_x(lcd_request.msg.payload.tile.col), 
                    lcd_tile_top_y(lcd_request.msg.payload.tile.row), 
                    TILE_W, 
                    TILE_H, 
                    lcd_request.msg.payload.tile.color_bg, 
                    false
                );
        
                // Draw the number on top of the tile
                lcd_draw_image(
                    lcd_tile_center_x(lcd_request.msg.payload.tile.col), 
                    lcd_tile_center_y(lcd_request.msg.payload.tile.row), 
                    FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].width,
                    FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].height,
                    FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].offset,
                    lcd_request.msg.payload.tile.color_fg,
                    lcd_request.msg.payload.tile.color_bg,
                    true 
                );
                
                status = true;
                break;

            case (LCD_CMD_DRAW_TILE_INVERTED):
                //Draw a tile at the specified location
                lcd_draw_rectangle(
                    lcd_tile_left_x(lcd_request.msg.payload.tile.col), 
                    lcd_tile_top_y(lcd_request.msg.payload.tile.row), 
                    TILE_W, 
                    TILE_H, 
                    lcd_request.msg.payload.tile.color_fg, 
                    false
                );
        
                // Draw the number on top of the tile
                lcd_draw_image(
                    lcd_tile_center_x(lcd_request.msg.payload.tile.col), 
                    lcd_tile_center_y(lcd_request.msg.payload.tile.row), 
                    FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].width,
                    FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].height,
                    FONT_NUM_LARGE_BITMAPS + FONT_CHAR_INFO_LARGE_NUMBERS[lcd_request.msg.payload.tile.number].offset,
                    lcd_request.msg.payload.tile.color_bg,
                    lcd_request.msg.payload.tile.color_fg,
                    true 
                );

                status = true;
                break;

            case (LCD_CMD_CLEAR_SCREEN):
                // Clear the screen with the specified color
                lcd_clear_screen(lcd_request.msg.payload.tile.color_bg);

                status = true;
                break;
            
            case LCD_CMD_PRINT_SW1_COUNT:
                printf("LCD Task: Received request to print SW1 count: %s\n", lcd_request.msg.payload.message);

                // Print to screeen at (10, 50)
                width = 10;
                
                for (int i = 0; i < 32; i++) {
                    if (lcd_request.msg.payload.message[i] == '\0') {
                        break;
                    }

                    // Convert character to ascii and get info from font info
                    location = ((int)lcd_request.msg.payload.message[i]) - Consolas_20ptFontInfo.start_char;
                    FONT_CHAR_INFO info = Consolas_20ptDescriptors[location];

                    // Draw the character to the screen
                    lcd_draw_image(
                        width, 
                        50, 
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

                status = true;
                break;
            
            case LCD_CMD_PRINT_SW2_COUNT:
                printf("LCD Task: Received request to print SW2 count: %s\n", lcd_request.msg.payload.message);

                // Print to screeen at (10, 100)
                width = 10;
                
                for (int i = 0; i < 32; i++) {
                    if (lcd_request.msg.payload.message[i] == '\0') {
                        break;
                    }

                    // Convert character to ascii and get info from font info
                    location = ((int)lcd_request.msg.payload.message[i]) - Consolas_20ptFontInfo.start_char;
                    FONT_CHAR_INFO info = Consolas_20ptDescriptors[location];

                    // Draw the character to the screen
                    lcd_draw_image(
                        width, 
                        100, 
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

                status = true;
                break;

            default:
                printf("LCD Task: Received invalid command\n");
                break;
        }

    }
}

/* LCD Task Initialization */
bool task_lcd_resources_init(QueueHandle_t queue_request){

    BaseType_t result;

    if (queue_request == NULL)
    {
        return false;
    }
    Queue_Requests = queue_request;

    /* Create the LCD Task */
    result= xTaskCreate(
        task_lcd,                       // Task function
        "LCD Task",                     // Task name
        TASK_LCD_STACK_SIZE,            // Stack size
        NULL,                           // Task parameters
        TASK_LCD_PRIORITY,              // Task priority
        NULL                            // Task handle
    );

    if(result != pdPASS)
    {
        return false;
    }   

    return true;
}
#endif