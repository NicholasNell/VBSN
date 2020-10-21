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

#define SLOT_LENGTH_MS 5000
#define SLOT_SCALER 1
#define MAX_SLOT_COUNT (60.0 * 5.0 / SLOT_SCALER)
#define POSSIBLE_TX_SLOT (10 / SLOT_SCALER)
#define GLOBAL_RX (50 / SLOT_SCALER)
#define COLLECT_DATA_SLOT_REL 5

void initScheduler();
int scheduler();
uint16_t getSlotCount();
void setSlotCount(uint16_t newSlotCount);
void incrementSlotCount();

#endif /* MY_SCHEDULER_H_ */
