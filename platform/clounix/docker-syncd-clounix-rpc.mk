# docker image for clounix syncd with rpc

DOCKER_SYNCD_CLOUNIX_RPC = docker-syncd-clounix-rpc.gz
$(DOCKER_SYNCD_CLOUNIX_RPC)_PATH = $(PLATFORM_PATH)/docker-syncd-clounix-rpc
$(DOCKER_SYNCD_CLOUNIX_RPC)_DEPENDS += $(SYNCD_RPC) $(LIBTHRIFT) $(PTF)
$(DOCKER_SYNCD_CLOUNIX_RPC)_DBG_DEPENDS += $(SYNCD_RPC_DBG) \
                                      $(LIBSWSSCOMMON_DBG) \
                                      $(LIBSAIMETADATA_DBG) \
                                      $(LIBSAIREDIS_DBG)
$(DOCKER_SYNCD_CLOUNIX_RPC)_FILES += $(DSSERVE)
$(DOCKER_SYNCD_CLOUNIX_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_CLOUNIX_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_CLOUNIX_RPC)
endif

$(DOCKER_SYNCD_CLOUNIX_RPC)_CONTAINER_NAME = syncd

$(DOCKER_SYNCD_CLOUNIX_RPC)_BASE_IMAGE_FILES += clx_diag:/usr/bin/clx_diag
$(DOCKER_SYNCD_CLOUNIX_RPC)_BASE_IMAGE_FILES += clx_ipython:/usr/bin/clx_ipython
$(DOCKER_SYNCD_CLOUNIX_RPC)_BASE_IMAGE_FILES += clx_icling:/usr/bin/clx_icling

$(DOCKER_SYNCD_CLOUNIX_RPC)_RUN_OPT += --net=host --privileged -t
$(DOCKER_SYNCD_CLOUNIX_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_CLOUNIX_RPC)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_CLOUNIX_RPC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_CLOUNIX_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
