/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include <my_flash.h>
#include <my_gps.h>
#include <my_UART.h>
#include <stdio.h>
#include "my_MAC.h"
#include "datagram.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "my_systick.h"
#include "my_gpio.h"

volatile bool hasData = false;
extern datagram_t myDatagram;
extern datagram_t rxdatagram;
volatile MACappState_t MACState = MAC_LISTEN;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool macFlag = false;

extern Gpio_t Led_rgb_green;
extern Gpio_t Led_rgb_blue;
extern Gpio_t Led_rgb_red;

schedule_t scheduleTable[MAX_NEIGHBOURS];

uint8_t neighbourTable[MAX_NEIGHBOURS];
uint8_t _nodeID;
volatile uint8_t _dataLen;
volatile uint8_t _numNeighbours;
volatile uint32_t _sleepTime;
volatile uint8_t _destID;
extern uint8_t RXBuffer[MAX_MESSAGE_LEN];
volatile MACRadioState_t RadioState;
extern uint8_t TXBuffer[MAX_MESSAGE_LEN];
uint8_t txDataArray[MAX_MESSAGE_LEN];

extern locationData gpsData;

static void scheduleSetup();
/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer();

//static void syncSchedule();
static bool stateMachine();

void MacInit() {
	uint8_t tempID;
	tempID = flashReadNodeID();
	if (tempID == 0xFF || tempID == 0x00) {
		uint8_t tempLat = (int32_t) (gpsData.lat * 1000000)
				% (int32_t) gpsData.lon;
		uint8_t tempLon = (int32_t) (gpsData.lon * 1000000)
				% (int32_t) gpsData.lat;
		_nodeID = tempLat + tempLon;
		if (_nodeID == 0xff) {
			_nodeID = _nodeID % tempLat;
		}
		flashWriteNodeID();
	} else {
		_nodeID = tempID;
	}

	_numNeighbours = 0;
	_sleepTime = SLEEP_TIME;
	scheduleSetup();
}

void MACreadySend(uint8_t *dataToSend, uint8_t dataLen) {
	hasData = true;
	_dataLen = dataLen;
	memcpy(txDataArray, dataToSend, _dataLen);
}

bool MACStateMachine() {
	if (macFlag) {
		stateMachine();
	}
	return true;
}

static void scheduleSetup() {
	mySchedule.nodeID = _nodeID;
	mySchedule.sleepTime = _sleepTime;
}

bool MACSend() {

}

bool MACRx(uint32_t timeout) {
	Radio.Rx(0);
	RadioState = RX;
	startLoRaTimer(timeout);
	while (true) {
		if (RadioState == RXTIMEOUT) {
			return false;
		} else if (RadioState == RXERROR) {
			return false;
		} else if (RadioState == RXDONE) {
			if (processRXBuffer())
				return true;
			else
				return false;
		}
	}
}

static bool processRXBuffer() {
	rxdatagram.header.flags = RXBuffer[4];

	switch (rxdatagram.header.flags) {
	case 0x01: 	// RTS
		MACState = MAC_CTS;
		break;
	case 0x02: 	// CTS
		MACState = MAC_DATA;
		break;
	case 0x03: 	// SYNC
		break;
	case 0x04: 	// DATA
		MACState = MAC_ACK;
		break;
	case 0x05: 	// ACK
		MACState = MAC_SLEEP;
		break;
	case 0x06: 	// DISC
		break;
	default:
		return false;
		break;
	}
	return true;
}

static bool stateMachine() {
	while (true) {
		switch (MACState) {
		case MAC_LISTEN:
			if (MACRx(1000)) {

			}
			break;
		case NODE_DISC:
			break;
		case MAC_SLEEP:	// SLEEP
			macFlag = false;
			break;
		case SYNC_MAC:
			break;
		case MAC_RTS:	// Send RTS
			break;
		case MAC_CTS:
			MACSend(); // Send CTS
			break;
		case MAC_DATA: // send sensor data

			break;
		default:
			break;
		}
	}
}

