PWD		= $(shell pwd)
SUB_BUILD_DIR = $(PWD)/build
INSTALL_SERVICE_DIR = $(SUB_BUILD_DIR)/lib/systemd/system/

all:
	@if [ ! -d ${INSTALL_SERVICE_DIR} ]; then mkdir -p ${INSTALL_SERVICE_DIR} ;fi
	cp -L $(PWD)/service/*.service  $(INSTALL_SERVICE_DIR)
