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
 * This is the BD18398 driver example code.
 *
 * No warranty. No liability. Use at own risk. Intended to be used as a
 * reference only.
 *
 * Copyright, ROHM Semiconductors 2021
 *
 * Author: Matti Vaittinen <matti.vaittinen@fi.rohmeurope.com>
 */

#include <errno.h>
#include <stdio.h>
#include "bd18398.h"
#include "led_demo_spi.h"
#include "bd18398_led.h"

#define RETURN_IF_LED_NOT_SANE(__this) {                \
	if (!(__this) || (__this)->id > LED_MAX_INDEX) {    \
        printf("%s: Bad LED ID\r\n", __func__);         \
                                                        \
		return -EINVAL;                                 \
    }                                                   \
}
/* The BD18398 EVK has 3 LED channels */
static struct bd18398_led __evk_leds[3] = {
	BD18398_ZERO_LED_INIT,
	BD18398_ZERO_LED_INIT,
	BD18398_ZERO_LED_INIT
};

static int hnd_reg(struct bd18398_led *_this,
		   void (*hnd)(struct bd18398_led * led, void *opaque),
		   void *opaque,
		   void(**phnd)(struct bd18398_led * led, void *opaque),
		   void **popaque)
{
	if(!_this ||(!hnd && !*phnd))
		return -EINVAL;
	if (!_this->initialized)
		return -EAGAIN;
	if (hnd && *phnd)
		return -EBUSY;

	*phnd = hnd;
	*popaque = opaque;

	return 0;
}

/*
 * register_lsd_handler - Register error handler for LED SHORT error.
 *
 * @hnd		Error handler function
 * @opaque	pointer passed to handler as such
 */
int register_lsd_handler(struct bd18398_led *_this,
			 void (*hnd)(struct bd18398_led * led, void *opaque),
			 void *opaque)
{
	return hnd_reg(_this, hnd, opaque, &_this->handle_lsd,
		       &_this->lsd_opaq);
}

/*
 * register_lod_handler - Register error handler for LED OPEN error.
 *
 * @hnd		Error handler function
 * @opaque	pointer passed to handler as such
 */
int register_lod_handler(struct bd18398_led *_this,
			 void (*hnd)(struct bd18398_led * led, void *opaque),
			 void *opaque)
{
	return hnd_reg(_this, hnd, opaque, &_this->handle_lod,
		       &_this->lod_opaq);
}

/**
 * register_sw_ocp_handler - Register error handler for SW OCP error.
 *
 * @hnd		Error handler function
 * @opaque	pointer passed to handler as such
 */
int register_sw_ocp_handler(struct bd18398_led *_this,
			    void (*hnd)(struct bd18398_led * led, void *opaque),
			    void *opaque)
{
	return hnd_reg(_this, hnd, opaque, &_this->handle_sw_ocp,
		       &_this->sw_ocp_opaq);
}

/**
 * register_led_ocp_handler - Register error handler for LED OCP error.
 *
 * @hnd		Error handler function
 * @opaque	pointer passed to handler as such
 */
int register_led_ocp_handler(struct bd18398_led *_this,
			     void (*hnd)(struct bd18398_led * led,
					 void *opaque), void *opaque)
{
	return hnd_reg(_this, hnd, opaque, &_this->handle_led_ocp,
		       &_this->led_ocp_opaq);
}

static bool led_is_enabled(struct bd18398_led *_this)
{
	RETURN_IF_LED_NOT_SANE(_this);

	return _this->cached_enabled;
}

static inline int led_set(struct bd18398_led *_this, bool on)
{
	uint8_t regval;
	int ret;

	/* Sanity checks */
	RETURN_IF_LED_NOT_SANE(_this);

	if (_this->cached_enabled == on)
		return 0;

	if (on)
		regval = _this->on_mask;
	else
		regval = 0;

	ret = led_demo_spi_update_bits(BD18398_LED_ON_REG, _this->on_mask,
				       regval);

	if (!ret)
		_this->cached_enabled = on;

	return ret;
}

/*
 * I don't expect this to be called after startup so I don't cache
 * the state here. We always access the HW instead.
 */
static int led_dimm_set(struct bd18398_led *_this, bool enable)
{
	uint8_t regval;

	/* Sanity checks */
	RETURN_IF_LED_NOT_SANE(_this);

	if (enable)
		regval = _this->dimm_mask;
	else
		regval = 0;

	return led_demo_spi_update_bits(BD18398_LED_ON_REG, _this->dimm_mask,
					regval);
}

