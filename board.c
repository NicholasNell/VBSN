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


/*!
 * LED GPIO pins objects
 */
Gpio_t Led1;
Gpio_t Led2;
Gpio_t Led3;

/*!
 * Initializes the unused GPIO to a know status
 */
static void BoardUnusedIoInit( void );

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

void BoardCriticalSectionBegin( uint32_t *mask ) {
	*mask = __get_PRIMASK();
	__disable_irq();
}

void BoardCriticalSectionEnd( uint32_t *mask ) {
	__set_PRIMASK(*mask);
}

void BoardInitPeriph( void ) {

}

void BoardInitMcu( void ) {
	BoardUnusedIoInit();

//	TimerAInteruptInit();
	TimerATimerInit();
	startTiming();
	RtcInit();

	// LEDs
	GpioInit(&Led1, LED_1, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	GpioInit(&Led2, LED_2, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);
	GpioInit(&Led3, LED_3, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0);

//        TODO: Implement Clock Config
//        SystemClockConfig( );

	UsbIsConnected = true;

//        RtcInit( );

//        TODO: Fix Board power source (Will need external IO pin to detect bat voltage)
	if (GetBoardPowerSource() == BATTERY_POWER) {
		// Disables OFF mode - Enables lowest power mode (STOP)
//            PCM_setPowerState(PCM_LPM3);
	}

	SX1276IoInit();
	SX1276IoIrqInit();

	GpioWrite(&Led1, 0);
	GpioWrite(&Led2, 0);
	GpioWrite(&Led3, 0);

	/*	if (McuInitialized == false) {
	 McuInitialized = true;
	 }*/
}

void BoardResetMcu( void ) {

	CRITICAL_SECTION_BEGIN( )
	;
	//Restart system
	NVIC_SystemReset();
	CRITICAL_SECTION_END( );
}

void BoardDeInitMcu( void ) {
	spi_close();
	SX1276IoDeInit();
}

uint16_t BoardBatteryMeasureVolage( void ) {
	return 0;
}

uint32_t BoardGetBatteryVoltage( void ) {
	return 0;
}

uint8_t BoardGetBatteryLevel( void ) {
	return 0;
}

static void BoardUnusedIoInit( void ) {
	/* Terminating all remaining pins to minimize power consumption. This is
	 done by register accesses for simplicity and to minimize branching API
	 calls */

	MAP_GPIO_setAsOutputPin(GPIO_PORT_P1, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P3, PIN_ALL16);
	MAP_GPIO_setAsOutputPin(GPIO_PORT_P4, PIN_ALL16);
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
	MAP_GPIO_setOutputLowOnPin(GPIO_PORT_P4, PIN_ALL16);
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

uint8_t GetBoardPowerSource( void ) {
	if (UsbIsConnected == false) {
		return BATTERY_POWER;
	}
	else {
		return USB_POWER;
	}
}
