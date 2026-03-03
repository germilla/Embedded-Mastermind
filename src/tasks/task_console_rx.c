/**
 * @file task_console_rx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-21
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "cyhal_uart.h"
/**
 * @brief
 * This file contains the implementation of the console receive (Rx) task.
 * The task is responsible for processing incoming console commands and
 * controlling the state of the LEDs accordingly.
 * 
 * The task uses a double buffer to process the incoming console commands.
 * The supported commands will be "RED_ON" and "RED_OFF" to control the red LED.
 */

/* ADD CODE */
/* Global Variables */
console_buffer_t console_buffer1;
console_buffer_t console_buffer2;

console_buffer_t *produce_console_buffer;
console_buffer_t *consume_console_buffer;

TaskHandle_t TaskHandle_Console_Rx;

/**
 * @brief
 * This function is the bottom half task for receiving console input.
 *
 * It waits for a task notification from the ISR indicating that a new 
 * command has been received. The task then processes the command and 
 * controls the state of the LEDs accordingly.
 *
 * @param param Unused parameter
 */
void task_console_rx(void *param)
{
    (void)param; // Unused parameter
    while (1)
    {
        /* ADD CODE */
        // Wait for a task notification from the ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        // Process the data in the console buffer pointer
        if (strcmp(consume_console_buffer->data, "RED ON") == 0)
        {
            // Turn on the red LED
            cyhal_gpio_write(PIN_LED_RED, 1);
        }
        else if (strcmp(consume_console_buffer->data, "RED OFF") == 0)
        {
            // Turn off the red LED
            cyhal_gpio_write(PIN_LED_RED, 0);
        }

    }
}

/**
 * @brief
 * This function initializes the resources for the console Rx task.
 * @return true if resources were initialized successfully
 * @return false if resource initialization failed
 */
bool task_console_resources_init_rx(void)
{
    BaseType_t rslt;

    /* ADD CODE */
    // Allocate an array of dta from the heap for each console buffer
    console_buffer1.data = (char *)pvPortMalloc(sizeof(CONSOLE_MAX_MESSAGE_LENGTH));
    console_buffer2.data = (char *)pvPortMalloc(sizeof(CONSOLE_MAX_MESSAGE_LENGTH));

    // Initialize the produce and consume buffer pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    // Initialize the index for each console buffer
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

    // Create the console Rx task
    rslt = xTaskCreate(
        task_console_rx,           // Task function
        "Console Rx Task",         // Name of the task (for debugging)
        configMINIMAL_STACK_SIZE,  // Stack size in words
        NULL,                      // Task parameter
        tskIDLE_PRIORITY + 1,      // Task priority
        &TaskHandle_Console_Rx     // Task handle
    );

    return (rslt == pdPASS); // Resources initialized successfully
}
#endif