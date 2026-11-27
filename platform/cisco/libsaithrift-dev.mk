# Copyright (c) 2026 Cisco Systems, Inc. and/or its affiliates.
# SPDX-License-Identifier: Apache-2.0

# libsaithrift-dev package

SAI_VER = 0.9.4

LIBSAITHRIFT_DEV = libsaithrift-dev_$(SAI_VER)_amd64.deb
$(LIBSAITHRIFT_DEV)_SRC_PATH = $(SRC_PATH)/sonic-sairedis/SAI
$(LIBSAITHRIFT_DEV)_DEPENDS += $(CISCO_BUILD_SAI) $(CISCO_BUILD_SAI_DEV) $(LIBNL3) $(LIBNL_GENL3)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(CISCO_BUILD_SAI) $(LIBNL3) $(LIBNL_GENL3)

SONIC_DPKG_DEBS += $(LIBSAITHRIFT_DEV)

PYTHON_SAITHRIFT = python-saithrift_$(SAI_VER)_amd64.deb
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(PYTHON_SAITHRIFT)))

SAISERVER = saiserver_$(SAI_VER)_amd64.deb
$(SAISERVER)_RDEPENDS += $(LIBTHRIFT) $(CISCO_BUILD_SAI) $(LIBSAIVS) $(LIBSAIVS_DEV)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER)))

SAISERVER_DBG = saiserver-dbg_$(SAI_VER)_amd64.deb
$(SAISERVER_DBG)_RDEPENDS += $(SAISERVER)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER_DBG)))

# Use the same selected SAI as the non-RPC build.
$(LIBSAIREDIS)_DEPENDS += $(CISCO_BUILD_SAI)
