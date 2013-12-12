/*-
 * Copyright (c) 2010, 2011 FUKAUMI Naoki.
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

#include <sys/stat.h>
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "rkcrc.h"

static char *progname;

static void
usage(void)
{

	fprintf(stderr, "usage: %s [-k|-p] infile outfile\n", progname);
	exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
	struct stat st;
	ssize_t nr;
	uint32_t crc;
	uint8_t buf[512];
	int ch, krnl, in, out, parm;

	progname = argv[0];

	krnl = parm = 0;
	while ((ch = getopt(argc, argv, "kp")) != -1) {
		switch (ch) {
		case 'k':
			krnl = 1;
			break;
		case 'p':
			parm = 1;
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 2 || (krnl && parm))
		usage();

	if ((in = open(argv[0], O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", argv[0]);

	if (fstat(in, &st) != 0)
		err(EXIT_FAILURE, "%s", argv[0]);

	if ((out = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
		err(EXIT_FAILURE, "%s", argv[1]);

	if (krnl || parm) {
		if (krnl) {
			buf[0] = 'K';
			buf[1] = 'R';
			buf[2] = 'N';
			buf[3] = 'L';
		} else {
			buf[0] = 'P';
			buf[1] = 'A';
			buf[2] = 'R';
			buf[3] = 'M';
		}
		buf[4] = (st.st_size >> 0) & 0xff;
		buf[5] = (st.st_size >> 8) & 0xff;
		buf[6] = (st.st_size >> 16) & 0xff;
		buf[7] = (st.st_size >> 24) & 0xff;

		write(out, buf, 8);
	}

	crc = 0;
	while ((nr = read(in, buf, sizeof(buf))) != -1 && nr != 0) {
		RKCRC(crc, buf, nr);
		write(out, buf, nr);
	}

	buf[0] = (crc >> 0) & 0xff;
	buf[1] = (crc >> 8) & 0xff;
	buf[2] = (crc >> 16) & 0xff;
	buf[3] = (crc >> 24) & 0xff;

	write(out, buf, 4);

	close(out);
	close(in);

	return EXIT_SUCCESS;
}