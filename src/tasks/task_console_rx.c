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
#include "task_eeprom.h"
#include "task_cap_touch.h"

#ifdef ECE353_FREERTOS
#include "drivers.h"
#include "task_console.h"
#include "cyhal_uart.h"
#include "devices.h"
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

device_request_msg_t request;
QueueHandle_t response_queue;

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

    device_response_msg_t response;
    char* data;

    while (1)
    {
        /* ADD CODE */
        // Wait for a task notification from the ISR
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        data = consume_console_buffer->data;

        // Process the data in the console buffer pointer
        if (strcmp(data, "RED ON") == 0)
        {
            // Turn on the red LED
            cyhal_gpio_write(PIN_LED_RED, 1);
        }
        else if (strcmp(data, "RED OFF") == 0)
        {
            // Turn off the red LED
            cyhal_gpio_write(PIN_LED_RED, 0);
        }
        else if (strncmp(data, "EEPROM R", 8) == 0) {
            // Parse request and setup queue for response
            request.response_queue = response_queue;
            if (!parse_cli_data(data, &request)) {
                task_console_printf("Failed to parse console command: %s", data);
                continue;
            }

            // Send the request to the EEPROM task
            xQueueSend(Queue_EEPROM_Requests, &request, portMAX_DELAY);

            // Wait for the response from the EEPROM task
            xQueueReceive(response_queue, &response, portMAX_DELAY);

            // Parse the response and print the value read from the EEPROM
            task_console_printf("Op: READ, Addr: 0x%04X, Val: 0x%02X\n", request.address, response.payload.eeprom);
        }
        else if (strncmp(data, "EEPROM W", 8) == 0) {
            // Parse request and setup queue for response
            request.response_queue = response_queue;
            if (!parse_cli_data(data, &request)) {
                task_console_printf("Failed to parse console command: %s", data);
                continue;
            }

            // Print the request to the console
            task_console_printf("Op: WRITE, Addr: 0x%04X, Val: 0x%02X\n", request.address, request.value);

            // Send the request to the EEPROM task
            xQueueSend(Queue_EEPROM_Requests, &request, portMAX_DELAY);

            // Wait for the response from the EEPROM task
            xQueueReceive(response_queue, &response, portMAX_DELAY);
        }
        else if (strncmp(data, "CAP_TOUCH", 9) == 0)
        {
            // Send a request to the capacitive touch task to read the current state of the buttons
            // and print the state to the console
            request.response_queue = response_queue;
            if (!parse_cli_data(data, &request)) {
                task_console_printf("Failed to parse console command: %s", data);
                continue;
            }
            
            // Send the request to the capacitive touch task
            xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY);

            // Wait for the response from the capacitive touch task
            xQueueReceive(response_queue, &response, portMAX_DELAY);

            // Check if the response was successful
            if (response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
            {
                task_console_printf("Failed to read Capacitive Touch data!\r\n");
                continue;
            }

            // Parse the response and print the state of the capacitive touch buttons
            task_console_printf("Cap Touch: Sensor 0=%d, Sensor 1=%d", response.payload.cap_touch[0], response.payload.cap_touch[1]);
        }
        else
        {
            task_console_printf("Unsupported command: %s\n", data);
        }

         // Reset the index for the consume buffer

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
    console_buffer1.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);
    console_buffer2.data = (char *)pvPortMalloc(CONSOLE_MAX_MESSAGE_LENGTH);

    // Initialize the produce and consume buffer pointers
    produce_console_buffer = &console_buffer1;
    consume_console_buffer = &console_buffer2;

    // Initialize the index for each console buffer
    produce_console_buffer->index = 0;
    consume_console_buffer->index = 0;

     // Create the response queue
    response_queue = xQueueCreate(1, sizeof(device_response_msg_t));
    
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