/*
 * datagram.c
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */


#include "datagram.h"

bool datagramInit( ) {
	bool ret = genericDriverInit();
	if (ret) setThisAddress(_thisAddress);
	return ret;
}

void setThisAddress( uint8_t thisAddress ) {
	_driver.setThisAddress(thisAddress);
	// Use this address in the transmitted FROM header
	setHeaderFrom(thisAddress);
	_thisAddress = thisAddress;
}

