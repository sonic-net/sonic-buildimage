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
MLNX_SDK_VERSION = 4.6.1062
MLNX_SDK_ISSU_VERSION = 101

MLNX_SDK_DRIVERS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers
MLNX_ASSETS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers-SONiC-Bins
MLNX_SDK_ASSETS_RELEASE_TAG = sdk-$(MLNX_SDK_VERSION)-$(BLDENV)-$(CONFIGURED_ARCH)
MLNX_SDK_ASSETS_URL = $(MLNX_ASSETS_GITHUB_URL)/releases/download/$(MLNX_SDK_ASSETS_RELEASE_TAG)
MLNX_SDK_DEB_VERSION = $(subst -,.,$(subst _,.,$(MLNX_SDK_VERSION)))

# Place here URL where SDK sources exist
MLNX_SDK_SOURCE_BASE_URL =

ifneq ($(MLNX_SDK_SOURCE_BASE_URL), )
SDK_FROM_SRC = y
else
SDK_FROM_SRC = n
endif

export MLNX_SDK_SOURCE_BASE_URL MLNX_SDK_VERSION MLNX_SDK_ISSU_VERSION MLNX_SDK_DEB_VERSION MLNX_ASSETS_GITHUB_URL MLNX_SDK_DRIVERS_GITHUB_URL

MLNX_SDK_RDEBS += $(APPLIBS) $(SX_COMPLIB) $(SX_EXAMPLES) \
                  $(SX_GEN_UTILS) $(SXD_LIBS) $(WJH_LIBS) $(SX_ACL_HELPER) \
                  $(SX_HASH_CALC) $(SX_OBJ_DESC_LIB)

MLNX_SDK_DEBS += $(APPLIBS_DEV) $(SX_COMPLIB_DEV) \
                 $(SX_COMPLIB_DEV_STATIC) $(SX_EXAMPLES_DEV) $(SX_GEN_UTILS_DEV) \
                 $(SXD_LIBS_DEV) $(SXD_LIBS_DEV_STATIC) $(WJH_LIBS_DEV) $(SX_ACL_HELPER_DEV) \
                 $(SX_HASH_CALC) $(SX_OBJ_DESC_LIB_DEV)

MLNX_SDK_DBG_DEBS += $(APPLIBS_DBGSYM) $(SX_COMPLIB_DBGSYM) \
         $(SX_EXAMPLES_DBGSYM) $(SX_GEN_UTILS_DBGSYM) \
         $(SXD_LIBS_DBGSYM) $(WJH_LIBS_DBGSYM) $(SX_ACL_HELPER_DBGSYM) \
         $(SX_HASH_CALC_DBGSYM) $(SX_OBJ_DESC_LIB_DBGSYM)

APPLIBS = applibs_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(APPLIBS)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/applibs
$(APPLIBS)_DEPENDS += $(SX_COMPLIB_DEV) $(SX_GEN_UTILS_DEV) $(SXD_LIBS_DEV) $(LIBNL3_DEV) $(LIBNL_GENL3_DEV)
$(APPLIBS)_RDEPENDS += $(SX_COMPLIB) $(SX_GEN_UTILS) $(SXD_LIBS) $(LIBNL3) $(LIBNL_GENL3)
APPLIBS_DEV = applibs-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(APPLIBS),$(APPLIBS_DEV)))
APPLIBS_DBGSYM = applibs-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(APPLIBS),$(APPLIBS_DBGSYM)))
endif

SX_COMPLIB = sx-complib_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_COMPLIB)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-complib
SX_COMPLIB_DEV = sx-complib-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_COMPLIB),$(SX_COMPLIB_DEV)))
SX_COMPLIB_DBGSYM = sx-complib-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_COMPLIB),$(SX_COMPLIB_DBGSYM)))
endif

SX_EXAMPLES = sx-examples_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_EXAMPLES)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-examples
$(SX_EXAMPLES)_DEPENDS += $(APPLIBS_DEV) $(SXD_LIBS_DEV)
$(SX_EXAMPLES)_RDEPENDS += $(APPLIBS) $(SXD_LIBS)
SX_EXAMPLES_DEV = sx-examples-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_EXAMPLES),$(SX_EXAMPLES_DEV)))
SX_EXAMPLES_DBGSYM = sx-examples-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_EXAMPLES),$(SX_EXAMPLES_DBGSYM)))
endif

SX_GEN_UTILS = sx-gen-utils_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_GEN_UTILS)_SRC_PATH += $(PLATFORM_PATH)/sdk-src/sx-gen-utils
$(SX_GEN_UTILS)_DEPENDS += $(SX_COMPLIB_DEV)
$(SX_GEN_UTILS)_RDEPENDS += $(SX_COMPLIB)
SX_GEN_UTILS_DEV = sx-gen-utils-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_GEN_UTILS),$(SX_GEN_UTILS_DEV)))
SX_GEN_UTILS_DBGSYM = sx-gen-utils-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_GEN_UTILS),$(SX_GEN_UTILS_DBGSYM)))
endif

