/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
/******************************************************************************
 * MSP432 Empty Project
 *
 * Description: An empty project that uses DriverLib
 *
 *                MSP432P401
 *             ------------------
 *         /|\|                  |
 *          | |                  |
 *          --|RST               |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 *            |                  |
 * Author:
 *******************************************************************************/
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/drivers/GPIO.h>

/* Standard Includes */
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include "my_ALOHA.h"
#include "my_timer.h"
#include "my_spi.h"
#include "my_RFM9x.h"
#include "my_gpio.h"
#include "board-config.h"
#include "board.h"
#include "sx1276-board.h"
#include "timer.h"
#include <stdio.h>


/*!
 * Constant values need to compute the RSSI value
 */
#define RSSI_OFFSET_LF                              -164
#define RSSI_OFFSET_HF                              -157

#define RF_FREQUENCY 868500000  // Hz
#define TX_OUTPUT_POWER 14	    // dBm
#define TX_TIMEOUT_VALUE 10000 	// ms
#define RX_TIMEOUT_VALUE 10000	// ms
#define LORA_BANDWIDTH 7        //  LoRa: [	0: 7.8 kHz,  1: 10.4 kHz,  2: 15.6 kHz,
//	3: 20.8 kHz, 4: 31.25 kHz, 5: 41.7 kHz,
//	6: 62.5 kHz, 7: 125 kHz,   8: 250 kHz,
// 	9: 500 kHz]
#define LORA_SPREADING_FACTOR 7 // [SF7..SF12]
#define LORA_CODINGRATE 1       // [1: 4/5, \
                                //  2: 4/6, \
                                //  3: 4/7, \
                                //  4: 4/8]
#define LORA_PREAMBLE_LENGTH 8  // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT 255   // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON false
#define LORA_IQ_INVERSION_ON false
#define LORA_FREQ_DEV 0
#define LORA_CRC_ON true
#define LORA_PAYLOAD_LEN 4
#define LORA_RX_CONTINUOUS false
#define LORA_FREQ_HOP_ENABLED false
#define LORA_FREQ_HOP_PERIOD false
#define LORA_bandwidthAfc 0
#define LORA_MAX_PAYLOAD_LEN 255
#define LORA_PRIVATE_SYNCWORD 0x55
#define LORA_IS_PUBLIC_NET false

uint8_t buffer[] = { 'H', 'E', 'L', 'L', 'O' };
bool DIO0Flag = false;
bool DIO1Flag = false;
bool DIO2Flag = false;
bool DIO3Flag = false;
bool DIO4Flag = false;

typedef enum {
	LOWPOWER, RX, RX_TIMEOUT, RX_ERROR, TX, TX_TIMEOUT, SLEEP
} States_t;


#define BUFFER_SIZE 64 // Define the payload size here

States_t State = LOWPOWER;

uint16_t BufferSize = BUFFER_SIZE;
uint8_t Buffer[BUFFER_SIZE] = { 0 };
/*		{ 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A',
 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A', 'A' };*/
int8_t RssiValue = 0;
int8_t SnrValue = 0;

uint8_t txMsg[] = { 'P', 'I', 'N', 'G' };

extern Gpio_t Led1;	//RED
extern Gpio_t Led2;	//GREEN
extern Gpio_t Led3;

extern bool TxFlag;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

