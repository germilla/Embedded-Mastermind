/**
 * @file task_joystick.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-14
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "main.h"

#ifdef ECE353_FREERTOS  
#include "drivers.h"
 #include "task_joystick.h"

 QueueHandle_t Queue_Joystick = NULL;

/* Message lookup table for joystick positions */
const char * const joystick_pos_names[] = {
    "Center",
    "Left",
    "Right",
    "Up",
    "Down",
    "Upper Left",
    "Upper Right",
    "Lower Left",
    "Lower Right"
};

 /**
  * @brief 
  *  Task used to monitor the joystick
  * @param arg 
  */
 void task_joystick(void *arg)
{
    (void)arg; // Unused parameter
    
    printf("Starting Joystick Task\n\r");

    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(30)); // Delay for 500 ms
        joystick_position_t prev_position;


        joystick_position_t position = joystick_get_pos();
        
        if (position != prev_position) {
            xQueueOverwrite(Queue_Joystick, &position);
            prev_position = position;
        }
        
    }
}


bool task_joystick_init(void)
{
    /* Create the Queue used to send Joystick Positions*/
    Queue_Joystick = xQueueCreate(1, sizeof(joystick_position_t));

    /* Create the joystick task */
    BaseType_t rslt = xTaskCreate(task_joystick, "Joystick Task", 512, NULL, 2, NULL);

    return (rslt == pdPASS) && (Queue_Joystick != NULL);
}
#endif