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



bool schedule_setup = false;
volatile schedule_t mySchedule;
extern datagram_t myDatagram;
volatile MACappState_t MACState = SYNCRX;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool sleepFlag = false;
bool syncFlag = false;
bool RTSFlag = false;
bool CTSFlag = false;
bool hasData = false;

static void scheduleSetup( );
/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer( );

static void syncSchedule( );
static bool RxStateMachine( );
static bool TxStateMachine( );

void MacInit( ) {
	do {
		uint8_t temp = SX1276Random8Bit();
		_nodeID = temp;
	} while (_nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = 10000;
	scheduleSetup();
	do {
		_ranNum = (uint16_t) SX1276Random();
	} while (_ranNum > 32000 || _ranNum < 10000);
}

bool MACStateMachine( ) {
	if (hasData) {
		return TxStateMachine();
	}
	else {
		return RxStateMachine();
	}
}

static void scheduleSetup( ) {
	uint8_t len = sizeof(datagram_t);
	uint16_t timeOnAir = SX1276GetTimeOnAir(
			MODEM_LORA,
			SX1276.Settings.LoRa.Bandwidth,
			SX1276.Settings.LoRa.Datarate,
			SX1276.Settings.LoRa.Coderate,
			SX1276.Settings.LoRa.PreambleLen,
			SX1276.Settings.LoRa.FixLen,
			len,
			SX1276.Settings.LoRa.CrcOn) * 5;
	mySchedule.nodeID = _nodeID;
	mySchedule.RTSTime = timeOnAir;
	mySchedule.numNeighbours = _numNeighbours;
	mySchedule.CTSTime = timeOnAir;
	mySchedule.dataTime = SX1276GetTimeOnAir(
			MODEM_LORA,
			SX1276.Settings.LoRa.Bandwidth,
			SX1276.Settings.LoRa.Datarate,
			SX1276.Settings.LoRa.Coderate,
			SX1276.Settings.LoRa.PreambleLen,
			SX1276.Settings.LoRa.FixLen,
			255,
			SX1276.Settings.LoRa.CrcOn) * 5;
	mySchedule.sleepTime = _sleepTime;
	mySchedule.syncTime = timeOnAir;
	_totalScheduleTime = mySchedule.RTSTime + mySchedule.CTSTime
			+ mySchedule.dataTime + mySchedule.sleepTime + mySchedule.syncTime;
	_scheduleTimeLeft = _totalScheduleTime;

}

bool MACSend( MessageType_t messageType, uint8_t *data, uint8_t len ) {
	_dataLen = len;
	createDatagram(messageType, data);
	datagramToArray();
	startLoRaTimer(5000);
	Radio.Send(TXBuffer, sizeof(myDatagram.header) + _dataLen);
	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			return true;
		}
		else if (RadioState == TXTIMEOUT) {
			return false;
		}
	}
}

bool MACRx( uint32_t timeout ) {
	Radio.Rx(0);
	RadioState = RX;
	startLoRaTimer(timeout);
	while (true) {
		if (RadioState == RXTIMEOUT) {
			return false;
		}
		else if (RadioState == RXERROR) {
			return false;
		}
		else if (RadioState == RXDONE) {
			if (processRXBuffer())
				return true;
			else return false;
		}
	}
}

static bool processRXBuffer( ) {
	if (ArrayToDatagram()) {
		switch (rxdatagram.header.messageType) {
			case SYNC: {
				bool containsNode = false;
				uint8_t i = 0;

				for (i = 0; i < MAX_NEIGHBOURS; i++) {
					if (rxdatagram.header.thisSchedule.nodeID
							== scheduleTable[i].nodeID) {
						containsNode = true;
						break;
					}
				}
				if (!containsNode) {
					_numNeighbours += 1;
					scheduleTable[_numNeighbours] =
							rxdatagram.header.thisSchedule;
				}
				else {

				}
			}
				break;
			case ACK:
				break;
			case DATA:
				break;
			case RTS:
				CTSFlag = true;
				break;
			case CTS:
				break;
			default:
				break;
		}
		return true;
	}
	else return false;
}

static void syncSchedule( ) {
	mySchedule = rxdatagram.header.thisSchedule;
	mySchedule.nodeID = _nodeID;
}

static bool RxStateMachine( ) {
switch (MACState) {
	case SYNCRX:
		if (!syncFlag) {
			if (MACRx(mySchedule.syncTime)) {

			}
			else {
				SX1276SetSleep();
				RadioState = RADIO_SLEEP;
			}
		}
		else {
			MACState = LISTEN_RTS;
			SystickInit(mySchedule.RTSTime, &RTSFlag);
			syncFlag = false;
			}

		break;
	case SYNCTX:
		if (MACSend(SYNC, emptyArray, 0)) {
			SX1276SetSleep();
			RadioState = RADIO_SLEEP;
			SystickInit(mySchedule.sleepTime, &sleepFlag);
			MACState = SLEEP;
			}
		break;
	case SLEEP:
		if (sleepFlag) {
			SystickInit(mySchedule.syncTime, &syncFlag);
			MACState = SYNCRX;
			sleepFlag = false;
			}
		break;
	case LISTEN_RTS:
		if (!RTSFlag) {
			if (MACRx(mySchedule.RTSTime)) {

			}
		}
			else {

		}
		break;
	case SEND_CTS:
		break;
	case SCHEDULE_SETUP:
		break;
	case SCHEDULE_ADOPT:
		syncSchedule();
		break;
	default:
		break;
	}
	return true;
}

static bool TxStateMachine( ) {
switch (MACState) {
	case SYNCRX:
		break;
	case SYNCTX:
		break;
	case SLEEP:
		break;
	case LISTEN_RTS:
		break;
	case SEND_CTS:
		break;
	case SCHEDULE_SETUP:
		break;
	case SCHEDULE_ADOPT:
		break;
	default:
		break;
}
	return true;
}
