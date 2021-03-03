/** @file EC5.h
 *
 * @brief Contains the ADC code for the Decagon EC-5 VMC sensor
 *
 *  Created on: 04 Feb 2021
 *      Author: njnel
 */

#ifndef EC5_H_
#define EC5_H_

#define EC5_POWER_PORT GPIO_PORT_P2
#define EC5_POWER_PIN GPIO_PIN2

void init_adc( void );
float get_vwc( );
float get_latest_value( void );

#endif /* EC5_H_ */

