/** @file my_flash.h
 *
 * @brief
 *
 *  Created on: 17 Sep 2020
 *      Author: njnel
 */

#ifndef MY_FLASH_H_
#define MY_FLASH_H_
#include <stdint.h>
#include "C:/ti/simplelink_msp432p4_sdk_3_40_01_02/source/ti/devices/msp432p4xx/driverlib/rtc_c.h"
#include "datagram.h"
#include "my_MAC.h"
#include "myNet.h"

#define ERROR_Erase 1;
#define ERROR_Write 2;
#define Completed 0;

#define FLASH_OK_IDENTIFIER 0xABCDEFAB

typedef struct {

		int lenOfBuffer;
		uint32_t flashValidIdentifier;
		RTC_C_Calendar lastWriteTime;
		nodeAddress thisNodeId;
		Neighbour_t neighbourTable[MAX_NEIGHBOURS];
		RouteEntry_t routingtable[MAX_ROUTES];
		Datagram_t receivedMessages[MAX_STORED_MSG];
} FlashData_t;

#define MYDATA_MEM_START 0x0003F000
#define NODE_ID_LOCATION 0
#define NODE_NEIGHBOUR_TABLE_LOCATION 1 // len = 255 bytes
#define NODE_NEIGHBOUR_SYNC_TABLE_LOCATION 256 // len = 255 *2

#define MemLength 766

int flashWriteBuffer( );
int flashWriteNodeID( );
int flashReadNodeID( );
int flashReadBuffer( );
int flashEraseAll( );
int flashInitBuffer( );
int flashWriteNeighbours( );
int flashReadNeighbours( );

/*!
 * \brief fills the flash data struct from local memory
 * @return
 */
int flashFillStructForWrite( );

/*!
 *  \brief fill the struct from flash memory
 * @return
 */
int flashFillStructFromMem( );

#endif /* MY_FLASH_H_ */
