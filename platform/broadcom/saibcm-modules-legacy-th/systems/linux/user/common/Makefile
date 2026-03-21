# -*- Makefile -*-
# $Id: Makefile,v 1.4 Broadcom SDK $
# $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
# 
# Permission is granted to use, copy, modify and/or distribute this
# software under either one of the licenses below.
# 
# License Option 1: GPL
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as
# published by the Free Software Foundation (the "GPL").
# 
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License version 2 (GPLv2) for more details.
# 
# You should have received a copy of the GNU General Public License
# version 2 (GPLv2) along with this source code.
# 
# 
# License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
# 
# This software is governed by the Broadcom Open Network Switch APIs license:
# https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
# 
# 
#

SDK :=$(shell if [ -n "$$SDK" ] ; then\
	echo $$SDK;\
	else\
	cd $(dir $(lastword $(MAKEFILE_LIST))); while /usr/bin/test ! -e RELEASE ; do \
	dir=`cd ../;pwd`;			\
	if [ "$$dir" = "/" ] ; then		\
	   echo Cannot find SDK in $(lastword $(MAKEFILE_LIST)) 1>&2;		\
	   exit 1;				\
	fi ;					\
	cd $$dir;				\
	done ;					\
	pwd;					\
	fi)

ifeq ($(SDK),)					
$(error Please run this in a tree)		
endif 				

export SDK

override-target=linux-$(platform)

ifeq ($(LINUX_MAKE_SHARED_LIB),1)
libext = so.$(SHAREDLIBVER)
else
libext = a
endif

ifeq (,$(platform))
$(error Internal error: platform variable is not set)
endif

include $(SDK)/make/Make.config
LOCALDIR=systems/linux/user/common
include $(SDK)/make/Make.depend
kernel-override=linux-$(platform)
ifdef BLDCONFIG
KERN_BLDROOT=${SDK}/${SDKBUILD}/${BLDCONFIG}/$(kernel-override)$(bldroot_suffix)
else
ifdef SDK_OUTDIR
KERN_BLDROOT=${SDK_OUTDIR}/${SDKBUILD}/$(kernel-override)$(bldroot_suffix)
else
KERN_BLDROOT=${SDK}/${SDKBUILD}/$(kernel-override)$(bldroot_suffix)
endif
endif

ifeq (,$(kernel_version))
kernel_version=2_4
endif

ifneq ($(kernel_version),2_4)
KOBJ=ko
else
KOBJ=o
endif

ifneq (,$(TOOLS_DIR))
override PATH := $(TOOLS_DIR):$(PATH)
endif

ifeq ($(DEST_DIR),)
DEST_DIR=${BLDDIR}
endif

KERNEL_BDE_LOCAL := linux-kernel-bde.$(KOBJ)
KERNEL_BDE := $(DEST_DIR)/$(KERNEL_BDE_LOCAL)

USER_BDE_LOCAL := linux-user-bde.$(KOBJ)
USER_BDE := $(DEST_DIR)/$(USER_BDE_LOCAL)

KNET_CB_LOCAL := linux-knet-cb.$(KOBJ)
KNET_CB := $(DEST_DIR)/$(KNET_CB_LOCAL)

export BUILD_BCM_GENL=1
export BUILD_GENL_PACKET=0
GENL_PACKET_LOCAL := genl-packet.$(KOBJ)
GENL_PACKET := $(DEST_DIR)/$(GENL_PACKET_LOCAL)
ifndef BUILD_GENL_PACKET
export BUILD_GENL_PACKET = 1
endif

BCM_GENL_LOCAL := linux-bcm-genl.$(KOBJ)
BCM_GENL := $(DEST_DIR)/$(BCM_GENL_LOCAL)

BCM_KNET_LOCAL := linux-bcm-knet.$(KOBJ)
BCM_KNET := $(DEST_DIR)/$(BCM_KNET_LOCAL)

BCM_PTP_LOCAL := linux-bcm-ptp-clock.$(KOBJ)
BCM_PTP := $(DEST_DIR)/$(BCM_PTP_LOCAL)

ifeq (,$(findstring DELIVER,$(MAKECMDGOALS)))
.DEFAULT_GOAL := all
all_targets := kernel_modules $(KERNEL_BDE) $(USER_BDE)

LOCAL_TARGETS :=

ifeq ($(NO_LOCAL_TARGETS),)
LOCAL_TARGETS +=$(patsubst %,$(realpath ..)/$(platform)/%,$(KERNEL_BDE_LOCAL) $(USER_BDE_LOCAL))

all_targets +=$(LOCAL_TARGETS)
endif

# Build all by default
ifndef BUILD_KNET
BUILD_KNET = 1
endif

ifndef BUILD_PTP
BUILD_PTP = 1
endif

ifeq ($(BUILD_KNET),1)
# Kernel network support
all_targets += $(BCM_KNET)
knet_subdirs = bcm-knet

ifeq ($(NO_LOCAL_TARGETS),)
LOCAL_TARGETS +=$(patsubst %,../$(platform)/%,$(BCM_KNET_LOCAL))
all_targets +=$(LOCAL_TARGETS)
endif

ifdef BUILD_KNET_CB
all_targets += $(KNET_CB)
knet_subdirs += knet-cb

ifdef BUILD_PTP
all_targets += $(BCM_PTP)
knet_subdirs += bcm-ptp-clock
endif

ifeq ($(NO_LOCAL_TARGETS),)
LOCAL_TARGETS +=$(patsubst %,../$(platform)/%,$(KNET_CB_LOCAL))
all_targets +=$(LOCAL_TARGETS)
endif
endif

