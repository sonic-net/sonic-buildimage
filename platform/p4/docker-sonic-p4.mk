# docker image for p4 sonic docker image

DOCKER_SONIC_P4 = docker-sonic-p4.gz
$(DOCKER_SONIC_P4)_PATH = $(PLATFORM_PATH)/docker-sonic-p4
$(DOCKER_SONIC_P4)_DEPENDS += $(SWSS) \
                              $(SYNCD) \
                              $(P4_SWITCH) \
                              $(REDIS_TOOLS) \
                              $(REDIS_SERVER) \
                              $(PYTHON_SWSSCOMMON) \
                              $(PYTHON3_SWSSCOMMON) \
                              $(LIBTEAMDCTL) \
                              $(LIBTEAM_UTILS) \
                              $(SONIC_DEVICE_DATA) \
                              $(SONIC_UTILITIES_PY3) \
                              $(IPROUTE2)

# ifeq ($(ROUTING_STACK), quagga)
$(DOCKER_SONIC_P4)_DEPENDS += $(QUAGGA)
# else ifeq ($(ROUTING_STACK), frr)
# $(DOCKER_SONIC_P4)_DEPENDS += $(FRR)
# else
# $(DOCKER_SONIC_P4)_DEPENDS += $(GOBGP)
# endif

$(DOCKER_SONIC_P4)_FILES += $(CONFIGDB_LOAD_SCRIPT) \
                            $(ARP_UPDATE_SCRIPT) \
                            $(ARP_UPDATE_VARS_TEMPLATE)

$(DOCKER_SONIC_P4)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE)
SONIC_DOCKER_IMAGES += $(DOCKER_SONIC_P4)
