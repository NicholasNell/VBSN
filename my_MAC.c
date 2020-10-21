/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#include <bme280_defs.h>
#include <datagram.h>
#include <main.h>
#include <my_flash.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_MAC.h>
#include <my_RFM9x.h>
#include <my_scheduler.h>
#include <my_timer.h>
#include <MAX44009.h>
#include <radio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

// Array of carrier sense times
static uint32_t carrierSenseTimes[0xff];
static uint8_t carrierSenseSlot = 0;

// Datagram containing the data to be sent over the medium
Datagram_t txDatagram;

// Does this node have data to send?
bool hasData = false;

// State of the MAC state machine
volatile MACappState_t MACState = NODE_DISC;

// empty array for testing
uint8_t emptyArray[0];

// pointer to the radio module object
extern SX1276_t SX1276;

// LED gpio objects
extern Gpio_t Led_rgb_green;
extern Gpio_t Led_rgb_blue;
extern Gpio_t Led_rgb_red;

// Size of recieved LoRa buffer
extern volatile uint8_t loraRxBufferSize;

// array of known neighbours
Neighbour_t neighbourTable[MAX_NEIGHBOURS];

// This node MAC parameters:
uint8_t _nodeID;
uint8_t _dataLen;
uint8_t _numNeighbours;
uint8_t _destID;
uint16_t _numMsgSent;
uint16_t _prevMsgId;
uint16_t _txSlot;
uint16_t _schedID;

// The LoRa RX Buffer
extern uint8_t RXBuffer[MAX_MESSAGE_LEN];

// Datagram from received message
Datagram_t rxdatagram;

// should mac init
extern bool macFlag;

// bme280Data
extern struct bme280_data bme280Data;

// soil moisture data
extern float soilMoisture;

// schedChange
extern bool schedChange;

// current RadioState
LoRaRadioState_t RadioState;

// LoRa transmit buffer
uint8_t TXBuffer[MAX_MESSAGE_LEN];

//GPS Data
extern LocationData gpsData;

// lora tx times
extern uint16_t loraTxtimes[255];

// data collection time
RTC_C_Calendar timeStamp;

/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer();

static bool MACSend(uint8_t msgType, uint8_t dest);

static bool MACRx(uint32_t timeout);

static void genID(bool genNew);

static void genID(bool genNew) {
	uint8_t tempID = 0x0;

	if (genNew) {
		while (tempID == 0xFF || tempID == 0x00 || tempID == _nodeID) {
			tempID = (uint8_t) rand();
		}
		_nodeID = tempID;
	} else {
		tempID = flashReadNodeID();

		while (tempID == 0xFF || tempID == 0x00) {
			tempID = (uint8_t) rand();
		}
		_nodeID = tempID;
	}
	flashWriteNodeID();
}

void MacInit() {

	uint8_t i;
	for (i = 0; i < 255; ++i) {
		{
			double temp = (double) rand();
			temp /= 65535.0;
			temp *= SLOT_LENGTH_MS;

			carrierSenseTimes[i] = (uint32_t) temp;
		}
	}

	genID(false);

	do {
		double temp = (double) rand();
		temp /= 65535.0;
		temp *= MAX_SLOT_COUNT * 2;
		_txSlot = (uint16_t) temp / POSSIBLE_TX_SLOT;
		_txSlot *= POSSIBLE_TX_SLOT;
	} while (_txSlot % GLOBAL_RX == 0);

	schedChange = true;

	_dataLen = 0;
	_numNeighbours = 0;
	_destID = BROADCAST_ADDRESS;
	_numMsgSent = 0;
	memset(neighbourTable, NULL, MAX_NEIGHBOURS);
}

