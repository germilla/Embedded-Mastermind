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
#include "main.h"

#if defined(EX12)
#include "buzzer.h"
#include "drivers.h"
#include "task_console.h"
#include "task_imu.h"

char APP_DESCRIPTION[] = "ECE353: Example 12 - FreeRTOS IMU";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
SemaphoreHandle_t Spi_Semaphore = NULL;
cyhal_spi_t *SPI_Obj;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the SPI interface */
    SPI_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if(SPI_Obj == NULL)
    {
        printf("SPI initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Initialize IMU CS pin
    rslt = cyhal_gpio_init(PIN_IMU_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, true);
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize IMU CS Pin!\n\r");
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
    if(!task_console_init())
    {
        printf("Console initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create semaphore for SPI access
    Spi_Semaphore = xSemaphoreCreateBinary();

    // Give the semaphore so that it is available for use by the tasks
    xSemaphoreGive(Spi_Semaphore);

    // Initialize the IMU task resources
    if(!task_imu_resources_init((void *)Spi_Semaphore, SPI_Obj, PIN_IMU_CS))
    {
        printf("Failed to initialize IMU task resources!\n\r");
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