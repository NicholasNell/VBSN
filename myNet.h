/*
 * myNet.h
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */

#ifndef MYNET_H_
#define MYNET_H_
#include <datagram.h>
#include <stdint.h>

#define MAX_ROUTES 20
#define MAX_HOPS 4
#define REVERSE_PATH_EXP_TIME 10

typedef struct {
	nodeAddress dest;
	uint8_t next_hop;
	uint8_t num_hops;
	uint8_t dest_seq_num;
	uint8_t num_active_neighbours;
	uint16_t expiration_time;
} RouteEntry_t;

typedef struct {
	nodeAddress destinationAddress;
	nodeAddress sourceAddress;
	uint8_t broadcastID;
	uint8_t expTime;
	uint8_t source_sequence_num;
} ReversePathInfo_t;

void netInit();
void addRoute(nodeAddress dest);
nodeAddress getDest(nodeAddress dest);
void sendRREQ();

// process RReq and determine if the message need to be broadcast or if a RRep needs to be sent

/*!
 *  \brief process RReq and determine if the message need to be broadcast or if a RRep needs to be sent
 * @return message type to be sent
 */
msgType_t processRreq();

#endif /* MYNET_H_ */
