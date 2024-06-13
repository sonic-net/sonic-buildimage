# docker image for invm saiserver

DOCKER_SAISERVER_INVM = docker-saiserver$(SAITHRIFT_VER)-invm.gz
$(DOCKER_SAISERVER_INVM)_PATH = $(PLATFORM_PATH)/docker-saiserver-invm
$(DOCKER_SAISERVER_INVM)_DEPENDS += $(SAISERVER)
$(DOCKER_SAISERVER_INVM)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_BOOKWORM)
SONIC_DOCKER_IMAGES += $(DOCKER_SAISERVER_INVM)

$(DOCKER_SAISERVER_INVM)_CONTAINER_NAME = saiserver$(SAITHRIFT_VER)
$(DOCKER_SAISERVER_INVM)_RUN_OPT += --privileged -t
$(DOCKER_SAISERVER_INVM)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SAISERVER_INVM)_RUN_OPT += -v /var/run/docker-saiserver:/var/run/sswsyncd
$(DOCKER_SAISERVER_INVM)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_SAISERVER_INVM)_RUN_OPT += -v /host/warmboot:/var/warmboot
