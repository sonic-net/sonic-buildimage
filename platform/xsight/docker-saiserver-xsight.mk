# docker image for xsight saiserver

DOCKER_SAISERVER_XSIGHT = docker-saiserver$(SAITHRIFT_VER)-xsight.gz
$(DOCKER_SAISERVER_XSIGHT)_PATH = $(PLATFORM_PATH)/docker-saiserver-xsight
$(DOCKER_SAISERVER_XSIGHT)_DEPENDS += $(SAISERVER)

# Use syncd_init_common.sh to init hardware platform
SYNCD_INIT_COMMON_SCRIPT = syncd_init_common.sh
$(SYNCD_INIT_COMMON_SCRIPT)_PATH = $(SRC_PATH)/sonic-sairedis/syncd/scripts
SONIC_COPY_FILES += $(SYNCD_INIT_COMMON_SCRIPT)

$(DOCKER_SAISERVER_XSIGHT)_FILES += $(SYNCD_INIT_COMMON_SCRIPT)
$(DOCKER_SAISERVER_XSIGHT)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BULLSEYE)
SONIC_DOCKER_IMAGES += $(DOCKER_SAISERVER_XSIGHT)
SONIC_BULLSEYE_DOCKERS += $(DOCKER_SAISERVER_XSIGHT)

#Support two versions of saiserver
$(DOCKER_SAISERVER_XSIGHT)_CONTAINER_NAME = saiserver$(SAITHRIFT_VER)

$(DOCKER_SAISERVER_XSIGHT)_RUN_OPT += --privileged -t
$(DOCKER_SAISERVER_XSIGHT)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SAISERVER_XSIGHT)_RUN_OPT += -v /var/run/docker-saiserver:/var/run/sswsyncd
$(DOCKER_SAISERVER_XSIGHT)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SAISERVER_XSIGHT)_RUN_OPT += -v /host/warmboot:/var/warmboot
