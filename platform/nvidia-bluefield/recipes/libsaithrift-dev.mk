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
# libsaithrift-dev package

SAI_VER = 0.9.4

LIBSAITHRIFT_DEV = libsaithrift-dev_$(SAI_VER)_arm64.deb
$(LIBSAITHRIFT_DEV)_SRC_PATH = $(SRC_PATH)/sonic-sairedis/SAI

$(LIBSAITHRIFT_DEV)_DEPENDS += $(LIBSAIREDIS) $(LIBTHRIFT_0_14_1) $(LIBTHRIFT_0_14_1_DEV) $(PYTHON3_THRIFT_0_14_1) $(THRIFT_0_14_1_COMPILER)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(LIBTHRIFT_0_14_1)

$(LIBSAITHRIFT_DEV)_DEPENDS += $(DPU_SAI)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(DPU_SAI)

$(LIBSAITHRIFT_DEV)_BUILD_ENV = SAITHRIFTV2=true SAITHRIFT_VER=v2 GEN_SAIRPC_OPTS='-e'

SONIC_DPKG_DEBS += $(LIBSAITHRIFT_DEV)

PYTHON_SAITHRIFT = python-saithrift$(SAITHRIFT_VER)_$(SAI_VER)_arm64.deb
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(PYTHON_SAITHRIFT)))

SAISERVER = saiserver$(SAITHRIFT_VER)_$(SAI_VER)_arm64.deb
$(SAISERVER)_RDEPENDS += $(LIBSAITHRIFT_DEV)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER)))

SAISERVER_DBG = saiserver$(SAITHRIFT_VER)-dbg_$(SAI_VER)_arm64.deb
$(SAISERVER_DBG)_RDEPENDS += $(SAISERVER)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER_DBG)))
