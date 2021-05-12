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
bool readyToUploadFlag = false;

extern bool netOp;
extern bool flashOK;

uint16_t txSlots[WINDOW_SCALER];

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
	memset(&receivedDatagrams, 0x00, sizeof(receivedDatagrams));
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
			_nodeID = pFlashData->thisNodeId;
		}

		_txSlot = pFlashData->_txSlot;

		_numNeighbours = pFlashData->_numNeighbours;
		memcpy(neighbourTable, pFlashData->neighbourTable,
				sizeof(pFlashData->neighbourTable));
	} else {
		if (hasGSM) {
			_nodeID = GATEWAY_ADDRESS;
		} else {
			gen_id(false);
		}

		double temp = (double) rand();
		temp /= RAND_MAX;
		temp *= (WINDOW_TIME_SEC);

		if (temp < GSM_UPLOAD_DATAGRAMS_TIME) {
			_txSlot = (uint16_t) temp + GSM_UPLOAD_DATAGRAMS_TIME + 1; // this ensures that there is no txSlot in the GSM upload window
		} else {
			_txSlot = (uint16_t) temp;
		}
		int i = 0;

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
	RouteEntry_t *route = NULL;
	while (true) {
		WDT_A_clearTimer();
		switch (MACState) {
		case MAC_SYNC_BROADCAST:
			send_uart_pc("Sending Sync Broadcast\n");
			if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])) {
				if (mac_send(MSG_SYNC, BROADCAST_ADDRESS)) {
					schedChange = false;
					MACState = MAC_SLEEP;
//					return true;
				} else {
					MACState = MAC_SLEEP;
//					return false;
				}
			}
			break;
		case MAC_LISTEN:
