# docker image for bfn saiserver
# Support two version of saiserver, v2 will use the new sai-ptfv2
DOCKER_SAISERVER_BFN = docker-saiserver$(SAITHRIFT_VER)-bfn.gz
$(DOCKER_SAISERVER_BFN)_PATH = $(PLATFORM_PATH)/docker-saiserver-bfn

# Use syncd_init_common.sh to init hardware platform
SYNCD_INIT_COMMON_SCRIPT = syncd_init_common.sh
$(SYNCD_INIT_COMMON_SCRIPT)_PATH = $(SRC_PATH)/sonic-sairedis/syncd/scripts
SONIC_COPY_FILES += $(SYNCD_INIT_COMMON_SCRIPT)

# Same dependence as syncd
$(DOCKER_SAISERVER_BFN)_DEPENDS += $(SAISERVER) 
# Install syncd for reuse the config fun
#$(DOCKER_SAISERVER_BFN)_DEPENDS += $(SYNCD)
$(DOCKER_SAISERVER_BFN)_DEPENDS += $(BFN_SAI) $(BFN_INGRASYS_PLATFORM) $(BFN_PLATFORM) $(LIBTHRIFT_0_14_1)
$(DOCKER_SAISERVER_BFN)_FILES += $(SYNCD_INIT_COMMON_SCRIPT)

# Same dependence as ENABLE_SYNCD_RPC 
$(DOCKER_SAISERVER_BFN)_DEPENDS += $(LIBSAITHRIFT_DEV) $(LIBTHRIFT_0_14_1_DEV)

# Runtime dependency on sai is set only for syncd
#$(SYNCD)_RDEPENDS += $(BFN_SAI) $(WNC_OSW1800_PLATFORM) $(BFN_INGRASYS_PLATFORM) $(BFN_PLATFORM)
$(DOCKER_SAISERVER_BFN)_RDEPENDS += $(BFN_SAI) $(BFN_INGRASYS_PLATFORM) $(BFN_PLATFORM)

$(DOCKER_SAISERVER_BFN)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BULLSEYE)

SONIC_DOCKER_IMAGES += $(DOCKER_SAISERVER_BFN)
SONIC_BULLSEYE_DOCKERS += $(DOCKER_SAISERVER_BFN)

# Only Support saiserver v2
$(DOCKER_SAISERVER_BFN)_CONTAINER_NAME = saiserver$(SAITHRIFT_VER)
$(DOCKER_SAISERVER_BFN)_VERSION = 1.0.0+rpc
$(DOCKER_SAISERVER_BFN)_PACKAGE_NAME = saiserver

$(DOCKER_SAISERVER_BFN)_RUN_OPT += --privileged -t
$(DOCKER_SAISERVER_BFN)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SAISERVER_BFN)_RUN_OPT += -v /var/run/docker-saiserver:/var/run/sswsyncd
$(DOCKER_SAISERVER_BFN)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SAISERVER_BFN)_RUN_OPT += -v /host/warmboot:/var/warmboot
