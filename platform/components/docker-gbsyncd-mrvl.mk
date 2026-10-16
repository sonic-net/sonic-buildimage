DOCKER_GBSYNCD_PLATFORM_CODE = mrvl

# MRVL_SAI (mrvllibsai_*.deb) is defined and registered as a SONIC_ONLINE_DEBS
# entry in platform/marvell-prestera/sai.mk. It ships libsai.so (eSAI), which
# is loaded by both the NPU syncd and this PHY (gearbox) syncd.

include $(PLATFORM_PATH)/../template/docker-gbsyncd-trixie.mk
$(DOCKER_GBSYNCD_BASE)_VERSION = 1.0.0
$(DOCKER_GBSYNCD_BASE)_PACKAGE_NAME = gbsyncd
$(DOCKER_GBSYNCD_BASE)_PATH = $(PLATFORM_PATH)/../components/docker-gbsyncd-mrvl
$(DOCKER_GBSYNCD_BASE)_DEPENDS += $(SYNCD) $(MRVL_SAI)
