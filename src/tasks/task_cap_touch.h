/**
 * @file task_cap_touch.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-2026
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#ifndef __TASK_CAP_TOUCH_H__
#define __TASK_CAP_TOUCH_H__


#include "main.h"

#if defined(ECE353_FREERTOS)

#include "drivers.h"
#include "cap_touch.h"

#include "devices.h"
#include "cyhal_spi.h"
#include "task_console.h"

#define TASK_CAP_TOUCH_STACK_SIZE    (configMINIMAL_STACK_SIZE*5)
#define TASK_CAP_TOUCH_PRIORITY      (tskIDLE_PRIORITY + 1U)

/* Function used to initialize resources for the IMU task */
bool task_cap_touch_resources_init(
    QueueHandle_t queue_request,
    SemaphoreHandle_t i2c_semaphore, 
    cyhal_i2c_t *i2c_obj, 
    cyhal_gpio_t pin_cap_touch_int
);

#endif
#endif