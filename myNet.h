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

#define MAX_ROUTES 10

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
uint8_t getDest(nodeAddress dest);
void sendRREQ();

#endif /* MYNET_H_ */
