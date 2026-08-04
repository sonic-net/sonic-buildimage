# docker image for brcm-legacy-th syncd with rpc

DOCKER_SYNCD_BRCM_LEGACY_TH_RPC = docker-syncd-brcm-legacy-th-rpc.gz
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_PATH = $(PLATFORM_PATH)/docker-syncd-brcm-legacy-th-rpc
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_DEPENDS += $(SYNCD_RPC)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_DEPENDS += $(SYNCD_RPC_DBG) \
                                    $(LIBSWSSCOMMON_DBG) \
                                    $(LIBSAIMETADATA_DBG) \
                                    $(LIBSAIREDIS_DBG) \
                                    $(SSWSYNCD)
endif
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_PYTHON_WHEELS += $(PTF_PY3)
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_LEGACY_TH_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)
endif

$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_CONTAINER_NAME = syncd
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_VERSION = 1.0.0+rpc
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_PACKAGE_NAME = syncd-legacy-th
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_RUN_OPT += --privileged -t
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro

$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_BASE_IMAGE_FILES += bcmcmd:/usr/bin/bcmcmd
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_BASE_IMAGE_FILES += bcmsh:/usr/bin/bcmsh
$(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)_MACHINE = broadcom-legacy-th

SONIC_TRIXIE_DOCKERS += $(DOCKER_SYNCD_BRCM_LEGACY_TH_RPC)
