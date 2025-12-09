#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2024-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
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

SDK_BASE_PATH = $(PLATFORM_PATH)/sdk-src/sonic-bluefield-packages/bin

# Place here URL where SDK sources exist
SDK_SOURCE_BASE_URL =
SDK_VERSION = 25.7-RC11

SDK_COLLECTX_URL = https://linux.mellanox.com/public/repo/doca/1.5.2/debian12/aarch64/

SDK_GH_BASE_URL = https://github.com/Mellanox/sonic-bluefield-packages/releases/download/dpu-sdk-$(SDK_VERSION)-$(BLDENV)/

# Place here URL where alternate SDK assets exist
SDK_ASSETS_BASE_URL =

# Use alternate assets URL if provided, otherwise use GitHub URL
SDK_ASSETS_URL = $(if $(SDK_ASSETS_BASE_URL),$(SDK_ASSETS_BASE_URL),$(SDK_GH_BASE_URL))

define make_url_sdk
	$(1)_URL="$(SDK_ASSETS_URL)/$(1)"

endef

define get_sdk_version_file_gh
	$(shell wget --quiet $(1) -O $(2))

endef

# Use source build if source URL is provided and no assets URL is set
SDK_FROM_SRC = $(if $(SDK_SOURCE_BASE_URL),$(if $(SDK_ASSETS_BASE_URL),n,y),n)

ifneq ($(SDK_SOURCE_BASE_URL), )
SDK_SOURCE_URL = $(SDK_SOURCE_BASE_URL)/$(subst -,/,$(SDK_VERSION))
SDK_VERSIONS_FILE = $(PLATFORM_PATH)/sdk-src/VERSIONS
$(eval $(call get_sdk_version_file_gh, "$(SDK_SOURCE_URL)/VERSIONS_FOR_SONIC_BUILD", $(SDK_VERSIONS_FILE)))
else
SDK_FROM_SRC = n
SDK_VERSIONS_FILE = $(PLATFORM_PATH)/sdk-src/VERSIONS
$(eval $(call get_sdk_version_file_gh, "$(SDK_GH_BASE_URL)/VERSIONS_FOR_SONIC_BUILD", $(SDK_VERSIONS_FILE)))
endif

export SDK_VERSION SDK_SOURCE_URL

define get_sdk_package_version_short
$(shell $(PLATFORM_PATH)/recipes/get_sdk_package_version.sh -s $(SDK_VERSIONS_FILE) $(1))
endef

define get_sdk_package_version_full
$(shell $(PLATFORM_PATH)/recipes/get_sdk_package_version.sh $(SDK_VERSIONS_FILE) $(1))
endef

SDK_DEBS =
SDK_SRC_TARGETS =
SDK_ONLINE_TARGETS =

# OFED and derived packages

OFED_VER_SHORT = $(call get_sdk_package_version_short,"ofed")
OFED_VER_FULL = $(call get_sdk_package_version_full,"ofed")
OFED_KERNEL_VER_SHORT = $(call get_sdk_package_version_short,"mlnx-ofed-kernel")
OFED_KERNEL_VER_FULL = $(call get_sdk_package_version_full,"mlnx-ofed-kernel")
MLNX_TOOLS_VER = $(call get_sdk_package_version_full,"mlnx-tools")

MLNX_TOOLS = mlnx-tools_$(MLNX_TOOLS_VER)_arm64.deb
$(MLNX_TOOLS)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/ofed

OFED_KERNEL_UTILS = mlnx-ofed-kernel-utils_$(OFED_KERNEL_VER_FULL)-1_arm64.deb
$(OFED_KERNEL_UTILS)_DEPENDS = $(MLNX_TOOLS)
OFED_KERNEL_DKMS = mlnx-ofed-kernel-dkms_$(OFED_KERNEL_VER_SHORT)-1_all.deb
$(OFED_KERNEL_DKMS)_DEPENDS = $(OFED_KERNEL_UTILS)

OFED_KERNEL = mlnx-ofed-kernel-modules-$(KVERSION)_$(OFED_KERNEL_VER_SHORT)_$(BUILD_ARCH).deb
$(OFED_KERNEL)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/ofed
$(OFED_KERNEL)_DEPENDS = $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON) $(OFED_KERNEL_DKMS)

