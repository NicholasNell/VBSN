/** @file my_scheduler.h
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#ifndef MY_SCHEDULER_H_
#define MY_SCHEDULER_H_

#include <stdint.h>

#define SLOT_LENGTH_MS 1000		// length of the time slots in ms
#define SLOT_SCALER 1			// used to increase the length of the time slots: only used for debugging
//#define MAX_SLOT_COUNT (60.0 * 5.0 / SLOT_SCALER)	// maximum slots, before counter resets
#define MAX_SLOT_COUNT 3600	// maximum slots, before counter resets
#define POSSIBLE_TX_SLOT 1		// possible slot modulus
#define GLOBAL_RX (10 / SLOT_SCALER)	// possible global rx modulus
#define COLLECT_DATA_SLOT_REL 10	// slot in which data is collected before a transmission
#define SYNC_PROB_ROUTE_DISC 20					// probability of sending out a sync message in a global rx slot.
#define SYNC_PROB_HELLO_MSG 5		// prob of root sending helo msg
#define TOTAL_NETWORK_NODES 5		// total nodes in the network. I've only made 5.
#define TIME_TO_SEND_SEC (60*5)		// the time between data transmissions
#define TIME_TO_SEND_DATA_OFFSET 25	//
#define TIME_TO_COLLECT_DATA_SEC 30 // time to collect data
#define TIME_TO_COLLECT_DATA_OFFSET	5	//
#define GSM_UPLOAD_DATAGRAMS_TIME 60	// time to upload datagrams
#define GSM_UPLOAD_DATAGRAM_OFFSET 55	// time offset from slot count
#define WRITE_FLASH_DATA_TIME 120		// Time between flash data writes
#define WRITE_FLASH_DATA_OFFSET	20		// write flash data offset
#define WINDOW_TIME_SEC (60*20)			//
#define WINDOW_SCALER (MAX_SLOT_COUNT / WINDOW_TIME_SEC)
#define GPS_WAKEUP_TIME (60*30)			// wake up gps every 15 minutes
#define FLASH_SAVE_DATA	(60 * 5)		// save flash data every 5 minutes

//!
//! \brief calculates a new sync probability based on the number of known neighbours.
bool get_sync(uint8_t per);

/*!
 * \brief returns the current slot count
 * @return
 */
uint16_t get_slot_count();

/*!
 * \brief sets the slot counter to the given value
 * @param newSlotCount
 */
void set_slot_count(uint16_t newSlotCount);

/*!
 * \brief increments the slot counter by one
 */
void increment_slot_count();

#endif /* MY_SCHEDULER_H_ */
