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

/**
 * @file
 * @brief GPIO-related tools and implementations for the Evaluation Kit
 *        protocol.
 */
#ifndef GPIO_ADAPTER_H
#define GPIO_ADAPTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

/* pin direction */
	enum {
		EVKIT_GPIO_PIN_INPUT,
		EVKIT_GPIO_PIN_OUTPUT,
	};

/* Selecting the input pin sense active high or low. */
	enum {
		EVKIT_GPIO_PIN_NOSENSE,
		EVKIT_GPIO_PIN_SENSE_LOW,
		EVKIT_GPIO_PIN_SENSE_HIGH,
	};

/* Input pin settings: Connect/Disconnect to input buffer. */
	enum {
		EVKIT_GPIO_PIN_DISCONNECTED,
		EVKIT_GPIO_PIN_CONNECTED,
	};

/* Input pin settings: Pin to be float or pulled Down/Up. */
	enum {
		EVKIT_GPIO_PIN_NOPULL,
		EVKIT_GPIO_PIN_PULLDOWN,
		EVKIT_GPIO_PIN_PULLUP,
	};

/* Output pin settings: Pin to be float or drive Low/High. */
	enum {
		EVKIT_GPIO_PIN_NODRIVE,
		EVKIT_GPIO_PIN_DRIVELOW,
		EVKIT_GPIO_PIN_DRIVEHIGH,
	};

/**@brief Interrupt pin configure params.
 *
 * @details Input pin configuration for interrupt request case => Evkit gpio structure
 */
	typedef
	    struct {
		uint8_t gpio_nbr;
		/* See: #defines prefixed with EVKIT_GPIO_PIN_ */
		uint8_t gpio_sense;
		uint8_t gpio_pin_pull;
	} EvkitGpioParams;

/**@brief Input/Output pin configuration.
 *
 * @details Structure for setting input/output pin configuration.
 */
	typedef
	    struct {
		uint8_t number;	/*!< Specifies the pin number. */
		uint8_t dir;	/*!< Specifies the pin direction */
		uint8_t input_connected;	/*!< Applied input pin only => Specifies the pin is connected/Disconnect to the input buffer high-z mode */
		uint8_t pin_position;	/*!< Different value depending the Intput/Output direction. */
	} EvkitGpioConfigParams;	/*!< Input values: nopull, pulldown, pullup; Output values: nodrive, drivelow, drivehigh */

	typedef uint8_t(*evkit_gpio_pin_read) (uint8_t gpio,
					       uint8_t * gpio_state);
	typedef uint8_t(*evkit_gpio_pin_write) (uint8_t gpio,
						uint8_t drivestate);
	typedef uint8_t(*evkit_gpio_pin_configure) (EvkitGpioConfigParams *
						    gpio_config);

	typedef struct {
		evkit_gpio_pin_read pin_read;
		evkit_gpio_pin_write pin_write;
		/* Optional. Set to NULL if unimplemented. */
		evkit_gpio_pin_configure pin_configure;
	} evkit_gpio_if_t;

	typedef enum {
		/* Control output with hardware. */
		GPIO_ADP_OCTRL_HW,
		/* Control output with software (via register bits). */
		GPIO_ADP_OCTRL_SW,
	} gpio_adp_octrl;

/** Initialize the GPIO adapter. */
	extern evkit_gpio_if_t *gpio_adp_init(void);

/** Read the state of a GPIO pin.
 *
 * The state is stored in `gpio_state` as either
 * `EVKIT_MSG_GPIO_PIN_SENSE_HIGH` or `EVKIT_MSG_GPIO_PIN_SENSE_LOW`.
 * (`EVKIT_MSG_GPIO_PIN_NOSENSE` is stored if there is an error.)
 *
 * @return An EVKIT status code.
 */
	extern uint8_t gpio_adp_read_state(uint8_t gpio_nbr,
					   uint8_t * gpio_state);

/** Set or clear a GPIO pin.
 *
 * @param gpio_nbr GPIO to manipulate.
 * @param drivestate Whether to drive high or low. Valid values are
 *     EVKIT_GPIO_PIN_DRIVEHIGH and EVKIT_GPIO_PIN_DRIVELOW.
 *
 * @return An EVKIT status code.
 */
	extern uint8_t gpio_adp_write_state(uint8_t gpio_nbr,
					    uint8_t drivestate);

/** Configure a GPIO pin.
 *
 * @param config GPIO pin settings.
 *
 * @return An EVKIT status code.
 */
	extern uint8_t gpio_adp_configure_pin(EvkitGpioConfigParams * config);

/* Set pin output control source. */
	uint8_t gpio_adp_set_octrl(uint8_t pin_nbr, gpio_adp_octrl ctrl);

#ifdef __cplusplus
}
#endif
#endif				/* GPIO_ADAPTER_H */
