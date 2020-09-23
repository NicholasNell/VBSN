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

uint16_t slotCount;
extern uint16_t neighbourSyncSlots[255];

extern uint8_t _numNeighbours;
extern uint16_t _txSlot;

extern volatile MACappState_t MACState;

void initScheduler() {
	// Set up interrupt to increment slots (Using GPS PPS signal)
	slotCount = 0;
}

int scheduler() {
	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) {
		if (slotCount == neighbourSyncSlots[var]) {
			MACState = MAC_LISTEN;
			MACStateMachine();
		}
	}

	if (slotCount == (_txSlot - 8)) { // collect Sensor Data
		helper_collectSensorData();
	} else if (slotCount == 30) {	// update GPS information
		sendUARTgps("$PCAS03,0,1,0,0,0,0,1,0*02\r\n");
	} else if (slotCount == _txSlot) { // send Sensor data
		MACState = MAC_RTS;
		MACStateMachine();
	}
	return true;
}

