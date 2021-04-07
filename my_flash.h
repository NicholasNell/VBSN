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
		uint8_t _numRoutes;
		uint8_t _nodeSequenceNumber;
		uint8_t _numNeighbours;
		uint8_t _broadcastID;
		uint8_t _destSequenceNumber;
		uint16_t _txSlot;
} FlashData_t;

#define MYDATA_MEM_START 0x0003F000
#define NODE_ID_LOCATION 0
#define NODE_NEIGHBOUR_TABLE_LOCATION 1 // len = 255 bytes
#define NODE_NEIGHBOUR_SYNC_TABLE_LOCATION 256 // len = 255 *2

#define MemLength 766

int flash_write_buffer( );
int flash_write_node_id( );
int flash_read_node_id( );
int flash_read_buffer( );
int flash_erase_all( );
int flash_init_buffer( );
int flash_write_neighbours( );
int flash_read_neighbours( );

//! Fills the structure with current values from RAM
//! @return None

int flash_fill_struct_for_write( );

/*!
 *  \brief fill the struct from flash memory
 * @return
 */
int flash_fill_struct_from_mem( );

/*!
 *  \brief	writes the struct to the flash memory
 * @return
 */
int flash_write_struct_to_flash( );

//!	fills the data struct with data from flash
//!
//! @return None
int flash_read_from_flash( void );

//! @return return true if flash data has been written else false
//! \brief checks the flash buffer for valid data and the loads it into sram
bool flash_check_for_data( void );

//! @return returns local flash data buffer
FlashData_t* get_flash_data_struct( void );

#endif /* MY_FLASH_H_ */
