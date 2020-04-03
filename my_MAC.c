/*
 * my_MAC.c
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */
#include "my_MAC.h"

#define ALOHA 1


void MACSend( uint8_t *buffer, uint8_t size ) {
#ifdef ALOHA
	SX1276SetSleep();  // Clear IRQ flags
	SX1276Send(buffer, size);

//	SX1276SetRx(500);

#else

#endif

}



