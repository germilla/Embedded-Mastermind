/**
 * @file leds.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "leds.h"

 cy_rslt_t leds_init_gpio(void) {
    cy_rslt_t result;

    // Initialize Red LED
    result = cyhal_gpio_init(PIN_LED_RED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize Green LED
    result = cyhal_gpio_init(PIN_LED_GREEN, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize Blue LED
    result = cyhal_gpio_init(PIN_LED_BLUE, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, false);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    return CY_RSLT_SUCCESS;
 }

 void leds_set_state(ece353_led_t led, ece353_led_state_t state) {
    switch (led) {
        case RED_LED:
            cyhal_gpio_write(PIN_LED_RED, state);
            break;
        case GREEN_LED:
            cyhal_gpio_write(PIN_LED_GREEN, state);
            break;
        case BLUE_LED:
            cyhal_gpio_write(PIN_LED_BLUE, state);
            break;
        default:
            // Invalid LED
            break;
    }
 }