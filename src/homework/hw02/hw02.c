 /**
 * @file hw02.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw02.h"

#if defined(HW02)

char APP_DESCRIPTION[] = "ECE353 S26 HW02";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
const char *Joystick_Pos_Strings[] = {
    [JOYSTICK_POS_CENTER] = "Center",
    [JOYSTICK_POS_LEFT] = "Left",
    [JOYSTICK_POS_RIGHT] = "Right",
    [JOYSTICK_POS_UP] = "Up",
    [JOYSTICK_POS_DOWN] = "Down",
    [JOYSTICK_POS_UPPER_LEFT] = "Upper Left",
    [JOYSTICK_POS_UPPER_RIGHT] = "Upper Right",
    [JOYSTICK_POS_LOWER_LEFT] = "Lower Left",
    [JOYSTICK_POS_LOWER_RIGHT] = "Lower Right"
};
EventGroupHandle_t ECE353_RTOS_Events;
QueueHandle_t xQueue_Request_LCD;
int selected_cypher_digit, cypher_number;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

// Helper function to update the selected number and redraw the tile with the new number, and unselect the previous tile
void number_select(int before, int after) {
    lcd_msg_request_t lcd_request;

    // Unselect the previous tile
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.row = 1 + (before / 4);
    lcd_request.msg.payload.tile.col = (before % 4);
    lcd_request.msg.payload.tile.number = before;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    // Select the new tile
    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    lcd_request.msg.payload.tile.row = 1 + (after / 4);
    lcd_request.msg.payload.tile.col = (after % 4);
    lcd_request.msg.payload.tile.number = after;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

}

void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    printf("Starting System Control Task\n\r");
   
    /* Draw the initial master mind game board */

    /* Allocate a lcd_msg_request_t variable */
    lcd_msg_request_t lcd_request;

    /* Write a message to the user in the Text Area of the screen*/
    lcd_request.msg.command = LCD_CMD_PRINT_MESSAGE;
    lcd_request.return_queue = NULL;
    snprintf(lcd_request.msg.payload.message, 32, "Select Your Cypher!");
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    /* Draw 4 blank tiles for the secret code */
    for(int col = 0; col < 4; col++)
    {
        lcd_request.msg.command = LCD_CMD_DRAW_TILE;
        lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        lcd_request.msg.payload.tile.col = col;
        lcd_request.msg.payload.tile.number = 0; // number is ignored for code tiles
        lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
        lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    }


    /* Draw numbers 0-7 for the user input*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    for (int row = 1; row < 3; row++) {
        for(int col = 0; col < 4; col++) {
            lcd_request.msg.payload.tile.row = row;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = col+(row-1)*4;

            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }

    /* Draw the number 0 as the selected user input and highlight first cypher digit*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_NUM_0_3;
    lcd_request.msg.payload.tile.col = 0;
    lcd_request.msg.payload.tile.number = 0;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    
    /* Allow the user to select 4 numbers for their cypher */

    selected_cypher_digit = 0;
    joystick_position_t joystick_position;
    bool joystick_moved = false;

    // If 4 digits have been selected, ignore joystick input
    while(cypher_number < 4) {
        // Wait for a message from the joystick task with the selected direction
        xQueueReceive(Queue_Joystick, &joystick_position, portMAX_DELAY);

        // Find the position of the currently selected number and unselect the previous tile
        switch(joystick_position) {
            case JOYSTICK_POS_UP:
                printf("Joystick Up\n\r");
                if (!joystick_moved && selected_cypher_digit > 3) {
                    // Move the selection up to the next number
                    number_select(selected_cypher_digit, selected_cypher_digit - 4);
                    selected_cypher_digit -= 4;
                    joystick_moved = true;
                }
                break;
            case JOYSTICK_POS_DOWN:
                printf("Joystick Down\n\r");
                if (!joystick_moved && selected_cypher_digit < 4) {
                    // Move the selection down to the next number
                    number_select(selected_cypher_digit, selected_cypher_digit + 4);
                    selected_cypher_digit += 4;
                    joystick_moved = true;
                }
                break;
            case JOYSTICK_POS_LEFT:
                printf("Joystick Left\n\r");
                if (!joystick_moved && (selected_cypher_digit % 4) > 0) {
                    // Move the selection left to the next number
                    number_select(selected_cypher_digit, selected_cypher_digit - 1);
                    selected_cypher_digit -= 1;
                    joystick_moved = true;
                }
                break;
            case JOYSTICK_POS_RIGHT:
                printf("Joystick Right\n\r");
                if (!joystick_moved && (selected_cypher_digit % 4) < 3) {
                    // Move the selection right to the next number
                    number_select(selected_cypher_digit, selected_cypher_digit + 1);
                    selected_cypher_digit += 1;
                    joystick_moved = true;
                }
                break;
            case JOYSTICK_POS_CENTER:
                printf("Joystick Center\n\r");
                joystick_moved = false; // Reset the joystick moved flag when the joystick is released
                break;
            default:
                printf("Invalid Joystick Position: %s\n\r", Joystick_Pos_Strings[joystick_position]);
            }
    }
}

// Handles clicking of the button and updates the screen accordingly
void button_handler() {
    cypher_number = 0;

    while (1) {
        // Wait for xEvent SW1 to be set
        xEventGroupWaitBits(ECE353_RTOS_Events, ECE353_BUTTON_1_PRESSED, pdTRUE, pdFALSE, portMAX_DELAY);
        
        if (cypher_number >= 4) {
            continue; // Ignore button presses after 4 digits have been selected
        }

        // When SW1 is pressed, update the currently selected cypher digit to the next number and redraw the tile with the new number
        lcd_msg_request_t lcd_request;
        lcd_request.msg.command = LCD_CMD_DRAW_TILE;
        lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        lcd_request.msg.payload.tile.col =  cypher_number;
        lcd_request.msg.payload.tile.number = selected_cypher_digit;
        lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
        lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

        cypher_number++;
        if (cypher_number >= 4) {
            continue; // Ignore button presses after 4 digits have been selected
        }

        // Highlight the next value
        lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        lcd_request.msg.payload.tile.col = cypher_number;
        lcd_request.msg.payload.tile.number = 0;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    }

}

/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 ************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    // Set text color to black
    printf("\x1b[30m");
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
        CY_ASSERT(0);
    }

    /* Initialize the joystick */
    joystick_init();

    /* Initialize the buttons*/
    buttons_init_gpio();

}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    /* Create the event group for RTOS events */
    ECE353_RTOS_Events = xEventGroupCreate();

    /* Create the FreeRTOS queues */
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));

    /* Create the FreeRTOS tasks */
    task_button_init();
    task_joystick_init();
    task_lcd_resources_init(xQueue_Request_LCD);
    xTaskCreate(task_hw02_system_control, "System Control Task", TASK_SYSTEM_CONTROL_STACK_SIZE, NULL, TASK_SYSTEM_CONTROL_PRIORITY, NULL);
    xTaskCreate(button_handler, "Button Handler Task", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL);

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif