# docker image for centec saiserver

DOCKER_SAISERVER_CENTEC = docker-saiserver-centec.gz
$(DOCKER_SAISERVER_CENTEC)_PATH = $(PLATFORM_PATH)/docker-saiserver-centec
$(DOCKER_SAISERVER_CENTEC)_DEPENDS += $(SAISERVER)
$(DOCKER_SAISERVER_CENTEC)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BULLSEYE)
SONIC_DOCKER_IMAGES += $(DOCKER_SAISERVER_CENTEC)

$(DOCKER_SAISERVER_CENTEC)_CONTAINER_NAME = saiserver
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += --privileged -t
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /var/run/docker-saiserver:/var/run/sswsyncd
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SAISERVER_CENTEC)_RUN_OPT += -v /host/warmboot:/var/warmboot

SONIC_BULLSEYE_DOCKERS += $(DOCKER_SAISERVER_CENTEC)
