/*
 * datagram.c
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */


#include "datagram.h"

bool datagramInit( ) {
	return false;
}

void createDatagram( ) {
	TXBuffer[0] = nodeID;		//ID byte 4
	TXBuffer[1] = nodeID >> 8;	//ID byte 3
	TXBuffer[2] = nodeID >> 16;	//ID byte 2
	TXBuffer[3] = nodeID >> 24;	//ID byte 1
	int i = 0;
	for (i = 0; i < sizeof(mySchedule.sleepTime); i++) {
		TXBuffer[i + 4] = mySchedule.sleepTime >> 8 * i;
	}
	for (i = 0; i < sizeof(mySchedule.numNeighbours); i++) {
		TXBuffer[i + 8] = mySchedule.numNeighbours >> 8 * i;
	}


	__no_operation();
}



