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

#define MAX_ROUTES 20
#define MAX_HOPS 4
#define REVERSE_PATH_EXP_TIME 100

typedef struct {
		nodeAddress dest;
		uint8_t next_hop;
		uint8_t num_hops;
		uint8_t dest_seq_num;
		uint16_t expiration_time;
} RouteEntry_t;

typedef struct {
		nodeAddress destinationAddress;	// the source of the rreq (ie the destination of the reverse path)
		nodeAddress nextHop;// the next hop on the way to the source of the rreq
		uint8_t hopcount;		// the number of hops to the source of the rreq

		uint8_t broadcastID;	// RReq boradcast ID (used to identify the rreq)
		uint8_t expTime;			// expiration time of the reverse path info
		uint8_t source_sequence_num;// source sequence number used to determine route freshness

} ReversePathInfo_t;

typedef struct {
		nodeAddress destinationAddress; // the final destination of the rreq, specified by the rrep
		nodeAddress nextHop;		// the node from which the rrep was received
		uint8_t hopcount;			// the number of hnops to the destination

		uint8_t expTime;			// expiration time of the reverse path info
		uint8_t dest_sequence_num; // destination sequence number used to determine route freshness
} ForwardPathInfo_t;

typedef enum netOps {
	NET_NONE,
	NET_REBROADCAST_RREQ,
	NET_BROADCAST_RREQ,
	NET_UNICAST_RREP,
	NET_WAIT
} NextNetOp_t;

void netInit( );

/*!
 * Add new entry to the routing table
 * @param dest
 */
void addRoute( RouteEntry_t routeToNode );

/*!
 *  get next hop in path to given destination
 * @param dest
 * @return
 */
nodeAddress getDest( nodeAddress dest );

/*!
 * send initial rreq for route discovery
 * @return return true if succesful
 */
bool sendRREQ( );

/*!
 * \brief	Rebroadcast rreq
 */
bool netReRReq( );

// process RReq and determine if the message need to be broadcast or if a RRep needs to be sent

/*!
 *  \brief process RReq and determine if the message need to be broadcast or if a RRep needs to be sent
 * @return message type to be sent
 */
NextNetOp_t processRreq( );

/*!
 * Adds a neighbouring node to the routing table.
 * @param dest: the neighbour ID
 */
void addRoutetoNeighbour( nodeAddress dest );

/*!
 * \brief looks for a route in the routing table and returns a copy of the entry if one is found, otherwise returns NULL
 * @param dest
 * @return return true if it has a route to the node
 */

bool hasRouteToNode( nodeAddress dest, RouteEntry_t *routeToNode );

/*!
 * \brief	determines what to do after the rrep has been received
 * @return	returns the next network operation
 */
NextNetOp_t processRrep( );

/*!
 *
 * \brief Adds the current reverse path to the routing table
 * @return returns true if route was added to path, false if not
 * wont add route to table if a newer one exists in the table.
 */
bool addReversePathToTable( );

/*!
 * \brief Adds the current forawrd path to the routing table.
 * @return return true if route was added to table
 */
bool addForwardPathToTable( );

/*!
 * \brief send a unicast rrep back to last hop from which a rreq was received
 * @return returns true if message was sent succesfully
 */
bool sendRRep( );

#endif /* MYNET_H_ */
