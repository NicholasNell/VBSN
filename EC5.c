/** @file EC5.c
 *
 * @brief
 *
 *  Created on: 04 Feb 2021
 *      Author: njnel
 */

#include <stdint.h>
#include <string.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include "EC5.h"
#include <string.h>
#include <my_gpio.h>

// defines
#define ADC_SAMPLE_COUNT 10

/* Statics */
static volatile uint16_t curADCResult;
static volatile float normalizedADCRes;

volatile bool convertingFlag = false;

volatile int currentSample = ADC_SAMPLE_COUNT;
uint16_t valueArray[ADC_SAMPLE_COUNT + 1];

static float vwc;

void init_adc( void ) {

	// init power pin
	GPIO_setAsOutputPin(EC5_POWER_PORT, EC5_POWER_PIN);
	GPIO_setOutputLowOnPin(EC5_POWER_PORT, EC5_POWER_PIN);

	// enable floating point operation
	MAP_FPU_enableModule();
	MAP_FPU_enableLazyStacking();

	// enable the ADC in single sample mode
	MAP_ADC14_enableModule();
	ADC14_initModule(
	ADC_CLOCKSOURCE_MCLK,
	ADC_PREDIVIDER_1,
	ADC_DIVIDER_1, 0);

	// COnfigure P5.4 as ADC input pin (A1)
	MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P5, GPIO_PIN4,
	GPIO_TERTIARY_MODULE_FUNCTION);

	/* Configuring ADC Memory */
	MAP_ADC14_configureSingleSampleMode(ADC_MEM1, true);
	MAP_ADC14_configureConversionMemory(ADC_MEM1, ADC_VREFPOS_AVCC_VREFNEG_VSS,
	ADC_INPUT_A1, false);

	/* Configuring Sample Timer */
	MAP_ADC14_enableSampleTimer(ADC_MANUAL_ITERATION);

	/* Enabling interrupts */
	MAP_Interrupt_enableInterrupt(INT_ADC14);
	MAP_Interrupt_enableMaster();
}

float get_vwc( ) {

	GPIO_setOutputHighOnPin(EC5_POWER_PORT, EC5_POWER_PIN);

	/* Enabling/Toggling Conversion */
	MAP_ADC14_enableConversion();
	MAP_ADC14_enableInterrupt(ADC_INT1);

	memset(valueArray, 0, ADC_SAMPLE_COUNT + 1);

	while (currentSample >= 0) {

		MAP_ADC14_toggleConversionTrigger();
		convertingFlag = true;

		while (convertingFlag)
			;;

	}

	currentSample = ADC_SAMPLE_COUNT;

	int total = 0;
	int i = 0;
	for (i = 0; i < ADC_SAMPLE_COUNT + 1; ++i) {
		total += valueArray[i];
	}

	float aveValue = (float) total / 11.0;

	normalizedADCRes = (aveValue * 3.3) / 16384;

	volatile float vwcValue = (0.0014 * normalizedADCRes * 1000) - 0.4697;

	MAP_ADC14_disableConversion();
	MAP_ADC14_disableInterrupt(ADC_INT1);

	GPIO_setOutputLowOnPin(EC5_POWER_PORT, EC5_POWER_PIN);

	vwc = vwcValue * 100;
	return vwc;
}

float get_latest_value( void ) {
	return vwc;
}

void ADC14_IRQHandler( void ) {
	uint64_t status = MAP_ADC14_getEnabledInterruptStatus();
	MAP_ADC14_clearInterruptFlag(status);

	if (ADC_INT1 & status) {
		valueArray[currentSample] = MAP_ADC14_getResult(ADC_MEM1);
//		normalizedADCRes = (curADCResult * 3.3) / 16384;

//		MAP_ADC14_toggleConversionTrigger();
		currentSample--;
		convertingFlag = false;
	}

}

