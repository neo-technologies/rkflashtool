/* rkflashtool - for RockChip based devices.
 *               (RK2808, RK2818, RK2918, RK2928, RK3066, RK3068 and RK3188)
 *
 * Copyright (C) 2010-2014 by Ivo van Poorten, Fukaumi Naoki, Guenter Knauf,
 *                            Ulrich Prinz, Steve Wilson
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY EXPRESS OR
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

/* hack to set binary mode for stdin / stdout on Windows */
#ifdef _WIN32
#include <fcntl.h>
int _CRT_fmode = _O_BINARY;
#endif

#include "version.h"

#define RKFT_BLOCKSIZE      0x4000      /* must be multiple of 512 */
#define RKFT_IDB_BLOCKSIZE  0x210
#define RKFT_IDB_INCR       0x20
#define RKFT_MEM_INCR       0x80
#define RKFT_OFF_INCR       (RKFT_BLOCKSIZE>>9)

#define RKFT_CMD_TESTUNITREADY      0x80000600
#define RKFT_CMD_READFLASHID        0x80000601
#define RKFT_CMD_READFLASHINFO      0x8000061a
#define RKFT_CMD_READCHIPINFO       0x8000061b
#define RKFT_CMD_READEFUSE          0x80000620

#define RKFT_CMD_SETDEVICEINFO      0x00000602
#define RKFT_CMD_ERASESYSTEMDISK    0x00000616
#define RKFT_CMD_SETRESETFLASG      0x0000061e
#define RKFT_CMD_RESETDEVICE        0x000006ff

#define RKFT_CMD_TESTBADBLOCK       0x80000a03
#define RKFT_CMD_READSECTOR         0x80000a04
#define RKFT_CMD_READLBA            0x80000a14
#define RKFT_CMD_READSDRAM          0x80000a17
#define RKFT_CMD_UNKNOWN1           0x80000a21

#define RKFT_CMD_WRITESECTOR        0x00000a05
#define RKFT_CMD_ERASESECTORS       0x00000a06
#define RKFT_CMD_UNKNOWN2           0x00000a0b
#define RKFT_CMD_WRITELBA           0x00000a15
#define RKFT_CMD_WRITESDRAM         0x00000a18
#define RKFT_CMD_EXECUTESDRAM       0x00000a19
#define RKFT_CMD_WRITEEFUSE         0x00000a1f
#define RKFT_CMD_UNKNOWN3           0x00000a22

#define RKFT_CMD_WRITESPARE         0x80001007
#define RKFT_CMD_READSPARE          0x80001008

#define RKFT_CMD_LOWERFORMAT        0x0000001c
#define RKFT_CMD_WRITENKB           0x00000030

#define SETBE16(a, v) do { \
                        ((uint8_t*)a)[1] =  v      & 0xff; \
                        ((uint8_t*)a)[0] = (v>>8 ) & 0xff; \
                      } while(0)

#define SETBE32(a, v) do { \
                        ((uint8_t*)a)[3] =  v      & 0xff; \
                        ((uint8_t*)a)[2] = (v>>8 ) & 0xff; \
                        ((uint8_t*)a)[1] = (v>>16) & 0xff; \
                        ((uint8_t*)a)[0] = (v>>24) & 0xff; \
                      } while(0)

typedef struct {
    uint16_t pid;
    char     name[8];
} t_pid;

const t_pid pidtab[] = {
    { 0x281a, "RK2818" },
    { 0x290a, "RK2918" },
    { 0x292a, "RK2928" },
    { 0x300a, "RK3066" },
    { 0x300b, "RK3168" },
    { 0x310b, "RK3188" },
    { 0, "" },
};

static uint8_t cmd[31], res[13], buf[RKFT_BLOCKSIZE];
static libusb_context *c;
static libusb_device_handle *h = NULL;
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
          "\trkflashtool p >file             \tfetch parameters\n"
          "\trkflashtool e offset size       \terase flash (fill with 0xff)\n"
          "\n"
          "\toffset and size are in units of 512 bytes\n");
}

static void send_cmd(uint32_t command, uint32_t offset, uint16_t nsectors) {
    long int r = random();

    memset(cmd, 0 , 31);
    memcpy(cmd, "USBC", 4);

    if (r)          SETBE32(cmd+4, r);
    if (offset)     SETBE32(cmd+17, offset);
    if (nsectors)   SETBE16(cmd+22, nsectors);
    if (command)    SETBE32(cmd+12, command);

    libusb_bulk_transfer(h, 2|LIBUSB_ENDPOINT_OUT, cmd, sizeof(cmd), &tmp, 0);
}

