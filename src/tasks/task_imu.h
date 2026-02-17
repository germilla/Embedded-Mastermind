/**
 * @file task_imu.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-16
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __TASK_IMU_H__
#define __TASK_IMU_H__

#include "main.h"
#if defined(ECE353_FREERTOS)
#include "drivers.h"
#include "devices.h"
#include "cyhal_spi.h"

extern QueueHandle_t Queue_IMU_Request;

bool system_sensors_get_imu(QueueHandle_t return_queue, int16_t imu_data[3]);

/* Function used to initialize resources for the IMU task */
bool task_imu_resources_init(void *spi_semaphore, cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin);

#endif /* ECE353_FREERTOS */

#endif /* __TASK_IMU_H__ */