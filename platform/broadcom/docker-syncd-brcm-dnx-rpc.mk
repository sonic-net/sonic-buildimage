# docker image for brcm syncd with rpc

DOCKER_SYNCD_BRCM_DNX_RPC = docker-syncd-brcm-dnx-rpc.gz
DOCKER_SYNCD_DNX_BASE = docker-syncd-brcm-dnx.gz
$(DOCKER_SYNCD_BRCM_DNX_RPC)_PATH = $(PLATFORM_PATH)/docker-syncd-brcm-dnx-rpc
$(DOCKER_SYNCD_BRCM_DNX_RPC)_DEPENDS += $(SYNCD_RPC)
$(DOCKER_SYNCD_BRCM_DNX_RPC)_DEPENDS += $(SSWSYNCD)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SYNCD_BRCM_DNX_RPC)_DEPENDS += $(SYNCD_RPC_DBG) \
                                    $(LIBSWSSCOMMON_DBG) \
                                    $(LIBSAIMETADATA_DBG) \
                                    $(LIBSAIREDIS_DBG)
endif
$(DOCKER_SYNCD_BRCM_DNX_RPC)_PYTHON_WHEELS += $(PTF_PY3)
$(DOCKER_SYNCD_BRCM_DNX_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_DNX_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_BRCM_DNX_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_BRCM_DNX_RPC)
endif

$(DOCKER_SYNCD_BRCM_DNX_RPC)_CONTAINER_NAME = syncd
$(DOCKER_SYNCD_BRCM_DNX_RPC)_VERSION = 1.0.0+rpc
$(DOCKER_SYNCD_BRCM_DNX_RPC)_PACKAGE_NAME = syncd-dnx
$(DOCKER_SYNCD_BRCM_DNX_RPC)_RUN_OPT += --privileged -t
$(DOCKER_SYNCD_BRCM_DNX_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_BRCM_DNX_RPC)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_BRCM_DNX_RPC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_BRCM_DNX_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro

$(DOCKER_SYNCD_BRCM_DNX_RPC)_BASE_IMAGE_FILES += bcmcmd:/usr/bin/bcmcmd
$(DOCKER_SYNCD_BRCM_DNX_RPC)_BASE_IMAGE_FILES += bcmsh:/usr/bin/bcmsh
$(DOCKER_SYNCD_BRCM_DNX_RPC)_MACHINE = broadcom-dnx

SONIC_BOOKWORM_DOCKERS += $(DOCKER_SYNCD_BRCM_DNX_RPC)
