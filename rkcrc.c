/*-
 * Copyright (c) 2010, 2011 Fukaumi Naoki
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

#include <sys/stat.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "rkcrc.h"
#include "rkflashtool.h"
#include "version.h"

#ifndef _WIN32
#define O_BINARY 0
#endif

static const char headers[2][4] = { "KRNL", "PARM" };

static const char *const strings[2] = { "info", "fatal" };

static void info_and_fatal(const int s, char *f, ...) {
    va_list ap;
    va_start(ap,f);
    fprintf(stderr, "rkcrc: %s: ", strings[s]);
    vfprintf(stderr, f, ap);
    va_end(ap);
    if (s) exit(s);
}

#define info(...)   info_and_fatal(0, __VA_ARGS__)
#define fatal(...)  info_and_fatal(1, __VA_ARGS__)

int main(int argc, char *argv[]) {
    struct stat st;
    ssize_t nr;
    uint32_t crc = 0;
    uint8_t buf[512];
    char *progname = argv[0];
    int ch, which = -1, in, out;

    while ((ch = getopt(argc, argv, "kp")) != -1) {
        switch (ch) {
        case 'k': which = 0; break;
        case 'p': which = 1; break;
        default: break;
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2)
        fatal("rkcrc v%d.%d\nusage: %s [-k|-p] infile outfile\n",
                    RKFLASHTOOL_VERSION_MAJOR,
                    RKFLASHTOOL_VERSION_MINOR, progname);


    if ((in = open(argv[0], O_BINARY | O_RDONLY)) == -1)
        fatal("%s: %s\n", argv[0], strerror(errno));

    if (fstat(in, &st) != 0)
        fatal("%s: %s\n", argv[0], strerror(errno));

    if ((out = open(argv[1], O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
        fatal("%s: %s\n", argv[1], strerror(errno));

    if (which >= 0) {
        memcpy(buf, headers[which], 4);
        PUT32LE(buf+4, st.st_size);
        if(write(out, buf, 8) != 8)
          fatal("%s: write error\n", argv[1]);
    }

    while ((nr = read(in, buf, sizeof(buf))) > 0) {
        crc = rkcrc32(crc, buf, nr);
        if(write(out, buf, nr) != nr)
          fatal("%s: write error\n", argv[1]);
    }

    PUT32LE(buf, crc);
    if(write(out, buf, 4) != 4)
      fatal("%s: write error\n", argv[1]);

    close(out);
    close(in);

    return 0;
}
