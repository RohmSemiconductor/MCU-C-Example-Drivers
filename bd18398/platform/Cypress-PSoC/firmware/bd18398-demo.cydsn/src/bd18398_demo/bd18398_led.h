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

/* This is the BD18398 driver example code. */

#ifndef __BD18398_LED_H
#define __BD18398_LED_H

#include <stdbool.h>
#include <stdint.h>

struct bd18398_led;

#define BD18398_ZERO_LED_INIT { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}

struct bd18398_led {
	bool fault;
	bool cached_enabled;
	bool initialized;
	uint8_t id;
	uint8_t on_mask;
	uint8_t dimm_mask;
	uint8_t iset_reg_h;
	uint8_t iset_reg_l;
	uint8_t bright_reg_h;
	uint8_t bright_reg_l;
	uint8_t status_reg;
	uint16_t cached_brightness;

	int (*on)(struct bd18398_led * _this);	///< Enable LED
	int (*off)(struct bd18398_led * _this);	///< Disable LED
	int (*dimm_on)(struct bd18398_led * _this);	///< Enable dimming
	int (*dimm_off)(struct bd18398_led * _this);	///< Disable dimming
	bool (*is_enabled)(struct bd18398_led * _this);	///< get LED state
	int (*set_max_current)(struct bd18398_led * _this, unsigned int mA);
	int (*set_brightness)(struct bd18398_led * _this, uint16_t brightness);	///< Set LED brightness
	 uint16_t(*get_brightness) (struct bd18398_led * _this);	///< Get LED brightness

	/* User registered error handlers and opaque ptrs */
	void (*handle_lsd)(struct bd18398_led * led, void *opaque);	///< short detection
	void *lsd_opaq;
	void (*handle_lod)(struct bd18398_led * led, void *opaque);	///< open detection
	void *lod_opaq;
	void (*handle_sw_ocp)(struct bd18398_led * led, void *opaque);	///< sw over-current
	void *sw_ocp_opaq;
	void (*handle_led_ocp)(struct bd18398_led * led, void *opaque);	///< led over-current
	void *led_ocp_opaq;
	bool (*is_faulty)(struct bd18398_led * _this);	///< has LED been set faulty

	/* TODO: Change these internal? */
	int (*set_faulty)(struct bd18398_led * _this);	///< set LED as faulty.
	int (*fault_cancel)(struct bd18398_led * _this);	///< Cancel LED fault state
};

enum bd18398_led_channels {
	BD18398_LED_1,
	BD18398_LED_2,
	BD18398_LED_3,
};

/* Please find the documentation from function implementation */
struct bd18398_led *led_init(enum bd18398_led_channels led_channel,
			     unsigned int max_mA);
void handle_led_errors(uint8_t errdet);
int register_lsd_handler(struct bd18398_led *_this,
			 void (*hnd)(struct bd18398_led * led, void *opaque),
			 void *opaque);
int register_lod_handler(struct bd18398_led *_this,
			 void (*hnd)(struct bd18398_led * led, void *opaque),
			 void *opaque);
int register_sw_ocp_handler(struct bd18398_led *_this,
			    void (*hnd)(struct bd18398_led * led, void *opaque),
			    void *opaque);
int register_led_ocp_handler(struct bd18398_led *_this,
			     void (*hnd)(struct bd18398_led * led,
					 void *opaque), void *opaque);

#endif
