/**
 * @file eeprom.c
 * @author Joe Krachey (jkrachey@wisc.edu)
 * @brief 
 * @version 0.1
 * @date 2023-10-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "eeprom.h"
#include "cyhal_hw_types.h"
#include <sys/types.h>


/** Determine if the EEPROM is busy writing the last
 *  transaction to non-volatile storage
 *
 * @param
 *
 */
void eeprom_wait_for_write(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[2] = {EEPROM_CMD_RDSR, 0x00}; // Set MSB for read operation
    uint8_t rx_buffer[2];
	
	do
	{
		// Assert the CS pin (active low)
		cyhal_gpio_write(cs_pin, 0);
		
		// Transmit the register address and value
		cyhal_spi_transfer(spi_obj, tx_buffer, 2, rx_buffer, 2, 0xFF);

		// Deassert the CS pin
		cyhal_gpio_write(cs_pin, 1);

		// Delay for a short time before checking again
		cyhal_system_delay_ms(5);
	} while ((rx_buffer[1] & 0x01) == 1); // Check WIP bit
}

/** Enables Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_enable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[1] = {EEPROM_CMD_WREN}; // Set MSB for read operation
    uint8_t rx_buffer[1];

	// Assert the CS pin (active low)
	cyhal_gpio_write(cs_pin, 0);
	
	// Transmit the register address and value
	cyhal_spi_transfer(spi_obj, tx_buffer, 1, rx_buffer, 1, 0xFF);

	// Deassert the CS pin
	cyhal_gpio_write(cs_pin, 1);
}

/** Disable Writes to the EEPROM
 *
 * @param
 *
 */
void eeprom_write_disable(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin)
{
	uint8_t tx_buffer[1] = {EEPROM_CMD_WRDI}; // Set MSB for read operation
    uint8_t rx_buffer[1];

	// Assert the CS pin (active low)
	cyhal_gpio_write(cs_pin, 0);
	
	// Transmit the register address and value
	cyhal_spi_transfer(spi_obj, tx_buffer, 1, rx_buffer, 1, 0xFF);

	// Deassert the CS pin
	cyhal_gpio_write(cs_pin, 1);
}

/** Writes a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 * @param data    -- value to write into memory
 *
 */
void eeprom_write_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address, uint8_t data)
{
	uint8_t tx_buffer[4] = {EEPROM_CMD_WRITE, (address >> 8) & 0xFF, address & 0xFF, data}; // Set MSB for read operation
    uint8_t rx_buffer[4];

	// Setup for write
	eeprom_wait_for_write(spi_obj, cs_pin);
	eeprom_write_enable(spi_obj, cs_pin);

	// Assert the CS pin (active low)
	cyhal_gpio_write(cs_pin, 0);
	
	// Transmit the register address and value
	cyhal_spi_transfer(spi_obj, tx_buffer, 4, rx_buffer, 4, 0xFF);

	// Deassert the CS pin
	cyhal_gpio_write(cs_pin, 1);

	// Wait for the write to complete and disable writes to the EEPROM
	eeprom_write_disable(spi_obj, cs_pin);
}

/** Reads a single byte to the specified address
 *
 * @param address -- 16 bit address in the EEPROM
 *
 */
uint8_t eeprom_read_byte(cyhal_spi_t *spi_obj, cyhal_gpio_t cs_pin, uint16_t address)
{	
	uint8_t tx_buffer[4] = {EEPROM_CMD_READ, (address >> 8) & 0xFF, address & 0xFF, 0x00}; // Set MSB for read operation
    uint8_t rx_buffer[4];

	// Setup for read
	eeprom_wait_for_write(spi_obj, cs_pin);

	// Assert the CS pin (active low)
	cyhal_gpio_write(cs_pin, 0);
	
	// Transmit the register address and value
	cyhal_spi_transfer(spi_obj, tx_buffer, 4, rx_buffer, 4, 0xFF);

	// Deassert the CS pin
	cyhal_gpio_write(cs_pin, 1);

	return rx_buffer[3];
}