/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#include <bme280_defs.h>
#include <datagram.h>
#include <EC5.h>
#include <main.h>
#include <my_flash.h>
#include <my_gpio.h>
#include <my_gps.h>
#include <my_MAC.h>
#include <my_RFM9x.h>
#include <my_scheduler.h>
#include <my_timer.h>
#include <my_UART.h>
#include <myNet.h>
#include <MAX44009.h>
#include <radio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

// Array of carrier sense times
static uint32_t carrierSenseTimes[0xff];

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
//extern Gpio_t Led_rgb_blue;
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
uint16_t _numRTSMissed;

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

//Number of data messages received
int _numMsgReceived = 0;

NextNetOp_t nextNetOp = NET_NONE;

bool hopMessageFlag = false;
bool readyToUploadFlag = false;
bool waitForAck = false;

extern bool netOp;
extern bool flashOK;

uint16_t txSlots[WINDOW_SCALER];

/*!
 * @brief Send
 */
static void mac_send_lora_data();

/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool process_rx_buffer();

//! @param msgType Type of message to send: 	MSG_NONE = 0,	MSG_RTS,	MSG_CTS,	MSG_DATA,	MSG_ACK,	MSG_SYNC,	MSG_RREQ,	MSG_RREP
//! @param dest: local destination to send message to
//! @return Return true if send succesful else, false
static bool mac_send(msgType_t msgType, nodeAddress dest);

static bool mac_rx(uint32_t timeout);

static void gen_id(bool genNew);

static bool mac_hop_message(nodeAddress dest);

static void set_predefined_id(void);

static void set_predefined_id(void) {
	_nodeID = MAC_ID1;
//	_nodeID = MAC_ID2;
	_nodeID = MAC_ID3;
//	_nodeID = MAC_ID4;
}

static void set_predefined_tx(void);

static void set_predefined_tx(void) {
	_txSlot = MAC_TX1;
//	_txSlot = MAC_TX2;
	_txSlot = MAC_TX3;
//	_txSlot = MAC_TX4;
}

static void gen_id(bool genNew) {
	nodeAddress tempID = 0x0;

	if (genNew) {
		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS
				|| tempID == _nodeID) {
			tempID = (nodeAddress) rand();
		}
		_nodeID = tempID;
	} else {
		tempID = flash_read_node_id();

		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS) {
			tempID = (nodeAddress) rand();
		}
		_nodeID = tempID;
	}
	flash_write_node_id();

}

void mac_init() {
	uint8_t i;
	for (i = 0; i < 255; ++i) {
		{
			double temp = (double) rand();
			temp /= (double) RAND_MAX;
			temp *= SLOT_LENGTH_MS * 0.2;

			carrierSenseTimes[i] = (uint32_t) temp;
		}
	}

	if (flashOK) {
		FlashData_t *pFlashData = get_flash_data_struct();
		if (hasGSM) {
			_nodeID = GATEWAY_ADDRESS;
		} else {
//			_nodeID = pFlashData->thisNodeId;
			set_predefined_id();
		}

//		_txSlot = pFlashData->_txSlot;
		set_predefined_tx();

		_numNeighbours = pFlashData->_numNeighbours;
		memcpy(neighbourTable, pFlashData->neighbourTable,
				sizeof(pFlashData->neighbourTable));
	} else {
		if (hasGSM) {
			_nodeID = GATEWAY_ADDRESS;
		} else {
//			gen_id(true);
			set_predefined_id();
		}

		double temp = (double) rand();
		temp /= RAND_MAX;
		temp *= (WINDOW_TIME_SEC);

		if (temp < GSM_UPLOAD_DATAGRAMS_TIME) {
			_txSlot = (uint16_t) temp + GSM_UPLOAD_DATAGRAMS_TIME + 1; // this ensures that there is no txSlot in the GSM upload window
		} else {
			_txSlot = (uint16_t) temp;
		}
//		set_predefined_tx();
		if (hasGSM) {
			_txSlot = 0;
		}

		schedChange = true;
		_dataLen = 0;
		_numNeighbours = 0;
		_numMsgSent = 0;
		memset(neighbourTable, NULL, MAX_NEIGHBOURS);

#if DEBUG
		char msg[30];
		memset(msg, 0, 30);
		sprintf(msg, "Node ID: %3d| TxSlot: %4d\n", _nodeID, _txSlot);
		send_uart_pc(msg);
#endif
	}

	for (i = 0; i < WINDOW_SCALER; i++) {
		txSlots[i] = i * WINDOW_TIME_SEC + _txSlot;
	}

}

