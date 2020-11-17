/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <myNet.h>
#include <string.h>
// Routing Table
RouteEntry_t routingtable[MAX_ROUTES];
uint16_t _numRoutes;
uint8_t _routeSequenceNumber;
uint8_t _broadcastID;
uint8_t _destSequenceNumber;

void netInit() {
	_numRoutes = 0;
	_routeSequenceNumber = 0;
	_destSequenceNumber = 0xFF;
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

nodeAddress getDest(nodeAddress dest) {
	return 0;
}

void sendRREQ() {

}