static int led_dimm_on(struct bd18398_led *_this)
{
	return led_dimm_set(_this, 1);
}

static int led_dimm_off(struct bd18398_led *_this)
{
	return led_dimm_set(_this, 0);
}

static int led_on(struct bd18398_led *_this)
{
	return led_set(_this, 1);
}

static int led_off(struct bd18398_led *_this)
{
	return led_set(_this, 0);
}

static int led_set_9bit_bright_regs(uint8_t reg_h, uint8_t reg_l,
				    uint16_t brightness, uint16_t cacheval)
{
	uint8_t val[2];
	int ret;

	if (cacheval == brightness)
		return 0;

	val[0] = brightness & 0x3;
	val[1] = (brightness >> 2);

	ret = led_demo_spi_write_to(reg_h, val[1], true);
	if (ret) {
		printf("Writing reg 0x%x FAILED!\r\n", reg_h);
		return ret;
	}

	ret = led_demo_spi_write_to(reg_l, val[0], true);
	if (ret) {
		printf
		    ("Writing low reg 0x%x FAILED. If WLOCK was set this may mess-up everything.\r\n",
		     reg_l);
		printf
		    ("We should propably just keep re-trying. Dor demo we just give-up (for now).\r\n");
		if (cacheval != 0xffff) {
			uint8_t tmp = (cacheval >> 2);
			/*
			 * If write failed, return also the high reg back to original
			 * and fail out.
			 */
			led_demo_spi_write_to(reg_h, tmp, true);
			return ret;
		}
	}

	return 0;

}

static int led_set_brightness(struct bd18398_led *_this, uint16_t brightness)
{
	int ret;

	/* Sanity checks */
	RETURN_IF_LED_NOT_SANE(_this);

	if (brightness > BRIGHTNESS_MAX) {
		printf("%s: Invalid brightness 0x%hx\r\n", __func__,
		       brightness);

		return -EINVAL;
	}

	/* Please note that you probably want to set the WLOCK prior this */
	ret = led_set_9bit_bright_regs(_this->bright_reg_h, _this->bright_reg_l,
				       brightness, _this->cached_brightness);
	if (!ret)
		_this->cached_brightness = brightness;

	return ret;
}

static uint16_t led_get_brightness(struct bd18398_led *_this)
{
	return _this->cached_brightness;
}

static int led_fault(struct bd18398_led *_this)
{
	/* Sanity checks */
	RETURN_IF_LED_NOT_SANE(_this);

	/*
	 * This is not the correct thing-to-do. We should probably disable
	 * failing LED but the demo env gives OPEN/SHORT errors all the time.
	 * Thus we leave the LED ON.
	 */
	return 0;

	if (_this->fault)
		return 0;

	_this->fault = true;

	return _this->off(_this);
}

static bool led_is_faulty(struct bd18398_led *_this)
{
	RETURN_IF_LED_NOT_SANE(_this);

	return _this->fault;
}

static int led_fault_cancel(struct bd18398_led *_this)
{
	RETURN_IF_LED_NOT_SANE(_this);
	if (!_this->fault)
		return 0;

	_this->fault = false;

	return 0;
}

static uint16_t compute_iset(unsigned int mA)
{
	unsigned int iset;

	/* we should get correct units, right? (mA * mOhm + uV)/uV */
	if (mA > 0xffffffff / 12 * BD18398_RSENSE + 200000) {
		printf("ISET computation Overflow\r\n");
		iset = BIT(10) - 1;
	} else if (mA < 0xffffffff / 12 * 1024 * BD18398_RSENSE + 200000 * 1024) {
		iset = (12 * mA * BD18398_RSENSE * 1024 + 200000 * 1024) /
		    BD18398_ADC_VREF;
	} else {
		iset = (12 * mA * BD18398_RSENSE + 200000) / BD18398_ADC_VREF *
		    1024;
	}
	if (iset > BIT(10) - 1) {
		printf("ISET computation Overflow (0x%x)\r\n", iset);
		iset = BIT(10) - 1;
	}
	printf("NOTE: Computed ISET=%x for Iled=%u mA\r\n", iset, mA);

	return (uint16_t) iset;
}

