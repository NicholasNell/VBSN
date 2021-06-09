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
#define NODE_ID_LOCATION 0
#define NODE_NEIGHBOUR_TABLE_LOCATION 1 // len = 255 bytes
#define NODE_NEIGHBOUR_SYNC_TABLE_LOCATION 256 // len = 255 *2
#define FLASH_DATA_SIZE 38

typedef struct {
	uint32_t flashValidIdentifier; 	// 4  bytes
	RTC_C_Calendar lastWriteTime;  	// 28 bytes
	uint8_t _nodeSequenceNumber;   	// 1  byte
	uint8_t _broadcastID;          	// 1  byte
	uint16_t _numDataSent;			// 2 bytes
	uint16_t _totalMsgSent;			// 2 bytes
} FlashData_t;						// 38 bytes

struct FlashOffset {
	int flashValidIdentifier;
	int lastWriteTime;
	int _nodeSequenceNumber;
	int _broadcastID;
	int _numDataSent;
	int _totalMsgSent;
};

#define MYDATA_MEM_START 0x00020000

int flash_write_buffer();
int flash_erase_all();
int flash_init_buffer();

//!
//! @brief	sets the memory offsets for use when storing and reading from flash
void flash_init_offset(void);

//! Fills the structure with current values from RAM
//! @return None

int flash_fill_struct_for_write();

/*!
 *  \brief	writes the struct to the flash memory
 * @return
 */
int flash_write_struct_to_flash();

//!	fills the data struct with data from flash
//!
//! @return None
int flash_read_from_flash(void);

//! @return return true if flash data has been written else false
//! \brief checks the flash buffer for valid data and the loads it into sram
bool flash_check_for_data(void);

//! @return returns local flash data buffer
FlashData_t* get_flash_data_struct(void);

bool get_flash_ok_flag();

uint32_t flash_calculate_sector(uint32_t Address);

RTC_C_Calendar flash_get_last_write_time();
uint8_t flash_get_broadcast_id();
uint8_t flash_get_num_data_sent();
uint8_t flash_get_total_msg_sent();
uint8_t flash_get_node_seq_num();
uint8_t flash_get_broadcast_id();

void flash_write_num_data_sent();
void flash_write_total_msg_sent();
void flash_write_rtc_time();
void flash_write_node_seq_num();
void flash_write_broadcast_id();

#endif /* MY_FLASH_H_ */
