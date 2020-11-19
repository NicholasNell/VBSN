/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <datagram.h>
#include <my_timer.h>
#include <myNet.h>
#include <string.h>
// Routing Table
RouteEntry_t routingtable[MAX_ROUTES];
uint16_t _numRoutes;
uint8_t _nodeSequenceNumber;
uint8_t _broadcastID;
uint8_t _destSequenceNumber;
bool notHasReversePathInfo = true;

static ReversePathInfo_t reversePathInfo;

extern Datagram_t rxdatagram;
extern nodeAddress _nodeID;

void netInit() {
	_numRoutes = 0;
	_nodeSequenceNumber = 0;
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
	_broadcastID++;
}

/*
 * When receiving a RReq:
 * Does node have the information to satisfy the RReq?:
 * 	either is the destination node, or knows the route to the destination node.
 * 	Or, it doesnt know the route and will rebroadcast the RReq.
 */

NextNetOp_t processRreq() {

	NextNetOp_t retVal = NET_NONE;

	nodeAddress source_addr = rxdatagram.data.Rreq.source_addr;
	uint8_t broadcast_id = rxdatagram.data.Rreq.broadcast_id;

	if (source_addr == reversePathInfo.sourceAddress
			&& broadcast_id == reversePathInfo.broadcastID) { // if this message has already been received; ignore
		retVal = NET_NONE;
		return retVal;
	} else {						// else if message has not been received yet
		if (rxdatagram.data.Rreq.dest_addr == _nodeID) {// check if node is destination node and return

			retVal = NET_UNICAST_RREP;
			return retVal;
		} else {	// check if route is known in routing table
			int i;
			for (i = 0; i < _numRoutes; ++i) {
				if (rxdatagram.data.Rreq.dest_addr == routingtable[i].dest) {
					if (rxdatagram.data.Rreq.dest_sequence_num
							> routingtable[i].dest_seq_num) {
						retVal = NET_REBROADCAST_RREQ;
					} else {
						retVal = NET_UNICAST_RREP;
						return retVal;
					}
				} else {
					retVal = NET_REBROADCAST_RREQ;
				}
			}
		}

		if (!notHasReversePathInfo) {
			reversePathInfo.broadcastID = broadcast_id;
			reversePathInfo.destinationAddress = rxdatagram.data.Rreq.dest_addr;
			reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
			reversePathInfo.sourceAddress = source_addr;
			reversePathInfo.source_sequence_num =
					rxdatagram.data.Rreq.source_sequence_num;
			notHasReversePathInfo = false;
			startTimer32Counter(REVERSE_PATH_EXP_TIME * 1000,
					&notHasReversePathInfo);	//
		}
	}
	return retVal;
}
