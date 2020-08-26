/*
 / _____)             _              | |
( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
(______/|_____)_|_|_| \__)_____)\____)_| |_|
    ( C )2014 Semtech

Description: Contains the callbacks for the IRQs and any application related details

License: Revised BSD License, see LICENSE.TXT file include in the project

Maintainer: Miguel Luis and Gregory Cristian
*/
#ifndef __MAIN_H__
#define __MAIN_H__

#include <stdint.h>
#include <stdbool.h>
#include "my_RFM9x.h"
#include "my_gpio.h"



/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

#define RF_FREQUENCY 868500000  // Hz
#define TX_OUTPUT_POWER 0	    // dBm
#define TX_TIMEOUT_VALUE 3000 	// ms
#define RX_TIMEOUT_VALUE 5000 //ms
#define LORA_BANDWIDTH 9      //  LoRa: [	0: 7.8 kHz,  1: 10.4 kHz,  2: 15.6 kHz,
//	3: 20.8 kHz, 4: 31.25 kHz, 5: 41.7 kHz,
//	6: 62.5 kHz, 7: 125 kHz,   8: 250 kHz,
// 	9: 500 kHz]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5, \
                                //  2: 4/6, \
                                //  3: 4/7, \
                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 1023   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define LORA_FREQ_DEV 0
#define LORA_CRC_ON true
#define LORA_PAYLOAD_LEN 255
#define LORA_RX_CONTINUOUS true
#define LORA_FREQ_HOP_ENABLED false
#define LORA_FREQ_HOP_PERIOD false
#define LORA_bandwidthAfc 0
#define LORA_MAX_PAYLOAD_LEN 255
#define LORA_PRIVATE_SYNCWORD 0x55
#define LORA_IS_PUBLIC_NET false

uint8_t TXBuffer[255] = { 0 };
uint8_t RXBuffer[255] = { 0 };
volatile uint8_t BufferSize = LORA_MAX_PAYLOAD_LEN;

bool DIO0Flag = false;
bool DIO1Flag = false;
bool DIO2Flag = false;
bool DIO3Flag = false;
bool DIO4Flag = false;
bool sendFlag = false;

uint32_t value;

volatile int8_t RssiValue = 0;
volatile int8_t SnrValue = 0;

extern Gpio_t Led_rgb_red;	//RED
extern Gpio_t Led_rgb_green;	//GREEN
extern Gpio_t Led_rgb_blue;		//BLUE
extern Gpio_t Led_user_red;

extern bool TxFlag;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void OnTxDone( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void OnTxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void OnRxTimeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void OnRxError( void );

#endif // __MAIN_H__
