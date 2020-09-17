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

#define MYDATA_MEM_START 0x0003F000
#define NODE_ID_LOCATION 0x0003F000

int writeVarToFlash(uint8_t *s1, uint16_t len, uint32_t adr);
int readVarFromFlash(void *s1, uint16_t len, uint32_t adr);
int flashEraseAll();

#endif /* MY_FLASH_H_ */
