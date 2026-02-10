/**
 * @file hw01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-06
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "hw01.h"

#if defined(HW01)

char APP_DESCRIPTION[] = "ECE353 Spring 2026: HW01 - Homework 1";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the HW01 application
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    rslt = lcd_initialize();

    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("!!!Error initializing LCD: %lu\n", rslt);
        CY_ASSERT(0);
    }
    else
    {
        printf("* LCD initialized successfully.\r\n");
    }
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the HW01 application
 */
void app_main(void)
{
   
    /* Allocate a lcd_msg_t variable */
    lcd_msg_t msg;

    /* Write a message to the user in the Text Area of the screen*/
    msg.command = LCD_CMD_PRINT_MESSAGE;
    snprintf(msg.payload.message, 32, "ECE353 HW01 S26");
    master_mind_handle_msg(&msg);


    /* Draw 4 blank tiles for the secret code */
    for(int col = 0; col < 4; col++)
    {
        msg.command = LCD_CMD_DRAW_TILE;
        msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        msg.payload.tile.col = col;
        msg.payload.tile.number = 0; // number is ignored for code tiles
        msg.payload.tile.color_fg = LCD_COLOR_RED;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        master_mind_handle_msg(&msg);
    }


    /* Draw numbers 0-3 for the user input*/
    for(int col = 0; col < 4; col++)
    {
        msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        msg.payload.tile.row = LCD_TILE_ROW_NUM_0_3;
        msg.payload.tile.col = col;
        msg.payload.tile.number = col;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;

        master_mind_handle_msg(&msg);
    }

    /* Draw the numbers for 4-7 for the user input*/
    for(int col = 0; col < 4; col++)
    {
        msg.command = LCD_CMD_DRAW_TILE_INVERTED;
        msg.payload.tile.row = LCD_TILE_ROW_NUM_4_7;
        msg.payload.tile.col = col;
        msg.payload.tile.number = col + 4;
        msg.payload.tile.color_fg = LCD_COLOR_GREEN;
        msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        master_mind_handle_msg(&msg);
    }

    while(1)
    {
    }
}
#endif