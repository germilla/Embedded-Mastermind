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

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/



void task_hw02_system_control(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

   
    /* Draw the initial master mind game board */

    /* Allow the user to select 4 numbers for their cypher */

    while(1)
    {
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

    /* Create the FreeRTOS queues */

    /* Create the FreeRTOS tasks */

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif