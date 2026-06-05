#
# $Id: $
#
# $Copyright: (c) 2023 Broadcom.
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
ifneq ($(FTDI_HOME), )
ADD_FLAGS += -DUSE_FTDI
ADD_FLAGS += -L. $(FTDI_HOME)/libftd2xx.a
ADD_FLAGS += -I$(FTDI_HOME)
endif

ADD_FLAGS += -L. -ldl -lrt -lpthread -Wl,-whole-archive ./../build/libphymodepil.a $(EPDM_HOME)/libepdm.a -Wl,-no-whole-archive -lm -DPLP_APERTA2_SUPPORT -DPLP_MACSEC_SUPPORT
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_WRITE_ENABLE
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_READ_ENABLE
ADD_FLAGS += -DBCM_PLP_TIMESYNC_SUPPORT

BIN = ./bin
INCLUDE_C_FILE = aperta2_common.c aperta2_macsec_common.c
EPDM_INCLUDE = $(EPDM_HOME)

# comment out the following line if no GNU READLINE library support
READLINE_LIBS = -lcurses -lreadline -DGNU_READLINE_SUPPORT

ifneq (,$(findstring BCM_PLP_TIMESYNC_SUPPORT,$(ADD_FLAGS)))
TIMESYNC_APP_HOME = .
TIMESYNC_FILES    = scallop_shell.c $(TIMESYNC_APP_HOME)/timesync2_t1234_tc.c
ADD_FLAGS        += $(READLINE_LIBS)

#ADD_FLAGS      += -DPLP_APERTA2_SINGLE_LANE_PORTS
#ADD_FLAGS      += -DPLP_APERTA2_DUAL_LANE_PORTS
#ADD_FLAGS      += -DPLP_APERTA2_QUAD_LANE_PORTS
ADD_FLAGS      += -DPLP_APERTA2_OCTAL_LANE_PORTS
#ADD_FLAGS      += -DPLP_APERTA2_RGB24_LANE_PORTS
#ADD_FLAGS      += -DPLP_APERTA2_FGB42_LANE_PORTS
#ADD_FLAGS      += -DPLP_APERTA2_FAILOVER
endif

all: output

output:
	$(CC) aperta2_macsec_static_bypass_800G_8x106G_PAM4_txfir_setget_phydump.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_800G_8x106G_PAM4_txfir_setget_phydump $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_100G_FGB_4x25G_NRZ_2x53G_PAM4.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_100G_FGB_4x25G_NRZ_2x53G_PAM4 $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_200G_2x106G_PAM4_AutoNeg.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_200G_2x106G_PAM4_AutoNeg $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_200G_RGB_2x106G_PAM4_4x53G_PAM4.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_200G_RGB_2x106G_PAM4_4x53G_PAM4 $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_25G_1x25G_NRZ_txfir_setget.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_25G_1x25G_NRZ_txfir_setget $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_400G_4x106G_PAM4_cl72.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_400G_4x106G_PAM4_cl72 $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_400G_8x53G_PAM4_synce.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_400G_8x53G_PAM4_synce $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_50G_1x50G_PAM4_sysfov_sysXing.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_50G_1x50G_PAM4_sysfov_sysXing $(ADD_FLAGS)
#	$(CC) aperta2_macsec_policy_bypass_400G_8x53G_PAM4.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_policy_bypass_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta2_macsec_transform_100G_2x50G_PAM4_sys_OctalXing.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_100G_2x50G_PAM4_sys_OctalXing $(ADD_FLAGS)
#	$(CC) aperta2_macsec_transform_200G_2x106G_PAM4_with_FOV.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_200G_2x106G_PAM4_with_FOV $(ADD_FLAGS)
#	$(CC) aperta2_macsec_transform_400G_4x106G_PAM4.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_400G_4x106G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta2_macsec_transform_50G_2x25G_NRZ_sys_OctalXing_sys_FOV.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_50G_2x25G_NRZ_sys_OctalXing_sys_FOV $(ADD_FLAGS)
	$(CC) aperta2_macsec_transform_pre_warmboot_100G_4x25G_NRZ.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_pre_warmboot_100G_4x25G_NRZ $(ADD_FLAGS)
	$(CC) aperta2_macsec_transform_warmboot_100G_4x25G_NRZ.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_transform_warmboot_100G_4x25G_NRZ $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_400G_4x106G_PAM4_pre_warmboot.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_400G_4x106G_PAM4_pre_warmboot $(ADD_FLAGS)
	$(CC) aperta2_macsec_static_bypass_400G_4x106G_PAM4_warmboot.c $(INCLUDE_C_FILE) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_macsec_static_bypass_400G_4x106G_PAM4_warmboot $(ADD_FLAGS)

cli:
	$(CC) aperta2_cli_sample.c $(INCLUDE_C_FILE) $(TIMESYNC_FILES) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta2_cli_sample $(ADD_FLAGS)

clean:
	rm -f $(BIN)/*
