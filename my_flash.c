/** @file my_flash.c
 *
 * @brief
 *
 *  Created on: 17 Sep 2020
 *      Author: njnel
 */

#include "my_flash.h"
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

/* Standard Includes */
#include <stdint.h>

#include <stdbool.h>
#include <string.h>

#define MY_FLASH_DATA_LEN 4096

static uint8_t myFlashData[MY_FLASH_DATA_LEN];
uint32_t lastWrite = MYDATA_MEM_START;

extern uint8_t _nodeID;

int flashWriteBuffer() {

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

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
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

	return Completed;
}

int flashReadBuffer() {

	/* Unprotecting Info Bank 0, Sector 0  */
	uint8_t i = 0;
	for (i = 0; i <= MemLength; i++) {
		myFlashData[i] = *(uint8_t*) (i + MYDATA_MEM_START);
	}
}

int flashEraseAll() {
	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START))
		while (1)
			;
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	return 0;
}

int flashWriteNodeID() {
	myFlashData[NODE_ID_LOCATION] = _nodeID;
	return flashWriteBuffer();
}

int flashReadNodeID() {
	myFlashData[NODE_ID_LOCATION] = *(uint8_t*) (MYDATA_MEM_START
			+ NODE_ID_LOCATION);
	_nodeID = myFlashData[NODE_ID_LOCATION];
	return _nodeID;
}

int flashInitBuffer() {
	memset(myFlashData, 0xFF, MY_FLASH_DATA_LEN);
}