/*
 * NOTE: This example is not safe regarding overflows and rounding errors.
 * For example the 12 * BD18398_RSENSE + 200000 and
 * 12 * 1024 * BD18398_RSENSE + 200000 * 1024 can overflow depending on
 * BD18398_RSENSE.
 */
static int set_max_current(struct bd18398_led *_this, unsigned int mA)
{
	uint16_t iset;

	iset = compute_iset(mA);

	return led_set_9bit_bright_regs(_this->iset_reg_h, _this->iset_reg_l,
					iset, 0xffff);
}

static int get_initial_state(struct bd18398_led *_this)
{
	uint8_t val = 0;
	int ret;

	ret = led_demo_spi_read_from(BD18398_LED_ON_REG, &val);
	if (ret)
		printf("Failed to read initial LED status\r\n");

	_this->cached_enabled = !!(val & _this->on_mask);
	printf("LED %u initial status %s\r\n", _this->id,
	       _this->cached_enabled ? "ON" : "OFF");

	ret = led_demo_spi_read_from(_this->bright_reg_l, &val);
	if (ret) {
		printf("Failed to read initial LED brightness\r\n");
		return ret;
	}
	_this->cached_brightness = val & 0x3;

	ret = led_demo_spi_read_from(_this->bright_reg_h, &val);
	if (ret) {
		printf("Failed to read initial LED brightness\r\n");
		return ret;
	}
	_this->cached_brightness |= (((uint16_t) val) << 2);

	return 0;
}

static int _led_init(struct bd18398_led *_this)
{
	uint8_t led_on_masks[] = {
		BD18398_LED1_ON_MASK, BD18398_LED2_ON_MASK,
		BD18398_LED3_ON_MASK,
	};
	uint8_t led_dimm_masks[] = {
		BD18398_LED1_DIMM_MASK, BD18398_LED2_DIMM_MASK,
		BD18398_LED3_DIMM_MASK,
	};
	uint8_t led_iset_l[] = { BD18398_LED1_CURRENT_REG_L,
		BD18398_LED2_CURRENT_REG_L, BD18398_LED3_CURRENT_REG_L
	};
	uint8_t led_iset_h[] = { BD18398_LED1_CURRENT_REG_H,
		BD18398_LED2_CURRENT_REG_H, BD18398_LED3_CURRENT_REG_H
	};
	uint8_t led_bright_l[] = { BD18398_LED1_DPWM_REG_L,
		BD18398_LED2_DPWM_REG_L, BD18398_LED3_DPWM_REG_L
	};
	uint8_t led_bright_h[] = { BD18398_LED1_DPWM_REG_H,
		BD18398_LED2_DPWM_REG_H, BD18398_LED3_DPWM_REG_H
	};
	int status_regs[] = {
		BD18398_LED1_STATUS_REG, BD18398_LED2_STATUS_REG,
		BD18398_LED3_STATUS_REG,
	};

	_this->fault = false;
	_this->on_mask = led_on_masks[_this->id];
	_this->dimm_mask = led_dimm_masks[_this->id];
	_this->iset_reg_l = led_iset_l[_this->id];
	_this->iset_reg_h = led_iset_h[_this->id];
	_this->bright_reg_l = led_bright_l[_this->id];
	_this->bright_reg_h = led_bright_h[_this->id];
	_this->status_reg = status_regs[_this->id];

	_this->on = led_on;
	_this->off = led_off;
	_this->set_brightness = led_set_brightness;
	_this->get_brightness = led_get_brightness;
	_this->set_max_current = set_max_current;
	_this->is_faulty = led_is_faulty;
	_this->set_faulty = led_fault;
	_this->fault_cancel = led_fault_cancel;
	_this->is_enabled = led_is_enabled;
	_this->handle_led_ocp = _this->handle_sw_ocp = _this->handle_lod =
	    _this->handle_lsd = NULL;
	_this->dimm_on = led_dimm_on;
	_this->dimm_off = led_dimm_off;
	_this->initialized = true;

	return get_initial_state(_this);
}

/** led_init - Initialize LED for use
 *
 * @_this:		Pointer to led struct. Caller must've allocated this.
 * @led_channel:	Number of channel. @see bd18398_led_channels
 * @max_mA:		Maximum current for LED channel.
 *
 * This call initializes the LED for use. After successfull call the
 * LED functions can be used
 *
 * Returns 0 on success.
 */