uint8_t numRetries = 0;

bool mac_state_machine() {
	static uint8_t carrierSenseSlot;
	RouteEntry_t route;
	while (true) {
		WDT_A_clearTimer();
		switch (MACState) {
		case MAC_SYNC_BROADCAST:
			send_uart_pc("MAC_SYNC_BROADCAST\n");
			if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])) {
				send_uart_pc("MAC_SYNC_BROADCAST: channel clear\n");
				if (mac_send(MSG_SYNC, BROADCAST_ADDRESS)) {
					send_uart_pc("MAC_SYNC_BROADCAST: sent RTS\n");
					schedChange = false;
					MACState = MAC_SLEEP;
//					return true;
				} else {
					send_uart_pc("MAC_SYNC_BROADCAST: sending RTS failed\n");
					MACState = MAC_SLEEP;
//					return false;
				}
			} else {
				send_uart_pc(
						"MAC_SYNC_BROADCAST: channel busy, prossessing RX message\n");
				MACState = MAC_SLEEP;
			}

			break;
		case MAC_LISTEN:
			send_uart_pc("MAC_LISTEN\n");
//				gpio_toggle(&Led_rgb_green); // toggle the green led if slot count was the same between the two messages
			if (!mac_rx(SLOT_LENGTH_MS)) {
				send_uart_pc("MAC_LISTEN: heard no message\n");
				MACState = MAC_SLEEP;
//				return false;
			}
			break;
		case MAC_SLEEP:
//			send_uart_pc("MAC_SLEEP\n");
			// SLEEP
			if (RadioState != RADIO_SLEEP) {
				send_uart_pc("MAC_SLEEP: set Sleep mode\n");
				SX1276SetSleep();
				RadioState = RADIO_SLEEP;
			}
			return false;
		case MAC_RTS:
			// Send RTS
			send_uart_pc("MAC_RTS\n");
//			numRetries++;
			if (has_route_to_node(GATEWAY_ADDRESS, &route)) {
				send_uart_pc("MAC_RTS: has route to dest\n");
//				if (SX1276IsChannelFree(MODEM_LORA,
//				RF_FREQUENCY,
//				LORA_RSSI_THRESHOLD, carrierSenseTimes[carrierSenseSlot++])) {
//					send_uart_pc("MAC_RTS: channel is clear\n");
				if (mac_send(MSG_DATA, route.next_hop)) { // Send RTS
					hopMessageFlag = false;
					waitForAck = true;
					send_uart_pc("MAC_RTS: sent RTS\n");
					if (!mac_rx(SLOT_LENGTH_MS)) {
						send_uart_pc("MAC_RTS: no CTS received\n");
						numRetries++;
						_numRTSMissed++;
						if (numRetries > 2) {
							remove_route_with_node(route.next_hop);
							remove_neighbour(route.next_hop);
						}
						// no response, send again in next slot

						MACState = MAC_SLEEP;
//						return false;
					}
				} else {
					send_uart_pc("MAC_RTS: send RTS failed\n");
					numRetries++;
					MACState = MAC_SLEEP;
//					return false;
				}
//				} else {
//					send_uart_pc("MAC_RTS: channel is busy\n");
//					numRetries++;
//					MACState = MAC_SLEEP;
////				return false;
//				}
			} else {
				numRetries++;
				_numRTSMissed++;
				send_uart_pc("MAC_RTS: no Route to dest\n");
				nextNetOp = NET_BROADCAST_RREQ;
				MACState = MAC_NET_OP;

			}
			MACState = MAC_SLEEP;
			break;
		case MAC_NET_OP:
			switch (nextNetOp) {
			case NET_NONE:
				send_uart_pc("NET_NONE\n");
				MACState = MAC_LISTEN;
				break;
			case NET_REBROADCAST_RREQ:
				send_uart_pc("NET_REBROADCAST_RREQ\n");
				if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])/*SX1276IsChannelFree(MODEM_LORA,
				 RF_FREQUENCY, LORA_RSSI_THRESHOLD,
				 carrierSenseTimes[carrierSenseSlot++])*/) {
					send_uart_pc("NET_REBROADCAST_RREQ: channel clear\n");
					if (net_re_rreq()) {
						send_uart_pc(
								"NET_REBROADCAST_RREQ: succesfully rebroadcast RREQ\n");
						nextNetOp = NET_WAIT;
					} else {
						send_uart_pc("NET_REBROADCAST_RREQ: failed re-RREQ\n");
						nextNetOp = NET_REBROADCAST_RREQ;
					}
				} else {
					send_uart_pc("NET_REBROADCAST_RREQ: channel busy\n");
					MACState = MAC_SLEEP;
				}
				break;
			case NET_BROADCAST_RREQ:
				send_uart_pc("NET_BROADCAST_RREQ\n");
				if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])/*SX1276IsChannelFree(MODEM_LORA,
				 RF_FREQUENCY, LORA_RSSI_THRESHOLD,
				 carrierSenseTimes[carrierSenseSlot++])*/) {
					send_uart_pc("NET_BROADCAST_RREQ: channel clear\n");
					if (send_rreq()) {
						send_uart_pc("NET_BROADCAST_RREQ: sent RREQ\n");
						nextNetOp = NET_WAIT;
					} else {
						send_uart_pc("NET_BROADCAST_RREQ: RREQ failed\n");
						nextNetOp = NET_BROADCAST_RREQ;
					}
				} else {
					send_uart_pc("NET_BROADCAST_RREQ: channel busy\n");
					MACState = MAC_SLEEP;
				}
				break;
			case NET_UNICAST_RREP:
				send_uart_pc("NET_UNICAST_RREP\n");

				send_rrep();
				nextNetOp = NET_NONE;
				MACState = MAC_SLEEP;
				break;
			case NET_WAIT:
				send_uart_pc("NET_WAIT\n");
				if (!mac_rx(SLOT_LENGTH_MS)) {
					send_uart_pc("NET_WAIT: heard no message\n");
					MACState = MAC_SLEEP;
				}
				break;
			default:
				MACState = MAC_SLEEP;
				break;
			}
			break;
		case MAC_WAIT:
			break;
