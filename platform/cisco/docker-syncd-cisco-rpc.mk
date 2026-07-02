# docker image for syncd with rpc

DOCKER_SYNCD_CISCO_RPC_STEM = docker-syncd-cisco-rpc
DOCKER_SYNCD_CISCO_RPC = $(DOCKER_SYNCD_CISCO_RPC_STEM).gz
$(DOCKER_SYNCD_CISCO_RPC)_PATH = $(PLATFORM_PATH)/$(DOCKER_SYNCD_CISCO_RPC_STEM)
$(DOCKER_SYNCD_CISCO_RPC)_DEPENDS += $(SYNCD_RPC)
$(DOCKER_SYNCD_CISCO_RPC)_PYTHON_WHEELS += $(PTF_PY3)
$(DOCKER_SYNCD_CISCO_RPC)_FILES += $(SUPERVISOR_PROC_EXIT_LISTENER_SCRIPT)
ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SYNCD_CISCO_RPC)_DEPENDS += $(SYNCD_RPC_DBG) \
                                    $(LIBSWSSCOMMON_DBG) \
                                    $(LIBSAIMETADATA_DBG) \
                                    $(LIBSAIREDIS_DBG)
endif

$(DOCKER_SYNCD_CISCO_RPC)_PYTHON_DEBS += $(CISCO_SFPD)
$(DOCKER_SYNCD_CISCO_RPC)_LOAD_DOCKERS += $(DOCKER_SYNCD_BASE)
SONIC_DOCKER_IMAGES += $(DOCKER_SYNCD_CISCO_RPC)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_SYNCD_CISCO_RPC)
ifeq ($(ENABLE_SYNCD_RPC),y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_SYNCD_CISCO_RPC)
endif

SONIC_BOOKWORM_DOCKERS += $(DOCKER_SYNCD_CISCO_RPC)

$(DOCKER_SYNCD_CISCO_RPC)_CONTAINER_NAME = syncd
$(DOCKER_SYNCD_CISCO_RPC)_VERSION = 1.0.0+rpc
$(DOCKER_SYNCD_CISCO_RPC)_PACKAGE_NAME = syncd
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += --privileged -t
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /host/warmboot:/var/warmboot
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /var/cache/cisco:/var/cache/cisco
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /usr/lib/cisco:/usr/lib/cisco
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /opt/cisco/silicon-one:/opt/cisco/silicon-one
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /var/run/config_gen:/var/run/config_gen
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -e LD_LIBRARY_PATH=/usr/lib/:/usr/lib/cisco/
$(DOCKER_SYNCD_CISCO_RPC)_RUN_OPT += -v /usr/release_info:/usr/release_info
