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
#include <myNet.h>
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

// Datagram from received message
Datagram_t rxDatagram;

Datagram_t receivedDatagrams[MAX_STORED_MSG];
uint8_t receivedMsgIndex = 0;

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
nodeAddress _nodeID;
uint8_t _dataLen;
uint8_t _numNeighbours;
uint16_t _numMsgSent;
uint16_t _prevMsgId;
uint16_t _txSlot;
uint16_t _schedID;

// The LoRa RX Buffer
extern uint8_t RXBuffer[MAX_MESSAGE_LEN];

// should mac init
extern bool macFlag;

// bme280Data
extern struct bme280_data bme280Data;

// soil moisture data
extern float soilMoisture;

// schedChange
extern bool schedChange;

// has GSM module, if it has, is a root
extern bool hasGSM;

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

extern uint8_t _nodeSequenceNumber;
extern uint8_t _broadcastID;
extern uint8_t _destSequenceNumber;

static NextNetOp_t nextNetOp = NET_NONE;

bool hopMessageFlag = false;

extern bool netOp;

/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool processRXBuffer( );

static bool MACSend( msgType_t msgType, nodeAddress dest );

static bool MACRx( uint32_t timeout );

static void genID( bool genNew );

static bool MAChopMessage( nodeAddress dest );

static void genID( bool genNew ) {
	nodeAddress tempID = 0x0;

	if (genNew) {
		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS
				|| tempID == _nodeID) {
			tempID = (nodeAddress) rand();
		}
		_nodeID = tempID;
	}
	else {
		tempID = flashReadNodeID();

		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS) {
			tempID = (nodeAddress) rand();
		}
		_nodeID = tempID;
	}
	flashWriteNodeID();
}

void MacInit( ) {
	uint8_t i;
	for (i = 0; i < 255; ++i) {
		{
			double temp = (double) rand();
			temp /= (double) RAND_MAX;
			temp *= SLOT_LENGTH_MS * 0.2;

			carrierSenseTimes[i] = (uint32_t) temp;
		}
	}

	if (hasGSM) {
		_nodeID = GATEWAY_ADDRESS;
	}
	else {
		genID(false);
	}

	do {
		double temp = (double) rand();
		temp /= RAND_MAX;
		temp *= MAX_SLOT_COUNT;
		_txSlot = (uint16_t) temp / POSSIBLE_TX_SLOT;
		_txSlot *= POSSIBLE_TX_SLOT;
	} while (_txSlot % GLOBAL_RX == 0);

	schedChange = true;
	_dataLen = 0;
	_numNeighbours = 0;
	_numMsgSent = 0;
	memset(neighbourTable, NULL, MAX_NEIGHBOURS);
}

bool MACStateMachine( ) {
	RouteEntry_t *route = NULL;
	while (true) {
		switch (MACState) {
			case MAC_SYNC_BROADCAST:
				if (!MACRx(carrierSenseTimes[carrierSenseSlot++])) {
					if (MACSend(MSG_SYNC, BROADCAST_ADDRESS)) {
						schedChange = false;
						MACState = MAC_SLEEP;
//					return true;
					}
					else {
						MACState = MAC_SLEEP;
//					return false;
					}
				}
				MACState = MAC_SLEEP;
				break;
			case MAC_LISTEN:
				if (!MACRx(SLOT_LENGTH_MS)) {
					MACState = MAC_SLEEP;
//				return false;
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

				if (hasRouteToNode(GATEWAY_ADDRESS, route)) {
					if (SX1276IsChannelFree(MODEM_LORA,
					RF_FREQUENCY, -60, carrierSenseTimes[carrierSenseSlot++])) {

						if (MACSend(MSG_RTS, route->next_hop)) { // Send RTS

							if (!MACRx(SLOT_LENGTH_MS)) {
								MACState = MAC_SLEEP;
//						return false;
							}
						}
						else {
							MACState = MAC_SLEEP;
//					return false;
						}
					}
					else {
						MACState = MAC_SLEEP;
//				return false;
					}
				}
				break;
			case MAC_NET_OP:
				switch (nextNetOp) {
					case NET_NONE:
						MACState = MAC_LISTEN;
						break;
					case NET_REBROADCAST_RREQ:
						if (SX1276IsChannelFree(
								MODEM_LORA,
								RF_FREQUENCY,
								-60,
								carrierSenseTimes[carrierSenseSlot++])) {
							if (netReRReq()) {
								nextNetOp = NET_WAIT;
							}
							else {
								nextNetOp = NET_REBROADCAST_RREQ;
							}
						}
						break;
					case NET_BROADCAST_RREQ:
						if (SX1276IsChannelFree(
								MODEM_LORA,
								RF_FREQUENCY,
								-60,
								carrierSenseTimes[carrierSenseSlot++])) {
							if (sendRREQ()) {
								nextNetOp = NET_WAIT;
							}
							else {
								nextNetOp = NET_BROADCAST_RREQ;
							}
						}
						break;
					case NET_UNICAST_RREP:
						break;
					case NET_WAIT:
						if (!MACRx(SLOT_LENGTH_MS)) {
							MACState = MAC_SLEEP;
						}
						break;
					default:
						break;
				}
				break;
			case MAC_WAIT:
				break;
//			return true;
			case MAC_HOP_MESSAGE:
				if (hasRouteToNode(GATEWAY_ADDRESS, route)) {
					if (SX1276IsChannelFree(MODEM_LORA,
					RF_FREQUENCY, -60, carrierSenseTimes[carrierSenseSlot++])) {

						if (MAChopMessage(route->next_hop)) { // Send RTS

							if (!MACRx(SLOT_LENGTH_MS)) {
								MACState = MAC_SLEEP;
								//						return false;
							}
						}
						else {
							MACState = MAC_SLEEP;
							//					return false;
						}
					}
					else {
						MACState = MAC_SLEEP;
						//				return false;
					}
				}
				break;

			default:
				return false;
		}
//		return false;
	}
}

bool MACSend( msgType_t msgType, nodeAddress dest ) {

	txDatagram.msgHeader.nextHop = dest;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.flags = msgType;
	_numMsgSent++;
	txDatagram.msgHeader.txSlot = _txSlot;

	if (msgType == MSG_DATA) {
		txDatagram.data.sensData.hum = bme280Data.humidity;
		txDatagram.data.sensData.press = bme280Data.pressure;
		txDatagram.data.sensData.temp = bme280Data.temperature;
		txDatagram.data.sensData.lux = getLux();
		txDatagram.data.sensData.gpsData = gpsData;
		txDatagram.data.sensData.soilMoisture = soilMoisture;
		txDatagram.data.sensData.tim = timeStamp;

		int size = sizeof(txDatagram);
		memcpy(TXBuffer, &txDatagram, size);
		startLoRaTimer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	}
	else {
		int size = sizeof(txDatagram.msgHeader);
		memcpy(TXBuffer, &txDatagram, size);
		startLoRaTimer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	}

	RadioState = TX;
	bool retVal = false;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		}
		else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stopLoRaTimer();
	return retVal;
}

