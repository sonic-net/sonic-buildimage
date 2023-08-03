#
# Copyright (c) 2016-2023 NVIDIA CORPORATION & AFFILIATES.
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
# docker image for mlnx syncd

DOCKER_SYNCD_PLATFORM_CODE = mlnx
include $(PLATFORM_PATH)/../template/docker-syncd-bullseye.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD) $(PYTHON_SDK_API) $(MFT) $(MFT_FWTRACE_CFG) $(IPROUTE2_MLNX)

ifeq ($(ENABLE_ASAN), y)
$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD_DBG)
endif

$(DOCKER_SYNCD_BASE)_FILES += $(ISSU_VERSION_FILE)

$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

ifeq ($(SDK_FROM_SRC), y)
$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(MLNX_SDK_DBG_DEBS) $(MLNX_SAI_DBGSYM)
endif

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
