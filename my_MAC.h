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
#define MAX_NEIGHBOURS 10
#define MAX_STORED_MSG 10

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
void mac_init( );

/*!
 * \brief the state machine that control the mac protocol
 * @return returns true if succesful
 */
bool mac_state_machine( );

/*!
 *
 * @param checks if given node address is a neighbour of this node
 * @return return true if node is a neighbour
 */
bool is_neighbour( nodeAddress node );

/*!
 * \brief add node to neighbour table
 * @param neighbour
 * @param txSlot
 */
void add_neighbour( nodeAddress neighbour, uint16_t txSlot );

/*!
 * \brief copies the contents of the txdatagram into the txBuffer and then transmits it. Only use if txdatagram has already been set up correctly.
 * @return returns true if succesfully sent
 */
bool mac_send_tx_datagram( );

/*!
 * \returns a reference to the neighour table
 * @return return a referce to neighbour table
 */
Neighbour_t* get_neighbour_table( );

/*!
 *
 * @return ref to received msg buffer
 */
Datagram_t* get_received_messages( );

//! @return the number of messages stored in the received messages index
uint8_t get_received_messages_index( void );

//!
//! resets to received message buffer to zero
void reset_received_msg_index( void );

#endif /* MY_MAC_H_ */
