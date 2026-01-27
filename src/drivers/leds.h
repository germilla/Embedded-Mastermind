/**
 * @file leds.h
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#ifndef __LEDS_H__
#define __LEDS_H__

#include <stdio.h>
#include "cybsp.h"
#include "cyhal_gpio.h"
#include "ece353-pins.h"

typedef enum {
    RED_LED = 0,
    GREEN_LED,
    BLUE_LED
} ece353_led_t;

typedef enum {
    LED_STATE_OFF = 0,
    LED_STATE_ON
} ece353_led_state_t;

// Function to initialize the LEDs
cy_rslt_t leds_init_gpio(void);

// Function to set the state of a specific LED
void leds_set_state(ece353_led_t led, ece353_led_state_t state);

#endif