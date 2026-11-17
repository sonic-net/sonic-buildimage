# docker image for database

DOCKER_DATABASE_STEM = docker-database
DOCKER_DATABASE = $(DOCKER_DATABASE_STEM).gz
DOCKER_DATABASE_DBG = $(DOCKER_DATABASE_STEM)-$(DBG_IMAGE_MARK).gz

$(DOCKER_DATABASE)_DEPENDS += $(LIBSWSSCOMMON) \
                              $(SONIC_DB_CLI) \
                              $(LIB_SONIC_DASH_API)

$(DOCKER_DATABASE)_PATH = $(DOCKERS_PATH)/$(DOCKER_DATABASE_STEM)

$(DOCKER_DATABASE)_DBG_DEPENDS = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_DEPENDS)

$(DOCKER_DATABASE)_DBG_IMAGE_PACKAGES = $($(DOCKER_CONFIG_ENGINE_TRIXIE)_DBG_IMAGE_PACKAGES)

$(DOCKER_DATABASE)_LOAD_DOCKERS += $(DOCKER_CONFIG_ENGINE_TRIXIE)

$(DOCKER_DATABASE)_VERSION = 1.0.0
$(DOCKER_DATABASE)_PACKAGE_NAME = database

SONIC_DOCKER_IMAGES += $(DOCKER_DATABASE)
SONIC_TRIXIE_DOCKERS += $(DOCKER_DATABASE)
SONIC_INSTALL_DOCKER_IMAGES += $(DOCKER_DATABASE)

SONIC_DOCKER_DBG_IMAGES += $(DOCKER_DATABASE_DBG)
SONIC_TRIXIE_DBG_DOCKERS += $(DOCKER_DATABASE_DBG)
SONIC_INSTALL_DOCKER_DBG_IMAGES += $(DOCKER_DATABASE_DBG)

$(DOCKER_DATABASE)_CONTAINER_NAME = database
$(DOCKER_DATABASE)_RUN_OPT += -t --security-opt apparmor=unconfined --security-opt="systempaths=unconfined"
$(DOCKER_DATABASE)_RUN_OPT += -v /etc/sonic:/etc/sonic:ro
$(DOCKER_DATABASE)_RUN_OPT += -v /etc/localtime:/etc/localtime:ro

$(DOCKER_DATABASE)_BASE_IMAGE_FILES += redis-cli:/usr/bin/redis-cli
$(DOCKER_DATABASE)_FILES += $(SYSCTL_NET_CONFIG)
$(DOCKER_DATABASE)_FILES += $(UPDATE_CHASSISDB_CONFIG_SCRIPT)

# Common/CLI and DASH source TARs replace the selected project package inputs.
# Other profiles retain the Make package prerequisites declared above.
ifeq ($(BLDENV),trixie)
SONIC_BAZEL_SWITCHABLE_IMAGES += $(DOCKER_DATABASE) $(DOCKER_DATABASE_DBG)
ifeq ($(BUILD_WITH_BAZEL_WHEN_AVAILABLE):$(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH):$(INCLUDE_FIPS):$(DBG_IMAGE_MARK),y:vs:amd64:y:dbg)
ifeq ($(filter y,$(CROSS_BUILD_ENVIRON) $(MULTIARCH_QEMU_ENVIRON) $(ENABLE_ASAN)),)
SONIC_BAZEL_DOCKER_IMAGES += $(DOCKER_DATABASE)
SONIC_BAZEL_DBG_DOCKER_IMAGES += $(DOCKER_DATABASE_DBG)
$(DOCKER_DATABASE)_BAZEL_TARGET = //dockers/docker-database:$(DOCKER_DATABASE)
$(DOCKER_DATABASE_DBG)_BAZEL_TARGET = //dockers/docker-database:$(DOCKER_DATABASE_DBG)
SONIC_BAZEL_MANIFESTS += docker-database docker-database-dbg
$(DOCKER_DATABASE)_BAZEL_ARGS = --//dockers/docker-database:image_version=$(SONIC_IMAGE_VERSION)
$(DOCKER_DATABASE_DBG)_BAZEL_ARGS = $($(DOCKER_DATABASE)_BAZEL_ARGS)
SONIC_BAZEL_COMPONENT_LABELS += docker-database
docker-database_MANIFEST_IMAGE = $(DOCKER_DATABASE)
docker-database-dbg_MANIFEST_IMAGE = $(DOCKER_DATABASE)
docker-database-dbg_MANIFEST_SUFFIX = dbg
$(DOCKER_DATABASE)_BAZEL_DEPENDS = $(TARGET_PATH)/bazel-manifests/docker-base-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-config-engine-trixie/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-database/manifest.json \
    $(TARGET_PATH)/bazel-manifests/docker-database/components.labels \
    $(TARGET_PATH)/bazel-manifests/docker-database/image.env
$(DOCKER_DATABASE_DBG)_BAZEL_DEPENDS = $($(DOCKER_DATABASE)_BAZEL_DEPENDS) \
    $(TARGET_PATH)/bazel-manifests/docker-database-dbg/manifest.json

# Pass the selected event database setting as an explicit image input.
# Unchanged bytes retain their timestamp; failures retain the previous input.
export INCLUDE_SYSTEM_EVENTD
$(TARGET_PATH)/bazel-manifests/docker-database/image.env: dockers/docker-database/bazel/prepare_environment.py bazel-manifests-force
	python3 -B dockers/docker-database/bazel/prepare_environment.py --output "$@"
endif
endif
endif
