# Ciena Platform modules

CIENA_PLATFORM_MODULE_VERSION = 1.0.0

export CIENA_PLATFORM_MODULE_VERSION

# Ciena CN8140 Platform Module — PRIMARY build target
CIENA_8140_PLATFORM_MODULE = sonic-platform-ciena-8140_$(CIENA_PLATFORM_MODULE_VERSION)_amd64.deb
$(CIENA_8140_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-ciena
$(CIENA_8140_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(CIENA_8140_PLATFORM_MODULE)_PLATFORM = x86_64-ciena-8140-r0
SONIC_DPKG_DEBS += $(CIENA_8140_PLATFORM_MODULE)

# fake-hwclock >= 0.15 for the 8140 boot-time clock floor.
#
# The host SoC has no battery-backed RTC; rtc_cmos is seeded from the BMC RTC
# each boot. fake-hwclock provides a forward-only clock floor as a backstop for
# the window before NTP is reachable (e.g. if the BMC RTC supercap is depleted).
#
# Debian trixie ships fake-hwclock 0.14, which has an inverted force-test in its
# "load" path (Debian bug #1093227): with the default FORCE=false it restores the
# saved timestamp UNCONDITIONALLY and can roll the BMC-seeded boot clock BACKWARD.
# The bug was introduced in 0.14 and fixed in 0.15. We vendor the fixed 0.15 deb
# (Architecture: all, no hard dependencies) and install it on the 8140 only, so
# no local override/workaround is needed. See platform/broadcom/extra-debs/.
FAKE_HWCLOCK = fake-hwclock_0.15_all.deb
$(FAKE_HWCLOCK)_PATH = $(PLATFORM_PATH)/extra-debs
$(FAKE_HWCLOCK)_PLATFORM = x86_64-ciena-8140-r0
ifneq (,$(wildcard $($(FAKE_HWCLOCK)_PATH)/$(FAKE_HWCLOCK)))
SONIC_COPY_DEBS += $(FAKE_HWCLOCK)
INSTALL_CIENA_FAKE_HWCLOCK = y
endif
export INSTALL_CIENA_FAKE_HWCLOCK