//			return true;
		case MAC_HOP_MESSAGE:
			send_uart_pc("MAC_HOP_MESSAGE\n");
			if (has_route_to_node(GATEWAY_ADDRESS, &route)) {
				send_uart_pc("MAC_HOP_MESSAGE: has route to dest\n");
				if (SX1276IsChannelFree(MODEM_LORA,
				RF_FREQUENCY, LORA_RSSI_THRESHOLD,
						carrierSenseTimes[carrierSenseSlot++])) {
					send_uart_pc("MAC_HOP_MESSAGE: channel clear\n");

					if (mac_send(MSG_RTS, route.next_hop)) { // Send RTS
						send_uart_pc("MAC_HOP_MESSAGE: sent Hop\n");

						if (!mac_rx(SLOT_LENGTH_MS)) {
							numRetries++;
							if (numRetries > 2) {
								remove_route_with_node(route.next_hop);
								remove_neighbour(route.next_hop);
							}
							send_uart_pc("MAC_HOP_MESSAGE: heard no ACK\n");
							MACState = MAC_SLEEP;
							return false;
						}
					} else {
						hopMessageFlag = false;
						MACState = MAC_SLEEP;
						//					return false;
					}
				} else {
					send_uart_pc("MAC_HOP_MESSAGE: channel not free\n");
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

extern uint8_t _numRoutes;

void build_tx_datagram(void) {
	RouteEntry_t route;
	has_route_to_node(GATEWAY_ADDRESS, &route);
	txDatagram.msgHeader.nextHop = route.next_hop;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.netDest = route.dest;
	txDatagram.msgHeader.hopCount = 0;
	txDatagram.msgHeader.flags = MSG_DATA;
	txDatagram.msgHeader.txSlot = _txSlot;
	txDatagram.msgHeader.curSlot = get_slot_count() + 5;

	txDatagram.data.sensData.hum = bme280Data.humidity;
	txDatagram.data.sensData.press = bme280Data.pressure;
	txDatagram.data.sensData.temp = bme280Data.temperature;
	txDatagram.data.sensData.lux = get_lux();
	txDatagram.data.sensData.gpsData = gpsData;
	txDatagram.data.sensData.soilMoisture = get_vwc();
	txDatagram.data.sensData.tim = timeStamp;
	txDatagram.netData.numDataSent = _numMsgSent;

	txDatagram.netData.numNeighbours = _numNeighbours;
	txDatagram.netData.numRoutes = _numRoutes;
	txDatagram.netData.rtsMissed = _numRTSMissed;
	int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.data.sensData)
			+ sizeof(txDatagram.netData);
	memcpy(TXBuffer, &txDatagram, size);
}

static void mac_send_lora_data() {
	int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.data.sensData)
			+ sizeof(txDatagram.netData);
	start_lora_timer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);
}

