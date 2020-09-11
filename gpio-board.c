/*
 * gpio-board.c
 *
 *  Created on: 03 Mar 2020
 *      Author: nicholas
 */
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <stdlib.h>
#include <math.h>
#include "board-config.h"
#include "gpio-board.h"

void GpioMcuInit(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config,
		PinTypes type, uint32_t value) {
	/*
	 * #define GPIO_PORT_P1                                                          1
	 #define GPIO_PORT_P2                                                          2
	 #define GPIO_PORT_P3                                                          3
	 #define GPIO_PORT_P4                                                          4
	 #define GPIO_PORT_P5                                                          5
	 #define GPIO_PORT_P6                                                          6
	 #define GPIO_PORT_P7                                                          7
	 #define GPIO_PORT_P8                                                          8
	 #define GPIO_PORT_P9                                                          9
	 #define GPIO_PORT_P10                                                         10
	 #define GPIO_PORT_PA                                                           1
	 #define GPIO_PORT_PB                                                           3
	 #define GPIO_PORT_PC                                                           5
	 #define GPIO_PORT_PD                                                           7
	 #define GPIO_PORT_PE                                                           9
	 #define GPIO_PORT_PJ                                                          11


	 #define GPIO_PIN0                                                      (0x0001)
	 #define GPIO_PIN1                                                      (0x0002)
	 #define GPIO_PIN2                                                      (0x0004)
	 #define GPIO_PIN3                                                      (0x0008)
	 #define GPIO_PIN4                                                      (0x0010)
	 #define GPIO_PIN5                                                      (0x0020)
	 #define GPIO_PIN6                                                      (0x0040)
	 #define GPIO_PIN7                                                      (0x0080)
	 #define GPIO_PIN8                                                      (0x0100)
	 #define GPIO_PIN9                                                      (0x0200)
	 #define GPIO_PIN10                                                     (0x0400)
	 #define GPIO_PIN11                                                     (0x0800)
	 #define GPIO_PIN12                                                     (0x1000)
	 #define GPIO_PIN13                                                     (0x2000)
	 #define GPIO_PIN14                                                     (0x4000)
	 #define GPIO_PIN15                                                     (0x8000)
	 #define PIN_ALL8                                                       (0xFF)
	 #define PIN_ALL16                                                      (0xFFFF)
	 */

	obj->pin = pin;

	if (pin == NC) {
		return;
	} // If pin is not connected return

	uint8_t t = (uint8_t) (obj->pin);
	uint8_t y = (floor(t / 15));
	t = t - ((16) * y);

	obj->pinIndex = 0x0001 << t;

	if ((obj->pin & 0xF0) == 0x00) {
		obj->portIndex = GPIO_PORT_P1;
	} else if ((obj->pin & 0xF0) == 0x10) {
		obj->portIndex = GPIO_PORT_P2;
	} else if ((obj->pin & 0xF0) == 0x20) {
		obj->portIndex = GPIO_PORT_P3;
	} else if ((obj->pin & 0xF0) == 0x30) {
		obj->portIndex = GPIO_PORT_P4;
	} else if ((obj->pin & 0xF0) == 0x40) {
		obj->portIndex = GPIO_PORT_P5;
	} else if ((obj->pin & 0xF0) == 0x50) {
		obj->portIndex = GPIO_PORT_P6;
	} else if ((obj->pin & 0xF0) == 0x60) {
		obj->portIndex = GPIO_PORT_P7;
	} else if ((obj->pin & 0xF0) == 0x70) {
		obj->portIndex = GPIO_PORT_P8;
	} else if ((obj->pin & 0xF0) == 0x80) {
		obj->portIndex = GPIO_PORT_P9;
	} else if ((obj->pin & 0xF0) == 0x90) {
		obj->portIndex = GPIO_PORT_P10;
	} else if ((obj->pin & 0xF0) == 0xF0) {
		obj->portIndex = GPIO_PORT_PJ;
	} else {
		return;
	} // determine port mapping for each pin

	if (mode == PIN_INPUT) {

		if (type == PIN_PULL_UP) {
			MAP_GPIO_setAsInputPinWithPullUpResistor(obj->portIndex,
					obj->pinIndex);
		} else if (type == PIN_PULL_DOWN) {
			MAP_GPIO_setAsInputPinWithPullDownResistor(obj->portIndex,
					obj->pinIndex);
		} else {
			MAP_GPIO_setAsInputPin(obj->portIndex, obj->pinIndex);
		}
	} // Set as input if defined as input pin
	else {
		MAP_GPIO_setAsOutputPin(obj->portIndex, obj->pinIndex);
		GpioMcuWrite(obj, value);
	} // set as output and low else
}

