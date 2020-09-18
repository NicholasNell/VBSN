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

#define MemLength 1

int flashWriteBuffer();
int flashWriteNodeID();
int flashReadNodeID();
int flashReadBuffer();
int flashEraseAll();
int flashInitBuffer();

#endif /* MY_FLASH_H_ */
