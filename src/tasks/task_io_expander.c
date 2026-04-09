/**
 * @file io_expander.c
 * @author your name (you@domain.com)
 * @brief
 * @version 0.1
 * @date 2023-09-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "main.h"
#if defined(ECE353_FREERTOS)
#include "cyhal_gpio.h"
#include "task_io_expander.h"
#include "task_console.h"
#include "rtos_events.h"
#include "devices.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
static void task_io_expander(void *param);
static void handler_io_expander_button(void *arg, cyhal_gpio_event_t event);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
static cyhal_i2c_t *I2C_Obj;
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to io expander */
QueueHandle_t Queue_IO_Expander_Requests;

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

bool system_sensors_io_expander_write(QueueHandle_t return_queue, uint8_t address, uint8_t value)
{
	device_request_msg_t request_packet;

	// Validate the address
	if (address != IOXP_ADDR_INPUT_PORT &&
		address != IOXP_ADDR_OUTPUT_PORT &&
		address != IOXP_ADDR_POLARITY &&
		address != IOXP_ADDR_CONFIG)
	{
		printf("Invalid IO Expander Address: 0x%X\r\n", address);
		return false;
	}

	// Send a request to the io expander task
	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_WRITE;
	request_packet.response_queue = return_queue;
	request_packet.address = address;
	request_packet.value = value;
	xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);
	
	return true;

}

bool system_sensors_io_expander_read(QueueHandle_t return_queue, uint8_t address, uint8_t *value)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	if(return_queue == NULL || value == NULL)
	{
		return false;
	}

	// Validate the address
	if (address != IOXP_ADDR_INPUT_PORT &&
		address != IOXP_ADDR_OUTPUT_PORT &&
		address != IOXP_ADDR_POLARITY &&
		address != IOXP_ADDR_CONFIG)
	{
		printf("Invalid IO Expander Address: 0x%X\r\n", address);
		return false;
	}

	// Send a request to the io expander task
	request_packet.device = DEVICE_IO_EXP;
	request_packet.operation = DEVICE_OP_READ;
	request_packet.address = address;
	request_packet.response_queue = return_queue;
	xQueueSend(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);

	// Wait for the response from the io expander task
	xQueueReceive(return_queue, &response_packet, portMAX_DELAY);

	// Return the value to the caller
	*value = response_packet.payload.io_expander;

	// Check the response status
	if (response_packet.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
	{
		return false;
	}

	return true;
}

/**
 * @brief
 * Task used to monitor the reception of command packets sent the io expander
 * @param param
 * Unused
 */
void task_io_expander(void *param)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;

	task_console_printf("Starting IO Expander Task\r\n");

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_IO_Expander_Requests, &request_packet, portMAX_DELAY);

		// Process the message
		switch (request_packet.operation)
		{
			case DEVICE_OP_WRITE:
				// Grab I2C semaphore
				xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

				// Write the value to the specified address
				cy_rslt_t rslt = i2c_write_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, request_packet.value);
				if (rslt != CY_RSLT_SUCCESS)
				{
					task_console_printf("Error writing to IO Expander: %d\r\n", rslt);
				}

				// Release I2C semaphore
				xSemaphoreGive(*I2C_Semaphore);
				break;

			case DEVICE_OP_READ:
				// Grab I2C semaphore
				xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

				// Read the value from the specified address
				rslt = i2c_read_u8(I2C_Obj, TCA9534_SUBORDINATE_ADDR, request_packet.address, &response_packet.payload.io_expander);
				if (rslt != CY_RSLT_SUCCESS)
				{
					task_console_printf("Error reading from IO Expander: %d\r\n", rslt);
				}

				// Release I2C semaphore
				xSemaphoreGive(*I2C_Semaphore);

				// Return the response to the caller
				response_packet.device = DEVICE_IO_EXP;
				response_packet.status = (rslt == CY_RSLT_SUCCESS) ? DEVICE_OPERATION_STATUS_READ_SUCCESS : DEVICE_OPERATION_STATUS_READ_FAILURE;
				xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
				break;

			default:
				task_console_printf("Invalid operation for IO Expander: %d\r\n", request_packet.operation);
				break;
		}

	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the IO Expander.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_io_expander_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;
	if (I2C_Semaphore == NULL)
	{
		return false;
	}

	/* Create the Queue used to control blinking of the status LED*/
	Queue_IO_Expander_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_IO_Expander_Requests == NULL)
	{
		return false;
	}

	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_io_expander,
		"Task IO Exp",
		configMINIMAL_STACK_SIZE,
		i2c_semaphore,
		tskIDLE_PRIORITY + 1,
		NULL) != pdPASS)
	{
		return false;
	}
	else 
	{
		return true;
	}	
}	
#endif