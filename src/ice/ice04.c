/**
 * @file ice04.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-01
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(ICE04)
#include "drivers.h"
#include <stdio.h>

char APP_DESCRIPTION[] = "ECE353: ICE 04 - PWM Buzzer";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/

//LED pwm objects
cyhal_pwm_t pwm_led_red;
cyhal_pwm_t pwm_led_green;
cyhal_pwm_t pwm_led_blue;

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
    
    
    // Initialze buttons
    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buttons GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    // Initialize button timer
    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buttons timer\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    // Initialize LEDs PWM
    rslt = leds_init_pwm(&pwm_led_red, &pwm_led_green, &pwm_led_blue);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize LED PWM\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }

    // Start PWM for each LED
    rslt = cyhal_pwm_start(&pwm_led_red);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start RED LED PWM\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }

    rslt = cyhal_pwm_start(&pwm_led_green);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start GREEN LED PWM\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }

    rslt = cyhal_pwm_start(&pwm_led_blue);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to start BLUE LED PWM\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
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
    int red_duty_cycle = 0;
    int green_duty_cycle = 0;
    int blue_duty_cycle = 0;
    bool print;
    while(1) {
        if (ECE353_Events.sw1) {
            ECE353_Events.sw1 = 0;

            // Increase Red LED brightness by 10%
            red_duty_cycle = (red_duty_cycle + 10) % 110;
            cyhal_pwm_set_duty_cycle(&pwm_led_red, red_duty_cycle, 1000);
            print = true;
        }
        else if (ECE353_Events.sw2) {
            ECE353_Events.sw2 = 0;

            // Increase Green LED brightness by 10%
            green_duty_cycle = (green_duty_cycle + 10) % 110;
            cyhal_pwm_set_duty_cycle(&pwm_led_green, green_duty_cycle, 1000);  
            print = true; 
        }
        else if (ECE353_Events.sw3) {
            ECE353_Events.sw3 = 0;

            // Increase Blue LED brightness by 10%
            blue_duty_cycle = (blue_duty_cycle + 10) % 110;
            cyhal_pwm_set_duty_cycle(&pwm_led_blue, blue_duty_cycle, 1000);
            print = true;
        }
        if (print) {
            printf("LED Duty Cycles - Red: %d%%, Green: %d%%, Blue: %d%%\n\r",
                   red_duty_cycle, green_duty_cycle, blue_duty_cycle);
            print = false;
        }
    }
}
#endif
