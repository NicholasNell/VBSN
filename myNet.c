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
#include <my_flash.h>
// Routing Table
RouteEntry_t routingtable[MAX_ROUTES];
uint8_t _numRoutes;
uint8_t _nodeSequenceNumber;
uint8_t _broadcastID;
uint8_t _destSequenceNumber;

static uint8_t IDSourceCombo[MAX_ROUTES][2];
static uint8_t IDSourceComboCounter = 0;

bool HasReversePathInfo = false;

static ReversePathInfo_t reversePathInfo;

bool HasForwardPathInfo = false;
static ForwardPathInfo_t forwardPathInfo;

extern Datagram_t rxDatagram;
extern Datagram_t txDatagram;
extern nodeAddress _nodeID;
extern uint16_t _txSlot;
// extern is flash ok flag
extern bool flashOK;

static bool check_received_rreq(uint8_t broadcast_id, uint8_t sourceAddress);

static void add_rreq_combo(uint8_t broadcast_id, uint8_t sourceAddress);

static void add_rreq_combo(uint8_t broadcast_id, uint8_t sourceAddress) {
	if (IDSourceComboCounter >= MAX_ROUTES) { /// only store last \ref{MAX_ROUTES} combos
		IDSourceComboCounter = 0;
	}
	IDSourceCombo[IDSourceComboCounter][0] = broadcast_id;
	IDSourceCombo[IDSourceComboCounter][1] = sourceAddress;
	IDSourceComboCounter++;
}

static bool check_received_rreq(uint8_t broadcast_id, uint8_t sourceAddress) {
	bool retVal = false;
	int i = 0;
	for (i = 0; i < IDSourceComboCounter; i++) {
		if (IDSourceCombo[i][0] == broadcast_id
				&& IDSourceCombo[i][1] == sourceAddress) { // if broadcast_id and source address match a previous message, return true
			retVal = true;
			return retVal;
		}
	}
	return retVal;
}

void net_init() {
	if (flashOK) {
		FlashData_t *pFlashData = get_flash_data_struct();
		_numRoutes = pFlashData->_numRoutes;
		_nodeSequenceNumber = pFlashData->_nodeSequenceNumber;
		_broadcastID = pFlashData->_broadcastID;
		_destSequenceNumber = pFlashData->_destSequenceNumber;
		memcpy(routingtable, pFlashData->routingtable,
				sizeof(pFlashData->routingtable));
	} else {
		_numRoutes = 0;
		_nodeSequenceNumber = 0;
		_destSequenceNumber = 0xFF;
		_broadcastID = 0;
		memset(routingtable, NULL, MAX_ROUTES);
		memset(IDSourceCombo, NULL, MAX_ROUTES * 2);
	}
}

void remove_route_with_node(nodeAddress node) {
	_nodeSequenceNumber++;
	int i;
	for (i = 0; i < MAX_ROUTES; i++) {
		if (routingtable[i].dest == node || routingtable[i].next_hop == node) {
			if (i == MAX_ROUTES - 1) {
				_numRoutes--;
			} else {
				int j;
				for (j = i; j < MAX_ROUTES - 1; j++) {
					routingtable[j] = routingtable[j + 1];
				}
				_numRoutes--;
			}

		}
	}
}

void add_route(RouteEntry_t routeToNode) {
	_nodeSequenceNumber++;
	routingtable[_numRoutes++] = routeToNode;
}

void add_route_to_neighbour(nodeAddress dest) {
	_nodeSequenceNumber++;
	RouteEntry_t newRoute;
	if (has_route_to_node(dest, &newRoute)) {
		if (newRoute.num_hops > 0) {
			newRoute.dest = dest;
			newRoute.dest_seq_num = 0;
			newRoute.expiration_time = 60;
			newRoute.next_hop = newRoute.dest;
			newRoute.num_hops = 0;
			int i = 0;
			for (i = 0; i < _numRoutes; ++i) {
				if (routingtable[i].dest == newRoute.dest) {
					routingtable[i] = newRoute;
				}
			}
		} else {
			return;
		}
	} else {
		newRoute.dest = dest;
		newRoute.dest_seq_num = 0;
		newRoute.expiration_time = 60;
		newRoute.next_hop = newRoute.dest;
		newRoute.num_hops = 0;

		routingtable[_numRoutes++] = newRoute;
	}
}

nodeAddress get_dest(nodeAddress dest) {
	return 0;
}

