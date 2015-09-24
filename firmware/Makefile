# ----------------------------------------------------------------------------
#         ATMEL Microcontroller Software Support  -  ROUSSET  -
# ----------------------------------------------------------------------------
# Copyright (c) 2006, Atmel Corporation
#
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
# 
# - Redistributions of source code must retain the above copyright notice,
# this list of conditions and the disclaiimer below.
# 
# - Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the disclaimer below in the documentation and/or
# other materials provided with the distribution. 
# 
# Atmel's name may not be used to endorse or promote products derived from
# this software without specific prior written permission. 
# 
# DISCLAIMER: THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
# DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
# EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# ---------------------------------------------------------------------------- 

# 	Makefile for compiling the USB CDC serial project

#-------------------------------------------------------------------------------
#		User-modifiable options
#-------------------------------------------------------------------------------

# Chip & board used for compilation
# (can be overriden by adding CHIP=chip and BOARD=board to the command-line)
CHIP  = at91sam7x256
BOARD = at91sam7x-ek

# Optimization level, put in comment for debugging
OPTIMIZATION = -Os

# AT91 library directory
AT91LIB = at91lib

# Output file basename
OUTPUT = firmware

# Output directories
BIN = bin
OBJ = obj

#-------------------------------------------------------------------------------
#		Tools
#-------------------------------------------------------------------------------

# Tool suffix when cross-compiling
CROSS = arm-elf-

# Compilation tools
CC = $(CROSS)gcc
SIZE = $(CROSS)size
OBJCOPY = $(CROSS)objcopy

# Flags
INCLUDES = -Isrc -I$(AT91LIB)/boards/$(BOARD) -I$(AT91LIB)/peripherals 
INCLUDES += -I$(AT91LIB)/components -I$(AT91LIB)/usbdevice -I$(AT91LIB)

CFLAGS = -Wall -mlong-calls
CFLAGS += -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -ffunction-sections
ASFLAGS = -g $(OPTIMIZATION) $(INCLUDES) -D$(CHIP) -D__ASSEMBLY__
LDFLAGS = -g $(OPTIMIZATION) -nostartfiles -Wl,--gc-sections

#-------------------------------------------------------------------------------
#		Files
#-------------------------------------------------------------------------------

# Directories where source files can be found
USB = $(AT91LIB)/usb
UTILITY = $(AT91LIB)/utility
PERIPH = $(AT91LIB)/peripherals
BOARDS = $(AT91LIB)/boards

VPATH += $(USB)/device/cdc-serial $(USB)/device/core $(USB)/common/core
VPATH += $(AT91LIB)/memories $(USB)/common/massstorage $(USB)/device/massstorage
VPATH += $(USB)/common/cdc
VPATH += $(UTILITY)
VPATH += $(PERIPH)/dbgu $(PERIPH)/aic $(PERIPH)/usart $(PERIPH)/pio
VPATH += $(BOARDS)/$(BOARD) $(BOARDS)/$(BOARD)/$(CHIP)
VPATH += src

# Objects built from C source files
C_OBJECTS = main.o gui.o registry.o

C_OBJECTS += CDCDSerialDriverDescriptors.o
C_OBJECTS += CDCSetControlLineStateRequest.o CDCLineCoding.o

C_OBJECTS += Media.o MEDSdram.o MEDFlash.o SBCMethods.o
C_OBJECTS += MSDLun.o MSDDriverDescriptors.o 

C_OBJECTS += USBD_OTGHS.o USBD_UDP.o USBD_UDPHS.o USBDDriver.o
C_OBJECTS += USBDCallbacks_Initialized.o
C_OBJECTS += USBDCallbacks_Reset.o
C_OBJECTS += USBDCallbacks_Resumed.o
C_OBJECTS += USBDCallbacks_Suspended.o
C_OBJECTS += USBDDriverCallbacks_InterfaceSettingChanged.o
C_OBJECTS += USBSetAddressRequest.o USBGenericDescriptor.o USBInterfaceRequest.o
C_OBJECTS += USBGenericRequest.o USBGetDescriptorRequest.o 
C_OBJECTS += USBSetConfigurationRequest.o USBFeatureRequest.o
C_OBJECTS += USBEndpointDescriptor.o USBConfigurationDescriptor.o
C_OBJECTS += string.o stdio.o
C_OBJECTS += aic.o usart.o pio.o pio_it.o
C_OBJECTS += board_memories.o board_lowlevel.o

VPATH += src/os
C_OBJECTS += messages.o malloc.o timer.o

VPATH += src/drivers
C_OBJECTS += ethernet.o display.o sdcard.o keyboard.o audio.o eeprom.o

VPATH += src/grlib
C_OBJECTS += grlib.o menus.o palette.o dialogs.o window.o

VPATH += src/usb
C_OBJECTS += MSDDriver.o CDCDSerialDriver.o adm8511.o adm8511Descriptors.o usbdevices.o

VPATH += src/fs
C_OBJECTS += romfs.o volume.o fat.o

VPATH += src/net
C_OBJECTS += bridge.o arp.o ip.o dhcp.o

VPATH += src/apps
C_OBJECTS += app_ping.o app_vct.o app_update.o

# Objects built from Assembly source files
ASM_OBJECTS = board_cstartup.o

# Append BIN & OBJ directory to files
C_OBJECTS := $(addprefix $(OBJ)/, $(C_OBJECTS))
ASM_OBJECTS := $(addprefix $(OBJ)/, $(ASM_OBJECTS))
OUTPUT := $(BIN)/$(OUTPUT)

OBJECTS = romfs/romfs.o

#-------------------------------------------------------------------------------
#		Rules
#-------------------------------------------------------------------------------

# Get the list of available targets from the board.mak file
include $(AT91LIB)/boards/$(BOARD)/board.mak

all: $(BIN) $(MEMORIES)

$(MEMORIES): $(ASM_OBJECTS) $(C_OBJECTS) $(OBJECTS)
	$(CC) $(LDFLAGS) -T"$(AT91LIB)/boards/$(BOARD)/$(CHIP)/$@.lds" -o $(OUTPUT)-$@.elf $^
	$(OBJCOPY) -O binary $(OUTPUT)-$@.elf $(OUTPUT)-$@.bin
	Debug/bin2efw $(OUTPUT)-$@.bin bin/pt-1.efw

$(C_OBJECTS): $(OBJ)/%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

$(ASM_OBJECTS): $(OBJ)/%.o: %.S Makefile $(OBJ)
	$(CC) $(ASFLAGS) -c -o $@ $<

$(OBJ) $(BIN):
	@mkdir $@

clean:
	-rm -f $(OBJ)/*.o $(BIN)/*.bin $(BIN)/*.elf

