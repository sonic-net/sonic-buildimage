# docker image for otel collector

DOCKER_OTEL_STEM = docker-sonic-otel
DOCKER_OTEL = $(DOCKER_OTEL_STEM).gz
DOCKER_OTEL_DBG = $(DOCKER_OTEL_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_OTEL)_PATH = $(DOCKERS_PATH)/$(DOCKER_OTEL_STEM)

# Since OTEL has similar debug requirements as based on docker-sonic-gnmi, inherit its debug dependencies
$(DOCKER_OTEL)_DBG_DEPENDS = $($(DOCKER_GNMI)_DBG_DEPENDS)

# Load the gnmi container as base dependency
$(DOCKER_OTEL)_LOAD_DOCKERS += $(DOCKER_GNMI)

$(DOCKER_OTEL)_VERSION = 1.0.0
$(DOCKER_OTEL)_PACKAGE_NAME = otel

$(DOCKER_OTEL)_DBG_IMAGE_PACKAGES = $($(DOCKER_GNMI)_DBG_IMAGE_PACKAGES)

# Add to build system
SONIC_DOCKER_IMAGES += $(DOCKER_OTEL)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_OTEL)
ifeq ($(INCLUDE_SYSTEM_OTEL), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_OTEL)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_OTEL_DBG)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_OTEL_DBG)
ifeq ($(INCLUDE_SYSTEM_OTEL), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_OTEL_DBG)
endif

# Runtime configuration
$(DOCKER_OTEL)_CONTAINER_NAME = otel
$(DOCKER_OTEL)_RUN_OPT += -t
$(DOCKER_OTEL)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_OTEL)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
# Expose OTEL collector ports
$(DOCKER_OTEL)_RUN_OPT += -p 4317:4317 -p 4318:4318

# Include monit configuration
$(DOCKER_OTEL)_BASE_IMAGE_FILES += monit_otel:/etc/monit/conf.d