bool mac_send(msgType_t msgType, nodeAddress dest) {

	txDatagram.msgHeader.nextHop = dest;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.netDest = dest;
	txDatagram.msgHeader.hopCount = 0;
	txDatagram.msgHeader.flags = msgType;

	txDatagram.msgHeader.txSlot = _txSlot;
	txDatagram.msgHeader.curSlot = get_slot_count();

	if (msgType == MSG_DATA) {
		txDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
		txDatagram.data.sensData.hum = bme280Data.humidity;
		txDatagram.data.sensData.press = bme280Data.pressure;
		txDatagram.data.sensData.temp = bme280Data.temperature;
		txDatagram.data.sensData.lux = get_lux();
		txDatagram.data.sensData.gpsData = gpsData;
		txDatagram.data.sensData.soilMoisture = get_vwc();
		txDatagram.data.sensData.tim = timeStamp;

		_numMsgSent++;
		txDatagram.netData.numDataSent = _numMsgSent;

		txDatagram.netData.numNeighbours = _numNeighbours;
		txDatagram.netData.numRoutes = _numRoutes;
		txDatagram.netData.rtsMissed = _numRTSMissed;
		int size = sizeof(txDatagram.msgHeader)
				+ sizeof(txDatagram.data.sensData) + sizeof(txDatagram.netData);
		memcpy(TXBuffer, &txDatagram, size);
		start_lora_timer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	} else {
		int size = sizeof(txDatagram.msgHeader);
		memcpy(TXBuffer, &txDatagram, size);
		start_lora_timer(loraTxtimes[size + 1]);
		SX1276Send(TXBuffer, size);
	}

	RadioState = TX;
	bool retVal = false;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		} else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stop_lora_timer();
	return retVal;
}

bool mac_rx(uint32_t timeout) {
	Radio.Rx(0);
	RadioState = RX;
	start_lora_timer(timeout);
	bool retVal = false;
	while (true) {
		if (RadioState == RXTIMEOUT) {
			retVal = false;
			break;
		} else if (RadioState == RXERROR) {
			MACState = MAC_SLEEP;
			retVal = false;
			break;
		} else if (RadioState == RXDONE) {
			if (process_rx_buffer()) {
				retVal = true;
				break;
			} else {
				retVal = false;
				break;
			}
		}
	}
	stop_lora_timer();
	return retVal;
}

