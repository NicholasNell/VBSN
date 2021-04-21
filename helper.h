/** @file helper.h
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#ifndef HELPER_H_
#define HELPER_H_
#include <stdint.h>

//!
//! \brief collects all sensor data
void helper_collect_sensor_data( );

//! @param hex number to be converted
//! @return the converted decimal value
//! @brief eg. 0x45 -> 45
int convert_hex_to_dec_by_byte( uint_fast8_t hex );

#endif /* HELPER_H_ */
