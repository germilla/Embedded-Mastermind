/**
 * @file task_buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-08-13
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "task_buttons.h"
 #include "task_console.h"

 #ifdef ECE353_FREERTOS
 /**
  * @brief 
  * Task used to debounce button presses (SW1, SW2, SW3).  
  * The falling edge of the button press is detected by de-bouncing
  * the button for 30mS. Each button should be sampled every 15mS.
  *
  * When a button press is detected, the corresponding event is set in
  * in the event group ECE353_RTOS_Events.
  *
  * @param arg 
  * Unused parameter
  */
 void task_buttons(void *arg)
 {
    (void)arg; // Unused parameter
    uint32_t button_count1 = 0;
    uint32_t button_count2 = 0;
    uint32_t button_count3 = 0;

    printf("Button Task Started\r\n");
    while (1)
    {
        // Monitor button SW1
        if ((PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1) == 0) {
            button_count1++;

             if (button_count1 == 2) {
                task_console_printf("SW1 Pressed\r\n");

                // Set global variable
                //xEventGroupSetBits(ECE353_RTOS_Events, ECE353_BUTTON_1_PRESSED);
             }
        }
        else {
            button_count1 = 0;
        }

        // Monitor button SW2
        if ((PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2) == 0) {
            button_count2++;

             if (button_count2 == 2) {
                task_console_printf("SW2 Pressed\r\n");

                // Set global variable
                xEventGroupSetBits(ECE353_RTOS_Events, ECE353_BUTTON_2_PRESSED);
             }
        }
        else {
            button_count2 = 0;
        }


        // Monitor button SW3
        if ((PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3) == 0) {
            button_count3++;

             if (button_count3 == 2) {
                task_console_printf("SW3 Pressed\r\n");

                // Set global variable
                //xEventGroupSetBits(ECE353_RTOS_Events, ECE353_BUTTON_3_PRESSED);
             }
        }
        else {
            button_count3 = 0;
        }

        // Debounce delay
        vTaskDelay(pdMS_TO_TICKS(30));
    }
 }

 /* Button Task Initialization */
bool task_button_init(void){

    BaseType_t result;

    // Create the button task
    result = xTaskCreate(
        task_buttons, 
        "Button Task", 
        configMINIMAL_STACK_SIZE, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL
    );

    if(result != pdPASS)
    {
        return false;
    }

    return true;
}
#endif