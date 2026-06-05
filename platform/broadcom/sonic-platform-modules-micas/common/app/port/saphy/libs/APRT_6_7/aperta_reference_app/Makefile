#
# $Id: $
#
# $Copyright: (c) 2021 Broadcom.
# Broadcom Proprietary and Confidential. All rights reserved.$
#

# This reference Makefile was intented to compile aperta_reference_app. User can optimize it 
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

ADD_FLAGS += -L. -ldl -lrt -lpthread -Wl,-whole-archive ./../build/libphymodepil.a $(EPDM_HOME)/libepdm.a -Wl,-no-whole-archive -lm -DPLP_APERTA_SUPPORT -DPLP_MACSEC_SUPPORT
ADD_FLAGS += -DBCM_PLP_CONTROL_PACKET_POLICY_BYPASS
#ADD_FLAGS += -DBCM_PLP_81384_DEFAULT_POLARITY
#ADD_FLAGS += -DBCM_PLP_DATA_PACKET_POLICY_BYPASS
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_WRITE_ENABLE
#ADD_FLAGS += -DBCM_PLP_MDIO_TRACE_READ_ENABLE
#ADD_FLAGS += -DTRAFFIC_DEBUG
# comment out the following line if no IEEE1588 TimeSync support
ADD_FLAGS += -DBCM_PLP_TIMESYNC_SUPPORT

# comment out the following line if no GNU READLINE library support
READLINE_LIBS = -lcurses -lreadline -DGNU_READLINE_SUPPORT

BIN = ./bin
INCLUDE_C_FILE = aperta_common.c
INCLUDE_HELPER_FILE = aperta_helper.c
EPDM_INCLUDE = $(EPDM_HOME)

ifneq (,$(findstring BCM_PLP_TIMESYNC_SUPPORT,$(ADD_FLAGS)))
TIMESYNC_APP_HOME = .
TIMESYNC_FILES    = scallop_shell.c $(TIMESYNC_APP_HOME)/timesync_refapp.c $(TIMESYNC_APP_HOME)/timesync_helper.c
ADD_FLAGS        += $(READLINE_LIBS)
# ADD_FLAGS      += -DUSER_GIVES_BOARD_NAME -DPLP_APERTA_10G_NRZ
# ADD_FLAGS      += -DPLP_APERTA_25G_50G_FOV   ## 50G/25G Failover mode support
# ADD_FLAGS      += -DPLP_APERTA_100G_RGB_FOV
# ADD_FLAGS      += -DPLP_APERTA_100G_PAM4
# ADD_FLAGS      += -DPLP_APERTA_100G_NRZ
endif

all: output


output:
#	$(CC) aperta_macsec_policy_bypass_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_policy_bypass_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_macsec_policy_bypass_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_policy_bypass_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_static_bypass_100G_4x25G_NRZ.c $(INCLUDE_C_FILE) $(INCLUDE_HELPER_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_static_bypass_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_transform_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_transform_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_transform_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_transform_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_macsec_key_renew_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_key_renew_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_macsec_key_exchange_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_key_exchange_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_macsec_KaY_bypass_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_KaY_bypass_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_auto_negotiation_25G_1x25G_NRZ.c $(INCLUDE_C_FILE) $(TIMESYNC_FILES) -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_auto_negotiation_25G_1x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_auto_negotiation_50G_1x50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_auto_negotiation_50G_1x50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_mixed_mode_10G_NRZ_50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_mixed_mode_10G_NRZ_50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_mixed_mode_40G_NRZ_100G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_mixed_mode_40G_NRZ_100G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_mixed_mode_100G_NRZ_200G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_mixed_mode_100G_NRZ_200G_PAM4 $(ADD_FLAGS)
	$(CC) aperta_81384_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_81384_100G_4x25G_NRZ $(ADD_FLAGS)
	$(CC) aperta_pre_warmboot_81384_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_pre_warmboot_81384_100G_4x25G_NRZ $(ADD_FLAGS)
	$(CC) aperta_warmboot_81384_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_warmboot_81384_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_transform_pre_warmboot_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_transform_pre_warmboot_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_transform_warmboot_100G_4x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_transform_warmboot_100G_4x25G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_synce_macsec_transform_400G_8x53G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_synce_macsec_transform_400G_8x53G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_gb_asym_primary_lane_50G_NRZ_50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_gb_asym_primary_lane_50G_NRZ_50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_gb_asym_alternate_lane_50G_NRZ_50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_gb_asym_alternate_lane_50G_NRZ_50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_gb_sym_primary_lane_50G_NRZ_50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_gb_sym_primary_lane_50G_NRZ_50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_gb_sym_alternate_lane_50G_NRZ_50G_PAM4.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_gb_sym_alternate_lane_50G_NRZ_50G_PAM4 $(ADD_FLAGS)
#	$(CC) aperta_rgb_asym_primary_lane_50G_PAM4_50G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_rgb_asym_primary_lane_50G_PAM4_50G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_rgb_asym_alternate_lane_50G_PAM4_50G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_rgb_asym_alternate_lane_50G_PAM4_50G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_rgb_sym_primary_lane_50G_PAM4_50G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_rgb_sym_primary_lane_50G_PAM4_50G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_rgb_sym_alternate_lane_50G_PAM4_50G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_rgb_sym_alternate_lane_50G_PAM4_50G_NRZ $(ADD_FLAGS)
	$(CC) aperta_failover_mux_sys_10G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_failover_mux_sys_10G_NRZ $(ADD_FLAGS)
	$(CC) aperta_failover_mux_line_10G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_failover_mux_line_10G_NRZ $(ADD_FLAGS)
#	$(CC) aperta_macsec_cleartag.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_cleartag $(ADD_FLAGS)
#	$(CC) aperta_macsec_cleartag_vlan.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_cleartag_vlan $(ADD_FLAGS)
#	$(CC) aperta_pcs_dump_25G_1x25G_NRZ.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_pcs_dump_25G_1x25G_NRZ $(ADD_FLAGS)
	$(CC) aperta_macsec_failover_100G_RGB.c $(INCLUDE_C_FILE)  -I $(EPDM_INCLUDE) -I ./ -o $(BIN)/aperta_macsec_failover_100G_RGB $(ADD_FLAGS)

cli:
ifneq (,$(findstring BCM_PLP_TIMESYNC_SUPPORT,$(ADD_FLAGS)))
	$(CC) aperta_cli_sample.c -g $(INCLUDE_C_FILE) $(TIMESYNC_FILES) -I $(EPDM_INCLUDE) -I $(PWD) -o $(BIN)/aperta_cli_sample $(ADD_FLAGS)
	$(CC) aperta_cli_failover_25G_50G.c -g $(INCLUDE_C_FILE) $(TIMESYNC_FILES) -I $(EPDM_INCLUDE) -I $(PWD) -o $(BIN)/aperta_cli_failover_25G_50G $(ADD_FLAGS)
endif

clean:
	rm -f $(BIN)/*
