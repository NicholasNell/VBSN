/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <my_timer.h>
#include <myNet.h>
#include <string.h>
// Routing Table
RouteEntry_t routingtable[MAX_ROUTES];
uint16_t _numRoutes;
uint8_t _routeSequenceNumber;
uint8_t _broadcastID;
uint8_t _destSequenceNumber;
bool hasReversePathInfo = false;

static ReversePathInfo_t reversePathInfo;

extern Datagram_t rxdatagram;

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

msgType_t processRreq() {

	msgType_t retVal = MSG_NONE;

	nodeAddress source_addr = rxdatagram.data.Rreq.source_addr;
	uint8_t broadcast_id = rxdatagram.data.Rreq.broadcast_id;

	if (source_addr == reversePathInfo.sourceAddress
			&& broadcast_id == reversePathInfo.broadcastID) { // if this message has already been received; ignore
		retVal = MSG_NONE;
		return retVal;
	} else {						// else if message has not been received yet
		if (rxdatagram.data.Rreq.dest_addr == _nodeID) {// check if node is destination node and return

			retVal = MSG_RREP;
		} else {	// check if route is known in routing table
			int i;
			for (i = 0; i < _numRoutes; ++i) {
				if (rxdatagram.data.Rreq.dest_addr == routingtable[i].dest) {
					retVal = MSG_RREP;
					return retVal;
				} else {
					retVal = MSG_RREQ;
				}
			}
		}

		reversePathInfo.broadcastID = broadcast_id;
		reversePathInfo.destinationAddress = rxdatagram.data.Rreq.dest_addr;
		reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
		reversePathInfo.sourceAddress = source_addr;
		reversePathInfo.source_sequence_num =
				rxdatagram.data.Rreq.source_sequence_num;
		hasReversePathInfo = true;
		startTimer32Counter(REVERSE_PATH_EXP_TIME * 1000, &hasReversePathInfo);	//
	}

	if (hasReversePathInfo) {

	}
	return retVal;
}
