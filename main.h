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

#include <bme280_defs.h>
#include <stdbool.h>

/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

#define RF_FREQUENCY 868500000  // Hz
#define TX_OUTPUT_POWER 15   // dBm
#define TX_TIMEOUT_VALUE 3000 	// ms
#define RX_TIMEOUT_VALUE 5000 //ms
#define LORA_BANDWIDTH 8      //  LoRa: [	0: 7.8 kHz,  1: 10.4 kHz,  2: 15.6 kHz,
//	3: 20.8 kHz, 4: 31.25 kHz, 5: 41.7 kHz,
//	6: 62.5 kHz, 7: 125 kHz,   8: 250 kHz,
// 	9: 500 kHz]
#define LORA_SPREADING_FACTOR 8 // [SF7..SF12]
#define LORA_CODINGRATE 4       // [1: 4/5, \
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
#define LORA_RSSI_THRESHOLD -50

#define DEBUG 0

bool get_is_root();

//!
//! \brief collects all sensor data
void helper_collect_sensor_data();

uint8_t* get_rx_buffer();

uint8_t get_lora_rx_buffer_size();

int8_t get_rssi_value();
int8_t get_snr_value();

bool get_gps_wake_flag();
void set_gps_wake_flag();
void reset_gps_wake_flag();

bool get_gsm_upload_done_flag();
void set_gsm_upload_done_flag();
void reset_gsm_upload_done_flag();

#endif // __MAIN_H__
