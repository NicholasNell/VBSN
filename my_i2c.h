/*
 * my_i2c.h
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#ifndef MY_I2C_H_
#define MY_I2C_H_

#include <stdint.h>

void i2cInit();
void i2cSend(uint8_t *TXDdata);

#endif /* MY_I2C_H_ */
