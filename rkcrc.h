/*-
 * Copyright (c) 2010,2014 FUKAUMI Naoki.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _RKCRC_H_
#define _RKCRC_H_

#include <stdint.h>

static inline uint16_t
rkcrc16(uint16_t crc, uint8_t *buf, uint64_t size)
{
	int i;

	while (size-- > 0) {
		crc ^= *buf++ << 8;
		for (i = 0; i < 8; i++) {
			if (crc & 0x8000)
				crc = (crc << 1) ^ 0x1021;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}


static inline uint32_t
rkcrc32(uint32_t crc, uint8_t *buf, uint64_t size)
{
	int i;

	while (size-- > 0) {
		crc ^= *buf++ << 24;
		for (i = 0; i < 8; i++) {
			if (crc & 0x80000000)
				crc = (crc << 1) ^ 0x04c10db7;
			else
				crc = (crc << 1);
		}
	}

	return crc;
}

#endif /* !_RKCRC_H_ */
