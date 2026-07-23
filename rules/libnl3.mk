# libnl3
#
# resolute: install stock Ubuntu libnl3 debs via SONIC_ONLINE_DEBS instead of
# building from source. SONiC's only libnl customization was the nh-id alias
# patch (rtnl_route_get_nh_id -> rtnl_route_get_nhid); those call sites were
# renamed in sonic-swss to the stock spelling, so no fork is needed and the
# official Ubuntu debs (identical package names/version) satisfy every
# downstream *_DEV build dependency. Version pinned for reproducibility.

LIBNL3_VERSION_BASE = 3.12.0
LIBNL3_VERSION = $(LIBNL3_VERSION_BASE)-2
LIBNL3_VERSION_SONIC = $(LIBNL3_VERSION)

export LIBNL3_VERSION_BASE
export LIBNL3_VERSION
export LIBNL3_VERSION_SONIC

LIBNL3_POOL_URL = http://archive.ubuntu.com/ubuntu/pool/main/libn/libnl3

LIBNL3 = libnl-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL3)_URL = $(LIBNL3_POOL_URL)/$(LIBNL3)

LIBNL3_DEV = libnl-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL3_DEV)_URL = $(LIBNL3_POOL_URL)/$(LIBNL3_DEV)

LIBNL_GENL3 = libnl-genl-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_GENL3)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_GENL3)

LIBNL_GENL3_DEV = libnl-genl-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_GENL3_DEV)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_GENL3_DEV)

LIBNL_ROUTE3 = libnl-route-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_ROUTE3)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_ROUTE3)

LIBNL_ROUTE3_DEV = libnl-route-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_ROUTE3_DEV)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_ROUTE3_DEV)

LIBNL_NF3 = libnl-nf-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_NF3)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_NF3)

LIBNL_NF3_DEV = libnl-nf-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_NF3_DEV)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_NF3_DEV)

LIBNL_CLI = libnl-cli-3-200_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_CLI)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_CLI)

LIBNL_CLI_DEV = libnl-cli-3-dev_$(LIBNL3_VERSION_SONIC)_$(CONFIGURED_ARCH).deb
$(LIBNL_CLI_DEV)_URL = $(LIBNL3_POOL_URL)/$(LIBNL_CLI_DEV)

# resolute: declare inter-package install order (mirrors the debs' Debian
# Depends). SONIC_ONLINE_DEBS runs `dpkg -i` per deb via the %-install rule;
# without these a -dev deb can install before its runtime lib (e.g.
# libnl-nf-3-dev before libnl-nf-3-200) and dpkg aborts "... is not installed".
$(LIBNL3_DEV)_DEPENDS       += $(LIBNL3)
$(LIBNL_GENL3)_DEPENDS      += $(LIBNL3)
$(LIBNL_GENL3_DEV)_DEPENDS  += $(LIBNL3_DEV) $(LIBNL_GENL3)
$(LIBNL_ROUTE3)_DEPENDS     += $(LIBNL3)
$(LIBNL_ROUTE3_DEV)_DEPENDS += $(LIBNL3_DEV) $(LIBNL_ROUTE3)
$(LIBNL_NF3)_DEPENDS        += $(LIBNL3) $(LIBNL_ROUTE3)
$(LIBNL_NF3_DEV)_DEPENDS    += $(LIBNL3_DEV) $(LIBNL_NF3) $(LIBNL_ROUTE3_DEV)
$(LIBNL_CLI)_DEPENDS        += $(LIBNL3) $(LIBNL_GENL3) $(LIBNL_ROUTE3) $(LIBNL_NF3)
$(LIBNL_CLI_DEV)_DEPENDS    += $(LIBNL3_DEV) $(LIBNL_CLI) $(LIBNL_GENL3_DEV) $(LIBNL_NF3_DEV) $(LIBNL_ROUTE3_DEV)

SONIC_ONLINE_DEBS += $(LIBNL3) $(LIBNL3_DEV) \
		     $(LIBNL_GENL3) $(LIBNL_GENL3_DEV) \
		     $(LIBNL_ROUTE3) $(LIBNL_ROUTE3_DEV) \
		     $(LIBNL_NF3) $(LIBNL_NF3_DEV) \
		     $(LIBNL_CLI) $(LIBNL_CLI_DEV)
