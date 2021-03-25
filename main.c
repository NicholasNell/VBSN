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

#include <bme280.h>
#include <bme280_defs.h>
#include <board.h>
#include <main.h>
#include <my_flash.h>
#include <my_gpio.h>
#include <my_GSM.h>
#include <my_MAC.h>
#include <my_rtc.h>
#include <my_RFM9x.h>
#include <my_scheduler.h>
#include <my_spi.h>
#include <my_systick.h>
#include <my_timer.h>
#include <my_UART.h>
#include <myNet.h>
#include <MAX44009.h>
#include <radio.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <EC5.h>
#include <helper.h>
#include <my_gps.h>
#include <sx1276Regs-Fsk.h>
#include <ti/devices/msp432p4xx/driverlib/gpio.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/rom_map.h>

// Uncomment for debug out sendUARTpc
//#define DEBUG

//uint8_t TXBuffer[255] = { 0 };
uint8_t RXBuffer[255] = { 0 };
volatile uint8_t loraRxBufferSize = LORA_MAX_PAYLOAD_LEN;

uint16_t loraTxtimes[255];

volatile int8_t RssiValue = 0;
volatile int8_t SnrValue = 0;

// Sensor Data Variables
struct bme280_data bme280Data;
struct bme280_dev bme280Dev;

// Sensor Availability:
bool bme280Working = false;
bool lightSensorWorking = false;
bool gpsWorking = false;

bool hasGSM = false;

bool MACFlag = true;

// Radio Module status flag:
bool rfm95Working = false;

// is this a root node??
bool isRoot = false;

extern bool schedFlag;

bool gpsWakeFlag = false;

/*!
 * Radio events function pointer
 */
static RadioEvents_t RadioEvents;

// MSP432 Developments board LED's
extern Gpio_t Led_rgb_red;	//RED
extern Gpio_t Led_rgb_green;	//GREEN
//extern Gpio_t Led_rgb_blue;		//BLUE
extern Gpio_t Led_user_red;

// MAC layer state
extern LoRaRadioState_t RadioState;

extern RTC_C_Calendar currentTime;

void print_lora_registers( void );

/*!
 * \brief Function to be executed on Radio Tx Done event
 */
void on_tx_done( void );

/*!
 * \brief Function to be executed on Radio Rx Done event
 */
void on_rx_done( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr );

/*!
 * \brief Function executed on Radio Tx Timeout event
 */
void on_tx_timeout( void );

/*!
 * \brief Function executed on Radio Rx Timeout event
 */
void on_rx_timeout( void );

/*!
 * \brief Function executed on Radio Rx Error event
 */
void on_rx_error( void );

void radio_init( ) {
	// Radio initialization
	RadioEvents.TxDone = on_tx_done;
	RadioEvents.RxDone = on_rx_done;
	RadioEvents.TxTimeout = on_tx_timeout;
	RadioEvents.RxTimeout = on_rx_timeout;
	RadioEvents.RxError = on_rx_error;

	// detect radio hardware
	if (Radio.Read(REG_VERSION) == 0x00) {
		send_uart_pc("Radio could not be detected!\n\r");
		rfm95Working = false;
		board_reset_mcu();// if no radio is detected reset the MCU and try again; no radio, no work
		return;
	}
	else {
		rfm95Working = true;
	}

	Radio.Init(&RadioEvents);

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

	int i;
	for (i = 0; i < 255; ++i) {
		loraTxtimes[i] = SX1276GetTimeOnAir(MODEM_LORA, LORA_BANDWIDTH,
		LORA_SPREADING_FACTOR, LORA_CODINGRATE, LORA_PREAMBLE_LENGTH,
		LORA_FIX_LENGTH_PAYLOAD_ON, i + 1, LORA_CRC_ON) + 10;
	}
}

