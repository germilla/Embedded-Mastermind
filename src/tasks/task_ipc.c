/**
 * @file task_ipc.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-03
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "task_ipc.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"
#include "cyhal_uart.h"
#include "main.h"
#include "task_console.h"

#if defined(ECE353_FREERTOS)

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_uart_t IPC_Uart_Obj;
cyhal_uart_cfg_t IPC_Uart_Config =
{
    .data_bits = 8,
    .stop_bits = 1,
    .parity = CYHAL_UART_PARITY_NONE,
    .rx_buffer = NULL,
    .rx_buffer_size = 0
};

uint32_t IPC_Actual_Baud;

/**
 * @brief 
 * Simple checksum calculation function.  Take the XOR of all bytes
 * except the start and checksum bytes.
 * @param packet 
 * @return __inline 
 */
static __inline uint8_t calculate_checksum(ipc_packet_t *packet)
{
    uint8_t checksum = 0;
    for(int i = 1; i < sizeof(ipc_packet_t) - 1; i++)
    {
        checksum ^= ((uint8_t*)packet)[i];
    }
    return checksum;
}   

/**
 * @brief 
 * This function is used to send a "fire" command to the opponent
 * @param row 
 * @param col 
 * @return true 
 * @return false 
 */
bool ipc_send_fire(uint8_t row, uint8_t col)
{
    /* ADD CODE */
    return true;
}

/**
 * @brief 
 *  This function is used to send a "result" command to the opponent
 * @param result 
 * @return true 
 * @return false 
 */
bool ipc_send_result(ipc_result_t result)
{
    /* ADD CODE */
    return true;
}

/**
 * @brief 
 * This function is used to send a "game control" command to the opponent
 * @param control 
 * @return true 
 * @return false 
 */
bool ipc_send_game_control(ipc_game_control_t control)
{
    /* ADD CODE */
    return true;
}

/**
 * @brief 
 * This function is used to send an "error" command to the opponent
 * @param error 
 * @return true 
 * @return false 
 */
bool ipc_send_error(ipc_error_t error)
{
    /* ADD CODE */
    return true;
}

/**
 * @brief
 * Interrupt handler for the IPC UART. This function handles both RX and TX interrupts.
 *
 * @param handler_arg Pointer to handler arguments (not used).
 * @param event The UART event that triggered the interrupt.
 */
void ipc_event_handler(void *handler_arg, cyhal_uart_event_t event)
{
    (void)handler_arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    char c;
    static uint8_t raw_data_index = 0;

    if ((event & CYHAL_UART_IRQ_RX_NOT_EMPTY) == CYHAL_UART_IRQ_RX_NOT_EMPTY)
    {
        /* ADD CODE */
    }
    if ((event & CYHAL_UART_IRQ_TX_EMPTY) == CYHAL_UART_IRQ_TX_EMPTY)
    {
    }
    else
    {
    }
}

bool task_ipc_init(void)
{
    cy_rslt_t rslt;

    // Initialize the IPC UART
   rslt =  cyhal_uart_init(
        &IPC_Uart_Obj, 
        PIN_IPC_TX, 
        PIN_IPC_RX, 
        NC, 
        NC, 
        NULL, 
        &IPC_Uart_Config
    );
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    rslt = cyhal_uart_set_baud(&IPC_Uart_Obj, 115200, &IPC_Actual_Baud);
    if (rslt != CY_RSLT_SUCCESS)
    {
        return false; // Initialization failed
    }

    cyhal_uart_clear(&IPC_Uart_Obj);

    // Register the UART handler
    cyhal_uart_register_callback(&IPC_Uart_Obj, ipc_event_handler, NULL);

    // Enable Rx Interrupts
    cyhal_uart_enable_event(
        &IPC_Uart_Obj,
        CYHAL_UART_IRQ_RX_NOT_EMPTY,
        INT_PRIORITY_IPC,
        true
    );


    if(task_ipc_resources_init_rx() == false)
    {
        return false; // Initialization failed
    }

    if(task_ipc_resources_init_tx() == false)
    {
        return false; // Initialization failed
    }

    return true; // Initialization successful
}
#endif  