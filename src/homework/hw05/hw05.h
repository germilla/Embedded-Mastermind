 /**
 * @file hw05.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-23
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #ifndef __HW05_H__
 #define __HW05_H__

 #include "main.h"

 #if defined(HW05)

#include "drivers.h"
#include "rtos_events.h"
#include "task_eeprom.h"
#include "task_cap_touch.h"
#include "task_console.h"

#define  TASK_SYSTEM_CONTROL_STACK_SIZE    (configMINIMAL_STACK_SIZE*5)
#define  TASK_SYSTEM_CONTROL_PRIORITY      (tskIDLE_PRIORITY + 1U)  

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/
void task_hw05_system_control(void *pvParameters);


 #endif

 #endif /* __HW05_H__ */