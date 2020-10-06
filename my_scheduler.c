/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_MAC.h>
#include "my_scheduler.h"

static uint16_t slotCount;

static uint16_t collectdataSlot;

extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

extern bool hasData;

bool schedChange = false;

void initScheduler() {
	// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
	collectdataSlot = _txSlot - COLLECT_DATA_SLOT_REL;
	slotCount = 1;
}

int scheduler() {
	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) {
		if (slotCount == neighbourTable[var].neighbourTxSlot) {
			MACState = MAC_LISTEN;
			break;
		}
	}

	if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
		MACState = MAC_LISTEN;
	}

	if (slotCount == _txSlot && hasData) {
		MACState = MAC_RTS;
	}

	if (schedChange && (slotCount % GLOBAL_RX == 0)) {
		MACState = MAC_SYNC_BROADCAST;
	}

	if (slotCount == collectdataSlot) {
		helper_collectSensorData();
		hasData = true;
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
