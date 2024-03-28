#
# Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES.
# Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -ex

MLXBF3_PINCTRL = pinctrl-mlxbf3-${MLXBF3_PINCTRL_DRIVER_VERSION}
MODULE_DIR := $(shell pwd)/tmp/${MLXBF3_PINCTRL}
GET_PACKAGE := $(shell pwd)/../recipes/get-soc-package.sh

$(addprefix $(DEST)/, $(MLXBF3_PINCTRL_DRIVER)): $(DEST)/% :
	rm -rf ./tmp/
	mkdir ./tmp/
	pushd ./tmp/

	${GET_PACKAGE} ${BSD_BASE_SOURCE_URL} ${MLXBF3_PINCTRL}

	pushd ${MLXBF3_PINCTRL}

	patch -p1 < ../../pinctrl-mlxbf3.c.patch

	# Build the package
	make -C /lib/modules/$(KVERSION)/build M=$(MODULE_DIR)

	mv ${MLXBF3_PINCTRL_DRIVER} $(DEST)/
	popd
