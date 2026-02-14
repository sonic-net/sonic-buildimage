#
# Copyright (c) 2017-2021 NVIDIA CORPORATION & AFFILIATES.
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
# docker image for mlnx syncd with rpc

DOCKER_SYNCD_MLNX_RPC = docker-syncd-mlnx-rpc.gz
$(DOCKER_SYNCD_MLNX_RPC)_PATH = $(PLATFORM_PATH)/docker-syncd-mlnx-rpc
$(DOCKER_SYNCD_MLNX_RPC)_PYTHON_WHEELS += $(PTF_PY3)
$(DOCKER_SYNCD_MLNX_RPC)_DEPENDS += $(SYNCD_RPC)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SYNCD_MLNX_RPC)_DEPENDS += $(SYNCD_RPC_DBG) \
                                    $(LIBSWSSCOMMON_DBG) \
                                    $(LIBSAIMETADATA_DBG) \
                                    $(LIBSAIREDIS_DBG)
endif

$(DOCKER_SYNCD_MLNX_RPC)_PYTHON_DEBS += $(MLNX_SFPD)
$(DOCKER_SYNCD_MLNX_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_MLNX_RPC)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_SYNCD_MLNX_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_MLNX_RPC)
endif

$(DOCKER_SYNCD_MLNX_RPC)_CONTAINER_NAME = syncd
$(DOCKER_SYNCD_MLNX_RPC)_VERSION = 1.0.0+rpc
$(DOCKER_SYNCD_MLNX_RPC)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_MLNX_RPC)_RUN_OPT += --privileged -t
$(DOCKER_SYNCD_MLNX_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_MLNX_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SYNCD_MLNX_RPC)_RUN_OPT += -v /host/warmboot:/var/warmboot
