#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

# sonic nvidia-bluefield installer image

SONIC_BF_IMAGE_BASE = sonic-nvidia-bluefield
$(SONIC_BF_IMAGE_BASE)_MACHINE = nvidia-bluefield

$(SONIC_BF_IMAGE_BASE)_DEPENDS += $(MLXBF_BOOTCTL_DEB) \
                                   $(BOOTIMAGES) \
                                   $(MLNX_BLUEFIELD_BUILD_SCRIPTS) \
                                   $(MLNX_TOOLS) \
                                   $(OFED_KERNEL_UTILS) \
                                   $(KERNEL_MFT) \
                                   $(MLNX_IPROUTE2)

# Install the packages during the build_debian phase
$(SONIC_BF_IMAGE_BASE)_INSTALLS += $(SYSTEMD_SONIC_GENERATOR) \
                                    $(KERNEL_MFT) \
                                    $(MFT_OEM) \
                                    $(MFT) \
                                    $(BOOTIMAGES) \
                                    $(MLXBF_BOOTCTL_DEB) \
                                    $(MLNX_BLUEFIELD_BUILD_SCRIPTS) \
                                    $(MLX_OPENIPMI_DEB) \
                                    $(MLX_OPENIPMI_SERVER_DEB) \
                                    $(BF_PLATFORM_MODULE) \
                                    $(OFED_KERNEL) \
                                    $(MLNX_TOOLS) \
                                    $(OFED_KERNEL_UTILS) \
                                    $(MLNX_IPROUTE2)

DISABLED_DOCKERS = $(DOCKER_SFLOW) $(DOCKER_MGMT_FRAMEWORK) $(DOCKER_NAT) $(DOCKER_TEAMD) $(DOCKER_ROUTER_ADVERTISER) $(DOCKER_MUX) $(DOCKER_SNMP) $(DOCKER_LLDP) $(DOCKER_RESTAPI)
DISABLED_PACKAGES_LOCAL = $(DOCKER_DHCP_RELAY) $(DOCKER_MACSEC)
DISABLED_FEATURE_FLAGS = INCLUDE_SFLOW INCLUDE_MGMT_FRAMEWORK INCLUDE_NAT INCLUDE_MACSEC INCLUDE_TEAMD INCLUDE_ROUTER_ADVERTISER INCLUDE_MUX INCLUDE_RESTAPI
$(info Disabling the following docker images: $(DISABLED_DOCKERS))
$(info Disabling the following packages:  $(DISABLED_PACKAGES_LOCAL))
$(info Disabling the following feature flags:  $(DISABLED_FEATURE_FLAGS))

SONIC_PACKAGES_LOCAL := $(filter-out $(DISABLED_PACKAGES_LOCAL), $(SONIC_PACKAGES_LOCAL))

$(foreach feature, $(DISABLED_FEATURE_FLAGS), $(eval override $(feature)=n ))

$(SONIC_BF_IMAGE_BASE)_DOCKERS = $(filter-out $(DISABLED_DOCKERS), $(SONIC_INSTALL_DOCKER_IMAGES))
$(SONIC_BF_IMAGE_BASE)_FILES = $(BF_FW_FILES) $(COMPONENT_VERSIONS_FILE)

# The traditional *.bin image. Works for sonic-sonic upgrade.
SONIC_BF_IMAGE_BIN = $(SONIC_BF_IMAGE_BASE).bin
$(SONIC_BF_IMAGE_BIN)_IMAGE_TYPE = onie
$(SONIC_BF_IMAGE_BIN)_MACHINE = $($(SONIC_BF_IMAGE_BASE)_MACHINE)
$(SONIC_BF_IMAGE_BIN)_INSTALLS += $($(SONIC_BF_IMAGE_BASE)_INSTALLS)
$(SONIC_BF_IMAGE_BIN)_DEPENDS += $($(SONIC_BF_IMAGE_BASE)_DEPENDS)
$(SONIC_BF_IMAGE_BIN)_DOCKERS += $($(SONIC_BF_IMAGE_BASE)_DOCKERS)
$(SONIC_BF_IMAGE_BIN)_LAZY_INSTALLS += $($(SONIC_BF_IMAGE_BASE)_LAZY_INSTALLS)
$(SONIC_BF_IMAGE_BIN)_FILES += $($(SONIC_BF_IMAGE_BASE)_FILES)

# BFB (Bluefield BootStream) style image
SONIC_BF_IMAGE_BFB = $(SONIC_BF_IMAGE_BASE).bfb
$(SONIC_BF_IMAGE_BFB)_IMAGE_TYPE = bfb
$(SONIC_BF_IMAGE_BFB)_MACHINE = $($(SONIC_BF_IMAGE_BASE)_MACHINE)
$(SONIC_BF_IMAGE_BFB)_INSTALLS += $($(SONIC_BF_IMAGE_BASE)_INSTALLS)
$(SONIC_BF_IMAGE_BFB)_DEPENDS += $($(SONIC_BF_IMAGE_BASE)_DEPENDS) $(MFT) $(MFT_OEM) $(KERNEL_MFT)
$(SONIC_BF_IMAGE_BFB)_DOCKERS += $($(SONIC_BF_IMAGE_BASE)_DOCKERS)
$(SONIC_BF_IMAGE_BFB)_LAZY_INSTALLS += $($(SONIC_BF_IMAGE_BASE)_LAZY_INSTALLS)
$(SONIC_BF_IMAGE_BFB)_FILES += $($(SONIC_BF_IMAGE_BASE)_FILES)

SONIC_INSTALLERS += $(SONIC_BF_IMAGE_BIN) $(SONIC_BF_IMAGE_BFB)
