# sonic-device-data Debian package

SONIC_DEVICE_DATA_VERSION = 1.0
SONIC_DEVICE_DATA_VERSION_FULL = $(SONIC_DEVICE_DATA_VERSION)-1

export SONIC_DEVICE_DATA_VERSION SONIC_DEVICE_DATA_VERSION_FULL

SONIC_DEVICE_DATA = sonic-device-data_$(SONIC_DEVICE_DATA_VERSION_FULL)_all.deb
$(SONIC_DEVICE_DATA)_SRC_PATH = $(SRC_PATH)/sonic-device-data
SONIC_MAKE_DEBS += $(SONIC_DEVICE_DATA)

installer_device_data_inputs = $(if $(installer_device_data_tar),$(installer_device_data_tar) \
        scripts/install_device_data.py src/sonic-device-data/src/debian/control src/sonic-device-data/src/debian/changelog,\
        $(IMAGE_DISTRO_DEBS_PATH)/$(SONIC_DEVICE_DATA))

ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(BLDENV):$(IMAGE_DISTRO),y:vs:amd64:trixie:trixie)
vs_device_data_tar := $(IMAGE_DISTRO_FILES_PATH)/sonic-device-data-vs.tar
$(TARGET_PATH)/sonic-vs.bin: private installer_device_data_tar := $(vs_device_data_tar)

# Ask Bazel about the current inputs even when Make retained yesterday's TAR.
# The shared exporter keeps its timestamp if the bytes did not change.
.PHONY: check-vs-device-data
$(vs_device_data_tar): check-vs-device-data
	@mkdir -p "$(@D)"
	$(HEADER)
	python3 -B tools/bazel/image/vs/build_device_data.py "$@" $(LOG)
	$(FOOTER)

SONIC_TARGET_LIST += $(vs_device_data_tar)
endif
