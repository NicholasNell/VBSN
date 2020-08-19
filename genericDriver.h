/*
 * genericDriver.h
 *
 *  Created on: 19 Aug 2020
 *      Author: nicholas
 */

#ifndef GENERICDRIVER_H_
#define GENERICDRIVER_H_

#include <stdint.h>
#include <stdbool.h>

#define RH_FLAGS_RESERVED                 0xf0
#define RH_FLAGS_APPLICATION_SPECIFIC     0x0f
#define RH_FLAGS_NONE                     0

typedef enum {
	RHModeInitialising = 0, ///< Transport is initialising. Initial default value until init() is called..
	RHModeSleep, ///< Transport hardware is in low power sleep mode (if supported)
	RHModeIdle,             ///< Transport is idle.
	RHModeTx,        ///< Transport is in the process of transmitting a message.
	RHModeRx,           ///< Transport is in the process of receiving a message.
	RHModeCad ///< Transport is in the process of detecting channel activity (if supported)
} RHMode;

/// The current transport operating mode
volatile RHMode _mode;

/// This node id
uint8_t _thisAddress;

/// Whether the transport is in promiscuous mode
bool _promiscuous;

/// TO header in the last received mesasge
volatile uint8_t _rxHeaderTo;

/// FROM header in the last received mesasge
volatile uint8_t _rxHeaderFrom;

/// ID header in the last received mesasge
volatile uint8_t _rxHeaderId;

/// FLAGS header in the last received mesasge
volatile uint8_t _rxHeaderFlags;

/// TO header to send in all messages
uint8_t _txHeaderTo;

/// FROM header to send in all messages
uint8_t _txHeaderFrom;

/// ID header to send in all messages
uint8_t _txHeaderId;

/// FLAGS header to send in all messages
uint8_t _txHeaderFlags;

/// The value of the last received RSSI value, in some transport specific units
volatile int16_t _lastRssi;

/// Count of the number of bad messages (eg bad checksum etc) received
volatile uint16_t _rxBad;

/// Count of the number of successfully transmitted messaged
volatile uint16_t _rxGood;

/// Count of the number of bad messages (correct checksum etc) received
volatile uint16_t _txGood;

/// Channel activity detected
volatile bool _cad;

/// Channel activity timeout in ms
unsigned int _cad_timeout;

bool genericDriverInit( );

#endif /* GENERICDRIVER_H_ */
