PWD		= $(shell pwd)
CC ?=gcc
INSTALL_MOD_DIR ?=extra
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUB_BUILD_DIR = $(PWD)/build
SERVICE_DIR = $(PWD)/service
SYSFS_DIR = $(PWD)/S3IP_sysfs
SYSFS_BUILD_DIR = $(SYSFS_DIR)/build
INSTALL_MODULE_DIR = $(SUB_BUILD_DIR)/$(KERNEL_SRC)/$(INSTALL_MOD_DIR)
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3/dist-packages
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system
INSTALL_CFG_DIR = $(SUB_BUILD_DIR)/etc/dfd_cfg
COMMON_TX_OUTPUT := $(shell pwd)/build
export SUB_BUILD_DIR COMMON_TX_OUTPUT

all : SUBDIRS_OBJ CHECK
CHECK :
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/lib/ ]; then cp -r $(PWD)/lib/* ${INSTALL_LIB_DIR} ;fi
	@if [ ! -d ${COMMON_TX_OUTPUT} ]; then mkdir -p ${COMMON_TX_OUTPUT} ;fi
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ -d $(PWD)/script/ ]; then cp -r $(PWD)/script/* ${INSTALL_SCRIPT_DIR} ;fi
	@if [ -d $(INSTALL_SCRIPT_DIR) ]; then chmod +x $(INSTALL_SCRIPT_DIR)/* ;fi
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	cp -r $(SERVICE_DIR)/*  $(INSTALL_SERVICE_DIR)
	@if [ ! -d ${INSTALL_MODULE_DIR} ]; then mkdir -p ${INSTALL_MODULE_DIR} ;fi
	cp -r $(SYSFS_BUILD_DIR)/S3IP_sysfs/*.ko $(INSTALL_MODULE_DIR)
	@if [ ! -d ${INSTALL_CFG_DIR} ]; then mkdir -p ${INSTALL_CFG_DIR} ;fi
	cp -r $(SYSFS_BUILD_DIR)/dfd_cfg/*  $(INSTALL_CFG_DIR)

SUBDIRS_OBJ :
	make -C $(SYSFS_DIR)

clean:
	rm -rf $(SUB_BUILD_DIR)