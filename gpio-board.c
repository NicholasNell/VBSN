/*
 * gpio-board.c
 *
 *  Created on: 03 Mar 2020
 *      Author: nicholas
 */

#include "board-config.h"
#include "gpio-board.h"

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value ) {
    obj.
}

void GpioMcuSetContext( Gpio_t *obj, void* context ) {

}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler ) {

}

void GpioMcuRemoveInterrupt( Gpio_t *obj ) {

}

void GpioMcuWrite( Gpio_t *obj, uint32_t value ) {

}

void GpioMcuToggle( Gpio_t *obj ) {

}

uint32_t GpioMcuRead( Gpio_t *obj ) {
    return 0;
}

/*void PORT1_IRQHandler ( void ) {

}*/
