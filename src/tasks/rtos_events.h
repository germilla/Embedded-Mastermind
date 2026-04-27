/**
 * @file rtos_events.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #ifndef __RTOS_EVENTS_H__
 #define __RTOS_EVENTS_H__
 #include "main.h"
 
#ifdef ECE353_FREERTOS

/*******************************************************************************
* Event Group for system events.
 ******************************************************************************/
extern EventGroupHandle_t ECE353_RTOS_Events;

/*******************************************************************************
* Macros used to define the system events
******************************************************************************/
#define ECE353_BUTTON_1_PRESSED    ( 1 << 0 ) // Event 1
#define ECE353_BUTTON_2_PRESSED    ( 1 << 1 ) // Event 2
#define ECE353_BUTTON_3_PRESSED    ( 1 << 2 ) // Event 3

#define ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED    ( 1 << 3 ) // Event 4
#define ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED    ( 1 << 4 ) // Event 5

#define ECE353_THEME_CHANGED                   ( 1 << 5 ) // Event 6: ambient light mode changed

#endif // ECE353_FREERTOS

#endif // __RTOS_EVENTS_H__