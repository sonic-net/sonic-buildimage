# docker image for BMP agent

DOCKER_BMP_STEM = docker-sonic-bmp
DOCKER_BMP = $(DOCKER_BMP_STEM).gz
DOCKER_BMP_DBG = $(DOCKER_BMP_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_BMP)_PATH = $(DOCKERS_PATH)/$(DOCKER_BMP_STEM)

$(DOCKER_BMP)_DEPENDS += $(LIBSWSSCOMMON) \
                         $(SONIC_BMPD)

$(DOCKER_BMP)_PYTHON_WHEELS = $(SONIC_BMPCFGD)
$(DOCKER_BMP)_INSTALL_DEBS = $(LIBSWSSCOMMON) \
                             $(SONIC_BMPD) \
                             $(PYTHON3_SWSSCOMMON) \
                             $(LIBYANG3_PY3)

$(DOCKER_BMP)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)

$(DOCKER_BMP)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_BMP)_VERSION = 1.0.0
$(DOCKER_BMP)_PACKAGE_NAME = bmp

$(DOCKER_BMP)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

SONIC_DOCKER_IMAGES += $(DOCKER_BMP)
SONIC_TRIXIE_DOCKERS += $(DOCKER_BMP)
ifeq ($(INCLUDE_SYSTEM_BMP), y)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_BMP)
endif

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_BMP_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_BMP_DBG)
ifeq ($(INCLUDE_SYSTEM_BMP), y)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_BMP_DBG)
endif

$(DOCKER_BMP)_CONTAINER_NAME = bmp
$(DOCKER_BMP)_RUN_OPT += -t
$(DOCKER_BMP)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_BMP)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro
$(DOCKER_BMP)_RUN_OPT += -v /var/run/dbus:/var/run/dbus:rw

$(DOCKER_BMP)_BASE_IMAGE_FILES += monit_bmp:/etc/monit/conf.d

# Select the native collector and configuration wheel on the supported VS profile.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_BMP) $(DOCKER_BMP_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_BMP)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_BMP_DBG)
$(DOCKER_BMP_DBG)_PATH = $($(DOCKER_BMP)_PATH)
$(DOCKER_BMP)_BAZEL_TARGET = //dockers/docker-sonic-bmp:$(DOCKER_BMP)
$(DOCKER_BMP_DBG)_BAZEL_TARGET = //dockers/docker-sonic-bmp:$(DOCKER_BMP_DBG)
SONIC_BAZEL_MANIFESTS += docker-sonic-bmp docker-sonic-bmp-dbg
docker-sonic-bmp_MANIFEST_IMAGE = $(DOCKER_BMP)
docker-sonic-bmp-dbg_MANIFEST_IMAGE = $(DOCKER_BMP)
docker-sonic-bmp-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_BMP)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-bmp/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-bmp/image.env
$(DOCKER_BMP_DBG)_BAZEL_DEPENDS = $($(DOCKER_BMP)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-sonic-bmp-dbg/manifest.json
SONIC_BAZEL_IMAGE_ENVIRONMENTS += docker-sonic-bmp
endif
endif
endif
