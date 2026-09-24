# fs s5800_48t4s Platform modules

FS_S5800_48T4S_PLATFORM_MODULE_VERSION =1.0

export FS_S5800_48T4S_PLATFORM_MODULE_VERSION

FS_S5800_48T4S_PLATFORM_MODULE = platform-modules-s5800-48t4s_$(FS_S5800_48T4S_PLATFORM_MODULE_VERSION)_arm64.deb

$(FS_S5800_48T4S_PLATFORM_MODULE)_SRC_PATH = $(PLATFORM_PATH)/sonic-platform-modules-fs
$(FS_S5800_48T4S_PLATFORM_MODULE)_DEPENDS += $(LINUX_HEADERS) $(LINUX_HEADERS_COMMON)
$(FS_S5800_48T4S_PLATFORM_MODULE)_PLATFORM = arm64-fs_s5800_48t4s-r0
# Both this and the e530 modules compile ../centec/centec-dal/, which causes a problem
# if both are being compiled at the same time. To get around that, add an artificial
# dependency here so that this part is serialized.
$(FS_S5800_48T4S_PLATFORM_MODULE)_AFTER = $(CENTEC_E530_48T4X_P_PLATFORM_MODULE)
SONIC_DPKG_DEBS += $(FS_S5800_48T4S_PLATFORM_MODULE)
