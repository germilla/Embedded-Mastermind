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

	// Read 2-bytes from the temperature register

	// Need to format the raw value read from the sensor
	// The LM75 returns a 9-bit, two's complement value

	// Convert the raw value to degrees Celsius
	// Each bit is worth 0.5 degrees C	

	return temp;
}

static uint8_t LM75_get_product_id(void)
{
	uint8_t prod_id = 0;

	/* ADD CODE */

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

	/* Wait for the response from the temp sensor task */
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

	/* Verify that the device was found on the I2C Bus */

	while (1)
	{
		/* Wait for a message */
		xQueueReceive(Queue_Temp_Sensor_Requests, &request_packet, portMAX_DELAY);
	
		/* ADD CODE */

		/* Read the Temperature*/

		/* Return the temperature */

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