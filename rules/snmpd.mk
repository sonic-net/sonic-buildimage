# snmpd package

ifeq ($(BLDENV),bookworm)
SNMPD_VERSION = 5.9.3+dfsg
SNMPD_VERSION_FULL = $(SNMPD_VERSION)-2+deb12u1
else ifeq ($(BLDENV),bullseye)
SNMPD_VERSION = 5.9+dfsg
SNMPD_VERSION_FULL = $(SNMPD_VERSION)-4+deb11u1
else
SNMPD_VERSION = 5.7.3+dfsg
SNMPD_VERSION_FULL = $(SNMPD_VERSION)-5
endif

export SNMPD_VERSION SNMPD_VERSION_FULL

LIBSNMP_BASE = libsnmp-base_$(SNMPD_VERSION_FULL)_all.deb
$(LIBSNMP_BASE)_SRC_PATH = $(SRC_PATH)/snmpd
$(LIBSNMP_BASE)_DEPENDS += $(LIBNL3_DEV)
$(LIBSNMP_BASE)_RDEPENDS += $(LIBNL3)
SONIC_MAKE_DEBS += $(LIBSNMP_BASE)

SNMPTRAPD = snmptrapd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(SNMPTRAPD)_DEPENDS += $(LIBSNMP) $(SNMPD)
$(SNMPTRAPD)_RDEPENDS += $(LIBSNMP) $(SNMPD)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMPTRAPD)))

SNMPTRAPD_DBG = snmptrapd-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(SNMPTRAPD_DBG)_DEPENDS += $(SNMPTRAPD)
$(SNMPTRAPD_DBG)_RDEPENDS += $(SNMPTRAPD)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMPTRAPD_DBG)))

SNMP = snmp_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(SNMP)_DEPENDS += $(LIBSNMP)
$(SNMP)_RDEPENDS += $(LIBSNMP)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMP)))

SNMPD = snmpd_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(SNMPD)_DEPENDS += $(LIBSNMP)
$(SNMPD)_RDEPENDS += $(LIBSNMP)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMPD)))

SNMP_DBG = snmp-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMP_DBG)))

SNMPD_DBG = snmpd-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(SNMPD_DBG)))

ifeq ($(BLDENV),bookworm)
LIBSNMP = libsnmp40_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
else ifeq ($(BLDENV),bullseye)
LIBSNMP = libsnmp40_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
else
LIBSNMP = libsnmp30_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
endif
$(LIBSNMP)_RDEPENDS += $(LIBSNMP_BASE)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(LIBSNMP)))

ifeq ($(BLDENV),bookworm)
LIBSNMP_DBG = libsnmp40-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
else ifeq ($(BLDENV),bullseye)
LIBSNMP_DBG = libsnmp40-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
else
LIBSNMP_DBG = libsnmp30-dbg_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
endif
$(LIBSNMP_DBG)_DEPENDS += $(LIBSNMP)
$(LIBSNMP_DBG)_RDEPENDS += $(LIBSNMP)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(LIBSNMP_DBG)))

LIBSNMP_DEV = libsnmp-dev_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(LIBSNMP_DEV)_DEPENDS += $(LIBSNMP)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(LIBSNMP_DEV)))

LIBSNMP_PERL = libsnmp-perl_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(LIBSNMP_PERL)_DEPENDS += $(LIBSNMP)
$(LIBSNMP_PERL)_RDEPENDS += $(LIBSNMP)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(LIBSNMP_PERL)))

LIBSNMP_PERL_DBG = libsnmp-perl-dbgsym_$(SNMPD_VERSION_FULL)_$(CONFIGURED_ARCH).deb
$(LIBSNMP_PERL_DBG)_DEPENDS += $(LIBSNMP_PERL)
$(LIBSNMP_PERL_DBG)_RDEPENDS += $(LIBSNMP_PERL)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(LIBSNMP_PERL_DBG)))

TKMIB = tkmib_$(SNMPD_VERSION_FULL)_all.deb
$(TKMIB)_DEPENDS += $(LIBSNMP_PERL)
$(TKMIB)_RDEPENDS += $(LIBSNMP_PERL)
$(eval $(call add_derived_package,$(LIBSNMP_BASE),$(TKMIB)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += snmpd

