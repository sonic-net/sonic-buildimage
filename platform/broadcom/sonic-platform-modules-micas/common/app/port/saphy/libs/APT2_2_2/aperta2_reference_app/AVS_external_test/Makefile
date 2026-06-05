#
# $Id: $
#
# $Copyright: (c) 2024 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#

# This reference Makefile was intented to compile aperta2_reference_app. User can optimize it 
# as required.

#Sample program is only a reference to the user to tell how to call BCM PLP APIs.
#Sample program may not work at different environments.


# which compiler
CC = gcc

# Define EPDM_HOME based on your EPDM installation directory
EPDM_HOME =

ADD_FLAGS = -g -Wall -Werror -Wno-uninitialized 

# Use FTDI_HOME variable to provide path to D2XX driver library for FTDI devices if
# an FTDI device will be used for MDIO read/write operations.
FTDI_HOME =
SMBUS_HOME=./smbus
ifneq ($(FTDI_HOME), )
ADD_FLAGS += -DUSE_FTDI
ADD_FLAGS += -L. $(FTDI_HOME)/libftd2xx.a
ADD_FLAGS += -I$(FTDI_HOME)
endif

ADD_FLAGS += -L. -ldl -lrt -lpthread -Wl,-whole-archive ./../../build/libphymodepil.a $(EPDM_HOME)/libepdm.a $(SMBUS_HOME)/lib/libsmbus.a -Wl,-no-whole-archive -lm -DPLP_APERTA2_SUPPORT -DPLP_MACSEC_SUPPORT
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_WRITE_ENABLE
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_READ_ENABLE


BIN = ./bin

# Create output directory if not available
$(shell mkdir -p $(BIN))

INCLUDE_C_FILE = aperta2_common.c
EPDM_INCLUDE = $(EPDM_HOME)
SMBUS_INCLUDE = $(SMBUS_HOME)/inc

all: output

output:
	$(CC) avs_external_test.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I $(SMBUS_INCLUDE) -I ./ -o $(BIN)/avs_external_test $(ADD_FLAGS)
clean:
	rm -f $(BIN)/*
