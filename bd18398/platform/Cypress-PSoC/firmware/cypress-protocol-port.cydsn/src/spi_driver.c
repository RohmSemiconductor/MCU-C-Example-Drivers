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

#include "spi_driver.h"
#include "SPIM.h"
#include "SPIM_CLK.h"
#include <errno.h>
#include "logger.h"
#include "util.h"

#define SPIM_FREQUENCY_MIN_KHZ 1
#define SPIM_FREQUENCY_MAX_KHZ 18000
#define SPIM_FREQUENCY_CONV_TO_HZ 1000

static bool enabled;

#if SPIM_RX_SOFTWARE_BUF_ENABLED || SPIM_TX_SOFTWARE_BUF_ENABLED
#error "SPI software FIFO cannot be used with this SPI driver"
#endif

void spi_drv_enable(void)
{
	/* The A3 adapter board connects SPI and I2C signals together
	 * (MOSI + SDA, SCLK + SCL). SCLK/SCL must be driven low before MOSI/SDA
	 * to avoid generating an unintentional I2C start condition.
	 */
	CyPins_SetPinDriveMode(SCLK_PINPC, CY_PINS_DM_STRONG);
	CyPins_ClearPin(SCLK_PINPC);
	CyPins_SetPinDriveMode(MOSI_PINPC, CY_PINS_DM_STRONG);
	CyPins_ClearPin(MOSI_PINPC);
	CyPins_SetPinDriveMode(MISO_PINPC, CY_PINS_DM_DIG_HIZ);
	CyPins_SetPinDriveMode(SS_PINPC, CY_PINS_DM_STRONG);
	spi_drv_clear_ncs();
	SPIM_CLK_Start();
	SPIM_Start();
	enabled = true;
	LOGGER_INFOF("bus1(spi): SPI has been enabled.");
}

void spi_drv_disable(void)
{
	spi_drv_clear_ncs();
	SPIM_Stop();
	SPIM_CLK_Stop();
	CyPins_SetPinDriveMode(MOSI_PINPC, CY_PINS_DM_DIG_HIZ);
	CyPins_SetPinDriveMode(MISO_PINPC, CY_PINS_DM_DIG_HIZ);
	CyPins_SetPinDriveMode(SCLK_PINPC, CY_PINS_DM_DIG_HIZ);
	CyPins_SetPinDriveMode(SS_PINPC, CY_PINS_DM_STRONG);
	CyPins_SetPin(SS_PINPC);
	enabled = false;
	LOGGER_INFOF("bus1(spi): SPI has been disabled.");
}

/** SPI driver status */
bool spi_drv_is_enabled(void)
{
	return enabled;
}

uint8_t raw_spi_drv_write_sync(uint8_t * buf, uint16_t len)
{

	raw_spi_drv_write_begin(buf, len);
	raw_spi_drv_sync();

	SPIM_ClearRxBuffer();
	return 0;
}

void spi_drv_readwrite_hdx_raw(uint8_t * tx_buf, uint16_t tx_size,
			       uint8_t * rx_buf, uint16_t rx_size)
{
	uint16_t read_count = 0;
	uint16_t write_count = 0;
	uint16_t total_xact_len = tx_size + rx_size;

	while (read_count < total_xact_len) {
		while (write_count < total_xact_len
		       && read_count + SPIM_FIFO_SIZE > write_count) {
			if (write_count < tx_size) {
				SPIM_WriteTxData(tx_buf[write_count]);
			} else {
				SPIM_WriteTxData(0);
			}
			++write_count;
		}

		if (SPIM_ReadRxStatus() & SPIM_STS_RX_FIFO_NOT_EMPTY) {
			if (read_count >= tx_size) {
				rx_buf[read_count - tx_size] =
				    SPIM_ReadRxData();
			} else {
				SPIM_ReadRxData();
			}
			++read_count;
		}
	}
}

/* NOTE: The SPI hardware FIFOs must be empty before calling this function.
 *       We don't do a flush here, because that would be too expensive. */
uint8_t spi_drv_readwrite_hdx(uint8_t * tx_buf, uint16_t tx_size,
			      uint8_t * rx_buf, uint16_t rx_size)
{
	//spi_drv_init_miso_cs();
	spi_drv_set_ncs();
	spi_drv_readwrite_hdx_raw(tx_buf, tx_size, rx_buf, rx_size);
	spi_drv_clear_ncs();

	return 0;
}

uint8_t spi_drv_readwrite_fdx(uint8_t * tx_buf, uint16_t tx_size,
			      uint8_t * rx_buf, uint16_t rx_size)
{
	//spi_drv_init_miso_cs();
	spi_drv_set_ncs();

	uint8_t const *tx_end = tx_buf + tx_size;
	uint8_t const *rx_end = rx_buf + rx_size;
	uint16_t xact_size = tx_size > rx_size ? tx_size : rx_size;
	uint16_t rx_cnt = 0;
	uint16_t tx_cnt = 0;

	while (rx_cnt < xact_size) {
		while (tx_cnt < xact_size && rx_cnt + SPIM_FIFO_SIZE > tx_cnt) {
			if (tx_buf != tx_end) {
				SPIM_WriteTxData(*tx_buf);
				tx_buf++;
			} else {
				SPIM_WriteTxData(0);
			}
			tx_cnt++;
		}

		if (SPIM_ReadRxStatus() & SPIM_STS_RX_FIFO_NOT_EMPTY) {
			if (rx_buf != rx_end) {
				*rx_buf = SPIM_ReadRxData();
				rx_buf++;
			} else {
				SPIM_ReadRxData();
			}
			rx_cnt++;
		}
	}

	spi_drv_clear_ncs();
	return 0;
}

uint8_t spi_drv_write(uint8_t * buf, uint16_t len)
{
	int ret;

	LOGGER_TRACEF("bus1(spi): %s: len = %u.", __func__, (unsigned)len);

	//spi_drv_init_miso_cs();
	spi_drv_set_ncs();
	ret = raw_spi_drv_write_sync(buf, len);
	/*
	 * Note, this reversed order of CS debounce and RX buffer clearing.
	 * I don't think that matters though but it's worth noting just in
	 * case we see problems emerging 
	 */
	spi_drv_clear_ncs();

	return ret;
}

uint8_t spi_drv_set_sclk_freq(uint32_t freq_khz)
{

	if (freq_khz < SPIM_FREQUENCY_MIN_KHZ
	    || freq_khz > SPIM_FREQUENCY_MAX_KHZ) {
		return EOPNOTSUPP;
	}

	/* The divider is for the SPI component; SCLK's frequency is half of the
	   component's. Divider is rounded up nearest integer avoid excessive SPI clock speeds. */
	uint16_t div = (uint16_t) CEIL_DIV(BCLK__BUS_CLK__HZ,
					   (freq_khz *
					    SPIM_FREQUENCY_CONV_TO_HZ * 2));

	SPIM_CLK_SetDividerValue(div);
	return 0;
}
