/**
 * @file buttons.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */

 #include "buttons.h"

cy_rslt_t buttons_init_gpio(void) {
    cy_rslt_t result;

    // Initialize SW1
    result = cyhal_gpio_init(PIN_BUTTON_SW1, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, true);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize SW2
    result = cyhal_gpio_init(PIN_BUTTON_SW2, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, true);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    // Initialize SW3
    result = cyhal_gpio_init(PIN_BUTTON_SW3, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, true);
    if (result != CY_RSLT_SUCCESS) {
        return result;
    }

    return CY_RSLT_SUCCESS;   
}

button_state_t buttons_get_state(ece353_button_t button) {  
    static button_state_t button1;
    static button_state_t button2;
    static button_state_t button3;
    bool new_state;

    switch (button) {
        case BUTTON_SW1:
            new_state = cyhal_gpio_read(PIN_BUTTON_SW1);
            if (new_state && button1 == 1) {
                button1 = 2;
                return BUTTON_STATE_RISING_EDGE;
            }
            else if (!new_state && button1 == 2) {
                button1 = 1;
                return BUTTON_STATE_FALLING_EDGE;
            }
            button1 = new_state ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW; 
            break;
        case BUTTON_SW2:
            new_state = cyhal_gpio_read(PIN_BUTTON_SW2);
            if (new_state && button2 == 1) {
                button2 = 2;
                return BUTTON_STATE_RISING_EDGE;
            }
            else if (!new_state && button2 == 2) {
                button2 = 1;
                return BUTTON_STATE_FALLING_EDGE;
            }
            button2 = new_state ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW; 
            break;
        case BUTTON_SW3:
            new_state = cyhal_gpio_read(PIN_BUTTON_SW3);
            if (new_state && button3 == 1) {
                button3 = 2;
                return BUTTON_STATE_RISING_EDGE;
            }
            else if (!new_state && button3 == 2) {
                button3 = 1;
                return BUTTON_STATE_FALLING_EDGE;
            }
            button3 = new_state ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW; 
            break;
        default:
            return BUTTON_STATE_LOW; // Default case, should not happen

        return new_state ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
    }
}