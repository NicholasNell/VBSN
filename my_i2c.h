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

#endif /* MY_I2C_H_ */