ifeq ($(SDK_FROM_SRC), y)
$(eval $(call add_derived_package,$(MLNX_TOOLS),$(OFED_KERNEL_UTILS)))
$(eval $(call add_derived_package,$(MLNX_TOOLS),$(OFED_KERNEL_DKMS)))
else
SDK_ONLINE_TARGETS += $(OFED_KERNEL_UTILS) $(OFED_KERNEL_DKMS)
endif

export OFED_VER_SHORT OFED_VER_FULL OFED_KERNEL OFED_KERNEL_UTILS OFED_KERNEL_VER_FULL MLNX_TOOLS OFED_KERNEL_DKMS

SDK_DEBS += $(MLNX_TOOLS) $(OFED_KERNEL_UTILS)
SDK_SRC_TARGETS += $(MLNX_TOOLS)

# MLNX iproute2
MLNX_IPROUTE2_VER = $(call get_sdk_package_version_full,"mlnx-iproute2")

MLNX_IPROUTE2 = mlnx-iproute2_$(MLNX_IPROUTE2_VER)_arm64.deb
$(MLNX_IPROUTE2)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/mlnx-iproute2

export MLNX_IPROUTE2_VER MLNX_IPROUTE2

MLNX_IPROUTE2_DERIVED_DEBS = 

export MLNX_IPROUTE2_DERIVED_DEBS

SDK_DEBS += $(MLNX_IPROUTE2) $(MLNX_IPROUTE2_DERIVED_DEBS)
SDK_SRC_TARGETS += $(MLNX_IPROUTE2)

# RDMA and derived packages

RDMA_CORE_VER = $(call get_sdk_package_version_full,"rdma-core")
RDMA_CORE = rdma-core_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
$(RDMA_CORE)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/rdma
$(RDMA_CORE)_RDEPENDS = $(LIBNL3)
$(RDMA_CORE)_DEPENDS = $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV)

IB_VERBS_PROV = ibverbs-providers_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
$(IB_VERBS_PROV)_DEPENDS = $(IB_VERBS) $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV)

IB_VERBS = libibverbs1_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
$(IB_VERBS)_DEPENDS = $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV)
$(IB_VERBS)_RDEPENDS = $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV)
IB_VERBS_DEV = libibverbs-dev_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
$(IB_VERBS_DEV)_DEPENDS = $(LIBNL3_DEV) $(LIBNL_ROUTE3_DEV) $(IB_VERBS) $(IB_VERBS_PROV)

IB_UMAD = libibumad3_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
IB_UMAD_DEV = libibumad-dev_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb

RDMACM = librdmacm1_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
RDMACM_DEV = librdmacm-dev_${RDMA_CORE_VER}_${CONFIGURED_ARCH}.deb
$(RDMACM_DEV)_DEPENDS = $(RDMACM) $(IB_VERBS_DEV)

export RDMA_CORE
export IB_VERBS IB_VERBS_DEV
export IB_VERBS_PROV
export IB_UMAD IB_UMAD_DEV
export RDMACM RDMACM_DEV

RDMA_CORE_DERIVED_DEBS = \
		$(IB_VERBS) \
		$(IB_VERBS_DEV) \
		$(IB_VERBS_PROV) \
		$(IB_UMAD) \
		$(IB_UMAD_DEV) \
		$(RDMACM) \
		$(RDMACM_DEV)

export RDMA_CORE_DERIVED_DEBS

SDK_SRC_TARGETS += $(RDMA_CORE)

ifeq ($(SDK_FROM_SRC), y)
$(eval $(call add_derived_package,$(RDMA_CORE),$(IB_VERBS)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(IB_VERBS_PROV)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(IB_VERBS_DEV)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(IB_UMAD)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(IB_UMAD_DEV)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(RDMACM)))
$(eval $(call add_derived_package,$(RDMA_CORE),$(RDMACM_DEV)))
else
SDK_ONLINE_TARGETS += $(RDMA_CORE_DERIVED_DEBS)
endif

# DPDK and derived packages

DPDK_VER = $(call get_sdk_package_version_full,"dpdk")

DPDK = mlnx-dpdk_${DPDK_VER}_${CONFIGURED_ARCH}.deb
$(DPDK)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/dpdk
$(DPDK)_RDEPENDS = $(IB_VERBS) $(IB_VERBS_PROV) $(IB_VERBS_DEV)

