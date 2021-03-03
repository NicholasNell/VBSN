/*!
 * \file      board.h
 *
 * \brief     Target board general functions implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
//#include "utilities.h"
/*!
 * Possible power sources
 */
enum BoardPowerSources {
	USB_POWER = 0, BATTERY_POWER,
};

/*
 * Clock Configuration:
 */
#define REFO_FREQ CS_REFO_128KHZ
#define LFXT_FREQ 32000
#define HFXT_FREQ 48000000

//ACLK = 128kHz
#define ACLK_SOURCE CS_REFOCLK_SELECT
#define ACLK_DIV CS_CLOCK_DIVIDER_1

//MCLK = 1.5MHz
#define MCLK_SOURCE CS_DCOCLK_SELECT
#define MCLK_DIV CS_CLOCK_DIVIDER_1

//HSMCLK = 24MHz
#define HSMCLK_SOURCE CS_MODOSC_SELECT
#define HSMCLK_DIV CS_CLOCK_DIVIDER_1

//SMCL = 24Mhz/16 = 1.5MHz
#define SMCLK_SOURCE CS_MODOSC_SELECT
#define SMCLK_DIV CS_CLOCK_DIVIDER_16

/*!
 * \brief Initializes the mcu.
 */
void board_init_mcu( void );

/*!
 * \brief Resets the mcu.
 */
void board_reset_mcu( void );

/*!
 * \brief Initializes the boards peripherals.
 */
void board_init_periph( void );

/*!
 * \brief De-initializes the target board peripherals to decrease power
 *        consumption.
 */
void board_deinit_mcu( void );

/*!
 * \brief Measure the Battery voltage
 *
 * \retval value  battery voltage in volts
 */
uint32_t board_get_battery_voltage( void );

/*!
 * \brief configures the system clock
 */
void system_clock_config( void );

#ifdef __cplusplus
}
#endif

#endif // __BOARD_H__
