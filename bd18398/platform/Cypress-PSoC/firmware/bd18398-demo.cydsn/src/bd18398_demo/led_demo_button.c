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

#include <stdio.h>
#include <cypins.h>
#include <GPIO_P1_1.h>
#include <ISR_GPIO_P1_1.h>
#include <GPIO_P1_1_aliases.h>
#include "bd18398_demo/led_demo_button.h"
#include "bd18398_demo/led_demo_time.h"
#include "gpio_adapter.h"

extern bool g_button_pushed;
extern uint32_t button_time;
extern void ack_button_irq();

#define BUTTON_PIN GPIO_P1_1_0

bool g_button_pushed = false;
uint32_t button_time = 0;

CY_ISR(isr_gpio_p1_1)
{
	uint8_t int_stat;

	GPIO_P1_1_SetInterruptMode(GPIO_P1_1_0_INTR, GPIO_P1_1_INTR_NONE);
	int_stat = GPIO_P1_1_ClearInterrupt();

	if (int_stat & 0x1) {
		g_button_pushed = 1;
		if (!button_time)
			button_time = led_demo_get_counter();
	}
}

void led_demo_enable_button_irq(void)
{
	ISR_GPIO_P1_1_StartEx(isr_gpio_p1_1);

	GPIO_P1_1_SetInterruptMode(GPIO_P1_1_INTR_ALL, GPIO_P1_1_INTR_NONE);
	GPIO_P1_1_SetInterruptMode(GPIO_P1_1_0_INTR, GPIO_P1_1_INTR_RISING);

	/* INPUT, PULL-UP */
	/* PULL-UP */
	CyPins_SetPin(BUTTON_PIN);
	CyPins_SetPinDriveMode(BUTTON_PIN, CY_PINS_DM_RES_UP);
}

/* Detect GPIO button pushes and handle state-machine accordinly */
bool led_demo_button_pushed()
{
	bool has_interrupted = g_button_pushed;
	static int ack_ctr_running = 0;

	if (ack_ctr_running) {
		if (100000 < get_us_since(button_time)) {
			ack_ctr_running = 0;
			button_time = 0;
			ack_button_irq();
		}
	}

	if (g_button_pushed) {
		ack_ctr_running = 1;
		g_button_pushed = false;
	}

	return has_interrupted;
}

void ack_button_irq()
{
	GPIO_P1_1_SetInterruptMode(GPIO_P1_1_0_INTR, GPIO_P1_1_INTR_RISING);
}
