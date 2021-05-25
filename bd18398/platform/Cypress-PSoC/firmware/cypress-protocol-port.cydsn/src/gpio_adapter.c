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
	/*
	   for (size_t i = 0; i < g_gpio_comp_count; ++i) {
	   struct gpio_component *comp = &g_gpio_comps[i];
	   comp->start_ex(comp->isr);
	   }

	   g_gpio_if.intr_has_triggered = gpio_adp_has_interrupted;
	   g_gpio_if.intr_disable = gpio_adp_disable_int;
	   g_gpio_if.intr_enable = gpio_adp_enable_int;
	   g_gpio_if.intr_delta = gpio_adp_pin_int_delta;
	 */
	g_gpio_if.pin_configure = gpio_adp_configure_pin;
	g_gpio_if.pin_read = gpio_adp_read_state;
	g_gpio_if.pin_write = gpio_adp_write_state;

	/* All configurable GPIO interrupts are disabled in here, because they
	 * should be disabled after init but are enabled in the schematic.
	 * (They're enabled in the schematic to expose the IRQ pins.) */
	/*
	   for (size_t i = 0; i < g_gpio_pin_count; ++i) {
	   if (!gpio_adp_pin_can_interrupt(i)) {
	   continue;
	   }
	   EvkitGpioParams gpio_params = {
	   .gpio_nbr = i,
	   .gpio_sense = EVKIT_GPIO_PIN_NOSENSE,
	   .gpio_pin_pull = EVKIT_GPIO_PIN_NOPULL,
	   };
	   gpio_adp_disable_int(&gpio_params);
	   printf("gpio_init: disable %lu\r\n", (unsigned long)i);
	   }
	 */

	LOGGER_TRACEF("gpio: Initialization done.");
	return &g_gpio_if;
}

#if 0
bool irq_enabled(uint8_t pin_no)
{
	struct gpio_pin *pin = &g_gpio_pins[pin_no];

	return pin->intr.enabled;
}

uint8_t gpio_adp_enable_int_raw(uint8_t pin_no, int pin_conf, int trigger,
				bool enable_timestamp)
{
	struct gpio_pin *pin = &g_gpio_pins[pin_no];
	struct gpio_component *comp = pin->intr.comp;
	uint16_t intr_mode;

	gpio_adp_configure_pin(&(EvkitGpioConfigParams) {
			       .number = pin_no,.dir =
			       EVKIT_GPIO_PIN_INPUT,.input_connected =
			       EVKIT_GPIO_PIN_CONNECTED,.pin_position =
			       pin_conf,});

	if (pin->reserved) {
		LOGGER_ERRORF
		    ("gpio: cannot enable interrupt on pin %u; pin is reserved",
		     (unsigned)pin_no);
		return EBUSY;
	}

	switch (trigger) {
	case EVKIT_GPIO_PIN_NOSENSE:
		intr_mode = comp->intr_modes.none;
		break;
	case EVKIT_GPIO_PIN_SENSE_LOW:
		intr_mode = comp->intr_modes.falling;
		break;
	case EVKIT_GPIO_PIN_SENSE_HIGH:
		intr_mode = comp->intr_modes.rising;
		break;
	default:
		return EINVAL;
	}
	comp->set_interrupt_mode(pin->intr.cy_pos, intr_mode);
	pin->intr.enabled = true;
	pin->reserved = true;

	if (enable_timestamp) {
		comp->timestamp_enables[comp->timestamp_enables_len] = pin;
		comp->timestamp_enables_len++;
	}

	LOGGER_INFOF("gpio: Interrupt enabled for pin %u.", (unsigned)pin_no);
	return 0;
}

uint8_t gpio_adp_enable_int(EvkitGpioParams * params, bool enable_timestamp)
{
	if (!is_pin_valid_logged(params->gpio_nbr)) {
		return EINVAL;
	}
	if (!gpio_adp_pin_can_interrupt(params->gpio_nbr)) {
		LOGGER_ERRORF("gpio: pin %u does not support interrupts",
			      (unsigned)params->gpio_nbr);
		return EOPNOTSUPP;
	}
	return gpio_adp_enable_int_raw(params->gpio_nbr, params->gpio_pin_pull,
				       params->gpio_sense, enable_timestamp);

}

