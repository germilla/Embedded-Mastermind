/**
 * @file cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-03-24
 * 
 * @copyright Copyright (c) 2026
 * 
 */
#include "cap_touch.h"
#include "drivers.h"
#include "task_console.h"

/**
 * @brief
 * Returns the ID of the FT6X06 capacitive touch sensor
 * @return uint8_t
 */
uint8_t cap_sensor_id(cyhal_i2c_t *I2C_Obj)
{
    uint8_t value = 82;
    cy_rslt_t rslt = CY_RSLT_SUCCESS;

    rslt = i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_FOCALTECH_ID_R, &value);

    if (rslt != CY_RSLT_SUCCESS)
    {
        task_console_printf("Error reading from sensor: %d\r\n", rslt);
        return -1;
    }

    return value;
}

bool read_cap_touch_sensor(cyhal_i2c_t *I2C_Obj, cyhal_gpio_t Cap_Touch_Int_Pin, uint16_t *touch_data)
{
    cy_rslt_t rslt = CY_RSLT_SUCCESS;
    uint8_t touch_data_buffer[4];
    uint8_t td_status = 0;

    // Read TD_STATUS (0x02) to check how many touch points are currently active.
    // Bits [3:0] = number of active touch points (0, 1, or 2).
    // This avoids relying on the INT pin, which pulses periodically even in
    // "polling mode" rather than staying continuously asserted.
    rslt = i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_TD_STATUS_R, &td_status);
    if (rslt != CY_RSLT_SUCCESS)
    {
        task_console_printf("Error reading TD_STATUS: %d\r\n", rslt);
        touch_data[0] = 0;
        touch_data[1] = 0;
        return false;
    }

    // No active touch points
    if ((td_status & 0x0F) == 0)
    {
        touch_data[0] = 0;
        touch_data[1] = 0;
        return false;
    }

    // Read X and Y coordinates for touch point 1
    rslt  = i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_TOUCH1_X_HIGH_R, &touch_data_buffer[0]);
    rslt |= i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_TOUCH1_X_LOW_R,  &touch_data_buffer[1]);
    rslt |= i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_TOUCH1_Y_HIGH_R, &touch_data_buffer[2]);
    rslt |= i2c_read_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_TOUCH1_Y_LOW_R,  &touch_data_buffer[3]);

    if (rslt != CY_RSLT_SUCCESS)
    {
        task_console_printf("Error reading touch coordinates: %d\r\n", rslt);
        return false;
    }

    // Bits [3:0] of the HIGH registers are the upper 4 bits of the 12-bit coordinate.
    // X is mirrored (sensor runs right-to-left), so subtract from MAX_X to correct.
    uint16_t raw_x = ((touch_data_buffer[0] & 0x0F) << 8) | touch_data_buffer[1];
    touch_data[1] = (FT6X06_MAX_X - 1) - raw_x;
    touch_data[0] = ((touch_data_buffer[2] & 0x0F) << 8) | touch_data_buffer[3];

    // Check for dummy readings where the sensor reports too high a value
    if (touch_data[1] > FT6X06_MAX_X || touch_data[1] > FT6X06_MAX_Y)
    {
        touch_data[0] = 0;
        touch_data[1] = 0;
        return false;
    }


    return true;
}

