/*
 * Library: libcrc
 * Git:     https://github.com/lammertb/libcrc
 * Author:  Lammert Bies
 *
 * This file is licensed under the MIT License as stated below
 *
 * Copyright (c) 1999-2016 Lammert Bies
 * Copyright (c) 2020 Thomas A. Early, N7TAE
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Description
 * -----------
 * The source file contains routines which calculate the CCITT CRC
 * values for an incomming byte string.
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "crc.h"

CCRC::CCRC()
{
	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++)
	{
		crc = 0;
		c   = i << 8;

		for (j=0; j<8; j++)
		{
			if ( (crc ^ c) & 0x8000 )
				crc = ( crc << 1 ) ^ CRC_POLY_16;
			else
				crc = crc << 1;

			c = c << 1;
		}

		crc_tab16[i] = crc;
	}
}

uint16_t CCRC::CalcCRC( const uint8_t *input_str, size_t num_bytes )
{
	uint16_t crc;
	const unsigned char *ptr;
	size_t a;

	crc = CRC_START_16;
	ptr = input_str;

	if ( ptr != NULL ) for (a=0; a<num_bytes; a++)
	{
		crc = (crc << 8) ^ crc_tab16[ ((crc >> 8) ^ (uint16_t) *ptr++) & 0x00FF ];
	}

	return crc;
}