bool MACRx( uint32_t timeout ) {
	Radio.Rx(0);
	RadioState = RX;
	startLoRaTimer(timeout);
	bool retVal = false;
	while (true) {
		if (RadioState == RXTIMEOUT) {
			retVal = false;
			break;
		}
		else if (RadioState == RXERROR) {
			retVal = false;
			break;
		}
		else if (RadioState == RXDONE) {
			if (processRXBuffer()) {
				retVal = true;
				break;
			}
			else {
				retVal = false;
				break;
			}
		}
	}
	stopLoRaTimer();
	return retVal;
}

static bool processRXBuffer( ) {
	memcpy(&rxDatagram, &RXBuffer, loraRxBufferSize);

	if (receivedMsgIndex == MAX_STORED_MSG) {
		receivedMsgIndex = 0;
	}
	receivedDatagrams[receivedMsgIndex] = rxDatagram;

	// check if message was meant for this node:
	if ((rxDatagram.msgHeader.nextHop == _nodeID)
			|| (rxDatagram.msgHeader.nextHop == BROADCAST_ADDRESS)) { // if destination is this node or broadcast check message type

		switch (rxDatagram.msgHeader.flags) {
			case MSG_RTS: 	// RTS
				MACSend(MSG_CTS, rxDatagram.msgHeader.localSource); // send CTS to requesting node
				MACState = MAC_LISTEN;
				break;
			case MSG_CTS: 	// CTS
				if (hasData) {
					MACSend(MSG_DATA, rxDatagram.msgHeader.localSource); // Send data to destination node
					MACState = MAC_LISTEN;
				}
				else {
					MACState = MAC_SLEEP;
				}
				break;
			case MSG_DATA: 	// DATA
				if (rxDatagram.msgHeader.netDest != _nodeID) {
					hopMessageFlag = true;
				}
				else {
					hopMessageFlag = false;
					// now either store the data for further later sending or get ready to upload data to cloud
				}

				MACSend(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
				if (!hopMessageFlag) {
					MACState = MAC_SLEEP;
				}
				else {
					MACState = MAC_RTS;
				}

				break;
			case MSG_ACK: 	// ACK
				hasData = false;	// data has succesfully been sent
				MACState = MAC_SLEEP;
				break;
			case MSG_RREP: 	// RREP
				MACSend(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
				nextNetOp = processRrep();
				MACState = MAC_NET_OP;
				netOp = true;
				break;

				/* SYNC and RREQ messages are broadcast, thus no ack, only RRep if route is known */
			case MSG_SYNC: 	// SYNC
			{
				addNeighbour(
						rxDatagram.msgHeader.localSource,
						rxDatagram.msgHeader.txSlot);

				MACState = MAC_SLEEP;

				if (rxDatagram.msgHeader.localSource == _nodeID) { // change ID because another node has the same ID
					genID(true);
					schedChange = true;
				}
				addRoutetoNeighbour(rxDatagram.msgHeader.localSource);
				break;
			}
			case MSG_RREQ: 	// RREQ
				// when a rreq is received, determine what actions to do next.
				nextNetOp = processRreq();
				MACState = MAC_NET_OP;
				netOp = true;
				break;
			default:
				return false;
		}
		return true;
	}
	else {
		MACState = MAC_SLEEP;
		return false;
	}
}

void addNeighbour( nodeAddress neighbour, uint16_t txSlot ) {
	Neighbour_t receivedNeighbour;
	receivedNeighbour.neighbourID = neighbour;
	receivedNeighbour.neighbourTxSlot = txSlot;
	neighbourTable[_numNeighbours++] = receivedNeighbour;
}

bool MACStartTransaction( nodeAddress destination, msgType_t msgType,
bool isSource ) {
	/*
	 *	nodeAddress nextHop; // Where the message needs to go (MAC LAYER, not final destination);
	 * 	nodeAddress localSource; // Where the message came from, not original source
	 * 	nodeAddress netSource;	// original message source
	 * 	nodeAddress netDest;	// final destination, probably gateway
	 * 	uint8_t hops;			// number of message hops from source to here
	 * 	uint8_t ttl;			// maximum number of hops
	 * 	uint16_t txSlot; // txSlot
	 * 	uint8_t flags;	 // see flags
	 */
	rxDatagram.msgHeader.nextHop = destination;
	rxDatagram.msgHeader.localSource = _nodeID;
	if (isSource) {
		rxDatagram.msgHeader.netSource = _nodeID;
	}
	else {

	}
//	rxdatagram.msgHeader.netDest =
	rxDatagram.msgHeader.ttl = MAX_HOPS;

	rxDatagram.msgHeader.txSlot = _txSlot;
	rxDatagram.msgHeader.flags = msgType;

	switch (msgType) {
		case MSG_SYNC:
			rxDatagram.msgHeader.ttl = 1;
			rxDatagram.msgHeader.netDest = BROADCAST_ADDRESS;
			rxDatagram.msgHeader.nextHop = BROADCAST_ADDRESS;
			break;
		case MSG_DATA:
			rxDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
			rxDatagram.msgHeader.nextHop = getDest(GATEWAY_ADDRESS);

			txDatagram.data.sensData.hum = bme280Data.humidity;
			txDatagram.data.sensData.press = bme280Data.pressure;
			txDatagram.data.sensData.temp = bme280Data.temperature;
			txDatagram.data.sensData.lux = getLux();
			txDatagram.data.sensData.gpsData = gpsData;
			txDatagram.data.sensData.soilMoisture = soilMoisture;
			txDatagram.data.sensData.tim = timeStamp;
			break;
		case MSG_RREQ:
			/*
			 nodeAddress source_addr;
			 uint8_t source_sequence_num;
			 uint8_t broadcast_id;
			 nodeAddress dest_addr;	// destination address: will probably be a gateway
			 uint8_t dest_sequence_num;	// destination sequence number
			 uint8_t hop_cnt;	// number of hops the message has gone through
			 */
			txDatagram.data.Rreq.source_addr = _nodeID;
			txDatagram.data.Rreq.source_sequence_num = _nodeSequenceNumber;
			txDatagram.data.Rreq.broadcast_id = _broadcastID;
			txDatagram.data.Rreq.dest_addr = GATEWAY_ADDRESS;
			txDatagram.data.Rreq.dest_sequence_num = _destSequenceNumber;
			txDatagram.data.Rreq.hop_cnt = 0;
			break;
		case MSG_RREP:
			/*
			 nodeAddress source_addr;	// source address, will be a sensor node
			 nodeAddress dest_addr;	// destination address: will probably be the gateway
			 uint8_t dest_sequence_num;	// destination sequence number
			 uint8_t hop_cnt;	// number of hops until now
			 uint8_t lifetime;	// TTL?
			 */
			txDatagram.data.Rrep.source_addr = _nodeID;
			break;
		default:
			break;
	}
	return 0;
}

bool isNeighbour( nodeAddress node ) {
	bool retVal = false;
	int i;
	for (i = 0; i < _numNeighbours; ++i) {
		if (node == neighbourTable[i].neighbourID) {
			retVal = true;
		}
	}
	return retVal;
}

bool MACSendTxDatagram( ) {
	bool retVal = false;
	int size = sizeof(txDatagram);
	memcpy(TXBuffer, &txDatagram, size);
	startLoRaTimer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);

	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		}
		else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stopLoRaTimer();
	return retVal;
}

static bool MAChopMessage( nodeAddress dest ) {

	memcpy(&txDatagram, &rxDatagram, loraRxBufferSize);
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
	txDatagram.msgHeader.nextHop = dest;
	txDatagram.msgHeader.txSlot = _txSlot;
	int size = sizeof(txDatagram);
	memcpy(TXBuffer, &txDatagram, size);
	startLoRaTimer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);

	RadioState = TX;
	bool retVal = false;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		}
		else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stopLoRaTimer();
	return retVal;
}

Neighbour_t* getNeighbourTable( ) {
	return neighbourTable;
}

Datagram_t* getReceivedMessages( ) {
	return receivedDatagrams;
}
