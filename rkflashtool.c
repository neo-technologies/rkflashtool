/* rkflashtool - for RK2808, RK2818, RK2918 and RK3066, RK3188 based devices
 *
 * Copyright (C) 2012 Astralix          (cleanup, addons, new CPUs)
 * Copyright (C) 2011 Ivo van Poorten   (complete rewrite for libusb)
 * Copyright (C) 2010 FUKAUMI Naoki     (reverse engineering of protocol)
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
 *
 * Build with:
 *
 *      gcc -o rkflashtool rkflashtool.c -lusb-1.0 -O2 -W -Wall -s
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

#define RKFLASHTOOL_VERSION_MAJOR      3
#define RKFLASHTOOL_VERSION_MINOR      3

#define VID_RK              0x2207
#define PID_RK2818          0x281a
#define PID_RK2918          0x290a
#define PID_RK3066          0x300a
#define PID_RK3168          0x300b
#define PID_RK3188          0x310b

#define RKFT_BLOCKSIZE      0x4000                  /* must be multiple of 512 */
#define RKFT_IDB_BLOCKSIZE  0x210
#define RKFT_IDB_INCR       0x20
#define RKFT_MEM_INCR       0x80
#define RKFT_OFF_INCR       (RKFT_BLOCKSIZE>>9)

#ifndef RKFT_DISPLAY
#define RKFT_DISPLAY	    0x100
#endif

#define RKFT_FILLBYTE 	    0xff

#define RKFT_CID            4
#define RKFT_FLAG           12
#define RKFT_COMMAND        13
#define RKFT_OFFSET         17
#define RKFT_SIZE           23

#define SETBE32(a, v) ((uint8_t*)a)[3] =  v      & 0xff; \
                      ((uint8_t*)a)[2] = (v>>8 ) & 0xff; \
                      ((uint8_t*)a)[1] = (v>>16) & 0xff; \
                      ((uint8_t*)a)[0] = (v>>24) & 0xff

typedef struct {
    uint16_t pid;
    char     name[8];
} t_pid;

const t_pid pidtab[] = {
    { PID_RK2818, "RK2818" },
    { PID_RK2918, "RK2918" },
    { PID_RK3066, "RK3066" },
    { PID_RK3168, "RK3168" },
    { PID_RK3188, "RK3188" },
    { 0, "" },
};


static uint8_t cmd[31] = { 'U', 'S', 'B', 'C', };
static uint8_t res[13];

static uint8_t buf[RKFT_BLOCKSIZE], cid;
static int tmp;

static const char *const strings[2] = { "info", "fatal" };

static void info_and_fatal(const int s, char *f, ...) {
    va_list ap;
    va_start(ap,f);
    fprintf(stderr, "rkflashtool: %s: ", strings[s]);
    vfprintf(stderr, f, ap);
    va_end(ap);
    if (s) exit(s);
}

#define info(...)   info_and_fatal(0, __VA_ARGS__)
#define fatal(...)  info_and_fatal(1, __VA_ARGS__)

static void usage(void) {
    fatal("usage:\n"
          "\trkflashtool b                   \treboot device\n"
          "\trkflashtool m offset size >file \tread 0x80 bytes DRAM\n"
          "\trkflashtool i offset blocks >file \tread IDB flash\n"
          "\trkflashtool r offset size >file \tread flash\n"
          "\trkflashtool w offset size <file \twrite flash\n"
          "\trkflashtool p >file             \tfetch parameters\n\n"
          "\toffset and size are in units of 512 bytes\n");
}

static void send_cmd(libusb_device_handle *h, int e, uint8_t flag,
                     uint32_t command, uint32_t offset, uint8_t size) {
    cmd[RKFT_CID ] = cid++;
    cmd[RKFT_FLAG] = flag;
    cmd[RKFT_SIZE] = size;

    SETBE32(&cmd[RKFT_COMMAND], command);
    SETBE32(&cmd[RKFT_OFFSET ], offset );

    libusb_bulk_transfer(h, e|LIBUSB_ENDPOINT_OUT, cmd, sizeof(cmd), &tmp, 0);
}

#define send_buf(h,e,s) libusb_bulk_transfer(h, e|LIBUSB_ENDPOINT_OUT, \
                                             buf, s, &tmp, 0)

#define recv_res(h,e) libusb_bulk_transfer(h, e|LIBUSB_ENDPOINT_IN, \
                                           res, sizeof(res), &tmp, 0)

#define recv_buf(h,e,s) libusb_bulk_transfer(h, e|LIBUSB_ENDPOINT_IN, \
                                             buf, s, &tmp, 0)

#define NEXT do { argc--;argv++; }while(0)

