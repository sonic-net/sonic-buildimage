PWD		= $(shell pwd)
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUB_BUILD_DIR = $(PWD)/build
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
export SUB_BUILD_DIR

all : CHECK
CHECK :
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	@if [ $(ls -A $(PWD)/script/ | wc -l) -gt 0 ]; then cp -r $(PWD)/script/* ${INSTALL_SCRIPT_DIR} ;fi
	@if [ -d $(INSTALL_SCRIPT_DIR) ]; then chmod +x $(INSTALL_SCRIPT_DIR)/* ;fi

clean:
	rm -rf $(SUB_BUILD_DIR)
