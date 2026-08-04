# docker image for centec syncd

DOCKER_SYNCD_PLATFORM_CODE = centec
include $(PLATFORM_PATH)/../template/docker-syncd-bullseye.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)

$(DOCKER_SYNCD_CENTEC)_DEPENDS += $(SYNCD_DBG) \
                                  $(LIBSWSSCOMMON_DBG) \
                                  $(LIBSAIMETADATA_DBG) \
                                  $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_CENTEC)_RUN_OPT += --privileged -t
$(DOCKER_SYNCD_CENTEC)_RUN_OPT += -v /host/machine.conf:/etc/machine.conf
$(DOCKER_SYNCD_CENTEC)_RUN_OPT += -v /var/run/docker-syncd:/var/run/sswsyncd
$(DOCKER_SYNCD_CENTEC)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
