/*
 * MAX44009.h
 *
 *  Created on: 08 Sep 2020
 *      Author: njnel
 */

#ifndef MAX44009_H_
#define MAX44009_H_

// Register Defines
#define MAX44009_ADDR 0x4A
#define MAX44009_INT_STATUS 0x00
#define MAX44009_INT_ENABLE 0x01
#define MAX44009_REG_CONFIG 0x02
#define MAX44009_LUX_HIGH_BYTE 0x03
#define MAX44009_LUX_LOW_BYTE 0x04
#define MAX44009_UP_THRESH_HIGH_BYTE 0x05
#define MAX44009_LOW_THRESH_HIGH_BYTE 0x06
#define MAX44009_THRESH_TIM 0x07

// Register Masks
#define MAX44009_INT_MASK 0x01
#define MAX44009_INT_ENABLE_MASK 0x01
#define MAX44009_CONFIG_CONT_MODE_MASK 0x40

void initMAX();
float getLight();

#endif /* MAX44009_H_ */
