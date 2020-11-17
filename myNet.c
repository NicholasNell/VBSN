/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <myNet.h>
#include <string.h>
// Routing Table
static RouteEntry_t routingtable[20];
static uint16_t _numRoutes;
static uint8_t _routeSequenceNumber;
static uint8_t _broadcastID;

void netInit() {
	_numRoutes = 0;
	_routeSequenceNumber = 0;
	_broadcastID = 0;
	memset(routingtable, NULL, MAX_ROUTES);
}

void addRoute(nodeAddress dest) {
	RouteEntry_t newRoute;
	newRoute.dest = dest;
	newRoute.dest_seq_num = 0;
	newRoute.expiration_time = NULL;
	newRoute.next_hop = newRoute.dest;
	newRoute.num_active_neighbours = 1;
	newRoute.num_hops = 1;

	routingtable[_numRoutes++] = newRoute;
}

uint8_t getDest(nodeAddress dest) {
	return 0;
}

void sendRREQ() {

}