bool MACStateMachine() {
	while (true) {
		switch (MACState) {
		case MAC_SYNC_BROADCAST:
			if (!MACRx(carrierSenseTimes[carrierSenseSlot++])) {
				if (MACSend(0x6, BROADCAST_ADDRESS)) {
					schedChange = false;
					MACState = MAC_SLEEP;
					return true;
				} else {
					MACState = MAC_SLEEP;
					return false;
				}
			}
			MACState = MAC_SLEEP;
			break;
		case MAC_LISTEN:
			if (!MACRx(SLOT_LENGTH_MS)) {
				MACState = MAC_SLEEP;
				return false;
			}
			break;
		case MAC_SLEEP:
			// SLEEP
			if (RadioState != RADIO_SLEEP) {
				SX1276SetSleep();
				RadioState = RADIO_SLEEP;
			}
			return false;
		case MAC_RTS:
			// Send RTS
			//			net_getNextDest(); // get the next hop destination from the network layer
			if (SX1276IsChannelFree(MODEM_LORA, RF_FREQUENCY, -80,
					carrierSenseTimes[carrierSenseSlot++])) {
				if (MACSend(0x1, neighbourTable[0].neighbourID)) {	// Send RTS
					if (!MACRx(5000)) {
//						if (MACSend(0x1, neighbourTable[0].neighbourID)) {
//							if (!MACRx(1000)) {
//								MACState = MAC_SLEEP;
//							}
//						}
						MACState = MAC_SLEEP;
						return false;
					}
				} else {
					MACState = MAC_SLEEP;
					return false;
				}
			} else {
				MACState = MAC_SLEEP;
				return false;
			}
			break;
		case MAC_CTS:
			if (MACSend(0x2, rxdatagram.macHeader.source)) {	// Send CTS
				if (!MACRx(5000)) {
//					if (MACSend(0x2, rxdatagram.macHeader.source)) {
//						if (!MACRx(1000)) {
//							MACState = MAC_SLEEP;
//						}
//					}
					MACState = MAC_SLEEP;
					return false;
				}
			} else {
				MACState = MAC_SLEEP;
				return false;
			}
			break;
		case MAC_DATA:
			// send sensor data
			if (MACSend(0x4, rxdatagram.macHeader.source)) {	// Send DATA
				if (!MACRx(5000)) {

//					if (MACSend(0x4, rxdatagram.macHeader.source)) {// Send DATA
//						if (!MACRx(1000)) {
//							MACState = MAC_SLEEP;
//							return false;
//						}
//					}
					MACState = MAC_SLEEP;
					return false;
				} else {
					MACState = MAC_SLEEP;
					return true;
				}
			} else {
				MACState = MAC_SLEEP;
				return false;
			}
		case MAC_ACK:
			if (MACSend(0x5, rxdatagram.macHeader.source)) { // Send ACK
				MACState = MAC_SLEEP;
				return true;
			} else {
				MACState = MAC_SLEEP;
				return false;
			}
		case MAC_WAIT:
			break;
		default:
			return false;
		}
		return false;
	}
}

bool MACSend(uint8_t msgType, uint8_t dest) {

	if (dest != NULL) {
		txDatagram.macHeader.dest = dest;
	} else {
		txDatagram.macHeader.dest = BROADCAST_ADDRESS;
	}
	txDatagram.macHeader.source = _nodeID;
	txDatagram.macHeader.flags = msgType;
	txDatagram.macHeader.msgID = (uint16_t) (_nodeID << 8)
			| (uint16_t) (_numMsgSent);
	txDatagram.macHeader.sched.txSlot = _txSlot;
	txDatagram.macHeader.sched.schedID = _nodeID;
	// message type:
	//	RTS:	0x1
	// 	CTS:	0x2
	// 	SYNC:	0x3
	// 	Data:	0x4
	// 	Ack:	0x5
	// 	Disc:  	0x6
	if (msgType == 0x4) {
		txDatagram.data.hum = bme280Data.humidity;
		txDatagram.data.press = bme280Data.pressure;
		txDatagram.data.temp = bme280Data.temperature;
		txDatagram.data.lux = getLux();
		txDatagram.data.gpsData = gpsData;
		txDatagram.data.soilMoisture = soilMoisture;
		txDatagram.data.tim = timeStamp;

		int size = sizeof(txDatagram);
		memcpy(TXBuffer, &txDatagram, size);
		startLoRaTimer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	} else {
		int size = sizeof(txDatagram.macHeader);
		memcpy(TXBuffer, &txDatagram, size);
		startLoRaTimer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	}

	RadioState = TX;
	bool retVal = false;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
		} else if (RadioState == TXTIMEOUT)
			retVal = false;
	}

	stopLoRaTimer();
	return retVal;
}

bool MACRx(uint32_t timeout) {
	Radio.Rx(0);
	RadioState = RX;
	startLoRaTimer(timeout);
	bool retVal = false;
	while (true) {
		if (RadioState == RXTIMEOUT) {
			retVal = false;
			break;
		} else if (RadioState == RXERROR) {
			retVal = false;
			break;
		} else if (RadioState == RXDONE) {
			if (processRXBuffer()) {
				retVal = true;
				break;
			} else {
				retVal = false;
				break;
			}
		}
	}
	stopLoRaTimer();
	return retVal;

}

static bool processRXBuffer() {
	memcpy(&rxdatagram, &RXBuffer, loraRxBufferSize);

	// check if message was meant for this node:
	if ((rxdatagram.macHeader.dest == _nodeID)
			|| (rxdatagram.macHeader.dest == BROADCAST_ADDRESS)) {// if destination is this node or broadcast check message type

		if (_prevMsgId != rxdatagram.macHeader.msgID) { // if a new message change prevMsgId to the id of the new message
			_prevMsgId = rxdatagram.macHeader.msgID;
		}

		switch (rxdatagram.macHeader.flags) {
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
			hasData = false;
			MACState = MAC_SLEEP;
			_numMsgSent++;
			break;
		case 0x06: 	// SYNC
		{
			Neighbour_t receivedNeighbour;
			receivedNeighbour.neighbourID = rxdatagram.macHeader.source;
			receivedNeighbour.neighbourTxSlot =
					rxdatagram.macHeader.sched.txSlot;
			neighbourTable[_numNeighbours++] = receivedNeighbour;
			MACState = MAC_SLEEP;

			if (rxdatagram.macHeader.source == _nodeID) {
				genID(true);
				schedChange = true;
			}
			break;
		}
		default:
			return false;
		}
		return true;
	} else {
		return false;
	}
}

