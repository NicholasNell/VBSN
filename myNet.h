/*
 * myNet.h
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */

#ifndef MYNET_H_
#define MYNET_H_

#define MAX_ROUTES 10

typedef struct {
	uint8_t dest;
	uint8_t next_hop;
	uint8_t num_hops;
	uint8_t dest_seq_num;
	uint8_t num_active_neighbours;
	uint16_t expiration_time;
} RouteEntry_t;

#endif /* MYNET_H_ */
