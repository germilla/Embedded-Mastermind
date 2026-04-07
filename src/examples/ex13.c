/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "cyhal_hw_types.h"
#include "main.h"

#if defined(EX13)
#include "drivers.h"
#include "task_console.h"
#include "task_temp_sensor.h"

char APP_DESCRIPTION[] = "ECE353: Ex 13 - FreeRTOS Temp Sensor";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Obj = NULL;
SemaphoreHandle_t I2C_Semaphore = NULL;
QueueHandle_t Queue_Temp_Sensor_Responses = NULL;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/
void task_system_control(void *arg);

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_system_control(void *arg)
{
    (void)arg; // Unused parameter
    
    task_console_printf("Starting System Control Task\r\n");

    float temperature = 0.0;
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Wait for the response from the temp sensor task */
        if(system_sensors_get_temp(Queue_Temp_Sensor_Responses, &temperature) == true)
        {
            task_console_printf("Temperature: %.2f C\r\n", temperature);
        }
        else
        {
            task_console_printf("Failed to read temperature from sensor!\r\n");
        }
    }
}

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the i2c interface */
    I2C_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if (I2C_Obj == NULL)
    {
        printf("I2C initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{
    /* Create a Queue for the temp sensor task */
    Queue_Temp_Sensor_Responses = xQueueCreate(1, sizeof(device_response_msg_t));
    
    /* Create the I2C Semaphore */
    I2C_Semaphore = xSemaphoreCreateBinary();
    if(I2C_Semaphore == NULL)
    {
        printf("Failed to create I2C semaphore!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Need to give the semaphore so that the first task that attempts to take
       the semaphore is successful */
    xSemaphoreGive(I2C_Semaphore);

    if(!task_console_init())
    {
        printf("Console initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_temp_sensor_resources_init(&I2C_Semaphore, I2C_Obj))
    {
        printf("Temp Sensor Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Create the system control task */
    if (xTaskCreate(
            task_system_control,
            "SYS_CTRL",
            5 * configMINIMAL_STACK_SIZE,
            NULL,
            tskIDLE_PRIORITY + 1,
            NULL) != pdPASS)
    {
        printf("System Control Task creation failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif