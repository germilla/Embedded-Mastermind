/**
 * @file devices.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "devices.h"

#if defined(ECE353_FREERTOS)
#include "task_imu.h"
#include "task_temp_sensor.h"
#include "task_light_sensor.h"
#include "task_io_expander.h"
#include "task_eeprom.h"
#include "task_console.h"

#endif /* ECE353_FREERTOS */
