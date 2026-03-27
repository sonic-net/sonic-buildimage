# docker image for sonic-gnmi development/CI
#
# This image contains all build dependencies needed to compile and test
# sonic-gnmi. It is intended for use in sonic-gnmi's standalone CI pipeline,
# replacing the manual dependency installation steps.
#
# Dependencies are derived from SONIC_TELEMETRY_BUILD_DEPENDS (telemetry.mk),
# so this image automatically stays in sync with the production build.

DOCKER_GNMI_DEV_STEM = docker-sonic-gnmi-dev
DOCKER_GNMI_DEV = $(DOCKER_GNMI_DEV_STEM).gz

$(DOCKER_GNMI_DEV)_PATH = $(DOCKERS_PATH)/$(DOCKER_GNMI_DEV_STEM)

# Reuse the build deps defined in telemetry.mk — no duplication
$(DOCKER_GNMI_DEV)_DEPENDS += $(SONIC_TELEMETRY_BUILD_DEPENDS)

$(DOCKER_GNMI_DEV)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)

$(DOCKER_GNMI_DEV)_VERSION = 1.0.0
$(DOCKER_GNMI_DEV)_PACKAGE_NAME = gnmi-dev

SONIC_DOCKER_IMAGES += $(DOCKER_GNMI_DEV)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_GNMI_DEV)
