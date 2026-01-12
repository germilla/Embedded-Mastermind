/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE11)
#include "rtos_events.h"
#include "drivers.h"
#include "task_buttons.h"
#include "task_console.h"
#include "task_ipc.h"

char APP_DESCRIPTION[] = "ECE353: ICE 11 - FreeRTOS IPC Rx/Tx";

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
 * This task will be used to verify the functionality of the IPC UART specification
 * by ....
 *
 * @param arg 
 * Unused parameter
 */
void task_system_control(void *arg)
{
    (void)arg; // Unused parameter
    EventBits_t events;

    ipc_result_t result = IPC_RESULT_MISS;
    ipc_game_control_t game_control = IPC_GAME_CONTROL_NEW_GAME;
    ipc_error_t error = IPC_ERROR_CHECKSUM;

    while(1)
    {
        // Wait for SW1 or SW2 to be pressed
        events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_RTOS_EVENTS_SW1 | ECE353_RTOS_EVENTS_SW2 | ECE353_RTOS_EVENTS_SW3,
                                    pdTRUE,
                                    pdFALSE,
                                    portMAX_DELAY);

        if(events & ECE353_RTOS_EVENTS_SW1)
        {
            if(!ipc_send_result(result))
            {
                // Handle send failure (optional)
            }
            result++;

            if(result > IPC_RESULT_SUNK)
            {
                result = IPC_RESULT_MISS;
            }
        }
        else if(events & ECE353_RTOS_EVENTS_SW2)
        {
            if(!ipc_send_game_control(game_control))
            {
                // Handle send failure (optional)
            }
            game_control++;

            if(game_control > IPC_GAME_CONTROL_END_GAME)
            {
                game_control = IPC_GAME_CONTROL_NEW_GAME;
            }       
        }
        else if(events & ECE353_RTOS_EVENTS_SW3)
        {
            if(!ipc_send_error(error))
            {
                // Handle send failure (optional)
            }
            error++;

            if(error > IPC_ERROR_SYSTEM_FAILURE)
            {
                error = IPC_ERROR_CHECKSUM;
            }       
        }

    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    // Initialize the LEDs
    rslt = leds_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("LED initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    rslt = buttons_init_gpio();

    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

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
    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();

    if(!task_buttons_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_console_init())
    {
        printf("Console initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the System Control Task
    xTaskCreate(
        task_system_control, 
        "System Control", 
        configMINIMAL_STACK_SIZE*5,      
        NULL, 
        2, 
        NULL
    );

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif