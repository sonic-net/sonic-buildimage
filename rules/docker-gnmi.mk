# docker image for GNMI agent

DOCKER_GNMI_STEM = docker-sonic-gnmi
DOCKER_GNMI = $(DOCKER_GNMI_STEM).gz
DOCKER_GNMI_DBG = $(DOCKER_GNMI_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_GNMI)_PATH = $(DOCKERS_PATH)/$(DOCKER_GNMI_STEM)

$(DOCKER_GNMI)_DEPENDS += $(SONIC_MGMT_COMMON)
$(DOCKER_GNMI)_DEPENDS += $(SONIC_TELEMETRY)
$(DOCKER_GNMI)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_DEPENDS)

$(DOCKER_GNMI)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)

$(DOCKER_GNMI)_VERSION = 1.0.0
$(DOCKER_GNMI)_PACKAGE_NAME = gnmi

$(DOCKER_GNMI)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_BOOKWORM)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_GNMI)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_GNMI)
ifeq ($(INCLUDE_SYSTEM_GNMI), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_GNMI)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_DBG)
SONIC_BOOKWORM_DBG_DOCKERS += $(DOCKER_GNMI_DBG)
ifeq ($(INCLUDE_SYSTEM_GNMI), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_GNMI_DBG)
endif

$(DOCKER_GNMI)_CONTAINER_NAME = gnmi
$(DOCKER_GNMI)_RUN_OPT += -t
$(DOCKER_GNMI)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_GNMI)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
$(DOCKER_GNMI)_RUN_OPT += -v /var/run/dbus:/var/run/dbus:rw
# For disk space monitoring.
$(DOCKER_GNMI)_RUN_OPT += -v /:/mnt/host:ro
# For sonic binary image download.
$(DOCKER_GNMI)_RUN_OPT += -v /tmp:/mnt/host/tmp:rw
# For sonic binary image downloads to persistent file system.
$(DOCKER_GNMI)_RUN_OPT += -v /var/tmp:/mnt/host/var/tmp:rw
# For host command execution in gnoi.
$(DOCKER_GNMI)_RUN_OPT += --pid=host
# For host command execution in gnoi.
$(DOCKER_GNMI)_RUN_OPT += --privileged
# For GNOI running sudo command in case of container NS remapping.
$(DOCKER_GNMI)_RUN_OPT += --userns=host




$(DOCKER_GNMI)_BASE_IMAGE_FILES += monit_gnmi:/etc/monit/conf.d