ifeq ($(BUILD_BCM_GENL),1)
ifeq ($(BUILD_GENL_PACKET),1)
all_targets += $(GENL_PACKET)
knet_subdirs += genl-packet

ifeq ($(NO_LOCAL_TARGETS),)
LOCAL_TARGETS +=$(patsubst %,../$(platform)/%,$(GENL_PACKET_LOCAL))
all_targets +=$(LOCAL_TARGETS)
endif
endif
all_targets += $(BCM_GENL)
knet_subdirs += bcm-genl

ifeq ($(NO_LOCAL_TARGETS),)
LOCAL_TARGETS +=$(patsubst %,../$(platform)/%,$(BCM_GENL_LOCAL))
all_targets +=$(LOCAL_TARGETS)
endif
endif


ADD_TO_CFLAGS += -I$(SDK)/systems/linux/kernel/modules/include
COND_KNET_LIBS = libuser.$(libext)
endif

all: $(BLDDIR)/.tree $(all_targets)

ifeq ($(NO_LOCAL_TARGETS),)
define LOCAL_TARGET_DEF
$(1) : $(DEST_DIR)/$(notdir $(1))
	cp $$? $$@
endef
endif

ifeq (1,$(USE_CLANG))
OPT_KERNEL_TOOLCHAIN = CC=clang LD=ld.lld AR=llvm-ar
include $(SDK)/make/Make.clang
ADD_TO_KCFLAGS += $(CLANG_ADD_CFLAGS)
endif

# User BDE libraries
ADD_TO_CFLAGS += -I$(SDK)/systems/bde/linux/include

CFLAGS += $(ADD_TO_CFLAGS)

#SAI_FIXUP
CFLAGS:=$(filter-out -fPIC, $(CFLAGS))

kernel_modules:
	$(MAKE) -C $(SDK)/systems/bde/linux/kernel $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)"
	$(MAKE) -C $(SDK)/systems/bde/linux/user/kernel $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)"
ifeq ($(BUILD_KNET),1)
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/shared $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/bcm-knet $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
ifneq (,$(filter knet-cb,$(knet_subdirs)))
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/knet-cb $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
endif
ifneq (,$(filter bcm-ptp-clock,$(knet_subdirs)))
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/bcm-ptp-clock $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
endif
ifneq (,$(filter bcm-genl,$(knet_subdirs)))
ifneq (,$(filter genl-packet,$(knet_subdirs)))
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/genl-packet $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
endif
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules/bcm-genl $(OPT_KERNEL_TOOLCHAIN) kernel_version=$(kernel_version) \
		OPT_CFLAGS="$(ADD_TO_KCFLAGS)" override-target=linux-$(platform)
endif
endif

$(KERNEL_BDE): $(KERN_BLDROOT)/$(KERNEL_BDE_LOCAL)
	mkdir -p $(@D)
	$(OBJCOPY) --strip-debug $< $@

$(USER_BDE): $(KERN_BLDROOT)/$(USER_BDE_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(BCM_KNET): $(KERN_BLDROOT)/$(BCM_KNET_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(KNET_CB): $(KERN_BLDROOT)/$(KNET_CB_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(BCM_PTP): $(KERN_BLDROOT)/$(BCM_PTP_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(GENL_PACKET): $(KERN_BLDROOT)/$(GENL_PACKET_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(BCM_GENL): $(KERN_BLDROOT)/$(BCM_GENL_LOCAL)
	$(OBJCOPY) --strip-debug $< $@

$(KERN_BLDROOT)/%.$(KOBJ): kernel_modules
	@echo Force execution of kernel_modules target > /dev/null

ifeq ($(NO_LOCAL_TARGETS),)
$(foreach targ,$(LOCAL_TARGETS),$(eval $(call LOCAL_TARGET_DEF,$(targ))))
endif

clean::
	$(MAKE) -C $(SDK)/systems/bde/linux/kernel $@
	$(MAKE) -C $(SDK)/systems/bde/linux/user/kernel $@
	$(MAKE) -C $(SDK)/systems/linux/kernel/modules \
		subdirs="shared $(knet_subdirs)" \
		override-target=linux-$(platform) $@
	$(RM) $(KERNEL_BDE) $(USER_BDE)
	$(RM) $(BCM_KNET) $(KNET_CB) $(GENL_PACKET) $(BCM_GENL)
	$(RM) $(KERN_BLDROOT)/$(KERNEL_BDE_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(USER_BDE_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(BCM_KNET_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(KNET_CB_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(GENL_PACKET_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(BCM_GENL_LOCAL)
	$(RM) $(KERN_BLDROOT)/$(BCM_PTP_LOCAL)
	$(RM) $(LOCAL_TARGETS)

distclean:: clean

.PHONY: variable

#
# Echo variable values used for configuration
# usage: make VAR=CC variable
#
variable::
	@echo $($(VAR))

else
ifndef DELIVERY
$(error DELIVERY required)
endif

DELIVERABLES := $(KERNEL_BDE) $(USER_BDE)
# 1= source
define DELIVERY_RULE
_DEL_TARG += $(DELIVERY)/$(notdir $(1))
$(DELIVERY)/$(notdir $(1)) : $(1)
	@mkdir -p $(DELIVERY)
ifeq ($(filter $(notdir $(BCM)) $(notdir $(BCM).dbg),$(notdir $1)),)
	$(OBJCOPY) --strip-debug $$< $$@
else
	$(CP) -p $$< $$@
endif
endef

$(foreach f,$(DELIVERABLES),$(eval $(call DELIVERY_RULE,$(f))))

DELIVER: $(_DEL_TARG)
endif

# Make.config defines remaining phony targets
.PHONY: build kernel_modules DELIVER

