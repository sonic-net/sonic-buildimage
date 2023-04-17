# sonic version yml file

sonic_version=$(SONIC_GET_VERSION)
sonic_asic_platform=$(CONFIGURED_PLATFORM)
sonic_os_version=$(SONIC_OS_VERSION)

export sonic_version
export sonic_asic_platform
export sonic_os_version

SONIC_VERSION = sonic_version.yml
$(SONIC_VERSION)_SRC_PATH = $(PLATFORM_PATH)/sonic-version
SONIC_MAKE_FILES += $(SONIC_VERSION)

SONIC_PHONY_TARGETS += $(addprefix $(FILES_PATH)/, $(SONIC_VERSION))
