# docker image for XSight Labs syncd

DOCKER_SYNCD_PLATFORM_CODE = xsight
include $(PLATFORM_PATH)/../template/docker-syncd-bookworm.mk

$(DOCKER_SYNCD_BASE)_DEPENDS += $(SYNCD)


$(DOCKER_SYNCD_BASE)_DBG_DEPENDS += $(SYNCD_DBG) \
                                $(LIBSWSSCOMMON_DBG) \
                                $(LIBSAIMETADATA_DBG) \
                                $(LIBSAIREDIS_DBG)

$(DOCKER_SYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_SYNCD_BASE)_PACKAGE_NAME = syncd

$(DOCKER_SYNCD_BASE)_RUN_OPT += -v /host/warmboot:/var/warmboot
# #27522 dropped --privileged; SAI needs /dev/xpci. Bind of the path is
# applied on docker start (after start.sh reloads xpci). --device would
# snapshot major/minor at create and go ENXIO after rmmod/modprobe.
# cgroup-rule is required: a bind mount does not grant devices cgroup access.
$(DOCKER_SYNCD_BASE)_RUN_OPT += --device-cgroup-rule='c *:* rwm'

