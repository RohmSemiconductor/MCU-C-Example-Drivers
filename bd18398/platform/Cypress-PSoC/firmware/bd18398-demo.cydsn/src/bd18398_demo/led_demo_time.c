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

/*
 * Timekeeping which extends the 24bit, 72MHz Cypress counter to 32bits.
 *
 * Nothing here is thread or IRQ safe. Do not use in any scheduling/IRQ code.
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include "CyLib.h"
#include "led_demo_time.h"

/* This timekeeping is racy if called from IRQs or threads. */
static unsigned long g_bigtime = 0;

static inline void tiktaktime(uint32_t time)
{
	static uint32_t prev = 0;

	if (prev < time)
		g_bigtime++;

	prev = time;
}

void tiktak(void)
{
	volatile uint32_t time = CySysTickGetValue();

	tiktaktime(time);
}

#define MHZ72_SYSTICK 72000000LU
#define TICKS2US(ticks) ((ticks + 36LU)/72LU)
#define TICKS2NS(ticks) \
__extension__ ({				\
	unsigned __tmp__;			\
						\
	if ((ticks)*1000 > (ticks))		\
		__tmp__ = (ticks)*1000 / 72;	\
	else if ((ticks)/72*100 > (ticks))	\
		__tmp__ = (ticks)/72*100;	\
	else					\
		__tmp__ = 0xffffffff;		\
						\
	__tmp__;				\
})

uint32_t led_demo_get_counter()
{
	uint32_t ctr;
	uint32_t time = CySysTickGetValue();

	tiktaktime(time);
	/* The CySysTickGetValue counter counts from 0xffffff to zero */
	ctr = (g_bigtime << 24) | (0xffffff - time);

	return ctr;
}

unsigned int get_us_since(uint32_t prev)
{
	uint32_t time, now;
	unsigned int us;

	now = led_demo_get_counter();

	if (now >= prev)
		time = now - prev;
	else
		time = 0xffffffffLU - prev + now;

	us = TICKS2US(time);

	return us;
}

unsigned int get_ns_since(uint32_t ctr, unsigned *ns)
{
	uint32_t time = led_demo_get_counter();

	if (time > ctr)
		time = ctr - time;
	else
		time = 0xffffffff - time + ctr;

	*ns = TICKS2NS(time);
	if (*ns == 0xffffffff)
		return 0;

	return -EOVERFLOW;
}
