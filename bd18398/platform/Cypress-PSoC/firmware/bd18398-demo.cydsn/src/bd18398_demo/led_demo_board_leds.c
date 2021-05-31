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

/* Control the EVK ERR and IND LEDs connected to Cypress GPIOs */

/* On real EVK these should be used */
#define LED_DEMO_ERR_LED 21
#define LED_DEMO_IND_LED 20

/* P2.1 (GPIO 8) is blue LED on cypress */
/*
#define LED_DEMO_ERR_LED 8
#define LED_DEMO_IND_LED 20
*/

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "CyLib.h"
#include "gpio_adapter.h"
#include "led_demo_board_leds.h"
#include "led_demo_time.h"

static void led_demo_set_board_led(int num, bool state)
{
	gpio_adp_configure_pin(&(EvkitGpioConfigParams) {
			       .number = num,.dir =
			       EVKIT_GPIO_PIN_OUTPUT,.input_connected =
			       EVKIT_GPIO_PIN_DISCONNECTED,.pin_position =
			       (state) ? EVKIT_GPIO_PIN_DRIVEHIGH :
			       EVKIT_GPIO_PIN_DRIVELOW,});
}

enum board_led_state {
	ERR_LED_OFF,
	ERR_LED_ON,
	ERR_LED_BLINK_SLOW_STARTING,
	ERR_LED_BLINK_SLOW,
	ERR_LED_BLINK_FAST_STARTING,
	ERR_LED_BLINK_FAST,
};

struct board_led {
	enum board_led_state state;
	uint32_t last_access;
	bool lit;
	int nmbr;
};

static void shut_board_led(struct board_led *l)
{
	led_demo_set_board_led(l->nmbr, false);
}

static void lit_board_led(struct board_led *l)
{
	led_demo_set_board_led(l->nmbr, true);
}

static void brd_led_toggle(struct board_led *l)
{
	if (!l)
		return;

	if (l->lit)
		shut_board_led(l);
	else
		lit_board_led(l);
	l->lit = !l->lit;
}

static void led_demo_board_led_blink(struct board_led *el, bool fast)
{
	if (!el)
		return;

	if (fast) {
		if (el->state != ERR_LED_BLINK_FAST) {
			el->state = ERR_LED_BLINK_FAST_STARTING;
			printf("board_led state changed to FAST\r\n");
		}
	} else {
		if (el->state != ERR_LED_BLINK_SLOW) {
			el->state = ERR_LED_BLINK_SLOW_STARTING;
			printf("board_led state changed to SLOW\r\n");
		}
	}
}

static struct board_led g_err_led = { 0, 0, 0,.nmbr = LED_DEMO_ERR_LED };
static struct board_led g_ind_led = { 0, 0, 0,.nmbr = LED_DEMO_IND_LED };

void led_demo_err_led_blink(bool fast)
{
	led_demo_board_led_blink(&g_err_led, fast);
}

int led_demo_err_led_blink_if_free(bool fast)
{
	if (g_err_led.state != ERR_LED_OFF && g_err_led.state != ERR_LED_ON)
		return -EBUSY;

	led_demo_board_led_blink(&g_err_led, fast);

	return 0;
}

int led_demo_ind_led_blink_if_free(bool fast)
{
	if (g_ind_led.state != ERR_LED_OFF && g_err_led.state != ERR_LED_ON)
		return -EBUSY;

	led_demo_board_led_blink(&g_ind_led, fast);

	return 0;
}

void led_demo_ind_led_blink(bool fast)
{
	led_demo_board_led_blink(&g_ind_led, fast);
}

static void led_demo_board_led_off(struct board_led *el)
{
	el->state = ERR_LED_OFF;
	shut_board_led(el);
}

static void led_demo_board_led_on(struct board_led *el)
{
	el->state = ERR_LED_ON;
	lit_board_led(el);
}

void led_demo_err_led_off(void)
{
	/*
	 * The ERR Led control logic is inverted. 
	 * When line is HIGH, ERR LED is OFF
	 */
	led_demo_board_led_on(&g_err_led);
}

void led_demo_err_led_on(void)
{
	/*
	 * The ERR Led control logic is inverted. 
	 * When line is HIGH, ERR LED is OFF
	 */
	led_demo_board_led_off(&g_err_led);
}

void led_demo_ind_led_off(void)
{
	led_demo_board_led_off(&g_ind_led);
}

void led_demo_ind_led_on(void)
{
	led_demo_board_led_on(&g_ind_led);
}

static void _led_demo_board_led_loop(struct board_led *el)
{
	unsigned int wait_us;
	unsigned int tmp;

	if (!el)
		return;

	switch (el->state) {
	case ERR_LED_BLINK_SLOW_STARTING:
		el->state = ERR_LED_BLINK_SLOW;
		wait_us = 0;
		break;
	case ERR_LED_BLINK_FAST_STARTING:
		el->state = ERR_LED_BLINK_FAST;
		wait_us = 0;
		break;
	case ERR_LED_BLINK_SLOW:
		wait_us = SLOW_BLINK_US;
		break;
	case ERR_LED_BLINK_FAST:
		wait_us = FAST_BLINK_US;
		break;
	default:
		return;
	}

	tmp = get_us_since(el->last_access);
	if (!wait_us || tmp > wait_us) {
		el->last_access = led_demo_get_counter();
		brd_led_toggle(el);
	}
}

void led_demo_board_led_loop(void)
{
	_led_demo_board_led_loop(&g_err_led);
	_led_demo_board_led_loop(&g_ind_led);
}
