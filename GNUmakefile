# Simple Makefile for RK Flash Tool

CC	= gcc
CFLAGS	= -O2 -W -Wall
LDFLAGS	= -s
LIBS	= -lusb-1.0

ifeq "$(OS)" "Windows_NT"
BINEXT	= .exe
CFLAGS	+= -Ilibusb/inc
LDFLAGS	+= -static -Llibusb/lib
endif

PROGS	= $(patsubst %.c,%$(BINEXT), $(wildcard *.c))


all: $(PROGS)

%$(BINEXT): %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(PROGS)