bool send_rreq() {
	bool retVal = false;

	RouteEntry_t newRoute;
	bool hasRoute = has_route_to_node(GATEWAY_ADDRESS, &newRoute);

	_broadcastID++;

	txDatagram.msgHeader.flags = MSG_RREQ;
	txDatagram.msgHeader.localSource = _nodeID;
	txDatagram.msgHeader.netDest = GATEWAY_ADDRESS;
	txDatagram.msgHeader.netSource = _nodeID;
	txDatagram.msgHeader.nextHop = BROADCAST_ADDRESS;
	txDatagram.msgHeader.hopCount = 0;
	txDatagram.msgHeader.txSlot = _txSlot;

	txDatagram.data.Rreq.broadcast_id = _broadcastID;
	txDatagram.data.Rreq.dest_addr = GATEWAY_ADDRESS;

	if (!hasRoute) { // if no route exist to node in question, set sequence number as zero
		txDatagram.data.Rreq.dest_sequence_num = 0;
	} else {
		txDatagram.data.Rreq.dest_sequence_num = newRoute.dest_seq_num;
	}

	txDatagram.data.Rreq.hop_cnt = 0; // hop count always starts at zero.
	txDatagram.data.Rreq.source_addr = _nodeID;
	txDatagram.data.Rreq.source_sequence_num = _nodeSequenceNumber;

	int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.data.Rreq);
	retVal = mac_send_tx_datagram(size);
	return retVal;
}

/*
 * When receiving a RReq:
 * Does node have the information to satisfy the RReq?:
 * 	either is the destination node, or knows the route to the destination node.
 * 	Or, it doesnt know the route and will rebroadcast the RReq.
 */

NextNetOp_t process_rreq() {

	NextNetOp_t retVal = NET_NONE;
	nodeAddress source_addr = rxDatagram.data.Rreq.source_addr;
	uint8_t broadcast_id = rxDatagram.data.Rreq.broadcast_id;

	if (rxDatagram.data.Rreq.source_addr == _nodeID) { // if this Rreq originated from this node, wait
		retVal = NET_WAIT;
		return retVal;
	}

	if (check_received_rreq(broadcast_id, source_addr)) { // already received this message, so ignore
		retVal = NET_WAIT;
		return retVal;
	} else {	// has not received this message, add to combo array
		add_rreq_combo(broadcast_id, source_addr);
	}

// if rreq came from a direct neighbour and this is a gateway, send rrep;
	if (rxDatagram.data.Rreq.dest_addr == _nodeID) {
		reversePathInfo.broadcastID = rxDatagram.data.Rreq.broadcast_id;
		reversePathInfo.destinationAddress = rxDatagram.data.Rreq.dest_addr;
		reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
		reversePathInfo.hopcount = rxDatagram.data.Rreq.hop_cnt;
		reversePathInfo.nextHop = rxDatagram.msgHeader.localSource;
		reversePathInfo.source_sequence_num =
				rxDatagram.data.Rreq.source_sequence_num;
		HasReversePathInfo = true;
		start_timer_32_counter(
		REVERSE_PATH_EXP_TIME * 1000, &HasReversePathInfo);
		retVal = NET_UNICAST_RREP;
		return retVal;
	}

	RouteEntry_t routeToNode;
	if (has_route_to_node(rxDatagram.data.Rreq.dest_addr, &routeToNode)) { // has route info, so send it back
		if (routeToNode.dest_seq_num < rxDatagram.data.Rreq.dest_sequence_num) { // Route stored in Routing table is old, delete and rebroadcast rreq

			reversePathInfo.broadcastID = rxDatagram.data.Rreq.broadcast_id;
			reversePathInfo.destinationAddress = rxDatagram.data.Rreq.dest_addr;
			reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
			reversePathInfo.hopcount = rxDatagram.data.Rreq.hop_cnt;
			reversePathInfo.nextHop = rxDatagram.msgHeader.localSource;
			reversePathInfo.source_sequence_num =
					rxDatagram.data.Rreq.source_sequence_num;
			HasReversePathInfo = true;
			start_timer_32_counter(
			REVERSE_PATH_EXP_TIME * 1000, &HasReversePathInfo);
			retVal = NET_REBROADCAST_RREQ;
		} else {	// has current route to destination
			reversePathInfo.broadcastID = rxDatagram.data.Rreq.broadcast_id;
			reversePathInfo.destinationAddress = rxDatagram.data.Rreq.dest_addr;
			reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
			reversePathInfo.hopcount = rxDatagram.data.Rreq.hop_cnt;
			reversePathInfo.nextHop = rxDatagram.msgHeader.localSource;
			reversePathInfo.source_sequence_num =
					rxDatagram.data.Rreq.source_sequence_num;
			HasReversePathInfo = true;
			start_timer_32_counter(
			REVERSE_PATH_EXP_TIME * 1000, &HasReversePathInfo);
			retVal = NET_UNICAST_RREP;
		}
	} else { //has no route info, so save reverse path info and rebroadcast rreq
		reversePathInfo.broadcastID = rxDatagram.data.Rreq.broadcast_id;
		reversePathInfo.destinationAddress = rxDatagram.data.Rreq.dest_addr;
		reversePathInfo.expTime = REVERSE_PATH_EXP_TIME;
		reversePathInfo.hopcount = rxDatagram.data.Rreq.hop_cnt;
		reversePathInfo.nextHop = rxDatagram.msgHeader.localSource;
		reversePathInfo.source_sequence_num =
				rxDatagram.data.Rreq.source_sequence_num;
		HasReversePathInfo = true;
		start_timer_32_counter(
		REVERSE_PATH_EXP_TIME * 1000, &HasReversePathInfo);
		retVal = NET_REBROADCAST_RREQ;
	}

	return retVal;
}

