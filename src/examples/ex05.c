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

#if defined(EX05)

#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 05 - FreeRTOS Tasks";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
volatile bool buzzer_enable = false;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
void task_button_sw1(void *arg);
void task_button_sw2(void *arg);
void task_buzzer2(void *arg);

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_button_sw1(void *arg)
{
    (void)arg;
    uint32_t button_count = 0;
    printf("SW1 Task Started\r\n");
    while(1)
    {
        // Check the button
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0) {
            button_count++;

             if (button_count == 2) {
                printf("SW1 Pressed\r\n");

                // Set global variable
                buzzer_enable = true;
             }
        }
        else {
            button_count = 0;
        }

        // Delay 15ms
        vTaskDelay(pdMS_TO_TICKS(15));

    }
}

void task_button_sw2(void *arg)
{
    (void)arg;
    uint32_t button_count = 0;
    printf("SW2 Task Started\r\n");
    while(1)
    {
        // Check the button
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0) {
            button_count++;

             if (button_count == 2) {
                printf("SW2 Pressed\r\n");

                // Set global variable
                buzzer_enable = false;
             }
        }
        else {
            button_count = 0;
        }

        // Delay 15ms
        vTaskDelay(pdMS_TO_TICKS(15));

    }
}

void task_buzzer2(void *arg)
{
    (void)arg;
    printf("BUZZER Task Started\r\n");
    while(1)
    {
        // Delay 100ms
        vTaskDelay(pdMS_TO_TICKS(100));

        if(buzzer_enable)
        {
            printf("BUZZER ON\r\n");
            buzzer_on();
        }
        else
        {
            printf("BUZZER OFF\r\n");
            buzzer_off();
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

    /* Initialize the buttons */
    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buttons GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }
    
     /* Initialize the timer for button debouncing */
    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Error initializing button timer\n\r");
        for(int i = 0; i < 100000; i++);
        CY_ASSERT(0);
    }

    /* Initialize the buzzer */
    buzzer_init(50.0, 2000);
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
    /* Register the tasks with FreeRTOS*/
    xTaskCreate(task_button_sw1, "SW1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_button_sw2, "SW2", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(task_buzzer2, "BUZZER", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif