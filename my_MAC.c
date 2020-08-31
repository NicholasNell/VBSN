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


volatile bool hasData = false;
bool schedule_setup = false;
volatile schedule_t mySchedule;
extern datagram_t myDatagram;
volatile MACappState_t MACState = MAC_SLEEP;
uint8_t emptyArray[0];
extern SX1276_t SX1276;
bool sleepFlag = true;
bool syncFlag = false;
bool RTSFlag = false;
bool CTSFlag = false;
bool DataFlag = false;

static void scheduleSetup( );
/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer( );

static void syncSchedule( );
static bool RxStateMachine( );
static bool TxStateMachine( );

void MacInit( ) {
//	do {
//		uint8_t temp = SX1276Random8Bit();
//		_nodeID = temp;
//		_ranNum = (uint16_t) (temp * 1000);
//	} while (_nodeID == 0xFF);
	_numNeighbours = 0;
	_sleepTime = 10000;
	scheduleSetup();
}

void MACreadySend( uint8_t *dataToSend, uint8_t dataLen ) {
	hasData = true;
	_dataLen = dataLen;
	memcpy(txDataArray, dataToSend, _dataLen);
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
//	startLoRaTimer(5000);
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
				schedule_setup = true;
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
	while (true) {
		switch (MACState) {
			case SYNC_MAC:
			{
				bool rxData = MACRx(mySchedule.syncTime);
				if (!schedule_setup) {
					while (!MACRx(mySchedule.syncTime)) {

					}
				}
				if (syncFlag) {
					syncFlag = false;
					if (rxData) {
						MACState = LISTEN_RTS;
						startTimerAcounter(mySchedule.RTSTime, &RTSFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.RTSTime + mySchedule.CTSTime
										+ mySchedule.dataTime
										+ mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
			}
				break;
			case MAC_SLEEP:
				if (sleepFlag) {
					startTimerAcounter(mySchedule.syncTime, &syncFlag);
					MACState = SYNC_MAC;
					sleepFlag = false;
				}
				return false;
			break;
			case LISTEN_RTS:
			{
				bool rxData = MACRx(mySchedule.RTSTime);
				if (RTSFlag) {
					RTSFlag = false;
					if (rxData) {
						MACState = SEND_CTS;
						startTimerAcounter(mySchedule.CTSTime, &CTSFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.CTSTime + mySchedule.dataTime
										+ mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
			}
				break;
			case SEND_CTS:
			{
				bool txData = MACSend(CTS, emptyArray, 0);
				if (CTSFlag) {
					CTSFlag = false;
					if (txData) {
						MACState = RXDATA;
						startTimerAcounter(mySchedule.dataTime, &DataFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.dataTime
										+ mySchedule.sleepTime,
								&sleepFlag);

						return false;
					}
				}
			}
				break;
			case RXDATA:
			{
				bool rxData = MACRx(mySchedule.dataTime);
				if (DataFlag) {
					DataFlag = false;
					if (rxData) {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
						return true;
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
						return false;
					}
				}
			}
				break;
			default:
				SX1276SetSleep();
				RadioState = RADIO_SLEEP;
				MACState = MAC_SLEEP;
				return false;
				break;
		}
	}
}

static bool TxStateMachine( ) {
	while (true) {
		switch (MACState) {
			case SYNC_MAC: {
				bool txData = MACSend(SYNC, emptyArray, 0);
				if (syncFlag) {
					syncFlag = false;
					if (txData) {
						MACState = SEND_RTS;
						startTimerAcounter(mySchedule.RTSTime, &RTSFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.RTSTime + mySchedule.CTSTime
										+ mySchedule.dataTime
										+ mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
				}
				break;
			case MAC_SLEEP: {
				if (sleepFlag) {
					startTimerAcounter(mySchedule.syncTime, &syncFlag);
					MACState = SYNC_MAC;
					sleepFlag = false;
				}

				return false;
			}
				break;
			case SEND_RTS: {
				bool txData = MACSend(RTS, emptyArray, 0);
				if (RTSFlag) {
					RTSFlag = false;
					if (txData) {
						MACState = LISTEN_CTS;
						startTimerAcounter(mySchedule.CTSTime, &CTSFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.CTSTime + mySchedule.dataTime
										+ mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
				}
				break;
			case LISTEN_CTS: {
				bool rxData = MACRx(mySchedule.CTSTime);
				if (CTSFlag) {
					CTSFlag = false;
					if (rxData) {
						MACState = TXDATA;
						startTimerAcounter(mySchedule.dataTime, &DataFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.dataTime + mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
				}
				break;
			case TXDATA: {
				bool txData = MACSend(DATA, txDataArray, _dataLen);
				if (DataFlag) {
					DataFlag = false;
					if (txData) {
						MACState = MAC_SLEEP;
						startTimerAcounter(mySchedule.sleepTime, &sleepFlag);
					}
					else {
						SX1276SetSleep();
						RadioState = RADIO_SLEEP;
						MACState = MAC_SLEEP;
						startTimerAcounter(	//Sleep for the rest of the schedule period;
								mySchedule.sleepTime,
								&sleepFlag);
						return false;
					}
				}
				}
				break;
			default:
				break;
		}
	}
	return true;

}
