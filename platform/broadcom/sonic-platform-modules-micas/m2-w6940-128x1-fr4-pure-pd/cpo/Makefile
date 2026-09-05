PWD		= $(shell pwd)
SUB_BUILD_DIR = $(PWD)/../build

INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3/dist-packages
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system

all:
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	cp -r $(PWD)/service/*  $(INSTALL_SERVICE_DIR)
	cp -r $(PWD)/script/*  $(INSTALL_SCRIPT_DIR)
	cp -r $(PWD)/lib/*  $(INSTALL_LIB_DIR)
	cp -r $(PWD)/config/*  $(INSTALL_SCRIPT_DIR)
	@if [ -d ${INSTALL_SCRIPT_DIR} ]; then chmod +x ${INSTALL_SCRIPT_DIR}/*;fi