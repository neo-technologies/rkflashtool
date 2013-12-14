/*
 * Copyright (c) 2010 Fukaumi Naoki
 * Copyright (c) 2013 Ivo van Poorten
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
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "version.h"

static uint8_t *buf;
static off_t size;
static unsigned int fsize, ioff, isize, noff;
static int fd;

static const char *const strings[2] = { "info", "fatal" };

static void info_and_fatal(const int s, const char *f, ...) {
    va_list ap;
    va_start(ap,f);
    fprintf(stderr, "rkunpack: %s: ", strings[s]);
    vfprintf(stderr, f, ap);
    va_end(ap);
    if (s) exit(s);
}

#define info(...)   info_and_fatal(0, __VA_ARGS__)
#define fatal(...)  info_and_fatal(1, __VA_ARGS__)

#define GET32LE(x) ((x)[0] | (x)[1] << 8 | (x)[2] << 16 | (x)[3] << 24)

static void write_file(const char *path, uint8_t *buffer, unsigned int length) {
    int img;
    if ((img = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1 ||
               write(img, buffer, length) == -1 ||
               close(img) == -1)
        fatal("%s: %s\n", path, strerror(errno));
}

static void unpack_rkaf(void) {
    uint8_t *p;
    const char *name, *path, *sep;
    char dir[PATH_MAX];
    int count;

    info("RKAF signature detected\n");

    fsize = GET32LE(buf+4) + 4;
    if (fsize != (unsigned)size)
        info("invalid file size (should be %u bytes)\n", fsize);
    else
        info("file size matches (%u bytes)\n", fsize);

    info("manufacturer: %s\n", buf + 0x48);
    info("model: %s\n", buf + 0x08);

    count = GET32LE(buf+0x88);

    info("number of files: %d\n", count);

    for (p = &buf[0x8c]; count > 0; p += 0x70, count--) {
        name = (const char *)p;
        path = (const char *)p + 0x20;

        ioff  = GET32LE(p+0x60);
        noff  = GET32LE(p+0x64);
        isize = GET32LE(p+0x68);
        fsize = GET32LE(p+0x6c);

        if (memcmp(path, "SELF", 4) == 0) {
            info("skipping SELF entry\n");
        } else {
            info("%08x-%08x %-24s (fsize: %d)\n", ioff, ioff + isize - 1, path, fsize);

            // strip header and footer of parameter file
            if (memcmp(name, "parameter", 9) == 0) {
                ioff += 8;
                fsize -= 12;
            }

            sep = path;
            while ((sep = strchr(sep, '/')) != NULL) {
                memcpy(dir, path, sep - path);
                dir[sep - path] = '\0';
                if (mkdir(dir, 0755) == -1 && errno != EEXIST)
                    fatal("%s: %s\n", dir, strerror(errno));
                sep++;
            }

            write_file(path, buf+ioff, fsize);
        }
    }
}

static void unpack_rkfw(void) {
    info("RKFW signature detected\n");
    info("version: %d.%d.%d\n", buf[8], buf[7], buf[6]);

    ioff  = GET32LE(buf+0x19);
    isize = GET32LE(buf+0x1d);

    if (memcmp(buf+ioff, "BOOT", 4))
        fatal("cannot find BOOT signature\n");
}

int main(int argc, char *argv[]) {

    if (argc != 2)
        fatal("rkunpack v%d.%d\nusage: %s update.img\n",
               RKFLASHTOOL_VERSION_MAJOR,
               RKFLASHTOOL_VERSION_MINOR, argv[0]);

    if ((fd = open(argv[1], O_RDONLY)) == -1)
        fatal("%s: %s\n", argv[1], strerror(errno));

    if ((size = lseek(fd, 0, SEEK_END)) == -1)
        fatal("%s: %s\n", argv[1], strerror(errno));

    if ((buf = mmap(NULL, size, PROT_READ, MAP_SHARED | MAP_FILE, fd, 0))
                                                        == MAP_FAILED)
        fatal("%s: %s\n", argv[1], strerror(errno));

         if (!memcmp(buf, "RKAF", 4)) unpack_rkaf();
    else if (!memcmp(buf, "RKFW", 4)) unpack_rkfw();
    else fatal("%s: invalid signature\n", argv[1]);

    printf("unpacked\n");

    munmap(buf, size);
    close(fd);

    return 0;
}
