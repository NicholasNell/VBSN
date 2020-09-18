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
bool schedule_setup = false;
volatile schedule_t mySchedule;
extern datagram_t myDatagram;
extern datagram_t rxdatagram;
volatile MACappState_t MACState = NODE_DISC;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool sleepFlag = true;
bool discFlag = false;

extern Gpio_t Led_rgb_green;
extern Gpio_t Led_rgb_blue;
extern Gpio_t Led_rgb_red;

schedule_t scheduleTable[MAX_NEIGHBOURS];

uint8_t neighbourTable[MAX_NEIGHBOURS];
uint16_t _RTSTime;	// the time this node will listen in ms
uint16_t _CTSTime;
uint16_t _dataTime;
uint16_t _syncTime;	// The time it takes to send a SYNC message in ms, node will use this to listen initially.
uint32_t _ranNum;
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
	if (sleepFlag) {
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
	if (ArrayToDatagram()) {
		switch (rxdatagram.header.flags) {
		case 0x01: {
//				syncSchedule();
			bool containsNode = false;
			uint8_t i = 0;
			for (i = 0; i < _numNeighbours; i++) {
				if (rxdatagram.header.source == neighbourTable[i]) {
					containsNode = true;
					break;
				}
			}
			if (!containsNode) {
				neighbourTable[_numNeighbours++] = rxdatagram.header.source;
			}
		}
			break;
		case 0x02:
			break;
		case 0x03:
			break;
		case 0x04:
			break;
		default:
			break;
		}
		return true;
	} else
		return false;
}

static bool stateMachine() {
	while (true) {
		switch (MACState) {
		case NODE_DISC:
			break;
		case MAC_SLEEP:
			break;
		case SYNC_MAC:
			break;
		case MAC_RTS:
			break;
		case MAC_CTS:
			break;
		case MAC_DATA:
			break;
		default:
			break;
		}
	}
}

