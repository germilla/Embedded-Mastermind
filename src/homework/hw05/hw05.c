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
#include "task_console.h"
#include "task_cap_touch.h"
#include "task_eeprom.h"
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
QueueHandle_t Response_Queue;

bool Cypher_Chosen = 0;
int guess_number = 1;

// Store the code that the other player sent us for use in the game
uint16_t cypher = 0;

/*****************************************************************************/
/* Function Definitions                                                      */
/*****************************************************************************/

/* This function will be used to discover other board. This function should not
 * return until the discovery is complete.  The discovery is complete when we
 * receive a discovery message from the other board OR we send a discovery
 * message that is Acked by the other board */
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

/* Helper method to print a message to the text area at the top of the LCD */
void print_top_lcd(const char *message) {
  lcd_msg_request_t lcd_request;
  lcd_request.msg.command = LCD_CMD_PRINT_MESSAGE;
  lcd_request.return_queue = NULL;
    snprintf(lcd_request.msg.payload.message, 32, "%-25s", message);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
}

/* At start of game show the numbers on the screen and allow user to chose 4 numbers, then returns those 4 numbers */
uint16_t number_select(void) {
    /* Allocate a lcd_msg_request_t variable */
    lcd_msg_request_t lcd_request;
    char select_prompt[32];
    snprintf(select_prompt, 32, (!Cypher_Chosen) ? "Select Your Cypher!     " : "Select Your Guess #%d!    ", guess_number);

    /* Write a message to the user in the Text Area of the screen*/
    lcd_request.msg.command = LCD_CMD_PRINT_MESSAGE;
    lcd_request.return_queue = NULL;
    snprintf(lcd_request.msg.payload.message, 32, (!Cypher_Chosen) ? "Select Your Cypher!     " : "Select Your Guess #%d!    ", guess_number);
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

    /* Highlight the first cypher digit*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
    lcd_request.msg.payload.tile.col = 0;
    lcd_request.msg.payload.tile.number = 0;
    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    // Wait for 4 button from the capactive touch sensor and update the cypher tiles accordingly
    int selected_numbers[4] = {0};
    int number_selected = 0;
    device_response_msg_t response;
    device_request_msg_t request;

    while (1)
    {
        while (number_selected < 4) {
            EventBits_t button_events;

            // Wait for user to select a number
            vTaskDelay(100);

            // Process SW1/SW2 while selecting digits.
            button_events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                                ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED,
                                                pdTRUE,
                                                pdFALSE,
                                                0);

            if (button_events & ECE353_BUTTON_2_PRESSED)
            {
                if (number_selected > 0)
                {
                    // Clear the currently highlighted cursor tile.
                    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
                    lcd_request.msg.payload.tile.col = number_selected;
                    lcd_request.msg.payload.tile.number = 0;
                    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                    number_selected--;
                    selected_numbers[number_selected] = 0;

                    // Clear and highlight the digit that will be re-selected.
                    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
                    lcd_request.msg.payload.tile.col = number_selected;
                    lcd_request.msg.payload.tile.number = 0;
                    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                    lcd_request.msg.payload.tile.col = number_selected;
                    lcd_request.msg.payload.tile.number = 0;
                    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                    lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
                }

                print_top_lcd(select_prompt);
                continue;
            }

            if (button_events & ECE353_BUTTON_1_PRESSED)
            {
                print_top_lcd("Enter 4 digits first");
                continue;
            }

            // Parse request and setup queue for response
            request.response_queue = Response_Queue;
            parse_cli_data("CAP_TOUCH", &request);
            
            // Send the request to the capacitive touch task and wait for a touch
            xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY);
            xQueueReceive(Response_Queue, &response, portMAX_DELAY);

            if (response.status != DEVICE_OPERATION_STATUS_READ_SUCCESS) {
                continue;
            }
            
            // Parse the response and determine which tile was touched
            uint16_t touch_x = response.payload.cap_touch[0];
            uint16_t touch_y = response.payload.cap_touch[1];

            // Determine which row and column the touch falls in
            int touched_row = -1;
            int touched_col = -1;

            // Check rows 1 and 2 (the number input rows)
            for (int row = 1; row < 3; row++) {
                int top_y = lcd_tile_top_y(row);
                if (touch_y >= top_y && touch_y < top_y + TILE_H) {
                    touched_row = row;
                    break;
                }
            }

            // Check columns 0-3
            for (int col = 0; col < 4; col++) {
                int left_x = lcd_tile_left_x(col);
                if (touch_x >= left_x && touch_x < left_x + TILE_W) {
                    touched_col = col;
                    break;
                }
            }

            // If the touch was not on a valid tile, try again
            if (touched_row == -1 || touched_col == -1) {
                continue;
            }

            // Convert row/col to tile number: row 1 has 0-3, row 2 has 4-7
            selected_numbers[number_selected] = touched_col + (touched_row - 1) * 4;

            // Update the top to show the selected number
            lcd_request.msg.command = LCD_CMD_DRAW_TILE;
            lcd_request.msg.payload.tile.col = number_selected;
            lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
            lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            
            // Highlight the next tile to be selected (only if there is one)
            if (number_selected + 1 < 4) {
                lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                lcd_request.msg.payload.tile.col = number_selected + 1;
                lcd_request.msg.payload.tile.number = 0;
                lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            }

            // Highlight the current tile that is being touched. Tile till touch is released later
            lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
            lcd_request.msg.payload.tile.col = selected_numbers[number_selected] % 4;
            lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
            lcd_request.msg.payload.tile.row = (selected_numbers[number_selected] / 4) + 1;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

            // Wait for the user to release the screen and unhighlight the current tile
            do {
                xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY);
                xQueueReceive(Response_Queue, &response, portMAX_DELAY);
            } while (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS);

            lcd_request.msg.command = LCD_CMD_DRAW_TILE;
            lcd_request.msg.payload.tile.col = selected_numbers[number_selected] % 4;
            lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
            lcd_request.msg.payload.tile.row = (selected_numbers[number_selected] / 4) + 1;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

            number_selected++;
        }

        print_top_lcd("SW1: Send SW2: Delete");

        EventBits_t events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                                  ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED,
                                                  pdTRUE,
                                                  pdFALSE,
                                                  portMAX_DELAY);

        if (events & ECE353_BUTTON_2_PRESSED)
        {
            if (number_selected > 0)
            {
                number_selected--;
                selected_numbers[number_selected] = 0;

                // Clear and re-highlight the digit that will be re-selected.
                lcd_request.msg.command = LCD_CMD_DRAW_TILE;
                lcd_request.msg.payload.tile.col = number_selected;
                lcd_request.msg.payload.tile.number = 0;
                lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                lcd_request.msg.payload.tile.col = number_selected;
                lcd_request.msg.payload.tile.number = 0;
                lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                lcd_request.msg.payload.tile.color_bg = LCD_COLOR_BLACK;
                xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            }

            print_top_lcd(select_prompt);
            continue;
        }

        if ((events & ECE353_BUTTON_1_PRESSED) && (number_selected == 4))
        {
            uint16_t selected_cypher =
                (selected_numbers[0] << 12) |
                (selected_numbers[1] << 8) |
                (selected_numbers[2] << 4) |
                selected_numbers[3];
            print_top_lcd("Cypher Sent!   ");
            return selected_cypher;
        }
    }
}

/**
 * @brief
 * This task will be used to run the game functionality. MAIN LOOP
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
    uint16_t code = number_select();

    Cypher_Chosen = true;

    task_console_printf("Selected Cypher: 0x%X\n\r", code);

    bool P1 = !(xEventGroupGetBits(ECE353_RTOS_Events) & ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED);

    /* Send the cypher to the other board */
    ipc_send_number(sequence_num, code);

    /* Wait for the ack */
    bool rslt = ipc_wait_for_ack(1000);

    /* Print out a message indicating if the ACK was received */
    if(rslt)
    {
        printf("Cypher message sent and ACK received!\n\r");
        sequence_num++;
    }
    else
    {
        printf("Cypher message sent but no ACK received!\n\r");
    }

    // Wait for other user's cypher
    events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED,
                                pdTRUE,
                                pdFALSE,
                                portMAX_DELAY);

    cypher = Sent_Code;

    task_console_printf("Other User's Cypher 0x%X!\n\r", cypher);

    task_console_printf("Starting Game! You are Player %d\n\r", P1 ? 1 : 2);

    // Main game loop. Keep taking guesses until either the user guesses the other player's cypher or the other player guesses the user's cypher. If both players guess each other's cypher on the same turn, it's a tie.
    uint16_t guess = 0;
    if (P1) {
        do {
            // Take guess
            guess = number_select();
            guess_number++;
            task_console_printf("You guessed: 0x%X\n\r", guess);

            // Parse the guess and determine how much is right
            // TODO TODO TODO
            
            // Send guess to other player
            ipc_send_number(sequence_num, guess);

            // Wait for ack
            rslt = ipc_wait_for_ack(1000);

            // Wait your turn
            print_top_lcd("Waiting for other player's guess...");
            events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                        ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED,
                                        pdTRUE,
                                        pdFALSE,
                                        portMAX_DELAY);
        } while (guess != cypher | Sent_Code != code);
    } else {
        do {
            // Wait your turn
            print_top_lcd("Waiting for other player's guess...");
            events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                        ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED,
                                        pdTRUE,
                                        pdFALSE,
                                        portMAX_DELAY);

            // Take guess
            guess = number_select();
            guess_number++;
            task_console_printf("You guessed: 0x%X\n\r", guess);

            // Parse the guess and determine how much is right
            // TODO TODO TODO

            // Send guess to other player
            ipc_send_number(sequence_num, guess);

            // Wait for ack
            rslt = ipc_wait_for_ack(1000);
        } while (guess != cypher | Sent_Code != code);
    }

    // Determine win/lose/tie and print result to console and LCD
    char* message;
    if (guess == cypher && Sent_Code == code) {
        message = "It's a tie!                ";
        task_console_printf("You and the other player guessed each other's cypher at the same time!\n\r");
    } else if (guess == cypher) {
        message = "You Win!                      ";
        task_console_printf("You win!\n\r");
    } else {
        message = "You Lose!                  ";
        task_console_printf("You Lose!\n\r");
    }
    print_top_lcd(message);

    
}

