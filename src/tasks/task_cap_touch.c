/**
 * @file task_cap_touch.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2026-01-07
 * 
 * @copyright Copyright (c) 2026
 * 
 */
 #include "task_cap_touch.h"

#if defined(ECE353_FREERTOS)
#include "rtos_events.h"
#include "main.h"

/*****************************************************************************/
/* Global Variables                                                          */
/*****************************************************************************/
QueueHandle_t        Queue_Request_Cap_Touch = NULL;
static SemaphoreHandle_t    I2C_Semaphore = NULL;
static cyhal_i2c_t         *I2C_Obj = NULL;
static cyhal_gpio_t        Cap_Touch_Int_Pin = NC;


void task_cap_touch(void *param)
{
    (void)param; // Unused parameter

    device_request_msg_t request_packet;
    device_response_msg_t response_packet;
    cy_rslt_t rslt = CY_RSLT_SUCCESS;

    // Wait for the FT6X06 to complete its internal boot sequence.
    // Without this delay, a manual MCU reset causes the sensor to ACK
    // but return 0x00 because its registers aren't populated yet.
    vTaskDelay(pdMS_TO_TICKS(200));

    // Grab I2C semaphore
    xSemaphoreTake(I2C_Semaphore, portMAX_DELAY);

    // Check I2C
    uint8_t id = cap_sensor_id(I2C_Obj);
    if (id != 0x11)
    {
        task_console_printf("\nCapacitive Touch Sensor ID Incorrect: 0x%x\r\n", id);
        vTaskSuspend(NULL);
    }
    else
    {
        task_console_printf("\nCapacitive Touch Sensor ID Verified: 0x%X\r\n", id);
    }

    // Configure GMODE register (0xA4) to polling mode (0x01).
    // In the default trigger mode, INT only pulses once at initial touch contact
    // and goes back high even while the finger is held — causing missed reads.
    // Polling mode keeps INT asserted (low) continuously while any finger is on
    // the sensor, making detection reliable regardless of timing.
    i2c_write_u8(I2C_Obj, FT6X06_I2C_ADDR, FT6X06_GMODE_R, 0x01);

    // Release I2C semaphore
    xSemaphoreGive(I2C_Semaphore);

    while(1)
    {
        // Wait for a request from the console task
        xQueueReceive(Queue_Request_Cap_Touch, &request_packet, portMAX_DELAY);

        // Grab the I2C semaphore to ensure exclusive access to the I2C bus
        xSemaphoreTake(I2C_Semaphore, portMAX_DELAY);

        // Read the state of the capacitive touch sensor and return the state to the console task
        if (!read_cap_touch_sensor(I2C_Obj, Cap_Touch_Int_Pin, response_packet.payload.cap_touch))
        {
            xSemaphoreGive(I2C_Semaphore);
            response_packet.status = DEVICE_OPERATION_STATUS_READ_FAILURE;
            xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
            continue;
        }

        // Release the I2C semaphore
        xSemaphoreGive(I2C_Semaphore);

        // Return the response to the caller
        response_packet.device = DEVICE_CAP_TOUCH;
        response_packet.status = DEVICE_OPERATION_STATUS_READ_SUCCESS;

        // Send the response back to the console task
        xQueueSend(request_packet.response_queue, &response_packet, portMAX_DELAY);
    }
}


bool task_cap_touch_resources_init(
    QueueHandle_t queue_request, 
    SemaphoreHandle_t i2c_semaphore, 
    cyhal_i2c_t *i2c_obj, 
    cyhal_gpio_t pin_cap_touch_int
)
{
    if(queue_request == NULL || i2c_semaphore == NULL || i2c_obj == NULL || pin_cap_touch_int == NC)
    {
        return false;
    }   

    /* Save the resources */
    Queue_Request_Cap_Touch = queue_request;
    I2C_Semaphore = i2c_semaphore;
    I2C_Obj = i2c_obj;
    Cap_Touch_Int_Pin = pin_cap_touch_int;

    // Create the task that will control the status LED */
    if(xTaskCreate(
        task_cap_touch,
        "Task Cap Touch",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL) != pdPASS)
    {
        return false;
    }
    return true;
}
#endif /* ECE353_FREERTOS */