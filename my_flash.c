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

static bool flashOK = false;

static FlashData_t myFlashDataStruct;

static struct FlashOffset flashOffset;

static uint8_t myFlashData[FLASH_DATA_SIZE];

void flash_init_offset(void) {
	int tempOffset = 0;
	flashOffset.flashValidIdentifier = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.flashValidIdentifier);
	flashOffset.lastWriteTime = tempOffset;
	tempOffset += sizeof(myFlashDataStruct.lastWriteTime);
	flashOffset._nodeSequenceNumber = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._nodeSequenceNumber);
	flashOffset._broadcastID = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._broadcastID);
	flashOffset._numDataSent = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._numDataSent);
	flashOffset._totalMsgSent = tempOffset;
	tempOffset += sizeof(myFlashDataStruct._totalMsgSent);
}

int flash_write_buffer() {

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,
			flash_calculate_sector(MYDATA_MEM_START));

	/* Trying to erase the sector. Within this function, the API will
	 automatically try to erase the maximum number of tries. If it fails,
	 trap in an infinite loop */
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START))
		return ERROR_Erase;

	/* Trying to program the memory. Within this function, the API will
	 automatically try to program the maximum number of tries. If it fails,
	 trap inside an infinite loop */
	if (!MAP_FlashCtl_programMemory(myFlashData, (void*) MYDATA_MEM_START,
	FLASH_DATA_SIZE))
		return ERROR_Write;

	/* Setting the sector back to protected  */
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,
			flash_calculate_sector(MYDATA_MEM_START));
	return Completed;
}

int flash_erase_all() {
	/* Unprotecting Info Bank 1  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,
			flash_calculate_sector(MYDATA_MEM_START));
	if (!MAP_FlashCtl_eraseSector(MYDATA_MEM_START))
		while (1)
			;
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1,
			flash_calculate_sector(MYDATA_MEM_START));
	return 0;
}

int flash_init_buffer() {
	memset(myFlashData, 0xFF, sizeof(myFlashDataStruct));
	return true;
}

int flash_fill_struct_for_write() {
	myFlashDataStruct.flashValidIdentifier = FLASH_OK_IDENTIFIER;
	flash_write_num_data_sent();
	flash_write_total_msg_sent();
	flash_write_rtc_time();
	flash_write_node_seq_num();
	flash_write_broadcast_id();
	return 1;
}

int flash_write_struct_to_flash() {
	memcpy(&myFlashData, &myFlashDataStruct, sizeof(myFlashDataStruct));
	flash_write_buffer();
	return 1;
}

int flash_read_from_flash(void) {
	memcpy(&myFlashData, (uint8_t*) MYDATA_MEM_START, FLASH_DATA_SIZE);
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

uint32_t flash_calculate_sector(uint32_t Address) {
	int sector = ((Address - 0x20000) / 4096);
	uint32_t retVal = NULL;
	switch (sector) {
	case 0:
		retVal = FLASH_SECTOR0;
		break;
	case 1:
		retVal = FLASH_SECTOR1;
		break;
	case 2:
		retVal = FLASH_SECTOR2;
		break;
	case 3:
		retVal = FLASH_SECTOR3;
		break;
	case 4:
		retVal = FLASH_SECTOR4;
		break;
	case 5:
		retVal = FLASH_SECTOR5;
		break;
	case 6:
		retVal = FLASH_SECTOR6;
		break;
	case 7:
		retVal = FLASH_SECTOR7;
		break;
	case 8:
		retVal = FLASH_SECTOR8;
		break;
	case 9:
		retVal = FLASH_SECTOR9;
		break;
	case 10:
		retVal = FLASH_SECTOR10;
		break;
	case 11:
		retVal = FLASH_SECTOR11;
		break;
	case 12:
		retVal = FLASH_SECTOR12;
		break;
	case 13:
		retVal = FLASH_SECTOR13;
		break;
	case 14:
		retVal = FLASH_SECTOR14;
		break;
	case 15:
		retVal = FLASH_SECTOR15;
		break;
	case 16:
		retVal = FLASH_SECTOR16;
		break;
	case 17:
		retVal = FLASH_SECTOR17;
		break;
	case 18:
		retVal = FLASH_SECTOR18;
		break;
	case 19:
		retVal = FLASH_SECTOR19;
		break;
	case 20:
		retVal = FLASH_SECTOR20;
		break;
	case 21:
		retVal = FLASH_SECTOR21;
		break;
	case 22:
		retVal = FLASH_SECTOR22;
		break;
	case 23:
		retVal = FLASH_SECTOR23;
		break;
	case 24:
		retVal = FLASH_SECTOR24;
		break;
	case 25:
		retVal = FLASH_SECTOR25;
		break;
	case 26:
		retVal = FLASH_SECTOR26;
		break;
	case 27:
		retVal = FLASH_SECTOR27;
		break;
	case 28:
		retVal = FLASH_SECTOR28;
		break;
	case 29:
		retVal = FLASH_SECTOR29;
		break;
	case 30:
		retVal = FLASH_SECTOR30;
		break;
	case 31:
		retVal = FLASH_SECTOR31;
		break;
	default:
		break;
	}
	return retVal;
}

RTC_C_Calendar flash_get_last_write_time() {
	memcpy(&myFlashDataStruct.lastWriteTime,
			(void*) (MYDATA_MEM_START + flashOffset.lastWriteTime),
			sizeof(myFlashDataStruct.lastWriteTime));
	return myFlashDataStruct.lastWriteTime;
}

uint8_t flash_get_num_data_sent() {
	memcpy(&myFlashDataStruct._numDataSent,
			(void*) (MYDATA_MEM_START + flashOffset._numDataSent),
			sizeof(myFlashDataStruct._numDataSent));
	return myFlashDataStruct._numDataSent;
}

uint8_t flash_get_total_msg_sent() {
	memcpy(&myFlashDataStruct._totalMsgSent,
			(void*) (MYDATA_MEM_START + flashOffset._totalMsgSent),
			sizeof(myFlashDataStruct._totalMsgSent));
	return myFlashDataStruct._totalMsgSent;
}

uint8_t flash_get_node_seq_num() {
	memcpy(&myFlashDataStruct._nodeSequenceNumber,
			(void*) (MYDATA_MEM_START + flashOffset._nodeSequenceNumber),
			sizeof(myFlashDataStruct._nodeSequenceNumber));
	return myFlashDataStruct._nodeSequenceNumber;
}

uint8_t flash_get_broadcast_id() {
	memcpy(&myFlashDataStruct._broadcastID,
			(void*) (MYDATA_MEM_START + flashOffset._broadcastID),
			sizeof(myFlashDataStruct._broadcastID));
	return myFlashDataStruct._broadcastID;
}

void flash_write_num_data_sent() {
	myFlashDataStruct._numDataSent = get_num_data_msg_sent();
}

void flash_write_total_msg_sent() {
	myFlashDataStruct._numDataSent = get_total_msg_sent();
}

void flash_write_rtc_time() {
	myFlashDataStruct.lastWriteTime = RTC_C_getCalendarTime();
}

void flash_write_node_seq_num() {
	myFlashDataStruct._nodeSequenceNumber = get_node_sequence_number();
}

void flash_write_broadcast_id() {
	myFlashDataStruct._broadcastID = get_broadcast_id();
}
