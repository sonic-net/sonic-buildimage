# docker image for centec saiserver

DOCKER_SAISERVER_CENTEC = docker-saiserver$(SAITHRIFT_VER)-centec.gz
$(DOCKER_SAISERVER_CENTEC)_PATH = $(PLATFORM_PATH)/docker-saiserver-centec
$(DOCKER_SAISERVER_CENTEC)_DEPENDS += $(SAISERVER)

# Use syncd_init_common.sh to init hardware platform
SYNCD_INIT_COMMON_SCRIPT = syncd_init_common.sh
$(SYNCD_INIT_COMMON_SCRIPT)_PATH = $(SRC_PATH)/sonic-sairedis/syncd/scripts
SONIC_COPY_FILES += $(SYNCD_INIT_COMMON_SCRIPT)

$(DOCKER_SAISERVER_CENTEC)_FILES += $(SYNCD_INIT_COMMON_SCRIPT)
$(DOCKER_SAISERVER_CENTEC)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)
SONIC_DOCKER_IMAGES += $(DOCKER_SAISERVER_CENTEC)
SONIC_BOOKWORM_DOCKERS += $(DOCKER_SAISERVER_CENTEC)

#Support two versions of saiserver
$(DOCKER_SAISERVER_CENTEC)_CONTAINER_NAME = saiserver$(SAITHRIFT_VER)

$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += --privileged -t
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /var/run/docker-saiserver:/var/run/sswsyncd
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /host/warmboot:/var/warmboot
