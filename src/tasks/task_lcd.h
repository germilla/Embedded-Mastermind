/**
 * @file task_lcd.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef TASK_LCD_H
#define TASK_LCD_H

#include "main.h"

#if defined(ECE353_FREERTOS)
#include "master_mind_lib.h"

#define TASK_LCD_STACK_SIZE    (configMINIMAL_STACK_SIZE*5)
#define TASK_LCD_PRIORITY      (tskIDLE_PRIORITY + 1U)

/* FreeRTOS Queue for LCD messages */
extern QueueHandle_t xQueue_Request_LCD;

/* LCD Return Codes */
typedef enum
{
    LCD_MSG_RESPONSE_SUCCESS = 0,
    LCD_MSG_RESPONSE_ERROR
} lcd_msg_response_t;

typedef struct
{
    lcd_msg_t       msg;
    QueueHandle_t   return_queue;
} lcd_msg_request_t;

/* LCD Task Initialization */
bool task_lcd_resources_init(QueueHandle_t queue_request);

#endif /* ECE353_FREERTOS */
#endif /* TASK_LCD_H */