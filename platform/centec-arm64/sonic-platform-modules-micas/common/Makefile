PWD		= $(shell pwd)
CC ?=gcc
INSTALL_MOD_DIR ?=extra
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
ifdef ENABLE_GCOV
ifeq ($(ENABLE_GCOV), y)
EXTRA_CFLAGS+= -fprofile-arcs -ftest-coverage -lgcov
endif
endif # ENABLE_GCOV
SUB_BUILD_DIR = $(PWD)/build
DIR_KERNEL_SRC = $(PWD)/modules
SCRIPT_DIR = $(PWD)/script
SERVICE_DIR = $(PWD)/service
BLACK_DRIVER_CONF_DIR = $(PWD)/modprobe_conf

app_dir = $(PWD)/app
app_build_dir = $(app_dir)/build
modules_build_dir = $(DIR_KERNEL_SRC)/build

INSTALL_MODULE_DIR = $(SUB_BUILD_DIR)/$(KERNEL_SRC)/$(INSTALL_MOD_DIR)
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3.9/dist-packages
INSTALL_BLACK_DRIVER = $(SUB_BUILD_DIR)/etc/modprobe.d
CTC_48GT4X_PHY_DIR = $(SUB_BUILD_DIR)/usr/share/sonic/device/arm64-micas_m2-w6010-48gt4x-fa-r0/M2-W6010-48GT4X-FA
CTC_48GT4X_R_PHY_DIR = $(SUB_BUILD_DIR)/usr/share/sonic/device/arm64-micas_m2-w6010-48gt4x-ra-r0/M2-W6010-48GT4X-RA

all:
	$(MAKE) -C $(app_dir)
	$(MAKE) -C $(DIR_KERNEL_SRC)
	@if [ ! -d ${INSTALL_MODULE_DIR} ]; then mkdir -p ${INSTALL_MODULE_DIR} ;fi
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	@if [ ! -d ${CTC_48GT4X_PHY_DIR} ]; then mkdir -p ${CTC_48GT4X_PHY_DIR}/phy_drv ;fi
	@if [ ! -d ${CTC_48GT4X_R_PHY_DIR} ]; then mkdir -p ${CTC_48GT4X_R_PHY_DIR}/phy_drv ;fi
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/lib/ ]; then cp -r $(PWD)/lib/* ${INSTALL_LIB_DIR} ;fi
	cp -r $(app_dir)/*.so $(CTC_48GT4X_PHY_DIR)/phy_drv
	cp -r $(app_dir)/*.so $(CTC_48GT4X_R_PHY_DIR)/phy_drv
	cp -r $(modules_build_dir)/*.ko $(INSTALL_MODULE_DIR)
	cp -r $(app_dir)/build/app/*  $(INSTALL_SCRIPT_DIR)
	if [ -d $(SCRIPT_DIR) ]; then cp -r $(SCRIPT_DIR)/*  $(INSTALL_SCRIPT_DIR) ;fi
	if [ -d $(SERVICE_DIR) ]; then cp -r $(SERVICE_DIR)/*  $(INSTALL_SERVICE_DIR) ;fi
	@if [ -d $(INSTALL_SCRIPT_DIR) ]; then chmod +x $(INSTALL_SCRIPT_DIR)/* ;fi
clean:
	rm -rf $(SUB_BUILD_DIR)
