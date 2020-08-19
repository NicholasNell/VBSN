/*
 * datagram.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include "genericDriver.h"
#include <stdbool.h>
#include <stdint.h>

#define RH_MAX_MESSAGE_LEN 255

/// Initialise this instance and the
/// driver connected to it.
bool datagramInit( );

/// Sets the address of this node. Defaults to 0.
/// This will be used to set the FROM address of all messages sent by this node.
/// In a conventional multinode system, all nodes will have a unique address
/// (which you could store in EEPROM).
/// \param[in] thisAddress The address of this node
void setThisAddress( uint8_t thisAddress );

#endif /* DATAGRAM_H_ */