#define send_buf(s) libusb_bulk_transfer(h, 2|LIBUSB_ENDPOINT_OUT, \
                                             buf, s, &tmp, 0)

#define recv_res()  libusb_bulk_transfer(h, 1|LIBUSB_ENDPOINT_IN, \
                                           res, sizeof(res), &tmp, 0)

#define recv_buf(s) libusb_bulk_transfer(h, 1|LIBUSB_ENDPOINT_IN, \
                                             buf, s, &tmp, 0)

#define NEXT do { argc--;argv++; }while(0)

int main(int argc, char **argv) {
    const t_pid *ppid = &pidtab[0];
    int offset = 0, size = 0;
    char action;

    info("rkflashtool v%d.%d\n", RKFLASHTOOL_VERSION_MAJOR,
                                 RKFLASHTOOL_VERSION_MINOR);
    
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
    
    if (libusb_init(&c)) fatal("cannot init libusb\n");

    libusb_set_debug(c, 3);
    
    /* Detect connected RockChip device */
    
    while ( !h && ppid->pid) {
        h = libusb_open_device_with_vid_pid(c, 0x2207, ppid->pid);
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

    /* Initialize bootloader interface */

    send_cmd(RKFT_CMD_TESTUNITREADY, 0, 0);
    recv_res();
    usleep(20*1000);

    /* Check and execute command */

    switch(action) {
    case 'b':   /* Reboot device */
        info("rebooting device...\n");
        send_cmd(RKFT_CMD_RESETDEVICE, 0, 0);
        recv_res();
        break;
    case 'r':   /* Read FLASH */
        while (size > 0) {
            info("reading flash memory at offset 0x%08x\n", offset);

            send_cmd(RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            recv_buf(RKFT_BLOCKSIZE);
            recv_res();

            if ( write(1, buf, RKFT_BLOCKSIZE) <= 0)
                fatal("Write error! Disk full?\n");
            
            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        break;
    case 'w':   /* Write FLASH */
        while (size > 0) {
            info("writing flash memory at offset 0x%08x\n", offset);

            if (read(0, buf, RKFT_BLOCKSIZE) <= 0)
                fatal("premature end-of-file reached.\n");

            send_cmd(RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
            send_buf(RKFT_BLOCKSIZE);
            recv_res();

            offset += RKFT_OFF_INCR;
            size   -= RKFT_OFF_INCR;
        }
        break;
    case 'p':   /* Retreive parameters */
        {
            uint32_t *p = (uint32_t*)buf;

            info("reading parameters at offset 0x%08x\n", offset);

            send_cmd(RKFT_CMD_READLBA, offset, RKFT_OFF_INCR);
            recv_buf(RKFT_BLOCKSIZE);
            recv_res();
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
        while (size > 0) {
            int sizeRead = size > RKFT_MEM_INCR ? RKFT_MEM_INCR : size;
            info("reading memory at offset 0x%08x size %x\n", offset, sizeRead);

            send_cmd(RKFT_CMD_READSDRAM, offset-0x60000000, sizeRead);
            recv_buf(sizeRead);
            recv_res();

            if ( write(1, buf, sizeRead) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
            offset += sizeRead;
            size -= sizeRead;
        }
        break;
    case 'i':   /* Read IDB */
        while (size > 0) {
            int sizeRead = size > RKFT_IDB_INCR ? RKFT_IDB_INCR : size;
            info("reading IDB flash memory at offset 0x%08x\n", offset);
             
            send_cmd(RKFT_CMD_READSECTOR, offset, sizeRead);
            recv_buf(RKFT_IDB_BLOCKSIZE * sizeRead);
            recv_res();
             
            if ( write(1, buf, RKFT_IDB_BLOCKSIZE * sizeRead) <= 0) {
                fatal("Write error! Disk full?\n");
                size = 0;
            }
            offset += sizeRead;
            size -= sizeRead;
        }
        break;
    case 'e':   /* Erase flash */
        memset(buf, 0xff, RKFT_BLOCKSIZE);
        while (size > 0) {
                info("erasing flash memory at offset 0x%08x\n", offset);

                send_cmd(RKFT_CMD_WRITELBA, offset, RKFT_OFF_INCR);
                send_buf(RKFT_BLOCKSIZE);
                recv_res();

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
