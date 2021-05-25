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

#include "bd18398_faults.h"
#include "bd18398_led.h"
#include <stdio.h>
#include "bd18398.h"
#include "led_demo_board_leds.h"
#include "led_demo_led.h"
#include "led_demo_spi.h"

/** is_failure_detected - get LED Driver fault statuses for all LEDs
 * @status	pointer where status bits are stored for further evaluation
 *
 * Query the LED IC status and feed the watchdog. This must be called more
 * frequently than once/second or IC assumes communication failure and
 * enters to LIMB HOME mode.
 *
 * Returns:	true if LED IC has detected failure. False if everything is Ok.
 */
bool is_failure_detected(uint8_t * status)
{
	int ret;

	ret = led_demo_spi_read_from(BD18398_STATUS_REG, status);

	return !(!ret && !(*status &= BD18398_STATUS_MASK));
}

/* Default error handlers - override these to do something meaningfull */
static void handle_uvlo()
{
	/* Turn off all LED VOUTs and set them faulty */
	evk_fault_all_leds();
	limited_print("UVLO\r\n", 1000);
	/* TODO: indicate fault using LED#2 on EVK board */
	led_demo_err_led_blink_if_free(true);
}

static void handle_pinuvlo()
{
	limited_print("PVIN_UVLO\r\n", 1000);
	evk_fault_all_leds();
	/* TODO: indicate fault using LED#2 on EVK board */
	led_demo_err_led_blink_if_free(false);
}

static void handle_crc()
{
	static int first = 0;

	if (first)
		limited_print("CRC\r\n", 1000);
}

/** handle_errors - call error handler functions for present errors
 *
 * @status	Error status obtained from call to @is_failure_detected
 *
 * This will execute error handlers for global erros (UVLO, CRC etc) and
 * also for LED errors as indicated by status bits (and by LED status
 * registers). LED error handlers are executed only if LEDs are initialized
 * by a call to led_init(). For LED errors the user-registered error handlers
 * are executed (see register_*_handler() - functions) or default handlers
 * if user has not registered any handlers.
 */
void handle_errors(uint8_t status)
{
	/* When SCP/OCP/foobar issue is detected, do things */
	int i;
	void (*hndlr[8])(void) = {
		[BD18398_STATUS_BIT_UVLO] = handle_uvlo,
		[BD18398_STATUS_BIT_PINUVLO] = handle_pinuvlo,
		[BD18398_STATUS_BIT_CRC] = handle_crc,
	};

	if (status & BD18398_STATUS_MASK_ERRDET)
		handle_led_errors(status & BD18398_STATUS_MASK_ERRDET);

	if (!(status & ~BD18398_STATUS_MASK_ERRDET))
		return;

	for (i = BD18398_STATUS_BIT_UVLO; i < 8; i++) {
		if ((1 << i) & status && hndlr[i])
			hndlr[i] ();
	}
}
