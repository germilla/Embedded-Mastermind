/**
 * @file ice03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE03)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 03 - Timer Interrupts/Debounce Buttons";

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
    cy_rslt_t rslt;

    console_init();
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the User LED */
    rslt = leds_init_gpio();
    if (rslt != CY_RSLT_SUCCESS) {
        printf("ERROR: leds_init_gpio() failed with error code %d\r\n", rslt);
        for (int i = 0; i < 1000000; i++);
        CY_ASSERT(0);
    }

    /* Initialize the Buttons */
    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS) {
        printf("ERROR: buttons_init_gpio() failed with error code %d\r\n", rslt);
        for (int i = 0; i < 1000000; i++);
        CY_ASSERT(0);
    }

    /* Initialize the Timer */
    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS) {
        printf("ERROR: buttons_init_timer() failed with error code %d\r\n", rslt);
        for (int i = 0; i < 1000000; i++);
        CY_ASSERT(0);
    }
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
    typedef enum {
        INIT,
        SW1_DET,
        SW2_DET_1,
        SW2_DET_2,
        SW3_DET
    } state_t;

    state_t currentState = INIT;
    while(1) {
        switch (currentState) {
            case INIT:
                leds_set_state(RED_LED, LED_STATE_ON);
                if (ECE353_Events.sw1) {
                    ECE353_Events.sw1 = 0;
                    currentState = SW1_DET;
                }
                break;
            case SW1_DET:
                leds_set_state(RED_LED, LED_STATE_ON);
                leds_set_state(BLUE_LED, LED_STATE_ON);
                if (ECE353_Events.sw2) {
                    ECE353_Events.sw2 = 0;
                    currentState = SW2_DET_1;
                }
                else if (ECE353_Events.sw3 || ECE353_Events.sw1) {
                    currentState = INIT;
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw3 = 0;
                }
                break;
            case SW2_DET_1:
                leds_set_state(BLUE_LED, LED_STATE_ON);
                if (ECE353_Events.sw2) {
                    ECE353_Events.sw2 = 0;
                    currentState = SW2_DET_2;
                }
                else if (ECE353_Events.sw3 || ECE353_Events.sw1) {
                    currentState = INIT;
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw3 = 0;
                }
                break;
            case SW2_DET_2:
                leds_set_state(BLUE_LED, LED_STATE_ON);
                leds_set_state(GREEN_LED, LED_STATE_ON);
                if (ECE353_Events.sw3) {
                    ECE353_Events.sw3 = 0;
                    currentState = SW3_DET;
                }
                else if (ECE353_Events.sw2 || ECE353_Events.sw1) {
                    currentState = INIT;
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw2 = 0;
                }
                break;
            case SW3_DET:
                leds_set_state(GREEN_LED, LED_STATE_ON);
                if (ECE353_Events.sw1 || ECE353_Events.sw2 || ECE353_Events.sw3) {
                    ECE353_Events.sw1 = 0;
                    ECE353_Events.sw2 = 0;
                    ECE353_Events.sw3 = 0;
                    currentState = INIT;
                }
                break;
            default:
                printf("ICE03: Unknown State!\n");
                currentState = INIT;
                break;
        }

        // Set buttons and LEDS back to off
        leds_set_state(GREEN_LED, LED_STATE_OFF);
        leds_set_state(BLUE_LED, LED_STATE_OFF);
        leds_set_state(RED_LED, LED_STATE_OFF);
    }

}
#endif