int main(int argc, char **argv) {
    const t_pid *ppid = &pidtab[0];
    libusb_context *c;
    libusb_device_handle *h = NULL;
    int offset = 0, size = 0;
    char action;

    info("rkflashtool v%d.%d\n", RKFLASHTOOL_VERSION_MAJOR, RKFLASHTOOL_VERSION_MINOR);
    
    NEXT; if (!argc) usage();

    action = **argv; NEXT;

    switch(action) {
    case 'b':
        if (argc) usage(); 
        break;
    case 'e':
    case 'r': 
    case 'w': 
    case 'm':
    case 'i':
        if (argc != 2) usage();
        offset = strtoul(argv[0], NULL, 0);
        size   = strtoul(argv[1], NULL, 0);
        break;
    case 'p':
        if (argc) usage();
        offset = 0;
        size = 1024;
        break; 
    default:
        usage();
    }

    /* Initialize libusb */
    
    if (libusb_init(&c)) 
        fatal("cannot init libusb\n");

    libusb_set_debug(c, 3);
    
    /* Detect connected RockChip device */
    
    while ( !h && ppid->pid) {
        h = libusb_open_device_with_vid_pid(c, VID_RK, ppid->pid);
        if (h) {
            info("Detected %s...\n", ppid->name);
            break;
        }
        ppid++;
    } 
    if (!h) fatal("cannot open device\n");

    /* Connect to device */
    
    if (libusb_kernel_driver_active(h, 0) == 1) {
        info("kernel driver active\n");
        if (!libusb_detach_kernel_driver(h, 0))
            info("driver detached\n");
    }

    if (libusb_claim_interface(h, 0) < 0)  
        fatal("cannot claim interface\n");
        
    info("interface claimed\n");
    send_cmd(h, 2, 0x80, 0x00060000, 0x00000000, 0x00);        /* Initialize bootloader interface */
    recv_res(h, 1);
    usleep(20*1000);

    /* Check and prepare command */

    switch(action) {
    case 'b':   /* Reboot device */
        info("rebooting device...\n");
        send_cmd(h, 2, 0x00, 0x0006ff00, 0x00000000, 0x00);
        recv_res(h, 1);
        break;
    case 'r':   /* Read FLASH */
        while (size > 0) 
        {
            info("reading flash memory at offset 0x%08x\n", offset);

            send_cmd(h, 2, 0x80, 0x000a1400, offset, RKFT_OFF_INCR);
            recv_buf(h, 1, RKFT_BLOCKSIZE);
            recv_res(h, 1);

            if ( write(1, buf, RKFT_BLOCKSIZE) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
            
            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        break;
    case 'w':   /* Write FLASH */
        while (size > 0) 
        {
            info("writing flash memory at offset 0x%08x\n", offset);

            memset(buf, 0, RKFT_BLOCKSIZE);
            if (read(0, buf, RKFT_BLOCKSIZE) <= 0) {
                info("EOF reached.\n");
                size = 0;
            }

            send_cmd(h, 2, 0x80, 0x000a1500, offset, RKFT_OFF_INCR);
            send_buf(h, 2, RKFT_BLOCKSIZE);
            recv_res(h, 1);

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        break;
    case 'p':   /* Retreive parameters */
        {
            uint32_t *p = (uint32_t*)buf;

            info("reading parameters at offset 0x%08x\n", offset);

            send_cmd(h, 2, 0x80, 0x000a1400, offset, RKFT_OFF_INCR);
            recv_buf(h, 1, RKFT_BLOCKSIZE);
            recv_res(h, 1);
            info("rkcrc: 0x%08x\n", *p++);
            size = *p;;
            info("size:  0x%08x\n", size);

            if ( write(1, &buf[8], size) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
        }
        break;
    case 'm':   /* Read RAM */
        while (size > 0) 
        {
            int sizeRead = size > RKFT_MEM_INCR ? RKFT_MEM_INCR : size;
            info("reading memory at offset 0x%08x size %x\n", offset, sizeRead);

            send_cmd(h, 2, 0x80, 0x000a1700, offset-0x60000000, sizeRead);
            recv_buf(h, 1, sizeRead);
            recv_res(h, 1);

            if ( write(1, buf, sizeRead) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
            offset += sizeRead;
            size -= sizeRead;
        }
        break;
    case 'i':   /* Read IDB */
        while (size > 0) 
        {
            int sizeRead = size > RKFT_IDB_INCR ? RKFT_IDB_INCR : size;
            info("reading IDB flash memory at offset 0x%08x\n", offset);
             
            send_cmd(h, 2, 0x80, 0x000a0400, offset, sizeRead);
            recv_buf(h, 1, RKFT_IDB_BLOCKSIZE * sizeRead);
            recv_res(h, 1);
             
            if ( write(1, buf, RKFT_IDB_BLOCKSIZE * sizeRead) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
            offset += sizeRead;
            size -= sizeRead;
        }
        break;
    case 'e':   /* Erase flash */
	memset(buf, RKFT_FILLBYTE, RKFT_BLOCKSIZE);
	while (size>0) {
		if (offset % RKFT_DISPLAY == 0)
			info("erasing flash memory at offset 0x%08x\r", offset);

		send_cmd(h, 2, 0x80, 0x000a1500, offset, RKFT_OFF_INCR);
		send_buf(h, 2, RKFT_BLOCKSIZE);
		recv_res(h, 1);

		offset += RKFT_OFF_INCR;
		size   -= RKFT_OFF_INCR;
	}
	fprintf(stderr, "\n");
	break;
    default:
        break;
    }

    /* Disconnect and close all interfaces */

    libusb_release_interface(h, 0);
    libusb_close(h);
    libusb_exit(c);
    return 0;
}