/**
 * @brief
 * This function is used to initialize any rtos connection used in the application.
 */
static void hw05_rtos_init(void) {

    // Create the semaphores for I2C and SPI
    I2C_Semaphore = xSemaphoreCreateBinary();
    if (I2C_Semaphore == NULL)
    {
        printf("Failed to create I2C semaphore!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    SPI_Semaphore = xSemaphoreCreateBinary();
    if (SPI_Semaphore == NULL)
    {
        printf("Failed to create SPI semaphore!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    xSemaphoreGive(I2C_Semaphore);
    xSemaphoreGive(SPI_Semaphore);

    // Initialize the EventGroup
    ECE353_RTOS_Events = xEventGroupCreate();

    /* Create the FreeRTOS queues */
    xQueue_Request_LCD = xQueueCreate(10, sizeof(lcd_msg_request_t));
    Queue_Request_Cap_Touch = xQueueCreate(1, sizeof(device_request_msg_t));
    Response_Queue = xQueueCreate(1, sizeof(device_response_msg_t));
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

    /* Initialize the CS pin for the EEPROM */
    cyhal_gpio_init(PIN_SPI_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);

    /* Initialize the interrupt pin for the capacitive touch sensor */
    cyhal_gpio_init(PIN_CAP_TOUCH_INT, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 0);


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
    // Initialize rtos connections
    hw05_rtos_init();

    // Initialize console
    if (!task_console_init()) {
        printf("Console Task resource initialization failed!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    // Initialize the resources for eeprom task
    if (!task_eeprom_resources_init(&SPI_Semaphore, SPI_Monarch_Obj, PIN_SPI_EEPROM_CS)){
        printf("EEPROM Task resource initialization failed!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    // Initialize the resources for the capacitive touch task
    if (!task_cap_touch_resources_init(Queue_Request_Cap_Touch, I2C_Semaphore, I2C_Monarch_Obj, PIN_CAP_TOUCH_INT)) {
        printf("Cap Touch Task resource initialization failed!\n\r");
        for (int i = 0; i < 100000; i++) {
        }
        CY_ASSERT(0);
    }

    // Initialize buttons
    if(!task_button_init())
    {
        printf("Button initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Initialize IPC
    if(!task_ipc_init())
    {
        printf("IPC initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Initialize lcd task
    if(!task_lcd_resources_init(xQueue_Request_LCD))
    {
        printf("LCD Task initialization failed!\n\r");
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