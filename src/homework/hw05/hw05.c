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
#include "task_light_sensor.h"

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

/* Dark mode flag: true when ambient light is low (dark environment).
 * Polled and updated by the task_ambient_light task. */
volatile bool Dark_Mode = true;

/* Threshold for ambient light sensor reading.
 * Readings below this value are considered "dark" (dark mode).
 * Readings at or above this value are considered "light" (light mode). */
#define LIGHT_THRESHOLD 100

// Store the code that the other player sent us for use in the game
uint16_t cypher = 0;

/*****************************************************************************/
/* Theme-aware color helpers                                                 */
/*****************************************************************************/

/* Returns the current background color for tiles based on Dark_Mode */
static inline uint16_t theme_bg(void)
{
    return Dark_Mode ? LCD_COLOR_BLACK : LCD_COLOR_WHITE;
}

/* Returns the current text foreground color (for status messages) */
static inline uint16_t theme_text_fg(void)
{
    return Dark_Mode ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
}

/* Returns the current text background color (for status messages) */
static inline uint16_t theme_text_bg(void)
{
    return Dark_Mode ? LCD_COLOR_BLACK : LCD_COLOR_WHITE;
}

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

/**
 * @brief
 * Redraws the full number-select UI: clears the screen with the current theme
 * background, reprints the status text, redraws all 8 number tiles (0-7),
 * redraws the cypher row with the currently selected values, and highlights
 * the active cursor position.
 *
 * @param prompt        The text to display in the top message area
 * @param selected      Array of 4 selected digit values
 * @param num_selected  How many digits have been chosen so far (0-4)
 */
