/*
 * my_MAC.h
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#ifndef MY_MAC_H_
#define MY_MAC_H_

#include <datagram.h>
#include <stdint.h>
#include <stdbool.h>

#define BROADCAST_ADDRESS 0xFF
#define GATEWAY_ADDRESS 0x00
#define MAX_MESSAGE_LEN 255
#define DEFAULT_RX_TIME 1000
#define SLEEP_TIME 60 * 2 * 1000
#define MAX_NEIGHBOURS 255

typedef enum {
	TX = 0, RX, TXDONE, RXDONE, RXTIMEOUT, TXTIMEOUT, RXERROR, RADIO_SLEEP
} LoRaRadioState_t;

typedef enum {
	MAC_SLEEP,
	NODE_DISC,
	MAC_RTS,
	MAC_CTS,
	MAC_DATA,
	MAC_LISTEN,
	MAC_ACK,
	MAC_SYNC_BROADCAST,
	MAC_WAIT,
	MAC_NET_OP,
	MAC_HOP_MESSAGE
} MACappState_t;

typedef struct {
		nodeAddress neighbourID;
		uint16_t neighbourTxSlot;
} Neighbour_t;

/*!
 * \brief Initialise the mac protocol and all node parameters
 */
void MacInit( );

/*!
 * \brief the state machine that control the mac protocol
 * @return returns true if succesful
 */
bool MACStateMachine( );

bool MACStartTransaction(
		nodeAddress destination,
		uint8_t msgType,
		bool isSource );

/*!
 *
 * @param checks if given node address is a neighbour of this node
 * @return return true if node is a neighbour
 */
bool isNeighbour( nodeAddress node );

/*!
 * \brief add node to neighbour table
 * @param neighbour
 * @param txSlot
 */
void addNeighbour( nodeAddress neighbour, uint16_t txSlot );

/*!
 * \brief copies the contents of the txdatagram into the txBuffer and then transmits it. Only use if txdatagram has already been set up correctly.
 * @return returns true if succesfully sent
 */
bool MACSendTxDatagram( );

#endif /* MY_MAC_H_ */
