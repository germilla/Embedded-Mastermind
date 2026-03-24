/**
 * @file imu.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2025-09-15
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "imu.h"
#include "cy_result.h"
#include "cyhal_hw_types.h"

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @param value 
 */
void imu_write_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t value
)
{
    // Write 2 bytes: Register address and value
    uint8_t tx_buffer[2] = {reg, value};
    uint8_t rx_buffer[2];

    // Assert the CS pin (active low)
    cyhal_gpio_write(cs_pin, 0);
    
    // Transmit the register address and value
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    // Deassert the CS pin
    cyhal_gpio_write(cs_pin, 1);
}

/**
 * @brief 
 * Read a register from the IMU
 * @param reg 
 * @return uint8_t 
 */
uint8_t imu_read_reg(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg
)
{
    // Write 2 bytes: Register address and value
    uint8_t tx_buffer[2] = {reg | 0x80, 0x00}; // Set MSB for read operation
    uint8_t rx_buffer[2];

    // Assert the CS pin (active low)
    cyhal_gpio_write(cs_pin, 0);
    
    // Transmit the register address and value
    cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

    // Deassert the CS pin
    cyhal_gpio_write(cs_pin, 1);

    return rx_buffer[1]; // Return the value read from the register
}

/**
 * @brief Read multiple registers from the IMU
 * 
 * @param reg 
 * @param buffer 
 * @param length 
 */
void imu_read_registers(
    cyhal_spi_t *spi_obj, 
    cyhal_gpio_t cs_pin, 
    uint8_t reg, 
    uint8_t *buffer, 
    uint8_t length
)
{
    uint8_t tx_buffer[length + 1];
    uint8_t rx_buffer[length + 1];
    tx_buffer[0] = reg | 0x80; // Set MSB for read operation

    // Assert the CS pin (active low)
    cyhal_gpio_write(cs_pin, 0);

    // Transmit the register address and read the values
    cyhal_spi_transfer(spi_obj, tx_buffer, length + 1, rx_buffer, length + 1, 0xFF);

    // Deassert the CS pin
    cyhal_gpio_write(cs_pin, 1);

    // Copy the read values to the provided buffer
    memcpy(buffer, &rx_buffer[1], length);

}

/**
 * @brief 
 * This function verifys the WHO_AM_I register and configures the IMU
 * to 104 Hz, ±2g, ±250 dps.
 * @param spi_obj 
 * @param cs_pin 
 * @return true 
 * @return false 
 */
bool imu_init(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
    // Wait for 15mS after power-up
    cyhal_system_delay_ms(15);

    // Verify the WHO_AM_I register
    uint8_t who_am_i = imu_read_reg(spi_obj, cs_pin, IMU_REG_WHO_AM_I);
    if (who_am_i != 0x6A)
    {
        return false;
    }

    // Reset the IMU
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C, 0x01); // CTRL3_C register

    // Wait for the reset to complete
    do
    {
        cyhal_system_delay_ms(1);
    } while (imu_read_reg(spi_obj, cs_pin, IMU_REG_CTRL3_C) & 0x01);

    // Configure the IMU:  104 Hz, ±2g, ±250 dps
    imu_write_reg(spi_obj, cs_pin, IMU_REG_CTRL1_XL, ODR_104HZ | FS_XL_2G);

    return true;
}