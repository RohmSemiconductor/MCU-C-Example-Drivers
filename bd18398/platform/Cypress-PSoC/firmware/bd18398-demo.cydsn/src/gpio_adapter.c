// SPDX-License-Identifier: MIT
/*
 * The MIT License (MIT)
 * Copyright (c) 2021 ROHM Co.,Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "gpio_adapter.h"
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include "CyLib.h"
#include "cypins.h"

#include "gpio_spec.h"
#include "logger.h"
#include "util.h"

static evkit_gpio_if_t g_gpio_if;

static inline bool gpio_adp_pin_can_interrupt(uint8_t pin_nbr)
{
	const struct gpio_pin *pin = &g_gpio_pins[pin_nbr];
	return pin->features & GPIO_ADP_PIN_FEAT_INTR;
}

static bool is_pin_valid_logged(uint8_t pin_nbr)
{
	if (pin_nbr >= g_gpio_pin_count) {
		LOGGER_ERRORF("gpio: %u is not a valid pin", (unsigned)pin_nbr);
		return false;
	}
	/* P12.5 is used for SPI-SCLK and does not work properly as a GPIO. */
	if (pin_nbr == 17) {
		LOGGER_ERRORF
		    ("gpio: pin %u maps to P12.5, which is no longer available for GPIO use",
		     (unsigned)pin_nbr);
		return false;
	}

	return true;
}

evkit_gpio_if_t *gpio_adp_init(void)
{
	g_gpio_if.pin_configure = gpio_adp_configure_pin;
	g_gpio_if.pin_read = gpio_adp_read_state;
	g_gpio_if.pin_write = gpio_adp_write_state;

	LOGGER_TRACEF("gpio: Initialization done.");
	return &g_gpio_if;
}

uint8_t gpio_adp_read_state(uint8_t gpio_nbr, uint8_t * gpio_state)
{
	if (!is_pin_valid_logged(gpio_nbr)) {
		*gpio_state = EVKIT_GPIO_PIN_NOSENSE;
		return EINVAL;
	}

	const struct gpio_pin *pin = &g_gpio_pins[gpio_nbr];
	if (!(pin->features & GPIO_ADP_PIN_FEAT_DIGITAL)) {
		LOGGER_ERRORF("gpio: pin %u does not support digital I/O",
			      (unsigned)gpio_nbr);
		return EOPNOTSUPP;
	}

	if (CyPins_ReadPin(pin->cy_alias)) {
		*gpio_state = EVKIT_GPIO_PIN_SENSE_HIGH;
	} else {
		*gpio_state = EVKIT_GPIO_PIN_SENSE_LOW;
	}

	return 0;
}

uint8_t gpio_adp_write_state(uint8_t gpio_nbr, uint8_t drivestate)
{
	if (!is_pin_valid_logged(gpio_nbr)) {
		return EINVAL;
	}

	const struct gpio_pin *pin = &g_gpio_pins[gpio_nbr];
	if (!(pin->features & GPIO_ADP_PIN_FEAT_DIGITAL)) {
		LOGGER_ERRORF("gpio: pin %u does not support digital I/O",
			      (unsigned)gpio_nbr);
		return EOPNOTSUPP;
	}
	if (pin->reserved) {
		LOGGER_ERRORF("gpio: cannot write to pin %u; pin is reserved",
			      (unsigned)gpio_nbr);
		return EBUSY;
	}

	switch (drivestate) {
	case EVKIT_GPIO_PIN_DRIVEHIGH:
		CyPins_SetPin(pin->cy_alias);
		break;
	case EVKIT_GPIO_PIN_DRIVELOW:
		CyPins_ClearPin(pin->cy_alias);
		break;
	default:
		return EINVAL;
	}

	return 0;
}

uint8_t gpio_adp_configure_pin(EvkitGpioConfigParams * config)
{
	if (!is_pin_valid_logged(config->number)) {
		return EINVAL;
	}

	const struct gpio_pin *pin = &g_gpio_pins[config->number];
	if (!(pin->features & GPIO_ADP_PIN_FEAT_DIGITAL)) {
		LOGGER_ERRORF("gpio: pin %u does not support digital I/O",
			      (unsigned)config->number);
		return EOPNOTSUPP;
	}

	if (config->dir == EVKIT_GPIO_PIN_OUTPUT) {
		switch (config->pin_position) {
		case EVKIT_GPIO_PIN_NODRIVE:
			CyPins_SetPinDriveMode(pin->cy_alias,
					       CY_PINS_DM_ALG_HIZ);
			break;
		case EVKIT_GPIO_PIN_DRIVEHIGH:
			CyPins_SetPinDriveMode(pin->cy_alias,
					       CY_PINS_DM_STRONG);
			CyPins_SetPin(pin->cy_alias);
			break;
		case EVKIT_GPIO_PIN_DRIVELOW:
			CyPins_SetPinDriveMode(pin->cy_alias,
					       CY_PINS_DM_STRONG);
			CyPins_ClearPin(pin->cy_alias);
			break;
		default:
			return EINVAL;
		}
	} else if (config->dir == EVKIT_GPIO_PIN_INPUT) {
		uint8_t input_mode;
		switch (config->pin_position) {
		case EVKIT_GPIO_PIN_NOPULL:
			input_mode = CY_PINS_DM_DIG_HIZ;
			break;
		case EVKIT_GPIO_PIN_PULLUP:
			input_mode = CY_PINS_DM_RES_UP;
			CyPins_SetPin(pin->cy_alias);
			break;
		case EVKIT_GPIO_PIN_PULLDOWN:
			input_mode = CY_PINS_DM_RES_DWN;
			CyPins_ClearPin(pin->cy_alias);
			break;
		default:
			return EINVAL;
		}
		CyPins_SetPinDriveMode(pin->cy_alias, input_mode);
	} else {
		return EINVAL;
	}

	return 0;
}
