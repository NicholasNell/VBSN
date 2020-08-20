/*
 * my_MAC.h
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#ifndef MY_MAC_H_
#define MY_MAC_H_

#include <stdint.h>
#include <stdbool.h>
#include "my_gpio.h"
#include "my_spi.h"
#include "radio.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include <stdio.h>

#define BROADCAST_ADDRESS 0xFF
#define MAX_LEN 255

typedef enum {
	SLEEP = 0, PENDING, RETRANSMIT, EXPIRED, ACK_RESP
} AlohaState_t;

typedef enum {
	LOWPOWER = 0, IDLE,

	RX, RX_TIMEOUT, RX_ERROR,

	TX, TX_TIMEOUT,

	CAD, CAD_DONE
} AppStates_t;

typedef enum {
	SYNC = 0, ACK, DATA, RTS, CTS
} MessageType_t;

typedef struct {
		uint32_t sleepTime;
		uint32_t listenTime;
		uint8_t numNeighbours;
		uint32_t syncTime;
} schedule_t;

typedef struct {
		uint8_t neighbourID;
		schedule_t scheduleTable[];
} scheduleTable_t;

volatile uint32_t nodeID;

volatile schedule_t mySchedule;



void MacInit( );

bool MACStateMachine( );

#endif /* MY_MAC_H_ */
