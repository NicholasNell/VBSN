/*
 * myNet.c
 *
 *  Created on: 26 Aug 2020
 *      Author: njnel
 */
#include <datagram.h>
#include <my_MAC.h>
#include <my_RFM9x.h>
#include <my_timer.h>
#include <myNet.h>
#include <string.h>
// Routing Table
RouteEntry_t routingtable[MAX_ROUTES];
uint16_t _numRoutes;
uint8_t _nodeSequenceNumber;
uint8_t _broadcastID;
uint8_t _destSequenceNumber;

static nodeAddress rrepNodeUnicast;
bool notHasReversePathInfo = true;

static ReversePathInfo_t reversePathInfo;

bool notHasForwardPathInfo = true;
static ForwardPathInfo_t forwardPathInfo;

extern Datagram_t rxDatagram;
extern Datagram_t txDatagram;
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

bool sendRREQ( ) {
	bool retVal = false;

	RouteEntry_t *newRoute;
	hasRouteToNode(GATEWAY_ADDRESS, newRoute);

	_broadcastID++;

	txDatagram.msgHeader.flags = MSG_RREQ;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.nextHop = BROADCAST_ADDRESS;
	txDatagram.msgHeader.ttl = 5;
	txDatagram.msgHeader.txSlot = _txSlot;

	txDatagram.data.Rreq.broadcast_id = _broadcastID;
	txDatagram.data.Rreq.dest_addr = GATEWAY_ADDRESS;

	if (newRoute == NULL) { // if no route exist to node in question, set sequence number as zero
		txDatagram.data.Rreq.dest_sequence_num = 0;
	}
	else if (newRoute->dest_seq_num >= 0) {
		txDatagram.data.Rreq.dest_sequence_num = newRoute->dest_seq_num;
	}

	txDatagram.data.Rreq.hop_cnt = 0; // hop count always starts at zero.
	txDatagram.data.Rreq.source_addr = _nodeID;
	txDatagram.data.Rreq.source_sequence_num = _nodeSequenceNumber;

	retVal = MACSendTxDatagram();
	return retVal;

}

/*
 * When receiving a RReq:
 * Does node have the information to satisfy the RReq?:
 * 	either is the destination node, or knows the route to the destination node.
 * 	Or, it doesnt know the route and will rebroadcast the RReq.
 */

NextNetOp_t processRreq( ) {

	NextNetOp_t retVal = NET_NONE;
	nodeAddress source_addr = rxDatagram.data.Rreq.source_addr;
	uint8_t broadcast_id = rxDatagram.data.Rreq.broadcast_id;

	if (!notHasReversePathInfo) {

		reversePathInfo.hopcount = rxDatagram.data.Rreq.hop_cnt + 1;
		reversePathInfo.nextHop = rxDatagram.msgHeader.localSource;
		reversePathInfo.destinationAddress = source_addr;

		reversePathInfo.source_sequence_num =
				rxDatagram.data.Rreq.source_sequence_num;
		reversePathInfo.broadcastID = broadcast_id;

		reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;

		addReversePathToTable();

		notHasReversePathInfo = false;
		startTimer32Counter(
		REVERSE_PATH_EXP_TIME * 1000, &notHasReversePathInfo);	//
	}

	// check if message has been received from known neighbour; if not, add to neighbour list and add a new entry to the routing table.
	bool isNeighbourFlag = isNeighbour(rxDatagram.msgHeader.localSource);

	if (!isNeighbourFlag) {
		addNeighbour(
				rxDatagram.msgHeader.localSource,
				rxDatagram.msgHeader.txSlot);
		addRoutetoNeighbour(rxDatagram.msgHeader.localSource);
	}

	if (reversePathInfo.destinationAddress == rxDatagram.data.Rreq.source_addr
			&& broadcast_id == reversePathInfo.broadcastID) { // if this message has already been received; ignore and wait for another message
		retVal = NET_WAIT;
		return retVal;
	}
	else {				// else if message has not been received yet
		if (rxDatagram.data.Rreq.dest_addr == _nodeID) {// check if node is destination node and return

			retVal = NET_UNICAST_RREP;
			rrepNodeUnicast = rxDatagram.msgHeader.localSource;
			return retVal;
		}
		else {// if node is not destination, check if route is known in routing table
			RouteEntry_t *tempRouteEntry = NULL;
			bool hasRoute = hasRouteToNode(
					rxDatagram.data.Rreq.dest_addr,
					tempRouteEntry);
			if (tempRouteEntry == NULL) {	// no known route, rebroadcast RReq.
				retVal = NET_REBROADCAST_RREQ;
			}
			else if (tempRouteEntry->dest == rxDatagram.data.Rreq.dest_addr) { // routing table conatins route and can now reply with the routing information
				if ((tempRouteEntry->dest_seq_num
						< rxDatagram.data.Rreq.dest_sequence_num)) { // routing table entry is old and a new route is available
					retVal = NET_REBROADCAST_RREQ;
				}
				else { // routing table entry is current, unicast route back to source
					retVal = NET_UNICAST_RREP;
					rrepNodeUnicast = rxDatagram.msgHeader.localSource;
				}
			}
			else {	// any other combination is invalid, do nothing.
				retVal = NET_NONE;
			}
		}
	}

	return retVal;
}

bool netReRReq( ) {
	bool retVal = false;
	// copy the contents of the rxdatagram into the txdatagram to send it.
	memcpy(
			&txDatagram,
			&rxDatagram,
			sizeof(rxDatagram.msgHeader) + sizeof(rxDatagram.data));

	txDatagram.data.Rreq.hop_cnt++;
	txDatagram.msgHeader.localSource = _nodeID;

	retVal = MACSendTxDatagram();
	return retVal;

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

	forwardPathInfo.destinationAddress = rxDatagram.data.Rrep.dest_addr;
	forwardPathInfo.nextHop = rxDatagram.msgHeader.localSource;
	forwardPathInfo.hopcount = rxDatagram.data.Rrep.hop_cnt + 1;
	forwardPathInfo.expTime = REVERSE_PATH_EXP_TIME;
	forwardPathInfo.dest_sequence_num = rxDatagram.data.Rrep.dest_sequence_num;

	bool addedPath = addForwardPathToTable();

	if (rxDatagram.data.Rrep.dest_addr == _nodeID) {
		retVal = NET_NONE;
		return retVal;
	}

	if (addedPath) {
		retVal = NET_UNICAST_RREP;
		RouteEntry_t *route;
		if (hasRouteToNode(rxDatagram.data.Rrep.dest_addr, route)) {
			rrepNodeUnicast = route->next_hop;
		}
		else {
			retVal = false;
			return retVal;
		}
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

bool sendRRep( ) {
	bool retVal = false;

	RouteEntry_t *newRoute;
	hasRouteToNode(rxDatagram.msgHeader.netDest, newRoute);

	txDatagram.msgHeader.flags = MSG_RREP;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netDest = newRoute->dest;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.nextHop = rrepNodeUnicast;
	txDatagram.msgHeader.ttl = 5;
	txDatagram.msgHeader.txSlot = _txSlot;

	txDatagram.data.Rrep.dest_addr = newRoute->dest;
	txDatagram.data.Rrep.dest_sequence_num = newRoute->dest_seq_num;
	txDatagram.data.Rrep.hop_cnt = newRoute->num_hops;
	txDatagram.data.Rrep.lifetime = 5;
	txDatagram.data.Rrep.source_addr = _nodeID;

	retVal = MACSendTxDatagram();
	return retVal;
}
