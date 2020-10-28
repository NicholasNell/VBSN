/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_MAC.h>
#include <my_scheduler.h>
#include <stdbool.h>

static uint16_t slotCount;

static uint16_t collectDataSlot;

extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

extern bool hasData;

bool schedChange = false;

uint8_t bracketNum = 0;
uint8_t txBracket = 0;

void initScheduler() {
	// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
	collectDataSlot = _txSlot - COLLECT_DATA_SLOT_REL;
	slotCount = 1;
}

int scheduler() {
	if (bracketNum == 4) {
		bracketNum = 0;
	}

	if (txBracket == 4) {
		txBracket = 4;
	}

	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) { // loop through all neighbours and listen if any of them are expected to transmit a message
		if (slotCount == neighbourTable[var].neighbourTxSlot) {
			MACState = MAC_LISTEN;
			break;
		}
	}

	if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
		MACState = MAC_LISTEN;
	}

	if ((slotCount == _txSlot) && (hasData) && (txBracket == bracketNum)) {	// if it is the nodes tx slot and it has data to send and it is in its correct bracket
		MACState = MAC_RTS;
	}

	if ((schedChange || !_numNeighbours) && (slotCount % GLOBAL_RX == 0)) {	// if the schedule has changed or node has no known neighbours and its a global rx slot then send Sync message
		MACState = MAC_SYNC_BROADCAST;
	}

	if (slotCount == collectDataSlot) {	// if slotcount equals collectdataSlot then collect sensor data and flag the hasData flag
		helper_collectSensorData();
		hasData = true;
	}

	if (slotCount == (_txSlot + 2) && (hasData == true)) {
		hasData = false;
		txBracket++;
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