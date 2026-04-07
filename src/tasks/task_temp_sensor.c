/**
 * @file task_temp_sensor.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "cy_result.h"
#include "main.h"

#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "task_temp_sensor.h"
#include "task_console.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/
/* I2C Object Handle */
static cyhal_i2c_t *I2C_Obj;

/* I2C Semaphore */
static SemaphoreHandle_t *I2C_Semaphore = NULL;

/* Queue used to send commands used to temp sensor */
QueueHandle_t Queue_Temp_Sensor_Requests;

/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/

/** Read the value of the input port
 *
 * @param reg The reg address to read
 *
 */
static float LM75_get_temp(void)
{
	float temp = 0;
	uint16_t raw_value =  0;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;

	// Read 2-bytes from the temperature register
	rslt = i2c_read_u16(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_TEMP_REG, (uint16_t*) &raw_value);

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_console_printf("Error reading from LM75: %d\r\n", rslt);
		return 0;
	}

	// Need to format the raw value read from the sensor
	// The LM75 returns a 9-bit, two's complement value
	temp = (float) (raw_value >> 7); // Shift the raw value to the right by 7 bits to get rid of unused bits and to align the value to the least significant bit

	// Convert the raw value to degrees Celsius
	// Each bit is worth 0.5 degrees C	
	temp = temp * 0.5;

	return temp;
}

static uint8_t LM75_get_product_id(void)
{
	uint8_t prod_id = 0;
	cy_rslt_t rslt = CY_RSLT_SUCCESS;

	/* ADD CODE */
	rslt = i2c_read_u8(I2C_Obj, LM75_SUBORDINATE_ADDR, LM75_PRODUCT_ID_REG, &prod_id);

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_console_printf("Error reading from LM75: %d\r\n", rslt);
		return 0;
	}

	return prod_id;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief 
 * This is a helper function that is called by tasks other than the temp sensor task
 * when they want to read the temperature from the sensor.
 */
bool system_sensors_get_temp(QueueHandle_t return_queue, float *temperature)
{
	device_request_msg_t packet;
	device_response_msg_t response;

	if(return_queue == NULL || temperature == NULL)
	{
		return false;
	}

	/* ADD CODE*/
	/* Send a request to the temp sensor task*/
	packet.device = DEVICE_TEMPERATURE;
	packet.operation = DEVICE_OP_READ;
	packet.response_queue = return_queue;

	// Send the request packet to the temp sensor task
	xQueueSend(Queue_Temp_Sensor_Requests, &packet, portMAX_DELAY);

	/* Wait for the response from the temp sensor task */
	xQueueReceive(return_queue, &response, portMAX_DELAY);

	/* Return the temperature to the caller */
	*temperature = response.payload.temperature;

	// Check the response status
	if (response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS)
	{
		return false;
	}

	return true;
}

/**
 * @brief
 * Task used to monitor the reception of command packets sent the temp sensor
 * @param param
 * Unused
 */
void task_temp_sensor(void *param)
{
	device_request_msg_t request_packet;
	device_response_msg_t response_packet;
	
	task_console_printf("Starting Temp Sensor Task\r\n");
	
	/* ADD CODE */

	// Grab I2C semaphore
	xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

	/* Verify that the device was found on the I2C Bus */
	uint8_t prod_id = LM75_get_product_id();

	if (prod_id != LM75_PRODUCT_ID)
	{
		task_console_printf("Error: LM75 Product ID Mismatch. Expected 0x%X, Read 0x%X\r\n", LM75_PRODUCT_ID, prod_id);
		CY_ASSERT(0);
	}
	else
	{
		task_console_printf("LM75 Product ID Verified: 0x%X\r\n", prod_id);
	}

	// Release I2C semaphore
	xSemaphoreGive(*I2C_Semaphore);

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_Temp_Sensor_Requests, &request_packet, portMAX_DELAY);
	
		/* ADD CODE */

		// Grab I2C semaphore
		xSemaphoreTake(*I2C_Semaphore, portMAX_DELAY);

		/* Read the Temperature*/
		float temp = LM75_get_temp();

		// Release I2C semaphore
		xSemaphoreGive(*I2C_Semaphore);

		/* Return the temperature */
		response_packet.device = DEVICE_TEMPERATURE;
		response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;
		response_packet.payload.temperature = temp;
		xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);


	}
}

/**
 * @brief
 * Initializes software resources related to the operation of
 * the Temp Sensor.  This function expects that the I2C bus had already
 * been initialized prior to the start of FreeRTOS.
 */
bool task_temp_sensor_resources_init(SemaphoreHandle_t *i2c_semaphore, cyhal_i2c_t *i2c_obj)
{
	/* Save the I2C object and semaphore */
	I2C_Obj = i2c_obj;
	I2C_Semaphore = i2c_semaphore;

	if (I2C_Semaphore == NULL || I2C_Obj == NULL)
	{
		return false;
	}	

	/* Create the Queue used to receive requests  */
	Queue_Temp_Sensor_Requests = xQueueCreate(1, sizeof(device_request_msg_t));
	if (Queue_Temp_Sensor_Requests == NULL)
	{
		return false;
	}
	
	/* Create the task that will control the status LED */
	if(xTaskCreate(
		task_temp_sensor,
		"Temp Sensor",
		10*configMINIMAL_STACK_SIZE,
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