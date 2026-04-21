 /**
 * @file hw05.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-08
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "hw05.h"

#if defined(HW05)
#include "drivers.h"
#include "task_buttons.h"
#include "task_ipc.h"
#include "rtos_events.h"
#include "task_lcd.h"

char APP_DESCRIPTION[] = "ECE353 S26 HW05";

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
cyhal_i2c_t *I2C_Monarch_Obj;
cyhal_spi_t *SPI_Monarch_Obj;

SemaphoreHandle_t I2C_Semaphore;
SemaphoreHandle_t SPI_Semaphore;

EventGroupHandle_t ECE353_RTOS_Events;

QueueHandle_t xQueue_Request_LCD;
int selected_cypher_digit, cypher_number;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/* This function will be used to discover other board. This function should not
 * return until the discovery is complete.  The discovery is complete when we receive
 * a discovery message from the other board OR we send a discovery message that is 
 * Acked by the other board */
void discover_board(uint16_t *sequence_num) {
  bool discovery_complete = false;
  while (discovery_complete == false) {
    ipc_send_discovery(*sequence_num);
    if (ipc_wait_for_ack(100)) {
      discovery_complete = true;
      printf("ACK received for discovery message!\n\r");
      printf("Discovery message received from other board!\n\r");
      (*sequence_num)++;
    } else {
      printf("No response to discovery message. Retrying...\n\r");
    }
  }
}

/* Show the numbers on the screen and allow user to chose 4 numbers, then returns those 4 numbers */
uint32_t number_select(void) {
    /* Allocate a lcd_msg_request_t variable */
    lcd_msg_request_t lcd_request;

    /* Write a message to the user in the Text Area of the screen*/
    lcd_request.msg.command = LCD_CMD_PRINT_MESSAGE;
    lcd_request.return_queue = NULL;
    snprintf(lcd_request.msg.payload.message, 32, "Select Your Cypher!");
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    /* Draw 4 blank tiles for the secret code */
    for(int col = 0; col < 4; col++)
    {
        lcd_request.msg.command = LCD_CMD_DRAW_TILE;
        lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        lcd_request.msg.payload.tile.col = col;
        lcd_request.msg.payload.tile.number = 0; // number is ignored for code tiles
        lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
        lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
        
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    }


    /* Draw numbers 0-7 for the user input*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    for (int row = 1; row < 3; row++) {
        for(int col = 0; col < 4; col++) {
            lcd_request.msg.payload.tile.row = row;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = col+(row-1)*4;

            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }

    /* Draw the number 0 as the selected user input and highlight first cypher digit*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_NUM_0_3;
    lcd_request.msg.payload.tile.col = 0;
    lcd_request.msg.payload.tile.number = 0;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
}

/**
 * @brief
 * This task will be used to verify the functionality of the IPC UART specification
 * by ....
 *
 * @param arg 
 * Unused parameter
 */
void task_system_control(void *arg)
{
    (void)arg; // Unused parameter
    EventBits_t events;

    uint16_t sequence_num = 0;

    /* Begin the discovery process. */
    discover_board(&sequence_num);
    sequence_num++;

    // Grab initial cypher
    uint32_t cypher = number_select();
    

    while(1)
    {
        // Wait for SW1, SW2,or SW3 to be pressed.  If you have not gotten task_buttons.c working yet, 
        // you will need to do so before you can proceed with this task. 
        events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED | ECE353_BUTTON_3_PRESSED,
                                    pdTRUE,
                                    pdFALSE,
                                    portMAX_DELAY);

        if(events & ECE353_BUTTON_1_PRESSED)
        {

            /* Send the active player message  */
            ipc_send_active_player(sequence_num);

            /* Wait for the ack */
            bool rslt = ipc_wait_for_ack(1000);

            /* Print out a message indicating if the ACK was received */
            if(rslt)
            {
                printf("Active Player message sent and ACK received!\n\r");
                sequence_num++;
            }
            else
            {
                printf("Active Player message sent but no ACK received!\n\r");
            }

        }
        else if(events & ECE353_BUTTON_2_PRESSED)
        {
            
            /* Send the inactive player message  */
            ipc_send_inactive_player(sequence_num);
            
            /* Wait for the ack */ 
            bool rslt = ipc_wait_for_ack(1000);

            /* Print out a message indicating if the ACK was received */
            if(rslt)
            {
                printf("Inactive Player message sent and ACK received!\n\r");
                sequence_num++;
            }
            else
            {
                printf("Inactive Player message sent but no ACK received!\n\r");
            }

        }
        else if(events & ECE353_BUTTON_3_PRESSED)
        {

            /* Send the status message with an error code  */
            ipc_send_status(sequence_num, IPC_STATUS_CRC_FAIL);

            /* Wait for the ack */
            bool rslt = ipc_wait_for_ack(1000);
            
            /* Print out a message indicating if the ACK was received */
            if(rslt)
            {
                printf("Status message sent and ACK received!\n\r");
                sequence_num++;
            }
            else
            {
                printf("Status message sent but no ACK received!\n\r");
            }
        }
        else
        {
             printf("Unknown Event!\n\r\n\r");
        }
    }
}


