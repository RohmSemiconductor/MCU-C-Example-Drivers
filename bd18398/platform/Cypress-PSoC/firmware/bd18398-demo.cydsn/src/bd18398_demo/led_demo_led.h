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

#ifndef LED_DEMO_LED_H
#define LED_DEMO_LED_H

#include <stdint.h>
#include <stdbool.h>

/* Query LED fault state (cached) */
bool evk_any_led_faulty(void);

/* Mark LED(s) faulty */
void evk_fault_all_leds(void);

/* Clear faulty mark and re-enable */
int evk_fault_cancel_all_leds(uint8_t *status, int num_status);
int evk_initialize_leds(void);
void evk_led_loop(void);
int evk_get_led_errors(uint8_t *status, int num_status);

/* Set LED pattern and timing */
/* LED patterns */
#define LED_OFF			0
#define LED_FULL_ON		1
#define LED_SMOOTH_CYCLE	2

/* LED timing */
#define LED_TIMING_START	0x00
#define LED_TIMING_QUARTER_PAST	0x30
#define LED_TIMING_HALF		0x60
#define LED_TIMING_QUARTER_TO	0x90

int evk_led_set_state(int led_no, uint8_t new_pattern, uint8_t new_timing);

#endif
