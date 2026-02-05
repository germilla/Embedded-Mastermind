/**
 * @file ex03.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-06-30
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "main.h"

#if defined(EX04)
#include "drivers.h"

char APP_DESCRIPTION[] = "ECE353: Example 04 - PWM with Interrupts";

/*****************************************************************************/
/* Macros                                                                    */
/*****************************************************************************/
// The general formula for calculating the number of ticks for a given frequency is:
// Ticks = (1 / Desired Frequency) / (1 / Timer Frequency) = Tick Count
//
// If we re-arrange the formula, we can see that:
// Tick Count = Timer Frequency / Desired Frequency
//
// For a 1000 Hz frequency with a 100 MHz timer, the calculation is:
// Tick Count = 100000000 / 1000 = 100000 ticks

// Because we need to toggle the buzzer on and off, we need to set the timer to 
// half the frequency, so we divide the tick count by 2.
#define TIMER_FREQUENCY 100000000       // Timer frequency in Hz
#define BUZZER_FREQUENCY_HZ_1000 1000  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_1500 1500  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_2000 2000  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_2500 2500  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_3000 3000  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_3500 3500  // Desired buzzer frequency in Hz
#define BUZZER_FREQUENCY_HZ_4000 4000  // Desired buzzer frequency in Hz

#define BUZZER_TICK_COUNT_1000_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_1000) / 2
#define BUZZER_TICK_COUNT_1500_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_1500) / 2
#define BUZZER_TICK_COUNT_2000_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_2000) / 2
#define BUZZER_TICK_COUNT_2500_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_2500) / 2
#define BUZZER_TICK_COUNT_3000_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_3000) / 2
#define BUZZER_TICK_COUNT_3500_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_3500) / 2
#define BUZZER_TICK_COUNT_4000_HZ (TIMER_FREQUENCY / BUZZER_FREQUENCY_HZ_4000) / 2

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_timer_t buzzer_timer;
cyhal_timer_cfg_t buzzer_timer_cfg;

typedef enum {
    BUZZER_INDEX_HZ_0000 = 0,
    BUZZER_INDEX_HZ_1000,
    BUZZER_INDEX_HZ_1500,
    BUZZER_INDEX_HZ_2000,
    BUZZER_INDEX_HZ_2500,
    BUZZER_INDEX_HZ_3000,
    BUZZER_INDEX_HZ_3500,
    BUZZER_INDEX_HZ_4000
} buzzer_index_t;

uint32_t BUZZER_TICKS[] = {
    0,
    BUZZER_TICK_COUNT_1000_HZ,
    BUZZER_TICK_COUNT_1500_HZ,
    BUZZER_TICK_COUNT_2000_HZ,
    BUZZER_TICK_COUNT_2500_HZ,
    BUZZER_TICK_COUNT_3000_HZ,
    BUZZER_TICK_COUNT_3500_HZ,
    BUZZER_TICK_COUNT_4000_HZ

};

char BUZZER_DBG_MESSAGES[][50] = {
    "Buzzer Off",
    "Buzzer 1.0 KHz",
    "Buzzer 1.5 KHz",
    "Buzzer 2.0 KHz",
    "Buzzer 2.5 KHz",
    "Buzzer 3.0 KHz",
    "Buzzer 3.5 KHz",
    "Buzzer 4.0 KHz"
};

cyhal_timer_t buzzer_obj;
cyhal_timer_cfg_t buzzer_cfg;

/*****************************************************************************/
/* Function Declarations                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

void buzzer_handler(void *handler_arg, cyhal_timer_event_t event)
{
    PORT_BUZZER->OUT_INV = MASK_BUZZER;
}

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

    /* Configure the gpio pin connected to buzzer as an output pin*/
    rslt = cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buzzer GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);
    }

    /* Initialize the buttons */
    rslt = buttons_init_gpio();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Failed to initialize buttons GPIO\n");
        for(int i = 0; i < 1000000; i++); // Delay for a while
        CY_ASSERT(0);   
    }

    /* Initialize the timer for button debouncing */
    rslt = buttons_init_timer();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("Error initializing button timer\n\r");
        for(int i = 0; i < 100000; i++);
        CY_ASSERT(0);
    }

    timer_init(&buzzer_obj, &buzzer_cfg, BUZZER_TICKS[BUZZER_INDEX_HZ_1000], buzzer_handler);

    cyhal_timer_stop(&buzzer_obj);

    cyhal_gpio_init(PIN_BUZZER, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 0);
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
    buzzer_index_t buzzer_index = BUZZER_INDEX_HZ_1000;
    while (1)
    {
        if (ECE353_Events.sw1)
        {
            ECE353_Events.sw1 = 0;

            buzzer_index++;
            
            if (buzzer_index >= sizeof(BUZZER_TICKS) / sizeof(BUZZER_TICKS[0]))
            {
                buzzer_index = BUZZER_INDEX_HZ_0000;
            }

            cyhal_timer_stop(&buzzer_obj);
            
            // Only turn on timer for tick counts > 0
            if (BUZZER_TICKS[buzzer_index] > 0)
            {
                buzzer_cfg.period = BUZZER_TICKS[buzzer_index];
                cyhal_timer_configure(&buzzer_obj, &buzzer_cfg);
                cyhal_timer_start(&buzzer_obj);
            }

             printf("%s\n\r", BUZZER_DBG_MESSAGES[buzzer_index]);
        }
    }
}
#endif
