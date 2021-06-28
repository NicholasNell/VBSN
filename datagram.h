/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <my_gps.h>
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

#define RH_MAX_MESSAGE_LEN 255

typedef enum msgFlags {
	MSG_NONE = 0,
	MSG_RTS,
	MSG_CTS,
	MSG_DATA,
	MSG_ACK,
	MSG_SYNC,
	MSG_RREQ,
	MSG_RREP,
	MSG_BROKEN_LINK
} msgType_t;

typedef uint8_t nodeAddress;

typedef struct {
	nodeAddress nextHop; // Where the message needs to go (MAC LAYER, not final destination);
	nodeAddress localSource; // Where the message came from, not original source
	nodeAddress netSource;		// original message source
	nodeAddress netDest;		// final destination, probably gateway
	uint8_t hopCount;					// message ID
	uint16_t txSlot; 			// txSlot
	uint16_t curSlot;			// This nodes current slotCount
	msgType_t flags;	 		// see flags
} Header_t;

typedef struct {
	int16_t rssi;
	int8_t snr;
} RadioData_t;

typedef struct {
	LocationData gpsData;
	float temp;
	float hum;
	float press;
	float lux;
	RTC_C_Calendar tim;
	float soilMoisture;
} MsgData_t;

typedef struct {
	nodeAddress source_addr;		// where the RReq came from
	uint8_t source_sequence_num;// the sequence number currently used by the source
	uint8_t broadcast_id;// The broadcast ID used by the source, ie the number of RReq's it has sent
	nodeAddress dest_addr;	// destination address: will probably be a gateway
	uint8_t dest_sequence_num;		// destination sequence number
} RReq_t;

typedef struct {
	nodeAddress source_addr;	// source address, will probably be a sensor
	nodeAddress dest_addr;	// destination address: will probably be a gateway
	uint8_t dest_sequence_num;	// destination sequence number
	uint8_t lifetime;	// TTL?
} RRep_t;

typedef struct {
	uint8_t numNeighbours;
	uint8_t numRoutes;
	uint16_t numDataSent;
	uint16_t rtsMissed;
	uint16_t timeToRoute;
	uint16_t totalMsgSent;
	uint8_t distToGate;
	uint16_t rreqTx;
	uint16_t rrepTx;
	uint16_t rreqReTx;
} NetData_t;

typedef struct {
	Header_t msgHeader;

	union Data {
		RReq_t Rreq;
		RRep_t Rrep;
		MsgData_t sensData;
	} data;
	NetData_t netData;
	RadioData_t radioData;

} Datagram_t;

#endif /* DATAGRAM_H_ */
