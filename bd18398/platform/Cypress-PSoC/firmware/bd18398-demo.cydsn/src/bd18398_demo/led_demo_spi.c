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
 * SPI wrapper layer to do BD18398 specific SPI handling like
 * CS toggling during read and CRC computation.
 *
 * The BD18398 does not support bulk operations.
 */

#include <stdint.h>
#include <errno.h>
#include <stdio.h>
#include "led_demo_spi.h"
#include "spi_driver.h"

/* Calculate CRC for data we send */
static uint8_t calc_crc8_msb(int poly, uint8_t initial, uint8_t * data,
			     unsigned len)
{
	unsigned int i, j;
	uint8_t crc = initial;

	for (i = 0; i < len; i++) {
		crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if (crc & 0x80)
				crc = (crc << 1) ^ poly;
			else
				crc <<= 1;
		}
	}
	return crc;
}

static int led_demo_spi_write_addr(uint8_t addr)
{
	uint8_t a[3];
	int ret;

	a[0] = addr;
	a[1] = 0xff;
	a[2] = calc_crc8_msb(0x131, 0, &a[0], 2);

	spi_drv_set_ncs();
	ret = raw_spi_drv_write_sync(&a[0], 3);
	spi_drv_clear_ncs();

	return ret;
}

static int led_demo_spi_write_data(uint8_t * data, int len)
{
	uint8_t crc;
	int ret;

	crc = calc_crc8_msb(0x131, 0, data, len);
	spi_drv_set_ncs();
	ret = raw_spi_drv_write_begin(data, len);
	if (ret)
		return ret;
	ret = raw_spi_drv_write_append(&crc, 1);
	raw_spi_drv_sync();

	SPIM_ClearRxBuffer();
	spi_drv_clear_ncs();

	return ret;
}

extern void led_demo_set_board_led_for_real(void);

/**
 * led_demo_spi_write to - write one byte to given register
 * @addr:	Register address to write to
 * @data:	Value to be written
 *
 * Write the value to a given register
 *
 * Return: Return 0 on success.
 */
int _led_demo_spi_write_to(uint8_t addr, uint8_t data)
{
	uint8_t b[2];

	b[0] = addr | 0x80;
	b[1] = data;

	return led_demo_spi_write_data(b, 2);
}

int led_demo_spi_write_to(uint8_t addr, uint8_t data, bool verify)
{
	int ret;

	ret = _led_demo_spi_write_to(addr, data);

	if (verify) {
		int tmpret;
		uint8_t tmp;

		tmpret = led_demo_spi_read_from(addr, &tmp);
		if (tmpret) {
			printf("Failed to read SPI for data verification%d\r\n",
			       tmpret);
		} else {
			if (tmp != data) {
				printf
				    ("SPI data write and read mismatch! 0x%x != 0x%x\r\n",
				     data, tmp);
				ret = -EINVAL;
			}
		}
	}
	return ret;
}

static int led_demo_spi_read(uint8_t * buf, uint16_t len)
{
	if (!len)
		return -EINVAL;

	spi_drv_readwrite_hdx_raw(NULL, 0, buf, len);
	raw_spi_drv_sync();
	spi_drv_clear_ncs();

	return 0;
}

/**
 * led_demo_spi_read_from - read one byte from given register
 * @addr:	Register address to read from
 * @data:	Pointer to location where read value is stored
 *
 * Read the value contained in a given register
 *
 * Return: Return 0 on success.
 */
int led_demo_spi_read_from(uint8_t addr, uint8_t * data)
{
	int ret;
	uint8_t b[3];

	ret = led_demo_spi_write_addr(addr);

	if (ret)
		return ret;

	/* How long do we need to keep CS cleared ? */
	CyDelayUs(100);
	spi_drv_set_ncs();
	ret = led_demo_spi_read(b, 3);

	*data = b[1];

	return ret;
}

/**
 * led_demo_spi_update_bits - update value of given bits
 * @addr:	Register address to read from
 * @mask:	Bits which are to be altered
 * @val:	New value for bits to be updated
 *
 * Update bits which are set high at mask to what is specified by val.
 * val bits that are not set in mask are ignored. NOTE: No locking to
 * protect the read-modify-write cycle is implemented. The write is only
 * performed if value needs to be changed. For volatile or special registers
 * you should use plain write instead.
 *
 * Return: Return 0 on success.
 */
int led_demo_spi_update_bits(uint8_t addr, uint8_t mask, uint8_t val)
{
	int ret;
	uint8_t new, cache;

	ret = led_demo_spi_read_from(addr, &cache);
	if (ret) {
		printf("SPI read FAILED %d\r\n", ret);
		return ret;
	}

	new = (cache & (~mask));
	new |= (val & mask);

	/*
	 * update bits is conceptually incompatible with volatile registers so
	 * we do not update value to IC if it is already the one we want to have
	 */
	if (new == cache)
		return 0;

	return led_demo_spi_write_to(addr, new, true);
}
