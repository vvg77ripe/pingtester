@echo off

bmp2raw images/config.bmp config.raw
bmp2raw images/netcard.bmp netcard.raw
bmp2raw images/sdcard.bmp sdcard.raw
bmp2raw images/ping.bmp ping.raw
bmp2raw images/ac_0.bmp ac_0.raw
bmp2raw images/ac_25.bmp ac_25.raw
bmp2raw images/ac_50.bmp ac_50.raw
bmp2raw images/ac_75.bmp ac_75.raw
bmp2raw images/ac_100.bmp ac_100.raw

mkromfs
arm-elf-objcopy -I binary -O elf32-littlearm -B arm --redefine-sym _binary_romfs_bin_start=__ROMFS_START --rename-section .data=.rodata romfs.bin romfs.o
