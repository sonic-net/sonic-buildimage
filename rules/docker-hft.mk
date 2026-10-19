# docker image for High Frequency Telemetry (HFT): runs countersyncd

DOCKER_HFT_STEM = docker-hft
DOCKER_HFT = $(DOCKER_HFT_STEM).gz
DOCKER_HFT_DBG = $(DOCKER_HFT_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_HFT)_PATH = $(DOCKERS_PATH)/$(DOCKER_HFT_STEM)

# countersyncd is shipped by the swss package, which docker-swss-layer-trixie
# already installs, so docker-hft only adds its own scripts on top of it.
$(DOCKER_HFT)_DBG_DEPENDS = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_DEPENDS)
$(DOCKER_HFT)_DBG_IMAGE_PACKAGES = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_HFT)_LOAD_DOCKERS += $(DOCKER_SWSS_LAYER_TRIXIE)

$(DOCKER_HFT)_VERSION = 1.0.0
$(DOCKER_HFT)_PACKAGE_NAME = hft

SONIC_DOCKER_IMAGES += $(DOCKER_HFT)
SONIC_TRIXIE_DOCKERS += $(DOCKER_HFT)
ifeq ($(INCLUDE_HFT), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_HFT)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_HFT_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_HFT_DBG)
ifeq ($(INCLUDE_HFT), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_HFT_DBG)
endif

$(DOCKER_HFT)_CONTAINER_NAME = hft
$(DOCKER_HFT)_RUN_OPT += -t --cap-add=NET_ADMIN
# Let countersyncd.sh raise net.core.rmem_max, which is only writable from the
# host network namespace, through a writable /proc/sys (same options as swss).
$(DOCKER_HFT)_RUN_OPT += --security-opt apparmor=unconfined --security-opt systempaths=unconfined
$(DOCKER_HFT)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_HFT)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
