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

uint32_t lastWrite = MYDATA_MEM_START;

int writeVarToFlash(uint8_t *s1, uint16_t len, uint32_t adr) {

	uint32_t writeAdr;
	if (adr == NULL) {
		writeAdr = lastWrite;
	} else {
		writeAdr = adr;
	}

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

	/* Trying to program the memory. Within this function, the API will
	 automatically try to program the maximum number of tries. If it fails,
	 trap inside an infinite loop */

	if (!MAP_FlashCtl_programMemory(s1, (void*) writeAdr, len))
		while (1)
			;
	lastWrite = writeAdr + len;
	/* Setting the sector back to protected  */
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);

	return 0;
}

int readVarFromFlash(void *s1, uint16_t len, uint32_t adr) {

	/* Unprotecting Info Bank 0, Sector 0  */
	MAP_FlashCtl_unprotectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	if (adr == NULL) {
		memcpy(s1, (uint32_t*) --lastWrite, len);
	} else {
		memcpy(s1, (uint32_t*) adr, len);
	}

//	memcpy(&var1LOCAL, (uint16_t*) MYDATA_MEM_START, 2);
	MAP_FlashCtl_protectSector(FLASH_MAIN_MEMORY_SPACE_BANK1, FLASH_SECTOR31);
	return 0;
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
