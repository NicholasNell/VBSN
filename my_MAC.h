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
#include <my_scheduler.h>
#include <myNet.h>

#define BROADCAST_ADDRESS 0xFF
#define GATEWAY_ADDRESS 0x00
#define MAX_MESSAGE_LEN 255
#define DEFAULT_RX_TIME 1000
#define SLEEP_TIME 60 * 2 * 1000
#define MAX_NEIGHBOURS 10
#define MAX_STORED_MSG 20

#define MAC_ID1 0xAB
#define MAC_ID2 0xBC
#define MAC_ID3 0xCD
#define MAC_ID4 0xDE

#define MAC_TX1 ((WINDOW_TIME_SEC-30) / 4)
#define MAC_TX2 ((WINDOW_TIME_SEC-30) / 4) * 2
#define MAC_TX3 ((WINDOW_TIME_SEC-30) / 4) * 3
#define MAC_TX4 ((WINDOW_TIME_SEC-30) / 4) * 4

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
void mac_init();

/*!
 * \brief the state machine that control the mac protocol
 * @return returns true if succesful
 */
bool mac_state_machine();

/*!
 * @brief builds the tx datagaram and readies it for sending
 */
void build_tx_datagram(void);

/*!
 *
 * @param checks if given node address is a neighbour of this node
 * @return return true if node is a neighbour
 */
bool is_neighbour(nodeAddress node);

/*!
 * \brief add node to neighbour table
 * @param neighbour
 * @param txSlot
 */
void add_neighbour(nodeAddress neighbour, uint16_t txSlot);

//! @param neighbour to remove
void remove_neighbour(nodeAddress neighbour);

/*!
 * \brief copies the contents of the txdatagram into the txBuffer and then transmits it. Only use if txdatagram has already been set up correctly.
 * @return returns true if succesfully sent
 */
bool mac_send_tx_datagram(int size);

/*!
 * \returns a reference to the neighour table
 * @return return a referce to neighbour table
 */
Neighbour_t* get_neighbour_table();

/*!
 *
 * @return ref to received msg buffer
 */
Datagram_t* get_received_messages();

//! @return the number of messages stored in the received messages index
uint8_t get_received_messages_index(void);

//!
//! resets to received message buffer to zero
void reset_received_msg_index(void);

//!
//! @brief decreases the received message index by one
void decrease_received_message_index(void);

//! @return number of retries
uint8_t get_num_retries(void);

//!
//! Resets number of retries
void reset_num_retries(void);

//!
//! Increments number of retries
void increment_num_retries(void);

uint8_t get_num_neighbours();

void increment_num_neighbours();

void decrement_num_neighbours();

void reset_num_neighbours();

uint16_t get_tx_slot();

bool get_hop_message_flag();

uint16_t* get_tx_slots();

LoRaRadioState_t get_lora_radio_state();

void set_lora_radio_state(LoRaRadioState_t state);

MACappState_t get_mac_app_state();
void set_mac_app_state(MACappState_t state);

nodeAddress get_node_id();
void set_node_id(nodeAddress id);

int get_num_msg_rx();

void set_next_net_op(NextNetOp_t net);

bool get_net_op_flag();
void set_net_op_flag();
void reset_net_op_flag();

uint16_t get_num_data_msg_sent();
uint16_t get_total_msg_sent();

uint16_t get_rreq_sent();
uint16_t get_rreq_re_sent();
uint16_t get_rrep_sent();

#endif /* MY_MAC_H_ */
