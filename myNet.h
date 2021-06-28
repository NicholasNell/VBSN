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
#define MAX_HOPS 5
#define REVERSE_PATH_EXP_TIME 60

typedef struct {
	nodeAddress dest;
	uint8_t next_hop;
	uint8_t num_hops;
	uint8_t dest_seq_num;
	uint16_t expiration_time;
} RouteEntry_t;

typedef struct {
	nodeAddress sourceAddress; // the source of the rreq (ie the destination of the reverse path)
	nodeAddress localSourceAddress;	// the next hop on the way to the source of the rreq
	uint8_t hopcount;		// the number of hops to the source of the rreq

	uint8_t broadcastID;	// RReq boradcast ID (used to identify the rreq)
	uint8_t expTime;			// expiration time of the reverse path info
	uint8_t source_sequence_num;// source sequence number used to determine route freshness

} ReversePathInfo_t;

typedef struct {
	nodeAddress destinationAddress; // the final destination of the rreq, specified by the rrep
	nodeAddress nextHop;	// the node from which the rrep was received
	uint8_t hopcount;			// the number of hnops to the destination

	uint8_t expTime;			// expiration time of the reverse path info
	uint8_t dest_sequence_num; // destination sequence number used to determine route freshness
} ForwardPathInfo_t;

typedef enum netOps {
	NET_NONE,
	NET_REBROADCAST_RREQ,
	NET_BROADCAST_RREQ,
	NET_UNICAST_RREP,
	NET_WAIT,

} NextNetOp_t;

void net_init();

/*!
 * Add new entry to the routing table
 * @param dest
 */
void add_route(RouteEntry_t routeToNode);

/*!
 *  get next hop in path to given destination
 * @param dest
 * @return
 */
nodeAddress get_dest(nodeAddress dest);

/*!
 * send initial rreq for route discovery
 * @return return true if succesful
 */
bool send_rreq();

/*!
 * \brief	Rebroadcast rreq
 */
bool net_re_rreq();

// process RReq and determine if the message need to be broadcast or if a RRep needs to be sent

/*!
 *  \brief process RReq and determine if the message need to be broadcast or if a RRep needs to be sent
 * @return message type to be sent
 */
NextNetOp_t process_rreq();

/*!
 * Adds a neighbouring node to the routing table.
 * @param dest: the neighbour ID
 */
void add_route_to_neighbour(nodeAddress dest);

/*!
 * \brief looks for a route in the routing table and save a copy of the entry if one is found, otherwise saves NULL
 * @param dest
 * @return return true if it has a route to the node
 */

bool has_route_to_node(nodeAddress dest, RouteEntry_t *routeToNode);

/*!
 * \brief	determines what to do after the rrep has been received
 * @return	returns the next network operation
 */
NextNetOp_t process_rrep();

/*!
 *
 * \brief Adds the current reverse path to the routing table
 * @return returns true if route was added to path, false if not
 * wont add route to table if a newer one exists in the table.
 */
bool add_reverse_path_to_table();

/*!
 * \brief Adds the current forawrd path to the routing table.
 * @return return true if route was added to table
 */
bool add_forward_path_to_table();

/*!
 * \brief send a unicast rrep back to last hop from which a rreq was received
 * @return returns true if message was sent succesfully
 */
bool send_rrep();

/*!
 *
 * @return a ptr to routingtable;
 */
RouteEntry_t* get_routing_table();

//! @param node node to be removed
void remove_route_with_node(nodeAddress node);

uint8_t get_num_routes();

void reset_num_routes();

uint8_t get_node_sequence_number();

uint8_t get_broadcast_id();

uint8_t get_distance_to_gateway();

#endif /* MYNET_H_ */
