/*
 * MAX44009.c
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#include "MAX44009.h"
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <math.h>
#include "my_timer.h"

static float lux;

bool initMAX() {
	bool retval;
	MAX44009_ON
	Delayms(800);
	I2C_setSlaveAddress(EUSCI_B1_BASE, MAX44009_ADDR); // Make sure correct i2c slave selected
	I2C_setMode(EUSCI_B1_BASE, EUSCI_B_I2C_TRANSMIT_MODE); // set to transmit mode for configuration
	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
		;
	retval = I2C_masterSendMultiByteStartWithTimeout(EUSCI_B1_BASE,
	MAX44009_REG_CONFIG, 500); // first byte is reg to be configured
	retval = I2C_masterSendMultiByteNextWithTimeout(EUSCI_B1_BASE, 0x00, 500); // Set in non continuous mode 800ms integration time
	retval = I2C_masterSendMultiByteStopWithTimeout(EUSCI_B1_BASE, 500);
//	while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG0))
//		;

	return retval;
}

void getLight() {
	uint8_t RXData[2];
	I2C_masterSendSingleByte(EUSCI_B1_BASE, MAX44009_LUX_HIGH_BYTE);
	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
		;

	RXData[0] = I2C_masterReceiveSingleByte(EUSCI_B1_BASE);
	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
		;
	I2C_masterSendSingleByte(EUSCI_B1_BASE, MAX44009_LUX_LOW_BYTE);
	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
		;
	RXData[1] = I2C_masterReceiveSingleByte(EUSCI_B1_BASE);
	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
		;

	// Convert the data to lux
	int exponent = (RXData[0] & 0xF0) >> 4;
	int mantissa = ((RXData[0] & 0x0F) << 4) | (RXData[1] & 0x0F);
	float luminance = pow(2, exponent) * mantissa * 0.045;
	lux = luminance;
}

float getLux() {
	return lux;
}
