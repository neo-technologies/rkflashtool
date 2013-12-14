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
#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "rkcrc.h"
#include "version.h"

static const char strings[2][4] = { "KRNL", "PARM" };

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
        default:
usage:
            fprintf(stderr, "%s v%d.%d\nusage: %s [-k|-p] infile outfile\n",
                    progname, RKFLASHTOOL_VERSION_MAJOR,
                    RKFLASHTOOL_VERSION_MINOR, progname);
            exit(1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc != 2 || which < 0) goto usage;

    if ((in = open(argv[0], O_RDONLY)) == -1)
        err(1, "%s", argv[0]);

    if (fstat(in, &st) != 0)
        err(1, "%s", argv[0]);

    if ((out = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1)
        err(1, "%s", argv[1]);

    if (which >= 0) {
        memcpy(buf, strings[which], 4);
        PUT32LE(buf+4, st.st_size);
        write(out, buf, 8);
    }

    while ((nr = read(in, buf, sizeof(buf))) > 0) {
        RKCRC(crc, buf, nr);
        write(out, buf, nr);
    }

    PUT32LE(buf, crc);
    write(out, buf, 4);

    close(out);
    close(in);

    return 0;
}