uint8_t gpio_adp_disable_int_raw(int pin_no)
{
	struct gpio_pin *pin = &g_gpio_pins[pin_no];
	struct gpio_component *comp = pin->intr.comp;
	uint8_t intr;
	uint16_t del_target_idx = UINT16_MAX;
	uint16_t i;

	if (!pin->intr.enabled) {
		return 0;
	}

	comp->set_interrupt_mode(pin->intr.cy_pos, comp->intr_modes.none);

	intr = CyEnterCriticalSection();
	/* Make sure we don't keep old interrupts around. */
	comp->intr_bits &= ~pin->intr.mask;
	CyExitCriticalSection(intr);

	for (i = 0; i < comp->timestamp_enables_len; ++i) {
		if (comp->timestamp_enables[i] == pin) {
			del_target_idx = i;
			break;
		}
	}

	if (del_target_idx != UINT16_MAX) {
		util_array_delete_v(comp->timestamp_enables,
				    comp->timestamp_enables_len,
				    del_target_idx);
		comp->timestamp_enables_len--;
	}
	pin->intr.enabled = false;
	pin->reserved = false;

	LOGGER_INFOF("gpio: Interrupt disabled for pin %u.", (unsigned)pin_no);
	return 0;
}

uint8_t gpio_adp_disable_int(EvkitGpioParams * params)
{
	if (!is_pin_valid_logged(params->gpio_nbr)) {
		return EINVAL;
	}
	if (!gpio_adp_pin_can_interrupt(params->gpio_nbr)) {
		LOGGER_ERRORF("gpio: pin %u does not support interrupts",
			      (unsigned)params->gpio_nbr);
		return EOPNOTSUPP;
	}

	return gpio_adp_disable_int_raw(params->gpio_nbr);
}

uint8_t gpio_adp_has_interrupted(uint8_t gpio_nbr, bool *has_interrupted)
{
	/* The pin validity should be checked when enabling interrupts. */
	CYASSERT(is_pin_valid_logged(gpio_nbr));

	struct gpio_pin *pin = &g_gpio_pins[gpio_nbr];
	struct gpio_component *comp = pin->intr.comp;
	uint8_t intr = CyEnterCriticalSection();
	if (comp->intr_bits & pin->intr.mask) {
		comp->intr_bits &= ~pin->intr.mask;
		*has_interrupted = true;
	} else {
		*has_interrupted = false;
	}
	CyExitCriticalSection(intr);

	return 0;
}
#endif

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

/*
uint8_t gpio_adp_pin_int_delta(uint8_t pin_nbr, uint32_t * delta_time)
{
	if (!is_pin_valid_logged(pin_nbr)) {
		return EINVAL;
	}

	const struct gpio_pin *pin = &g_gpio_pins[pin_nbr];

	uint8_t intr = CyEnterCriticalSection();
	int64_t diff =
	    (int64_t) pin->intr.prev_time - (int64_t) pin->intr.cur_time;
	CyExitCriticalSection(intr);
	const int time_bitwidth = 24;
	if (diff < 0) {
		diff = diff + (1 << time_bitwidth);
	}

	*delta_time = diff;
	return 0;
}

uint8_t gpio_adp_set_octrl(uint8_t pin_nbr, gpio_adp_octrl ctrl)
{
	if (!is_pin_valid_logged(pin_nbr)) {
		return EINVAL;
	}
	struct gpio_pin *pin = &g_gpio_pins[pin_nbr];
	switch (ctrl) {
	case GPIO_ADP_OCTRL_SW:
		*pin->reg_byp &= ~(1 << pin->reg_byp_shift);
		break;
	case GPIO_ADP_OCTRL_HW:
		*pin->reg_byp |= (1 << pin->reg_byp_shift);
		break;
	default:
		return EINVAL;
	}

	return 0;
}
*/
