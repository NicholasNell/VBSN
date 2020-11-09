/** @file my_i2c.h
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#ifndef MY_I2C_H_
#define MY_I2C_H_

#include <stdint.h>
#define SLAVE_ADDRESS_LIGHT_SENSOR       0x4A //light sensor

void i2cInit();
uint8_t i2cSend(uint8_t addr, uint8_t *buffer, uint8_t bufLen);
uint8_t i2cTxRxSingleBytes(uint8_t addr, uint8_t *txBuf, uint8_t *rxBuf,
		uint8_t numBytes);

#endif /* MY_I2C_H_ */
