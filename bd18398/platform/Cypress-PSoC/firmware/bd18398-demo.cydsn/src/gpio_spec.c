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

#include "gpio_spec.h"

#include "cyfitter.h"
#include "GPIO_P0_1.h"
#include "GPIO_P1_0.h"
#include "GPIO_P1_1.h"
#include "GPIO_P2_0.h"
#include "GPIO_P3_0.h"
#include "GPIO_P3_2.h"
#include "GPIO_P12_2.h"
#include "GPIO_P15_0.h"
#include "GPIO_P15_1.h"
#include "GPIO_P3_1_aliases.h"

#include "util.h"

/* Generate an initializer for a gpio_pin struct.
 *
 * comp_name => PINS component name.
 * hwpos => GPIO pin position within the component.
 * comp_ptr => Pointer to the associated gpio_component struct.
 */

#define PIN_INIT_DIGITAL(comp_name, hwpos) { \
    .features = GPIO_ADP_PIN_FEAT_DIGITAL, \
    .reserved = false, \
    .cy_alias = comp_name ## _ ## hwpos, \
    .reg_byp = (reg8 *)comp_name ## __BYP, \
    .reg_byp_shift = comp_name ## __ ## hwpos ## __SHIFT, \
}

#define PIN_INIT_ANALOG(comp_name, hwpos, amux_chan) { \
    .features = GPIO_ADP_PIN_FEAT_ANALOG, \
    .reserved = true, \
    .cy_alias = comp_name ## _ ## hwpos, \
    .reg_byp = (reg8 *)comp_name ## __BYP, \
    .reg_byp_shift = comp_name ## __ ## hwpos ## __SHIFT, \
    .analog.mux_chan = amux_chan, \
}

/* All pins managed by this module. */
struct gpio_pin g_gpio_pins[] = {
	/* P12.3 */
	PIN_INIT_DIGITAL(GPIO_P12_2, 0 /* &COMP_GPIO_P12_2 */ ),
	/* P3.0 */
	PIN_INIT_DIGITAL(GPIO_P3_0, 0),
	/* P3.1 */
	PIN_INIT_DIGITAL(GPIO_P3_0, 1),
	/* P3.2 */
	PIN_INIT_DIGITAL(GPIO_P3_0, 2),
	/* P3.3 */
	PIN_INIT_DIGITAL(GPIO_P3_0, 3),
	/* P3.4 */
	PIN_INIT_ANALOG(GPIO_P3_1, 0, 1),
	/* P3.5 */
	PIN_INIT_ANALOG(GPIO_P3_1, 1, 2),
	/* P3.6 */
	PIN_INIT_ANALOG(GPIO_P3_1, 2, 3),
	/* P2.1 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 1),
	/* P2.2 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 2),
	/* P2.3 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 3),
	/* P2.4 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 4),
	/* P2.5 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 5),
	/* P2.6 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 6),
	/* P2.7 */
	PIN_INIT_DIGITAL(GPIO_P2_0, 7),
	/* P1.5 */
	PIN_INIT_DIGITAL(GPIO_P1_0, 0),
	/* P1.7 */
	PIN_INIT_DIGITAL(GPIO_P1_0, 2),
	/* Pin 17 used to be P12.5, which is now reserved for SPI and cannot be
	 * used as a GPIO pin. */
	/* P2.0 */
	[18] = PIN_INIT_DIGITAL(GPIO_P2_0, 0),
	/* P0.5 */
	PIN_INIT_DIGITAL(GPIO_P0_1, 0),
	/* P0.6 IND LED */
	PIN_INIT_DIGITAL(GPIO_P0_1, 1),
	/* P0.7 ERR LED */
	PIN_INIT_DIGITAL(GPIO_P0_1, 2),
	/* P1.2 - BUTTON */
	PIN_INIT_DIGITAL(GPIO_P1_1, 0),
	/* P15.0 */
	PIN_INIT_DIGITAL(GPIO_P15_0, 0),
	/* P15.1 */
	PIN_INIT_DIGITAL(GPIO_P15_0, 1),
	/* P15.5 */
	PIN_INIT_DIGITAL(GPIO_P15_1, 0),
	/* P1.3 */
	PIN_INIT_DIGITAL(GPIO_P1_1, 1),
	/* P1.4 */
	PIN_INIT_DIGITAL(GPIO_P1_1, 2),
	/* P3.7 */
	PIN_INIT_DIGITAL(GPIO_P3_2, 0),
};

unsigned g_gpio_pin_count = UTIL_ARRAY_LEN(g_gpio_pins);
