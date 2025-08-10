PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUB_BUILD_DIR = $(PWD)/build
SERVICE_DIR = $(PWD)/service
INSTALL_DIR = $(SUB_BUILD_DIR)/$(KERNEL_SRC)/$(INSTALL_MOD_DIR)
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3.9/dist-packages
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system
INSTALL_ETC_DIR = $(SUB_BUILD_DIR)/etc
UNSUPPORT_KERVER = 4.9.189

all:
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/lib/ ]; then cp -r $(PWD)/lib/* ${INSTALL_LIB_DIR} ;fi
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ -d $(PWD)/script/ ]; then cp -r $(PWD)/script/* ${INSTALL_SCRIPT_DIR} ;fi
	@if [ ! -d ${INSTALL_ETC_DIR} ]; then mkdir -p ${INSTALL_ETC_DIR} ;fi
	@if [ -d $(INSTALL_SCRIPT_DIR) ]; then chmod +x $(INSTALL_SCRIPT_DIR)/* ;fi
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	if [ -d $(SERVICE_DIR) ]; then cp -r $(SERVICE_DIR)/*  $(INSTALL_SERVICE_DIR) ;fi
clean:
	rm -rf $(SUB_BUILD_DIR)

