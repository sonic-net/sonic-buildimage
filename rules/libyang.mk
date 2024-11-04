# libyang

LIBYANG_VERSION_BASE = 1.0
LIBYANG_VERSION = $(LIBYANG_VERSION_BASE).73
LIBYANG_SUBVERSION = 1

export LIBYANG_VERSION_BASE
export LIBYANG_VERSION
export LIBYANG_SUBVERSION

LIBYANG = libyang_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG)_SRC_PATH = $(SRC_PATH)/libyang
# introduce artifical dependency between LIBYANG and FRR
# make sure LIBYANG is compile after FRR
$(LIBYANG)_AFTER = $(FRR)
SONIC_MAKE_DEBS += $(LIBYANG)

LIBYANG_DEV = libyang-dev_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_DEV)))

LIBYANG_DBGSYM = libyang-dbgsym_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_DBGSYM)))

LIBYANG_CPP = libyang-cpp_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_CPP)_DEPENDS += $(LIBYANG)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_CPP)))

LIBYANG_PY3 = python3-yang_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_PY3)_DEPENDS += $(LIBYANG) $(LIBYANG_CPP)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_PY3)))

$(eval $(call add_conflict_package,$(LIBYANG),$(LIBYANG1),$(LIBYANG2)))
$(eval $(call add_conflict_package,$(LIBYANG_DEV),$(LIBYANG1_DEV),$(LIBYANG2_DEV)))

export LIBYANG LIBYANG_DBGSYM LIBYANG_DEV LIBYANG_CPP LIBYANG_PY3