extern volatile MACappState_t MACState;
extern bool uploadOwnData;
int main( void ) {
	/* Stop Watchdog  */
	MAP_WDT_A_holdTimer();
	SysCtl_setWDTTimeoutResetType(SYSCTL_SOFT_RESET);
	WDT_A_initWatchdogTimer(WDT_A_CLOCKSOURCE_SMCLK,
	WDT_A_CLOCKITERATIONS_128M);	// aprox 16 seconds

	MAP_WDT_A_startTimer();

	isRoot = false;	// Assume not a root node at first

	// Initialise all ports and communication protocols
	board_init_mcu();
	rtc_init();

	// Button 1 interrupt
	MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
	MAP_Interrupt_enableInterrupt(INT_PORT1);

	srand(SX1276Random());	// Seeding Random Number generator

	// Initialise UART to PC
	uart_init_pc();

	//Initialise UART to GPS;
	uart_init_gps();

//	 Initialise the RFM95 Radio Module
	radio_init();

	hasGSM = init_gsm();	//Check if has GSm module

	if (hasGSM) {
		isRoot = true;
		gpio_toggle(&Led_rgb_green);
	}
	else {
		isRoot = false;
	}

	if (bme280_user_init(&bme280Dev, &bme280Data) >= 0) {
		bme280_get_data(&bme280Dev, &bme280Data);
		bme280Working = true;
	}
	else {
		bme280Working = false;
	}

	lightSensorWorking = init_max();
	if (lightSensorWorking) {
		get_light();
	}

//	// Have to awit for GPS to get a lock before operation can continue
//	run_systick_function_ms(1000);
//	while (!gpsWorking) {
//		if (systimer_ready_check()) {
//			gpio_flash_lED(&Led_user_red, 50);
//			run_systick_function_ms(1000);
//		}
//	}
//
//	gps_set_low_power();

//	gpio_write(&Led_user_red, 0);
//	char b[255];
//	LocationData gpsData = get_gps_data();
//	int len = sprintf(
//			b,
//			"gpsWorking. Lon: %f\tLat: %f\n",
//			gpsData.lon,
//			gpsData.lat);
//	SX1276Send((uint8_t*) b, len);
//	flash_fill_struct_for_write();

//	 Initialise the Network and  MAC protocol
	net_init();
	mac_init();

	init_adc();

	// Volumetric Water Content Sensor
	get_vwc();

	init_scheduler();

	MAP_WDT_A_holdTimer();
	SysCtl_setWDTTimeoutResetType(SYSCTL_SOFT_RESET);
	WDT_A_initWatchdogTimer(WDT_A_CLOCKSOURCE_BCLK,
	WDT_A_CLOCKITERATIONS_512K);	// aprox 16 seconds

	MAP_WDT_A_startTimer();

	while (1) {

		if (schedFlag) {
			schedFlag = false;
			scheduler();
			MAP_WDT_A_clearTimer();
		}

		if (uploadOwnData) {
			gsm_upload_my_data();
			uploadOwnData = false;
		}

		if (gpsWakeFlag) {
			send_uart_gps("H\r\n");
			gpsWakeFlag = false;
		}
//		PCM_gotoLPM3InterruptSafe();
	}
}

extern bool schedChange;

void PORT1_IRQHandler( void ) {
	uint32_t status;

	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, status);

	if (status & GPIO_PIN1) {

	}
}

extern bool setRTCFlag;
void PORT3_IRQHandler( void ) {
	uint32_t status;
	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P3);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P3, status);
	if (status & GPS_PPS_PIN) {

		if (!gpsWorking) {
			send_uart_gps(PMTK_SET_NMEA_OUTPUT_RMCONLY);
		}

		if (setRTCFlag) {
			MAP_RTC_C_holdClock();
			rtc_init();
			send_uart_gps(PMTK_STANDBY);
			setRTCFlag = false;
		}
//		schedFlag = true;
//		incrementSlotCount();
//
//		if (getSlotCount() == MAX_SLOT_COUNT + 1) {
//			setSlotCount(0);
//		}
//
//		RtcInit(currentTime);
	}
}

void PORT2_IRQHandler( void ) {
	uint32_t status;
	status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P2);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P2, status);

#ifdef DEBUG
	send_uart_pc("LoRa Interrupt Triggered: ");
#endif
	if (status & GPIO_PIN4) {
		SX1276OnDio0Irq();
	}
	else if (status & GPIO_PIN6) {
		SX1276OnDio1Irq();
	}
	else if (status & GPIO_PIN7) {
		SX1276OnDio2Irq();
	}
}

void on_tx_done( void ) {
	SX1276clearIRQFlags();
	RadioState = TXDONE;
#ifdef DEBUG
	send_uart_pc("TxDone\n");
#endif
}

void on_rx_done( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr ) {
	SX1276clearIRQFlags();
	loraRxBufferSize = size;
	memcpy(RXBuffer, payload, loraRxBufferSize);
	RssiValue = rssi;
	SnrValue = snr;
	RadioState = RXDONE;
#ifdef DEBUG
	send_uart_pc("RxDone\n");
	GpioFlashLED(&Led_rgb_green, 10);
#endif
}

void on_tx_timeout( void ) {
	Radio.Sleep();
	RadioState = TXTIMEOUT;
#ifdef DEBUG
	send_uart_pc("TxTimeout\n");
#endif
}

void on_rx_timeout( void ) {
	SX1276clearIRQFlags();
	Radio.Sleep();
	RadioState = RXTIMEOUT;
#ifdef DEBUG

	send_uart_pc("RxTimeout\n");
	GpioFlashLED(&Led_rgb_red, 10);
#endif
}

void on_rx_error( void ) {
	SX1276clearIRQFlags();
	Radio.Sleep();
	RadioState = RXERROR;
#ifdef DEBUG
	send_uart_pc("RxError\n");
#endif
}

void print_lora_registers( void ) {

	uint8_t registers[] = { 0x01, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
							0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x014,
							0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c,
							0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24,
							0x25, 0x26, 0x27, 0x4b };

	uint8_t i;
	for (i = 0; i < sizeof(registers); i++) {

		send_uart_integer(registers[i]);
		send_uart_pc(": ");
		send_uart_integer(spi_read_rfm(registers[i]));
		send_uart_pc("\n");
	}
}
