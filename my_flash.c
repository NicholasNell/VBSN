/** @file my_flash.c
 *
 * @brief
 *
 *  Created on: 17 Sep 2020
 *      Author: njnel
 */

#include <my_MAC.h>
#include "my_flash.h"
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>

#include <stdbool.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

#define MY_FLASH_DATA_LEN 4096

static FlashData_t myFlashDataStruct;

static uint8_t myFlashData[MY_FLASH_DATA_LEN];
uint32_t lastWrite = MYDATA_MEM_START;

extern uint8_t _nodeID;

// array of known neighbours
extern Neighbour_t neighbourTable[MAX_NEIGHBOURS];

int flashWriteBuffer( ) {

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

	/* Trying to erase the sector. Within this function, the API will
	 automatically try to erase the maximum number of tries. If it fails,
	 trap in an infinite loop */
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START)) return ERROR_Erase;

	/* Trying to program the memory. Within this function, the API will
	 automatically try to program the maximum number of tries. If it fails,
	 trap inside an infinite loop */
	if (!MAP_FlashCtl_programMemory(
			myFlashData,
			(void*) MYDATA_MEM_START,
			4096)) return ERROR_Write;

	/* Setting the sector back to protected  */
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

	return Completed;
}

int flashReadBuffer( ) {

	/* Unprotecting Info Bank 0, Sector 0  */
//	uint8_t i = 0;
//	for (i = 0; i <= MemLength; i++) {
//		myFlashData[i] = *(uint8_t*) (i + MYDATA_MEM_START);
//	}
	memcpy(&myFlashData, (uint32_t*) (MYDATA_MEM_START), MY_FLASH_DATA_LEN);
	return true;
}

int flashEraseAll( ) {
	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START)) while (1)
		;
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	return 0;
}

int flashWriteNodeID( ) {
	myFlashData[NODE_ID_LOCATION] = _nodeID;
	return flashWriteBuffer();
}

int flashReadNodeID( ) {
	myFlashData[NODE_ID_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_ID_LOCATION);
	_nodeID = myFlashData[NODE_ID_LOCATION];
	return _nodeID;
}

int flashInitBuffer( ) {
	memcpy(myFlashData, &myFlashDataStruct, sizeof(myFlashDataStruct));
	return true;
}

int flashWriteNeighbours( ) {
	memcpy(
			&myFlashData + NODE_NEIGHBOUR_SYNC_TABLE_LOCATION,
			&neighbourTable,
			sizeof(neighbourTable));
	return flashWriteBuffer();
}

int flashReadNeighbours( ) {
	myFlashData[NODE_NEIGHBOUR_TABLE_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_NEIGHBOUR_TABLE_LOCATION);
	memcpy(
			&neighbourTable,
			&myFlashData + NODE_NEIGHBOUR_TABLE_LOCATION,
			sizeof(neighbourTable));
	return true;
}

int flashFillStructForWrite( ) {
	myFlashDataStruct.flashValidIdentifier = FLASH_OK_IDENTIFIER;
	myFlashDataStruct.thisNodeId = _nodeID;
	myFlashDataStruct.lastWriteTime = RTC_C_getCalendarTime();
	Neighbour_t *ptr = getNeighbourTable();
	memcpy(&myFlashDataStruct.neighbourTable, &ptr, sizeof(*ptr));
	RouteEntry_t *ptr1 = getRoutingTable();
	memcpy(&myFlashDataStruct.routingtable, &ptr1, sizeof(*ptr1));
	Datagram_t *ptr2 = getReceivedMessages();
	memcpy(&myFlashDataStruct.receivedMessages, &ptr2, sizeof(*ptr2));
	myFlashDataStruct.lenOfBuffer = 2 + 4 + sizeof(RTC_C_Calendar)
			+ sizeof(nodeAddress) + sizeof(*ptr) * MAX_NEIGHBOURS
			+ sizeof(*ptr1) * MAX_ROUTES + sizeof(*ptr2) * MAX_STORED_MSG;
	return 1;
}
