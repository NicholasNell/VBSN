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

 * Author: Nicholas
 *******************************************************************************/
/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/drivers/GPIO.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>

/* Standard Includes */
#include "main.h"
#include <stdio.h>
#include "board.h"
#include "my_MAC.h"
#include <string.h>
#include "sx1276Regs-Fsk.h"
#include "sx1276Regs-LoRa.h"
#include "my_spi.h"
#include "bme280.h"
#include "MAX44009.h"

// Uncomment for debug outputs
//#define DEBUG

uint8_t data[] = { 'H', 'E', 'L', 'L', 'O' };

void printRegisters(void);
float lux;
void RadioInit() {
	// Radio initialization
	RadioEvents.TxDone = OnTxDone;
	RadioEvents.RxDone = OnRxDone;
	RadioEvents.TxTimeout = OnTxTimeout;
	RadioEvents.RxTimeout = OnRxTimeout;
	RadioEvents.RxError = OnRxError;
	Radio.Init(&RadioEvents);

	// detect radio hardware
	while (Radio.Read(REG_VERSION) == 0x00) {
		puts("Radio could not be detected!\n\r");
		Delayms(1000);
	}
//	printf("RadioRegVersion: 0x%2X\n", Radio.Read(REG_VERSION));

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
	// set radio frequency channel
	Radio.SetChannel( RF_FREQUENCY);
	Radio.Sleep();
}
bool myFlag = false;
int main(void) {
	/* Stop Watchdog  */
	MAP_WDT_A_holdTimer();

	bool isRoot = false;
	if (isRoot)
		printf("ROOT!\n");

	BoardInitMcu();

	RadioInit();

	MacInit();

	bme280UserInit(&bme280Dev, &bme280Data);
	bme280GetData(&bme280Dev, &bme280Data);
	initMAX();
	lux = getLight();
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
	MAP_Interrupt_enableInterrupt(INT_PORT1);
	while (1) {
	}
}

void PORT1_IRQHandler(void) {
	uint32_t status;

	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, status);

	/* Toggling the output on the LED */

	if (status & GPIO_PIN1) {
//		MACreadySend(data, 5);

		bme280GetData(&bme280Dev, &bme280Data);
		lux = getLight();

	}
}

void PORT2_IRQHandler(void) {
	uint32_t status;
	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P2);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P2, status);

#ifdef DEBUG
	puts("LoRa Interrupt Triggered");
#endif
	if (status & GPIO_PIN4) {
		SX1276OnDio0Irq();
	} else if (status & GPIO_PIN6) {
		SX1276OnDio1Irq();
	} else if (status & GPIO_PIN3) {
	} else if (status & GPIO_PIN7) {
		SX1276OnDio2Irq();
	}
}

void OnTxDone(void) {
	SX1276clearIRQFlags();
	Radio.Sleep();
	RadioState = TXDONE;
#ifdef DEBUG
	puts("TxDone");
#endif
}

void OnRxDone(uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr) {
	SX1276clearIRQFlags();
//	Radio.Sleep();
	BufferSize = size;
	memcpy(RXBuffer, payload, BufferSize);
	RssiValue = rssi;
	SnrValue = snr;
	RadioState = RXDONE;
#ifdef DEBUG
	puts("RxDone");
	GpioFlashLED(&Led_rgb_green, 10);
#endif
}

void OnTxTimeout(void) {
	Radio.Sleep();
	RadioState = TXTIMEOUT;
#ifdef DEBUG
	puts("TxTimeout");
#endif
}

void OnRxTimeout(void) {
	SX1276clearIRQFlags();
	Radio.Sleep();
	RadioState = RXTIMEOUT;
#ifdef DEBUG

	puts("RxTimeout");
	GpioFlashLED(&Led_rgb_red, 10);
#endif
}

void OnRxError(void) {
	Radio.Sleep();
#ifdef DEBUG
	puts("RxError");
#endif
}

void printRegisters(void) {

	uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
			0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x014, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
			0x23, 0x24, 0x25, 0x26, 0x27, 0x4b };

	uint8_t i;
	for (i = 0; i < sizeof(registers); i++) {
		printf("0x%2X", registers[i]);
		printf(": ");
		printf("0x%2X\n", spiRead_RFM(registers[i]));
	}
}
