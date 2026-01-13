# libyang

LIBYANG_VERSION_BASE = 1.0
LIBYANG_VERSION = $(LIBYANG_VERSION_BASE).73
LIBYANG_SUBVERSION = 1

export LIBYANG_VERSION_BASE
export LIBYANG_VERSION
export LIBYANG_SUBVERSION

LIBYANG = libyang_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
ifeq ($(BLDENV),trixie)
$(LIBYANG)_DEPENDS += $(LIBPCRE3_DEV) $(LIBPCRE3) $(LIBPCRE16_3) $(LIBPCRE32_3) $(LIBPCRECPP0V5)
$(LIBYANG)_RDEPENDS += $(LIBPCRE3)
endif
$(LIBYANG)_SRC_PATH = $(SRC_PATH)/libyang
ifeq ($(BLDENV),bookworm)
# introduce artifical dependency between LIBYANG and FRR
# make sure LIBYANG is compile after FRR
$(LIBYANG)_AFTER = $(FRR)
endif
SONIC_MAKE_DEBS += $(LIBYANG)

LIBYANG_DEV = libyang-dev_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_DEV)))

LIBYANG_DBGSYM = libyang-dbgsym_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_DBGSYM)))

LIBYANG_CPP = libyang-cpp_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_CPP)_DEPENDS += $(LIBYANG)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_CPP)))

LIBYANG_CPP_DBG = libyang-cpp-dbgsym_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_CPP_DBG)_DEPENDS += $(LIBYANG_CPP)
$(LIBYANG_CPP_DBG)_RDEPENDS += $(LIBYANG_CPP)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_CPP_DBG)))

LIBYANG_PY3 = python3-yang_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_PY3)_DEPENDS += $(LIBYANG) $(LIBYANG_CPP)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_PY3)))

LIBYANG_PY3_DBG = python3-yang-dbgsym_$(LIBYANG_VERSION)_$(CONFIGURED_ARCH).deb
$(LIBYANG_PY3_DBG)_DEPENDS += $(LIBYANG_PY3)
$(LIBYANG_PY3_DBG)_RDEPENDS += $(LIBYANG_PY3)
$(eval $(call add_derived_package,$(LIBYANG),$(LIBYANG_PY3_DBG)))

#$(eval $(call add_conflict_package,$(LIBYANG),$(LIBYANG3)))
$(eval $(call add_conflict_package,$(LIBYANG_DEV),$(LIBYANG3_DEV)))

export LIBYANG LIBYANG_DBGSYM LIBYANG_DEV LIBYANG_CPP LIBYANG_CPP_DBG LIBYANG_PY3 LIBYANG_PY3_DBG
