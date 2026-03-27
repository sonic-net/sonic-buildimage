# docker image for sonic-gnmi development/CI
#
# This image contains all build dependencies needed to compile and test
# sonic-gnmi. It is intended for use in sonic-gnmi's standalone CI pipeline,
# replacing the manual dependency installation steps.
#
# The _DEPENDS list is derived from $(SONIC_TELEMETRY)_DEPENDS in telemetry.mk,
# ensuring this image stays in sync with the production build environment.
# When telemetry.mk gains a new dependency, add it here too.

DOCKER_GNMI_DEV_STEM = docker-sonic-gnmi-dev
DOCKER_GNMI_DEV = $(DOCKER_GNMI_DEV_STEM).gz

$(DOCKER_GNMI_DEV)_PATH = $(DOCKERS_PATH)/$(DOCKER_GNMI_DEV_STEM)

# Use the same build dependencies as the telemetry (gnmi) deb package.
# This is the single source of truth — when telemetry.mk changes its
# _DEPENDS, this image picks up the same debs.
$(DOCKER_GNMI_DEV)_DEPENDS += $(SONIC_MGMT_COMMON)
$(DOCKER_GNMI_DEV)_DEPENDS += $(SONIC_MGMT_COMMON_CODEGEN)
$(DOCKER_GNMI_DEV)_DEPENDS += $(LIBSWSSCOMMON_DEV)
$(DOCKER_GNMI_DEV)_DEPENDS += $(LIBSWSSCOMMON)

$(DOCKER_GNMI_DEV)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)

$(DOCKER_GNMI_DEV)_VERSION = 1.0.0
$(DOCKER_GNMI_DEV)_PACKAGE_NAME = gnmi-dev

SONIC_DOCKER_IMAGES += $(DOCKER_GNMI_DEV)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_GNMI_DEV)
