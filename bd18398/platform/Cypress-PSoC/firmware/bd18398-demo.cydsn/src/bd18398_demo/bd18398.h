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
 *
 * Author: Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>
 */

#ifndef _BD18398_H
#define _BD18398_H
#include <stdint.h>

#define BIT(foo)			(1 << (foo))

#define BD18398_NUM_LEDS		3
#define LED_MAX_INDEX			(BD18398_NUM_LEDS - 1)
#define BD18398_STATUS_REG		0x18
#define BD18398_LED1_STATUS_REG		0x19
#define BD18398_LED2_STATUS_REG		0x1A
#define BD18398_LED3_STATUS_REG		0x1B
#define BD18398_STATUS_MASK		0xf7
#define BD18398_STATUS_BIT_WDT		7
#define BD18398_STATUS_MASK_WDT		BIT(BD18398_STATUS_BIT_WDT)
#define BD18398_STATUS_BIT_CRC		6
#define BD18398_STATUS_MASK_CRC		BIT(BD18398_STATUS_BIT_CRC)
#define BD18398_STATUS_BIT_PINUVLO	5
#define BD18398_STATUS_MASK_PINUVLO	BIT(BD18398_STATUS_BIT_PINUVLO)
#define BD18398_STATUS_BIT_UVLO		4
#define BD18398_STATUS_MASK_UVLO	BIT(4)
#define BD18398_STATUS_MASK_ERRDET	0x07

#define BD18398_LED_ERR_BIT_LSD		0
#define BD18398_LED_ERR_BIT_LOD		1
#define BD18398_LED_ERR_BIT_SWOCP	2
#define BD18398_LED_ERR_BIT_LOCP	3

#define BD18398_LED_DIMSET_MASK		0x7
#define BD18398_LED_DIMSET_REG		0x3

/* The LED current (units of mA) when brightness is set to max */
#define BD18398_MAX_LED_CURRENT		500UL

/* A/D full scale reference voltage uV */
#define BD18398_ADC_VREF		2500000

/* Sense resistor size in milliohms */
#define BD18398_RSENSE			100UL

/* For ISET */
#define BD18398_LED1_CURRENT_REG_H	0x04
#define BD18398_LED1_CURRENT_REG_L	0x05
#define BD18398_LED2_CURRENT_REG_H	0x06
#define BD18398_LED2_CURRENT_REG_L	0x07
#define BD18398_LED3_CURRENT_REG_H	0x08
#define BD18398_LED3_CURRENT_REG_L	0x09

/* For DPWM */
#define BD18398_LED1_DPWM_REG_H		0x0a
#define BD18398_LED1_DPWM_REG_L		0x0b
#define BD18398_LED2_DPWM_REG_H		0x0c
#define BD18398_LED2_DPWM_REG_L		0x0d
#define BD18398_LED3_DPWM_REG_H		0x0e
#define BD18398_LED3_DPWM_REG_L		0x0f

/* iset value is 10 bits */
#define BRIGHTNESS_MAX			(BIT(10) - 1)
#define BD18398_LED_ON_REG		0x14
#define BD18398_LED1_ON_MASK		(BIT(0) /*| BIT(4)*/)
#define BD18398_LED2_ON_MASK		(BIT(1) /*| BIT(5)*/)
#define BD18398_LED3_ON_MASK		(BIT(2) /*| BIT(6)*/)

#define BD18398_LED1_DIMM_MASK		(BIT(4))
#define BD18398_LED2_DIMM_MASK		(BIT(5))
#define BD18398_LED3_DIMM_MASK		(BIT(6))

#define BD18398_SYS_REG			0x0
#define BD18398_RESET_MASK		BIT(0)
#define BD18398_WLOCK_MASK		BIT(7)

int led_demo_set_config(void);
void led_demo_purge_config(void);
void led_demo_loop(void);
uint32_t led_demo_get_counter(void);

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#endif

#define limited_print(foo, lim)			\
{						\
	static unsigned int __plaaplaa = 0;	\
						\
	if (!(__plaaplaa % (lim)))		\
		printf(foo);			\
	__plaaplaa++;				\
}

#define limited_print2(foo, bar, lim)		\
{						\
	static unsigned int __plaaplaa = 0;	\
						\
	if (!(__plaaplaa % (lim)))		\
		printf(foo, bar);		\
	__plaaplaa++;				\
}

#endif
