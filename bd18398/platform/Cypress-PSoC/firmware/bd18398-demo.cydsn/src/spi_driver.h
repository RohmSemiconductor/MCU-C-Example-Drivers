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
 * @brief Evaluation Kit protocol BUS1 SPI implementation.
 *
 * Supported bus parameters:
 * - target: EVKIT_BUS1_TARGET_SPI0
 * - identifier: 0
 */
#ifndef SPI_DRIVER_H
#define SPI_DRIVER_H

#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include "cypins.h"
#include "GPIO_P1_0_aliases.h"
#include "GPIO_P12_0_aliases.h"
#include "SPIM.h"

#define SS_PINPC GPIO_P1_0_0
#define MOSI_PINPC GPIO_P1_0_1
#define MISO_PINPC GPIO_P1_0_2
#define SCLK_PINPC GPIO_P12_0_2

/* Configure pin settings for CS and MISO.
 *
 * These pins can be used as GPIOs elsewhere after SPI enable, which means that
 * the pin settings can change to something that doesn't work with SPI. This
 * function can be used to reconfigure the pins to work with SPI again.
 */
static inline void spi_drv_init_miso_cs(void)
{
	CyPins_SetPinDriveMode(MISO_PINPC, CY_PINS_DM_DIG_HIZ);
	CyPins_SetPinDriveMode(SS_PINPC, CY_PINS_DM_STRONG);
	CyPins_SetPin(SS_PINPC);
}

/* Set the nCS pin (active-low chip-select). */
static inline void spi_drv_set_ncs(void)
{
	/* The Cypress API is used directly instead of going through GPIO adapter
	 * for less overhead. */
	CyPins_ClearPin(SS_PINPC);
}

/* Clear the nCS pin (active-low chip-select). */
static inline void spi_drv_clear_ncs(void)
{
	/* The Cypress API is used directly instead of going through GPIO adapter
	 * for less overhead. */
	CyPins_SetPin(SS_PINPC);
}

static inline uint8_t raw_spi_drv_write_append(uint8_t * buf, uint16_t len)
{

	for (uint16_t i = 0; i < len; ++i) {
		SPIM_WriteTxData(buf[i]);
	}

	return 0;
}

static inline uint8_t raw_spi_drv_write_begin(uint8_t * buf, uint16_t len)
{
	SPIM_ClearTxBuffer();

	return raw_spi_drv_write_append(buf, len);
}

static inline void raw_spi_drv_sync(void)
{
	while (!(SPIM_ReadTxStatus() & SPIM_STS_SPI_DONE)) ;
}

void spi_drv_readwrite_hdx_raw(uint8_t * tx_buf, uint16_t tx_size,
			       uint8_t * rx_buf, uint16_t rx_size);

/** Initialize the SPI module and enable its pins. */
extern void spi_drv_enable(void);

/** Stop the SPI module and disable its pins. */
extern void spi_drv_disable(void);

/** SPI driver status */
bool spi_drv_is_enabled(void);

uint8_t raw_spi_drv_write_sync(uint8_t * buf, uint16_t len);

/* Half-duplex SPI read-write.
 *
 * The TX data is written first and then the response is read.
 * See below for an illustration.
 * (W = TX data, R = RX data, X = don't care).
 *
 *     MOSI: W[0] W[1] W[n] 0    0    0    ...
 *     MISO: X    X    X    R[0] R[1] R[n] ...
 *
 * Returns 0, EVKIT_ERR_BUS1, or EVKIT_ERR_FEAT_SUPPORTED.
 */
uint8_t spi_drv_readwrite_hdx(uint8_t * tx_buf, uint16_t tx_size,
			      uint8_t * rx_buf, uint16_t rx_size);

/** Write data from a buffer to the SPI slave.
 *
 * @param params Bus parameters.
 * @param buf Buffer from which to write data.
 * @param len Amount of bytes to write to the slave.
 *
 * @returns 0, EVKIT_ERR_BUS1, or EVKIT_ERR_FEAT_SUPPORTED.
 */
extern uint8_t spi_drv_write(uint8_t * buf, uint16_t len);

/** Do a full-duplex read-write SPI operation.
 *
 * The contents of the write buffer are written to SPI and for each written
 * byte, a byte is read into the receive buffer.
 *
 * @param params Bus parameters.
 * @param[in] tx_buf Write buffer.
 * @param[out] rx_buf Receive buffer.
 * @param tx_size Amount of bytes to write.
 * @param rx_size Amount of bytes to read.
 *
 * @returns 0 or EOPNOTSUPP.
 */
extern uint8_t spi_drv_readwrite_fdx(uint8_t * tx_buf, uint16_t tx_size,
				     uint8_t * rx_buf, uint16_t rx_size);

/** Set the SCLK frequency.
 *
 * Supported frequencies are 1 kHz - 18 MHz.
 *
 * Note! Divider is calculated by the firmware using the 72 MHz master clock
 * and client requested SPI frequency. Divider is rounded up nearest integer.
 *
 * @returns 0 or EOPNOTSUPP.
 */
extern uint8_t spi_drv_set_sclk_freq(uint32_t freq_khz);

#endif				/* SPI_DRIVER_H */
