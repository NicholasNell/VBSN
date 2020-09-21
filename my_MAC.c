/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#include <bme280_defs.h>
#include <datagram.h>
#include <my_flash.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_MAC.h>
#include <my_RFM9x.h>
#include <my_timer.h>
#include <radio.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

// Datagram containing the data to be sent over the medium
Datagram_t txDatagram;

// Does this node have data to send?
volatile bool hasData = false;

// State of the MAC state machine
volatile MACappState_t MACState = MAC_LISTEN;

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
uint8_t neighbourTable[MAX_NEIGHBOURS];

// This node MAC parameters:
uint8_t _nodeID;
uint8_t _dataLen;
uint8_t _numNeighbours;
uint8_t _destID;
uint16_t _numMsgSent;

// The LoRa RX Buffer
extern uint8_t RXBuffer[MAX_MESSAGE_LEN];
Datagram_t rxdatagram;

// bme280Data
extern struct bme280_data bme280Data;

//light data
extern float lux;

// time
extern RTC_C_Calendar currentTime;

// current RadioState
LoRaRadioState_t RadioState;

// LoRa transmit buffer
uint8_t TXBuffer[MAX_MESSAGE_LEN];

//GPS Data
extern LocationData gpsData;

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

	_dataLen = 0;
	_numNeighbours = 0;
	_destID = BROADCAST_ADDRESS;
	_numMsgSent = 0;
}

void MACreadySend(uint8_t *dataToSend, uint8_t dataLen) {

}

bool MACStateMachine() {

	stateMachine();

	return true;
}

bool MACSend(uint8_t msgType, uint8_t dest) {

	if (dest != NULL) {
		txDatagram.header.dest = dest;
	} else {
		txDatagram.header.dest = BROADCAST_ADDRESS;
	}
	txDatagram.header.source = _nodeID;
	txDatagram.header.flags = msgType;
	txDatagram.header.msgID = (uint16_t) (_nodeID << 8)
			| (uint16_t) (_numMsgSent);
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
		txDatagram.data.lux = lux;
		txDatagram.data.gpsData = gpsData;
		txDatagram.data.soilMoisture = 100;
		currentTime = RTC_C_getCalendarTime();
		txDatagram.data.tim = currentTime;

		memcpy(TXBuffer, &txDatagram, sizeof(txDatagram));
		startLoRaTimer(1000);
		SX1276Send(TXBuffer, sizeof(txDatagram));
	} else {
		memcpy(TXBuffer, &txDatagram, sizeof(txDatagram.header));
		startLoRaTimer(1000);
		SX1276Send(TXBuffer, sizeof(txDatagram.header));
	}

	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			_numMsgSent++;
			return true;
		} else if (RadioState == TXTIMEOUT)
			return false;
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
	memcpy(&rxdatagram, &RXBuffer, loraRxBufferSize);

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
			break;
		case SYNC_MAC:
			break;
		case MAC_RTS:	// Send RTS
			break;
		case MAC_CTS:
			MACSend(0x2, BROADCAST_ADDRESS); // Send CTS
			break;
		case MAC_DATA: // send sensor data

			break;
		default:
			break;
		}
	}
}

