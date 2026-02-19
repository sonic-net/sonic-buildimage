# docker image for nephos syncd with rpc

DOCKER_SYNCD_NEPHOS_RPC = docker-syncd-nephos-rpc.gz
$(DOCKER_SYNCD_NEPHOS_RPC)_PATH = $(PLATFORM_PATH)/docker-syncd-nephos-rpc
$(DOCKER_SYNCD_NEPHOS_RPC)_DEPENDS += $(SYNCD_RPC) $(LIBTHRIFT) $(PTF)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SYNCD_NEPHOS_RPC)_DEPENDS += $(SYNCD_RPC_DBG) \
                                      $(LIBSWSSCOMMON_DBG) \
                                      $(LIBSAIMETADATA_DBG) \
                                      $(LIBSAIREDIS_DBG)
endif
$(DOCKER_SYNCD_NEPHOS_RPC)_FILES += $(DSSERVE) $(NPX_DIAG)
$(DOCKER_SYNCD_NEPHOS_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_NEPHOS_RPC)
SONIC_STRETCH_DOCKERS += $(DOCKER_SYNCD_NEPHOS_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_NEPHOS_RPC)
endif

$(DOCKER_SYNCD_NEPHOS_RPC)_CONTAINER_NAME = syncd
$(DOCKER_SYNCD_NEPHOS_RPC)_VERSION = 1.0.0+rpc
$(DOCKER_SYNCD_NEPHOS_RPC)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += --cap-add=SYS_RAWIO --cap-add=SYS_ADMIN --cap-add=NET_ADMIN --cap-add=NET_RAW -t
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += --security-opt apparmor=unconfined --security-opt="systempaths=unconfined"
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_NEPHOS_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
