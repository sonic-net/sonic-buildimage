# eventd package

SONIC_EVENTD_VERSION = 1.0.0-0
SONIC_EVENTD_PKG_NAME = eventd

SONIC_EVENTD = sonic-$(SONIC_EVENTD_PKG_NAME)_$(SONIC_EVENTD_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_EVENTD)_SRC_PATH = $(SRC_PATH)/sonic-eventd
$(SONIC_EVENTD)_DEPENDS += $(LIBSWSSCOMMON)  $(LIBSWSSCOMMON_DEV)

SONIC_DPKG_DEBS += $(SONIC_EVENTD)

SONIC_EVENTD_DBG = sonic-$(SONIC_EVENTD_PKG_NAME)-dbgsym_$(SONIC_EVENTD_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SONIC_EVENTD),$(SONIC_EVENTD_DBG)))

SONIC_RSYSLOG_PLUGIN = sonic-rsyslog-plugin_$(SONIC_EVENTD_VERSION)_$(CONFIGURED_ARCH).deb
$(eval $(call add_derived_package,$(SONIC_EVENTD),$(SONIC_RSYSLOG_PLUGIN)))

SONIC_RSYSLOG_PLUGIN_DBG = sonic-rsyslog-plugin-dbgsym_$(SONIC_EVENTD_VERSION)_$(CONFIGURED_ARCH).deb
$(SONIC_RSYSLOG_PLUGIN_DBG)_DEPENDS += $(SONIC_RSYSLOG_PLUGIN)
$(SONIC_RSYSLOG_PLUGIN_DBG)_RDEPENDS += $(SONIC_RSYSLOG_PLUGIN)
$(eval $(call add_derived_package,$(SONIC_EVENTD),$(SONIC_RSYSLOG_PLUGIN_DBG)))

# The .c, .cpp, .h & .hpp files under src/{$DBG_SRC_ARCHIVE list}
# are archived into debug one image to facilitate debugging.
#
DBG_SRC_ARCHIVE += sonic-eventd
