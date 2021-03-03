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

int flash_write_buffer( ) {

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

int flash_read_buffer( ) {
	memcpy(&myFlashData, (uint32_t*) (MYDATA_MEM_START), MY_FLASH_DATA_LEN);
	return true;
}

int flash_erase_all( ) {
	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START)) while (1)
		;
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	return 0;
}

int flash_write_node_id( ) {
	myFlashData[NODE_ID_LOCATION] = _nodeID;
	return flash_write_buffer();
}

int flash_read_node_id( ) {
	myFlashData[NODE_ID_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_ID_LOCATION);
	_nodeID = myFlashData[NODE_ID_LOCATION];
	return _nodeID;
}

int flash_init_buffer( ) {
	memcpy(myFlashData, &myFlashDataStruct, sizeof(myFlashDataStruct));
	return true;
}

int flash_write_neighbours( ) {
	memcpy(
			&myFlashData + NODE_NEIGHBOUR_SYNC_TABLE_LOCATION,
			&neighbourTable,
			sizeof(neighbourTable));
	return flash_write_buffer();
}

int flash_read_neighbours( ) {
	myFlashData[NODE_NEIGHBOUR_TABLE_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_NEIGHBOUR_TABLE_LOCATION);
	memcpy(
			&neighbourTable,
			&myFlashData + NODE_NEIGHBOUR_TABLE_LOCATION,
			sizeof(neighbourTable));
	return true;
}

int flash_fill_struct_for_write( ) {
	myFlashDataStruct.flashValidIdentifier = FLASH_OK_IDENTIFIER;
	myFlashDataStruct.thisNodeId = _nodeID;
	myFlashDataStruct.lastWriteTime = RTC_C_getCalendarTime();
	Neighbour_t *ptr = get_neighbour_table();
	memcpy(&myFlashDataStruct.neighbourTable, &ptr, sizeof(*ptr));
	RouteEntry_t *ptr1 = get_routing_table();
	memcpy(&myFlashDataStruct.routingtable, &ptr1, sizeof(*ptr1));
	Datagram_t *ptr2 = get_received_messages();
	memcpy(&myFlashDataStruct.receivedMessages, &ptr2, sizeof(*ptr2));
	myFlashDataStruct.lenOfBuffer = 2 + 4 + sizeof(RTC_C_Calendar)
			+ sizeof(nodeAddress) + sizeof(*ptr) * MAX_NEIGHBOURS
			+ sizeof(*ptr1) * MAX_ROUTES + sizeof(*ptr2) * MAX_STORED_MSG;
	return 1;
}

int flash_write_struct_to_flash( ) {
	memcpy(&myFlashData, &myFlashDataStruct, MY_FLASH_DATA_LEN);
	flash_write_buffer();
	return 1;
}

int flash_read_from_flash( void ) {
	memcpy(&myFlashDataStruct, (uint8_t*) MYDATA_MEM_START, 1);
	return 1;
}
