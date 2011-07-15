# This is a cleaned up version of the "ADAM2 App" available from http://www.seattlewireless.net/ADAM2.
# It's just a simple MIPS program which will print a hello message, when called from ADAM2. Ideally, you
# want to load up a Linux kernel or whatever, but this app serves a good test when just playing around with
# the low-level stuff.
# 
# An ADAM2 app is basically a MIPS compiled program, which has been objcopied into the "binary" format (to 
# reduce filesize), and slapped with a header (also described at the URL mentioned above).
#
# Please note that you will need a MIPS cross compiler installed to compile this! If your compiler is not
# called 'mipsel-linux-gcc' (it should), you can pass a custom name prefix using:
#
# 	'CROSS_COMPILE=my-compiler-prefix- make'
#
# (or by setting the CROSS_COMPILE environment variable).
#
# For more info about ADAM2, a bootloader used in many TI AR7-based systems, check out:
# 
# 	http://www.seattlewireless.net/ADAM2
#
# We could use objcopy to directly produce a binary file, but since TI has provided a SREC to BIN converter
# utility, which also adds the necessary ADAM2 app headers, we go ELF -> (objcopy) -> SREC -> (srec2bin) -> 
# BIN.

# How to use:
# 1) Build using "make". This will produce the final ADAM2 application file, called "adam2app.bin"
# 2) Check the docs at http://www.seattlewireless.net/ADAM2 for details on how to upload your program to
#    the device.
# 3) You need a serial cable attached to your device in order for this application to make sense, as it
#    only prints stuff to the debugconsole ;-)

# Tools
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJCOPY = $(CROSS_COMPILE)objcopy
SREC2BIN = ./srec2bin
ADAM2DUMP = adam2dump

ifndef CROSS_COMPILE
	CROSS_COMPILE = mipsel-linux-
endif

# Input & Output
LDSCRIPT = ld.script
PROG = adam2app.elf
BIN = adam2app.bin
SREC = adam2app.srec
OBJS = adam2_app.o

# Compiler & Linker settings
CFLAGS = -Wall -Os -fno-builtin -fomit-frame-pointer -fno-strict-aliasing -fno-inline -fno-common -mips32
LDFLAGS = -nostdlib -nodefaultlibs -T$(LDSCRIPT) -Map=map.lst -N -Ttext $(LOADADDR) -G 0 -e main

# Platform specifics
LOADADDR  = 0x94192000

$(BIN): $(SREC) $(SREC2BIN)
	$(SREC2BIN) $< $@

$(SREC): $(PROG)
	$(OBJCOPY) $< -O srec $@

$(PROG): $(OBJS)
	$(LD) $(LDFLAGS) -o $(PROG) $(OBJS)

$(SREC2BIN):
	gcc -o $(SREC2BIN) srec2bin.c

adam2dump:
	gcc -o $(ADAM2DUMP) adam2_dump.c
	
clean:
	rm -rf $(OBJS) $(PROG) $(BIN) $(SREC) map.lst $(SREC2BIN) $(ADAM2DUMP)

all: $(BIN) adam2dump
	@echo "Done!"
