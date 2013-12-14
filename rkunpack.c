/*-
 * Copyright (c) 2010 FUKAUMI Naoki.
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	off_t size;
	uint32_t fsize, ioff, isize, noff;
	uint8_t *buf, *p;
	int fd, count, img;
	const char *name, *path, *sep;
	char dir[PATH_MAX];

	if (argc != 2) {
		fprintf(stderr, "usage: %s update.img\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1)
		err(EXIT_FAILURE, "%s", argv[1]);

	if ((size = lseek(fd, 0, SEEK_END)) == -1)
		err(EXIT_FAILURE, "%s", argv[1]);

	if ((buf = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0))
	    == MAP_FAILED)
		err(EXIT_FAILURE, "%s", argv[1]);

	if (memcmp(&buf[0x00], "RKAF", 4) != 0)
		errx(EXIT_FAILURE, "invalid signature");

	fsize = (buf[4] | buf[5] << 8 | buf[6] << 16 | buf[7] << 24) + 4;
	if (fsize != size)
		fprintf(stderr, "invalid file size (should be %u bytes)",
		    fsize);

	printf("manufacturer %s model %s\n", &buf[0x48], &buf[0x08]);

	count = buf[0x88] | buf[0x89] << 8 | buf[0x8a] << 16 | buf[0x8b] << 24;

	printf("%d files\n", count);
	printf("-------------------------------------------------------------------------------\n");

	for (p = &buf[0x8c]; count > 0; p += 0x70, count--) {
		name = (const char *)p;
		path = (const char *)p + 0x20;
		ioff = p[0x60] | p[0x61] << 8 | p[0x62] << 16 | p[0x63] << 24;
		noff = p[0x64] | p[0x65] << 8 | p[0x66] << 16 | p[0x67] << 24;
		isize = p[0x68] | p[0x69] << 8 | p[0x6a] << 16 | p[0x6b] << 24;
		fsize = p[0x6c] | p[0x6d] << 8 | p[0x6e] << 16 | p[0x6f] << 24;

		if (memcmp(path, "SELF", 4) == 0)
			printf("----------------- %s:%s(%s)", name, path,
			    argv[1]);
		else {
			printf("%08x-%08x %s:%s", ioff, ioff + isize - 1, name,
			    path);

			if (memcmp(name, "parameter", 9) == 0) {
				ioff += 8;
				fsize -= 12;
			}

			sep = path;
			while ((sep = strchr(sep, '/')) != NULL) {
				memcpy(dir, path, sep - path);
				dir[sep - path] = '\0';
				if (mkdir(dir, 0755) == -1 && errno != EEXIST)
					err(EXIT_FAILURE, "%s", dir);
				sep++;
			}

			if ((img = open(path, O_WRONLY | O_CREAT | O_TRUNC,
			    0644)) == -1 ||
			    write(img, &buf[ioff], fsize) == -1 ||
			    close(img) == -1)
				err(EXIT_FAILURE, "%s", path);
		}

		printf(" %d bytes", fsize);

		if (noff != 0xffffffffU)
			printf(" NAND offset 0x%x", noff);

		printf("\n");
	}

	printf("-------------------------------------------------------------------------------\n");
	printf("unpacked\n");

	munmap(buf, size);
	close(fd);

	return EXIT_SUCCESS;
}
