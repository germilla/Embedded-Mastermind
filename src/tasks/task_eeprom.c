/**
 * @file task_eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-17
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
 #include "task_eeprom.h"
 #include "task_console.h"

 /* Global Variables */
QueueHandle_t Queue_EEPROM_Requests;
static SemaphoreHandle_t *SPI_Semaphore = NULL;
static cyhal_spi_t *eeprom_spi_obj = NULL;
static cyhal_gpio_t eeprom_cs_pin = NC;

/**
 * @brief 
 *  This function acts as the published interface to write data to the EEPROM.
 *  It will format the device request message and send it to the EEPROM task.
 *
 *  If the user provides a valid return queue, this function will wait for
 *  the response from the EEPROM task before returning.  If the user provides
 *  a NULL return queue, this function will return immediately after sending
 *  the request.
 * @param return_queue 
 * @param address 
 * @param data 
 * @param length 
 * @return true 
 * @return false 
 */
bool system_sensors_eeprom_write(QueueHandle_t return_queue, uint16_t address, uint8_t data)
{
    device_request_msg_t request;
    device_response_msg_t response;

    // Setup the request message
    request.device = DEVICE_EEPROM;
    request.operation = DEVICE_OP_WRITE;
    request.address = address;
    request.value = data;
    request.response_queue = return_queue;

    // Send the request to the EEPROM task
    xQueueSend(Queue_EEPROM_Requests, &request, portMAX_DELAY);

    if (return_queue == NULL)
    {
        // If no return queue was provided, return immediately
        return true;
    }

    // Receive the response from the EEPROM task if a return queue was provided
    xQueueReceive(return_queue, &response, portMAX_DELAY);

    return (response.status == DEVICE_OPERATION_STATUS_WRITE_SUCCESS);
}   

/**
 * @brief 
 * This function is the published interface to read data from the EEPROM.
 * It will format the device request message and send it to the EEPROM task.
 *
 * The value read from the EEPROM will be returned via the data pointer
 *
 * @param return_queue 
 * @param address 
 * @param data 
 * @return true 
 * @return false 
 */
bool system_sensors_eeprom_read(QueueHandle_t return_queue, uint16_t address, uint8_t *data)
{
    device_request_msg_t request;
    device_response_msg_t response;

    if(return_queue == NULL || data == NULL)
    {
        return false;
    }

    // Setup the request message
    request.device = DEVICE_EEPROM;
    request.operation = DEVICE_OP_READ;
    request.address = address;
    request.response_queue = return_queue;
    
    // Send the request to the EEPROM task
    xQueueSend(Queue_EEPROM_Requests, &request, portMAX_DELAY);
    
    // Receive the response from the EEPROM task
    xQueueReceive(return_queue, &response, portMAX_DELAY);

    if (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS)
    {
        *data = response.payload.eeprom;
    }

    return (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS);
}

/**
 * @brief 
 *  Task used to monitor the reception of command packets sent the EEPROM
 * @param arg 
 */
void task_eeprom(void *arg)
 {
    (void) arg;
    device_request_msg_t request_packet;
    device_response_msg_t response_packet;

    task_console_printf("Starting EEPROM Task\r\n");

    while(1)
    {
        // Wait for a request to arrive
        xQueueReceive(
            Queue_EEPROM_Requests, 
            &request_packet, 
            portMAX_DELAY
        );

        response_packet.device = DEVICE_EEPROM;

        // Check if the request is a read or write and perform the appropriate action
        if (request_packet.operation == DEVICE_OP_READ)
        {
            xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);
            response_packet.payload.eeprom = eeprom_read_byte(eeprom_spi_obj, eeprom_cs_pin, request_packet.address);
            xSemaphoreGive(*SPI_Semaphore);
            response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
        }
        else if (request_packet.operation == DEVICE_OP_WRITE)
        {
            xSemaphoreTake(*SPI_Semaphore, portMAX_DELAY);
            eeprom_write_byte(eeprom_spi_obj, eeprom_cs_pin, request_packet.address, request_packet.value);
            xSemaphoreGive(*SPI_Semaphore);
            response_packet.status = DEVICE_OPERATION_STATUS_WRITE_SUCCESS;
        }

        // Send the response back to the requester
        if (request_packet.response_queue != NULL)
        {
            xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
        }
    }
}


/**
 * @brief 
 * Function used to initialize resources for the EEPROM task
 * @param spi_semaphore 
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool task_eeprom_resources_init(SemaphoreHandle_t *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
 {
    if(spi_semaphore == NULL || spi_obj == NULL || cs_pin == NC)
    {
        return false;
    }

    // Save handles for access to the SPI peripheral and semaphore
    SPI_Semaphore = spi_semaphore;
    eeprom_spi_obj = spi_obj;
    eeprom_cs_pin = cs_pin;

    /*Create the EEPROM Requests Queue */
    Queue_EEPROM_Requests = xQueueCreate(1, sizeof(device_request_msg_t ));

    /* Create the FreeRTOS task for the EEPROM */
    if (xTaskCreate(
        task_eeprom, 
        "EEPROM", 
        10*configMINIMAL_STACK_SIZE, 
        spi_semaphore, 
        tskIDLE_PRIORITY + 1, 
        NULL) != pdPASS)
    {
        return false;
    }
    return true;
 }

#endif /* ECE353_FREERTOS */    