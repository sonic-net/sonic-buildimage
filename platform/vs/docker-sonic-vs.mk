# docker image for virtual switch based sonic docker image

DOCKER_SONIC_VS = docker-sonic-vs.gz
$(DOCKER_SONIC_VS)_PATH = $(PLATFORM_PATH)/docker-sonic-vs
$(DOCKER_SONIC_VS)_DEPENDS += $(SYNCD_VS) \
                              $(PYTHON3_SWSSCOMMON) \
                              $(LIBTEAMDCTL) \
                              $(LIBTEAM_UTILS) \
                              $(SONIC_DEVICE_DATA) \
                              $(LIBYANG) \
                              $(LIBYANG_CPP) \
                              $(LIBYANG_PY3) \
                              $(SONIC_UTILITIES_DATA) \
                              $(SONIC_HOST_SERVICES_DATA) \
                              $(SYSMGR)

# Include feature dockers — auto-merges DEPENDS, PYTHON_WHEELS,
# and provides --build-context for COPY --from=<feature> in Dockerfile.j2
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_LLDP)
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_FPM_FRR)
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_TEAMD)
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_NAT)
$(DOCKER_SONIC_VS)_INCLUDE_DOCKER += $(DOCKER_SFLOW)

$(DOCKER_SONIC_VS)_PYTHON_WHEELS += $(SONIC_PY_COMMON_PY3) \
                                    $(SONIC_PLATFORM_COMMON_PY3) \
                                    $(SONIC_YANG_MODELS_PY3) \
                                    $(SONIC_YANG_MGMT_PY3) \
                                    $(SONIC_UTILITIES_PY3) \
                                    $(SONIC_HOST_SERVICES_PY3)

ifeq ($(INSTALL_DEBUG_TOOLS), y)
$(DOCKER_SONIC_VS)_DEPENDS += $(LIBSWSSCOMMON_DBG) \
                              $(LIBSAIREDIS_DBG) \
                              $(LIBSAIVS_DBG) \
                              $(SYNCD_VS_DBG) \
                              $(SYSMGR_DBG)
endif

ifeq ($(SONIC_ROUTING_STACK), frr)
$(DOCKER_SONIC_VS)_DEPENDS += $(FRR)
else
$(DOCKER_SONIC_VS)_DEPENDS += $(GOBGP)
endif

ifeq ($(INCLUDE_FIPS), y)
$(DOCKER_SONIC_VS)_DEPENDS += $(FIPS_KRB5_ALL)
endif

$(DOCKER_SONIC_VS)_FILES += $(CONFIGDB_LOAD_SCRIPT) \
                            $(ARP_UPDATE_SCRIPT) \
                            $(ARP_UPDATE_VARS_TEMPLATE) \
                            $(BUFFERS_CONFIG_TEMPLATE) \
                            $(QOS_CONFIG_TEMPLATE) \
                            $(SONIC_VERSION) \
                            $(UPDATE_CHASSISDB_CONFIG_SCRIPT) \
                            $(COPP_CONFIG_TEMPLATE)

$(DOCKER_SONIC_VS)_LOAD_DOCKERS += $(DOCKER_SWSS_LAYER_BOOKWORM)
SONIC_DOCKER_IMAGES += $(DOCKER_SONIC_VS)

SONIC_BOOKWORM_DOCKERS += $(DOCKER_SONIC_VS)

# constants.yml is still needed in the build context for bgpcfgd
DOCKER_SONIC_VS_CONSTANTS = $(PLATFORM_PATH)/docker-sonic-vs/constants.yml
$(DOCKER_SONIC_VS_CONSTANTS): files/image_config/constants/constants.yml
	cp -f $< $@

$(TARGET_PATH)/docker-sonic-vs.gz : $(DOCKER_SONIC_VS_CONSTANTS)

