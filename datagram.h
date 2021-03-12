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

// msgType Flags
//#define MSG_NONE	0b00000000
//#define MSG_RTS 	0b00000001
//#define MSG_CTS 	0b00000010
//#define MSG_DATA 	0b00000100
//#define MSG_ACK 	0b00001000
//#define MSG_SYNC 	0b00010000
//#define MSG_RREQ 	0b00100000
//#define MSG_RREP	0b01000000

typedef enum msgFlags {
	MSG_NONE = 0,
	MSG_RTS,
	MSG_CTS,
	MSG_DATA,
	MSG_ACK,
	MSG_SYNC,
	MSG_RREQ,
	MSG_RREP
} msgType_t;

typedef uint8_t nodeAddress;

typedef struct {
		nodeAddress nextHop; // Where the message needs to go (MAC LAYER, not final destination);
		nodeAddress localSource; // Where the message came from, not original source
		nodeAddress netSource;		// original message source
		nodeAddress netDest;		// final destination, probably gateway
		uint8_t ttl;				// maximum number of hops
		uint16_t txSlot; 			// txSlot
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
		nodeAddress dest_addr;// destination address: will probably be a gateway
		uint8_t dest_sequence_num;		// destination sequence number
		uint8_t hop_cnt;		// number of hops the message has gone through
} RReq_t;

typedef struct {
		nodeAddress source_addr;// source address, will probably be a gateway
		nodeAddress dest_addr;// destination address: will probably be a sensor
		uint8_t dest_sequence_num;	// destination sequence number
		uint8_t hop_cnt;	// number of hops until now
		uint8_t lifetime;	// TTL?
} RRep_t;

typedef struct {
		Header_t msgHeader;
		RadioData_t radioData;
		union Data {
				RReq_t Rreq;
				RRep_t Rrep;
				MsgData_t sensData;
		} data;

} Datagram_t;

#endif /* DATAGRAM_H_ */