bool net_re_rreq() {
	bool retVal = false;
// copy the contents of the rxdatagram into the txdatagram to send it.
	memcpy(&txDatagram, &rxDatagram,
			sizeof(rxDatagram.msgHeader) + sizeof(rxDatagram.data));

	if (txDatagram.data.Rreq.source_addr != _nodeID) { // only rebroadcast if not own Rreq

		if (txDatagram.data.Rreq.hop_cnt <= MAX_HOPS) {	// no more than 5 hops
			txDatagram.data.Rreq.hop_cnt++;
			txDatagram.msgHeader.localSource = _nodeID;
			int size = sizeof(txDatagram.msgHeader)
					+ sizeof(txDatagram.data.Rreq);
			retVal = mac_send_tx_datagram(size);
		}
	}
	return retVal;
}

bool has_route_to_node(nodeAddress dest, RouteEntry_t *routeToNode) {
	if (routeToNode == NULL)
		;
	bool retVal = false;
	if (dest == _nodeID) {
		retVal = true;
		return retVal;
	}
	int i;
	for (i = 0; i < _numRoutes; ++i) {
		if (dest == routingtable[i].dest) {
			retVal = true;
//			routeToNode = &routingtable[i];
//			memcpy(routeToNode, &routingtable[i], sizeof(RouteEntry_t));
			routeToNode->dest = routingtable[i].dest;
			routeToNode->dest_seq_num = routingtable[i].dest_seq_num;
			routeToNode->expiration_time = routingtable[i].expiration_time;
			routeToNode->next_hop = routingtable[i].next_hop;
			routeToNode->num_hops = routingtable[i].num_hops;

			break;
		}

	}
	return retVal;
}

NextNetOp_t process_rrep() {
	NextNetOp_t retVal = NET_NONE;

	forwardPathInfo.destinationAddress = rxDatagram.msgHeader.netDest;
	forwardPathInfo.nextHop = rxDatagram.msgHeader.localSource;
	forwardPathInfo.hopcount = rxDatagram.data.Rrep.hop_cnt + 1;
	forwardPathInfo.expTime = REVERSE_PATH_EXP_TIME;
	forwardPathInfo.dest_sequence_num = rxDatagram.data.Rrep.dest_sequence_num;

	bool addedPath = add_forward_path_to_table();

	if (rxDatagram.data.Rrep.dest_addr == _nodeID) {
		retVal = NET_NONE;
		return retVal;
	} else if (addedPath) {
		retVal = NET_UNICAST_RREP;
	}
	return retVal;
}

bool add_reverse_path_to_table() {

	bool retVal = false;
// first check if an entry exists, then check its sequence number
	RouteEntry_t routeToNode;
	bool hasRoute = has_route_to_node(reversePathInfo.destinationAddress,
			&routeToNode);

	RouteEntry_t newRouteToNode;
	newRouteToNode.dest = reversePathInfo.destinationAddress;
	newRouteToNode.dest_seq_num = reversePathInfo.source_sequence_num;
	newRouteToNode.expiration_time = reversePathInfo.expTime;
	newRouteToNode.next_hop = reversePathInfo.nextHop;
	newRouteToNode.num_hops = reversePathInfo.hopcount + 1;

	if (hasRoute) {	// entry already exist within the routing table, now compare the sequence numbers
		if (routeToNode.dest_seq_num < reversePathInfo.source_sequence_num) {
			memcpy(&routeToNode, &newRouteToNode, sizeof(RouteEntry_t));
			retVal = true;
		}
	} else {	// does not have a route, so add one
		add_route(newRouteToNode);
		retVal = true;
	}
	return retVal;
}

