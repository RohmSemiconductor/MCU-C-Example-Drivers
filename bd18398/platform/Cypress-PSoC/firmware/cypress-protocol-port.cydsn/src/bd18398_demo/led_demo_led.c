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

/* This is the BD18398 EVK example code using the bd18398_led object.
 *
 * Build some 'blinking / fading' logic for LEDs using BD18398 and Cypress
 * MCU timing facilities. Provide also functions to initialize / fault all
 * EVK LEDs (all channels) at one call.
 */
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bd18398.h"
#include "led_demo_led.h"
#include "led_demo_spi.h"
#include "led_demo_time.h"
#include "bd18398_led.h"
#include "led_demo_board_leds.h"

/*
 * When we change LED from 0 => full => 0 in 'smooth' way
 * we increase or decrease current in register once / 3 ms
 */
#define LED_SMOOTH_CYCLE_MS	3
#define LED_SMOOTH_CYCLE_FULL	(LED_SMOOTH_CYCLE_MS * BRIGHTNESS_MAX)
#define LED_TIMING_MASK		0xf0
#define LED_PATTERN_MASK	0x0f

#define LED_TIMING_ADJUSTED_BRIGHTNESS(bright, timing)			       \
__extension__({								       \
	uint16_t __foo__;						       \
									       \
	if ((timing) < LED_TIMING_QUARTER_PAST)				       \
		__foo__ = ((bright) % BRIGHTNESS_MAX);			       \
	else if ((timing) < LED_TIMING_HALF)				       \
		__foo__ = (((bright) + BRIGHTNESS_MAX/4) % BRIGHTNESS_MAX);    \
	else if ((timing) < LED_TIMING_QUARTER_TO)			       \
		__foo__ = (((bright) + BRIGHTNESS_MAX/2) % BRIGHTNESS_MAX);    \
	else								       \
		__foo__ = (((bright) + (BRIGHTNESS_MAX*3)/4) % BRIGHTNESS_MAX);\
	__foo__;							       \
})

struct led_demo_led {
	uint32_t last_acces;
	uint32_t smooth_bright;
	uint8_t led_flags;	/* Composed of patterns and timings */
	uint8_t led_prev_flags;
	struct bd18398_led *led_hw;
};

static struct led_demo_led g_leds[BD18398_NUM_LEDS] = { {0, 0, 0, 0, 0} };

static int evk_led_fault(int led_no)
{
	struct led_demo_led *l = &g_leds[led_no];
	struct bd18398_led *led_hw;

	if (led_no > LED_MAX_INDEX || !l->led_hw)
		return -EINVAL;

	led_hw = l->led_hw;

	/*
	 * This is not the correct thing-to-do. We should disable
	 * failing LED but the demo env gives OPEN/SHORT errors all the time.
	 * Thus we leave the LED ON.
	 */
	return 0;

	if (led_hw->is_faulty(led_hw))
		return 0;

	led_hw->set_faulty(led_hw);

	return led_hw->off(led_hw);
}

static int evk_led_fault_cancel(int led_no)
{
	struct led_demo_led *l = &g_leds[led_no];
	struct bd18398_led *led_hw;
	int ret;

	if (led_no > LED_MAX_INDEX || !l->led_hw)
		return -EINVAL;

	led_hw = l->led_hw;

	if (!led_hw->is_faulty(led_hw))
		return 0;

	ret = led_hw->fault_cancel(led_hw);
	if (ret)
		return ret;

	return led_hw->on(led_hw);
}

static inline void set_all_leds_fault(bool fault)
{
	int i;
	int (*f)(int);

	if (fault)
		f = evk_led_fault;
	else
		f = evk_led_fault_cancel;

	for (i = 0; i < BD18398_NUM_LEDS; i++)
		f(i);
}

/* Set state of all EVK LEDs */
bool evk_any_led_faulty(void)
{
	int i;

	for (i = 0; i < BD18398_NUM_LEDS; i++)
		if (g_leds[i].led_hw
		    && g_leds[i].led_hw->is_faulty(g_leds[i].led_hw))
			return true;

	return false;
}

void evk_fault_cancel_all_leds()
{
	set_all_leds_fault(false);
}

void evk_fault_all_leds()
{
	set_all_leds_fault(true);
}

