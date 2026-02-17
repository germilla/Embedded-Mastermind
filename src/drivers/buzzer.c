/**
 * @file buzzer.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-07-10
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "buzzer.h"


cyhal_pwm_t buzzer_obj;

cy_rslt_t buzzer_init(float duty_cycle, uint32_t frequency)
{
    cy_rslt_t rslt;

    /* Configure the gpio pin connected to buzzer as an output pin*/
    rslt = cyhal_pwm_init(&buzzer_obj, PIN_BUZZER, NULL);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buzzer GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    // Set the duty cycle and frequency for the buzzer
    rslt = cyhal_pwm_set_duty_cycle(&buzzer_obj, duty_cycle, frequency);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to set buzzer duty cycle and frequency\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    return CY_RSLT_SUCCESS;
}

void buzzer_on(void)
{
    cyhal_pwm_start(&buzzer_obj);
}

void buzzer_off(void)
{
    cyhal_pwm_stop(&buzzer_obj);
}