bool add_forward_path_to_table() {
	bool retVal = false;
	RouteEntry_t routeToNode;
	bool hasRoute = has_route_to_node(forwardPathInfo.destinationAddress,
			&routeToNode);
	RouteEntry_t newRouteToNode;
	newRouteToNode.dest = forwardPathInfo.destinationAddress;
	newRouteToNode.dest_seq_num = forwardPathInfo.dest_sequence_num;
	newRouteToNode.expiration_time = forwardPathInfo.expTime;
	newRouteToNode.next_hop = forwardPathInfo.nextHop;
	newRouteToNode.num_hops = forwardPathInfo.hopcount;

	if (hasRoute
			&& (routeToNode.dest_seq_num > forwardPathInfo.dest_sequence_num)) {// entry already exist within the routing table, now compare the sequence numbers

		memcpy(&routeToNode, &newRouteToNode, sizeof(RouteEntry_t));
		add_route(newRouteToNode);
		retVal = true;

	} else {	// does not have a route, so add one
		add_route(newRouteToNode);
		retVal = true;
	}
	return retVal;
}

extern bool isRoot;
bool send_rrep() {
	bool retVal = false;

	if (HasReversePathInfo && isRoot) {	// the gateway received a rreq, is sending a rrep
		RouteEntry_t newRoute;
		has_route_to_node(reversePathInfo.nextHop, &newRoute);

		txDatagram.msgHeader.flags = MSG_RREP;
		txDatagram.msgHeader.localSource = _nodeID;
		txDatagram.msgHeader.netDest = rxDatagram.msgHeader.netSource;
		txDatagram.msgHeader.netSource = _nodeID;
		txDatagram.msgHeader.nextHop = reversePathInfo.nextHop;
		txDatagram.msgHeader.hopCount = 0;
		txDatagram.msgHeader.txSlot = _txSlot;

		txDatagram.data.Rrep.dest_addr = rxDatagram.data.Rreq.source_addr;
		txDatagram.data.Rrep.dest_sequence_num = _nodeSequenceNumber;
		txDatagram.data.Rrep.hop_cnt = 0;
		txDatagram.data.Rrep.lifetime = MAX_HOPS;
		txDatagram.data.Rrep.source_addr = _nodeID;
		int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.data.Rrep);
		retVal = mac_send_tx_datagram(size);
		HasReversePathInfo = false;
	} else if (HasReversePathInfo && !isRoot) {
		RouteEntry_t newRoute;
		has_route_to_node(reversePathInfo.nextHop, &newRoute);

		txDatagram.msgHeader.flags = MSG_RREP;
		txDatagram.msgHeader.localSource = _nodeID;
		txDatagram.msgHeader.netDest = reversePathInfo.destinationAddress;
		txDatagram.msgHeader.netSource = rxDatagram.data.Rrep.source_addr;
		txDatagram.msgHeader.nextHop = reversePathInfo.nextHop;
		txDatagram.msgHeader.hopCount = reversePathInfo.hopcount + 1;
		txDatagram.msgHeader.txSlot = _txSlot;

		txDatagram.data.Rrep.dest_addr = rxDatagram.data.Rreq.source_addr;
		txDatagram.data.Rrep.dest_sequence_num =
				rxDatagram.data.Rrep.dest_sequence_num;
		txDatagram.data.Rrep.hop_cnt = rxDatagram.data.Rrep.hop_cnt + 1;
		txDatagram.data.Rrep.lifetime = MAX_HOPS;
		txDatagram.data.Rrep.source_addr = rxDatagram.data.Rrep.source_addr;
		int size = sizeof(txDatagram.msgHeader) + sizeof(txDatagram.data.Rrep);
		retVal = mac_send_tx_datagram(size);
		HasReversePathInfo = false;
	} else {
		retVal = false;
	}

	return retVal;
}

RouteEntry_t* get_routing_table() {
	return routingtable;
}
