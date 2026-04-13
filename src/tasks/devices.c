/**
 * @file devices.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-10-27
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "devices.h"

#if defined(ECE353_FREERTOS)
#include "task_imu.h"
#include "task_temp_sensor.h"
#include "task_light_sensor.h"
#include "task_io_expander.h"
#include "task_eeprom.h"
#include "task_console.h"
#include <stdlib.h>

#endif /* ECE353_FREERTOS */

bool parse_cli_data(char *data, device_request_msg_t *request) {

    // Check if the command starts with "EEPROM W"
    if (strncmp(data, "EEPROM W", 8) == 0) {
        char *ptr = data + 8;
        char *next_ptr;
        
        unsigned int parsed_addr = strtoul(ptr, &next_ptr, 16);
        if (ptr == next_ptr) {
            printf("Failed to parse EEPROM W command. Address missing.\n");
            return false;
        }
        ptr = next_ptr;
        
        unsigned int parsed_value = strtoul(ptr, &next_ptr, 16);
        if (ptr == next_ptr) {
            printf("Failed to parse EEPROM W command. Value missing.\n");
            return false;
        }

        // Load the parsed values into the request structure
        request->address = (uint16_t)parsed_addr;
        request->value = (uint8_t)parsed_value;
        request->device = DEVICE_EEPROM;
        request->operation = DEVICE_OP_WRITE;
        return true;
    }
    // Check if the command starts with "EEPROM R"
    else if (strncmp(data, "EEPROM R", 8) == 0) {
        char *ptr = data + 8;
        char *next_ptr;
        
        unsigned int parsed_addr = strtoul(ptr, &next_ptr, 16);
        if (ptr == next_ptr) {
            printf("Failed to parse EEPROM R command. Address missing.\n");
            return false;
        }

        // Load the parsed value into the request structure
        request->address = (uint16_t)parsed_addr;
        request->device = DEVICE_EEPROM;
        request->operation = DEVICE_OP_READ;
        return true;
    }
    else if (strncmp(data, "CAP_TOUCH", 9) == 0)
    {
        request->device = DEVICE_CAP_TOUCH;
        request->operation = DEVICE_OP_READ;
        return true;
    }

    // If the command is not recognized, return false
    printf("Unknown command: '%s'\n", data);
    return false;
}
