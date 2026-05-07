#  Copyright Statement:
#  --------------------
#  This software and the information contained therein are protected by
#  copyright and other intellectual property laws and terms herein is
#  confidential. The software may not be copied and the information
#  contained herein may not be used or disclosed except with the written
#  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
#
#  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
#  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
#  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
#  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
#  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
#  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
#  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
#  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
#  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
#  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
#
#  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
#  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
#  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
#  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
#  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
#
#  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
#  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
#  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
#  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
#
################################################################################
################################################################################
KERNEL_MODULE := $(dir $(realpath $(lastword $(MAKEFILE_LIST))))

################################################################################
MAKE                := $(shell which make)
RM                  := rm -rf
MKDIR               := mkdir -p
CP                  := cp
MV                  := mv
TEST_PATH           := test -d

KVERSION            ?= $(shell uname -r)
OS_PATH             := /lib/modules/$(KVERSION)/build
KNET_INC_PATH		:= $(KERNEL_MODULE)/linux/include/
MT_INC_PATH			:= $(KERNEL_MODULE)/linux/include/nb/
BUILD_OUTPUT_DIR    := $(KERNEL_MODULE)/build
MODULE_OUTPUT_DIR   := $(BUILD_OUTPUT_DIR)/module

all: compile install
################################################################################
EXTRA_CFLAGS += -I$(KNET_INC_PATH)
EXTRA_CFLAGS += -I$(MT_INC_PATH)
EXTRA_CFLAGS += -DCLX_EN_NETIF
EXTRA_CFLAGS += -DCLX_EN_DEBUG
EXTRA_CFLAGS += -DCLX_LINUX_USER_MODE
EXTRA_CFLAGS += -DCLX_EN_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_FUNCTION
EXTRA_CFLAGS += -DCLX_EN_COMPILER_SUPPORT_LONG_LONG

ifeq ($(shell uname -m),x86_64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
ifeq ($(shell uname -m),aarch64)
EXTRA_CFLAGS += -DCLX_EN_HOST_64_BIT_LITTLE_ENDIAN
EXTRA_CFLAGS += -DCLX_EN_64BIT_ADDR
else
EXTRA_CFLAGS += -DCLX_EN_HOST_32_BIT_LITTLE_ENDIAN
endif
endif

COMPILE_TIME := $(shell date -u +'%Y-%m-%d_%H:%M:%S-%Z')
EXTRA_CFLAGS += -DKNET_DEV_MAKE_COMPILE_TIME=\"$(COMPILE_TIME)\"
DIR_TO_CHECK_FOR_SDK := ../../../../.git
DIR_TO_CHECK_FOR_KNET := .git
ifneq ($(or $(wildcard $(DIR_TO_CHECK_FOR_SDK)), $(wildcard $(DIR_TO_CHECK_FOR_KNET))),)
GIT_HASH := $(shell git rev-parse --short HEAD 2> /dev/null)
EXTRA_CFLAGS += -DKNET_DEV_MAKE_GIT_HASH=\"$(GIT_HASH)\"
else
EXTRA_CFLAGS += -DKNET_DEV_RELEASED
endif

################################################################################
DEV_MODULE_NAME            := clx_dev

################################################################################
DEV_OBJS_TOTAL             := linux/src/knet_dev.o
DEV_OBJS_TOTAL             += linux/src/knet_pci.o
DEV_OBJS_TOTAL             += linux/src/knet_intr.o
DEV_OBJS_TOTAL             += linux/src/knet_dma.o
DEV_OBJS_TOTAL             += linux/src/knet_buffer.o
DEV_OBJS_TOTAL             += linux/src/knet_drv.o
DEV_OBJS_TOTAL             += linux/src/knet_netif.o
DEV_OBJS_TOTAL             += linux/src/knet_netlink.o

CHIP_OBJS_TOTAL            := linux/src/nb/nb_chip.o
CHIP_OBJS_TOTAL            += linux/src/nb/nb_drv.o


obj-m                      := $(DEV_MODULE_NAME).o
$(DEV_MODULE_NAME)-objs    := $(DEV_OBJS_TOTAL)
$(DEV_MODULE_NAME)-objs    += $(CHIP_OBJS_TOTAL)

KBUILD_EXTRA_SYMBOLS       := $(BUILD_OUTPUT_DIR)/Module.symvers
################################################################################
folder:
	$(TEST_PATH) $(BUILD_OUTPUT_DIR) || $(MKDIR) $(BUILD_OUTPUT_DIR)
	$(TEST_PATH) $(BUILD_OUTPUT_DIR)/src || $(MKDIR) $(BUILD_OUTPUT_DIR)/src

compile:: folder
	touch $(BUILD_OUTPUT_DIR)/Makefile
	touch $(KBUILD_EXTRA_SYMBOLS)
	$(MAKE) -C $(OS_PATH) M=$(BUILD_OUTPUT_DIR) src=$(shell pwd) modules EXTRA_CFLAGS="$(EXTRA_CFLAGS)" KBUILD_EXTRA_SYMBOLS=$(KBUILD_EXTRA_SYMBOLS)

install::
	$(TEST_PATH) $(MODULE_OUTPUT_DIR) || $(MKDIR) $(MODULE_OUTPUT_DIR)
	$(MV) $(BUILD_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko $(MODULE_OUTPUT_DIR)/$(DEV_MODULE_NAME).ko

clean::
	$(RM) $(BUILD_OUTPUT_DIR)

.PHONY: all compile install clean
.NOTPARALLEL: all compile install clean

