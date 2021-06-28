/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_flash.h>
#include <my_gpio.h>
#include <my_GSM.h>
#include <my_MAC.h>
#include <my_RFM9x.h>
#include <my_scheduler.h>
#include <my_UART.h>
#include <myNet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>
#include <utilities.h>

static uint16_t slotCount;

bool get_sync(uint8_t per) {

	int ran = rand();
	int compVal = (100 - per) * RAND_MAX / 100;
	if (ran > compVal) {
		return true;
	} else {
		return false;
	}
}

uint16_t get_slot_count() {
	return slotCount;
}

void set_slot_count(uint16_t newSlotCount) {
	slotCount = newSlotCount;
}

void increment_slot_count() {
	slotCount++;
}
