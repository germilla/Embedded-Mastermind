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
    }
    return new_state ? BUTTON_STATE_HIGH : BUTTON_STATE_LOW;
}

static cyhal_timer_t button_timer;
static cyhal_timer_cfg_t button_timer_cfg;

static void button_timer_handler(void *arg, cyhal_timer_event_t event) {
    // Timer interrupt handler for button debouncing
    // This function can be used to update button states if needed

    static uint8_t button_counts[3] = {[0]=0, [1]=0, [2]=0};

    uint8_t sw1 = PORT_BUTTON_SW1->IN & MASK_BUTTON_PIN_SW1;
    uint8_t sw2 = PORT_BUTTON_SW2->IN & MASK_BUTTON_PIN_SW2;
    uint8_t sw3 = PORT_BUTTON_SW3->IN & MASK_BUTTON_PIN_SW3;

    if (sw1 == 0) {
        button_counts[0]++;

        if (button_counts[0] == 5) {
            ECE353_Events.sw1 = 1;
        }
    } else {
        button_counts[0] = 0;
    }

    if (sw2 == 0) {
        button_counts[1]++;

        if (button_counts[1] == 5) {
            ECE353_Events.sw2 = 1;
        }
    } else {
        button_counts[1] = 0;
    }

    if (sw3 == 0) {
        button_counts[2]++;

        if (button_counts[2] == 5) {
            ECE353_Events.sw3 = 1;
        }
    } else {
        button_counts[2] = 0;
    }
}

// Function to initialize the timer for button debouncing
cy_rslt_t buttons_init_timer(void) {

    // Initialize the timer for button debouncing
    return timer_init(&button_timer, &button_timer_cfg, 500000, button_timer_handler); // 10 ms debounce time;
}