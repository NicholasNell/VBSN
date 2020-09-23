/** @file my_scheduler.c
 *
 * @brief
 *
 *  Created on: 23 Sep 2020
 *      Author: njnel
 */

#include <helper.h>
#include <my_MAC.h>
#include <my_UART.h>
#include "my_scheduler.h"
#include <stdint.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>

uint16_t slotCount;
extern uint16_t neighbourSyncSlots[255];

extern uint8_t _numNeighbours;

void initSchedule() {
	// Set up interrupt to increment slots (Using GPS PPS signal)

	MAP_GPIO_setAsInputPinWithPullDownResistor(GPIO_PORT_P4, GPIO_PIN4);
	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, GPIO_PIN4);
	MAP_GPIO_enableInterrupt(GPIO_PORT_P4, GPIO_PIN4);
	MAP_Interrupt_enableInterrupt(INT_PORT4);

	slotCount = 0;
}

void scheduler() {
	int var = 0;
	for (var = 0; var < _numNeighbours; ++var) {
		if (slotCount == neighbourSyncSlots[var]) {
			MACRx(1000);
		}
	}

	if (slotCount == 40) { // collect Sensor Data
		helper_collectSensorData();
	} else if (slotCount == 30) {	// update GPS information
		sendUARTgps("$PCAS03,0,1,0,0,0,0,0,0*03\r\n");
	} else if (slotCount == 35) { // update gps timing information
		sendUARTgps("$PCAS03,0,0,0,0,0,0,1,0*03\r\n");
	} else if (slotCount == 50) {

	} else {

	}
}

void PORT4_IRQHandler(void) {
	uint32_t status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P4);

	MAP_GPIO_clearInterruptFlag(GPIO_PORT_P4, status);

	if (status & GPIO_PIN4) {

	}
}

