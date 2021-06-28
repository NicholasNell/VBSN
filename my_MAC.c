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
#include <my_rtc.h>
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
static uint16_t loraTxtimes[255];
static uint32_t carrierSenseTimes[0xff];
static uint8_t numRetries = 0;

// Datagram containing the data to be sent over the medium
Datagram_t txDatagram;

// Datagram from received message
Datagram_t rxDatagram;

static Datagram_t receivedDatagrams[MAX_STORED_MSG];
static uint8_t receivedMsgIndex = 0;

// State of the MAC state machine
static volatile MACappState_t MACState = NODE_DISC;

// array of known neighbours
static Neighbour_t neighbourTable[MAX_NEIGHBOURS];

// This node MAC parameters:
static nodeAddress _nodeID;
static uint8_t _numNeighbours;
static uint16_t _numDataMsgSent;
static uint16_t _txSlot;
static uint16_t _numRTSMissed;
static uint16_t _totalMsgSent;
static uint16_t _rreqSent = 0;
static uint16_t _rrepSent = 0;
static uint16_t _rreqReBroadcast = 0;
static Datagram_t msgToHop;

// bme280Data
extern struct bme280_data bme280Data;

// current RadioState
static volatile LoRaRadioState_t RadioState;

// LoRa transmit buffer
static uint8_t TXBuffer[MAX_MESSAGE_LEN];

//Number of data messages received
static int _numMsgReceived = 0;

volatile static NextNetOp_t nextNetOp = NET_NONE;

static bool hopMessageFlag = false;
static bool waitForAck = false;

volatile static bool netOp;

static uint16_t txSlots[WINDOW_SCALER];

/*!
 * \brief Return true if message is successfully processed. Returns false if not.
 */
static bool process_rx_buffer();

//! @param msgType Type of message to send: 	MSG_NONE = 0,	MSG_RTS,	MSG_CTS,	MSG_DATA,	MSG_ACK,	MSG_SYNC,	MSG_RREQ,	MSG_RREP
//! @param dest: local destination to send message to
//! @return Return true if send succesful else, false
static bool mac_send(msgType_t msgType, nodeAddress dest);

static bool mac_rx(uint32_t timeout);

//static void gen_id(bool genNew);

static bool mac_hop_message(nodeAddress dest);

static void set_predefined_id(void);

static void set_predefined_id(void) {
	_nodeID = MAC_ID1;
	_nodeID = MAC_ID2;
	_nodeID = MAC_ID3;
	_nodeID = MAC_ID4;
}

static void set_predefined_tx(void);

static void set_predefined_tx(void) {
	_txSlot = MAC_TX1;
	_txSlot = MAC_TX2;
	_txSlot = MAC_TX3;
	_txSlot = MAC_TX4;
}

// uncomment this function + prototype if youb want to randomly generate new ID's