void GpioMcuSetContext(Gpio_t *obj, void *context) {
	obj->Context = context;
}

void GpioMcuSetInterrupt(Gpio_t *obj, IrqModes irqMode,
		IrqPriorities irqPriority) {
	/*
	 #define INT_PORT1                                       (51)  PORT1 IRQ
	 #define INT_PORT2                                       (52)  PORT2 IRQ
	 #define INT_PORT3                                       (53)  PORT3 IRQ
	 #define INT_PORT4                                       (54)  PORT4 IRQ
	 #define INT_PORT5                                       (55)  PORT5 IRQ
	 #define INT_PORT6                                       (56)  PORT6 IRQ
	 */
	uint32_t INT_PORTx = 0x0;
	switch (obj->portIndex) {
	case GPIO_PORT_P1:
		INT_PORTx = INT_PORT1;
		break;
	case GPIO_PORT_P2:
		INT_PORTx = INT_PORT2;
		break;
	case GPIO_PORT_P3:
		INT_PORTx = INT_PORT3;
		break;
	case GPIO_PORT_P4:
		INT_PORTx = INT_PORT4;
		break;
	case GPIO_PORT_P5:
		INT_PORTx = INT_PORT5;
		break;
	case GPIO_PORT_P6:
		INT_PORTx = INT_PORT6;
		break;
	default:
		break;
	}

	if (irqMode == IRQ_RISING_EDGE) {
		MAP_GPIO_clearInterruptFlag(obj->portIndex, obj->pinIndex);
		GPIO_enableInterrupt(obj->portIndex, obj->pinIndex);
		Interrupt_setPriority(INT_PORTx, 0x20);
		MAP_Interrupt_enableInterrupt(INT_PORTx);
		MAP_GPIO_interruptEdgeSelect(obj->portIndex, obj->pinIndex,
		GPIO_LOW_TO_HIGH_TRANSITION);

	} else if (irqMode == IRQ_FALLING_EDGE) {
		MAP_GPIO_clearInterruptFlag(obj->portIndex, obj->pinIndex);
		MAP_GPIO_enableInterrupt(obj->portIndex, obj->pinIndex);
		Interrupt_setPriority(INT_PORTx, 0x20);
		MAP_Interrupt_enableInterrupt(INT_PORTx);
		MAP_GPIO_interruptEdgeSelect(obj->portIndex, obj->pinIndex,
		GPIO_HIGH_TO_LOW_TRANSITION);
	} else {
		MAP_GPIO_clearInterruptFlag(obj->portIndex, obj->pinIndex);
		MAP_GPIO_enableInterrupt(obj->portIndex, obj->pinIndex);
		Interrupt_setPriority(INT_PORTx, 0x20);
		MAP_Interrupt_enableInterrupt(INT_PORTx);
	}
}

void GpioMcuRemoveInterrupt(Gpio_t *obj) {
	GPIO_disableInterrupt(obj->portIndex, obj->pinIndex);
}

void GpioMcuWrite(Gpio_t *obj, uint32_t value) {
	if (obj == NULL) {
		while (1)
			;
	} // Error if object does not exist

	if (obj->pin == NC) {
		return;
	} // return if pin is not connected

	GPIO_setAsOutputPin(obj->portIndex, obj->pinIndex);
	if (value == 0) {
		GPIO_setOutputLowOnPin(obj->portIndex, obj->pinIndex);
	} else {
		GPIO_setOutputHighOnPin(obj->portIndex, obj->pinIndex);
	} // if value = 0 set low else set high.
}

void GpioMcuToggle(Gpio_t *obj) {
	if (obj == NULL) {
		while (1)
			;
	}

	// Check if pin is not connected
	if (obj->pin == NC) {
		return;
	}
	GPIO_toggleOutputOnPin(obj->portIndex, obj->pinIndex);
}

uint32_t GpioMcuRead(Gpio_t *obj) {
	if (obj == NULL) {
		while (1)
			;
	}
	// Check if pin is not connected
	if (obj->pin == NC) {
		return 0;
	}
	return GPIO_getInputPinValue(obj->portIndex, obj->pinIndex);
}
