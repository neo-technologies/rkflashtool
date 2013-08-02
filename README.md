README
======

This modified repository hosts tools for Arnova tablet development from

https://github.com/justgr/arnova-tools

to support rk3066 Android dongles.

Use install-debian.sh to install dependecies and type make to build it

RK3188 support thanks to Omegamoon.

rkflashtool
===========

Originally from http://forum.xda-developers.com/showthread.php?t=1286305

If a new version appears, please create a fork of this repo, add changes, 
and request a pull. The changes will be merged into this repository. If 
the original author also sets up a repository, please let me know.

COMMENT
=======

Modified by Astralix to add some more informative console output

Modified by Astralix to add parameters readout command 'p'

Added in Omegamoon's RK3188 parameters.

Merged in the "erase" function from Galland's rkflashtool_rk3066 (thesawolf)

USAGE
=====

Read parameters:
- sudo ./rkflashtool p > parm.bin
- cat parm.bin
- The output will show 2 hex numbers and the partition (,0x00008000@0x00010000(recovery),)
- When using the rkflashtool, swap the 2 hex numbers found in the parm.bin for the respective partition
- ie. Writing to recovery would be:
sudo ./rkflashtool w 0x10000 0x8000 < recovery.img

**command usage:**
- rkflashtool b				reboot device
- rkflashtool r offset size >file	read flash
- rkflashtool w offset size <file	write flash

- rkflashtool m offset size >file	read 0x80 bytes DRAM
- rkflashtool i offset blocks >file	read IDB flash
- rkflashtool p >file			fetch parameters

- rkflashtool e offset size		erase flash (fill with 0xff)

offset and size are in units (blocks) of 512 bytes
