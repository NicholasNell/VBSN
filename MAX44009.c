/*
 * MAX44009.c
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#include <math.h>
#include <my_i2c.h>
#include <my_timer.h>
#include <MAX44009.h>
#include <stdint.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/i2c.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>
#include <ti/devices/msp432p4xx/inc/msp432p401r.h>
#include <utilities.h>

static float lux;

bool init_max( ) {
	bool retval;
	MAX44009_ON
	delay_ms(800);

	uint8_t sendBuf[] = { MAX44009_REG_CONFIG, 0x00 };
	retval = (bool) i2c_send(MAX44009_ADDR, sendBuf, strlen((char*) sendBuf));
//	I2C_setSlaveAddress(EUSCI_B1_BASE, MAX44009_ADDR); // Make sure correct i2c slave selected
//	I2C_setMode(EUSCI_B1_BASE, EUSCI_B_I2C_TRANSMIT_MODE); // set to transmit mode for configuration
//	while (MAP_I2C_masterIsStopSent(EUSCI_B1_BASE))
//		;
//	retval = I2C_masterSendMultiByteStartWithTimeout(EUSCI_B1_BASE,
//	MAX44009_REG_CONFIG, 500); // first byte is reg to be configured
//	retval = I2C_masterSendMultiByteNextWithTimeout(EUSCI_B1_BASE, 0x00, 500); // Set in non continuous mode 800ms integration time
//	retval = I2C_masterSendMultiByteStopWithTimeout(EUSCI_B1_BASE, 500);
////	while (!(EUSCI_B1->IFG & EUSCI_B_IFG_TXIFG0))
////		;

	return retval;
}

void get_light( ) {
	uint8_t RXData[2];
	uint8_t txBuf[] = { MAX44009_LUX_HIGH_BYTE, MAX44009_LUX_LOW_BYTE };
	i2c_tx_rx_single_bytes(MAX44009_ADDR, txBuf, RXData, 2);

	// Convert the data to lux
	volatile int exponent = (RXData[0] & 0xF0) >> 4;
	volatile int mantissa = ((RXData[0] & 0x0F) << 4) | (RXData[1] & 0x0F);
	volatile float luminance = pow(2, exponent) * mantissa * 0.045;
	lux = luminance;
}

float get_lux( ) {
	return lux;
}
