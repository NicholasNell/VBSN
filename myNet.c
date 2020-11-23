/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <datagram.h>
#include <my_MAC.h>
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

bool notHasForwardPathInfo = true;
static ForwardPathInfo_t forwardPathInfo;

extern Datagram_t rxdatagram;
extern nodeAddress _nodeID;

void netInit( ) {
	_numRoutes = 0;
	_nodeSequenceNumber = 0;
	_destSequenceNumber = 0xFF;
	_broadcastID = 0;
	memset(routingtable, NULL, MAX_ROUTES);
}

void addRoute( RouteEntry_t routeToNode ) {
	routingtable[_numRoutes++] = routeToNode;
}

void addRoutetoNeighbour( nodeAddress dest ) {
	RouteEntry_t newRoute;
	newRoute.dest = dest;
	newRoute.dest_seq_num = 0;
	newRoute.expiration_time = NULL;
	newRoute.next_hop = newRoute.dest;
	newRoute.num_hops = 1;

	routingtable[_numRoutes++] = newRoute;
}

nodeAddress getDest( nodeAddress dest ) {
	return 0;
}

void sendRREQ( ) {
	_broadcastID++;
}

/*
 * When receiving a RReq:
 * Does node have the information to satisfy the RReq?:
 * 	either is the destination node, or knows the route to the destination node.
 * 	Or, it doesnt know the route and will rebroadcast the RReq.
 */

NextNetOp_t processRreq( ) {

	NextNetOp_t retVal = NET_NONE;
	nodeAddress source_addr = rxdatagram.data.Rreq.source_addr;
	uint8_t broadcast_id = rxdatagram.data.Rreq.broadcast_id;

	if (!notHasReversePathInfo) {

		reversePathInfo.hopcount = rxdatagram.data.Rreq.hop_cnt + 1;
		reversePathInfo.nextHop = rxdatagram.msgHeader.localSource;
		reversePathInfo.destinationAddress = source_addr;

		reversePathInfo.source_sequence_num =
				rxdatagram.data.Rreq.source_sequence_num;
		reversePathInfo.broadcastID = broadcast_id;

		reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;

		addReversePathToTable();

		notHasReversePathInfo = false;
		startTimer32Counter(
		REVERSE_PATH_EXP_TIME * 1000, &notHasReversePathInfo);	//
	}

	// check if message has been received from known neighbour; if not, add to neighbour list and add a new entry to the routing table.
	bool isNeighbourFlag = isNeighbour(rxdatagram.msgHeader.localSource);

	if (!isNeighbourFlag) {
		addNeighbour(
				rxdatagram.msgHeader.localSource,
				rxdatagram.msgHeader.txSlot);
		addRoutetoNeighbour(rxdatagram.msgHeader.localSource);
	}

	if (reversePathInfo.destinationAddress == rxdatagram.data.Rreq.source_addr
			&& broadcast_id == reversePathInfo.broadcastID) { // if this message has already been received; ignore and wait for another message
		retVal = NET_WAIT;
		return retVal;
	}
	else {				// else if message has not been received yet
		if (rxdatagram.data.Rreq.dest_addr == _nodeID) {// check if node is destination node and return

			retVal = NET_UNICAST_RREP;
			return retVal;
		}
		else {// if node is not destination, check if route is known in routing table
			RouteEntry_t *tempRouteEntry = NULL;
			bool hasRoute = hasRouteToNode(
					rxdatagram.data.Rreq.dest_addr,
					tempRouteEntry);
			if (tempRouteEntry == NULL) {	// no known route, rebroadcast RReq.
				retVal = NET_REBROADCAST_RREQ;
			}
			else if (tempRouteEntry->dest == rxdatagram.data.Rreq.dest_addr) { // routing table conatins route and can now reply with the routing information
				if ((tempRouteEntry->dest_seq_num
						< rxdatagram.data.Rreq.dest_sequence_num)) { // routing table entry is old and a new route is available
					retVal = NET_REBROADCAST_RREQ;
				}
				else { // routing table entry is current, unicast route back to source
					retVal = NET_UNICAST_RREP;
				}
			}
			else {	// any other combination is invalid, do nothing.
				retVal = NET_NONE;
			}
		}
	}

	return retVal;
}

