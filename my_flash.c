/** @file my_flash.c
 *
 * @brief
 *
 *  Created on: 17 Sep 2020
 *      Author: njnel
 */

#include <my_MAC.h>
#include "my_flash.h"
#include <myNet.h>

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
//#include <stdint.h>
//#include <stdbool.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/rtc_c.h>

#define MY_FLASH_DATA_LEN 4096

static bool flashOK = false;

static FlashData_t myFlashDataStruct;

static struct FlashOffset flashOffset;

static uint8_t myFlashData[MY_FLASH_DATA_LEN];
uint32_t lastWrite = MYDATA_MEM_START;

extern uint8_t _numRoutes;

void flash_init_offset(void) {
	int tempOffset = 0;
	flashOffset.flashValidIdentifier = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.flashValidIdentifier);
	flashOffset.lastWriteTime = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.lastWriteTime);
	flashOffset.thisNodeId = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.thisNodeId);
	flashOffset.neighbourTable = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.neighbourTable);
	flashOffset.routingtable = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.routingtable);
	flashOffset.receivedMessages = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.receivedMessages);
	flashOffset._numRoutes = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._numRoutes);
	flashOffset._nodeSequenceNumber = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._nodeSequenceNumber);
	flashOffset._numNeighbours = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._numNeighbours);
	flashOffset._broadcastID = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._broadcastID);
	flashOffset._txSlot = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._txSlot);

}

int flash_write_buffer() {

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR16);

	/* Trying to erase the sector. Within this function, the API will
	 automatically try to erase the maximum number of tries. If it fails,
	 trap in an infinite loop */
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START))
		return ERROR_Erase;

	/* Trying to program the memory. Within this function, the API will
	 automatically try to program the maximum number of tries. If it fails,
	 trap inside an infinite loop */
	if (!MAP_FlashCtl_programMemory(myFlashData, (void*) MYDATA_MEM_START,
			4096))
		return ERROR_Write;

	/* Setting the sector back to protected  */
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR16);
	return Completed;
}

int flash_read_buffer() {
	memcpy(&myFlashData, (uint32_t*) (MYDATA_MEM_START), MY_FLASH_DATA_LEN);
	return true;
}

int flash_erase_all() {
	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR16);
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START))
		while (1)
			;
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR16);
	return 0;
}

int flash_write_node_id() {
	myFlashData[NODE_ID_LOCATION] = get_node_id();
	return flash_write_buffer();
}

int flash_read_node_id() {
	myFlashData[NODE_ID_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_ID_LOCATION);
	set_node_id(myFlashData[NODE_ID_LOCATION]);
	return myFlashData[NODE_ID_LOCATION];
}

int flash_init_buffer() {
	memcpy(myFlashData, &myFlashDataStruct, sizeof(myFlashDataStruct));
	return true;
}

int flash_fill_struct_for_write() {
	myFlashDataStruct.flashValidIdentifier = FLASH_OK_IDENTIFIER;
	myFlashDataStruct.thisNodeId = get_node_id();
	myFlashDataStruct.lastWriteTime = RTC_C_getCalendarTime();
	if (get_num_neighbours() > 0) {
		Neighbour_t *ptr = get_neighbour_table();
		memcpy(&myFlashDataStruct.neighbourTable, &ptr, sizeof(*ptr));
	}
	RouteEntry_t *ptr1 = get_routing_table();
	memcpy(&myFlashDataStruct.routingtable, &ptr1, sizeof(*ptr1));
	if (get_received_messages_index() > 0) {
		Datagram_t *ptr2 = get_received_messages();
		memcpy(&myFlashDataStruct.receivedMessages, &ptr2, sizeof(*ptr2));
	}
	myFlashDataStruct._numRoutes = _numRoutes;
	myFlashDataStruct._nodeSequenceNumber = get_node_sequence_number();
	myFlashDataStruct._numNeighbours = get_num_neighbours();
	myFlashDataStruct._broadcastID = get_broadcast_id();
	myFlashDataStruct._txSlot = get_tx_slot();
	return 1;
}

int flash_write_struct_to_flash() {
	memcpy(&myFlashData, &myFlashDataStruct, sizeof(myFlashDataStruct));
	flash_write_buffer();
	return 1;
}

int flash_read_from_flash(void) {
	memcpy(&myFlashData, (uint8_t*) MYDATA_MEM_START, MY_FLASH_DATA_LEN);
	memcpy(&myFlashDataStruct, &myFlashData, sizeof(myFlashDataStruct));
	return 1;
}

bool flash_check_for_data(void) {
	flash_read_from_flash();
	if (myFlashDataStruct.flashValidIdentifier == FLASH_OK_IDENTIFIER) {
		flashOK = true;
		return true;
	} else {
		flashOK = false;
		return false;
	}

}

FlashData_t* get_flash_data_struct(void) {
	return &myFlashDataStruct;
}

bool get_flash_ok_flag() {
	return flashOK;
}