DPDK_DEV = mlnx-dpdk-dev_${DPDK_VER}_${CONFIGURED_ARCH}.deb
$(DPDK)_DEPENDS = $(RDMA_CORE) $(IB_VERBS) $(IB_VERBS_PROV) $(IB_VERBS_DEV)
$(DPDK_DEV)_RDEPENDS = $(DPDK)

$(eval $(call add_derived_package,$(DPDK),$(DPDK_DEV)))

export DPDK DPDK_DEV

DPDK_DERIVED_DEBS = $(DPDK_DEV)
export DPDK_DERIVED_DEBS

SDK_DEBS += $(DPDK) $(DPDK_DERIVED_DEBS)
SDK_SRC_TARGETS += $(DPDK)

# RXP compiler and derived packages

RXPCOMPILER_VER = $(call get_sdk_package_version_full,"rxp-tools")

RXPCOMPILER = rxp-compiler_$(RXPCOMPILER_VER)_arm64.deb
$(RXPCOMPILER)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/rxp-compiler
LIBRXPCOMPILER_DEV = librxpcompiler-dev_$(RXPCOMPILER_VER)_arm64.deb

$(eval $(call add_derived_package,$(RXPCOMPILER),$(LIBRXPCOMPILER_DEV)))

export RXPCOMPILER LIBRXPCOMPILER_DEV

RXPCOMPILER_DERIVED_DEBS = $(LIBRXPCOMPILER_DEV)
export RXPCOMPILER_DERIVED_DEBS

SDK_DEBS += $(RXPCOMPILER) $(RXPCOMPILER_DERIVED_DEBS)
SDK_SRC_TARGETS += $(RXPCOMPILER)

# GRPC and derived packages

LIBGRPC_VER = $(call get_sdk_package_version_full,"grpc")

LIBGRPC_DEV = libgrpc-dev_$(LIBGRPC_VER)_arm64.deb
$(LIBGRPC_DEV)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/grpc

export LIBGRPC_DEV LIBGRPC_VER

SDK_DEBS += $(LIBGRPC_DEV)
SDK_SRC_TARGETS += $(LIBGRPC_DEV)

# DOCA and derived packages

DOCA_VERSION = $(call get_sdk_package_version_full,"doca")
DOCA_DEB_VERSION = $(DOCA_VERSION)-1

DOCA_COMMON = doca-sdk-common_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_COMMON)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/doca
$(DOCA_COMMON)_RDEPENDS = $(DPDK) $(RXPCOMPILER) $(LIBRXPCOMPILER_DEV) $(LIBGRPC_DEV) $(LIB_NV_HWS)
$(DOCA_COMMON)_DEPENDS = $(RXPCOMPILER) $(LIBRXPCOMPILER_DEV) $(DPDK_DEV) $(LIBGRPC_DEV) $(LIB_NV_HWS_DEV)
DOCA_COMMON_DEV = libdoca-sdk-common-dev_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_COMMON_DEV)_DEPENDS = $(DOCA_COMMON)

SDK_SRC_TARGETS += $(DOCA_COMMON)

DOCA_DEV_DEBS += $(DOCA_COMMON_DEV)
export DOCA_COMMON DOCA_COMMON_DEV

DOCA_ARGP = doca-sdk-argp_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_ARGP)_DEPENDS += $(DOCA_COMMON)
DOCA_ARGP_DEV = libdoca-sdk-argp-dev_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_ARGP_DEV)_DEPENDS = $(DOCA_ARGP) $(DOCA_COMMON_DEV)

DOCA_DEBS += $(DOCA_ARGP)
DOCA_DEV_DEBS += $(DOCA_ARGP_DEV)
 export DOCA_ARGP DOCA_ARGP_DEV

DOCA_DPDK_BRIDGE = doca-sdk-dpdk-bridge_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_DPDK_BRIDGE)_DEPENDS += $(DOCA_COMMON)
DOCA_DPDK_BRIDGE_DEV = libdoca-sdk-dpdk-bridge-dev_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_DPDK_BRIDGE_DEV)_DEPENDS = $(DOCA_DPDK_BRIDGE) $(DOCA_COMMON_DEV)

DOCA_DEBS += $(DOCA_DPDK_BRIDGE)
DOCA_DEV_DEBS += $(DOCA_DPDK_BRIDGE_DEV)
export DOCA_DPDK_BRIDGE DOCA_DPDK_BRIDGE_DEV

