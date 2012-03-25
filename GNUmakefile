# Simple Makefile for RK Flash Tool

CC	= $(CROSSPREFIX)gcc
LD	= $(CC)
CFLAGS	= -O2 -W -Wall
LDFLAGS	= -s
LIBS	= -lusb-1.0

ifdef LIBUSB
CFLAGS	+= -I$(LIBUSB)/include
LDFLAGS	+= -L$(LIBUSB)/lib
endif

MACH	= $(shell $(CC) -dumpmachine)
ifeq ($(findstring mingw,$(MACH)),mingw)
LDFLAGS	+= -static
USE_RES	= 1
endif
ifeq ($(findstring cygwin,$(MACH)),cygwin)
USE_RES	= 1
endif

ifeq ($(USE_RES),1)
RC	= $(CROSSPREFIX)windres
RCFLAGS	= -O coff -i
BINEXT	= .exe
RESFILE	= %.res
AWK	= awk
VERMAJ	= $(shell $(AWK) '/RKFLASHTOOL_VER_MAJOR/{print $$3}' rkflashtool.c)
VERMIN	= $(shell $(AWK) '/RKFLASHTOOL_VER_MINOR/{print $$3}' rkflashtool.c)
VERREV	= 0
LCOPYR	= 2011-2012 Ivo van Poorten
FDESCR	= Flashtool for RK2808, RK2818 and RK2918 based tablets
WWWURL	= http://sourceforge.net/projects/rkflashtool/
ifeq ($(findstring /sh,$(SHELL)),/sh)
DL	= '
endif
endif

PROGS	= $(patsubst %.c,%$(BINEXT), $(wildcard *.c))


all: $(PROGS)

%$(BINEXT): %.c $(RESFILE)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(PROGS) *.res *.rc

%.res: %.rc
	$(RC) $(RCFLAGS) $< -o $@

%.rc: GNUmakefile
	@echo $(DL)1 VERSIONINFO $(DL)>$@
	@echo $(DL) FILEVERSION $(VERMAJ),$(VERMIN),$(VERREV),0 $(DL)>>$@
	@echo $(DL) PRODUCTVERSION $(VERMAJ),$(VERMIN),$(VERREV),0 $(DL)>>$@
	@echo $(DL) FILEFLAGSMASK 0x3fL $(DL)>>$@
	@echo $(DL) FILEOS 0x40004L $(DL)>>$@
	@echo $(DL) FILEFLAGS 0x0L $(DL)>>$@
	@echo $(DL) FILETYPE 0x1L $(DL)>>$@
	@echo $(DL) FILESUBTYPE 0x0L $(DL)>>$@
	@echo $(DL)BEGIN $(DL)>>$@
	@echo $(DL)  BLOCK "StringFileInfo" $(DL)>>$@
	@echo $(DL)  BEGIN $(DL)>>$@
	@echo $(DL)    BLOCK "040904E4" $(DL)>>$@
	@echo $(DL)    BEGIN $(DL)>>$@
	@echo $(DL)      VALUE "LegalCopyright","\251 $(LCOPYR)\0" $(DL)>>$@
#	@echo $(DL)      VALUE "CompanyName","$(COMPANY)\0" $(DL)>>$@
	@echo $(DL)      VALUE "ProductName","$(notdir $(@:.rc=)).exe\0" $(DL)>>$@
	@echo $(DL)      VALUE "ProductVersion","$(VERMAJ).$(VERMIN).$(VERREV)\0" $(DL)>>$@
	@echo $(DL)      VALUE "License","Released under BSD license.\0" $(DL)>>$@
	@echo $(DL)      VALUE "FileDescription","$(FDESCR)\0" $(DL)>>$@
	@echo $(DL)      VALUE "FileVersion","$(VERMAJ).$(VERMIN).$(VERREV)\0" $(DL)>>$@
	@echo $(DL)      VALUE "InternalName","$(notdir $(@:.rc=))\0" $(DL)>>$@
	@echo $(DL)      VALUE "OriginalFilename","$(notdir $(@:.rc=)).exe\0" $(DL)>>$@
	@echo $(DL)      VALUE "WWW","$(WWWURL)\0" $(DL)>>$@
	@echo $(DL)    END $(DL)>>$@
	@echo $(DL)  END $(DL)>>$@
	@echo $(DL)  BLOCK "VarFileInfo" $(DL)>>$@
	@echo $(DL)  BEGIN $(DL)>>$@
	@echo $(DL)    VALUE "Translation", 0x409, 1252 $(DL)>>$@
	@echo $(DL)  END $(DL)>>$@
	@echo $(DL)END $(DL)>>$@

