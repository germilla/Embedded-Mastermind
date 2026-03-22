/**
 * @file task_ipc_tx.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "circular_buffer.h"
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "task_ipc.h"

/* Global Variables */
TaskHandle_t TaskHandle_IPC_Tx = NULL;

/* FreeRTOS Queue used to send packets to the IPC Tx Task */
QueueHandle_t Queue_IPC_Tx; 

/**
 * @brief
 * This task is used to process outgoing IPC packets.  The task will block
 * on a FreeRTOS Queue.  When a packet is received, the task transmit 1 byte
 * at a time in a polling manner.
 *
 * @param param
 * Unused parameter
 */
void task_ipc_tx(void *param)
{
    ipc_packet_t packet;

    while(1)
    {
        /* ADD CODE */

        /* Receive the IPC packet from the queue */
        xQueueReceive(Queue_IPC_Tx, &packet, portMAX_DELAY);

        /* Transmit the IPC packet 1-byte at a time */
        uint8_t *packet_bytes = (uint8_t*)&packet;
        for(int i = 0; i < sizeof(ipc_packet_t); i++)
        {
            /* Transmit the byte */
            cy_rslt_t tx_status = cyhal_uart_putc(&IPC_Uart_Obj, packet_bytes[i]);

            /* Wait for the Tx buffer to be empty before sending the next byte */
            while(tx_status != CY_RSLT_SUCCESS) {
                tx_status = cyhal_uart_putc(&IPC_Uart_Obj, packet_bytes[i]);
            }
        }
    }
}

bool task_ipc_resources_init_tx(void)
{
    /* Create the FreeRTOS Queue */
    Queue_IPC_Tx = xQueueCreate(IPC_TX_QUEUE_LENGTH, sizeof(ipc_packet_t));
    if(Queue_IPC_Tx == NULL)
    {
        return false;
    }

    /* Start the IPC Tx Task */
    BaseType_t task_ipc_tx_status = xTaskCreate(
        task_ipc_tx,                 // Function that implements the task.
        "IPC Tx Task",               // Text name for the task.
        IPC_STACK_SIZE,             // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,               // Priority at which the task is created.
        &TaskHandle_IPC_Tx          // Used to pass out the created task's handle.
    );

    if(task_ipc_tx_status != pdPASS)
    {
        return false;
    }
    else {
        return true; // Resources initialized successfully
    }
}

#endif