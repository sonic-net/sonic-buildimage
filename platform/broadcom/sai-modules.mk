# Broadcom SAI modules
# resolute: XGS kmod only — DNX/Jericho and legacy-Tomahawk kmods dropped
# (no dell platform uses them; see rules.mk).

BRCM_OPENNSL_KERNEL_VERSION = 15.2.0.0.0.0.0.0
BRCM_OPENNSL_KERNEL = opennsl-modules_$(BRCM_OPENNSL_KERNEL_VERSION)_amd64.deb
$(BRCM_OPENNSL_KERNEL)_SRC_PATH = $(PLATFORM_PATH)/saibcm-modules
$(BRCM_OPENNSL_KERNEL)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(BRCM_OPENNSL_KERNEL)_BUILD_ENV += PKG_NAME=$(BRCM_OPENNSL_KERNEL)
$(BRCM_OPENNSL_KERNEL)_MACHINE = broadcom
SONIC_DPKG_DEBS += $(BRCM_OPENNSL_KERNEL)