//				gpio_toggle(&Led_rgb_green); // toggle the green led if slot count was the same between the two messages
			if (!mac_rx(SLOT_LENGTH_MS)) {
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
			send_uart_pc("Sending RTS\n");
//			numRetries++;
			if (has_route_to_node(GATEWAY_ADDRESS, route)) {
				send_uart_pc("Node Has a Route to the destination\n");
				if (SX1276IsChannelFree(MODEM_LORA,
				RF_FREQUENCY,
				LORA_RSSI_THRESHOLD, carrierSenseTimes[carrierSenseSlot++])) {

					if (mac_send(MSG_RTS, route->next_hop)) { // Send RTS

						if (!mac_rx(SLOT_LENGTH_MS)) {
							send_uart_pc("No CTS msg Received\n");
							numRetries++;
							// no response, send again in next slot

							MACState = MAC_SLEEP;
//						return false;
						}
					} else {
						send_uart_pc("Sending RTS Failed\n");
						numRetries++;
						MACState = MAC_SLEEP;
//					return false;
					}
				} else {
					send_uart_pc(
							"RTS carrier sense detecting activity on channel\n");
					numRetries++;
					MACState = MAC_SLEEP;
//				return false;
				}
			} else {
				send_uart_pc("RTS has no route to destination, sending RReq\n");
				nextNetOp = NET_BROADCAST_RREQ;
				MACState = MAC_NET_OP;

			}
			break;
		case MAC_NET_OP:
			switch (nextNetOp) {
			case NET_NONE:
				MACState = MAC_LISTEN;
				break;
			case NET_REBROADCAST_RREQ:
				send_uart_pc("Rebroadcasting Rreq\n");
				if (SX1276IsChannelFree(MODEM_LORA,
				RF_FREQUENCY, -60, carrierSenseTimes[carrierSenseSlot++])) {
					if (net_re_rreq()) {
						nextNetOp = NET_WAIT;
					} else {
						nextNetOp = NET_REBROADCAST_RREQ;
					}
				}
				break;
			case NET_BROADCAST_RREQ:
				send_uart_pc("Broadcasting Rreq\n");
				if (SX1276IsChannelFree(MODEM_LORA,
				RF_FREQUENCY, -60, carrierSenseTimes[carrierSenseSlot++])) {
					if (send_rreq()) {
						nextNetOp = NET_WAIT;
					} else {
						nextNetOp = NET_BROADCAST_RREQ;
					}
				}
				break;
			case NET_UNICAST_RREP:
				send_rrep();
				MACState = MAC_SLEEP;
				break;
			case NET_WAIT:
				if (!mac_rx(SLOT_LENGTH_MS)) {
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
			send_uart_pc("Mac Hop message\n");
			if (has_route_to_node(GATEWAY_ADDRESS, route)) {
				if (SX1276IsChannelFree(MODEM_LORA,
				RF_FREQUENCY, -60, carrierSenseTimes[carrierSenseSlot++])) {

					if (mac_hop_message(route->next_hop)) { // Send RTS

						if (!mac_rx(SLOT_LENGTH_MS)) {
							MACState = MAC_SLEEP;
							//						return false;
						}
					} else {
						MACState = MAC_SLEEP;
						//					return false;
					}
				} else {
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

bool mac_send(msgType_t msgType, nodeAddress dest) {

	txDatagram.msgHeader.nextHop = dest;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.netDest = dest;
	txDatagram.msgHeader.ID = 5;
	txDatagram.msgHeader.flags = msgType;
	_numMsgSent++;
	txDatagram.msgHeader.txSlot = _txSlot;
	txDatagram.msgHeader.curSlot = get_slot_count();

	if (msgType == MSG_DATA) {
		txDatagram.data.sensData.hum = bme280Data.humidity;
		txDatagram.data.sensData.press = bme280Data.pressure;
		txDatagram.data.sensData.temp = bme280Data.temperature;
		txDatagram.data.sensData.lux = get_lux();
		txDatagram.data.sensData.gpsData = gpsData;
		txDatagram.data.sensData.soilMoisture = soilMoisture;
		txDatagram.data.sensData.tim = timeStamp;

		int size = sizeof(txDatagram);
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

static bool process_rx_buffer() {

	memcpy(&rxDatagram, &RXBuffer, loraRxBufferSize);

	if (receivedMsgIndex == MAX_STORED_MSG) {
		receivedMsgIndex = 0;
	}
//	receivedDatagrams[receivedMsgIndex] = rxDatagram;

	// Comapare slotr counts of the two messages, if different, change this slot Count to the one from the received message
	if (rxDatagram.msgHeader.curSlot != get_slot_count()) {
		set_slot_count(rxDatagram.msgHeader.curSlot);
	}
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
			mac_send(MSG_DATA, rxDatagram.msgHeader.localSource); // Send data to destination node
			MACState = MAC_LISTEN;

			break;
		case MSG_DATA: 	// DATA
			if (rxDatagram.msgHeader.netDest != _nodeID) {
				hopMessageFlag = true;
			} else {
				hopMessageFlag = false;
				// now either store the data for further later sending or get ready to upload data to cloud
				receivedDatagrams[receivedMsgIndex++] = rxDatagram;
				readyToUploadFlag = true;
			}

			mac_send(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
			if (!hopMessageFlag) {
				MACState = MAC_SLEEP;
			} else {
				MACState = MAC_RTS;
			}

			break;
		case MSG_ACK: 	// ACK
//				hasData = false;	// data has succesfully been sent
			numRetries = 0;
			MACState = MAC_SLEEP;
			add_neighbour(rxDatagram.msgHeader.localSource,
					rxDatagram.msgHeader.txSlot);
			add_route_to_neighbour(rxDatagram.msgHeader.localSource);
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
			add_neighbour(rxDatagram.msgHeader.localSource,
					rxDatagram.msgHeader.txSlot);

			MACState = MAC_SLEEP;

			if (rxDatagram.msgHeader.localSource == _nodeID) { // change ID because another node has the same ID
				gen_id(true);
				schedChange = true;
			}
			add_route_to_neighbour(rxDatagram.msgHeader.localSource);
			break;
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

bool mac_send_tx_datagram() {
	bool retVal = false;
	int size = sizeof(txDatagram);
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
	int size = sizeof(txDatagram);
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
