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
#include <my_scheduler.h>
#include <my_timer.h>
#include <MAX44009.h>
#include <radio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>
#include <utilities.h>

// Array of carrier sense times
static uint8_t carrierSenseTimes[0xff];

// Datagram containing the data to be sent over the medium
Datagram_t txDatagram;

// Does this node have data to send?
volatile bool hasData = false;

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
uint8_t neighbourTable[MAX_NEIGHBOURS];

uint16_t neighbourSyncSlots[MAX_NEIGHBOURS];

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

// current RadioState
LoRaRadioState_t RadioState;

// LoRa transmit buffer
uint8_t TXBuffer[MAX_MESSAGE_LEN];

//GPS Data
extern LocationData gpsData;

/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer();

static bool MACSend(uint8_t msgType, uint8_t dest);

static bool MACRx(uint32_t timeout);

//static void syncSchedule();
static bool stateMachine();

void MacInit() {
	uint8_t tempID;
	uint8_t i;
	for (i = 0; i < 255; ++i) {
//		if (carrierSenseTimes[i] == 0) {
		carrierSenseTimes[i] = (uint8_t) rand();
//		}
	}
	tempID = flashReadNodeID();
	srand(SX1276Random8Bit());

	if (tempID == 0xFF || tempID == 0x00) {
		tempID = rand();
		_nodeID = tempID;
		flashWriteNodeID();
	} else {
		_nodeID = tempID;
	}

	_txSlot = (uint16_t) (((double) SX1276Random() / (uint32_t) (0xFFFFFFFF))
			* MAX_SLOT_COUNT);
	_txSlot /= 10;
	_txSlot *= 10;
	_dataLen = 0;
	_numNeighbours = 0;
	_destID = BROADCAST_ADDRESS;
	_numMsgSent = 0;
	memset(neighbourSyncSlots, NULL, MAX_NEIGHBOURS);
	memset(neighbourTable, NULL, MAX_NEIGHBOURS);
}

void MACreadySend(uint8_t *dataToSend, uint8_t dataLen) {

}

bool MACStateMachine() {
	stateMachine();
	return true;
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
		txDatagram.data.tim = RTC_C_getCalendarTime();

		memcpy(TXBuffer, &txDatagram, sizeof(txDatagram));
		startLoRaTimer(1000);
		SX1276Send(TXBuffer, sizeof(txDatagram));
	} else {
		memcpy(TXBuffer, &txDatagram, sizeof(txDatagram.macHeader));
		startLoRaTimer(1000);
		SX1276Send(TXBuffer, sizeof(txDatagram.macHeader));
	}

	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
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
			if (processRXBuffer()) {
				return true;
			} else {
				return false;
			}
		}
	}
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
			neighbourTable[_numNeighbours++] = rxdatagram.macHeader.source;
			neighbourSyncSlots[_numNeighbours] =
					rxdatagram.macHeader.sched.rxSlot;

			break;
		default:
			return false;
		}
		return true;
	} else {	// if message was not meant for this node start listening again
		MACState = MAC_LISTEN;
		return true;
	}
}

static bool stateMachine() {
	while (true) {
		if (MACState == NODE_DISC) {
			MACRx(1000);
		} else {
			switch (MACState) {
			case MAC_LISTEN:
				if (!MACRx(30000)) {// If no message received in 30sec go back to sleep
					SX1276SetSleep();
					RadioState = RADIO_SLEEP;
					MACState = MAC_SLEEP;
					return false;
				}
				break;

				/* Every node can only tx in its syncslot and the syncslots of it's neighbours (which if everything works, will be the same).
				 * Rx slots the same for every node (e.g. every 10 slots), so nodes always listen in rx slots for external commands. Or additional neighbours.
				 *
				 * 	Node Discovery SuedoCode
				 * Generate own syncSlot.
				 * Listen until own sync slot occurs.
				 * if rx sync msg:
				 * 	set sync slot to received sync slot and update neighbour table.
				 * 	continue rx until end of random time.
				 * 	added addition sync slots to array of sync slots and update neighbour table.
				 * else if timeout
				 * 	tx own sync slot.
				 * 	sleep.
				 *
				 * write sync slot and neighbours to flash
				 *
				 */
			case MAC_SLEEP:	// SLEEP
				if (RadioState != RADIO_SLEEP) {
					SX1276SetSleep();
					RadioState = RADIO_SLEEP;
				}
				return false;
			case SYNC_MAC:
				return false;
			case MAC_RTS:	// Send RTS
//			net_getNextDest(); // get the next hop destination from the network layer
				if (MACSend(0x1, BROADCAST_ADDRESS)) {	// Send RTS
					if (!MACRx(1000)) {
						MACState = MAC_SLEEP;
						return false;
					}
				} else {
					MACState = MAC_SLEEP;
					return false;
				}
				break;
			case MAC_CTS:
				if (MACSend(0x2, BROADCAST_ADDRESS)) {	// Send CTS
					if (!MACRx(1000)) {
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
				if (MACSend(0x4, BROADCAST_ADDRESS)) {	// Send DATA
					if (!MACRx(1000)) {
						MACState = MAC_SLEEP;
						return false;
					} else {
						return true;
					}
				} else {
					MACState = MAC_SLEEP;
					return false;
				}
			case MAC_ACK:
				if (MACSend(0x5, BROADCAST_ADDRESS)) { // Send ACK
					MACState = MAC_SLEEP;
					return true;
				} else {
					MACState = MAC_SLEEP;
					return false;
				}
			default:
				return false;
			}
		}
	}
}

