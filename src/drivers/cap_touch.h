#ifndef __CAP_TOUCH_H__
#define __CAP_TOUCH_H__

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "stdio.h"

#define FT6X06_I2C_ADDR                 0x38    
#define FT6X06_MAX_X                    240     // Display width in pixels (used to mirror X axis)
#define FT6X06_MAX_Y                    320     // Display height in pixels (used to mirror Y axis)

//*****************************************************************************
// Fill out the #defines below to configure which pins are connected to
// the I2C Bus
//*****************************************************************************
#define FT6X06_FOCALTECH_ID_R         0xA8
#define FT6X06_TD_STATUS_R            0x02
#define FT6X06_GMODE_R                0xA4   // Interrupt mode: 0x00=trigger, 0x01=polling
#define FT6X06_TOUCH1_X_HIGH_R        0x03
#define FT6X06_TOUCH1_X_LOW_R         0x04
#define FT6X06_TOUCH1_Y_HIGH_R        0x05
#define FT6X06_TOUCH1_Y_LOW_R         0x06
#define FT6X06_TOUCH1_SIZE_R          0x07
#define FT6X06_TOUCH1_MISC_R          0x08

#define FT6X06_TOUCH2_X_HIGH_R        0x09
#define FT6X06_TOUCH2_X_LOW_R         0x0A
#define FT6X06_TOUCH2_Y_HIGH_R        0x0B
#define FT6X06_TOUCH2_Y_LOW_R         0x0C
#define FT6X06_TOUCH2_SIZE_R          0x0D
#define FT6X06_TOUCH2_MISC_R          0x0E


uint8_t cap_sensor_id(cyhal_i2c_t *I2C_Obj);

bool read_cap_touch_sensor(cyhal_i2c_t *I2C_Obj, cyhal_gpio_t Cap_Touch_Int_Pin, uint16_t *touch_data);

#endif

