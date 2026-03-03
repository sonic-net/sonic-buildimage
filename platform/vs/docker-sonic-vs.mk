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
                              $(SYSMGR) \
                              $(LLDPD)

$(DOCKER_SONIC_VS)_PYTHON_WHEELS += $(SONIC_PY_COMMON_PY3) \
                                    $(SONIC_PLATFORM_COMMON_PY3) \
                                    $(SONIC_YANG_MODELS_PY3) \
                                    $(SONIC_YANG_MGMT_PY3) \
                                    $(SONIC_UTILITIES_PY3) \
                                    $(SONIC_HOST_SERVICES_PY3) \
                                    $(SONIC_BGPCFGD) \
                                    $(DBSYNCD_PY3)

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

# Copy shared FRR bgpcfgd templates and constants.yml into build context
# These files are shared with docker-fpm-frr to avoid duplication
DOCKER_SONIC_VS_FRR_TEMPLATES = $(PLATFORM_PATH)/docker-sonic-vs/.frr-templates-stamp
$(DOCKER_SONIC_VS_FRR_TEMPLATES): $(shell find dockers/docker-fpm-frr/frr -type f 2>/dev/null)
	cp -rf dockers/docker-fpm-frr/frr/bgpd $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -rf dockers/docker-fpm-frr/frr/common $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -rf dockers/docker-fpm-frr/frr/zebra $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -rf dockers/docker-fpm-frr/frr/staticd $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f dockers/docker-fpm-frr/frr/gen_frr.conf.j2 $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f dockers/docker-fpm-frr/frr/frr.conf.j2 $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f dockers/docker-fpm-frr/frr/frr_vars.j2 $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f dockers/docker-fpm-frr/frr/isolate.j2 $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f dockers/docker-fpm-frr/frr/unisolate.j2 $(PLATFORM_PATH)/docker-sonic-vs/frr/
	cp -f files/image_config/constants/constants.yml $(PLATFORM_PATH)/docker-sonic-vs/
	cp -f dockers/docker-lldp/lldpmgrd $(PLATFORM_PATH)/docker-sonic-vs/
	cp -f dockers/docker-lldp/lldpd.conf.j2 $(PLATFORM_PATH)/docker-sonic-vs/
	cp -f dockers/docker-lldp/lldpdSysDescr.conf.j2 $(PLATFORM_PATH)/docker-sonic-vs/
	cp -f dockers/docker-lldp/waitfor_lldp_ready.sh $(PLATFORM_PATH)/docker-sonic-vs/
	touch $@

$(TARGET_PATH)/docker-sonic-vs.gz : $(DOCKER_SONIC_VS_FRR_TEMPLATES)
