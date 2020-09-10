/*
 * my_spi.c
 *
 *  Created on: 19 Feb 2020
 *      Author: nicholas
 */
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "my_spi.h"

void spi_open(void) {

	uint32_t smclkFreq = CS_getSMCLK();

	eUSCI_SPI_MasterConfig spiMasterConfig = {
	EUSCI_B_SPI_CLOCKSOURCE_SMCLK, smclkFreq, 1500000,
	EUSCI_B_SPI_MSB_FIRST,
	EUSCI_B_SPI_PHASE_DATA_CHANGED_ONFIRST_CAPTURED_ON_NEXT,
	EUSCI_B_SPI_CLOCKPOLARITY_INACTIVITY_HIGH,
	EUSCI_SPI_3PIN }; //EUSCI_SPI_4PIN_UCxSTE_ACTIVE_HIGH
//    Setting up SPI mode and opening
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P1,
	GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7, GPIO_PRIMARY_MODULE_FUNCTION);

//    Chip select (NSS) Port 5 Pin 2 set High
	GPIO_setOutputHighOnPin(GPIO_PORT_P5, GPIO_PIN2);
	GPIO_setAsOutputPin(GPIO_PORT_P5, GPIO_PIN2);

	SPI_initMaster(EUSCI_B0_BASE, &spiMasterConfig);

	SPI_enableModule(EUSCI_B0_BASE);

}

void spi_close(void) {
	// Disable SPI module
	SPI_disableModule(EUSCI_B0_BASE);
}

void spiWrite_RFM(uint16_t addr, uint8_t val) {
	RFM95_NSS_LOW;
	uint8_t address = addr | RFM_SPI_WRITE_MASK; //  read: mask 0x80 for write access; mask 0x7F for read access
	spi_read_write(address);
	val = spi_read_write(val);
	RFM95_NSS_HIGH;
//	spiRead_RFM(0x01);

}

uint8_t spiRead_RFM(uint16_t addr) {
	uint8_t val;
	RFM95_NSS_LOW;  // To start SPI coms to rmf95 nss must be pulled low
	uint8_t value_to_send = addr & ~RFM_SPI_WRITE_MASK; //  read: mask 0x80 for write access; mask 0x7F
	val = spi_read_write(value_to_send);
	val = spi_read_write(0);
	RFM95_NSS_HIGH; // To stop SPI coms to rmf95 nss must be pulled high
	return val;
}

int8_t user_spi_read_bme280(uint8_t reg_addr, uint8_t *reg_data, uint32_t len,
		void *intf_ptr) {
	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	/*
	 * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
	 * to be set low to activate the relevant device on the SPI bus
	 */

	/*
	 * Data on the bus should be like
	 * |----------------+---------------------+-------------|
	 * | MOSI           | MISO                | Chip Select |
	 * |----------------+---------------------|-------------|
	 * | (don't care)   | (don't care)        | HIGH        |
	 * | (reg_addr)     | (don't care)        | LOW         |
	 * | (don't care)   | (reg_data[0])       | LOW         |
	 * | (....)         | (....)              | LOW         |
	 * | (don't care)   | (reg_data[len - 1]) | LOW         |
	 * | (don't care)   | (don't care)        | HIGH        |
	 * |----------------+---------------------|-------------|
	 */
	uint8_t val;
	BME280_CS_LOW;
	int i = 0;
	spi_read_write(reg_addr | 0x80);

	for (i = 0; i < len; i++) {
		reg_data[i] = spi_read_write(0);
	}
	BME280_CS_HIGH;
	return rslt;
}

int8_t user_spi_write_bme280(uint8_t reg_addr, const uint8_t *reg_data,
		uint32_t len, void *intf_ptr) {
	int8_t rslt = 0; /* Return 0 for Success, non-zero for failure */

	/*
	 * The parameter intf_ptr can be used as a variable to select which Chip Select pin has
	 * to be set low to activate the relevant device on the SPI bus
	 */

	/*
	 * Data on the bus should be like
	 * |----------------+---------------------+-------------|
	 * | MOSI           | MISO                | Chip Select |
	 * |----------------+---------------------|-------------|
	 * | (don't care)   | (don't care)        | HIGH        |
	 * | (reg_addr)     | (don't care)        | LOW         |
	 * | (don't care)   | (reg_data[0])       | LOW         |
	 * | (....)         | (....)              | LOW         |
	 * | (don't care)   | (reg_data[len - 1]) | LOW         |
	 * | (don't care)   | (don't care)        | HIGH        |
	 * |----------------+---------------------|-------------|
	 */
	uint8_t val;
	BME280_CS_LOW;
	int i = 0;
	spi_read_write(reg_addr & ~0x80);
	for (i = 0; i < len; i++) {
		spi_read_write(reg_data[i]);
	}
	BME280_CS_HIGH;
	return rslt;
}

int spi_read_write(uint8_t pBuff) {
	int RXData;
	while (!(SPI_getInterruptStatus(EUSCI_B0_BASE, EUSCI_SPI_TRANSMIT_INTERRUPT)))
		;
	/* Transmitting data to slave */
	SPI_transmitData(EUSCI_B0_BASE, pBuff);
	while (!(SPI_getInterruptStatus(EUSCI_B0_BASE,
	EUSCI_B_SPI_TRANSMIT_INTERRUPT)))
		;
	RXData = SPI_receiveData(EUSCI_B0_BASE);

	return RXData;
}