struct bd18398_led *led_init(enum bd18398_led_channels led_channel,
			     unsigned int max_mA)
{
	struct bd18398_led *l;
	int ret;

	/* We have only 3 channels */
	if ((int)led_channel < 0 || led_channel > 2)
		return NULL;

	l = &__evk_leds[led_channel];

	l->id = led_channel;

	if (_led_init(l))
		return NULL;

	ret = l->set_max_current(l, max_mA);
	if (ret)
		printf("Failed to set MAX current to %u mA\r\n", max_mA);

	ret = l->dimm_on(l);
	if (ret)
		printf("Failed to enable LED %d dimming\r\n", l->id);

	return l;
}

/* Default LED error status handlers */
static void handle_led_ocp(struct bd18398_led *led)
{
	if (!led->initialized)
		return;
	limited_print2("LED %d OCP\r\n", led->id, 1000);
}

static void handle_sw_ocp(struct bd18398_led *led)
{
	if (!led->initialized)
		return;
	limited_print2("LED_SW %d OCP\r\n", led->id, 1000);
}

static void handle_lsd(struct bd18398_led *led)
{
	if (!led->initialized)
		return;
	limited_print2("LED %d SHORT\r\n", led->id, 1000);
}

static void handle_lod(struct bd18398_led *led)
{
	if (!led->initialized)
		return;
	limited_print2("LED %d OPEN\r\n", led->id, 1000);
}

static void led_err(struct bd18398_led *led)
{
	uint8_t ledstatus;
	int i, ret;
	/*
	   LSD - Led short
	   LOD - LED open
	   LOCP - Led over current
	   SWOCP - (SW - what is SW in this context, switch?) over current
	 */
	void (*led_err_hnd[])(struct bd18398_led * led) = {
		[BD18398_LED_ERR_BIT_LSD] = handle_lsd,
		[BD18398_LED_ERR_BIT_LOD] = handle_lod,
		[BD18398_LED_ERR_BIT_SWOCP] = handle_sw_ocp,
		[BD18398_LED_ERR_BIT_LOCP] = handle_led_ocp,
	};
	void (*user_err_hnd[])(struct bd18398_led * led, void *opaque) = {
		[BD18398_LED_ERR_BIT_LSD] = led->handle_lsd,
		[BD18398_LED_ERR_BIT_LOD] = led->handle_lod,
		[BD18398_LED_ERR_BIT_SWOCP] = led->handle_sw_ocp,
		[BD18398_LED_ERR_BIT_LOCP] = led->handle_led_ocp,
	};
	void *opaq[] = {
		[BD18398_LED_ERR_BIT_LSD] = led->lsd_opaq,
		[BD18398_LED_ERR_BIT_LOD] = led->lod_opaq,
		[BD18398_LED_ERR_BIT_SWOCP] = led->sw_ocp_opaq,
		[BD18398_LED_ERR_BIT_LOCP] = led->led_ocp_opaq,
	};

	ret = led_demo_spi_read_from(led->status_reg, &ledstatus);
	/* We assume all errors if we can't access IC */
	if (ret)
		ledstatus = 0xff;

	for (i = 0; i < (int)ARRAY_SIZE(led_err_hnd); i++) {
		if (ledstatus & (1 << i)) {
			if (user_err_hnd[i])
				user_err_hnd[i] (led, opaq[i]);
			else if (led_err_hnd[i])
				led_err_hnd[i] (led);
		}
	}
	/* Fault (and turn OFF) LED which indicates error */
	led->set_faulty(led);
/*	The demo env I have sets short/open error bits all the time.
 *	Can't turn off the LED for demo.
 *	led->off(led);
 */
}

/**
 * handle_led_errors - Handle LED errors
 *
 * @errdet:	errdet bits from status register
 *
 * Read specific status registers and call corresponding error handlers for
 * LED errors adverticed by the errdet bits of the main status register.
 * Will call user-registered error handlers (see register_*_handler()
 * - functions) or the default handlers if user has not setup any specific
 * hanlders. As writing of this the @handle_errors() calls this internally
 * so user should not need to explicitly call this. See also
 * @is_failure_detected for obtaining the main status bits.
 */
void handle_led_errors(uint8_t errdet)
{
	int i;

	for (i = 0; i < 3; i++)
		if (errdet & (1 << i)) {
			if (__evk_leds[i].initialized)
				led_err(&__evk_leds[i]);
			else
				limited_print2
				    ("LED error channel %d but LED not yet initialized\r\n",
				     i, 1000);
		}
}
