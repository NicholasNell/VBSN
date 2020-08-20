/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"
#include <string.h>
#include "datagram.h"

void scheduleSetup( );

void MacInit( ) {
	nodeID = SX1276Random();
}

bool MACStateMachine( ) {
	return false;
}

void sceduleSetup( ) {
	uint32_t len = sizeof(datagram_t);
	mySchedule.listenTime = SX1276GetTimeOnAir(MODEM_LORA, len);
	mySchedule.numNeighbours = 0;
	mySchedule.sleepTime = 0;
	mySchedule.syncTime = 0;
}