DOCA_FLOW = doca-sdk-flow_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_FLOW)_DEPENDS += $(DOCA_COMMON) $(DOCA_DPDK_BRIDGE)
DOCA_FLOW_DEV = libdoca-sdk-flow-dev_${DOCA_DEB_VERSION}_${CONFIGURED_ARCH}.deb
$(DOCA_FLOW_DEV)_DEPENDS = $(DOCA_FLOW) $(DOCA_DPDK_BRIDGE_DEV)

DOCA_DEBS += $(DOCA_FLOW)
DOCA_DEV_DEBS += $(DOCA_FLOW_DEV)

export DOCA_FLOW DOCA_FLOW_DEV
export DOCA_DEBS DOCA_DEV_DEBS

SDK_DEBS += $(DOCA_DEBS) $(DOCA_DEV_DEBS)

ifeq ($(SDK_FROM_SRC), y)
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_COMMON_DEV)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_ARGP)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_ARGP_DEV)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_DPDK_BRIDGE)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_DPDK_BRIDGE_DEV)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_FLOW)))
$(eval $(call add_derived_package,$(DOCA_COMMON),$(DOCA_FLOW_DEV)))
else
SONIC_ONLINE_DEBS += $(DOCA_DEBS) $(DOCA_DEV_DEBS)
endif

# hw-steering packages, needed for doca-flow runtime
NV_HWS_VERSION = $(call get_sdk_package_version_full,"nv_hws")

LIB_NV_HWS = libnvhws1_${NV_HWS_VERSION}_${CONFIGURED_ARCH}.deb
$(LIB_NV_HWS)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/nv_hws
$(LIB_NV_HWS)_DEPENDS = $(IB_VERBS_DEV)

LIB_NV_HWS_DEV = libnvhws-dev_${NV_HWS_VERSION}_${CONFIGURED_ARCH}.deb
$(LIB_NV_HWS_DEV)_DEPENDS = $(LIB_NV_HWS)

ifeq ($(SDK_FROM_SRC), y)
$(eval $(call add_derived_package,$(LIB_NV_HWS),$(LIB_NV_HWS_DEV)))
else
SONIC_ONLINE_DEBS += $(LIB_NV_HWS) $(LIB_NV_HWS_DEV)
endif

SDK_SRC_TARGETS += $(LIB_NV_HWS)
SDK_DEBS += $(LIB_NV_HWS) $(LIB_NV_HWS_DEV)

export LIB_NV_HWS LIB_NV_HWS_DEV

# SDN Appliance

SDN_APPL_VER=$(call get_sdk_package_version_full,"nasa")
SDN_APPL = sdn-appliance_${SDN_APPL_VER}_${CONFIGURED_ARCH}.deb
$(SDN_APPL)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sdn
$(SDN_APPL)_RDEPENDS = $(DOCA_COMMON) $(DOCA_DEBS) $(MLNX_TOOLS) $(OFED_KERNEL_UTILS) $(MLNX_IPROUTE2)
$(SDN_APPL)_DEPENDS = $(DOCA_COMMON) $(DOCA_DEBS) $(DOCA_DEV_DEBS) $(DPDK_DEV) $(MLNX_TOOLS) $(OFED_KERNEL_UTILS)

export SDN_APPL

SDN_APPL_DERIVED_DEBS =
export SDN_APPL_DERIVED_DEBS

SDK_DEBS += $(SDN_APPL) $(SDN_APPL_DERIVED_DEBS)
SDK_SRC_TARGETS += $(SDN_APPL)

ifeq ($(SDK_FROM_SRC), y)
SONIC_MAKE_DEBS += $(SDK_SRC_TARGETS)
SONIC_ONLINE_DEBS += $(SDK_ONLINE_TARGETS)
else
$(eval $(foreach deb,$(SDK_SRC_TARGETS) $(SDK_ONLINE_TARGETS) $(SDK_DEBS),$(call make_url_sdk,$(deb))))
SONIC_ONLINE_DEBS += $(SDK_SRC_TARGETS) $(SDK_ONLINE_TARGETS)
endif

# Always build the platform modules, don't save the deb file to artifactory
SONIC_MAKE_DEBS += $(OFED_KERNEL)

SDK_PACKAGES = $(OFED_KERNEL_UTILS) $(MLNX_IPROUTE2) $(SDK_ONLINE_TARGETS) $(SDK_SRC_TARGETS)

sdk-packages: $(addprefix $(DEBS_PATH)/, $(SDK_PACKAGES))

SONIC_PHONY_TARGETS += sdk-packages
