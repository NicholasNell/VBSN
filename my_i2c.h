/** @file my_i2c.h
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#ifndef MY_I2C_H_
#define MY_I2C_H_

#include <stdint.h>
#define SLAVE_ADDRESS_LIGHT_SENSOR       0x4A //light sensor

void i2c_init();
uint8_t i2c_send(uint8_t addr, uint8_t *buffer, uint8_t bufLen);
uint8_t i2c_tx_rx_single_bytes(uint8_t addr, uint8_t *txBuf, uint8_t *rxBuf,
		uint8_t numBytes);

#endif /* MY_I2C_H_ */
