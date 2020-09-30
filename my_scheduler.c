/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_MAC.h>
#include <my_UART.h>
#include "my_scheduler.h"
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

static uint16_t slotCount;
extern uint16_t neighbourSyncSlots[255];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

void initScheduler() {
	// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
	slotCount = 0;
}

int scheduler() {
	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) {
		if (slotCount == neighbourSyncSlots[var]) {
			MACState = MAC_LISTEN;
		}
	}

	if (slotCount % 100 == 0) {
		MACState = MAC_LISTEN;
	}

	if (slotCount % 10 == 0) {
		MACStateMachine();
	} else if (slotCount == _txSlot) {
		MACState = MAC_RTS;
	}

	return true;
}

uint16_t getSlotCount() {
	return slotCount;
}

void setSlotCount(uint16_t newSlotCount) {
	slotCount = newSlotCount;
}

void incrementSlotCount() {
	slotCount++;
}
