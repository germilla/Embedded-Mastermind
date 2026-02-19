#include "joystick.h"
#include "cy_result.h"
#include "cyhal_adc.h"

/* Static Global Variables */
static cyhal_adc_t joystick_adc_obj;
static cyhal_adc_channel_t joystick_adc_chan_x_obj;
static cyhal_adc_channel_t joystick_adc_chan_y_obj;
const cyhal_adc_channel_config_t channel_config = { .enable_averaging = false, .min_acquisition_ns = 220, .enabled = true };

const cyhal_adc_config_t joystick_config = {
    .continuous_scanning = true, 
    .resolution = 12, 
    .average_count = 1, 
    .average_mode_flags = 0,
    .ext_vref_mv = 0,
    .vneg = CYHAL_ADC_VNEG_VREF,
    .vref = CYHAL_ADC_REF_VDDA_DIV_2,
    .ext_vref = NC,
    .is_bypassed = false,
    .bypass_pin = NC
};

/* Public API */

/** Initialize the ADC channels connected to the Joystick
 *
 * @param - None
 */
cy_rslt_t joystick_init(void)
{
    cy_rslt_t rslt;

    /* Initialize ADC.*/
    rslt = cyhal_adc_init(&joystick_adc_obj, PIN_ANALOG_JOY_X, NULL);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Set the Reference Voltage to be VDDA */
    rslt = cyhal_adc_configure(&joystick_adc_obj, &joystick_config);
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }


    /* Initialize X direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_x_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_X, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );
    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

    /* Initialize Y direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_y_obj, 
        &joystick_adc_obj, 
        PIN_ANALOG_JOY_Y, 
        CYHAL_ADC_VNEG, 
        &channel_config
    );

    if(rslt != CY_RSLT_SUCCESS)
    {
        return rslt; // If the initialization fails, return the error code
    }

}

/** Read X direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_x(void)
{
    return cyhal_adc_read_u16(&joystick_adc_chan_x_obj);

}

/** Read Y direction of Joystick 
 *
 * @param - None
 */
uint16_t  joystick_read_y(void)
{
    return cyhal_adc_read_u16(&joystick_adc_chan_y_obj);

}


/**
 * @brief 
 * Returns the current position of the joystick 
 * @return joystick_position_t 
 */
joystick_position_t joystick_get_pos(void)
{
    uint16_t x_val;
    uint16_t y_val;
    joystick_position_t position = JOYSTICK_POS_CENTER;
    x_val = joystick_read_x();
    y_val = joystick_read_y();

    if (x_val < JOYSTICK_THRESH_X_RIGHT && y_val < JOYSTICK_THRESH_Y_DOWN)
        position = JOYSTICK_POS_LOWER_RIGHT;
    else if (x_val < JOYSTICK_THRESH_X_RIGHT && y_val > JOYSTICK_THRESH_Y_UP)
        position = JOYSTICK_POS_UPPER_RIGHT;
    else if (x_val > JOYSTICK_THRESH_X_LEFT && y_val < JOYSTICK_THRESH_Y_DOWN)
        position = JOYSTICK_POS_LOWER_LEFT;
    else if (x_val > JOYSTICK_THRESH_X_LEFT && y_val > JOYSTICK_THRESH_Y_UP)
        position = JOYSTICK_POS_UPPER_LEFT;
    else if (x_val < JOYSTICK_THRESH_X_RIGHT)
        position = JOYSTICK_POS_RIGHT;
    else if (x_val > JOYSTICK_THRESH_X_LEFT)
        position = JOYSTICK_POS_LEFT;
    else if (y_val < JOYSTICK_THRESH_Y_DOWN)
        position = JOYSTICK_POS_DOWN;
    else if (y_val > JOYSTICK_THRESH_Y_UP)
        position = JOYSTICK_POS_UP;
    
    
    return position;
}
