/**
 * @file task_buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#ifdef ECE353_FREERTOS

#include "task_buzzer.h"

/**
 * @brief 
 * Task used to control the buzzer based on button events.
 * 
 * SW1 -- Turn buzzer on
 * SW2 -- Turn buzzer off
 *
 * @param arg 
 * Unused parameter
 */

EventBits_t uxBits;

void task_buzzer(void *arg)
{
    (void)arg; // Unused parameter
    printf("BUZZER Task Started\r\n");
    while(1)
    {
        uxBits = xEventGroupWaitBits(
            ECE353_RTOS_Events,    // The event group being tested.
            ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED, // The bits within the event group to wait for.
            pdTRUE,                // BIT_0 and BIT_4 should be cleared before returning.
            pdFALSE,               // Don't wait for both bits, either bit will do.
            portMAX_DELAY);        // Wait indefinitely.

        if((uxBits & ECE353_BUTTON_1_PRESSED) != 0)
        {
            printf("BUZZER ON\r\n");
            buzzer_on();
        }
        else if ((uxBits & ECE353_BUTTON_2_PRESSED) != 0)
        {
            printf("BUZZER OFF\r\n");
            buzzer_off();
        }

    }
}
#endif