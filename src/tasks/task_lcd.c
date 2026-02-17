/**
 * @file task_lcd.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-18
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_lcd.h"

#if defined(ECE353_FREERTOS)

/* FreeRTOS Queue for LCD messages */
static QueueHandle_t Queue_Requests = NULL;

/* LCD Task */
void task_lcd(void *pvParameters)
{
    (void)pvParameters; // Unused parameter

    lcd_msg_request_t lcd_request;
    lcd_msg_response_t response;
    bool status = false;

    while(1)
    {
    }
}

/* LCD Task Initialization */
bool task_lcd_resources_init(QueueHandle_t queue_request){

    BaseType_t result;

    if (queue_request == NULL)
    {
        return false;
    }
    Queue_Requests = queue_request;

    /* Create the LCD Task */
    result= xTaskCreate(
        task_lcd,                       // Task function
        "LCD Task",                     // Task name
        TASK_LCD_STACK_SIZE,            // Stack size
        NULL,                           // Task parameters
        TASK_LCD_PRIORITY,              // Task priority
        NULL                            // Task handle
    );

    if(result != pdPASS)
    {
        return false;
    }   

    return true;
}
#endif