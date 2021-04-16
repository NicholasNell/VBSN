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
#define MAX_SLOT_COUNT (60.0 * 1.0 / SLOT_SCALER)	// maximum slots, before counter resets
#define POSSIBLE_TX_SLOT 1		// possible slot modulus
#define GLOBAL_RX (5 / SLOT_SCALER)	// possible global rx modulus
#define COLLECT_DATA_SLOT_REL 10	// slot in which data is collected before a transmission
#define SYNC_PROB 5					// probability of sending out a sync message in a global rx slot.
#define TOTAL_NETWORK_NODES 5		// total nodes in the network. I've only made 5.

/*!
 * \brief	inititial the scheduler, set tx slot and data collection slot
 */
void init_scheduler( );

/*!
 *  \brief	decides what happes in this particular slot
 * @return int return value
 */
int scheduler( );

/*!
 * \brief returns the current slot count
 * @return
 */
uint16_t get_slot_count( );

/*!
 * \brief sets the slot counter to the given value
 * @param newSlotCount
 */
void set_slot_count( uint16_t newSlotCount );

/*!
 * \brief increments the slot counter by one
 */
void increment_slot_count( );

#endif /* MY_SCHEDULER_H_ */