//static void gen_id(bool genNew) {
//	nodeAddress tempID = 0x0;
//
//	if (genNew) {
//		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS
//				|| tempID == _nodeID) {
//			tempID = (nodeAddress) rand();
//		}
//		_nodeID = tempID;
//	} else {
//
//		while (tempID == BROADCAST_ADDRESS || tempID == GATEWAY_ADDRESS) {
//			tempID = (nodeAddress) rand();
//		}
//		_nodeID = tempID;
//	}
//
//}

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

	if (get_flash_ok_flag()) {
		if (get_is_root()) {
			_nodeID = GATEWAY_ADDRESS;
		} else {
			set_predefined_id();
		}

		set_predefined_tx();
		_numDataMsgSent = flash_get_num_data_sent();
		_totalMsgSent = flash_get_total_msg_sent();

	} else {
		if (get_is_root()) {
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
		if (get_is_root()) {
			_txSlot = 0;
		}

		reset_num_neighbours();
		_numDataMsgSent = 0;
		_totalMsgSent = 0;
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

	for (i = 0; i < 255; ++i) {
		loraTxtimes[i] = SX1276GetTimeOnAir(MODEM_LORA, LORA_BANDWIDTH,
		LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
		LORA_FIX_LENGTH_PAYLOAD_ON, i + 1, LORA_CRC_ON) + 10;
	}

}

bool mac_state_machine() {
	static uint8_t carrierSenseSlot;
	static RouteEntry_t route;
	while (true) {
		switch (MACState) {
		case MAC_SYNC_BROADCAST:
			send_uart_pc("MAC_SYNC_BROADCAST\n");
			if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])) {
				send_uart_pc("MAC_SYNC_BROADCAST: channel clear\n");
				if (mac_send(MSG_SYNC, BROADCAST_ADDRESS)) {
					send_uart_pc("MAC_SYNC_BROADCAST: sent Sync\n");
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
						increment_num_retries();
						_numRTSMissed++;
						if (get_num_retries() > 5) {
							remove_route_with_node(route.next_hop);
							remove_neighbour(route.next_hop);
							set_gps_wake_flag();
						}
						// no response, send again in next slot

						MACState = MAC_SLEEP;
//						return false;
					} else {
						route.expiration_time = get_slot_count() - 1; // reset the expiration timer onthe route you just succesfully used
					}
				} else {
					send_uart_pc("MAC_RTS: send RTS failed\n");
					increment_num_retries();
					MACState = MAC_SLEEP;
//					return false;
				}
//				} else {
//					send_uart_pc("MAC_RTS: channel is busy\n");
//					increment_num_retries();
//					MACState = MAC_SLEEP;
////				return false;
//				}
			} else {
				increment_num_retries();
				_numRTSMissed++;
				send_uart_pc("MAC_RTS: no Route to dest\n");
				nextNetOp = NET_BROADCAST_RREQ;
				MACState = MAC_SLEEP;
				netOp = true;

			}
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
						_rreqReBroadcast++;
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
						_rreqSent++;
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
				if (!get_is_root()) { // if yuo arent the root, wait to see if the root responds first
					if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])) {
						send_rrep();
					}
				} else {	// if you are the root, send rrep immediately
					send_rrep();
				}
				_rrepSent++;
				nextNetOp = NET_NONE;
				MACState = MAC_SLEEP;
				break;
			case NET_BROKEN_LINK:
				if (!mac_rx(carrierSenseTimes[carrierSenseSlot++])) {
					if (mac_send(MSG_BROKEN_LINK, BROADCAST_ADDRESS)) {
						MACState = MAC_SLEEP;
					}
				}
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
				if (mac_hop_message(route.next_hop)) { // Send RTS
					send_uart_pc("MAC_HOP_MESSAGE: sent Hop\n");

					if (!mac_rx(SLOT_LENGTH_MS)) {
						increment_num_retries();
						if (get_num_retries() > 2) {
							remove_route_with_node(route.next_hop);
							remove_neighbour(route.next_hop);
							set_gps_wake_flag();
						}
						send_uart_pc("MAC_HOP_MESSAGE: heard no ACK\n");
						MACState = MAC_SLEEP;
						return false;
					}
				} else {
					hopMessageFlag = true;
					MACState = MAC_SLEEP;
					//					return false;
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
		txDatagram.data.sensData.gpsData = get_gps_data();
		txDatagram.data.sensData.soilMoisture = get_vwc();
		txDatagram.data.sensData.tim = RTC_C_getCalendarTime();

		_numDataMsgSent++;
		txDatagram.netData.numDataSent = _numDataMsgSent;
		txDatagram.netData.timeToRoute = get_time_to_route();
		txDatagram.netData.numNeighbours = get_num_neighbours();
		txDatagram.netData.numRoutes = get_num_routes();
		txDatagram.netData.totalMsgSent = _totalMsgSent;
		txDatagram.netData.rtsMissed = _numRTSMissed;
		txDatagram.netData.distToGate = get_distance_to_gateway();
		txDatagram.netData.rreqTx = _rreqSent;
		txDatagram.netData.rreqReTx = _rreqReBroadcast;
		txDatagram.netData.rrepTx = _rrepSent;

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

static bool process_rx_buffer() {

	memcpy(&rxDatagram, get_rx_buffer(), get_lora_rx_buffer_size());

	if (get_is_root()
			&& (rxDatagram.msgHeader.localSource == 0xBC
					|| rxDatagram.msgHeader.localSource == 0xCD)) { // gateway can't hear BC or CD
		return false;
	}

	if (get_node_id() == 0xBC && rxDatagram.msgHeader.localSource == 0x00) { // Node BC can't hear gateway
		return false;
	}
	if (get_node_id() == 0xAB && rxDatagram.msgHeader.localSource == 0xCD) { // Node AB can't hear CD
		return false;
	}

	if (get_node_id() == 0xCD
			&& (rxDatagram.msgHeader.localSource == 0xAB
					|| rxDatagram.msgHeader.localSource == 0x00)) {
		return false;
	}
	if (rxDatagram.msgHeader.flags > MSG_RREP) { // if rx message does not match any known flags discard
		MACState = MAC_SLEEP;
		return false;
	}

	if (receivedMsgIndex == MAX_STORED_MSG) {
		receivedMsgIndex = 0;
	}

//	if (rxDatagram.msgHeader.localSource == _nodeID) { // change ID because another node has the same ID
//		gen_id(true);
//	}
//	receivedDatagrams[receivedMsgIndex] = rxDatagram;

// Comapare slotr counts of the two messages, if different, change this slot Count to the one from the received message
//	if (rxDatagram.msgHeader.curSlot != get_slot_count()) {
//		set_slot_count(rxDatagram.msgHeader.curSlot);
//	}
//	else {
//
//	}

// check if message was meant for this node:
	add_neighbour(rxDatagram.msgHeader.localSource,
			rxDatagram.msgHeader.txSlot);
	add_route_to_neighbour(rxDatagram.msgHeader.localSource);
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
				memcpy(&msgToHop, &rxDatagram, get_lora_rx_buffer_size());
			} else if (get_is_root()) {
				hopMessageFlag = false;
				// now either store the data for further later sending or get ready to upload data to cloud
				rxDatagram.radioData.rssi = get_rssi_value();
				rxDatagram.radioData.snr = get_snr_value();
				receivedDatagrams[receivedMsgIndex++] = rxDatagram;
				_numMsgReceived++;
			}

			mac_send(MSG_ACK, rxDatagram.msgHeader.localSource); // Send Ack back to transmitting node
			if (!hopMessageFlag) {
				MACState = MAC_SLEEP;
			} else {
				MACState = MAC_HOP_MESSAGE;
			}
//			MACState = MAC_SLEEP;

			break;
		case MSG_ACK: 	// ACK
			WDT_A_clearTimer();
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
			add_neighbour(rxDatagram.msgHeader.localSource,
					rxDatagram.msgHeader.txSlot);
			add_route_to_neighbour(rxDatagram.msgHeader.localSource);
			MACState = MAC_SLEEP;
			return true;
		}
		case MSG_RREQ: 	// RREQ
			// when a rreq is received, determine what actions to do next.
			nextNetOp = process_rreq();
			MACState = MAC_NET_OP;
			netOp = true;
			break;
		case MSG_BROKEN_LINK:
			remove_route_with_node(rxDatagram.msgHeader.localSource);
			remove_neighbour(rxDatagram.msgHeader.localSource);
			MACState = MAC_SLEEP;
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
	neighbourTable[get_num_neighbours()] = receivedNeighbour;
	increment_num_neighbours();
}

