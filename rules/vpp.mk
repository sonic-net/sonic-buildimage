# libvpp package

VPP_VERSION_BASE = 2506
VPP_VERSION = $(VPP_VERSION_BASE)-0.1
VPP_VERSION_SONIC = $(VPP_VERSION)+b1sonic1
VPP_SRC_PATH = platform/vpp/vppbld

export VPP_VERSION_SONIC
export VPP_SRC_PATH

VPPINFRA = libvppinfra_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPPINFRA)_SRC_PATH = $(VPP_SRC_PATH)
$(VPPINFRA)_DEPENDS += $(LIBNL3) $(LIBNL3_DEV) $(LIBNL_ROUTE3) $(LIBNL_ROUTE3_DEV)
SONIC_MAKE_DEBS += $(VPPINFRA)

VPP_MAIN = vpp_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPP_MAIN)_DEPENDS += $(VPPINFRA)
$(VPP_MAIN)_RDEPENDS += $(VPPINFRA)
$(eval $(call add_derived_package,$(VPPINFRA),$(VPP_MAIN)))

LIBNUMA := $(shell apt-get download --print-uris libnuma1 | cut -d " " -f2)
$(LIBNUMA)_URL := $(shell apt-get download --print-uris libnuma1 | cut -d " " -f1)
SONIC_ONLINE_DEBS += $(LIBNUMA)

LIBMBEDCRYPTO3 := $(shell apt-get download --print-uris libmbedcrypto3 | cut -d " " -f2)
$(LIBMBEDCRYPTO3)_URL := $(shell apt-get download --print-uris libmbedcrypto3 | cut -d " " -f1)
SONIC_ONLINE_DEBS += $(LIBMBEDCRYPTO3)


LIBMBEDX509 := $(shell apt-get download --print-uris libmbedx509-0 | cut -d " " -f2)
$(LIBMBEDX509)_URL := $(shell apt-get download --print-uris libmbedx509-0 | cut -d " " -f1)
SONIC_ONLINE_DEBS += $(LIBMBEDX509)
$(LIBMBEDX509)_DEPENDS += $(LIBMBEDCRYPTO3)
$(LIBMBEDX509)_RDEPENDS += $(LIBMBEDCRYPTO3)


LIBMBEDTLS12 := $(shell apt-get download --print-uris libmbedtls12 | cut -d " " -f2)
$(LIBMBEDTLS12)_URL := $(shell apt-get download --print-uris libmbedtls12 | cut -d " " -f1)
SONIC_ONLINE_DEBS += $(LIBMBEDTLS12)
$(LIBMBEDTLS12)_DEPENDS += $(LIBMBEDCRYPTO3) $(LIBMBEDX509)
$(LIBMBEDTLS12)_RDEPENDS += $(LIBMBEDCRYPTO3) $(LIBMBEDX509)

VPP_PLUGIN_CORE = vpp-plugin-core_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPP_PLUGIN_CORE)_DEPENDS += $(LIBMBEDCRYPTO3) $(LIBMBEDX509) $(LIBMBEDTLS12) $(VPP_MAIN)
$(VPP_PLUGIN_CORE)_RDEPENDS += $(LIBMBEDCRYPTO3) $(LIBMBEDX509) $(LIBMBEDTLS12) $(VPP_MAIN)
$(eval $(call add_derived_package,$(VPPINFRA),$(VPP_PLUGIN_CORE)))

VPP_PLUGIN_DPDK = vpp-plugin-dpdk_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(VPPINFRA),$(VPP_PLUGIN_DPDK)))
$(VPP_PLUGIN_DPDK)_DEPENDS += $(LIBNUMA) $(VPP_MAIN)
$(VPP_PLUGIN_DPDK)_RDEPENDS += $(LIBNUMA) $(VPP_MAIN)

VPPINFRA_DEV = libvppinfra-dev_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPPINFRA_DEV)_DEPENDS += $(VPPINFRA)
$(eval $(call add_derived_package,$(VPPINFRA),$(VPPINFRA_DEV)))

VPP_PLUGIN_DEV = vpp-plugin-devtools_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPP_PLUGIN_DEV)_DEPENDS += $(VPP_MAIN) $(VPPINFRA)
$(VPP_PLUGIN_DEV)_RDEPENDS += $(VPP_MAIN) $(VPPINFRA)
$(eval $(call add_derived_package,$(VPPINFRA),$(VPP_PLUGIN_DEV)))

VPP_DEV = vpp-dev_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(VPP_DEV)_DEPENDS += $(VPPINFRA_DEV)
$(VPP_DEV)_RDEPENDS += $(VPPINFRA_DEV)
$(eval $(call add_derived_package,$(VPPINFRA),$(VPP_DEV)))

VPPDBG = vpp-dbg_$(VPP_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(VPPINFRA),$(VPPDBG)))

VPP_VERSION_FILE = vpp_$(VPP_VERSION_SONIC)_commit

export VPP_MAIN VPPINFRA VPP_PLUGIN_CORE VPP_PLUGIN_DPDK VPP_PLUGIN_DEV VPP_DEV VPPINFRA_DEV VPPDBG VPP_VERSION_FILE
