/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
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
volatile MACappState_t MACState = NODE_DISC;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool sleepFlag = true;
bool syncFlag = false;
bool RTSFlag = false;
bool CTSFlag = false;
bool DataFlag = false;

extern Gpio_t Led_rgb_green;
extern Gpio_t Led_rgb_blue;
extern Gpio_t Led_rgb_red;

static void scheduleSetup();
/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer();

static void syncSchedule();
static bool RxStateMachine();

void MacInit() {
	do {
		uint8_t temp = SX1276Random8Bit();
		_nodeID = temp;
		_ranNum = (uint16_t) (temp * 86) + 10000;
	} while (_nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = _ranNum;
}

void MACreadySend(uint8_t *dataToSend, uint8_t dataLen) {
	hasData = true;
	_dataLen = dataLen;
	memcpy(txDataArray, dataToSend, _dataLen);
}

bool MACStateMachine() {
	if (sleepFlag) {
		RxStateMachine();
	}
	return true;
}

static void scheduleSetup() {
	mySchedule.nodeID = _nodeID;
	mySchedule.sleepTime = _sleepTime;
	startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
}

bool MACSend(MessageType_t messageType, uint8_t *data, uint8_t len) {
	_dataLen = len;
	createDatagram(messageType, data);
	datagramToArray();
//	startLoRaTimer(5000);
	Radio.Send(TXBuffer, sizeof(myDatagram.header) + _dataLen);
	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			return true;
		} else if (RadioState == TXTIMEOUT) {
			return false;
		}
	}
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
		switch (rxdatagram.header.messageType) {
		case SYNC: {
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
		case DATA:
			break;
		case RTS:
			break;
		case CTS:
			break;
		default:
			break;
		}
		return true;
	} else
		return false;
}

static void syncSchedule() {
	mySchedule.sleepTime = rxdatagram.header.nextWake;
	_sleepTime = mySchedule.sleepTime;
	mySchedule.nodeID = _nodeID;
	startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
}

static bool RxStateMachine() {
	while (true) {
		switch (MACState) {
		case NODE_DISC: {
			bool rxSyncMsg = MACRx(_sleepTime);
			if (rxSyncMsg) {
				syncSchedule();
				MACSend(SYNC, emptyArray, 00);
				MACState = MAC_SLEEP;
				SX1276SetSleep();
			} else {
				scheduleSetup();
				startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
				if (MACSend(SYNC, emptyArray, 00)) {
					MACRx(100);
					MACState = MAC_SLEEP;
					SX1276SetSleep();
				} else {
					MACState = NODE_DISC;
				}
			}
		}
			break;
		case MAC_SLEEP:
			if (sleepFlag) {
				sleepFlag = false;
//			startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
				MACState = SYNC_MAC;

			} else {
				return false;
			}
			break;
		case SYNC_MAC:
			if (hasData) {
				Delayms(10);
				if (MACSend(SYNC, emptyArray, 0)) {
					MACState = MAC_RTS;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			} else {
				if (MACRx(100)) {
					MACState = MAC_RTS;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			}
			GPIO_setOutputLowOnPin(GPIO_PORT_P2, GPIO_PIN2);
			break;
		case MAC_RTS:
			if (hasData) {
				Delayms(10);
				if (MACSend(RTS, emptyArray, 0)) {
					MACState = MAC_CTS;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			} else {
				if (MACRx(100)) {
					MACState = MAC_CTS;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			}
			break;
		case MAC_CTS:
			if (hasData) {
				if (MACRx(100)) {
					MACState = MAC_DATA;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			} else {
				Delayms(10);
				if (MACSend(CTS, emptyArray, 0)) {
					MACState = MAC_DATA;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			}
			break;
		case MAC_DATA:
			if (hasData) {
				Delayms(10);
				if (MACSend(DATA, txDataArray, 5)) {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
					hasData = false;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			} else {
				if (MACRx(200)) {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				} else {
					SX1276SetSleep();
					MACState = MAC_SLEEP;
				}
			}
			break;
		}
	}
}

