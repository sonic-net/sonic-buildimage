################################################################################
# Copyright (C) 2019  Nephos, Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of version 2 of the GNU General Public
# License as published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# version 2 along with this program.
################################################################################
BUILD_OUTPUT_DIR    := $(NPS_MODULES_DIR)/build
################################################################################
#OS_PATH             := /lib/modules/$(shell uname -r)/build
OS_PATH             := /lib/modules/$(KVERSION)/build

################################################################################
MAKE                := make
RM                  := rm -rf
MKDIR               := mkdir -p
CP                  := cp
MV                  := mv
TEST_PATH           := test -d
################################################################################
export BUILD_OUTPUT_DIR
export OS_PATH
