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

#include <errno.h>
#include <stddef.h>
#include <stdbool.h>

#include "PWM_0.h"
#include "PWM_1.h"
#include "PWM_2.h"
#include "REG_PWM_0.h"
#include "REG_PWM_1.h"
#include "REG_PWM_2.h"
#include "GPIO_P2_0.h"
#include "cypins.h"

#include "pwm_adapter.h"
#include "gpio_adapter.h"
#include "gpio_spec.h"
#include "util.h"
#include "logger.h"

#define PWM_CLK_FREQ 1000000
/* Certain min/max duty cycle configs can cause the compare value to be
 * period_counts+1. Because of this, the minimum frequency must be calculated
 * so that max_period_counts < 2^16-1 to avoid overflowing the compare
 * register.
 */
#define PWM_FREQ_MIN 16
#define PWM_FREQ_MAX UINT16_MAX

#define PWM_MAKE(mod_name, pin_nbr) { \
    .pin = pin_nbr, \
    .enabled = false, \
    .pol = EVKIT_PWM_POL_HIGH, \
    .duty = 32767, /* 50% */ \
    .init = mod_name ## _Init, \
    .enable = mod_name ## _Enable, \
    .stop = mod_name ## _Stop, \
    .set_compare_mode = mod_name ## _SetCompareMode, \
    .read_period = mod_name ## _ReadPeriod, \
    .write_period = mod_name ## _WritePeriod, \
    .write_compare = mod_name ## _WriteCompare, \
    .write_reset = REG_ ## mod_name ## _Write, \
}

struct pwm_mod {
	/* The PWM output pin that this module uses. */
	uint8_t pin;
	/* Enable state. */
	bool enabled;
	/* Polarity. Uses EVKIT_PWM_POL_* defines for values. */
	uint8_t pol;
	/* Duty cycle [0%,100%] scaled to [0,UINT16_MAX-1]. */
	uint16_t duty;

	/* Cypress module-specific PWM and register API functions. */
	void (*init)(void);
	void (*enable)(void);
	void (*stop)(void);
	void (*set_compare_mode)(uint8);
	 uint16(*read_period) (void);
	void (*write_period)(uint16);
	void (*write_compare)(uint16);
	void (*write_reset)(uint8);
};

static struct pwm_mod *pwm_get_by_pin(uint8_t);

static struct pwm_mod m_mods[] = {
	PWM_MAKE(PWM_0, 10),	// P2.3
	PWM_MAKE(PWM_1, 11),	// P2.4
	PWM_MAKE(PWM_2, 12),	// P2.5
};

static const size_t m_mods_count = UTIL_ARRAY_LEN(m_mods);

static evkit_pwm_if_t m_pwm_if = {
	.configure = pwm_adp_configure,
};

evkit_pwm_if_t *pwm_adp_init(void)
{
	for (size_t i = 0; i < m_mods_count; ++i) {
		struct pwm_mod *pwm = &m_mods[i];
		gpio_adp_set_octrl(pwm->pin, GPIO_ADP_OCTRL_SW);
		pwm->init();
		pwm->stop();
	}
	return &m_pwm_if;
}

uint8_t pwm_adp_configure(const EvkitPWMParams * params)
{
	struct pwm_mod *pwm = pwm_get_by_pin(params->pin);
	if (pwm == NULL) {
		LOGGER_ERRORF("pwm: invalid pin for PWM (%u)",
			      (unsigned)params->pin);
		return EINVAL;
	}

	if (!params->enable) {
		if (!pwm->enabled) {
			return 0;
		}
		pwm->stop();
		CyPins_SetPinDriveMode(g_gpio_pins[pwm->pin].cy_alias,
				       CY_PINS_DM_DIG_HIZ);
		gpio_adp_set_octrl(params->pin, GPIO_ADP_OCTRL_SW);
		pwm->enabled = false;
		g_gpio_pins[pwm->pin].reserved = false;
		return 0;
	}

	if (params->freq_hz != EVKIT_PWM_FREQ_NULL) {
		if (params->freq_hz < PWM_FREQ_MIN) {
			LOGGER_ERRORF("pwm: invalid frequency (%u Hz)",
				      (unsigned)params->freq_hz);
			return EINVAL;
		}
		/* -1 is to adjust for how the PWM hardware works. */
		pwm->write_period(DIV_UINT_ROUND(PWM_CLK_FREQ, params->freq_hz)
				  - 1);
	}

	if (params->duty != EVKIT_PWM_DUTY_NULL) {
		pwm->duty = params->duty;
	}
	/* The period register has period_counts-1 because of HW. The +1 here
	 * adjusts for that. */
	uint16_t period = pwm->read_period() + 1;
	/* The division removes the scaling that the duty value has. */
	uint16_t compare = DIV_UINT_ROUND((uint32_t) period *
					  (uint32_t) pwm->duty,
					  (uint32_t) (UINT16_MAX - 1));

	uint8_t pol = pwm->pol;
	if (params->polarity != EVKIT_PWM_POL_NULL) {
		pol = params->polarity;
	}
	/* The compare modes work so that the PWM output is high when the
	 * comparison between the PWM counter and compare register is true.
	 * (The counter counts down from period_reg to 0.)
	 *
	 * The PWM pulse is aligned to the left and compare modes are 
	 * chosen to provide both polarities for the PWM signal.
	 * Compare modes >= and < are used to switch signal polarity.
	 * Minimum and maximum duty cycles can be achieved.
	 * When compare is >=, 0% = period_reg+1 (comparison is always false)
	 *                     100% = 0 (comparison is always true)
	 * When compare is <, 0% = period_reg+1 (comparison is always true)
	 *                  , 100% = 0 (comparison is always false)
	 */
	compare = period - compare;
	switch (pol) {
	case EVKIT_PWM_POL_HIGH:
		pwm->set_compare_mode(PWM_0__B_PWM__GREATER_THAN_OR_EQUAL_TO);
		break;
	case EVKIT_PWM_POL_LOW:
		pwm->set_compare_mode(PWM_0__B_PWM__LESS_THAN);
		break;
	default:
		LOGGER_ERRORF("pwm: invalid polarity value (0x%02x)",
			      (unsigned)pol);
		return EINVAL;
	}
	pwm->pol = pol;
	pwm->write_compare(compare);

	if (!pwm->enabled) {
		if (g_gpio_pins[pwm->pin].reserved) {
			return EBUSY;
		}
		pwm->write_reset(1);
		gpio_adp_set_octrl(params->pin, GPIO_ADP_OCTRL_HW);
		CyPins_SetPinDriveMode(g_gpio_pins[pwm->pin].cy_alias,
				       CY_PINS_DM_STRONG);
		pwm->enable();
		pwm->write_reset(0);
		pwm->enabled = true;
		g_gpio_pins[pwm->pin].reserved = true;
	}

	return 0;
}

static struct pwm_mod *pwm_get_by_pin(uint8_t pin)
{
	for (size_t i = 0; i < m_mods_count; ++i) {
		struct pwm_mod *mod = &m_mods[i];
		if (mod->pin == pin) {
			return mod;
		}
	}
	return NULL;
}
