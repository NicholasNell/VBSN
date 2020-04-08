/*
 * my_ALOHA.h
 *
 *  Created on: 02 Apr 2020
 *      Author: Nicholas
 */

#ifndef MY_ALOHA_H_
#define MY_ALOHA_H_

#include <stdint.h>
#include <stdbool.h>
#include "my_gpio.h"
#include "my_spi.h"
#include "radio.h"
#include "my_timer.h"
#include "my_RFM9x.h"
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include <string.h>

typedef enum {
	IDLE = 0, PENDING, RETRANSMIT, EXPIRED, ACK_RESP
} AlohaState_t;

uint32_t AlohaAckTimeout;
float delay;
int attempts;

typedef struct
{
    uint8_t fid;
    uint8_t no;
} HeaderStruct;

typedef struct
{
    uint8_t pd0;
    uint8_t pd1;
} DataStruct;

void createAlohaPacket(uint8_t *output, HeaderStruct *header, DataStruct *data);

bool dissectAlohaPacket(uint8_t *input, HeaderStruct *header, DataStruct *data);

AlohaState_t state;

bool sendALOHAmessage( uint8_t *buffer, uint8_t size );

#endif /* MY_ALOHA_H_ */
