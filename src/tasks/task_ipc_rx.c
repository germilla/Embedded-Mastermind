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

        printf("Packet received with bits:");
        for(int i = 0; i < sizeof(ipc_packet_t); i++)
        {
            printf(" 0x%02X", ((uint8_t*)IPC_Rx_Consume_Buffer)[i]);
        }
        printf("\n\r");

        if(validate_packet((ipc_packet_t *)IPC_Rx_Consume_Buffer) == true) 
        {
            
            // Process the received IPC packet
            switch  (((ipc_packet_t *)IPC_Rx_Consume_Buffer)->cmd) {
                case IPC_CMD_DISCOVERY:
                    printf("Received Discovery Command\r\n");
                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                case IPC_CMD_ACTIVE_PLAYER:
                    printf("Received Active Player Command\r\n");
                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                case IPC_CMD_INACTIVE_PLAYER:
                    printf("Received Inactive Player Command\r\n");
                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                case IPC_CMD_STATUS:
                    printf("Received Status Command\r\n");
                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                case IPC_CMD_ACK:
                    printf("Received ACK Command\r\n");

                    // Set event group bits
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED);
                    break;
                case IPC_CMD_NUMBER:
                    printf("Received Number Command: 0x%08lX\r\n", (unsigned long)((ipc_packet_t *)IPC_Rx_Consume_Buffer)->payload.number);

                    // Set event group bits
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED);

                    // Store the received number in a global variable for use in the system control task
                    Sent_Code = ((ipc_packet_t *)IPC_Rx_Consume_Buffer)->payload.number;

                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                case IPC_CMD_NEW_GAME:
                    printf("Received New Game Command\r\n");
                    /* Signal the system control task that the other board wants a new game */
                    xEventGroupSetBits(ECE353_RTOS_Events, ECE353_RTOS_EVENTS_IPC_NEW_GAME);
                    ipc_send_ack(((ipc_packet_t *)IPC_Rx_Consume_Buffer)->sequence_num);
                    break;
                default:
                    printf("Received Unknown Command\r\n");
            }
        }
        else {
            printf("Invalid IPC packet received!\n\r");
        }
    }
}

bool task_ipc_resources_init_rx(void)
{
    // Create the IPC Rx Task
    BaseType_t task_ipc_rx_status = xTaskCreate(
        task_ipc_rx,                 // Function that implements the task.
        "IPC Rx Task",               // Text name for the task.
        IPC_STACK_SIZE,             // Stack size in words, not bytes.
        NULL,                       // Parameter passed into the task.
        IPC_PRIORITY,               // Priority at which the task is created.
        &TaskHandle_IPC_Rx          // Used to pass out the created task's handle.
    );

    if(task_ipc_rx_status != pdPASS)
    {
        return false;
    }

    return true;    
}

#endif