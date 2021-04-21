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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/wdt_a.h>

static uint16_t slotCount;

extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

extern bool hasData;

extern bool hopMessageFlag;
extern bool readyToUploadFlag;

extern bool isRoot;

extern Gpio_t Led_user_red;

bool schedChange = false;
bool netOp = false;
volatile uint8_t syncProbability = SYNC_PROB; // the probability of sending out a node discovery message in a global rx slot (as a percentage)

//!
//! \brief calculates a new sync probability based on the number of known neighbours.
void calculate_new_sync_prob( void );

void calculate_new_sync_prob( void ) {

	syncProbability = SYNC_PROB / (1 + _numNeighbours);
	/*
	 30.0
	 22.5
	 16.666666666666668
	 11.25
	 6.0
	 */
}

void init_scheduler( ) {
	// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
//	int temp = _txSlot - COLLECT_DATA_SLOT_REL;
//	if (temp < 0) {
//		collectDataSlot = MAX_SLOT_COUNT + temp;
//	}
//	else {
//		collectDataSlot = _txSlot - COLLECT_DATA_SLOT_REL;
//	}

	slotCount = 1;

	calculate_new_sync_prob();
}

bool uploadOwnData = false;

bool collectDataFlag = false;

extern RTC_C_Calendar currentTime;
int scheduler( ) {
//	gpio_toggle(&Led_user_red);
	WDT_A_clearTimer();
	MACState = MAC_LISTEN;	// by default, listen
	bool macStateMachineEnable = true;

//	if ((slotCount + UPLOAD_DATAGRAM_OFFSET) % UPLOAD_DATAGRAMS_TIME == 0
//			&& isRoot) {
//		gsm_upload_stored_datagrams();
//	}
//	if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
//		char s[5];
//		memset(s, 0, 5);
//		sprintf(s, "%d", convert_hex_to_dec_by_byte(currentTime.seconds));
//		SX1276Send((uint8_t*) s, 5);
//		gpio_toggle(&Led_user_red);
//	}

	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) { // loop through all neighbours and listen if any of them are expected to transmit a message
		if (slotCount % neighbourTable[var].neighbourTxSlot == 0) {
			MACState = MAC_LISTEN;
			macStateMachineEnable = true;
			break;
		}
	}

	if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
		MACState = MAC_LISTEN;
		macStateMachineEnable = true;
	}

	if (((slotCount + _txSlot) % TIME_TO_SEND_SEC == 0)) { // if it is the node's tx slot and it has data to send and it is in its correct bracket
		if (!isRoot) {
			MACState = MAC_RTS;
			macStateMachineEnable = true;
		}
		else {
			MACState = MAC_SLEEP;
			uploadOwnData = true;
		}
	}

//	if (!isRoot) {
	calculate_new_sync_prob();
	if ((schedChange && (slotCount % GLOBAL_RX == 0))
			|| ((rand() * 100 < syncProbability * RAND_MAX)
					&& (slotCount % GLOBAL_RX == 0))) { // if the schedule has changed or node has no known neighbours and its a global rx slot then send Sync message
		MACState = MAC_SYNC_BROADCAST;
		macStateMachineEnable = true;
	}
//	}

	if ((slotCount + TIME_TO_COLLECT_DATA_OFFSET) % TIME_TO_COLLECT_DATA_SEC
			== 0) { // if slotcount equals collectdataSlot then collect sensor data and flag the hasData flag
		collectDataFlag = true;
	}

	if (slotCount % (_txSlot) && (hasData == true)) {
//		hasData = false;
	}

	if ((slotCount % GLOBAL_RX == 0) && netOp) { // Perform a network layer operation in the global RX window
		netOp = false;
		macStateMachineEnable = true;
		MACState = MAC_NET_OP;
	}

	if ((slotCount % GLOBAL_RX == 0) && hopMessageFlag) { // hop the received message in the global RX window
		hopMessageFlag = false;
		MACState = MAC_HOP_MESSAGE;
		macStateMachineEnable = true;
	}

//	if (readyToUploadFlag) {
//		readyToUploadFlag = false;
//		gsm_upload_stored_datagrams();
//	}
//	if ((slotCount + WRITE_FLASH_DATA_OFFSET) % WRITE_FLASH_DATA_TIME == 0) {
//		flash_fill_struct_for_write();
//		flash_write_struct_to_flash();
//	}

	if (macStateMachineEnable) {
		mac_state_machine();
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
