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

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "spi_driver.h"

#include "bd18398_demo/bd18398.h"
#include "bd18398_demo/bd18398_faults.h"
#include "bd18398_demo/bd18398_led.h"
#include "bd18398_demo/led_demo_board_leds.h"
#include "bd18398_demo/led_demo_button.h"
#include "bd18398_demo/led_demo_led.h"
#include "bd18398_demo/led_demo_spi.h"
#include "bd18398_demo/led_demo_time.h"
#include "bd18398_demo/bd18398_evk.h"

/*
 * The demo code consists of states changed by a button push.
 *
 * Button push is detected as a Cypress GPIO IRQ. At each state we change
 * LED channel status / LED patterns.
 *
 * Implement main loop which checks the state, updates timing information
 * and calls the LED control functions.
 */
enum standalone_state {
	STANDALONE_STARTED,
	STANDALONE_PUSH1_CH2LIT,
	STANDALONE_PUSH2_CH3LIT,
	STANDALONE_PUSH3_RAMP_ALL,
	STANDALONE_PUSH4_RAMP_ONE,
	STANDALONE_PUSH5_LIMB,
	STANDALONE_STATE_LAST,
};

/* Configure EVK once at start-up */
int led_demo_set_config()
{
	int ret;

	/* LED IC related config */
	ret = bd18398_evk_config();
	if (ret)
		return ret;

	/* LED example object initialization */
	evk_initialize_leds();

	return 0;
}

/*
 * Section for running the demo State machine
 */

static void led_demo_next_state(int *state)
{
	*state = ((*state + 1) % STANDALONE_STATE_LAST);
	printf("button push detected - state %u\r\n", *state);
}

/* Functions for state specific actions. */

/* Initial state */
static void led_demo_set_initial_state(void)
{
	/*
	 *  Normal mode:
	 *      - LED #1 on board lights up (green)
	 *      - Go into LED active mode
	 *      - Set 1st channel to lit up LEDs
	 */
	printf("Set start state - ind-on, led1 FULL, led2 OFF. led3 OFF\r\n");

	led_demo_ind_led_on();

	evk_led_set_state(0, LED_FULL_ON, LED_TIMING_START);
	evk_led_set_state(1, LED_OFF, LED_TIMING_START);
	evk_led_set_state(2, LED_OFF, LED_TIMING_START);
}

/* State after first button press */
static void led_demo_set_state1(void)
{
	/* - Channel 2 lit-up */

	printf("Set state 2 - led1 FULL, led2 FULL. led3 OFF\r\n");
	evk_led_set_state(0, LED_FULL_ON, LED_TIMING_START);
	evk_led_set_state(1, LED_FULL_ON, LED_TIMING_START);
	evk_led_set_state(2, LED_OFF, LED_TIMING_START);
}

/* State after second button press */
static void led_demo_set_state2(void)
{
	/* Channel 3 lit-up */

	printf("Set state 3 - led1 FULL, led2 FULL. led3 FULL\r\n");

	evk_led_set_state(0, LED_FULL_ON, LED_TIMING_START);
	evk_led_set_state(1, LED_FULL_ON, LED_TIMING_START);
	evk_led_set_state(2, LED_FULL_ON, LED_TIMING_START);
}

/* State after third button press */
static void led_demo_set_state3(void)
{
	/* Ramp up/down all channels (w/ dimm) */

	printf("Set state 4 - CYCLE ALL same phase\r\n");
	evk_led_set_state(0, LED_SMOOTH_CYCLE, LED_TIMING_START);
	evk_led_set_state(1, LED_SMOOTH_CYCLE, LED_TIMING_START);
	evk_led_set_state(2, LED_SMOOTH_CYCLE, LED_TIMING_START);
}

/* State after 4.th button press */
static void led_demo_set_state4(void)
{
	/* Ramp up/down individual channels (w/ dimm) */

	printf("Set state 5 - CYCLE ALL different phase\r\n");
	evk_led_set_state(0, LED_SMOOTH_CYCLE, LED_TIMING_START);
	evk_led_set_state(1, LED_SMOOTH_CYCLE, LED_TIMING_QUARTER_PAST);
	evk_led_set_state(2, LED_SMOOTH_CYCLE, LED_TIMING_HALF);
}

/* State after 5.th button press */
static void led_demo_set_state5(void)
{
	/* Limb home */

	/* Here we just turn OFF the LEDs. The event loop does not
	 * poll status registers while we stay in this state so after
	 * 1 sec the IC should go to LIMB-HOME mode
	 */

	printf
	    ("Set state 6 - LIMB HOME - Set all channels OFF and stop polling\r\n");
	evk_led_set_state(0, LED_OFF, LED_TIMING_START);
	evk_led_set_state(1, LED_OFF, LED_TIMING_START);
	evk_led_set_state(2, LED_OFF, LED_TIMING_START);
	led_demo_err_led_off();
}

/* Function to cleat all cahced falts from LEDs */
bool clear_faults()
{
	if (!evk_any_led_faulty())
		return false;

	evk_fault_cancel_all_leds();
	return true;
}

extern void tiktak(void);
extern void led_demo_board_led_loop(void);

/* The MCU-only demo mode main loop. */
void led_demo_loop(void)
{
	static int state = -1;
	uint8_t status;

	/* Call time-keeping function to update our SW time counter */
	tiktak();

	/* Animate the Cypress board LEDs according to the state */
	led_demo_board_led_loop();

	/*
	 * Poll the status registers (and ping watchdog) if we are not in demo
	 * mode representing the limb-home mode. If we are in limb-home demo
	 * mode, then we skip polling which will eventually cause watchdog
	 * to trigger and transfer the IC to limb-home mode.
	 */
	if (state != STANDALONE_PUSH5_LIMB) {
		/*
		 * Update the LED channel outputs - blink, fade etc depending
		 * on demo mode.
		 */
		evk_led_loop();
		/*
		 * Poll IC status (and ping WDG). Handle errors if status != 0
		 * Please note, the error handling is not re-entrant or
		 * thread-safe
		 */
		if (is_failure_detected(&status)) {
			handle_errors(status);
		} else {
			led_demo_err_led_off();
			led_demo_ind_led_on();
		}
	} else {
		led_demo_ind_led_off();
	}

	if (led_demo_button_pushed()) {
		led_demo_next_state(&state);

		switch (state) {
		case STANDALONE_STARTED:
			led_demo_set_initial_state();
			break;
		case STANDALONE_PUSH1_CH2LIT:
			led_demo_set_state1();
			break;
		case STANDALONE_PUSH2_CH3LIT:
			led_demo_set_state2();
			break;
		case STANDALONE_PUSH3_RAMP_ALL:
			led_demo_set_state3();
			break;
		case STANDALONE_PUSH4_RAMP_ONE:
			led_demo_set_state4();
			break;
		case STANDALONE_PUSH5_LIMB:
			led_demo_set_state5();
			break;
		default:
			break;
		}
	} else if (state == -1) {
		led_demo_set_initial_state();
		state = STANDALONE_STARTED;
	}
}
