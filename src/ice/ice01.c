/**
 * @file ice01.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE01)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: ICE 01 - Memory Mapped IO - GPIO";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/**
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 */
void app_init_hw(void)
{
    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    // Initialize the buttons
    buttons_init_gpio();
    leds_init_gpio();
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void)
{

    while(1)
    {
        // Read the state of SW1
        button_state_t sw1_state = buttons_get_state(BUTTON_SW1);
        if (sw1_state == BUTTON_STATE_FALLING_EDGE) {
            printf("SW1 Pressed!\n\r");
            leds_set_state(RED_LED, LED_STATE_ON);
        }
        else if (sw1_state == BUTTON_STATE_RISING_EDGE) {
            printf("SW1 Released!\n\r");
            leds_set_state(RED_LED, LED_STATE_OFF);
        }
        button_state_t sw2_state = buttons_get_state(BUTTON_SW2);
        if (sw2_state == BUTTON_STATE_FALLING_EDGE) {
            printf("SW2 Pressed!\n\r");
            leds_set_state(GREEN_LED, LED_STATE_ON);
        }
        else if (sw2_state == BUTTON_STATE_RISING_EDGE) {
            printf("SW2 Released!\n\r");
            leds_set_state(GREEN_LED, LED_STATE_OFF);
        }
        button_state_t sw3_state = buttons_get_state(BUTTON_SW3);
        if (sw3_state == BUTTON_STATE_FALLING_EDGE) {
            printf("SW3 Pressed!\n\r");
            leds_set_state(BLUE_LED, LED_STATE_ON);
        }
        else if (sw3_state == BUTTON_STATE_RISING_EDGE) {
            printf("SW3 Released!\n\r");
            leds_set_state(BLUE_LED, LED_STATE_OFF);
        }

        /* Sleep for 100mS */
        cyhal_system_delay_ms(100);

    }
}
#endif