/**
 * @brief
 * This function is used to initialize any semaphores used in the application.
 *
 * The I2C and SPI busses are both shared resources that will require a
 * semaphore to protect access to them.  You should create a binary semaphore
 * for each bus and give the semaphore once after creating it to ensure that
 * it is available for use.
 */
static void hw05_semaphores_init(void) {
  I2C_Semaphore = xSemaphoreCreateBinary();
  SPI_Semaphore = xSemaphoreCreateBinary();

  xSemaphoreGive(I2C_Semaphore);
  xSemaphoreGive(SPI_Semaphore);
}



/*************************************************
 * @brief
 * This function will initialize all of the hardware resources for
 * the ICE
 ************************************************/
void app_init_hw(void)
{
    cy_rslt_t rslt;

    console_init();
    // Plain serial output: no ANSI color/theme escape sequences
    printf("\x1b[2J\x1b[;H");
    printf("**************************************************\n\r");
    printf("* %s\n\r", APP_DESCRIPTION);
    printf("* Date: %s\n\r", __DATE__);
    printf("* Time: %s\n\r", __TIME__);
    printf("* Name:%s\n\r", NAME);
    printf("**************************************************\n\r");

    /* Initialize the I2C interface */
    I2C_Monarch_Obj = i2c_init(PIN_I2C_SDA, PIN_I2C_SCL);
    if (I2C_Monarch_Obj == NULL) {
        printf("I2C Initialization Failed!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    /* Initialize SPI Interface */
    SPI_Monarch_Obj = spi_init(PIN_SPI_MOSI, PIN_SPI_MISO, PIN_SPI_CLK);
    if (SPI_Monarch_Obj == NULL) {
        printf("SPI Initialization Failed!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    /* Initialize the CS pin for the EEPROM */
    cyhal_gpio_init(PIN_SPI_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT,
                    CYHAL_GPIO_DRIVE_STRONG, 1);

    // Initialize buttons
    rslt = buttons_init_gpio();
    if(rslt != CY_RSLT_SUCCESS)
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    rslt = lcd_initialize();
    if (rslt != CY_RSLT_SUCCESS)
    {
        printf("LCD initialization failed!\n\r");
        for(int i = 0; i < 100000; i++) {}
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
    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();

    /* Create the FreeRTOS queues */
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));

    // Initialize semaphores
    hw05_semaphores_init();

    if(!task_button_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the System Control Task
    xTaskCreate(
        task_system_control, 
        "System Control", 
        configMINIMAL_STACK_SIZE*5,      
        NULL, 
        2, 
        NULL
    );
    
    /* Start the scheduler*/
    vTaskStartScheduler();

    /* Will never reach this loop once the scheduler starts */
    while (1)
    {
    }
}
#endif