/*
 * board-config.h
 *
 *  Created on: 03 Mar 2020
 *      Author: nicholas
 */

#ifndef BOARD_CONFIG_H_
#define BOARD_CONFIG_H_


#define RADIO_NSS                                   P5_2

#define RADIO_DIO_0                                 P2_4
#define RADIO_DIO_1                                 P2_6
#define RADIO_DIO_2                                 P2_7
#define RADIO_DIO_3                                 P3_5
#define RADIO_DIO_4                                 P2_3
#define RADIO_DIO_5                                 NC

#define RADIO_RESET                                 P3_7

#define RADIO_MOSI                                  P1_6
#define RADIO_MISO                                  P1_7
#define RADIO_SCLK                                  P1_5

#define LED_RGB_RED                                 P2_0
#define LED_RGB_GREEN                               P2_1
#define LED_RGB_BLUE                                P2_2
#define LED_USER_RED								P1_0

#define BOARD_TCXO_WAKEUP_TIME                      1


#endif /* BOARD_CONFIG_H_ */
