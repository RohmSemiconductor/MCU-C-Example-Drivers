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

#include <stdio.h>

#include "project.h"
#include "gpio_adapter.h"
#include "pwm_adapter.h"
#include "spi_driver.h"

#include "bd18398_demo/led_demo_spi.h"
#include "bd18398_demo/bd18398.h"
#include "bd18398_demo/led_demo_led.h"

static void init_systicks(void)
{
	CySysTickStart();
	CySysTickDisableInterrupt();
	/* Counter maximum range is 24 bits. */
	CySysTickSetReload(0x00FFFFFF);
	CySysTickSetClockSource(CY_SYS_SYST_CSR_CLK_SRC_SYSCLK);
	CySysTickClear();
}

/* The GPIO module should be initialized before calling this. */
static void set_led(void)
{
	gpio_adp_configure_pin(&(EvkitGpioConfigParams) {
			       .number = 8,.dir =
			       EVKIT_GPIO_PIN_OUTPUT,.input_connected =
			       EVKIT_GPIO_PIN_DISCONNECTED,.pin_position =
			       EVKIT_GPIO_PIN_DRIVEHIGH,});
}

int main(void)
{
	evkit_gpio_if_t *gpio __attribute__((unused));
	CyGlobalIntEnable;
	init_systicks();
	UART_Start();

	gpio = gpio_adp_init();
	printf("GPIO init done\r\n");

	spi_drv_enable();
	printf("SPI init done\r\n");

	set_led();
	printf("LED pin configured\r\n");
	led_demo_set_config();

	for (;;) {
		static unsigned int i = 0;
		int loop = i % 100000;

		led_demo_loop();

		if (!loop)
			printf("DEMO Alive\r\n");

		i++;
	}
	led_demo_purge_config();
}
