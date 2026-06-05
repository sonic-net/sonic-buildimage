#
# $Id$
# $Copyright: (c) 2020 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$


# This sample program is only a reference to demonstrate the usage of
# bcm_pm_if APIs. This program might not work for all environments.

# Compiler
CC = gcc

# EPDM installation directory
EPDM_HOME =
# Use FTDI_HOME variable to provide path to D2XX driver library for FTDI devices if
# an FTDI device will be used for MDIO read/write operations.
FTDI_HOME =

# GCC flags
ADD_FLAGS = -g -Wall -Werror -Wno-uninitialized -Wno-unused-function
ADD_FLAGS += -DPLP_BARCHETTA_SUPPORT -DSERDES_API_FLOATING_POINT
ADD_FLAGS += -DSINGLE_CHIP_SETUP
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_WRITE_ENABLE
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_READ_ENABLE
#ADD_FLAGS += -DBCM_PLP_FORCE_TX_TRAINING_ENABLE
ifneq ($(FTDI_HOME), )
ADD_FLAGS += -DUSE_FTDI
INC_LIBS += -L. $(FTDI_HOME)/libftd2xx.a
INC_PATH += -I$(FTDI_HOME)
endif

# comment out the following line if no CLI Shell support
ADD_FLAGS += -DBCM_PLP_CLI_SHELL_SUPPORT
# comment out the following line if no GNU READLINE library support (for editable CLI inputs)
ADD_FLAGS += -lcurses -lreadline -DGNU_READLINE_SUPPORT

ifneq (,$(findstring BCM_PLP_CLI_SHELL_SUPPORT,$(ADD_FLAGS)))
CLI_SHELL_APP_HOME = .
CLI_SHELL_FILES    = scallop_shell.c
endif

# Libraries used for the application
INC_LIBS += -L. -ldl -lrt -lpthread -Wl,-whole-archive ./../build/libphymodepil.a $(EPDM_HOME)/libepdm.a -Wl,-no-whole-archive -lm

# Include path
INC_PATH += -I. -I$(EPDM_HOME)

# Output binary
OUTPUT_DIR = ./bin

# Common files
COMMON_SRCS = barchetta_common.c

all:
	$(CC) barchetta_reference_app_400G_8x50G_PAM4.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_400G_8x50G_PAM4 $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_50G_1x50G_PAM4.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_50G_1x50G_PAM4 $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_100G_4x25G_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_100G_4x25G_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_40G_4x10G_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_40G_4x10G_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_25G_1x25G_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_25G_1x25G_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_line_force_training_AN_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_line_force_training_AN_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_sys_force_training_AN_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_sys_force_training_AN_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_line_force_training_PAM4.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_line_force_training_PAM4 $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_sys_force_training_PAM4.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_sys_force_training_PAM4 $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_10G_force_fec_cl74_NRZ.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_10G_force_fec_cl74_NRZ $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_fc8.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_fc8 $(INC_LIBS) $(ADD_FLAGS)
	$(CC) barchetta_reference_app_25G_1x25G_NRZ_synce.c $(COMMON_SRCS) $(INC_PATH) -o $(OUTPUT_DIR)/barchetta_reference_app_25G_1x25G_NRZ_synce $(INC_LIBS) $(ADD_FLAGS)

cli:
	$(CC) barchetta_reference_app_cli.c $(COMMON_SRCS) $(INC_PATH) $(CLI_SHELL_FILES) -o $(OUTPUT_DIR)/barchetta_reference_app_cli $(INC_LIBS) $(ADD_FLAGS)

clean:
	rm -f $(OUTPUT_DIR)/*

$(shell mkdir -p $(OUTPUT_DIR))

