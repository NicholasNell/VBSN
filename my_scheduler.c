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

bool get_sync(void) {

	static uint16_t lastSync = 0;
	bool retval = false;
	int ran = (float) rand() / RAND_MAX * SYNC_PROB;
	if ((slotCount - lastSync) < 0) {
		lastSync = 0;
	} else if ((slotCount - lastSync) > ran) {
		lastSync = slotCount;
		retval = true;
	}
	return retval;
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
