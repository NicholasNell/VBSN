/*
 * board.c
 *
 *  Created on: 04 Mar 2020
 *      Author: nicholas
 */

#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "utilities.h"
#include "my_gpio.h"
#include "my_spi.h"
#include "my_timer.h"
#include "board-config.h"
#include "sx1276-board.h"
#include "board.h"
#include "my_rtc.h"
#include "my_i2c.h"

/*!
 * LED GPIO pins objects
 */
Gpio_t Led_rgb_red;
Gpio_t Led_rgb_green;
Gpio_t Led_rgb_blue;
Gpio_t Led_user_red;

/*!
 * Initializes the unused GPIO to a know status
 */
static void BoardUnusedIoInit(void);

/*!
 * Timer used at first boot to calibrate the SystemWakeupTime
 */
//static TimerEvent_t CalibrateSystemWakeupTimeTimer;
/*!
 * Flag to indicate if the MCU is Initialized
 */
//static bool McuInitialized = false;
/*!
 * Flag used to indicate if board is powered from the USB
 */
static bool UsbIsConnected = false;

/*!
 * Flag to indicate if the SystemWakeupTime is Calibrated
 */
static volatile bool SystemWakeupTimeCalibrated = false;

void BoardCriticalSectionBegin(uint32_t *mask) {
	*mask = __get_PRIMASK();
	__disable_irq();
}

void BoardCriticalSectionEnd(uint32_t *mask) {
	__set_PRIMASK(*mask);
}

void BoardInitPeriph(void) {

}

void BoardInitMcu(void) {
	BoardUnusedIoInit();

	SystemClockConfig();

	DelayTimerInit();
	RtcInit();

	i2cInit();

	// LEDs
	GpioInit(&Led_rgb_red,
	LED_RGB_RED, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	GpioInit(&Led_rgb_green,
	LED_RGB_GREEN, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	GpioInit(&Led_rgb_blue,
	LED_RGB_BLUE, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	GpioInit(&Led_user_red,
	LED_USER_RED, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);

	UsbIsConnected = true;

	if (GetBoardPowerSource() == BATTERY_POWER) {
		// Disables OFF mode - Enables lowest power mode (STOP)
//            PCM_setPowerState(PCM_LPM3);
	}

	SX1276IoInit();
	SX1276IoIrqInit();

	GpioWrite(&Led_rgb_red, 0);
	GpioWrite(&Led_rgb_green, 0);
	GpioWrite(&Led_rgb_blue, 0);
	GpioWrite(&Led_user_red, 0);

	/*	if (McuInitialized == false) {
	 McuInitialized = true;
	 }*/
}

void BoardResetMcu(void) {

	CRITICAL_SECTION_BEGIN();
	//Restart system
	NVIC_SystemReset();
	CRITICAL_SECTION_END();
}

void BoardDeInitMcu(void) {
	spi_close();
	SX1276IoDeInit();
}

uint16_t BoardBatteryMeasureVolage(void) {
	return 0;
}

uint32_t BoardGetBatteryVoltage(void) {
	return 0;
}

uint8_t BoardGetBatteryLevel(void) {
	return 0;
}

static void BoardUnusedIoInit(void) {
	/* Terminating all remaining pins to minimize power consumption. This is
	 done by register accesses for simplicity and to minimize branching API
	 calls */

	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, PIN_ALL16);
//	MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P5, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P6, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P7, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P8, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P9, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P10, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PA, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PB, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PC, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PD, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PE, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_PJ, PIN_ALL16);

	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P1, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P2, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P3, PIN_ALL16);
//	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P5, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P6, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P7, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P8, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P9, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P10, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PA, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PB, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PC, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PD, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PE, PIN_ALL16);
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_PJ, PIN_ALL16);
}

uint8_t GetBoardPowerSource(void) {
	if (UsbIsConnected == false) {
		return BATTERY_POWER;
	} else {
		return USB_POWER;
	}
}

void SystemClockConfig(void) {
	/*
	 * What clock goes where??:
	 * ACLK:	use with TIMER_A and SPI
	 * SMCLK: 	use with SPI and TIMER_A
	 */
	MAP_CS_setReferenceOscillatorFrequency(REFO_FREQ);
	MAP_CS_setExternalClockSourceFrequency(LFXT_FREQ, HFXT_FREQ); //LFXT_FREQ
//	MAP_CS_startLFXT(CS_LFXT_DRIVE0);
	MAP_FPU_enableModule(); // makes sure the FPU is enabled before DCO frequency tuning
	MAP_CS_setDCOFrequency(1500000);

	MAP_CS_initClockSignal(CS_ACLK, ACLK_SOURCE, ACLK_DIV); //128kHz
	MAP_CS_initClockSignal(CS_MCLK, MCLK_SOURCE, MCLK_DIV); // 1.5MHz
	MAP_CS_initClockSignal(CS_HSMCLK, HSMCLK_SOURCE, HSMCLK_DIV); // 24MHz
	MAP_CS_initClockSignal(CS_SMCLK, SMCLK_SOURCE, SMCLK_DIV);
	MAP_CS_initClockSignal(CS_BCLK, CS_LFXTCLK_SELECT, CS_CLOCK_DIVIDER_1);

}
