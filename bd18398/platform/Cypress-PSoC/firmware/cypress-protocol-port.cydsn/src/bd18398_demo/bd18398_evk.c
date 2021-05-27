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

/* We need this for raw access to chipselect to overcome DS2 bug */
#include "../spi_driver.h"

#include "bd18398.h"
#include "bd18398_evk.h"
#include "led_demo_spi.h"
#include "led_demo_time.h"
#include "led_demo_button.h"
#include "led_demo_board_leds.h"

static void led_demo_reset()
{
	led_demo_spi_update_bits(BD18398_SYS_REG, BD18398_RESET_MASK,
				 BD18398_RESET_MASK);
	LED_DEMO_DELAY_MS(10);
}

static void led_demo_enable_wlock()
{
	int ret;
	uint8_t val;

	ret =
	    led_demo_spi_update_bits(BD18398_SYS_REG, BD18398_WLOCK_MASK,
				     BD18398_WLOCK_MASK);
	if (ret)
		printf("Failed to set wlock\r\n");

	ret = led_demo_spi_read_from(BD18398_SYS_REG, &val);
	if (ret || (val & BD18398_WLOCK_MASK) != BD18398_WLOCK_MASK)
		printf("SPI failure\r\n");
}

int bd18398_evk_config()
{
	/* Initialize SPI pins */
	spi_drv_init_miso_cs();

	/* Set SPI clk frequency to 1MHz */
	spi_drv_set_sclk_freq(1000);

	/* DS2 bug - chipselect must be set LOW to start the IC */
	spi_drv_set_ncs();
	LED_DEMO_DELAY_MS(1);
	spi_drv_clear_ncs();

	led_demo_reset();
	LED_DEMO_DELAY_MS(200);
	spi_drv_set_ncs();
	LED_DEMO_DELAY_MS(1);
	spi_drv_clear_ncs();

	led_demo_enable_wlock();
	led_demo_enable_button_irq();

	/* We turn on the IND LED at startup */
	led_demo_ind_led_on();

	return 0;
}
