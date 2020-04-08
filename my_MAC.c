/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"

#define ALOHA 1

bool MACSend( uint8_t *buffer, uint8_t size ) {
#ifdef ALOHA

	bool NotRXFlag = false;

	SX1276SetSleep();  // Clear IRQ flags
	SX1276Send(buffer, size);
	startTiming();
	uint8_t i = spiRead_RFM(REG_LR_IRQFLAGS);
	while (!( i & RFLR_IRQFLAGS_TXDONE_MASK)) {
		i = spiRead_RFM(REG_LR_IRQFLAGS);
		if (getTiming() > 5E6) {
			stopTiming();
			break;
		}
	}
	stopTiming();
	startTiming();
	SX1276SetRx(500);
	i = spiRead_RFM(REG_LR_IRQFLAGS);
	while (!( i & RFLR_IRQFLAGS_RXDONE_MASK)) {
		i = spiRead_RFM(REG_LR_IRQFLAGS);
		if (getTiming() > 5E6) {
			stopTiming();
			NotRXFlag = true;
			break;
		}
	}
	return NotRXFlag;

#else

#endif

}
