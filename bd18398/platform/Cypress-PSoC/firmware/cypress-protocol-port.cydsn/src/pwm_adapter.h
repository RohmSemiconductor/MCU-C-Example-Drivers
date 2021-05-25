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

#ifndef PWM_ADAPTER_H
#define PWM_ADAPTER_H

#include <stdint.h>
#include <stdbool.h>

#define EVKIT_PWM_DUTY_NULL                                 65535
#define EVKIT_PWM_FREQ_NULL                                 0
#define EVKIT_PWM_POL_NULL                                  0
#define EVKIT_PWM_POL_HIGH                                  1
#define EVKIT_PWM_POL_LOW                                   2

typedef struct {
	bool enable;
	uint8_t polarity;
	uint8_t pin;
	uint16_t freq_hz;
	uint16_t duty;
} EvkitPWMParams;

typedef uint8_t(*evkit_pwm_configure) (const EvkitPWMParams * params);

typedef struct {
	evkit_pwm_configure configure;
} evkit_pwm_if_t;

evkit_pwm_if_t *pwm_adp_init(void);
uint8_t pwm_adp_configure(const EvkitPWMParams *);

#endif
