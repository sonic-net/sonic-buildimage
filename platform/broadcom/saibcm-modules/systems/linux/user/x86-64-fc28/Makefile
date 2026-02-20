# -*- Makefile -*-
# $Id: Makefile,v 0.1 Broadcom SDK $
#
# $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
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
# This make job requires the following environment variables to be set:
#
# SDK                - path to StrataXGS SDK root directory
#
# Optionally the following environment variables can be set to
# override the default build server configuration:
#
# TOOLS_DIR      - path to build tools (if not in PATH already)
# CROSS_COMPILE  - cross compile tools prefix
# LINUX_INCLUDE  - path to Linux kernel include directory
#

# SHARED_LIBRARY_SUPPORT will enable the shared library creation - update the BINUTILSVER version.
ifneq ($(filter $(SHARED_LIBRARY_SUPPORT) $(COMPILE_OPENNSA_SUPPORT),1),)
# Newer GNU BINUTILS are neeed in order to avoid a known internal bug
# in GCC that results in a linker error when creating a .so library
export BINUTILSVER = 2.27
endif

SDK :=$(shell if [ -n "$$SDK" ] ; then\
    echo $$SDK;\
    else\
    cd $(dir $(lastword $(MAKEFILE_LIST))); while /usr/bin/test ! -e RELEASE ; do \
    dir=`cd ../;pwd`;            \
    if [ "$$dir" = "/" ] ; then        \
       echo Cannot find SDK in $(lastword $(MAKEFILE_LIST)) 1>&2;        \
       exit 1;                \
    fi ;                    \
    cd $$dir;                \
    done ;                    \
    pwd;                    \
    fi)

ifeq ($(SDK),)
$(error Please run this in a tree)
endif

export SDK

ifeq (1,$(KERNEL_5_10))
override kernel_version=5_10
else
override kernel_version=4_18
endif
platform=x86-64-fc28

export LINKER_RELAX = 1

# SHARED_LIBRARY_SUPPORT will enable the shared library creation - add a PIC to the compilation flags.
ifneq ($(filter $(SHARED_LIBRARY_SUPPORT) $(COMPILE_OPENNSA_SUPPORT),1),)
ADD_TO_CFLAGS = -frecord-gcc-switches -DSAL_THREAD_STACK_MIN=8388608
export ADD_TO_CFLAGS
endif

include ${SDK}/make/Make.linux
