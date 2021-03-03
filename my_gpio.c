/*!
 * \file      gpio.c
 *
 * \brief     GPIO driver implementation
 *
 * \remark: Relies on the specific board GPIO implementation as well as on
 *          IO expander driver implementation if one is available on the target
 *          board.
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include "gpio-board.h"
#include <my_gpio.h>
#include "my_timer.h"

void gpio_init(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config,
		PinTypes type, uint32_t value) {
	gpio_mcu_init(obj, pin, mode, config, type, value);
}

void gpio_set_context(Gpio_t *obj, void *context) {
	gpio_mcu_set_context(obj, context);
}

void gpio_set_interrupt(Gpio_t *obj, IrqModes irqMode,
		IrqPriorities irqPriority) {
	gpio_mcu_set_interrupt(obj, irqMode, irqPriority);
}

void gpio_remove_interrupt(Gpio_t *obj) {
	gpio_mcu_remove_interrupt(obj);
}

void gpio_write(Gpio_t *obj, uint32_t value) {
	gpio_mcu_write(obj, value);
}

void gpio_toggle(Gpio_t *obj) {
	gpio_mcu_toggle(obj);
}

uint32_t gpio_read(Gpio_t *obj) {
	return gpio_mcu_read(obj);
}

void gpio_flash_lED(Gpio_t *obj, uint8_t delay) {
	gpio_write(obj, 1);
	delay_ms(delay);
	gpio_write(obj, 0);
}
