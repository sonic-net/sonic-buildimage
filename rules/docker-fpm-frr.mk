# docker image for fpm-frr

DOCKER_FPM_FRR_STEM = docker-fpm-frr
DOCKER_FPM_FRR = $(DOCKER_FPM_FRR_STEM).gz
DOCKER_FPM_FRR_DBG = $(DOCKER_FPM_FRR_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_FPM_FRR)_PATH = $(DOCKERS_PATH)/$(DOCKER_FPM_FRR_STEM)
$(DOCKER_FPM_FRR)_PYTHON_WHEELS += $(SONIC_BGPCFGD) $(SONIC_FRR_MGMT_FRAMEWORK)

$(DOCKER_FPM_FRR)_DEPENDS += $(FRR) $(SWSS) $(LIBYANG3)
ifeq ($(ENABLE_FRR_SNMP_AGENT),y)
$(DOCKER_FPM_FRR)_DEPENDS += $(FRR_SNMP)
endif
$(DOCKER_FPM_FRR)_DBG_DEPENDS = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_DEPENDS)
$(DOCKER_FPM_FRR)_DBG_DEPENDS += $(SWSS_DBG) $(LIBSWSSCOMMON_DBG) \
                                $(FRR_DBG) $(LIBYANG3_DBG)
ifeq ($(ENABLE_FRR_SNMP_AGENT),y)
$(DOCKER_FPM_FRR)_DBG_DEPENDS += $(FRR_SNMP_DBG)
endif

$(DOCKER_FPM_FRR)_DBG_IMAGE_PACKAGES = $($(DOCKER_SWSS_LAYER_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_FPM_FRR)_LOAD_DOCKERS += $(DOCKER_SWSS_LAYER_TRIXIE)

$(DOCKER_FPM_FRR)_VERSION = 1.0.0
$(DOCKER_FPM_FRR)_PACKAGE_NAME = fpm-frr
$(DOCKER_FPM_FRR)_WARM_SHUTDOWN_BEFORE = swss
$(DOCKER_FPM_FRR)_WARM_SHUTDOWN_AFTER = radv
$(DOCKER_FPM_FRR)_FAST_SHUTDOWN_BEFORE = swss
$(DOCKER_FPM_FRR)_FAST_SHUTDOWN_AFTER = radv

SONIC_DOCKER_IMAGES += $(DOCKER_FPM_FRR)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_FPM_FRR_DBG)

$(DOCKER_FPM_FRR)_CONTAINER_NAME = bgp
$(DOCKER_FPM_FRR)_RUN_OPT += -t --cap-add=NET_ADMIN --cap-add=SYS_ADMIN
ifeq ($(ENABLE_ASAN), y)
$(DOCKER_FPM_FRR)_RUN_OPT += --cap-add=SYS_PTRACE
endif
$(DOCKER_FPM_FRR)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_FPM_FRR)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro


$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += vtysh:/usr/bin/vtysh
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += rvtysh:/usr/bin/rvtysh
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += TSA:/usr/bin/TSA
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += TSB:/usr/bin/TSB
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += TSC:/usr/bin/TSC
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += TS:/usr/bin/TS
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += platform_utils:/usr/bin/platform_utils
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += idf_isolation:/usr/bin/idf_isolation
$(DOCKER_FPM_FRR)_BASE_IMAGE_FILES += prefix_list:/usr/bin/prefix_list

SONIC_TRIXIE_DOCKERS += $(DOCKER_FPM_FRR)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_FPM_FRR_DBG)

# The source OCI graph implements the native Trixie VS/FIPS profile, including
# both FRR features, the standard dbg suffix and service IDs from rules/config. Other settings
# retain the legacy recipe and its package/host-wrapper declarations above.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_FPM_FRR) $(DOCKER_FPM_FRR_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS),y:vs:amd64:y)
ifeq ($(ENABLE_FRR_TCMALLOC):$(ENABLE_FRR_SNMP_AGENT):$(FRR_USER_UID):$(FRR_USER_GID):$(DBG_IMAGE_MARK),y:y:300:300:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_FPM_FRR)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_FPM_FRR_DBG)
$(DOCKER_FPM_FRR_DBG)_PATH = $($(DOCKER_FPM_FRR)_PATH)
$(DOCKER_FPM_FRR)_BAZEL_TARGET = //dockers/docker-fpm-frr:$(DOCKER_FPM_FRR)
$(DOCKER_FPM_FRR_DBG)_BAZEL_TARGET = //dockers/docker-fpm-frr:$(DOCKER_FPM_FRR_DBG)
$(DOCKER_FPM_FRR)_BAZEL_ARGS = --//src/sonic-frr:tcmalloc=true --//src/sonic-frr:snmp=true --//dockers/docker-fpm-frr:image_version=$(SONIC_IMAGE_VERSION)
$(DOCKER_FPM_FRR_DBG)_BAZEL_ARGS = $($(DOCKER_FPM_FRR)_BAZEL_ARGS)
SONIC_BAZEL_MANIFESTS += docker-fpm-frr docker-fpm-frr-$(DBG_IMAGE_MARK)
docker-fpm-frr_MANIFEST_IMAGE = $(DOCKER_FPM_FRR)
docker-fpm-frr-$(DBG_IMAGE_MARK)_MANIFEST_IMAGE = $(DOCKER_FPM_FRR)
docker-fpm-frr-$(DBG_IMAGE_MARK)_MANIFEST_SUFFIX = $(DBG_IMAGE_MARK)
$(DOCKER_FPM_FRR)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-swss-layer-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-fpm-frr/manifest.json
$(DOCKER_FPM_FRR_DBG)_BAZEL_DEPENDS = $($(DOCKER_FPM_FRR)_BAZEL_DEPENDS) $(TARGET_PATH)/bazel-manifests/docker-fpm-frr-$(DBG_IMAGE_MARK)/manifest.json
endif
endif
endif
endif