static void redraw_ui(const char *prompt, const int selected[4], int num_selected)
{
    lcd_msg_request_t lcd_request;
    lcd_request.return_queue = NULL;

    /* Step 1: Clear the screen with the current theme background */
    lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    /* Step 2: Reprint the status text */
    print_top_lcd(prompt);

    /* Step 3: Redraw the 8 number-input tiles (rows 1-2, cols 0-3) */
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = theme_bg();
    for (int row = 1; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            lcd_request.msg.payload.tile.row = row;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = col + (row - 1) * 4;
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }

    /* Step 4: Redraw the cypher row tiles with their current values */
    for (int col = 0; col < 4; col++) {
        if (col < num_selected) {
            /* Already-selected digit: show the chosen number */
            lcd_request.msg.command = LCD_CMD_DRAW_TILE;
            lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = selected[col];
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
            lcd_request.msg.payload.tile.color_bg = theme_bg();
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        } else if (col == num_selected) {
            /* Active cursor position: draw inverted (highlighted) blank */
            lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
            lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = 0;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
            lcd_request.msg.payload.tile.color_bg = theme_bg();
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        } else {
            /* Future digit slot: draw normal blank */
            lcd_request.msg.command = LCD_CMD_DRAW_TILE;
            lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = 0;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
            lcd_request.msg.payload.tile.color_bg = theme_bg();
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }
}

/**
 * @brief
 * Redraws the screen during a waiting state (e.g. waiting for other player).
 * Clears the screen background, reprints the text message, and redraws
 * the number tiles (0-7) and the cypher row with the given packed digits.
 *
 * @param message         The text to display in the top message area
 * @param packed_digits   The 4 packed digits to show in the cypher row (4 nibbles)
 */
static void redraw_wait_screen(const char *message, uint16_t packed_digits)
{
    lcd_msg_request_t lcd_request;
    lcd_request.return_queue = NULL;

    /* Clear the screen with the current theme background */
    lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    /* Reprint the status text */
    print_top_lcd(message);

    /* Redraw the 8 number-input tiles (rows 1-2, cols 0-3) */
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = theme_bg();
    for (int row = 1; row < 3; row++) {
        for (int col = 0; col < 4; col++) {
            lcd_request.msg.payload.tile.row = row;
            lcd_request.msg.payload.tile.col = col;
            lcd_request.msg.payload.tile.number = col + (row - 1) * 4;
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        }
    }

    /* Unpack the 4 digits and redraw the cypher row */
    int digits[4];
    digits[0] = (packed_digits >> 12) & 0xF;
    digits[1] = (packed_digits >> 8)  & 0xF;
    digits[2] = (packed_digits >> 4)  & 0xF;
    digits[3] =  packed_digits        & 0xF;

    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
    lcd_request.msg.payload.tile.color_bg = theme_bg();
    for (int col = 0; col < 4; col++) {
        lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
        lcd_request.msg.payload.tile.col = col;
        lcd_request.msg.payload.tile.number = digits[col];
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    }
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
        lcd_request.msg.payload.tile.color_bg = theme_bg();
        
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
    }


    /* Draw numbers 0-7 for the user input*/
    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
    lcd_request.msg.payload.tile.color_bg = theme_bg();
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

            /* Check if the ambient light theme changed; if so, redraw the full UI */
            EventBits_t theme_bits = xEventGroupWaitBits(ECE353_RTOS_Events,
                                                          ECE353_THEME_CHANGED,
                                                          pdTRUE, pdFALSE, 0);
            if (theme_bits & ECE353_THEME_CHANGED)
            {
                redraw_ui(select_prompt, selected_numbers, number_selected);
            }

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
                    lcd_request.msg.payload.tile.color_bg = theme_bg();
                    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                    number_selected--;
                    selected_numbers[number_selected] = 0;

                    // Clear and highlight the digit that will be re-selected.
                    lcd_request.msg.command = LCD_CMD_DRAW_TILE;
                    lcd_request.msg.payload.tile.col = number_selected;
                    lcd_request.msg.payload.tile.number = 0;
                    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                    lcd_request.msg.payload.tile.color_bg = theme_bg();
                    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                    lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                    lcd_request.msg.payload.tile.col = number_selected;
                    lcd_request.msg.payload.tile.number = 0;
                    lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                    lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                    lcd_request.msg.payload.tile.color_bg = theme_bg();
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
            lcd_request.msg.payload.tile.color_bg = theme_bg();
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            
            // Highlight the next tile to be selected (only if there is one)
            if (number_selected + 1 < 4) {
                lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                lcd_request.msg.payload.tile.col = number_selected + 1;
                lcd_request.msg.payload.tile.number = 0;
                lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                lcd_request.msg.payload.tile.color_bg = theme_bg();
                xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
            }

            // Highlight the current tile that is being touched. Tile till touch is released later
            lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
            lcd_request.msg.payload.tile.col = selected_numbers[number_selected] % 4;
            lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
            lcd_request.msg.payload.tile.row = (selected_numbers[number_selected] / 4) + 1;
            lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
            lcd_request.msg.payload.tile.color_bg = theme_bg();
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
            lcd_request.msg.payload.tile.color_bg = theme_bg();
            xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

            number_selected++;
        }

        print_top_lcd("SW1: Send SW2: Delete");

        /* Wait for a button press, but also check for theme changes */
        EventBits_t events;
        while (1)
        {
            events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                          ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED | ECE353_THEME_CHANGED,
                                          pdTRUE,
                                          pdFALSE,
                                          portMAX_DELAY);

            if (events & ECE353_THEME_CHANGED)
            {
                /* Theme changed while waiting for confirm/delete; redraw and keep waiting */
                redraw_ui("SW1: Send SW2: Delete", selected_numbers, number_selected);
                continue;
            }
            break; /* Got a button press, proceed */
        }

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
                lcd_request.msg.payload.tile.color_bg = theme_bg();
                xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

                lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
                lcd_request.msg.payload.tile.col = number_selected;
                lcd_request.msg.payload.tile.number = 0;
                lcd_request.msg.payload.tile.row = LCD_TILE_ROW_CYPHER;
                lcd_request.msg.payload.tile.color_fg = LCD_COLOR_RED;
                lcd_request.msg.payload.tile.color_bg = theme_bg();
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

    // Wait for other user's cypher — also handle theme changes
    while (1)
    {
        events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                    ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
                                    pdTRUE,
                                    pdFALSE,
                                    portMAX_DELAY);
        if (events & ECE353_THEME_CHANGED)
        {
            /* Theme changed: redraw screen with tiles showing the sent cypher */
            redraw_wait_screen("Cypher Sent!   ", code);
            continue;
        }
        break; /* Got IPC_NUM_RECEIVED */
    }

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

            // Wait your turn — also handle theme changes while waiting
            print_top_lcd("Waiting for other player's guess...");
            while (1)
            {
                events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                            ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
                                            pdTRUE,
                                            pdFALSE,
                                            portMAX_DELAY);
                if (events & ECE353_THEME_CHANGED)
                {
                    /* Theme changed: redraw screen with tiles showing the last guess */
                    redraw_wait_screen("Waiting for other player's guess...", guess);
                    continue;
                }
                break; /* Got IPC_NUM_RECEIVED */
            }
        } while (guess != cypher | Sent_Code != code);
    } else {
        do {
            // Wait your turn — also handle theme changes while waiting
            print_top_lcd("Waiting for other player's guess...");
            while (1)
            {
                events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                            ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
                                            pdTRUE,
                                            pdFALSE,
                                            portMAX_DELAY);
                if (events & ECE353_THEME_CHANGED)
                {
                    /* Theme changed: redraw screen with tiles showing the sent cypher
                     * (P2 hasn't guessed yet on first iteration, use code as fallback) */
                    redraw_wait_screen("Waiting for other player's guess...", (guess != 0) ? guess : code);
                    continue;
                }
                break; /* Got IPC_NUM_RECEIVED */
            }

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
 * Task that periodically reads the ambient light sensor and updates Dark_Mode.
 * When the mode changes, it sends an LCD_CMD_UPDATE_THEME command so the LCD
 * task repaints the background, and then sends a LCD_CMD_PRINT_MESSAGE to
 * redisplay the current status message in the correct colors.
 *
 * @param arg
 * Unused parameter
 */
void task_ambient_light(void *arg)
{
    (void)arg;

    /* Create a private response queue for sensor readings */
    QueueHandle_t light_response_queue = xQueueCreate(1, sizeof(device_response_msg_t));
    if (light_response_queue == NULL)
    {
        printf("Failed to create light sensor response queue!\n\r");
        vTaskSuspend(NULL);
    }

    uint16_t ambient_light = 0;

    /* Perform an initial read to set Dark_Mode before any UI is drawn */
    if (system_sensors_get_light(light_response_queue, &ambient_light))
    {
        Dark_Mode = (ambient_light < LIGHT_THRESHOLD);
        task_console_printf("Initial ambient light: %u -> %s mode\n\r",
                            ambient_light, Dark_Mode ? "DARK" : "LIGHT");
    }

    /* Continuously poll the light sensor and detect changes */
    while (1)
    {
        /* Wait 500 ms between readings to match the sensor measurement rate */
        vTaskDelay(pdMS_TO_TICKS(500));

        /* Read the ambient light level from the sensor */
        if (!system_sensors_get_light(light_response_queue, &ambient_light))
        {
            /* Sensor read failed; try again next cycle */
            continue;
        }

        /* Determine the new mode from the sensor reading */
        bool new_dark_mode = (ambient_light < LIGHT_THRESHOLD);

        /* Only update the UI when the mode actually changes */
        if (new_dark_mode != Dark_Mode)
        {
            Dark_Mode = new_dark_mode;

            task_console_printf("Ambient light: %u -> switching to %s mode\n\r",
                                ambient_light, Dark_Mode ? "DARK" : "LIGHT");

            /* Set event bit so the game loops know to redraw the UI */
            xEventGroupSetBits(ECE353_RTOS_Events, ECE353_THEME_CHANGED);
        }
    }
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

    // Initialize the light sensor task resources
    if(!task_light_sensor_resources_init(&I2C_Semaphore, I2C_Monarch_Obj))
    {
        printf("Light Sensor Task initialization failed!\n\r");
        for(int i = 0; i < 10000; i++);
        CY_ASSERT(0);
    }

    // Create the ambient light monitoring task (runs continuously)
    xTaskCreate(
        task_ambient_light,
        "Ambient Light",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

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