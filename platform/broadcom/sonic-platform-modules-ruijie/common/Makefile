PWD		= $(shell pwd)
CC ?=gcc
INSTALL_MOD_DIR ?=extra
KVERSION   ?= $(shell uname -r)
KERNEL_SRC ?=  /lib/modules/$(KVERSION)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUB_BUILD_DIR = $(PWD)/build
DIR_KERNEL_SRC = $(PWD)/modules
SCRIPT_DIR = $(PWD)/script
SERVICE_DIR = $(PWD)/service
DEPMOD_CONF_DIR = $(PWD)/depmod_conf

KBUILD_EXTRA_SYMBOLS += $(DIR_KERNEL_SRC)/Module.symvers
export KBUILD_EXTRA_SYMBOLS

INSTALL_MODULE_DIR = $(SUB_BUILD_DIR)/$(KERNEL_SRC)/$(INSTALL_MOD_DIR)
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3.7/dist-packages
INSTALL_DEPMOD_CONF  = $(SUB_BUILD_DIR)/etc/depmod.d

all:
	$(MAKE) -C $(KERNEL_SRC)/build M=$(DIR_KERNEL_SRC) modules
	@if [ ! -d ${INSTALL_MODULE_DIR} ]; then mkdir -p ${INSTALL_MODULE_DIR} ;fi
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/lib/ ]; then cp -r $(PWD)/lib/* ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/lib/ ]; then cp -r $(PWD)/lib/* ${INSTALL_LIB_DIR2} ;fi
	cp -r $(DIR_KERNEL_SRC)/*.ko $(INSTALL_MODULE_DIR)
	@if [ ! -d ${INSTALL_DEPMOD_CONF} ]; then mkdir -p ${INSTALL_DEPMOD_CONF} ;fi
	cp -r  $(DEPMOD_CONF_DIR)/*  $(INSTALL_DEPMOD_CONF)
	cp -r $(SCRIPT_DIR)/*  $(INSTALL_SCRIPT_DIR)
	cp -r $(SERVICE_DIR)/*  $(INSTALL_SERVICE_DIR)
	@if [ -d $(INSTALL_SCRIPT_DIR) ]; then chmod +x $(INSTALL_SCRIPT_DIR)/* ;fi
clean:
	rm -f ${DIR_KERNEL_SRC}/*.o ${DIR_KERNEL_SRC}/*.ko ${DIR_KERNEL_SRC}/*.mod.c ${DIR_KERNEL_SRC}/.*.cmd
	rm -f ${DIR_KERNEL_SRC}/Module.markers ${DIR_KERNEL_SRC}/Module.symvers ${DIR_KERNEL_SRC}/modules.order
	rm -rf ${DIR_KERNEL_SRC}/.tmp_versions
	rm -rf $(SUB_BUILD_DIR)
