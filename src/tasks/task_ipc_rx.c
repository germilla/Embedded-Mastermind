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
#include <stdbool.h>

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"
#include "task_console.h"

/* Globals */
TaskHandle_t TaskHandle_IPC_Rx = NULL;

/* Use a double buffering strategy for IPC packets */
static volatile ipc_packet_t IPC_Rx_Buffer0;
static volatile ipc_packet_t IPC_Rx_Buffer1;

volatile ipc_packet_t* volatile IPC_Rx_Produce_Buffer = &IPC_Rx_Buffer0;
volatile ipc_packet_t* volatile IPC_Rx_Consume_Buffer = &IPC_Rx_Buffer1;

/**
 * @brief
 *
 * This task is used to process received IPC packets.  The task will block
 * on a FreeRTOS Task Notification.  When a notification is received,
 * the task will process the IPC packet stored in the consume buffer.
 *
 * For validation purposes, the task will print out the contents of the
 * received IPC packet to the console.
 * 
 * @param arg
 * Unused parameter
 */
void task_ipc_rx(void *param)
{
    while(1)
    {
        // Wait for a FreeRTOS Task Notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* ADD CODE */
    }
}

bool task_ipc_resources_init_rx(void)
{
    // Create the IPC Rx Task
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,                 // Function that implements the task.
        "IPC Rx Task",               // Text name for the task.
        5*configMINIMAL_STACK_SIZE,    // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        tskIDLE_PRIORITY + 1,       // Priority at which the task is created.
        &TaskHandle_IPC_Rx          // Used to pass out the created task's handle.
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;    
}

#endif