/*
 * my_i2c.c
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#ifndef MY_I2C_C_
#define MY_I2C_C_

#include "my_i2c.h"

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Defines */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

/* I2C Master Configuration Parameter */
const eUSCI_I2C_MasterConfig i2cConfig = {
EUSCI_B_I2C_CLOCKSOURCE_SMCLK,          // SMCLK Clock Source
		1500000,                                // SMCLK = 3MHz
		EUSCI_B_I2C_SET_DATA_RATE_100KBPS,      // Desired I2C Clock of 100khz
		0,                                      // No byte counter threshold
		EUSCI_B_I2C_NO_AUTO_STOP                // No Autostop
		};

void i2cInit() {
	/* Select Port 6 for I2C - Set Pin 4, 5 to input Primary Module Function,
	 *   (UCB1SIMO/UCB1SDA, UCB1SOMI/UCB1SCL).
	 */
	P6->SEL0 |= BIT4;
	P6->SEL1 &= ~BIT4;
	P6->SEL0 |= BIT5;
	P6->SEL1 &= ~BIT5;

	/* Initializing I2C Master to SMCLK at 100khz with no autostop */
	I2C_initMaster(EUSCI_B1_BASE, &i2cConfig);

	/* Specify slave address */
	I2C_setSlaveAddress(EUSCI_B1_BASE, SLAVE_ADDRESS_LIGHT_SENSOR);

	I2C_setMode(EUSCI_B1_BASE, EUSCI_B_I2C_TRANSMIT_MODE);
	/* Enable I2C Module to start operations */
	I2C_enableModule(EUSCI_B1_BASE);
//	Interrupt_enableInterrupt(INT_EUSCIB1);
}

#endif /* MY_I2C_C_ */
