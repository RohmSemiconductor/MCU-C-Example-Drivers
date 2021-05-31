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

/* Pin and component data structures and their global storage. */
#ifndef GPIO_SPEC_H
#define GPIO_SPEC_H

#include <stdint.h>
#include <stdbool.h>

#include "CyLib.h"
#include "cytypes.h"

/* Support for analog input. */
#define GPIO_ADP_PIN_FEAT_ANALOG 0x01
/* Support for digital input/output. */
#define GPIO_ADP_PIN_FEAT_DIGITAL 0x02
/* Support for interrupts. */
#define GPIO_ADP_PIN_FEAT_INTR 0x08

struct gpio_component;

struct gpio_pin {
	/* Feature flags. */
	uint16_t features;
	/* Flag for the pin being in exclusive use for something (e.g. PWM). */
	bool reserved;

	/* Cypress name for the pin. (Used by Cypress pin API.) */
	uint32_t cy_alias;

	/* GPIO port bypass register and bit position. */
	reg8 *reg_byp;
	uint8_t reg_byp_shift;

	union {
		/* Interrupt parameters.
		 * Available when features has GPIO_ADP_PIN_FEAT_INTR. */
		struct {
			/* Pin position for the Cypress pin API. */
			uint16_t cy_pos;
			/* The PINS component to which this pin belongs to. */
			struct gpio_component *comp;
		} intr;

		/* Analog parameters.
		 * Available when features has GPIO_ADP_PIN_FEAT_ANALOG. */
		struct {
			/* ADC mux channel to which a pin is attached. */
			uint8_t mux_chan;
		} analog;
	};
};

extern unsigned g_gpio_comp_count;

/* All the pins available. */
extern struct gpio_pin g_gpio_pins[];
extern unsigned g_gpio_pin_count;

#endif