void netReRReq( ) {

}

bool hasRouteToNode( nodeAddress dest, RouteEntry_t *routeToNode ) {
	if (routeToNode == NULL) ;
	bool retVal = false;
	int i;
	for (i = 0; i < _numRoutes; ++i) {
		if (dest == routingtable[i].dest) {
			retVal = true;
			routeToNode = &routingtable[i];
		}
		else {
			routeToNode = NULL;
		}
	}
	return retVal;
}

NextNetOp_t processRrep( ) {
	NextNetOp_t retVal = NET_NONE;

	forwardPathInfo.destinationAddress = rxdatagram.data.Rrep.dest_addr;
	forwardPathInfo.nextHop = rxdatagram.msgHeader.localSource;
	forwardPathInfo.hopcount = rxdatagram.data.Rrep.hop_cnt + 1;
	forwardPathInfo.expTime = REVERSE_PATH_EXP_TIME;
	forwardPathInfo.dest_sequence_num = rxdatagram.data.Rrep.dest_sequence_num;

	bool addedPath = addForwardPathToTable();

	if (rxdatagram.data.Rrep.dest_addr == _nodeID) {
		retVal = NET_NONE;
		return retVal;
	}

	if (addedPath) {
		retVal = NET_UNICAST_RREP;
	}
	else {
		retVal = NET_NONE;
	}

	return retVal;

}

bool addReversePathToTable( ) {

	bool retVal = false;
	// first check if an entry exists, then check its sequence number
	RouteEntry_t *routeToNode = NULL;
	bool hasRoute = hasRouteToNode(
			reversePathInfo.destinationAddress,
			routeToNode);

	RouteEntry_t newRouteToNode;
	newRouteToNode.dest = reversePathInfo.destinationAddress;
	newRouteToNode.dest_seq_num = reversePathInfo.source_sequence_num;
	newRouteToNode.expiration_time = reversePathInfo.expTime;
	newRouteToNode.next_hop = reversePathInfo.nextHop;
	newRouteToNode.num_hops = reversePathInfo.hopcount + 1;

	if (hasRoute) {	// entry already exist within the routing table, now compare the sequence numbers
		if (routeToNode->dest_seq_num < reversePathInfo.source_sequence_num) {
			memcpy(routeToNode, &newRouteToNode, sizeof(RouteEntry_t));
			retVal = true;
		}
	}
	else {	// does not have a route, so add one
		addRoute(newRouteToNode);
		retVal = true;
	}
	return retVal;
}

bool addForwardPathToTable( ) {
	bool retVal = false;
	RouteEntry_t *routeToNode = NULL;
	bool hasRoute = hasRouteToNode(
			forwardPathInfo.destinationAddress,
			routeToNode);
	RouteEntry_t newRouteToNode;
	newRouteToNode.dest = forwardPathInfo.destinationAddress;
	newRouteToNode.dest_seq_num = forwardPathInfo.dest_sequence_num;
	newRouteToNode.expiration_time = forwardPathInfo.expTime;
	newRouteToNode.next_hop = forwardPathInfo.nextHop;
	newRouteToNode.num_hops = forwardPathInfo.hopcount + 1;

	if (hasRoute) {	// entry already exist within the routing table, now compare the sequence numbers
		if (routeToNode->dest_seq_num < forwardPathInfo.dest_sequence_num) {
			memcpy(routeToNode, &newRouteToNode, sizeof(RouteEntry_t));
			retVal = true;
		}
	}
	else {	// does not have a route, so add one
		addRoute(newRouteToNode);
		retVal = true;
	}
	return retVal;
}
