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

extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

extern bool hasData;

extern bool hopMessageFlag;
extern bool readyToUploadFlag;

extern bool isRoot;

extern Gpio_t Led_user_red;

// GSM upload slots
uint16_t gsmStopSlot[WINDOW_SCALER];
uint16_t gsmStartSlot[WINDOW_SCALER];

extern uint16_t txSlots[WINDOW_SCALER];

bool schedChange = false;
bool netOp = false;

extern uint8_t numRetries;

static uint16_t lastSync = 0;
//!
//! \brief calculates a new sync probability based on the number of known neighbours.
static bool get_sync(void);

static bool get_sync(void) {

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

void init_scheduler() {
// Set up interrupt to increment slots (Using GPS PPS signal[Maybe])
//	int temp = _txSlot - COLLECT_DATA_SLOT_REL;
//	if (temp < 0) {
//		collectDataSlot = MAX_SLOT_COUNT + temp;
//	}
//	else {
//		collectDataSlot = _txSlot - COLLECT_DATA_SLOT_REL;
//	}

	int i = 0;
	for (i = 0; i < WINDOW_SCALER; i++) {

		gsmStartSlot[i] = i * WINDOW_TIME_SEC;

		gsmStopSlot[i] = i * WINDOW_TIME_SEC + GSM_UPLOAD_DATAGRAMS_TIME;
	}

//	slotCount = 1;
	lastSync = 0;
}

bool uploadOwnData = false;

bool collectDataFlag = false;

extern RTC_C_Calendar currentTime;

extern bool gpsWakeFlag;

bool saveFlashData = false;

extern NextNetOp_t nextNetOp;

static int waitCounter = 0;
extern bool resetFlag;

extern uint8_t _numRoutes;
int scheduler() {
	waitCounter++;

//	if (waitCounter > WINDOW_TIME_SEC) {
//		resetFlag = true;
//	}

	if (_numRoutes >= MAX_ROUTES) {
		_numRoutes = 0;
	}

	WDT_A_clearTimer();
	MACState = MAC_SLEEP;	// by default, sleep
	bool macStateMachineEnable = false;
	int i = 0;
	static bool hasSentGSM = false;
	i = slotCount / WINDOW_TIME_SEC;

	if (slotCount % GPS_WAKEUP_TIME == 0) {
		gpsWakeFlag = true;
	}

	if (slotCount % FLASH_SAVE_DATA == 0) {
		saveFlashData = true;
	}

	if ((slotCount >= gsmStartSlot[i]) && (slotCount <= gsmStopSlot[i])) {
		if (isRoot) {
			if (!hasSentGSM) {
				waitCounter = 0;
				send_uart_pc("Uploading Stored datagrams\n");
				gsm_upload_stored_datagrams();
				hasSentGSM = true;
			}
		} else {
			MACState = MAC_SLEEP;
		}
	} else {
		hasSentGSM = false;

		if (slotCount % GLOBAL_RX == 0) { // Global Sync Slots
			RouteEntry_t tempRoute;
			if (netOp) { // Perform a network layer operation in the global RX window
				netOp = false;

				MACState = MAC_NET_OP;
			} else if (hopMessageFlag) { // hop the received message in the global RX window
				MACState = MAC_HOP_MESSAGE;
				if (numRetries > 3) {
					numRetries = 0;
					MACState = MAC_LISTEN;
				}

			} else if ((numRetries > 0)) {
				if (!isRoot) {
					MACState = MAC_RTS;
					if (numRetries > 3) {
						numRetries = 0;
						MACState = MAC_LISTEN;
					}

				}
			} else if (get_sync()) {
				if (!has_route_to_node(GATEWAY_ADDRESS, &tempRoute)
						&& !isRoot) {
					MACState = MAC_NET_OP;
					nextNetOp = NET_BROADCAST_RREQ;
				} else {
					MACState = MAC_SYNC_BROADCAST;
				}
			} else {
				MACState = MAC_LISTEN;
			}
			macStateMachineEnable = true;
		}

		if (slotCount == (txSlots[i] - 5)) { // if slotcount equals collectdataSlot then collect sensor data and flag the hasData flag
			collectDataFlag = true;
		}

		bool neighbourTx = false;
		int var = 0;
		for (var = 0; var < _numNeighbours; ++var) { // loop through all neighbours and listen if any of them are expected to transmit a message
			if (neighbourTable[var].neighbourTxSlot
					== (slotCount - (i * WINDOW_TIME_SEC))) {
				MACState = MAC_LISTEN;
				send_uart_pc("Neighbour Tx Slot\n");
				macStateMachineEnable = true;
				neighbourTx = true;

				break;
			}
		}

		if (slotCount == txSlots[i]) { // if it is the node's tx slot and it has data to send and it is in its correct bracket
			if (!isRoot) {
				waitCounter = 0;
				MACState = MAC_RTS;
				macStateMachineEnable = true;
			}
		}

		if (macStateMachineEnable) {
			mac_state_machine();
		}
	}

	return true;
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