extern volatile int8_t RssiValue;
extern volatile int8_t SnrValue;
static bool process_rx_buffer() {

	memcpy(&rxDatagram, &RXBuffer, loraRxBufferSize);

	if (receivedMsgIndex == MAX_STORED_MSG) {
		receivedMsgIndex = 0;
	}

	add_neighbour(rxDatagram.msgHeader.localSource,
			rxDatagram.msgHeader.txSlot);
	add_route_to_neighbour(rxDatagram.msgHeader.localSource);

	if (rxDatagram.msgHeader.localSource == _nodeID) { // change ID because another node has the same ID
		gen_id(true);
		schedChange = true;
	}
//	receivedDatagrams[receivedMsgIndex] = rxDatagram;

	// Comapare slotr counts of the two messages, if different, change this slot Count to the one from the received message
//	if (rxDatagram.msgHeader.curSlot != get_slot_count()) {
//		set_slot_count(rxDatagram.msgHeader.curSlot);
//	}
//	else {
//
//	}

	// check if message was meant for this node:
	if ((rxDatagram.msgHeader.nextHop == _nodeID)
			|| (rxDatagram.msgHeader.nextHop == BROADCAST_ADDRESS)) { // if destination is this node or broadcast check message type

		switch (rxDatagram.msgHeader.flags) {
		case MSG_RTS: 	// RTS
			mac_send(MSG_CTS, rxDatagram.msgHeader.localSource); // send CTS to requesting node
			MACState = MAC_LISTEN;
			break;
		case MSG_CTS: 	// CTS
			send_uart_pc("CTS Msg Received\n");
			if (hopMessageFlag) {
				RouteEntry_t route;
				has_route_to_node(GATEWAY_ADDRESS, &route);
				mac_hop_message(route.next_hop);
			} else {
				mac_send(MSG_DATA, rxDatagram.msgHeader.localSource); // Send data to destination node
//				hasData = true;
				MACState = MAC_LISTEN;
			}

			break;
		case MSG_DATA: 	// DATA
			if (rxDatagram.msgHeader.netDest != _nodeID) {
				hopMessageFlag = true;
			} else {
				hopMessageFlag = false;
				// now either store the data for further later sending or get ready to upload data to cloud
				rxDatagram.radioData.rssi = RssiValue;
				rxDatagram.radioData.snr = SnrValue;
				receivedDatagrams[receivedMsgIndex++] = rxDatagram;
				readyToUploadFlag = true;
				_numMsgReceived++;
			}

			mac_send(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
			if (!hopMessageFlag) {
				MACState = MAC_SLEEP;
			} else {
				MACState = MAC_HOP_MESSAGE;
			}

			break;
		case MSG_ACK: 	// ACK
			if (hopMessageFlag) {
				hopMessageFlag = false;
			}
			if (waitForAck) {
				waitForAck = false;
			}
			numRetries = 0;
			nextNetOp = NET_NONE;
			MACState = MAC_SLEEP;
			break;
		case MSG_RREP: 	// RREP
			send_uart_pc("Rx RREP\n");
			mac_send(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
			nextNetOp = process_rrep();
			MACState = MAC_NET_OP;
			netOp = true;
			break;

			/* SYNC and RREQ messages are broadcast, thus no ack, only RRep if route is known */
		case MSG_SYNC: 	// SYNC
		{
//			mac_send(MSG_ACK, rxDatagram.msgHeader.localSource);

			MACState = MAC_SLEEP;
			return true;
		}
		case MSG_RREQ: 	// RREQ
			// when a rreq is received, determine what actions to do next.
			nextNetOp = process_rreq();
			MACState = MAC_NET_OP;
			netOp = true;
			break;
		default:
			return false;
		}
		return true;
	} else {
		MACState = MAC_SLEEP;
		return false;
	}
}

void add_neighbour(nodeAddress neighbour, uint16_t txSlot) {
	Neighbour_t receivedNeighbour;
	if (is_neighbour(neighbour)) {
		return;
	}
	receivedNeighbour.neighbourID = neighbour;
	receivedNeighbour.neighbourTxSlot = txSlot;
	neighbourTable[_numNeighbours++] = receivedNeighbour;
}

bool is_neighbour(nodeAddress node) {
	bool retVal = false;
	int i;
	for (i = 0; i < _numNeighbours; ++i) {
		if (node == neighbourTable[i].neighbourID) {
			retVal = true;
		}
	}
	return retVal;
}

bool mac_send_tx_datagram(int size) {
	bool retVal = false;
	memcpy(TXBuffer, &txDatagram, size);
	start_lora_timer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);

	RadioState = TX;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		} else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stop_lora_timer();
	return retVal;
}

static bool mac_hop_message(nodeAddress dest) {

	memcpy(&txDatagram, &rxDatagram, loraRxBufferSize);
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
	txDatagram.msgHeader.nextHop = dest;
	txDatagram.msgHeader.txSlot = _txSlot;
	txDatagram.msgHeader.hopCount = txDatagram.msgHeader.hopCount + 1;
	txDatagram.msgHeader.netSource = rxDatagram.msgHeader.netSource;

	txDatagram.netData.numDataSent = _numMsgSent;
	_numMsgSent++;
	txDatagram.netData.numNeighbours = _numNeighbours;
	txDatagram.netData.numRoutes = _numRoutes;
	txDatagram.netData.rtsMissed = _numRTSMissed;
	int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.netData);
	memcpy(TXBuffer, &txDatagram, size);
	start_lora_timer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);

	RadioState = TX;
	bool retVal = false;
	while (true) {
		if (RadioState == TXDONE) {
			retVal = true;
			break;
		} else if (RadioState == TXTIMEOUT) {
			retVal = false;
			break;
		}
	}
	stop_lora_timer();
	return retVal;
}

void remove_neighbour(nodeAddress neighbour) {
	int i;
	for (i = 0; i < _numNeighbours; i++) {
		if (neighbourTable[i].neighbourID == neighbour) {
			if (i == MAX_NEIGHBOURS - 1) {
				_numNeighbours--;
			} else {
				int j;
				for (j = i; j < MAX_NEIGHBOURS - 1; j++) {
					neighbourTable[j] = neighbourTable[j + 1];
				}
				_numNeighbours--;
			}

		}
	}
}

Neighbour_t* get_neighbour_table() {
	return neighbourTable;
}

Datagram_t* get_received_messages() {
	return receivedDatagrams;
}

uint8_t get_received_messages_index(void) {
	return receivedMsgIndex;
}

void reset_received_msg_index(void) {
	receivedMsgIndex = 0;
}

void decrease_received_message_index(void) {
	receivedMsgIndex--;
}
