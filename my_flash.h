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

#define ERROR_Erase 1;
#define ERROR_Write 2;
#define Completed 0;

#define MYDATA_MEM_START 0x0003F000
#define NODE_ID_LOCATION 0
#define NODE_NEIGHBOUR_TABLE_LOCATION 1 // len = 255 bytes
#define NODE_NEIGHBOUR_SYNC_TABLE_LOCATION 256 // len = 255 *2

#define MemLength 766

int flashWriteBuffer();
int flashWriteNodeID();
int flashReadNodeID();
int flashReadBuffer();
int flashEraseAll();
int flashInitBuffer();
int flashWriteNeighbours();
int flashReadNeighbours();

#endif /* MY_FLASH_H_ */