SXD_LIBS = sxd-libs_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SXD_LIBS)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sxd-libs
$(SXD_LIBS)_DEPENDS += $(SX_COMPLIB_DEV) $(SX_GEN_UTILS_DEV)
SXD_LIBS_DEV = sxd-libs-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SXD_LIBS),$(SXD_LIBS_DEV)))
SXD_LIBS_DBGSYM = sxd-libs-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SXD_LIBS),$(SXD_LIBS_DBGSYM)))
endif

#packages that are required for runtime only
PYTHON_SDK_API = python-sdk-api_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(PYTHON_SDK_API)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/python-sdk-api
$(PYTHON_SDK_API)_DEPENDS += $(APPLIBS_DEV) $(SXD_LIBS_DEV) $(SWIG)
$(PYTHON_SDK_API)_RDEPENDS += $(APPLIBS) $(SXD_LIBS)
PYTHON_SDK_API_DBGSYM = python-sdk-api-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(PYTHON_SDK_API),$(PYTHON_SDK_API_DBGSYM)))
endif

SX_ACL_HELPER = sx-acl-helper_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_ACL_HELPER)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-acl-helper
$(SX_ACL_HELPER)_DEPENDS += $(SX_COMPLIB_DEV) $(SXD_LIBS_DEV) $(APPLIBS_DEV)
$(SX_ACL_HELPER)_RDEPENDS += $(SX_COMPLIB) $(PYTHON_SDK_API)
SX_ACL_HELPER_DEV = sx-acl-helper-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_ACL_HELPER),$(SX_ACL_HELPER_DEV)))
SX_ACL_HELPER_DBGSYM = sx-acl-helper-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_ACL_HELPER),$(SX_ACL_HELPER_DBGSYM)))
endif

SX_OBJ_DESC_LIB = sx-obj-desc-lib_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_OBJ_DESC_LIB)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-obj-desc-lib
$(SX_OBJ_DESC_LIB)_DEPENDS += $(APPLIBS_DEV) $(SX_COMPLIB_DEV) $(SXD_LIBS_DEV)
$(SX_OBJ_DESC_LIB)_RDEPENDS += $(APPLIBS) $(SX_COMPLIB) $(PYTHON_SDK_API)
SX_OBJ_DESC_LIB_DEV = sx-obj-desc-lib-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_OBJ_DESC_LIB),$(SX_OBJ_DESC_LIB_DEV)))
SX_OBJ_DESC_LIB_DBGSYM = sx-obj-desc-lib-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_OBJ_DESC_LIB),$(SX_OBJ_DESC_LIB_DBGSYM)))
endif

WJH_LIBS = wjh-libs_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(WJH_LIBS)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/wjh-libs
$(WJH_LIBS)_DEPENDS += $(SX_COMPLIB_DEV) $(SXD_LIBS_DEV) $(APPLIBS_DEV) $(SX_ACL_HELPER_DEV)
$(WJH_LIBS)_RDEPENDS += $(SX_COMPLIB) $(PYTHON_SDK_API) $(SX_ACL_HELPER)
WJH_LIBS_DEV = wjh-libs-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(WJH_LIBS),$(WJH_LIBS_DEV)))
WJH_LIBS_DBGSYM = wjh-libs-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(WJH_LIBS),$(WJH_LIBS_DBGSYM)))
endif

SX_HASH_CALC = sx-hash-calc_1.mlnx.$(MLNX_SDK_DEB_VERSION)_amd64.deb
$(SX_HASH_CALC)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-hash-calc
$(SX_HASH_CALC)_DEPENDS += $(SX_COMPLIB_DEV) $(SXD_LIBS_DEV) $(APPLIBS_DEV) $(SX_GEN_UTILS_DEV)
$(SX_HASH_CALC)_RDEPENDS += $(SX_COMPLIB) $(SXD_LIBS) $(APPLIBS)
SX_HASH_CALC_DBGSYM = sx-hash-calc-dbgsym_1.mlnx.$(MLNX_SDK_DEB_VERSION)_amd64.deb
ifeq ($(SDK_FROM_SRC),y)
$(eval $(call add_derived_package,$(SX_HASH_CALC),$(SX_HASH_CALC_DBGSYM)))
endif

SX_KERNEL = sx-kernel_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(SX_KERNEL)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(SX_KERNEL)_SRC_PATH = $(PLATFORM_PATH)/sdk-src/sx-kernel
SX_KERNEL_DEV = sx-kernel-dev_1.mlnx.$(MLNX_SDK_DEB_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SX_KERNEL),$(SX_KERNEL_DEV)))

define make_url
	$(1)_URL = $(MLNX_SDK_ASSETS_URL)/$(1)

endef

$(eval $(foreach deb,$(MLNX_SDK_DEBS) $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API),$(call make_url,$(deb))))

SONIC_MAKE_DEBS += $(SX_KERNEL)

ifeq ($(SDK_FROM_SRC), y)
SONIC_MAKE_DEBS += $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API)
else
SONIC_ONLINE_DEBS += $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API)
endif

mlnx-sdk-packages: $(addprefix $(DEBS_PATH)/, $(MLNX_SDK_RDEBS) $(PYTHON_SDK_API) $(SX_KERNEL))

SONIC_PHONY_TARGETS += mlnx-sdk-packages