bool is_neighbour(nodeAddress node) {
	bool retVal = false;
	int i;
	for (i = 0; i < get_num_neighbours(); ++i) {
		if (node == neighbourTable[i].neighbourID) {
			retVal = true;
		}
	}
	return retVal;
}

bool mac_send_tx_datagram(int size) {
	bool retVal = false;
	memset(TXBuffer, 0, MAX_MESSAGE_LEN);
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
	return (retVal);
}

static bool mac_hop_message(nodeAddress dest) {
	int size = sizeof(msgToHop.msgHeader) + sizeof(msgToHop.data)
			+ sizeof(msgToHop.netData);
	msgToHop.msgHeader.nextHop = dest;
	msgToHop.msgHeader.localSource = _nodeID;
	msgToHop.msgHeader.netSource = rxDatagram.msgHeader.netSource;
	msgToHop.msgHeader.netDest = GATEWAY_ADDRESS;
	msgToHop.msgHeader.hopCount = rxDatagram.msgHeader.hopCount + 1;
	msgToHop.msgHeader.txSlot = _txSlot;
	msgToHop.msgHeader.curSlot = get_slot_count();
	msgToHop.msgHeader.flags = MSG_DATA;

	memcpy(TXBuffer, &msgToHop, size);
	start_lora_timer(loraTxtimes[size + 1]);
	SX1276Send(TXBuffer, size);
	waitForAck = true;
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
	for (i = 0; i < get_num_neighbours(); i++) {
		if (neighbourTable[i].neighbourID == neighbour) {
			if (i == MAX_NEIGHBOURS - 1) {
				decrement_num_neighbours();
			} else {
				int j;
				for (j = i; j < MAX_NEIGHBOURS - 1; j++) {
					neighbourTable[j] = neighbourTable[j + 1];
				}
				decrement_num_neighbours();
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

uint8_t get_num_retries(void) {
	return numRetries;
}

void reset_num_retries(void) {
	numRetries = 0;
}

void increment_num_retries(void) {
	numRetries++;
}

uint8_t get_num_neighbours() {
	return _numNeighbours;
}

void increment_num_neighbours() {
	_numNeighbours++;
}

void decrement_num_neighbours() {
	_numNeighbours--;
}

void reset_num_neighbours() {
	_numNeighbours = 0;
}

uint16_t get_tx_slot() {
	return _txSlot;
}

bool get_hop_message_flag() {
	return hopMessageFlag;
}

uint16_t* get_tx_slots() {
	return txSlots;
}

LoRaRadioState_t get_lora_radio_state() {
	return RadioState;
}

void set_lora_radio_state(LoRaRadioState_t state) {
	RadioState = state;
}

MACappState_t get_mac_app_state() {
	return MACState;
}

void set_mac_app_state(MACappState_t state) {
	MACState = state;
}

nodeAddress get_node_id() {
	return _nodeID;
}

void set_node_id(nodeAddress id) {
	_nodeID = id;
}

int get_num_msg_rx() {
	return _numMsgReceived;
}

void set_next_net_op(NextNetOp_t net) {
	nextNetOp = net;
}

bool get_net_op_flag() {
	return netOp;
}

void set_net_op_flag() {
	netOp = true;
}

void reset_net_op_flag() {
	netOp = false;
}

uint16_t get_num_data_msg_sent() {
	return _numDataMsgSent;
}

uint16_t get_total_msg_sent() {
	return _totalMsgSent;
}

uint16_t get_rreq_sent() {
	return _rreqSent;
}

uint16_t get_rreq_re_sent() {
	return _rreqReBroadcast;
}

uint16_t get_rrep_sent() {
	return _rrepSent;
}

void increment_total_msg_tx() {
	_totalMsgSent++;
}