/* Set simple 'LED patterns' */
int evk_led_set_state(int led_no, uint8_t new_pattern, uint8_t new_timing)
{
	struct led_demo_led *l = &g_leds[led_no];
	struct bd18398_led *led_hw;
	uint8_t pattern;
	int ret = 0;

	if (led_no > LED_MAX_INDEX || !l->led_hw) {
		printf("set_led_state: invalid LED\r\n");
		return -EINVAL;
	}

	led_hw = l->led_hw;

	if (!led_hw) {
		printf("No LED HW - init failed?\r\n");
		return -EINVAL;
	}

	if (led_hw->is_faulty(led_hw)) {
		printf("set_led_state: Led marked faulty - bailing out\r\n");
		return 0;
	}

	l->led_flags &= (~LED_TIMING_MASK);
	l->led_flags |= (new_timing & LED_TIMING_MASK);
	pattern = (l->led_flags & LED_PATTERN_MASK);

	if (new_pattern != pattern) {
		/* New pattern was set */
		l->led_flags &= ~(LED_PATTERN_MASK);
		l->led_flags |= new_pattern;
		switch (new_pattern) {
		case LED_OFF:
			led_hw->off(led_hw);
			ret = led_hw->set_brightness(led_hw, 0);
			l->last_acces = 0;
			break;
		case LED_FULL_ON:
			led_hw->on(led_hw);
			ret = led_hw->set_brightness(led_hw, BRIGHTNESS_MAX);
			l->last_acces = 0;
			break;
		case LED_SMOOTH_CYCLE:
			{
				uint8_t timing = l->led_flags & LED_TIMING_MASK;
				uint8_t brightness;

				led_hw->on(led_hw);
				l->last_acces = led_demo_get_counter();
				l->smooth_bright = 1,
				    brightness =
				    LED_TIMING_ADJUSTED_BRIGHTNESS(1, timing);
				ret =
				    led_hw->set_brightness(led_hw, brightness);
				break;
			}
		default:
			return -EINVAL;
		}
	}

	return ret;
}

/* The main loop to do LED fading */
void evk_led_loop()
{
	int i;
	int ret;

	for (i = 0; i < BD18398_NUM_LEDS; i++) {
		struct led_demo_led *l = &g_leds[i];
		struct bd18398_led *led_hw = l->led_hw;
		uint8_t pattern, timing;

		if (!led_hw)
			continue;

		if (led_hw->is_faulty(led_hw)) {
			printf("LED %d faulty - skipping handling\r\n", i);
			continue;
		}

		timing = (l->led_flags & LED_TIMING_MASK);
		pattern = (l->led_flags & LED_PATTERN_MASK);
		switch (pattern) {
		case LED_SMOOTH_CYCLE:
			{
				uint16_t brightness;

				if (1000 > get_us_since(l->last_acces))
					break;
				else
					l->smooth_bright =
					    ((l->smooth_bright +
					      1) % BRIGHTNESS_MAX);

				brightness =
				    LED_TIMING_ADJUSTED_BRIGHTNESS
				    (l->smooth_bright, timing);

				ret =
				    led_hw->set_brightness(led_hw, brightness);
				if (ret)
					printf("Failed to set brightness\r\n");
				break;
			}
		default:
			break;
		}
	}
}

/* Just an example of user registered error handler */
static void LED_user_handler_short(struct bd18398_led *_this
				   __attribute__((unused)), void *opaque
				   __attribute__((unused)))
{
	/*
	 * You can enable these prints just to test how the opaque ptr can be
	 * used. That frequent printing is likely to render the demo pretty
	 * unusable
	 */
	/*
	 * printf("User SHORT handler for led %d\r\n", _this->id);
	 * printf("Extra message: %s\r\n", (char *)opaque);
	 */
	/* The SHORT error should have precedence. Blink fast unconditionally */
	led_demo_err_led_blink(true);
}

static void LED_user_handler_open(struct bd18398_led *_this
				  __attribute__((unused)), void *opaque
				  __attribute__((unused)))
{
	led_demo_err_led_blink_if_free(false);
	/* printf("User OPEN handler for led %d\r\n", _this->id); */
}

/*
 * We set these as opaque ptr for LED SHORT error handler. LED driver
 * does not evaluate the opaque ptr but just passes it to hanler as given.
 * You can use the opaque ptr as a way to give IC specific data to handler
 */
static const char *text[] = {
	"LED1 - this could be information for handler",
	"LED2 - this could be information for handler",
	"LED3 - this could be information for handler",
};

/* Initialize all LED channels on EVK */
int evk_initialize_leds()
{
	int i;
	struct bd18398_led *led;
	int ret;
	int led_ids[BD18398_NUM_LEDS] = {
		BD18398_LED_1, BD18398_LED_2, BD18398_LED_3
	};

	memset(&g_leds, 0, sizeof(g_leds));

	for (i = 0; i < BD18398_NUM_LEDS; i++) {
		led = led_init(led_ids[i], BD18398_MAX_LED_CURRENT);
		g_leds[i].led_hw = led;
		if (!led) {
			printf("LED %d init failed\r\n", led_ids[i]);
			continue;
		}
		/*
		 * This is just an example of registering an error handler for
		 * LED errors. Similar registration functions are available
		 * for other but LED SHORT error.
		 */
		ret = register_lsd_handler(led, LED_user_handler_short,
					   (void *)text[i]);
		if (ret)
			printf
			    ("Could not register SHORT error handler for LED %d, error %d\r\n",
			     led->id, ret);

		ret = register_lod_handler(led, LED_user_handler_open, NULL);
		if (ret)
			printf
			    ("Could not register OPEN error handler for LED %d, error %d\r\n",
			     led->id, ret);

		ret = led->set_brightness(led, 0);
		if (ret)
			printf("LED %d: Failed to set brightness\n", led->id);
	}

	return 0;
}
