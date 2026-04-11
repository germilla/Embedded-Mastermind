/**
 * @file task_cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #include "task_cap_touch.h"

#if defined(ECE353_FREERTOS)
#include "rtos_events.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
static QueueHandle_t        Queue_Request_Cap_Touch = NULL;
static SemaphoreHandle_t    I2C_Semaphore = NULL;
static cyhal_i2c_t         *I2C_Obj = NULL;
static cyhal_gpio_t        Cap_Touch_Int_Pin = NC;

bool task_cap_touch_resources_init(
    QueueHandle_t queue_request, 
    SemaphoreHandle_t i2c_semaphore, 
    cyhal_i2c_t *i2c_obj, 
    cyhal_gpio_t pin_cap_touch_int
)
{
    if(queue_request == NULL || i2c_semaphore == NULL || i2c_obj == NULL || pin_cap_touch_int == NC)
    {
        return false;
    }   

    /* Save the resources */
    Queue_Request_Cap_Touch = queue_request;
    I2C_Semaphore = i2c_semaphore;
    I2C_Obj = i2c_obj;
    Cap_Touch_Int_Pin = pin_cap_touch_int;

    /* ADD CODE */
    return true;
}
#endif /* ECE353_FREERTOS */