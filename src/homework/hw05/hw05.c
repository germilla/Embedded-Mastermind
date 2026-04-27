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
#include "rtos_events.h"
#include "task_buttons.h"
#include "task_cap_touch.h"
#include "task_console.h"
#include "task_eeprom.h"
#include "task_ipc.h"
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
static inline uint16_t theme_bg(void) {
  return Dark_Mode ? LCD_COLOR_BLACK : LCD_COLOR_WHITE;
}

/* Returns the current text foreground color (for status messages) */
static inline uint16_t theme_text_fg(void) {
  return Dark_Mode ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
}

/* Returns the current text background color (for status messages) */
static inline uint16_t theme_text_bg(void) {
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
static void redraw_ui(const char *prompt, const int selected[4],
                      int num_selected) {
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
 * @param packed_digits   The 4 packed digits to show in the cypher row (4
 * nibbles)
 */
static void redraw_wait_screen(const char *message, uint16_t packed_digits) {
  lcd_msg_request_t lcd_request;
  lcd_request.return_queue = NULL;

  /* Clear the screen with the current theme background */
  lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Reprint the status text only — no tiles during the waiting state */
  print_top_lcd(message);
}

/**
 * @brief
 * Evaluates a 4-digit guess against a 4-digit cypher.
 * Uses the standard Master Mind algorithm:
 *   1. First pass: count exact matches (correct digit in correct position).
 *   2. Second pass: for each remaining guess digit, check if it appears
 *      among the remaining cypher digits (correct digit in wrong position).
 *
 * @param guess_packed   The 4-digit guess packed into a uint16_t (4 nibbles)
 * @param cypher_packed  The 4-digit secret cypher packed into a uint16_t (4
 * nibbles)
 * @param exact_out      Pointer to store the count of exact-position matches
 * @param wrong_pos_out  Pointer to store the count of wrong-position matches
 */
static void evaluate_guess(uint16_t guess_packed, uint16_t cypher_packed,
                           int *exact_out, int *wrong_pos_out) {
  /* Unpack both the guess and cypher into 4-element arrays */
  int g[4], c[4];
  for (int i = 0; i < 4; i++) {
    g[i] = (guess_packed >> (12 - i * 4)) & 0xF;
    c[i] = (cypher_packed >> (12 - i * 4)) & 0xF;
  }

  /* Track which positions have been matched so we don't double-count */
  bool g_used[4] = {false, false, false, false};
  bool c_used[4] = {false, false, false, false};

  /* First pass: count exact matches (correct digit AND correct position) */
  int exact = 0;
  for (int i = 0; i < 4; i++) {
    if (g[i] == c[i]) {
      exact++;
      g_used[i] = true;
      c_used[i] = true;
    }
  }

  /* Second pass: count wrong-position matches.
   * For each unmatched guess digit, look for an unmatched cypher digit
   * with the same value. */
  int wrong_pos = 0;
  for (int i = 0; i < 4; i++) {
    if (g_used[i])
      continue; /* Already an exact match */
    for (int j = 0; j < 4; j++) {
      if (c_used[j])
        continue; /* Already matched */
      if (g[i] == c[j]) {
        wrong_pos++;
        c_used[j] = true; /* Mark this cypher digit as consumed */
        break;
      }
    }
  }

  *exact_out = exact;
  *wrong_pos_out = wrong_pos;
}

/**
 * @brief
 * Displays the feedback screen after a guess is submitted.
 * Clears the LCD (removing all number tiles), then shows two lines:
 *   Line 1: "X correct pos & num"
 *   Line 2: "X correct num wrong pos"
 * The screen stays on this feedback view until SW1 is pressed,
 * at which point the function returns so the game can continue.
 *
 * @param guess      The packed 4-digit guess (4 nibbles in a uint16_t)
 * @param exact      Number of digits in the correct position
 * @param wrong_pos  Number of correct digits in wrong positions
 */
static void show_feedback_screen(uint16_t guess, int exact, int wrong_pos) {
  lcd_msg_request_t lcd_request;
  lcd_request.return_queue = NULL;

  /* Step 1: Clear the entire screen to the current theme background */
  lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Step 2: Print the title showing which code was guessed */
  char title[32];
  snprintf(title, 32, "Guess Feedback - %d%d%d%d", (guess >> 12) & 0xF,
           (guess >> 8) & 0xF, (guess >> 4) & 0xF, guess & 0xF);
  print_top_lcd(title);

  /* Step 3: Print exact-match line using SW1_COUNT position (y=50) */
  char line1[32];
  snprintf(line1, 32, "%d correct pos & num  ", exact);
  lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
  lcd_request.return_queue = NULL;
  snprintf(lcd_request.msg.payload.message, 32, "%s", line1);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Step 4: Print wrong-position line using SW2_COUNT position (y=100) */
  char line2[32];
  snprintf(line2, 32, "%d right num wrong pos", wrong_pos);
  lcd_request.msg.command = LCD_CMD_PRINT_SW2_COUNT;
  lcd_request.return_queue = NULL;
  snprintf(lcd_request.msg.payload.message, 32, "%s", line2);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Display a prompt below the feedback lines so the user knows to press SW1 */
  lcd_request.msg.command = LCD_CMD_PRINT_LINE3;
  lcd_request.return_queue = NULL;
  snprintf(lcd_request.msg.payload.message, 32, "Press SW1 to continue");
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Step 5: Wait for SW1 press to continue.
   * Also handle theme changes so the feedback screen repaints correctly. */
  while (1) {
    EventBits_t events = xEventGroupWaitBits(
        ECE353_RTOS_Events, ECE353_BUTTON_1_PRESSED | ECE353_THEME_CHANGED,
        pdTRUE, pdFALSE, portMAX_DELAY);

    if (events & ECE353_THEME_CHANGED) {
      /* Redraw the feedback screen in the new theme by re-entering the
       * function. This avoids duplicating all the drawing logic above. */
      show_feedback_screen(guess, exact, wrong_pos);
      return;
    }

    if (events & ECE353_BUTTON_1_PRESSED) {
      /* SW1 pressed: clear the screen before returning to the game */
      lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
      lcd_request.return_queue = NULL;
      xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
      break;
    }
  }
}

/* At start of game show the numbers on the screen and allow user to chose 4
 * numbers, then returns those 4 numbers */
uint16_t number_select(void) {
  /* Allocate a lcd_msg_request_t variable */
  lcd_msg_request_t lcd_request;

  /* Clear the screen first so any leftover content (e.g. feedback text) is gone
   */
  lcd_request.return_queue = NULL;
  lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
  char select_prompt[32];
  snprintf(select_prompt, 32,
           (!Cypher_Chosen) ? "Select Your Cypher!     "
                            : "Select Your Guess #%d!    ",
           guess_number);

  /* Write a message to the user in the Text Area of the screen*/
  lcd_request.msg.command = LCD_CMD_PRINT_MESSAGE;
  lcd_request.return_queue = NULL;
  snprintf(lcd_request.msg.payload.message, 32,
           (!Cypher_Chosen) ? "Select Your Cypher!     "
                            : "Select Your Guess #%d!    ",
           guess_number);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Draw 4 blank tiles for the secret code */
  for (int col = 0; col < 4; col++) {
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
    for (int col = 0; col < 4; col++) {
      lcd_request.msg.payload.tile.row = row;
      lcd_request.msg.payload.tile.col = col;
      lcd_request.msg.payload.tile.number = col + (row - 1) * 4;

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

  // Wait for 4 button from the capactive touch sensor and update the cypher
  // tiles accordingly
  int selected_numbers[4] = {0};
  int number_selected = 0;
  device_response_msg_t response;
  device_request_msg_t request;

  while (1) {
    while (number_selected < 4) {
      EventBits_t button_events;

      // Wait for user to select a number
      vTaskDelay(100);

      /* Check if the ambient light theme changed; if so, redraw the full UI */
      EventBits_t theme_bits = xEventGroupWaitBits(
          ECE353_RTOS_Events, ECE353_THEME_CHANGED, pdTRUE, pdFALSE, 0);
      if (theme_bits & ECE353_THEME_CHANGED) {
        redraw_ui(select_prompt, selected_numbers, number_selected);
      }

      // Process SW1/SW2 while selecting digits.
      button_events = xEventGroupWaitBits(
          ECE353_RTOS_Events, ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_2_PRESSED,
          pdTRUE, pdFALSE, 0);

      if (button_events & ECE353_BUTTON_2_PRESSED) {
        if (number_selected > 0) {
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

      if (button_events & ECE353_BUTTON_1_PRESSED) {
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

      // Highlight the current tile that is being touched. Tile till touch is
      // released later
      lcd_request.msg.command = LCD_CMD_DRAW_TILE_INVERTED;
      lcd_request.msg.payload.tile.col = selected_numbers[number_selected] % 4;
      lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
      lcd_request.msg.payload.tile.row =
          (selected_numbers[number_selected] / 4) + 1;
      lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
      lcd_request.msg.payload.tile.color_bg = theme_bg();
      xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

      // Wait for the user to release the screen and unhighlight the current
      // tile
      do {
        xQueueSend(Queue_Request_Cap_Touch, &request, portMAX_DELAY);
        xQueueReceive(Response_Queue, &response, portMAX_DELAY);
      } while (response.status == DEVICE_OPERATION_STATUS_READ_SUCCESS);

      lcd_request.msg.command = LCD_CMD_DRAW_TILE;
      lcd_request.msg.payload.tile.col = selected_numbers[number_selected] % 4;
      lcd_request.msg.payload.tile.number = selected_numbers[number_selected];
      lcd_request.msg.payload.tile.row =
          (selected_numbers[number_selected] / 4) + 1;
      lcd_request.msg.payload.tile.color_fg = LCD_COLOR_GREEN;
      lcd_request.msg.payload.tile.color_bg = theme_bg();
      xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

      number_selected++;
    }

    print_top_lcd("SW1: Send SW2: Delete");

    /* Wait for a button press, but also check for theme changes */
    EventBits_t events;
    while (1) {
      events = xEventGroupWaitBits(ECE353_RTOS_Events,
                                   ECE353_BUTTON_1_PRESSED |
                                       ECE353_BUTTON_2_PRESSED |
                                       ECE353_THEME_CHANGED,
                                   pdTRUE, pdFALSE, portMAX_DELAY);

      if (events & ECE353_THEME_CHANGED) {
        /* Theme changed while waiting for confirm/delete; redraw and keep
         * waiting */
        redraw_ui("SW1: Send SW2: Delete", selected_numbers, number_selected);
        continue;
      }
      break; /* Got a button press, proceed */
    }

    if (events & ECE353_BUTTON_2_PRESSED) {
      if (number_selected > 0) {
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

    if ((events & ECE353_BUTTON_1_PRESSED) && (number_selected == 4)) {
      uint16_t selected_cypher =
          (selected_numbers[0] << 12) | (selected_numbers[1] << 8) |
          (selected_numbers[2] << 4) | selected_numbers[3];
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
/**
 * @brief
 * Reads the best (lowest) guess count from EEPROM using the gatekeeper task.
 * Returns BEST_SCORE_NONE (0xFFFF) if no score has been stored yet.
 *
 * @return uint16_t  The stored best score, or BEST_SCORE_NONE.
 */
static uint16_t eeprom_read_best_score(void) {
  /* Private response queue for EEPROM reads */
  QueueHandle_t eq = xQueueCreate(1, sizeof(device_response_msg_t));
  if (eq == NULL) return BEST_SCORE_NONE;

  uint8_t lo = 0xFF, hi = 0xFF;
  system_sensors_eeprom_read(eq, EEPROM_ADDR_BEST_SCORE_LO, &lo);
  system_sensors_eeprom_read(eq, EEPROM_ADDR_BEST_SCORE_HI, &hi);
  vQueueDelete(eq);

  uint16_t score = ((uint16_t)hi << 8) | lo;
  return score;
}

/**
 * @brief
 * Writes a new best score to EEPROM using the gatekeeper task.
 *
 * @param score  The guess count to persist.
 */
static void eeprom_write_best_score(uint16_t score) {
  QueueHandle_t eq = xQueueCreate(1, sizeof(device_response_msg_t));
  if (eq == NULL) return;

  system_sensors_eeprom_write(eq, EEPROM_ADDR_BEST_SCORE_LO, (uint8_t)(score & 0xFF));
  system_sensors_eeprom_write(eq, EEPROM_ADDR_BEST_SCORE_HI, (uint8_t)((score >> 8) & 0xFF));
  vQueueDelete(eq);
}

/**
 * @brief
 * Clears (erases) the best score stored in EEPROM by writing the
 * sentinel value BEST_SCORE_NONE.
 */
static void eeprom_clear_best_score(void) {
  eeprom_write_best_score(BEST_SCORE_NONE);
}

/**
 * @brief
 * Displays the end-of-game screen on the LCD.
 * Shows win/lose/tie result, guess counts, and the best score.
 * Waits for SW1 to continue (and handles theme changes).
 *
 * @param result_msg   "You Win!", "You Lose!", or "It's a tie!"
 * @param my_guesses   Number of guesses this player used
 * @param best_score   All-time best score stored in EEPROM (or BEST_SCORE_NONE)
 */
static void show_end_screen(const char *result_msg, int my_guesses,
                            uint16_t best_score) {
  lcd_msg_request_t lcd_request;
  lcd_request.return_queue = NULL;

  /* Clear the screen */
  lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Line 0: result message */
  print_top_lcd(result_msg);

  /* Line 1: your guess count */
  char line1[32];
  snprintf(line1, 32, "Your guesses: %d     ", my_guesses);
  lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
  snprintf(lcd_request.msg.payload.message, 32, "%s", line1);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Line 2: best score */
  char line2[32];
  if (best_score == BEST_SCORE_NONE) {
    snprintf(line2, 32, "Best score: none     ");
  } else {
    snprintf(line2, 32, "Best score: %d       ", best_score);
  }
  lcd_request.msg.command = LCD_CMD_PRINT_SW2_COUNT;
  snprintf(lcd_request.msg.payload.message, 32, "%s", line2);
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

  /* Line 3: prompt to start new game */
  lcd_request.msg.command = LCD_CMD_PRINT_LINE3;
  snprintf(lcd_request.msg.payload.message, 32, "SW1: New Game         ");
  xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
}

/**
 * @brief
 * This task runs the main game loop. It wraps the entire game flow
 * inside an outer new-game loop so a fresh round can start without
 * a power cycle.
 *
 * Flow per round:
 *   1. Read best score from EEPROM and display it on LCD.
 *   2. Allow SW3 to clear the high score (only before gameplay begins).
 *   3. Discover the other board.
 *   4. Select secret cypher, exchange with opponent.
 *   5. Turn-based guessing until someone cracks the code.
 *   6. Determine win/lose/tie; update EEPROM if new best.
 *   7. Display results; wait for SW1 to start a new game.
 *   8. Synchronize with other board and loop back to step 1.
 */
void task_system_control(void *arg) {
  (void)arg;
  EventBits_t events;
  uint16_t sequence_num = 0;
  bool rslt;

  /* ===== Outer new-game loop ===== */
  while (1) {

    /* ---------- Reset per-round state ---------- */
    Cypher_Chosen = false;
    guess_number = 1;
    cypher = 0;
    Sent_Code = (uint16_t)-1;

    /* Clear any stale event bits from a previous round */
    xEventGroupClearBits(ECE353_RTOS_Events,
                         ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED |
                         ECE353_RTOS_EVENTS_IPC_ACK_RECEIVED |
                         ECE353_RTOS_EVENTS_IPC_NEW_GAME |
                         ECE353_BUTTON_1_PRESSED |
                         ECE353_BUTTON_2_PRESSED |
                         ECE353_BUTTON_3_PRESSED);

    /* ---------- 1. Read and display best score ---------- */
    uint16_t best_score = eeprom_read_best_score();

    lcd_msg_request_t lcd_request;
    lcd_request.return_queue = NULL;

    /* Clear screen */
    lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    /* Show best score on LCD during initialization */
    if (best_score == BEST_SCORE_NONE) {
      print_top_lcd("Best: none  SW3:Reset");
    } else {
      char best_msg[32];
      snprintf(best_msg, 32, "Best: %d  SW3:Reset  ", best_score);
      print_top_lcd(best_msg);
    }

    task_console_printf("EEPROM best score: %s\n\r",
                        (best_score == BEST_SCORE_NONE) ? "none"
                        : "exists");

    /* ---------- 2. SW3 can clear the high score before game starts ----------
     * Wait for either SW1 (continue to discovery) or SW3 (reset score).
     * Also handle theme changes. */
    lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
    snprintf(lcd_request.msg.payload.message, 32, "SW1: Start Game      ");
    lcd_request.return_queue = NULL;
    xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

    while (1) {
      events = xEventGroupWaitBits(
          ECE353_RTOS_Events,
          ECE353_BUTTON_1_PRESSED | ECE353_BUTTON_3_PRESSED |
              ECE353_THEME_CHANGED,
          pdTRUE, pdFALSE, portMAX_DELAY);

      if (events & ECE353_THEME_CHANGED) {
        /* Repaint the init screen */
        lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
        lcd_request.return_queue = NULL;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);

        if (best_score == BEST_SCORE_NONE) {
          print_top_lcd("Best: none  SW3:Reset");
        } else {
          char best_msg[32];
          snprintf(best_msg, 32, "Best: %d  SW3:Reset  ", best_score);
          print_top_lcd(best_msg);
        }
        lcd_request.msg.command = LCD_CMD_PRINT_SW1_COUNT;
        snprintf(lcd_request.msg.payload.message, 32, "SW1: Start Game      ");
        lcd_request.return_queue = NULL;
        xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
        continue;
      }

      if (events & ECE353_BUTTON_3_PRESSED) {
        /* Reset the stored high score in EEPROM */
        eeprom_clear_best_score();
        best_score = BEST_SCORE_NONE;
        task_console_printf("High score cleared by SW3!\n\r");

        print_top_lcd("Score cleared!       ");
        vTaskDelay(pdMS_TO_TICKS(1000));
        print_top_lcd("Best: none  SW3:Reset");
        continue; /* Keep waiting for SW1 */
      }

      if (events & ECE353_BUTTON_1_PRESSED) {
        break; /* Proceed to discovery */
      }
    }

    /* ---------- 3. Board discovery ---------- */
    print_top_lcd("Searching for board..");
    discover_board(&sequence_num);
    sequence_num++;

    /* ---------- 4. Cypher selection and exchange ---------- */
    uint16_t code = number_select();
    Cypher_Chosen = true;
    task_console_printf("Selected Cypher: 0x%X\n\r", code);

    /* Determine player order: the first to finish selecting is P1 */
    bool P1 = !(xEventGroupGetBits(ECE353_RTOS_Events) &
                ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED);

    /* Send the cypher to the other board */
    ipc_send_number(sequence_num, code);
    rslt = ipc_wait_for_ack(1000);
    if (rslt) {
      printf("Cypher message sent and ACK received!\n\r");
      sequence_num++;
    } else {
      printf("Cypher message sent but no ACK received!\n\r");
    }

    /* Wait for the other player's cypher */
    while (1) {
      events = xEventGroupWaitBits(
          ECE353_RTOS_Events,
          ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
          pdTRUE, pdFALSE, portMAX_DELAY);
      if (events & ECE353_THEME_CHANGED) {
        redraw_wait_screen("Cypher Sent!   ", code);
        continue;
      }
      break;
    }

    cypher = Sent_Code;
    task_console_printf("Other User's Cypher 0x%X!\n\r", cypher);
    task_console_printf("Starting Game! You are Player %d\n\r", P1 ? 1 : 2);

    /* ---------- 5. Main game loop ---------- */
    uint16_t guess = 0;
    bool i_won = false;
    bool they_won = false;

    if (P1) {
      /* Player 1 guesses first each round */
      do {
        /* Take a guess */
        guess = number_select();
        guess_number++;
        task_console_printf("You guessed: 0x%X\n\r", guess);

        /* Evaluate locally and show feedback */
        int exact_p1 = 0, wrong_pos_p1 = 0;
        evaluate_guess(guess, cypher, &exact_p1, &wrong_pos_p1);
        task_console_printf("Feedback: %d exact, %d wrong pos\n\r",
                            exact_p1, wrong_pos_p1);
        show_feedback_screen(guess, exact_p1, wrong_pos_p1);

        /* Send guess to other player */
        ipc_send_number(sequence_num, guess);
        rslt = ipc_wait_for_ack(1000);

        /* Check win conditions */
        i_won = (guess == cypher);

        /* Wait for opponent's guess */
        print_top_lcd("Waiting for guess...");
        while (1) {
          events = xEventGroupWaitBits(
              ECE353_RTOS_Events,
              ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
              pdTRUE, pdFALSE, portMAX_DELAY);
          if (events & ECE353_THEME_CHANGED) {
            redraw_wait_screen("Waiting for guess...", guess);
            continue;
          }
          break;
        }

        they_won = (Sent_Code == code);
      } while (!i_won && !they_won);

    } else {
      /* Player 2 waits first each round */
      do {
        /* Wait for opponent's guess */
        print_top_lcd("Waiting for guess...");
        while (1) {
          events = xEventGroupWaitBits(
              ECE353_RTOS_Events,
              ECE353_RTOS_EVENTS_IPC_NUM_RECEIVED | ECE353_THEME_CHANGED,
              pdTRUE, pdFALSE, portMAX_DELAY);
          if (events & ECE353_THEME_CHANGED) {
            redraw_wait_screen("Waiting for guess...",
                               (guess != 0) ? guess : code);
            continue;
          }
          break;
        }

        they_won = (Sent_Code == code);

        /* Take a guess */
        guess = number_select();
        guess_number++;
        task_console_printf("You guessed: 0x%X\n\r", guess);

        /* Evaluate locally and show feedback */
        int exact_p2 = 0, wrong_pos_p2 = 0;
        evaluate_guess(guess, cypher, &exact_p2, &wrong_pos_p2);
        task_console_printf("Feedback: %d exact, %d wrong pos\n\r",
                            exact_p2, wrong_pos_p2);
        show_feedback_screen(guess, exact_p2, wrong_pos_p2);

        /* Send guess to other player */
        ipc_send_number(sequence_num, guess);
        rslt = ipc_wait_for_ack(1000);

        i_won = (guess == cypher);
      } while (!i_won && !they_won);
    }

    /* ---------- 6. Determine result and update EEPROM ---------- */
    int my_guesses = guess_number - 1; /* guess_number was incremented after each guess */
    const char *result_msg;

    if (i_won && they_won) {
      result_msg = "It's a tie!          ";
      task_console_printf("Tie! Both guessed in the same round.\n\r");
    } else if (i_won) {
      result_msg = "You Win!             ";
      task_console_printf("You win in %d guesses!\n\r", my_guesses);
    } else {
      result_msg = "You Lose!            ";
      task_console_printf("You lose. Used %d guesses.\n\r", my_guesses);
    }

    /* Update EEPROM best score if we won (or tied) with fewer guesses */
    if (i_won) {
      if (best_score == BEST_SCORE_NONE ||
          (uint16_t)my_guesses < best_score) {
        eeprom_write_best_score((uint16_t)my_guesses);
        best_score = (uint16_t)my_guesses;
        task_console_printf("New best score: %d!\n\r", my_guesses);
      }
    }

    /* ---------- 7. Display end screen, wait for SW1 to acknowledge ---------- */
    show_end_screen(result_msg, my_guesses, best_score);

    /* Phase 1: Wait for SW1 to acknowledge the result (You Win / You Lose / Tie).
     * Ignore IPC_NEW_GAME here so the result screen stays visible. */
    while (1) {
      events = xEventGroupWaitBits(
          ECE353_RTOS_Events,
          ECE353_BUTTON_1_PRESSED | ECE353_THEME_CHANGED,
          pdTRUE, pdFALSE, portMAX_DELAY);

      if (events & ECE353_THEME_CHANGED) {
        show_end_screen(result_msg, my_guesses, best_score);
        continue;
      }

      if (events & ECE353_BUTTON_1_PRESSED) {
        break; /* Player has seen the result, proceed to new-game sync */
      }
    }

    /* Phase 2: New-game synchronization with the other board */
    /* Tell the other board we want a new game */
    ipc_send_new_game(sequence_num);
    ipc_wait_for_ack(1000);
    sequence_num++;

    /* Check if the other board already sent new-game while we were on the result screen */
    EventBits_t new_game_bits = xEventGroupWaitBits(
        ECE353_RTOS_Events,
        ECE353_RTOS_EVENTS_IPC_NEW_GAME,
        pdTRUE, pdFALSE, 0);

    if (!(new_game_bits & ECE353_RTOS_EVENTS_IPC_NEW_GAME)) {
      /* Other board hasn't sent new game yet; wait for it */
      print_top_lcd("Waiting for player..");
      while (1) {
        EventBits_t ev = xEventGroupWaitBits(
            ECE353_RTOS_Events,
            ECE353_RTOS_EVENTS_IPC_NEW_GAME | ECE353_THEME_CHANGED,
            pdTRUE, pdFALSE, portMAX_DELAY);
        if (ev & ECE353_THEME_CHANGED) {
          lcd_request.msg.command = LCD_CMD_UPDATE_THEME;
          lcd_request.return_queue = NULL;
          xQueueSend(xQueue_Request_LCD, &lcd_request, portMAX_DELAY);
          print_top_lcd("Waiting for player..");
          continue;
        }
        if (ev & ECE353_RTOS_EVENTS_IPC_NEW_GAME) break;
      }
    }

    task_console_printf("=== Starting New Game ===\n\r");
    /* Loop back to the top of the outer while(1) for a fresh round */

  } /* end outer new-game loop */
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
void task_ambient_light(void *arg) {
  (void)arg;

  /* Create a private response queue for sensor readings */
  QueueHandle_t light_response_queue =
      xQueueCreate(1, sizeof(device_response_msg_t));
  if (light_response_queue == NULL) {
    printf("Failed to create light sensor response queue!\n\r");
    vTaskSuspend(NULL);
  }

  uint16_t ambient_light = 0;

  /* Perform an initial read to set Dark_Mode before any UI is drawn */
  if (system_sensors_get_light(light_response_queue, &ambient_light)) {
    Dark_Mode = (ambient_light < LIGHT_THRESHOLD);
    task_console_printf("Initial ambient light: %u -> %s mode\n\r",
                        ambient_light, Dark_Mode ? "DARK" : "LIGHT");
  }

  /* Continuously poll the light sensor and detect changes */
  while (1) {
    /* Wait 500 ms between readings to match the sensor measurement rate */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Read the ambient light level from the sensor */
    if (!system_sensors_get_light(light_response_queue, &ambient_light)) {
      /* Sensor read failed; try again next cycle */
      continue;
    }

    /* Determine the new mode from the sensor reading */
    bool new_dark_mode = (ambient_light < LIGHT_THRESHOLD);

    /* Only update the UI when the mode actually changes */
    if (new_dark_mode != Dark_Mode) {
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
 * This function is used to initialize any rtos connection used in the
 * application.
 */
static void hw05_rtos_init(void) {

  // Create the semaphores for I2C and SPI
  I2C_Semaphore = xSemaphoreCreateBinary();
  if (I2C_Semaphore == NULL) {
    printf("Failed to create I2C semaphore!\n\r");
    for (int i = 0; i < 100000; i++) {
    }
    CY_ASSERT(0);
  }

  SPI_Semaphore = xSemaphoreCreateBinary();
  if (SPI_Semaphore == NULL) {
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
void app_init_hw(void) {
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
  if (rslt != CY_RSLT_SUCCESS) {
    printf("Button initialization failed!\n\r");
    for (int i = 0; i < 10000; i++)
      ;
    CY_ASSERT(0);
  }

  rslt = lcd_initialize();
  if (rslt != CY_RSLT_SUCCESS) {
    printf("LCD initialization failed!\n\r");
    for (int i = 0; i < 100000; i++) {
    }
    CY_ASSERT(0);
  }

  /* Initialize the CS pin for the EEPROM */
  cyhal_gpio_init(PIN_SPI_EEPROM_CS, CYHAL_GPIO_DIR_OUTPUT,
                  CYHAL_GPIO_DRIVE_STRONG, 1);

  /* Initialize the interrupt pin for the capacitive touch sensor */
  cyhal_gpio_init(PIN_CAP_TOUCH_INT, CYHAL_GPIO_DIR_INPUT,
                  CYHAL_GPIO_DRIVE_NONE, 0);
}

/*****************************************************************************/
/* Application Code                                                          */
/*****************************************************************************/
/**
 * @brief
 * This function implements the behavioral requirements for the ICE
 */
void app_main(void) {
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
  if (!task_eeprom_resources_init(&SPI_Semaphore, SPI_Monarch_Obj,
                                  PIN_SPI_EEPROM_CS)) {
    printf("EEPROM Task resource initialization failed!\n\r");
    for (int i = 0; i < 100000; i++) {
    }
    CY_ASSERT(0);
  }

  // Initialize the resources for the capacitive touch task
  if (!task_cap_touch_resources_init(Queue_Request_Cap_Touch, I2C_Semaphore,
                                     I2C_Monarch_Obj, PIN_CAP_TOUCH_INT)) {
    printf("Cap Touch Task resource initialization failed!\n\r");
    for (int i = 0; i < 100000; i++) {
    }
    CY_ASSERT(0);
  }

  // Initialize buttons
  if (!task_button_init()) {
    printf("Button initialization failed!\n\r");
    for (int i = 0; i < 10000; i++)
      ;
    CY_ASSERT(0);
  }

  // Initialize IPC
  if (!task_ipc_init()) {
    printf("IPC initialization failed!\n\r");
    for (int i = 0; i < 10000; i++)
      ;
    CY_ASSERT(0);
  }

  // Initialize lcd task
  if (!task_lcd_resources_init(xQueue_Request_LCD)) {
    printf("LCD Task initialization failed!\n\r");
    for (int i = 0; i < 10000; i++)
      ;
    CY_ASSERT(0);
  }

  // Initialize the light sensor task resources
  if (!task_light_sensor_resources_init(&I2C_Semaphore, I2C_Monarch_Obj)) {
    printf("Light Sensor Task initialization failed!\n\r");
    for (int i = 0; i < 10000; i++)
      ;
    CY_ASSERT(0);
  }

  // Create the ambient light monitoring task (runs continuously)
  xTaskCreate(task_ambient_light, "Ambient Light", configMINIMAL_STACK_SIZE * 2,
              NULL, tskIDLE_PRIORITY + 1, NULL);

  // Create the System Control Task
  xTaskCreate(task_system_control, "System Control",
              configMINIMAL_STACK_SIZE * 5, NULL, 2, NULL);

  /* Start the scheduler*/
  vTaskStartScheduler();

  /* Will never reach this loop once the scheduler starts */
  while (1) {
  }
}
#endif