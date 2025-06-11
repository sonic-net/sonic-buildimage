# bash package
#
# Created to patch plugin support in the bash-package included in Debian-13 (Trixie)
# release.

BASH_VERSION_MAJOR = 5.2.37
BASH_VERSION_FULL = $(BASH_VERSION_MAJOR)-2

export BASH_VERSION_MAJOR BASH_VERSION_FULL

BASH = bash_$(BASH_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(BASH)_SRC_PATH = $(SRC_PATH)/bash
SONIC_MAKE_DEBS += $(BASH)

BASH_DBG = bash-dbgsym_$(BASH_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(BASH_DBG)_RDEPENDS += $(BASH)
$(eval $(call add_derived_package,$(BASH),$(BASH_DBG)))

export BASH_DBG
