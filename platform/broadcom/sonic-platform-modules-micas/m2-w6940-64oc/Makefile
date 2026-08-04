PWD		= $(shell pwd)
DIR_KERNEL_SRC = $(PWD)/modules/driver
EXTRA_CFLAGS:= -I$(M)/include
EXTRA_CFLAGS+= -Wall
SUB_BUILD_DIR = $(PWD)/build
INSTALL_DIR = $(SUB_BUILD_DIR)/$(KERNEL_SRC)/$(INSTALL_MOD_DIR)
INSTALL_SCRIPT_DIR = $(SUB_BUILD_DIR)/usr/local/bin
INSTALL_LIB_DIR = $(SUB_BUILD_DIR)/usr/lib/python3/dist-packages
INSTALL_S3IP_SYSFS_CFG_DIR = $(SUB_BUILD_DIR)/etc/s3ip_sysfs_cfg
INSTALL_S3IP_CONFIG_DIR = $(SUB_BUILD_DIR)/etc/s3ip

all:
	$(MAKE) -C $(KBUILD_OUTPUT) M=$(DIR_KERNEL_SRC) modules
	@if [ ! -d ${INSTALL_DIR} ]; then mkdir -p ${INSTALL_DIR} ;fi
	cp -r $(DIR_KERNEL_SRC)/*.ko $(INSTALL_DIR)
	@if [ ! -d ${INSTALL_SCRIPT_DIR} ]; then mkdir -p ${INSTALL_SCRIPT_DIR} ;fi
	cp -r $(PWD)/config/*  $(INSTALL_SCRIPT_DIR)
	@if [ ! -d ${INSTALL_LIB_DIR} ]; then mkdir -p ${INSTALL_LIB_DIR} ;fi
	@if [ -d $(PWD)/hal-config/ ]; then cp -r $(PWD)/hal-config/* ${INSTALL_LIB_DIR} ;fi
	@if [ ! -d ${INSTALL_S3IP_CONFIG_DIR} ]; then mkdir -p ${INSTALL_S3IP_CONFIG_DIR} ;fi
	@if [ -d $(PWD)/s3ip_config/ ]; then cp -r $(PWD)/s3ip_config/* ${INSTALL_S3IP_CONFIG_DIR} ;fi
	@if [ ! -d ${INSTALL_S3IP_SYSFS_CFG_DIR} ]; then mkdir -p ${INSTALL_S3IP_SYSFS_CFG_DIR} ;fi
	@if [ -d $(PWD)/s3ip_sysfs_cfg/ ]; then cp -r $(PWD)/s3ip_sysfs_cfg/* ${INSTALL_S3IP_SYSFS_CFG_DIR} ;fi
clean:
	rm -f ${DIR_KERNEL_SRC}/*.o ${DIR_KERNEL_SRC}/*.ko ${DIR_KERNEL_SRC}/*.mod.c ${DIR_KERNEL_SRC}/.*.cmd ${DIR_KERNEL_SRC}/*.mod
	rm -f ${DIR_KERNEL_SRC}/Module.markers ${DIR_KERNEL_SRC}/Module.symvers ${DIR_KERNEL_SRC}/modules.order
	rm -rf ${DIR_KERNEL_SRC}/.tmp_versions
	rm -rf $(SUB_BUILD_DIR)
