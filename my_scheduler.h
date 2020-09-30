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

#define MAX_SLOT_COUNT 60 * 20
#define SLOT_LENGTH_MS 1000

void initScheduler();
int scheduler();
uint16_t getSlotCount();
void setSlotCount(uint16_t newSlotCount);
void incrementSlotCount();

#endif /* MY_SCHEDULER_H_ */
