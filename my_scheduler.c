/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_GSM.h>
#include <my_MAC.h>
#include <my_scheduler.h>
#include <stdbool.h>
#include <stdlib.h>

static uint16_t slotCount;

static uint16_t collectDataSlot;

extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

extern bool hasData;

extern bool hopMessageFlag;

extern bool isRoot;

bool schedChange = false;
bool netOp = false;

void init_scheduler( ) {
	// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
	collectDataSlot = _txSlot - COLLECT_DATA_SLOT_REL;
	slotCount = 1;
}

int scheduler( ) {
	bool macStateMachineEnable = false;
	bool uploadOwnData = false;

	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) { // loop through all neighbours and listen if any of them are expected to transmit a message
		if (slotCount == neighbourTable[var].neighbourTxSlot) {
			MACState = MAC_LISTEN;
			macStateMachineEnable = true;
			break;
		}
	}

	if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
		MACState = MAC_LISTEN;
		macStateMachineEnable = true;
	}

	if ((slotCount == _txSlot) && (hasData)) { // if it is the nodes tx slot and it has data to send and it is in its correct bracket
		if (!isRoot) {
			MACState = MAC_RTS;
			macStateMachineEnable = true;
		}
		else {
			MACState = MAC_SLEEP;
			uploadOwnData = true;
		}
	}

	if ((schedChange && (slotCount % GLOBAL_RX == 0))
			|| ((rand() * 100 < SYNC_PROB * RAND_MAX)
					&& (slotCount % GLOBAL_RX == 0))) { // if the schedule has changed or node has no known neighbours and its a global rx slot then send Sync message
		MACState = MAC_SYNC_BROADCAST;
		macStateMachineEnable = true;
	}

	if (slotCount == collectDataSlot) {	// if slotcount equals collectdataSlot then collect sensor data and flag the hasData flag
		helper_collect_sensor_data();
		hasData = true;
	}

	if (slotCount == (_txSlot + 2) && (hasData == true)) {
		hasData = false;
	}

	if ((slotCount % GLOBAL_RX == 0) && netOp) {
		netOp = false;
		macStateMachineEnable = true;
		MACState = MAC_NET_OP;
	}

	if ((slotCount % GLOBAL_RX == 0) && hopMessageFlag) {
		hopMessageFlag = false;
		MACState = MAC_HOP_MESSAGE;
		macStateMachineEnable = true;
	}

	if (macStateMachineEnable) {
		mac_state_machine();
	}
	if (uploadOwnData) {
		gsm_upload_my_data();
	}
	return true;
}

uint16_t get_slot_count( ) {
	return slotCount;
}

void set_slot_count( uint16_t newSlotCount ) {
	slotCount = newSlotCount;
}

void increment_slot_count( ) {
	slotCount++;
}