void RadioInit( ) {
	// Radio initialization
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;
	Radio.Init(&RadioEvents);

	// detect radio hardware
	while (Radio.Read( REG_VERSION) == 0x00) {
//		printf("Radio could not be detected!\n\r");
		Delayms(1000);
	}
//	printf("RadioRegVersion: 0x%X\r\n", Radio.Read( REG_VERSION));

// set radio frequency channel
	Radio.SetChannel( RF_FREQUENCY);

	Radio.SetTxConfig(MODEM_LORA,
	TX_OUTPUT_POWER,
	LORA_FREQ_DEV,
	LORA_BANDWIDTH,
	LORA_SPREADING_FACTOR,
	LORA_CODINGRATE,
	LORA_PREAMBLE_LENGTH,
	LORA_FIX_LENGTH_PAYLOAD_ON,
	LORA_CRC_ON, LORA_FREQ_HOP_ENABLED, LORA_FREQ_HOP_PERIOD,
	LORA_IQ_INVERSION_ON, TX_TIMEOUT_VALUE);

	Radio.SetRxConfig(MODEM_LORA,
	LORA_BANDWIDTH,
	LORA_SPREADING_FACTOR,
	LORA_CODINGRATE, LORA_bandwidthAfc,
	LORA_PREAMBLE_LENGTH, LORA_SYMBOL_TIMEOUT,
	LORA_FIX_LENGTH_PAYLOAD_ON, LORA_PAYLOAD_LEN,
	LORA_CRC_ON, LORA_FREQ_HOP_ENABLED, LORA_FREQ_HOP_PERIOD,
	LORA_IQ_INVERSION_ON, LORA_RX_CONTINUOUS);

	Radio.SetMaxPayloadLength(MODEM_LORA, LORA_MAX_PAYLOAD_LEN);
	Radio.SetPublicNetwork(LORA_IS_PUBLIC_NET, LORA_PRIVATE_SYNCWORD);
	Radio.Sleep();

	State = SLEEP;
}

int main( void ) {
	/* Stop Watchdog  */
	MAP_WDT_A_holdTimer();

	BoardInitMcu();
	RadioInit();

	startTimerAcounter();
//	Radio.Rx(0);
//	State = RX;
//	GpioToggle(&Led2);

	while (1) {
		if (DIO0Flag) {
			DIO0Flag = false;
			SX1276OnDio0Irq();
		}
		else if (DIO1Flag) {
			DIO4Flag = false;
			SX1276OnDio1Irq();
		}
		else if (DIO2Flag) {
			DIO2Flag = false;
			SX1276OnDio2Irq();
		}
		else if (DIO4Flag) {
			DIO4Flag = false;
			SX1276OnDio4Irq();
		}

//		if (State == SLEEP) {
//			Radio.Rx(0);
//			State = RX;
//		}

		if (getTimerAcounterValue() >= 10 * 1000 * 1000) {
			resetTimerAcounterValue();
			GpioFlashLED(&Led1, 100);
			Radio.Send(txMsg, LORA_PAYLOAD_LEN);
			State = TX;
			Delayms(1000);
			Radio.Sleep();
			State = SLEEP;
		}

	}
}

void PORT2_IRQHandler( void ) {
	uint32_t status;

//	uint8_t i = Radio.Read( REG_LR_IRQFLAGS);
	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P2);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P2, status);

	/* Toggling the output on the LED */
	if (status & GPIO_PIN4) {
		DIO0Flag = true;
	}
	else if (status & GPIO_PIN6) {
		DIO1Flag = true;
	}
	else if (status & GPIO_PIN3) {
		DIO4Flag = true;
	}
	else if (status & GPIO_PIN7) {
		DIO2Flag = true;
	}
}

void OnTxDone( void ) {
	Radio.Sleep();
//	GpioFlashLED(&Led2, 100);
	State = SLEEP;
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ) {
	Radio.Sleep();
	GpioFlashLED(&Led3, 100);
	BufferSize = size;
	memcpy(Buffer, payload, BufferSize);
	RssiValue = rssi;
	SnrValue = snr;
	State = SLEEP;
}

void OnTxTimeout( void ) {
	Radio.Sleep();
	State = TX_TIMEOUT;
}

void OnRxTimeout( void ) {
	GpioFlashLED(&Led3, 10);
	Radio.Sleep();
	State = RX_TIMEOUT;

}

void OnRxError( void ) {
	Radio.Sleep();
	State = RX_ERROR;
}

