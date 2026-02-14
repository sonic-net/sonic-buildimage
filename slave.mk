###############################################################################
## Presettings
###############################################################################

# Select bash for commands
.ONESHELL:
SHELL = /bin/bash
.SHELLFLAGS += -e
USER = $(shell id -un)
UID = $(shell id -u)
GUID = $(shell id -g)

ifeq ($(SONIC_IMAGE_VERSION),)
	override SONIC_IMAGE_VERSION := $(shell export BUILD_TIMESTAMP=$(BUILD_TIMESTAMP) && export BUILD_NUMBER=$(BUILD_NUMBER) && . functions.sh && sonic_get_version)
endif

.SECONDEXPANSION:

NULL :=
SPACE := $(NULL) $(NULL)

###############################################################################
## General definitions
###############################################################################

SRC_PATH = src
RULES_PATH = rules
TARGET_PATH = target
DOCKERS_PATH = dockers
BLDENV := $(shell lsb_release -cs)
DEBS_PATH = $(TARGET_PATH)/debs/$(BLDENV)
FILES_PATH = $(TARGET_PATH)/files/$(BLDENV)
PHONY_PATH = $(TARGET_PATH)/phony/$(BLDENV)
PYTHON_DEBS_PATH = $(TARGET_PATH)/python-debs/$(BLDENV)
PYTHON_WHEELS_PATH = $(TARGET_PATH)/python-wheels/$(BLDENV)
PROJECT_ROOT := $(shell pwd)
JESSIE_DEBS_PATH = $(TARGET_PATH)/debs/jessie
JESSIE_FILES_PATH = $(TARGET_PATH)/files/jessie
STRETCH_DEBS_PATH = $(TARGET_PATH)/debs/stretch
STRETCH_FILES_PATH = $(TARGET_PATH)/files/stretch
BUSTER_DEBS_PATH = $(TARGET_PATH)/debs/buster
BUSTER_FILES_PATH = $(TARGET_PATH)/files/buster
BULLSEYE_DEBS_PATH = $(TARGET_PATH)/debs/bullseye
BULLSEYE_FILES_PATH = $(TARGET_PATH)/files/bullseye
BOOKWORM_DEBS_PATH = $(TARGET_PATH)/debs/bookworm
BOOKWORM_FILES_PATH = $(TARGET_PATH)/files/bookworm
BOOKWORM_PHONY_PATH = $(TARGET_PATH)/phony/bookworm
TRIXIE_DEBS_PATH = $(TARGET_PATH)/debs/trixie
TRIXIE_FILES_PATH = $(TARGET_PATH)/files/trixie
TRIXIE_PHONY_PATH = $(TARGET_PATH)/phony/trixie

DBG_IMAGE_MARK = dbg
DBG_SRC_ARCHIVE_FILE = $(TARGET_PATH)/sonic_src.tar.gz
BUILD_WORKDIR = /sonic
DPKG_ADMINDIR_PATH = $(BUILD_WORKDIR)/dpkg
SLAVE_DIR ?= sonic-slave-$(BLDENV)

CONFIGURED_PLATFORM = $(if $(PLATFORM),$(PLATFORM),$(shell [ -f .platform ] && cat .platform || echo generic))
PLATFORM_PATH = platform/$(CONFIGURED_PLATFORM)
CONFIGURED_ARCH := $(shell [ -f .arch ] && cat .arch || echo amd64)
ifeq ($(PLATFORM_ARCH),)
	override PLATFORM_ARCH = $(CONFIGURED_ARCH)
endif
DOCKER_BASE_ARCH := $(CONFIGURED_ARCH)
ifeq ($(CONFIGURED_ARCH),armhf)
	override DOCKER_BASE_ARCH = arm32v7
else
ifeq ($(CONFIGURED_ARCH),arm64)
	override DOCKER_BASE_ARCH = arm64v8
endif
endif

IMAGE_DISTRO := trixie
IMAGE_DISTRO_DEBS_PATH = $(TARGET_PATH)/debs/$(IMAGE_DISTRO)
IMAGE_DISTRO_FILES_PATH = $(TARGET_PATH)/files/$(IMAGE_DISTRO)

# Python 2 packages will not be available in Bullseye and newer
ifneq ($(filter bullseye bookworm trixie,$(BLDENV)),)
ENABLE_PY2_MODULES = n
else
ENABLE_PY2_MODULES = y
endif

# Python version for PTF image
PTF_ENV_PY_VER = $(if $(SONIC_PTF_ENV_PY_VER),$(SONIC_PTF_ENV_PY_VER),mixed)

export BUILD_NUMBER
export BUILD_TIMESTAMP
export SONIC_IMAGE_VERSION
export CONFIGURED_PLATFORM
export CONFIGURED_ARCH
export TARGET_BOOTLOADER
export PYTHON_WHEELS_PATH
export IMAGE_DISTRO
export IMAGE_DISTRO_DEBS_PATH
export MULTIARCH_QEMU_ENVIRON
export DOCKER_BASE_ARCH
export CROSS_BUILD_ENVIRON
export BLDENV
export BUILD_WORKDIR
export MIRROR_SNAPSHOT
export SONIC_OS_VERSION
export FILES_PATH
export PROJECT_ROOT
export PTF_ENV_PY_VER

###############################################################################
## Utility rules
## Define configuration, help etc.
###############################################################################

# Install the updated build hooks if INSHOOKS flag is set
export INSHOOKS=y
$(if $(INSHOOKS),$(shell sudo dpkg -i /usr/local/share/buildinfo/sonic-build-hooks_1.0_all.deb &>/dev/null))

.platform :
ifneq ($(CONFIGURED_PLATFORM),generic)
	$(Q)echo Build system is not configured, please run make configure
	$(Q)exit 1
endif

configure :
	$(Q)mkdir -p $(JESSIE_DEBS_PATH)
	$(Q)mkdir -p $(STRETCH_DEBS_PATH)
	$(Q)mkdir -p $(BUSTER_DEBS_PATH)
	$(Q)mkdir -p $(BULLSEYE_DEBS_PATH)
	$(Q)mkdir -p $(BOOKWORM_DEBS_PATH)
	$(Q)mkdir -p $(TRIXIE_DEBS_PATH)
	$(Q)mkdir -p $(FILES_PATH)
	$(Q)mkdir -p $(JESSIE_FILES_PATH)
	$(Q)mkdir -p $(STRETCH_FILES_PATH)
	$(Q)mkdir -p $(BUSTER_FILES_PATH)
	$(Q)mkdir -p $(BULLSEYE_FILES_PATH)
	$(Q)mkdir -p $(BOOKWORM_FILES_PATH)
	$(Q)mkdir -p $(BOOKWORM_PHONY_PATH)
	$(Q)mkdir -p $(TRIXIE_FILES_PATH)
	$(Q)mkdir -p $(TRIXIE_PHONY_PATH)
	$(Q)mkdir -p $(PYTHON_DEBS_PATH)
	$(Q)mkdir -p $(PYTHON_WHEELS_PATH)
	$(Q)mkdir -p $(DPKG_ADMINDIR_PATH)
	$(Q)mkdir -p $(TARGET_PATH)/vcache
	$(Q)echo $(PLATFORM) > .platform
	$(Q)echo $(PLATFORM_ARCH) > .arch

distclean : .platform clean
	$(Q)rm -f .platform
	$(Q)rm -f .arch

list :
	$(Q)$(foreach target,$(SONIC_TARGET_LIST),echo $(target);)

###############################################################################
## Include other rules
###############################################################################

include $(RULES_PATH)/config
-include $(RULES_PATH)/config.user


###############################################################################
## Version control related exports
###############################################################################
export PACKAGE_URL_PREFIX
export TRUSTED_GPG_URLS
export SONIC_VERSION_CONTROL_COMPONENTS
DEFAULT_CONTAINER_REGISTRY := $(SONIC_DEFAULT_CONTAINER_REGISTRY)
export DEFAULT_CONTAINER_REGISTRY
export MIRROR_URLS
export MIRROR_SECURITY_URLS

ifeq ($(SONIC_ENABLE_PFCWD_ON_START),y)
ENABLE_PFCWD_ON_START = y
endif

ifeq ($(SONIC_INCLUDE_SYSTEM_TELEMETRY),y)
INCLUDE_SYSTEM_TELEMETRY = y
endif

ifeq ($(SONIC_INCLUDE_SYSTEM_GNMI),y)
INCLUDE_SYSTEM_GNMI = y
endif

ifeq ($(SONIC_INCLUDE_SYSTEM_OTEL),y)
INCLUDE_SYSTEM_OTEL = y
endif

ifeq ($(SONIC_INCLUDE_SYSTEM_BMP),y)
INCLUDE_SYSTEM_BMP = y
endif

ifeq ($(SONIC_INCLUDE_SYSTEM_EVENTD),y)
INCLUDE_SYSTEM_EVENTD = y
endif

ifeq ($(SONIC_INCLUDE_RESTAPI),y)
INCLUDE_RESTAPI = y
endif

ifeq ($(SONIC_ENABLE_SYNCD_RPC),y)
ENABLE_SYNCD_RPC = y
endif

ifeq ($(SONIC_INSTALL_DEBUG_TOOLS),y)
INSTALL_DEBUG_TOOLS = y
endif

ifeq ($(SONIC_SAITHRIFT_V2),y)
SAITHRIFT_V2 = y
SAITHRIFT_VER = v2
endif

ifeq ($(SONIC_INCLUDE_SFLOW),y)
INCLUDE_SFLOW = y
endif

ifeq ($(SONIC_INCLUDE_NAT),y)
INCLUDE_NAT = y
endif

ifeq ($(SONIC_INCLUDE_P4RT),y)
INCLUDE_P4RT = y
endif

# Pre-built Bazel is not available for armhf, so exclude P4RT
# TODO(PINS): Remove when Bazel binaries are available for armhf
ifeq ($(CONFIGURED_ARCH),armhf)
ifeq ($(INCLUDE_P4RT),y)
$(Q)echo "Disabling P4RT due to incompatible CPU architecture: $(CONFIGURED_ARCH)"
endif
override INCLUDE_P4RT = n
endif

# Pre-built Bazel is not available for arm64, so exclude P4RT
# TODO(PINS): Remove when Bazel binaries are available for arm64
ifeq ($(CONFIGURED_ARCH),arm64)
ifeq ($(INCLUDE_P4RT),y)
$(Q)echo "Disabling P4RT due to incompatible CPU architecture: $(CONFIGURED_ARCH)"
endif
override INCLUDE_P4RT = n
endif

ifeq ($(SONIC_INCLUDE_MACSEC),y)
INCLUDE_MACSEC = y
endif

ifeq ($(SONIC_INCLUDE_STP),y)
INCLUDE_STP = y
endif

ifneq ($(SONIC_INCLUDE_TEAMD),)
override INCLUDE_TEAMD = $(SONIC_INCLUDE_TEAMD)
endif

ifneq ($(SONIC_INCLUDE_ROUTER_ADVERTISER),)
override INCLUDE_ROUTER_ADVERTISER = $(SONIC_INCLUDE_ROUTER_ADVERTISER)
endif

ifeq ($(ENABLE_AUTO_TECH_SUPPORT),y)
ENABLE_AUTO_TECH_SUPPORT = y
endif

ifeq ($(SONIC_INCLUDE_MUX),y)
INCLUDE_MUX = y
endif

ifeq ($(SONIC_INCLUDE_BOOTCHART),y)
INCLUDE_BOOTCHART = y
endif

ifeq ($(SONIC_ENABLE_BOOTCHART),y)
ENABLE_BOOTCHART = y
endif

ifeq ($(ENABLE_ASAN),y)
ifneq ($(CONFIGURED_ARCH),amd64)
$(Q)echo "Disabling SWSS address sanitizer due to incompatible CPU architecture: $(CONFIGURED_ARCH)"
override ENABLE_ASAN = n
endif
endif

include $(RULES_PATH)/functions

ifeq ($(SONIC_USE_PDDF_FRAMEWORK),y)
PDDF_SUPPORT = y
else
PDDF_SUPPORT = n
endif
export PDDF_SUPPORT

include $(RULES_PATH)/*.mk
ifneq ($(CONFIGURED_PLATFORM), undefined)
ifeq ($(PDDF_SUPPORT), y)
PDDF_DIR = pddf
PLATFORM_PDDF_PATH = platform/$(PDDF_DIR)
include $(PLATFORM_PDDF_PATH)/rules.mk
endif
include $(PLATFORM_PATH)/rules.mk
endif

ifeq ($(USERNAME),)
override USERNAME := $(DEFAULT_USERNAME)
else
$(warning USERNAME given on command line: could be visible to other users)
endif

ifeq ($(PASSWORD),)
override PASSWORD := $(DEFAULT_PASSWORD)
else
$(warning PASSWORD given on command line: could be visible to other users)
endif

ifeq ($(SONIC_DEBUGGING_ON),y)
DEB_BUILD_OPTIONS_GENERIC := nostrip
endif

ifeq ($(SONIC_PROFILING_ON),y)
DEB_BUILD_OPTIONS_GENERIC := nostrip noopt
endif

ifeq ($(SONIC_BUILD_JOBS),)
override SONIC_BUILD_JOBS := $(SONIC_CONFIG_BUILD_JOBS)
endif

DOCKER_IMAGE_REF = $*-$(DOCKER_USERNAME):$(DOCKER_USERTAG)
DOCKER_DBG_IMAGE_REF = $*-$(DBG_IMAGE_MARK)-$(DOCKER_USERNAME):$(DOCKER_USERTAG)
export DOCKER_USERNAME DOCKER_USERTAG

ifeq ($(VS_PREPARE_MEM),)
override VS_PREPARE_MEM := $(DEFAULT_VS_PREPARE_MEM)
endif

ifeq ($(KERNEL_PROCURE_METHOD),)
override KERNEL_PROCURE_METHOD := $(DEFAULT_KERNEL_PROCURE_METHOD)
endif

ifeq ($(BUILD_LOG_TIMESTAMP),)
override BUILD_LOG_TIMESTAMP := $(DEFAULT_BUILD_LOG_TIMESTAMP)
endif

MAKEFLAGS += -j $(SONIC_BUILD_JOBS)
export SONIC_CONFIG_MAKE_JOBS

ifeq ($(CONFIGURED_PLATFORM),vs)
export BUILD_MULTIASIC_KVM
endif

ifeq ($(CROSS_BUILD_ENVIRON),y)
DEB_BUILD_OPTIONS_GENERIC += nocheck
export $(dpkg-architecture -a$(CONFIGURED_ARCH))
ifeq ($(ENABLE_PY2_MODULES),n)
ANT_DEB_CROSS_PROFILES=nopython2
endif
ANT_DEB_CROSS_OPT := -a$(CONFIGURED_ARCH) -Pcross,nocheck,$(ANT_DEB_CROSS_PROFILES)
ANT_DEB_CONFIG := CONFIG_SITE=/etc/dpkg-cross/cross-config.$(CONFIGURED_ARCH)

VIRTENV_BASE_CROSS_PYTHON2 = /python_virtualenv/env2/
VIRTENV_BASE_CROSS_PYTHON3 = /python_virtualenv/env3/
VIRTENV_BIN_CROSS_PYTHON2 = $(VIRTENV_BASE_CROSS_PYTHON2)/bin/
VIRTENV_BIN_CROSS_PYTHON3 = $(VIRTENV_BASE_CROSS_PYTHON3)/bin/
VIRTENV_LIB_CROSS_PYTHON2 = $(VIRTENV_BASE_CROSS_PYTHON2)/lib/
VIRTENV_LIB_CROSS_PYTHON3 = $(VIRTENV_BASE_CROSS_PYTHON3)/lib/

CROSS_HOST_TYPE = $(shell dpkg-architecture -a $(CONFIGURED_ARCH) -q DEB_HOST_MULTIARCH)

ifeq ($(CONFIGURED_ARCH),armhf)
GOARCH=arm
else ifeq ($(CONFIGURED_ARCH),arm64)
GOARCH=arm64
endif

CROSS_COMPILE = $(CROSS_HOST_TYPE)-
CC = $(CROSS_COMPILE)gcc
CXX = $(CROSS_COMPILE)g++
AR = $(CROSS_COMPILE)ar
LD = $(CROSS_COMPILE)ld
CROSS_LIB_PATH = /usr/$(CROSS_HOST_TYPE)/lib/
CROSS_BIN_PATH = /usr/$(CROSS_HOST_TYPE)/bin/
CROSS_PKGS_LIB_PATH = /usr/lib/$(CROSS_HOST_TYPE)

CROSS_LIBPERL_VERSION = $(shell dpkg -s libperl-dev:$(CONFIGURED_ARCH)|grep Version|awk  '{split($$2,v,"-"); print v[1];}')
CROSS_PERL_CORE_PATH = $(CROSS_PKGS_LIB_PATH)/perl/$(CROSS_LIBPERL_VERSION)/CORE/

CROSS_COMPILE_FLAGS := CGO_ENABLED=1 GOOS=linux GOARCH=$(GOARCH) CROSS_COMPILE=$(CROSS_COMPILE) OVERRIDE_HOST_TYPE=$(CROSS_HOST_TYPE) CROSS_LIB_PATH=$(CROSS_LIB_PATH) CROSS_BIN_PATH=$(CROSS_BIN_PATH) CROSS_HOST_TYPE=$(CROSS_HOST_TYPE) CROSS_PKGS_LIB_PATH=$(CROSS_PKGS_LIB_PATH) CROSS_PERL_CORE_PATH=$(CROSS_PERL_CORE_PATH) CC=$(CC) CXX=$(CXX) AR=$(AR) LD=$(LD)
else ifeq ($(BUILD_SKIP_TEST),y)
DEB_BUILD_OPTIONS_GENERIC += nocheck
endif

ifeq ($(CROSS_BUILD_ENVIRON),y)
ifeq ($(CONFIGURED_ARCH),armhf)
RUST_CROSS_COMPILE_TARGET = armv7-unknown-linux-gnueabihf
else ifeq ($(CONFIGURED_ARCH),arm64)
RUST_CROSS_COMPILE_TARGET = aarch64-unknown-linux-gnu
endif
export RUST_CROSS_COMPILE_TARGET
endif

###############################################################################
## Routing stack related exports
###############################################################################

export SONIC_ROUTING_STACK
export FRR_USER_UID
export FRR_USER_GID
export INCLUDE_FIPS
export ENABLE_FIPS

###############################################################################
## Build Options
###############################################################################
export DEB_BUILD_OPTIONS = hardening=+all

###############################################################################
## Dumping key config attributes associated to current building exercise
###############################################################################

ifndef SONIC_BUILD_QUIETER
$(info SONiC Build System)
$(info )
$(info Build Configuration)
$(info "CONFIGURED_PLATFORM"             : "$(if $(PLATFORM),$(PLATFORM),$(CONFIGURED_PLATFORM))")
$(info "CONFIGURED_ARCH"                 : "$(if $(PLATFORM_ARCH),$(PLATFORM_ARCH),$(CONFIGURED_ARCH))")
$(info "SONIC_CONFIG_PRINT_DEPENDENCIES" : "$(SONIC_CONFIG_PRINT_DEPENDENCIES)")
$(info "SONIC_BUILD_JOBS"                : "$(SONIC_BUILD_JOBS)")
$(info "SONIC_CONFIG_MAKE_JOBS"          : "$(SONIC_CONFIG_MAKE_JOBS)")
$(info "USE_NATIVE_DOCKERD_FOR_BUILD"    : "$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD)")
$(info "SONIC_USE_DOCKER_BUILDKIT"       : "$(SONIC_USE_DOCKER_BUILDKIT)")
$(info "USERNAME"                        : "$(USERNAME)")
$(info "PASSWORD"                        : "$(PASSWORD)")
$(info "CHANGE_DEFAULT_PASSWORD"         : "$(CHANGE_DEFAULT_PASSWORD)")
$(info "SECURE_UPGRADE_MODE"             : "$(SECURE_UPGRADE_MODE)")
$(info "SECURE_UPGRADE_DEV_SIGNING_KEY"  : "$(SECURE_UPGRADE_DEV_SIGNING_KEY)")
$(info "SECURE_UPGRADE_SIGNING_CERT"     : "$(SECURE_UPGRADE_SIGNING_CERT)")
$(info "SECURE_UPGRADE_KERNEL_CAFILE"    : "$(SECURE_UPGRADE_KERNEL_CAFILE)")
$(info "SECURE_UPGRADE_PROD_SIGNING_TOOL": "$(SECURE_UPGRADE_PROD_SIGNING_TOOL)")
$(info "SECURE_UPGRADE_PROD_TOOL_ARGS"   : "$(SECURE_UPGRADE_PROD_TOOL_ARGS)")
$(info "ONIE_IMAGE_PART_SIZE"            : "$(ONIE_IMAGE_PART_SIZE)")
$(info "SHUTDOWN_BGP_ON_START"           : "$(SHUTDOWN_BGP_ON_START)")
$(info "ENABLE_PFCWD_ON_START"           : "$(ENABLE_PFCWD_ON_START)")
$(info "SONIC_BUFFER_MODEL"              : "$(SONIC_BUFFER_MODEL)")
$(info "INSTALL_DEBUG_TOOLS"             : "$(INSTALL_DEBUG_TOOLS)")
$(info "ROUTING_STACK"                   : "$(SONIC_ROUTING_STACK)")
ifeq ($(SONIC_ROUTING_STACK),frr)
$(info "FRR_USER_UID"                    : "$(FRR_USER_UID)")
$(info "FRR_USER_GID"                    : "$(FRR_USER_GID)")
endif
$(info "ENABLE_SYNCD_RPC"                : "$(ENABLE_SYNCD_RPC)")
$(info "SAITHRIFT_V2"                    : "$(SAITHRIFT_V2)")
$(info "ENABLE_ORGANIZATION_EXTENSIONS"  : "$(ENABLE_ORGANIZATION_EXTENSIONS)")
$(info "HTTP_PROXY"                      : "$(HTTP_PROXY)")
$(info "HTTPS_PROXY"                     : "$(HTTPS_PROXY)")
$(info "NO_PROXY"                        : "$(NO_PROXY)")
$(info "ENABLE_ZTP"                      : "$(ENABLE_ZTP)")
$(info "INCLUDE_PDE"                     : "$(INCLUDE_PDE)")
$(info "SONIC_DEBUGGING_ON"              : "$(SONIC_DEBUGGING_ON)")
$(info "SONIC_PROFILING_ON"              : "$(SONIC_PROFILING_ON)")
$(info "KERNEL_PROCURE_METHOD"           : "$(KERNEL_PROCURE_METHOD)")
$(info "BUILD_TIMESTAMP"                 : "$(BUILD_TIMESTAMP)")
$(info "BUILD_LOG_TIMESTAMP"             : "$(BUILD_LOG_TIMESTAMP)")
$(info "SONIC_IMAGE_VERSION"             : "$(SONIC_IMAGE_VERSION)")
$(info "BLDENV"                          : "$(BLDENV)")
$(info "VS_PREPARE_MEM"                  : "$(VS_PREPARE_MEM)")
$(info "INCLUDE_MGMT_FRAMEWORK"          : "$(INCLUDE_MGMT_FRAMEWORK)")
$(info "INCLUDE_ICCPD"                   : "$(INCLUDE_ICCPD)")
$(info "INCLUDE_STP"                     : "$(INCLUDE_STP)")
$(info "INCLUDE_SYSTEM_TELEMETRY"        : "$(INCLUDE_SYSTEM_TELEMETRY)")
$(info "INCLUDE_SYSTEM_GNMI"             : "$(INCLUDE_SYSTEM_GNMI)")
$(info "INCLUDE_SYSTEM_BMP"              : "$(INCLUDE_SYSTEM_BMP)")
$(info "INCLUDE_SYSTEM_OTEL"             : "$(INCLUDE_SYSTEM_OTEL)")
$(info "INCLUDE_SYSTEM_EVENTD"           : "$(INCLUDE_SYSTEM_EVENTD)")
$(info "ENABLE_HOST_SERVICE_ON_START"    : "$(ENABLE_HOST_SERVICE_ON_START)")
$(info "INCLUDE_RESTAPI"                 : "$(INCLUDE_RESTAPI)")
$(info "INCLUDE_SFLOW"                   : "$(INCLUDE_SFLOW)")
$(info "INCLUDE_NAT"                     : "$(INCLUDE_NAT)")
$(info "INCLUDE_DHCP_RELAY"              : "$(INCLUDE_DHCP_RELAY)")
$(info "INCLUDE_DHCP_SERVER"             : "$(INCLUDE_DHCP_SERVER)")
$(info "INCLUDE_P4RT"                    : "$(INCLUDE_P4RT)")
$(info "INCLUDE_VS_DASH_SAI"             : "$(INCLUDE_VS_DASH_SAI)")
$(info "INCLUDE_KUBERNETES"              : "$(INCLUDE_KUBERNETES)")
$(info "INCLUDE_KUBERNETES_MASTER"       : "$(INCLUDE_KUBERNETES_MASTER)")
$(info "INCLUDE_MACSEC"                  : "$(INCLUDE_MACSEC)")
$(info "INCLUDE_MUX"                     : "$(INCLUDE_MUX)")
$(info "INCLUDE_TEAMD"                   : "$(INCLUDE_TEAMD)")
$(info "INCLUDE_DASH_HA"                 : "$(INCLUDE_DASH_HA)")
$(info "INCLUDE_ROUTER_ADVERTISER"       : "$(INCLUDE_ROUTER_ADVERTISER)")
$(info "INCLUDE_BOOTCHART                : "$(INCLUDE_BOOTCHART)")
$(info "ENABLE_BOOTCHART                 : "$(ENABLE_BOOTCHART)")
$(info "INCLUDE_FIPS"                    : "$(INCLUDE_FIPS)")
$(info "ENABLE_TRANSLIB_WRITE"           : "$(ENABLE_TRANSLIB_WRITE)")
$(info "ENABLE_NATIVE_WRITE"             : "$(ENABLE_NATIVE_WRITE)")
$(info "ENABLE_DIALOUT"                  : "$(ENABLE_DIALOUT)")
$(info "ENABLE_AUTO_TECH_SUPPORT"        : "$(ENABLE_AUTO_TECH_SUPPORT)")
$(info "PDDF_SUPPORT"                    : "$(PDDF_SUPPORT)")
$(info "MULTIARCH_QEMU_ENVIRON"          : "$(MULTIARCH_QEMU_ENVIRON)")
$(info "SONIC_VERSION_CONTROL_COMPONENTS": "$(SONIC_VERSION_CONTROL_COMPONENTS)")
$(info "ENABLE_ASAN"                     : "$(ENABLE_ASAN)")
$(info "DEFAULT_CONTAINER_REGISTRY"      : "$(SONIC_DEFAULT_CONTAINER_REGISTRY)")
ifeq ($(CONFIGURED_PLATFORM),vs)
$(info "BUILD_MULTIASIC_KVM"             : "$(BUILD_MULTIASIC_KVM)")
endif
$(info "CROSS_BUILD_ENVIRON"             : "$(CROSS_BUILD_ENVIRON)")
$(info "INCLUDE_EXTERNAL_PATCHES"        : "$(INCLUDE_EXTERNAL_PATCHES)")
$(info "PTF_ENV_PY_VER"                  : "$(PTF_ENV_PY_VER)")
$(info "ENABLE_MULTIDB"                  : "$(ENABLE_MULTIDB)")
$(info )
else
$(info SONiC Build System for $(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH))
endif

# Definition of SONIC_RFS_TARGETS
define rfs_get_installer_dependencies
$(call rfs_build_target_name,$(1),$($(1)_MACHINE))
endef

define rfs_build_target_name
$(1)__$(2)__rfs.squashfs
endef

define rfs_define_target
$(eval rfs_target=$(call rfs_build_target_name,$(1),$($(1)_MACHINE)))
$(eval $(rfs_target)_INSTALLER=$(1))
$(eval $(rfs_target)_MACHINE=$($(1)_MACHINE))
$(eval SONIC_RFS_TARGETS+=$(rfs_target))

$(if $($(1)_DEPENDENT_MACHINE),\
	$(eval dependent_rfs_target=$(call rfs_build_target_name,$(1),$($(1)_DEPENDENT_MACHINE)))
	$(eval $(dependent_rfs_target)_INSTALLER=$(1))
	$(eval $(dependent_rfs_target)_MACHINE=$($(1)_DEPENDENT_MACHINE))
	$(eval SONIC_RFS_TARGETS+=$(dependent_rfs_target))
	$(eval $(rfs_target)_DEPENDENT_RFS=$(dependent_rfs_target)))
endef

$(foreach installer,$(SONIC_INSTALLERS),$(eval $(call rfs_define_target,$(installer))))
$(foreach installer, $(SONIC_INSTALLERS), $(eval $(installer)_RFS_DEPENDS=$(call rfs_get_installer_dependencies,$(installer))))

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_RFS_TARGETS))

# Overwrite the buildinfo in slave container
ifeq ($(filter clean,$(MAKECMDGOALS)),)
$(shell DBGOPT='$(DBGOPT)' scripts/prepare_slave_container_buildinfo.sh $(SLAVE_DIR) $(CONFIGURED_ARCH) $(BLDENV))
endif
include Makefile.cache


###############################################################################
## Generic rules section
## All rules must go after includes for propper targets expansion
###############################################################################

export kernel_procure_method=$(KERNEL_PROCURE_METHOD)
export vs_build_prepare_mem=$(VS_PREPARE_MEM)

###############################################################################
## Canned sequences
###############################################################################
## When multiple builds are triggered on the same build server that causes the docker image naming problem because
## all the build jobs are trying to create the same docker image with latest as tag.
## This happens only when sonic docker images are built using native host dockerd.
##
##     docker-swss:latest <=SAVE/LOAD=> docker-swss-<user>:<tag>

# $(call docker-get-tag,tag)
# Get the docker tag. For packages it is an image version, for other dockers it stays latest.
#
# $(1) => Docker name

define docker-get-tag
$(shell [ ! -z $(filter $(1).gz,$(SONIC_PACKAGES_LOCAL)) ] && [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ] && echo $(SONIC_IMAGE_VERSION) || echo latest)
endef

# $(call docker-image-save,from,to)
# Sonic docker images are always created with username as extension. During the save operation,
# it removes the username extension from docker image and saved them as compressed tar file for SONiC image generation.
# The save operation is protected with lock for parallel build.
#
# $(1) => Docker name
# $(2) => Docker target name

define docker-image-save
    @echo "Attempting docker image lock for $(1) save" $(LOG)
    $(call MOD_LOCK,$(1),$(DOCKER_LOCKDIR),$(DOCKER_LOCKFILE_SUFFIX),$(DOCKER_LOCKFILE_TIMEOUT))
    @echo "Obtained docker image lock for $(1) save" $(LOG)
    @echo "Tagging docker image $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG) as $(1):$(call docker-get-tag,$(1))" $(LOG)
    docker tag $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG) $(1):$(call docker-get-tag,$(1)) $(LOG)
    @echo "Saving docker image $(1):$(call docker-get-tag,$(1))" $(LOG)
        docker save $(1):$(call docker-get-tag,$(1)) | pigz -c > $(2)
    if [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ]; then
        @echo "Removing docker image $(1):$(call docker-get-tag,$(1))" $(LOG)
        docker rmi -f $(1):$(call docker-get-tag,$(1)) $(LOG)
    fi
    $(call MOD_UNLOCK,$(1))
    @echo "Released docker image lock for $(1) save" $(LOG)
    @echo "Removing docker image $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG)" $(LOG)
    docker rmi -f $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG) $(LOG)
endef

# $(call docker-image-load,from)
# Sonic docker images are always created with username as extension. During the load operation,
# it loads the docker image from compressed tar file and tag them with username as extension.
# The load operation is protected with lock for parallel build.
#
# $(1) => Docker name
# $(2) => Docker target name
define docker-image-load
    @echo "Attempting docker image lock for $(1) load" $(LOG)
    $(call MOD_LOCK,$(1),$(DOCKER_LOCKDIR),$(DOCKER_LOCKFILE_SUFFIX),$(DOCKER_LOCKFILE_TIMEOUT))
    @echo "Obtained docker image lock for $(1) load" $(LOG)
    @echo "Loading docker image $(TARGET_PATH)/$(1).gz" $(LOG)
    docker load -i $(TARGET_PATH)/$(1).gz $(LOG)
    @echo "Tagging docker image $(1):$(call docker-get-tag,$(1)) as $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG)" $(LOG)
    docker tag $(1):$(call docker-get-tag,$(1)) $(1)-$(DOCKER_USERNAME):$(DOCKER_USERTAG) $(LOG)
    if [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ]; then
        @echo "Removing docker image $(1):latest" $(LOG)
        docker rmi -f $(1):$(call docker-get-tag,$(1)) $(LOG)
    fi
    $(call MOD_UNLOCK,$(1))
    @echo "Released docker image lock for $(1) load" $(LOG)
endef

###############################################################################
## Local targets
###############################################################################

# Copy debian packages from local directory
# Add new package for copy:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_PATH = path/to/some_new_deb.deb
#     SONIC_COPY_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_COPY_DEBS)) : $(DEBS_PATH)/% : .platform \
	$(call dpkg_depend,$(DEBS_PATH)/%.dep)

	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		$(foreach deb,$* $($*_DERIVED_DEBS), \
			{ cp $($(deb)_PATH)/$(deb) $(DEBS_PATH)/ $(LOG) || exit 1 ; } ; )

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)

	fi

	$(FOOTER)


SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_COPY_DEBS))

# Copy regular files from local directory
# Add new package for copy:
#     SOME_NEW_FILE = some_new_file
#     $(SOME_NEW_FILE)_PATH = path/to/some_new_file
#     SONIC_COPY_FILES += $(SOME_NEW_FILE)
$(addprefix $(FILES_PATH)/, $(SONIC_COPY_FILES)) : $(FILES_PATH)/% : .platform
	$(HEADER)
	cp $($*_PATH)/$* $(FILES_PATH)/ $(LOG) || exit 1
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(FILES_PATH)/, $(SONIC_COPY_FILES))

###############################################################################
## Online targets
###############################################################################

# Download debian packages from online location
# Add new package for download:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_URL = https://url/to/this/deb.deb
#     SONIC_ONLINE_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_ONLINE_DEBS)) : $(DEBS_PATH)/% : .platform \
	$(call dpkg_depend,$(DEBS_PATH)/%.dep)

	$(HEADER)
	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache or exists in target/ directory
	if [ -z '$($*_CACHE_LOADED)' ] && [ ! -e $(DEBS_PATH)/$* ] ; then

		$(foreach deb,$* $($*_DERIVED_DEBS), \
			{ SKIP_BUILD_HOOK=$($*_SKIP_VERSION) curl -L -f -o $(DEBS_PATH)/$(deb) $($(deb)_CURL_OPTIONS) $($(deb)_URL) $(LOG) || { exit 1 ; } } ; )

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)
	fi
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_ONLINE_DEBS))

# Download regular files from online location
# Files are stored in deb packages directory for convenience
# Add new file for download:
#     SOME_NEW_FILE = some_new_file
#     $(SOME_NEW_FILE)_URL = https://url/to/this/file
#     SONIC_ONLINE_FILES += $(SOME_NEW_FILE)
$(addprefix $(FILES_PATH)/, $(SONIC_ONLINE_FILES)) : $(FILES_PATH)/% : .platform
	$(HEADER)
	SKIP_BUILD_HOOK=$($*_SKIP_VERSION) curl -L -f -o $@ $($*_CURL_OPTIONS) $($*_URL) $(LOG)
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(FILES_PATH)/, $(SONIC_ONLINE_FILES))

###############################################################################
## Build targets
###############################################################################

# Build project using build.sh script
# They are essentially a one-time build projects that get sources from some URL
# and compile them
# Add new file for build:
#     SOME_NEW_FILE = some_new_deb.deb
#     $(SOME_NEW_FILE)_SRC_PATH = $(SRC_PATH)/project_name
#     $(SOME_NEW_FILE)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ...
#     $(SOME_NEW_FILE)_PHONIES = $(SOME_PHONY_NAME) ...
#     SONIC_MAKE_FILES += $(SOME_NEW_FILE)
$(addprefix $(FILES_PATH)/, $(SONIC_MAKE_FILES)) : $(FILES_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) $$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) \
			$$(addprefix $(DEBS_PATH)/,$$($$*_AFTER)) \
			$(call dpkg_depend,$(FILES_PATH)/%.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

		# Skip building the target if it is already loaded from cache
		if [ -z '$($*_CACHE_LOADED)' ] ; then
		# Remove target to force rebuild
		rm -f $(addprefix $(FILES_PATH)/, $*)
		# Apply series of patches if exist
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*_SRC_PATH)).patch quilt push -a; popd; fi $(LOG)
		# Build project and take package
		make DEST=$(shell pwd)/$(FILES_PATH) -C $($*_SRC_PATH) $(shell pwd)/$(FILES_PATH)/$* $(LOG)
		# Clean up
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi $(LOG)

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)

	fi

	# Uninstall unneeded build dependency
	$(call UNINSTALL_DEBS,$($*_UNINSTALLS))

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(FILES_PATH)/, $(SONIC_MAKE_FILES))


# Phony build target
#
# Can be used to enforce strict build order in dependencies that cannot otherwise
# be done.
#
# Does not actually use .PHONY but instead a file.  This is to prevent unnecessary
# rebuilds of things like containers which would otherwise think a change occurred.
#
# Also omitted is dpkg_depend() since this is not a cacheable item.
#
# Add new phony:
#     SOME_PHONY = some_phony
#     $(SOME_PHONY)_DEPENDS = $(SOME_DEB) ...
#     $(SOME_PHONY)_WHEEL_DEPENDS = $(SOME_WHEEL) ...
#     SONIC_PHONIES += $(SOME_PHONY)
$(addprefix $(PHONY_PATH)/, $(SONIC_PHONIES)) : $(PHONY_PATH)/% : .platform \
			$$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
			$$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) \
			$$(addprefix $(FILES_PATH)/,$$($$*_FILES))
	$(HEADER)
	touch $@
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(PHONY_PATH)/, $(SONIC_PHONIES))


###############################################################################
## Debian package related targets
###############################################################################

# Build project using build.sh script
# They are essentially a one-time build projects that get sources from some URL
# and compile them
# Add new package for build:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name
#     $(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ...
#     $(SOME_NEW_DEB)_PHONIES = $(SOME_PHONY_NAME) ...
#     SONIC_MAKE_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_MAKE_DEBS)) : $(DEBS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
			$$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) \
			$$(addprefix $(DEBS_PATH)/,$$($$*_AFTER)) \
			$$(addprefix $(FILES_PATH)/,$$($$*_FILES)) \
			$$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) \
			$(call dpkg_depend,$(DEBS_PATH)/%.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		# Remove target to force rebuild
		rm -f $(addprefix $(DEBS_PATH)/, $* $($*_DERIVED_DEBS) $($*_EXTRA_DEBS))
		# Apply series of patches if exist
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*_SRC_PATH)).patch quilt push -a; popd; fi $(LOG)
		# Build project and take package
		$(SETUP_OVERLAYFS_FOR_DPKG_ADMINDIR)
		DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC}" $(ANT_DEB_CONFIG) $(CROSS_COMPILE_FLAGS) make -j$(SONIC_CONFIG_MAKE_JOBS) DEST=$(shell pwd)/$(DEBS_PATH) -C $($*_SRC_PATH) $(shell pwd)/$(DEBS_PATH)/$* $(LOG)
		# Clean up
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi $(LOG)

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)

	fi

	# Uninstall unneeded build dependency
	$(call UNINSTALL_DEBS,$($*_UNINSTALLS))

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_MAKE_DEBS))


# Build project with dpkg-buildpackage
# Add new package for build:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name
#     $(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ...
#     $(SOME_NEW_DEB)_PHONIES = $(SOME_PHONY_NAME) ...
#     SONIC_DPKG_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_DPKG_DEBS)) : $(DEBS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
			$$(addprefix $(DEBS_PATH)/,$$($$*_AFTER)) \
			$$(addprefix $(FILES_PATH)/,$$($$*_AFTER_FILES)) \
			$$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) \
			$(call dpkg_depend,$(DEBS_PATH)/%.dep )
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		# Remove old build logs if they exist
		rm -f $($*_SRC_PATH)/debian/*.debhelper.log
		# Apply series of patches if exist
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*_SRC_PATH)).patch quilt push -a; popd; fi $(LOG)
		# Build project
		pushd $($*_SRC_PATH) $(LOG_SIMPLE)
		if [ -f ./autogen.sh ]; then ./autogen.sh $(LOG); fi
		$(SETUP_OVERLAYFS_FOR_DPKG_ADMINDIR)
		$(if $($*_DPKG_TARGET),
			${$*_BUILD_ENV} DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC} ${$*_DEB_BUILD_OPTIONS}" DEB_BUILD_PROFILES="${$*_DEB_BUILD_PROFILES}" $(ANT_DEB_CONFIG) $(CROSS_COMPILE_FLAGS) timeout --preserve-status -s 9 -k 10 $(BUILD_PROCESS_TIMEOUT) dpkg-buildpackage -rfakeroot -b $(ANT_DEB_CROSS_OPT) -us -uc -tc -j$(SONIC_CONFIG_MAKE_JOBS) --as-root -T$($*_DPKG_TARGET) --admindir $$mergedir $(LOG),
			${$*_BUILD_ENV} DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC} ${$*_DEB_BUILD_OPTIONS}" DEB_BUILD_PROFILES="${$*_DEB_BUILD_PROFILES}" $(ANT_DEB_CONFIG) $(CROSS_COMPILE_FLAGS) timeout --preserve-status -s 9 -k 10 $(BUILD_PROCESS_TIMEOUT) dpkg-buildpackage -rfakeroot -b $(ANT_DEB_CROSS_OPT) -us -uc -tc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $$mergedir $(LOG)
		)
		popd $(LOG_SIMPLE)
		# Clean up
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi
		# Take built package(s)
		mv -f $(addprefix $($*_SRC_PATH)/../, $* $($*_DERIVED_DEBS) $($*_EXTRA_DEBS)) $(DEBS_PATH) $(LOG)

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)
	fi

	# Uninstall unneeded build dependency
	$(call UNINSTALL_DEBS,$($*_UNINSTALLS))

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_DPKG_DEBS))

# Rules for derived debian packages (dev, dbg, etc.)
# All noise takes place in main deb recipe, so we are just telling that
# we depend on it and move our deb to other targets
# Add new dev package:
#     $(eval $(call add_derived_package,$(ORIGINAL_DEB),derived_deb_file.deb))
$(addprefix $(DEBS_PATH)/, $(SONIC_DERIVED_DEBS)) : $(DEBS_PATH)/% : .platform $$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))
	$(HEADER)
	# All noise takes place in main deb recipe, so we are just telling that
	# we depend on it
	# Put newer timestamp
	[ -f $@ ] && touch $@
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_DERIVED_DEBS))

# Rules for extra debian packages
# All noise takes place in main deb recipe, so we are just telling that
# we need to build the main deb and move our deb to other targets
# Add new dev package:
#     $(eval $(call add_extra_package,$(ORIGINAL_DEB),extra_deb_file.deb))
$(addprefix $(DEBS_PATH)/, $(SONIC_EXTRA_DEBS)) : $(DEBS_PATH)/% : .platform $$(addprefix $(DEBS_PATH)/,$$($$*_MAIN_DEB))
	$(HEADER)
	# All noise takes place in main deb recipe, so we are just telling that
	# we depend on it
	# Put newer timestamp
	[ -f $@ ] && touch $@
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(DEBS_PATH)/, $(SONIC_EXTRA_DEBS))

# Targets for installing debian packages prior to build one that depends on them
SONIC_INSTALL_DEBS = $(addsuffix -install,$(addprefix $(DEBS_PATH)/, \
			$(SONIC_ONLINE_DEBS) \
			$(SONIC_COPY_DEBS) \
			$(SONIC_MAKE_DEBS) \
			$(SONIC_DPKG_DEBS) \
			$(SONIC_PYTHON_STDEB_DEBS) \
			$(SONIC_DERIVED_DEBS) \
			$(SONIC_EXTRA_DEBS)))
$(SONIC_INSTALL_DEBS) : $(DEBS_PATH)/%-install : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) $$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) $(DEBS_PATH)/$$*
	$(HEADER)
	[ -f $(DEBS_PATH)/$* ] || { echo $(DEBS_PATH)/$* does not exist $(LOG) && false $(LOG) }
	for i in {1..360}; do
		# wait for conflicted packages to be uninstalled
		$(foreach deb, $($*_CONFLICT_DEBS), \
			{ while dpkg -s $(firstword $(subst _, ,$(basename $(deb)))) | grep "^Version: $(word 2, $(subst _, ,$(basename $(deb))))" &> /dev/null; do echo "waiting for $(deb) to be uninstalled" $(LOG); sleep 1; done } )
		# put a lock here because dpkg does not allow installing packages in parallel
		if mkdir $(DEBS_PATH)/dpkg_lock &> /dev/null; then
ifneq ($(CROSS_BUILD_ENVIRON),y)
			{ sudo DEBIAN_FRONTEND=noninteractive $($*_DEB_INSTALL_OPTS) dpkg -i $(DEBS_PATH)/$* $(LOG) && rm -d $(DEBS_PATH)/dpkg_lock && break; } || { set +e; rm -d $(DEBS_PATH)/dpkg_lock; sudo lsof /var/lib/dpkg/lock-frontend; ps aux; exit 1 ; }
else
			# Relocate debian packages python libraries to the cross python virtual env location
			{ sudo DEBIAN_FRONTEND=noninteractive $($*_DEB_INSTALL_OPTS) dpkg -i $(if $(findstring $(LINUX_HEADERS),$*),--force-depends) $(DEBS_PATH)/$* $(LOG) && \
			rm -rf tmp && mkdir tmp && dpkg -x $(DEBS_PATH)/$* tmp && (sudo cp -rf tmp/usr/lib/python2*/dist-packages/* $(VIRTENV_LIB_CROSS_PYTHON2)/python2*/site-packages/ 2>/dev/null || true) && \
			(sudo cp -rf tmp/usr/lib/python3/dist-packages/* $(VIRTENV_LIB_CROSS_PYTHON3)/python3.*/site-packages/ 2>/dev/null || true) && \
			rm -d $(DEBS_PATH)/dpkg_lock && break; } || { set +e; rm -d $(DEBS_PATH)/dpkg_lock; sudo lsof /var/lib/dpkg/lock-frontend; ps aux; exit 1 ; }
endif
		fi
		sleep 10
	done
	$(FOOTER)


###############################################################################
## Python packages
###############################################################################

# Build project with python setup.py --command-packages=stdeb.command
# Add new package for build:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_SRC_PATH = $(SRC_PATH)/project_name
#     $(SOME_NEW_DEB)_DEPENDS = $(SOME_OTHER_DEB1) $(SOME_OTHER_DEB2) ...
#     $(SOME_NEW_DEB)_PHONIES += $(SOME_PHONY_NAME)
#     SONIC_PYTHON_STDEB_DEBS += $(SOME_NEW_DEB)
$(addprefix $(PYTHON_DEBS_PATH)/, $(SONIC_PYTHON_STDEB_DEBS)) : $(PYTHON_DEBS_PATH)/% : .platform \
		$$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEBS_DEPENDS))) \
		$$(addsuffix -install,$$(addprefix $(PYTHON_DEBS_PATH)/,$$($$*_DEPENDS))) \
		$$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) \
		$$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) \
		$(call dpkg_depend,$(PYTHON_DEBS_PATH)/%.dep)

	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		# Apply series of patches if exist
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*_SRC_PATH)).patch quilt push -a; popd; fi $(LOG)
		# Build project
		pushd $($*_SRC_PATH) $(LOG_SIMPLE)
		rm -rf deb_dist/* $(LOG)
		python setup.py --command-packages=stdeb.command bdist_deb $(LOG)
		popd $(LOG_SIMPLE)
		# Clean up
		if [ -f $($*_SRC_PATH).patch/series ]; then pushd $($*_SRC_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi $(LOG)
		# Take built package(s)
		mv -f $(addprefix $($*_SRC_PATH)/deb_dist/, $* $($*_DERIVED_DEBS)) $(PYTHON_DEBS_PATH) $(LOG)

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)
	fi

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(PYTHON_DEBS_PATH)/, $(SONIC_PYTHON_STDEB_DEBS))

# Build project using python setup.py bdist_wheel
# Projects that generate python wheels
# Add new package for build:
#     SOME_NEW_WHL = some_new_whl.whl
#     $(SOME_NEW_WHL)_SRC_PATH = $(SRC_PATH)/project_name
#     $(SOME_NEW_WHL)_PYTHON_VERSION = 2 (or 3)
#     $(SOME_NEW_WHL)_DEPENDS = $(SOME_OTHER_WHL1) $(SOME_OTHER_WHL2) ...
#     $(SOME_NEW_WHL)_PHONIES = $(SOME_PHONY_NAME) ...
#     SONIC_PYTHON_WHEELS += $(SOME_NEW_WHL)
$(addprefix $(PYTHON_WHEELS_PATH)/, $(SONIC_PYTHON_WHEELS)) : $(PYTHON_WHEELS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_DEPENDS))) $$(addprefix $(PHONY_PATH)/,$$($$*_PHONIES)) \
			$(call dpkg_depend,$(PYTHON_WHEELS_PATH)/%.dep) \
			$$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEBS_DEPENDS)))
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		pushd $($*_SRC_PATH) $(LOG_SIMPLE)
		# apply series of patches if exist
		if [ -f ../$(notdir $($*_SRC_PATH)).patch/series ]; then ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*_SRC_PATH)).patch quilt push -a; fi $(LOG)
ifneq ($(CROSS_BUILD_ENVIRON),y)
		# Use pip instead of later setup.py to install dependencies into user home, but uninstall self
		{ pip$($*_PYTHON_VERSION) install . && pip$($*_PYTHON_VERSION) uninstall --yes `python$($*_PYTHON_VERSION) setup.py --name`; } $(LOG)
ifneq ($(filter bookworm trixie,$(BLDENV)),)
		if [ ! "$($*_TEST)" = "n" ] && [ ! "$(BUILD_SKIP_TEST)" = "y" ]; then pip$($*_PYTHON_VERSION) install ".[testing]" && pip$($*_PYTHON_VERSION) uninstall --yes `python$($*_PYTHON_VERSION) setup.py --name` && timeout --preserve-status -s 9 -k 10 $(BUILD_PROCESS_TIMEOUT) python$($*_PYTHON_VERSION) -m pytest; fi $(LOG)
		python$($*_PYTHON_VERSION) -m build -n $(LOG)
else
		if [ ! "$($*_TEST)" = "n" ] && [ ! "$(BUILD_SKIP_TEST)" = "y" ]; then timeout --preserve-status -s 9 -k 10 $(BUILD_PROCESS_TIMEOUT) python$($*_PYTHON_VERSION) setup.py test $(LOG); fi
		python$($*_PYTHON_VERSION) setup.py bdist_wheel $(LOG)
endif
else
		{
			export PATH=$(VIRTENV_BIN_CROSS_PYTHON$($*_PYTHON_VERSION)):${PATH}
			python$($*_PYTHON_VERSION) setup.py build $(LOG)
			if [ ! "$($*_TEST)" = "n" ] && [ ! "$(BUILD_SKIP_TEST)" = "y" ]; then timeout --preserve-status -s 9 -k 10 $(BUILD_PROCESS_TIMEOUT) python$($*_PYTHON_VERSION) setup.py test $(LOG); fi
			python$($*_PYTHON_VERSION) setup.py bdist_wheel $(LOG)
		}
endif
		# clean up
		if [ -f ../$(notdir $($*_SRC_PATH)).patch/series ]; then quilt pop -a -f; [ -d .pc ] && rm -rf .pc; fi $(LOG)
		popd $(LOG_SIMPLE)
		mv -f $($*_SRC_PATH)/dist/$* $(PYTHON_WHEELS_PATH) $(LOG)

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*,$@)
	fi

	# Uninstall unneeded build dependency
	$(call UNINSTALL_DEBS,$($*_UNINSTALLS))

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(PYTHON_WHEELS_PATH)/, $(SONIC_PYTHON_WHEELS))

# Targets for installing python wheels.
# Autogenerated
SONIC_INSTALL_WHEELS = $(addsuffix -install, $(addprefix $(PYTHON_WHEELS_PATH)/, $(SONIC_PYTHON_WHEELS)))
$(SONIC_INSTALL_WHEELS) : $(PYTHON_WHEELS_PATH)/%-install : .platform $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_DEPENDS))) $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEBS_DEPENDS))) $(PYTHON_WHEELS_PATH)/$$*
	$(HEADER)
	[ -f $(PYTHON_WHEELS_PATH)/$* ] || { echo $(PYTHON_WHEELS_PATH)/$* does not exist $(LOG) && exit 1; }
	# put a lock here to avoid race conditions
	while true; do
	if mkdir $(PYTHON_WHEELS_PATH)/pip_lock &> /dev/null; then
ifneq ($(CROSS_BUILD_ENVIRON),y)
	{ sudo -E SKIP_BUILD_HOOK=Y pip$($*_PYTHON_VERSION) install $(PYTHON_WHEELS_PATH)/$* $(LOG) && rm -d $(PYTHON_WHEELS_PATH)/pip_lock && break; } || { rm -d $(PYTHON_WHEELS_PATH)/pip_lock && exit 1 ; }
else
	# Link python script and data expected location to the cross python virtual env istallation locations
	{ PATH=$(VIRTENV_BIN_CROSS_PYTHON$($*_PYTHON_VERSION)):${PATH} sudo -E $(VIRTENV_BIN_CROSS_PYTHON$($*_PYTHON_VERSION))/pip$($*_PYTHON_VERSION) install $(PYTHON_WHEELS_PATH)/$* $(LOG) && $(if $(findstring $(SONIC_CONFIG_ENGINE_PY3),$*),(sudo ln -s $(VIRTENV_BIN_CROSS_PYTHON$($*_PYTHON_VERSION))/sonic-cfggen /usr/local/bin/sonic-cfggen 2>/dev/null || true), true ) && $(if $(findstring $(SONIC_YANG_MODELS_PY3),$*),(sudo ln -s $(VIRTENV_BASE_CROSS_PYTHON3)/yang-models /usr/local/yang-models 2>/dev/null || true), true ) && rm -d $(PYTHON_WHEELS_PATH)/pip_lock && break; } || { rm -d $(PYTHON_WHEELS_PATH)/pip_lock && exit 1 ; }
endif
	fi
	done
	$(FOOTER)

###############################################################################
## Docker images related targets
###############################################################################

# start docker daemon
docker-start :
	$(Q)sudo sed -i 's/--storage-driver=vfs/--storage-driver=$(SONIC_SLAVE_DOCKER_DRIVER)/' /etc/default/docker
	$(Q)sudo sed -i -e '/http_proxy/d' -e '/https_proxy/d' /etc/default/docker
	$(Q)sudo bash -c "{ echo \"export http_proxy=$$http_proxy\"; \
	            echo \"export https_proxy=$$https_proxy\"; \
	            echo \"export no_proxy=$$no_proxy\"; } >> /etc/default/docker"
	$(Q)test x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) != x"y" && sudo service docker status &> /dev/null || ( sudo service docker start &> /dev/null && ./scripts/wait_for_docker.sh 60 )

# targets for building simple docker images that do not depend on any debian packages
$(addprefix $(TARGET_PATH)/, $(SONIC_SIMPLE_DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform docker-start $$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$($$*.gz_LOAD_DOCKERS)))
	$(HEADER)
	# Apply series of patches if exist
	if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*.gz_PATH)).patch quilt push -a; popd; fi $(LOG)
	# Prepare docker build info
	SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
	TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
	SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
	DBGOPT='$(DBGOPT)' \
	scripts/prepare_docker_buildinfo.sh $* $($*.gz_PATH)/Dockerfile $(CONFIGURED_ARCH) $(TARGET_DOCKERFILE)/Dockerfile.buildinfo $(LOG)
	docker info $(LOG)
	docker build --no-cache \
		--build-arg http_proxy=$(HTTP_PROXY) \
		--build-arg https_proxy=$(HTTPS_PROXY) \
		--build-arg no_proxy=$(NO_PROXY) \
		--build-arg user=$(USER) \
		--build-arg uid=$(UID) \
		--build-arg guid=$(GUID) \
		--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
		--label Tag=$(SONIC_IMAGE_VERSION) \
		-f $(TARGET_DOCKERFILE)/Dockerfile.buildinfo \
		-t $(DOCKER_IMAGE_REF) $($*.gz_PATH) $(LOG)

	if [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ]; then docker tag $(DOCKER_IMAGE_REF) $*; fi
	SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) ARCH=${CONFIGURED_ARCH} \
		DBGOPT='$(DBGOPT)' \
		scripts/collect_docker_version_files.sh $* $(TARGET_PATH) $(DOCKER_IMAGE_REF) $($*.gz_PATH) $(LOG)

	$(call docker-image-save,$*,$@)

	# Clean up
	if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_SIMPLE_DOCKER_IMAGES))

DOCKER_IMAGES_FOR_INSTALLERS := $(sort $(foreach installer,$(SONIC_INSTALLERS),$($(installer)_DOCKERS)))

$(foreach DOCKER_IMAGE,$(SONIC_JESSIE_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(JESSIE_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_JESSIE_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(JESSIE_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_JESSIE_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(JESSIE_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_JESSIE_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(JESSIE_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_STRETCH_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(STRETCH_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_STRETCH_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(STRETCH_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_STRETCH_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(STRETCH_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_STRETCH_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(STRETCH_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BUSTER_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BUSTER_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BUSTER_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BUSTER_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BUSTER_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BUSTER_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BUSTER_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BUSTER_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BULLSEYE_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BULLSEYE_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BULLSEYE_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BULLSEYE_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BULLSEYE_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BULLSEYE_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BULLSEYE_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BULLSEYE_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BOOKWORM_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BOOKWORM_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BOOKWORM_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BOOKWORM_FILES_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BOOKWORM_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_DEBS_PATH := $(BOOKWORM_DEBS_PATH)))
$(foreach DOCKER_IMAGE,$(SONIC_BOOKWORM_DBG_DOCKERS), $(eval $(DOCKER_IMAGE)_FILES_PATH := $(BOOKWORM_FILES_PATH)))

ifeq ($(BLDENV),jessie)
	DOCKER_IMAGES := $(SONIC_JESSIE_DOCKERS)
	DOCKER_DBG_IMAGES := $(SONIC_JESSIE_DBG_DOCKERS)
	JESSIE_DOCKER_IMAGES = $(filter $(SONIC_JESSIE_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS))
	JESSIE_DBG_DOCKER_IMAGES = $(filter $(SONIC_JESSIE_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS))
else
ifeq ($(BLDENV),stretch)
	DOCKER_IMAGES := $(SONIC_STRETCH_DOCKERS)
	DOCKER_DBG_IMAGES := $(SONIC_STRETCH_DBG_DOCKERS)
	STRETCH_DOCKER_IMAGES = $(filter $(SONIC_STRETCH_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS))
	STRETCH_DBG_DOCKER_IMAGES = $(filter $(SONIC_STRETCH_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS))
else
ifeq ($(BLDENV),buster)
	DOCKER_IMAGES := $(SONIC_BUSTER_DOCKERS)
	DOCKER_DBG_IMAGES := $(SONIC_BUSTER_DBG_DOCKERS)
	BUSTER_DOCKER_IMAGES = $(filter $(SONIC_BUSTER_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
	BUSTER_DBG_DOCKER_IMAGES = $(filter $(SONIC_BUSTER_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
else
ifeq ($(BLDENV),bullseye)
	DOCKER_IMAGES := $(SONIC_BULLSEYE_DOCKERS)
	DOCKER_DBG_IMAGES := $(SONIC_BULLSEYE_DBG_DOCKERS)
	BULLSEYE_DOCKER_IMAGES = $(filter $(SONIC_BULLSEYE_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
	BULLSEYE_DBG_DOCKER_IMAGES = $(filter $(SONIC_BULLSEYE_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
else
ifeq ($(BLDENV),bookworm)
	DOCKER_IMAGES := $(SONIC_BOOKWORM_DOCKERS)
	DOCKER_DBG_IMAGES := $(SONIC_BOOKWORM_DBG_DOCKERS)
	BOOKWORM_DOCKER_IMAGES = $(filter $(SONIC_BOOKWORM_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
	BOOKWORM_DBG_DOCKER_IMAGES = $(filter $(SONIC_BOOKWORM_DBG_DOCKERS),$(DOCKER_IMAGES_FOR_INSTALLERS) $(EXTRA_DOCKER_TARGETS) $(SONIC_PACKAGES_LOCAL))
else
	DOCKER_IMAGES = $(filter-out $(SONIC_JESSIE_DOCKERS) $(SONIC_STRETCH_DOCKERS) $(SONIC_BUSTER_DOCKERS) $(SONIC_BULLSEYE_DOCKERS) $(SONIC_BOOKWORM_DOCKERS),$(SONIC_DOCKER_IMAGES))
	DOCKER_DBG_IMAGES = $(filter-out $(SONIC_JESSIE_DBG_DOCKERS) $(SONIC_STRETCH_DBG_DOCKERS) $(SONIC_BUSTER_DBG_DOCKERS) $(SONIC_BULLSEYE_DBG_DOCKERS) $(SONIC_BOOKWORM_DBG_DOCKERS), $(SONIC_DOCKER_DBG_IMAGES))
endif
endif
endif
endif
endif

$(foreach IMAGE,$(DOCKER_IMAGES), $(eval $(IMAGE)_DEBS_PATH := $(DEBS_PATH)))
$(foreach IMAGE,$(DOCKER_IMAGES), $(eval $(IMAGE)_FILES_PATH := $(FILES_PATH)))
$(foreach IMAGE,$(DOCKER_DBG_IMAGES), $(eval $(IMAGE)_DEBS_PATH := $(DEBS_PATH)))
$(foreach IMAGE,$(DOCKER_DBG_IMAGES), $(eval $(IMAGE)_FILES_PATH := $(FILES_PATH)))

# Targets for downloaded docker images
$(addprefix $(TARGET_PATH)/,$(DOWNLOADED_DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform \
		$$(%.gz_DEP_FILES)
	$(HEADER)

	rm -rf $@ $@.log
	wget "$($*.gz_URL)" -O target/$(DOWNLOADED_DOCKER_IMAGES) $(LOG)

	$(FOOTER)

# Targets for copy docker images
$(addprefix $(TARGET_PATH)/,$(COPY_DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform \
		$$(%.gz_DEP_FILES)
	$(HEADER)

	rm -rf $@ $@.log
	cp "$($*.gz_PATH)/$*.gz" target/$(COPY_DOCKER_IMAGES) $(LOG)

	$(FOOTER)

# Targets for building docker images
$(addprefix $(TARGET_PATH)/, $(DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform docker-start \
		$$(addprefix $$($$*.gz_DEBS_PATH)/,$$($$*.gz_DEPENDS)) \
		$$(addprefix $(TARGET_PATH)/,$$($$*.gz_AFTER)) \
		$$(addprefix $$($$*.gz_FILES_PATH)/,$$($$*.gz_FILES)) \
		$$(addprefix $(PYTHON_DEBS_PATH)/,$$($$*.gz_PYTHON_DEBS)) \
		$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*.gz_PYTHON_WHEELS)) \
		$$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$($$*.gz_LOAD_DOCKERS))) \
		$$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*.gz_INSTALL_PYTHON_WHEELS))) \
		$$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*.gz_INSTALL_DEBS))) \
		$$($$*.gz_PATH)/Dockerfile.j2 \
		$(call dpkg_depend,$(TARGET_PATH)/%.gz.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*.gz,$@)
	$(eval $*_CACHE_LOADED:=$($*.gz_CACHE_LOADED))
	# Skip building the target if it is already loaded from cache
	if [ -z '$($*.gz_CACHE_LOADED)' ] ; then

		# Apply series of patches if exist
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*.gz_PATH)).patch quilt push -a; popd; fi $(LOG)
		mkdir -p $($*.gz_PATH)/debs $(LOG)
		mkdir -p $($*.gz_PATH)/files $(LOG)
		mkdir -p $($*.gz_PATH)/python-debs $(LOG)
		mkdir -p $($*.gz_PATH)/python-wheels $(LOG)
		mkdir -p $(TARGET_PATH)/vcache/$* $($*.gz_PATH)/vcache $(LOG)
		sudo mount --bind $($*.gz_DEBS_PATH) $($*.gz_PATH)/debs $(LOG)
		sudo mount --bind $($*.gz_FILES_PATH) $($*.gz_PATH)/files $(LOG)
		sudo mount --bind $(PYTHON_DEBS_PATH) $($*.gz_PATH)/python-debs $(LOG)
		sudo mount --bind $(PYTHON_WHEELS_PATH) $($*.gz_PATH)/python-wheels $(LOG)
		# Export variables for j2. Use path for unique variable names, e.g. docker_orchagent_debs
		export include_system_eventd="$(INCLUDE_SYSTEM_EVENTD)"
		export build_reduce_image_size="$(BUILD_REDUCE_IMAGE_SIZE)"
		export sonic_asic_platform="$(patsubst %-$(CONFIGURED_ARCH),%,$(CONFIGURED_PLATFORM))"
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_debs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DEPENDS),RDEPENDS))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_pydebs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_PYTHON_DEBS)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_whls=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_PYTHON_WHEELS)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_PACKAGES)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_pkgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_APT_PACKAGES)))\n" | awk '!a[$$0]++'))
		if [ -d $($*.gz_PATH)/cli-plugin-tests/ ] && [ ! "$(BUILD_SKIP_TEST)" = "y" ]; then pushd $($*.gz_PATH)/cli-plugin-tests; PATH=$(VIRTENV_BIN_CROSS_PYTHON$($(SONIC_UTILITIES_PY3)_PYTHON_VERSION)):${PATH} PYTHONPATH=$(shell realpath $($*.gz_PATH)):${PYTHONPATH} pytest-$($(SONIC_UTILITIES_PY3)_PYTHON_VERSION) -v $(LOG); popd; fi
		# Label docker image with componenets versions
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_labels=$(foreach component,\
			$(call expand,$($*.gz_DEPENDS),RDEPENDS) \
			$(call expand,$($*.gz_PYTHON_DEBS)) \
			$(call expand,$($*.gz_PYTHON_WHEELS)),\
			$(shell [[ ! -z "$($(component)_VERSION)" && ! -z "$($(component)_NAME)" ]] && \
				echo "--label com.azure.sonic.versions.$($(component)_NAME)=$($(component)_VERSION)")))
		j2 $($*.gz_PATH)/Dockerfile.j2 > $($*.gz_PATH)/Dockerfile
		$(call generate_manifest,$*)
		# Prepare docker build info
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
		DBGOPT='$(DBGOPT)' \
		scripts/prepare_docker_buildinfo.sh $* $($*.gz_PATH)/Dockerfile $(CONFIGURED_ARCH) $(LOG)
		docker info $(LOG)
		docker build --no-cache \
			--build-arg http_proxy=$(HTTP_PROXY) \
			--build-arg https_proxy=$(HTTPS_PROXY) \
			--build-arg no_proxy=$(NO_PROXY) \
			--build-arg user=$(USER) \
			--build-arg uid=$(UID) \
			--build-arg guid=$(GUID) \
			--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
			--build-arg frr_user_uid=$(FRR_USER_UID) \
			--build-arg frr_user_gid=$(FRR_USER_GID) \
			--build-arg SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
			--build-arg SONIC_VERSION_CACHE_SOURCE=$(SONIC_VERSION_CACHE_SOURCE) \
			--build-arg image_version=$(SONIC_IMAGE_VERSION) \
			--label com.azure.sonic.manifest="$$(cat $($*.gz_PATH)/manifest.json)" \
			--label Tag=$(SONIC_IMAGE_VERSION) \
		        $($(subst -,_,$(notdir $($*.gz_PATH)))_labels) \
			-t $(DOCKER_IMAGE_REF) $($*.gz_PATH) $(LOG)

		if [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ]; then docker tag $(DOCKER_IMAGE_REF) $*; fi
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) ARCH=${CONFIGURED_ARCH}\
			DBGOPT='$(DBGOPT)' \
			scripts/collect_docker_version_files.sh $* $(TARGET_PATH) $(DOCKER_IMAGE_REF) $($*.gz_PATH) $($*.gz_PATH)/Dockerfile $(LOG)
		if [ ! -z $(filter $*.gz,$(SONIC_PACKAGES_LOCAL)) ]; then docker tag $(DOCKER_IMAGE_REF) $*:$(SONIC_IMAGE_VERSION); fi

		$(call docker-image-save,$*,$@)

		# Clean up
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*.gz,$@)
	fi

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(DOCKER_IMAGES))

# Targets for building docker debug images
$(addprefix $(TARGET_PATH)/, $(DOCKER_DBG_IMAGES)) : $(TARGET_PATH)/%-$(DBG_IMAGE_MARK).gz : .platform docker-start \
		$$(addprefix $(TARGET_PATH)/,$$($$*.gz_AFTER)) \
		$$(addprefix $$($$*.gz_DEBS_PATH)/,$$($$*.gz_DBG_DEPENDS)) \
		$$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$*.gz)) \
		$(call dpkg_depend,$(TARGET_PATH)/%-$(DBG_IMAGE_MARK).gz.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*-$(DBG_IMAGE_MARK).gz,$@)
	$(eval $*_CACHE_LOADED:=$($*-$(DBG_IMAGE_MARK).gz_CACHE_LOADED))
	# Skip building the target if it is already loaded from cache
	if [ -z '$($*-$(DBG_IMAGE_MARK).gz_CACHE_LOADED)' ] ; then

		mkdir -p $($*.gz_PATH)/debs $(LOG)
		sudo mount --bind $($*.gz_DEBS_PATH) $($*.gz_PATH)/debs $(LOG)
		mkdir -p $(TARGET_PATH)/vcache/$*-dbg $($*.gz_PATH)/vcache $(LOG)
		# Export variables for j2. Use path for unique variable names, e.g. docker_orchagent_debs
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_debs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_DEPENDS),RDEPENDS))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_image_dbgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_IMAGE_PACKAGES)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_pkgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_APT_PACKAGES),RDEPENDS))\n" | awk '!a[$$0]++'))
		./build_debug_docker_j2.sh $(DOCKER_IMAGE_REF) $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_debs $(subst -,_,$(notdir $($*.gz_PATH)))_image_dbgs > $($*.gz_PATH)/Dockerfile-dbg.j2
		j2 $($*.gz_PATH)/Dockerfile-dbg.j2 > $($*.gz_PATH)/Dockerfile-dbg
		$(call generate_manifest,$*,dbg)
		# Prepare docker build info
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
		DBGOPT='$(DBGOPT)' \
		scripts/prepare_docker_buildinfo.sh $*-dbg $($*.gz_PATH)/Dockerfile-dbg $(CONFIGURED_ARCH) $(LOG)
		docker info $(LOG)
		docker build \
			--no-cache \
			--build-arg http_proxy=$(HTTP_PROXY) \
			--build-arg https_proxy=$(HTTPS_PROXY) \
			--build-arg no_proxy=$(NO_PROXY) \
			--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
			--build-arg SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
			--build-arg SONIC_VERSION_CACHE_SOURCE=$(SONIC_VERSION_CACHE_SOURCE) \
			--label com.azure.sonic.manifest="$$(cat $($*.gz_PATH)/manifest.json)" \
			--label Tag=$(SONIC_IMAGE_VERSION) \
			--file $($*.gz_PATH)/Dockerfile-dbg \
			-t $(DOCKER_DBG_IMAGE_REF) $($*.gz_PATH) $(LOG)

		if [ x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) == x"y" ]; then docker tag $(DOCKER_IMAGE_REF) $*; fi
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) ARCH=${CONFIGURED_ARCH}\
			DBGOPT='$(DBGOPT)' \
			scripts/collect_docker_version_files.sh $*-dbg $(TARGET_PATH) $(DOCKER_DBG_IMAGE_REF) $($*.gz_PATH)  $($*.gz_PATH)/Dockerfile-dbg $(LOG)
		if [ ! -z $(filter $*.gz,$(SONIC_PACKAGES_LOCAL)) ]; then docker tag $(DOCKER_IMAGE_REF) $*:$(SONIC_IMAGE_VERSION); fi

		$(call docker-image-save,$*-$(DBG_IMAGE_MARK),$@)

		# Clean up
		docker rmi -f $(DOCKER_IMAGE_REF) &> /dev/null || true
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*-$(DBG_IMAGE_MARK).gz,$@)
	fi

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(DOCKER_DBG_IMAGES))

DOCKER_LOAD_TARGETS = $(addsuffix -load,$(addprefix $(TARGET_PATH)/, \
		      $(SONIC_SIMPLE_DOCKER_IMAGES) \
		      $(DOWNLOADED_DOCKER_IMAGES) \
		      $(COPY_DOCKER_IMAGES) \
		      $(DOCKER_IMAGES) \
		      $(DOCKER_DBG_IMAGES)))

ifeq ($(BLDENV),trixie)
DOCKER_LOAD_TARGETS += $(addsuffix -load,$(addprefix $(TARGET_PATH)/, \
		      $(SONIC_JESSIE_DOCKERS) \
		      $(SONIC_STRETCH_DOCKERS) \
		      $(SONIC_BUSTER_DOCKERS) \
		      $(SONIC_BULLSEYE_DOCKERS) \
		      $(SONIC_BOOKWORM_DOCKERS)))

endif

$(DOCKER_LOAD_TARGETS) : $(TARGET_PATH)/%.gz-load : .platform docker-start $$(TARGET_PATH)/$$*.gz
	$(HEADER)
	$(call docker-image-load,$*)
	$(FOOTER)

###############################################################################
## Installers
###############################################################################

$(addprefix $(TARGET_PATH)/, $(SONIC_RFS_TARGETS)) : $(TARGET_PATH)/% : \
        .platform \
        build_debian.sh \
        $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(INITRAMFS_TOOLS) $(LINUX_KERNEL) $(GRUB2_COMMON)) \
        $$(addprefix $(TARGET_PATH)/,$$($$*_DEPENDENT_RFS)) \
        $(call dpkg_depend,$(TARGET_PATH)/%.dep)
	$(HEADER)

    # $(call LOAD_CACHE,$*,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*_CACHE_LOADED)' ] ; then

		$(eval installer=$($*_INSTALLER))
		$(eval machine=$($*_MACHINE))

		export debs_path="$(IMAGE_DISTRO_DEBS_PATH)"
		export initramfs_tools="$(IMAGE_DISTRO_DEBS_PATH)/$(INITRAMFS_TOOLS)"
		export linux_kernel="$(IMAGE_DISTRO_DEBS_PATH)/$(LINUX_KERNEL)"
		export kversion="$(KVERSION)"
		export image_type="$($(installer)_IMAGE_TYPE)"
		export sonicadmin_user="$(USERNAME)"
		export sonic_asic_platform="$(patsubst %-$(CONFIGURED_ARCH),%,$(CONFIGURED_PLATFORM))"
		export RFS_SPLIT_FIRST_STAGE=y
		export RFS_SPLIT_LAST_STAGE=n

		j2 -f env files/initramfs-tools/union-mount.j2 onie-image.conf > files/initramfs-tools/union-mount
		j2 -f env files/initramfs-tools/arista-convertfs.j2 onie-image.conf > files/initramfs-tools/arista-convertfs

		RFS_SQUASHFS_NAME=$* \
		USERNAME="$(USERNAME)" \
		PASSWORD="$(PASSWORD)" \
		CHANGE_DEFAULT_PASSWORD="$(CHANGE_DEFAULT_PASSWORD)" \
		BMC_NOS_ACCOUNT_USERNAME="$(BMC_NOS_ACCOUNT_USERNAME)" \
		BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD="$(BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD)" \
		TARGET_MACHINE=$(machine) \
		IMAGE_TYPE=$($(installer)_IMAGE_TYPE) \
		TARGET_PATH=$(TARGET_PATH) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		DBGOPT='$(DBGOPT)' \
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
		MULTIARCH_QEMU_ENVIRON=$(MULTIARCH_QEMU_ENVIRON) \
		CROSS_BUILD_ENVIRON=$(CROSS_BUILD_ENVIRON) \
		MASTER_KUBERNETES_VERSION=$(MASTER_KUBERNETES_VERSION) \
		MASTER_CRI_DOCKERD=$(MASTER_CRI_DOCKERD) \
			./build_debian.sh $(LOG)

		$(call SAVE_CACHE,$*,$@)

	fi

	$(FOOTER)

# targets for building installers with base image
$(addprefix $(TARGET_PATH)/, $(SONIC_INSTALLERS)) : $(TARGET_PATH)/% : \
        .platform \
        onie-image.conf \
        build_debian.sh \
        scripts/dbg_files.sh \
        build_image.sh \
        $$(addsuffix -install,$$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$$($$*_DEPENDS))) \
        $$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$$($$*_INSTALLS)) \
        $$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$$($$*_LAZY_INSTALLS)) \
        $$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$$($$*_LAZY_BUILD_INSTALLS)) \
		$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_PYTHON_WHEELS)) \
        $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(INITRAMFS_TOOLS) \
                $(LINUX_KERNEL) \
                $(LINUX_KBUILD) \
                $(SONIC_DEVICE_DATA) \
                $(IFUPDOWN2) \
                $(KDUMP_TOOLS) \
                $(LIBPAM_RADIUS) \
                $(LIBNSS_RADIUS) \
                $(LIBPAM_TACPLUS) \
                $(LIBNSS_TACPLUS) \
                $(MONIT) \
                $(OPENSSH_SERVER) \
                $(PYTHON3_SWSSCOMMON) \
                $(SONIC_DB_CLI) \
                $(SONIC_NETTOOLS) \
                $(SONIC_RSYSLOG_PLUGIN) \
                $(SONIC_UTILITIES_DATA) \
                $(SONIC_CTRMGRD_RS) \
                $(SONIC_HOST_SERVICES_RS) \
                $(SONIC_HOST_SERVICES_DATA) \
                $(BASH) \
                $(BASH_TACPLUS) \
                $(AUDISP_TACPLUS) \
                $(SYSLOG_COUNTER) \
                $(GRUB2_COMMON)) \
        $$(addprefix $(TARGET_PATH)/,$$($$*_DOCKERS)) \
        $$(addprefix $(TARGET_PATH)/,$$(SONIC_PACKAGES_LOCAL)) \
        $$(addprefix $(FILES_PATH)/,$$($$*_FILES)) \
        $(if $(findstring y,$(ENABLE_ZTP)),$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(SONIC_ZTP))) \
        $(if $(findstring y,$(INCLUDE_FIPS)),$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(SYMCRYPT_OPENSSL))) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_UTILITIES_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_COMMON_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(REDIS_DUMP_LOAD_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_API_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_API_PY3)) \
        $(if $(findstring y,$(PDDF_SUPPORT)),$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY2))) \
        $(if $(findstring y,$(PDDF_SUPPORT)),$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY3))) \
        $(if $(findstring amd64,$(CONFIGURED_ARCH)),$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(RASDAEMON))) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MODELS_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CTRMGRD)) \
        $(addprefix $(FILES_PATH)/,$($(SONIC_CTRMGRD)_FILES)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MGMT_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SYSTEM_HEALTH)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_HOST_SERVICES_PY3)) \
        $$(addprefix $(TARGET_PATH)/,$$($$*_RFS_DEPENDS))

	$(HEADER)
	# Pass initramfs and linux kernel explicitly. They are used for all platforms
	export debs_path="$(IMAGE_DISTRO_DEBS_PATH)"
	export files_path="$(FILES_PATH)"
	export python_debs_path="$(PYTHON_DEBS_PATH)"
	export initramfs_tools="$(IMAGE_DISTRO_DEBS_PATH)/$(INITRAMFS_TOOLS)"
	export linux_kernel="$(IMAGE_DISTRO_DEBS_PATH)/$(LINUX_KERNEL)"
	export onie_recovery_image="$(FILES_PATH)/$(ONIE_RECOVERY_IMAGE)"
	export onie_recovery_kvm_4asic_image="$(FILES_PATH)/$(ONIE_RECOVERY_KVM_4ASIC_IMAGE)"
	export onie_recovery_kvm_6asic_image="$(FILES_PATH)/$(ONIE_RECOVERY_KVM_4ASIC_IMAGE)"
	export kversion="$(KVERSION)"
	export image_type="$($*_IMAGE_TYPE)"
	export sonicadmin_user="$(USERNAME)"
	export sonic_asic_platform="$(patsubst %-$(CONFIGURED_ARCH),%,$(CONFIGURED_PLATFORM))"
	export enable_organization_extensions="$(ENABLE_ORGANIZATION_EXTENSIONS)"
	export enable_ztp="$(ENABLE_ZTP)"
	export include_teamd="$(INCLUDE_TEAMD)"
	export include_router_advertiser="$(INCLUDE_ROUTER_ADVERTISER)"
	export sonic_su_dev_signing_key="$(SECURE_UPGRADE_DEV_SIGNING_KEY)"
	export sonic_su_signing_cert="$(SECURE_UPGRADE_SIGNING_CERT)"
	export sonic_su_kernel_cafile="$(SECURE_UPGRADE_KERNEL_CAFILE)"
	export sonic_su_mode="$(SECURE_UPGRADE_MODE)"
	export sonic_su_prod_signing_tool="/sonic/scripts/$(shell basename -- $(SECURE_UPGRADE_PROD_SIGNING_TOOL))"
	export include_system_telemetry="$(INCLUDE_SYSTEM_TELEMETRY)"
	export include_system_otel="$(INCLUDE_SYSTEM_OTEL)"
	export include_system_gnmi="$(INCLUDE_SYSTEM_GNMI)"
	export include_system_bmp="$(INCLUDE_SYSTEM_BMP)"
	export include_system_eventd="$(INCLUDE_SYSTEM_EVENTD)"
	export build_reduce_image_size="$(BUILD_REDUCE_IMAGE_SIZE)"
	export include_restapi="$(INCLUDE_RESTAPI)"
	export include_nat="$(INCLUDE_NAT)"
	export include_p4rt="$(INCLUDE_P4RT)"
	export include_sflow="$(INCLUDE_SFLOW)"
	export enable_auto_tech_support="$(ENABLE_AUTO_TECH_SUPPORT)"
	export enable_asan="$(ENABLE_ASAN)"
	export include_macsec="$(INCLUDE_MACSEC)"
	export include_dhcp_server="$(INCLUDE_DHCP_SERVER)"
	export include_mgmt_framework="$(INCLUDE_MGMT_FRAMEWORK)"
	export include_iccpd="$(INCLUDE_ICCPD)"
	export include_dash_ha="$(INCLUDE_DASH_HA)"
	export include_stp="$(INCLUDE_STP)"
	export pddf_support="$(PDDF_SUPPORT)"
	export include_pde="$(INCLUDE_PDE)"
	export shutdown_bgp_on_start="$(SHUTDOWN_BGP_ON_START)"
	export default_buffer_model="$(SONIC_BUFFER_MODEL)"
	export include_kubernetes="$(INCLUDE_KUBERNETES)"
	export include_kubernetes_master="$(INCLUDE_KUBERNETES_MASTER)"
	export kube_docker_proxy="$(KUBE_DOCKER_PROXY)"
	export enable_pfcwd_on_start="$(ENABLE_PFCWD_ON_START)"
	export installer_debs="$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$($*_INSTALLS) $(FIPS_BASEIMAGE_INSTALLERS))"
	export installer_python_debs="$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(FIPS_BASEIMAGE_PYTHON_INSTALLERS))"
	export lazy_installer_debs="$(foreach deb, $($*_LAZY_INSTALLS),$(foreach device, $($(deb)_PLATFORM),$(addprefix $(device)@, $(IMAGE_DISTRO_DEBS_PATH)/$(deb))))"
	export lazy_build_installer_debs="$(foreach deb, $($*_LAZY_BUILD_INSTALLS), $(addprefix $($(deb)_MACHINE)|,$(deb)))"
	export installer_python_wheels="$(addprefix $(PYTHON_WHEELS_PATH)/,$($*_PYTHON_WHEELS))"
	export installer_images="$(foreach docker, $($*_DOCKERS),\
				$(addprefix $($(docker:-dbg.gz=.gz)_PACKAGE_NAME)|,\
				$(addprefix $($(docker:-dbg.gz=.gz)_PATH)|,\
				$(addprefix $($(docker:-dbg.gz=.gz)_MACHINE)|,\
				$(addprefix $(TARGET_PATH)/,$(addsuffix :$($(docker:-dbg.gz=.gz)_VERSION),$(docker)))))))"
	export sonic_packages="$(foreach package, $(SONIC_PACKAGES),\
				$(addsuffix |$($(package)_DEFAULT_FEATURE_STATE_ENABLED),\
				$(addsuffix |$($(package)_DEFAULT_FEATURE_OWNER),\
				$(addsuffix |$($(package)_VERSION),\
				$(addsuffix |$($(package)_REPOSITORY), $(package))))))"
	export sonic_local_packages="$(foreach package, $(SONIC_PACKAGES_LOCAL),\
				$(addsuffix |$($(package)_DEFAULT_FEATURE_STATE_ENABLED),\
				$(addsuffix |$($(package)_DEFAULT_FEATURE_OWNER),\
				$(addsuffix |$(addprefix $(TARGET_PATH)/, $(package)), $(package)))))"
	export sonic_py_common_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY2))"
	export sonic_py_common_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY3))"
	export config_engine_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY2))"
	export config_engine_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY3))"
	export platform_common_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_COMMON_PY3))"
	export redis_dump_load_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(REDIS_DUMP_LOAD_PY2))"
	export redis_dump_load_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(REDIS_DUMP_LOAD_PY3))"
	export install_debug_image="$(INSTALL_DEBUG_TOOLS)"
	export sonic_yang_models_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MODELS_PY3))"
	export sonic_ctrmgmt_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CTRMGRD))"
	export sonic_yang_mgmt_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MGMT_PY3))"
	export multi_instance="false"
	export python_swss_debs="$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$($(LIBSWSSCOMMON)_RDEPENDS))"
	export python_swss_debs+=" $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(LIBSWSSCOMMON)) $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(PYTHON_SWSSCOMMON)) $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(PYTHON3_SWSSCOMMON))"
	export sonic_utilities_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_UTILITIES_PY3))"
	export sonic_host_services_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_HOST_SERVICES_PY3))"
	export components="$(foreach component,$(notdir $^),\
		$(shell [[ ! -z '$($(component)_VERSION)' && ! -z '$($(component)_NAME)' ]] && \
			echo $($(component)_NAME)==$($(component)_VERSION)))"
	export include_mux="$(INCLUDE_MUX)"
	export include_bootchart="$(INCLUDE_BOOTCHART)"
	export enable_bootchart="$(ENABLE_BOOTCHART)"
	export enable_multidb="$(ENABLE_MULTIDB)"
	$(foreach docker, $($*_DOCKERS),\
		export docker_image="$(docker)"
		export docker_image_name="$(basename $(docker))"
		export docker_container_name="$($(docker:-dbg.gz=.gz)_CONTAINER_NAME)"
		export mount_default_tmpfs="y"
		$(eval $(docker:-dbg.gz=.gz)_RUN_OPT += $($(docker:-dbg.gz=.gz)_$($*_IMAGE_TYPE)_RUN_OPT))
		export docker_image_run_opt="$($(docker:-dbg.gz=.gz)_RUN_OPT)"

		if [ -f files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 ]; then
			j2 files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service

			# Set the flag GLOBAL for all the global system-wide dockers.
			$(if $(shell ls files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_GLOBAL = yes)
			)
		fi
		# Any service template, inside instance directory, will be used to generate .service and @.service file.
		if [ -f files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 ]; then
			export multi_instance="true"
			j2 files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME)@.service
			$(if $(shell ls files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_TEMPLATE = yes)
			)
			export multi_instance="false"
			j2 files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service
		fi
		# Any service template, inside share_image directory, will be used to generate -chassis.service file.
		# TODO: need better way to name the image-shared service
		if [ -f files/build_templates/share_image/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 ]; then
			j2 files/build_templates/share_image/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME)-chassis.service
			$(if $(shell ls files/build_templates/share_image/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_SHARE = yes)
			)
		fi

		j2 files/build_templates/docker_image_ctl.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh
		chmod +x $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh

		$(if $($(docker:-dbg.gz=.gz)_MACHINE),\
			mv $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh $($(docker:-dbg.gz=.gz)_MACHINE)_$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh
		)
	)

	# Exported variables are used by sonic_debian_extension.sh
	export installer_start_scripts="$(foreach docker, $($*_DOCKERS),$(addsuffix .sh, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME)))"
	export feature_vs_image_names="$(foreach docker, $($*_DOCKERS), $(addsuffix :, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME):$(docker:-dbg.gz=.gz)))"

	# Marks template services with an "@" according to systemd convention
	# If the $($docker)_TEMPLATE) variable is set, the service will be treated as a template
	# If the $($docker)_GLOBAL) and $($docker)_TEMPLATE) variables are set the service will be added both as a global and template service.
	$(foreach docker, $($*_DOCKERS),\
		$(if $($(docker:-dbg.gz=.gz)_TEMPLATE),\
			$(if $($(docker:-dbg.gz=.gz)_GLOBAL),\
				$(eval SERVICES += "$(addsuffix .service, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))")\
			)\
			$(eval SERVICES += "$(addsuffix @.service, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))"),\
			$(eval SERVICES += "$(addsuffix .service, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))")
		)
		$(if $($(docker:-dbg.gz=.gz)_SHARE),\
			$(eval SERVICES += "$(addsuffix -chassis.service, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))")
		)
	)
	export installer_services="$(SERVICES)"

	export installer_extra_files="$(foreach docker, $($*_DOCKERS), $(foreach file, $($(docker:-dbg.gz=.gz)_BASE_IMAGE_FILES), $($(docker:-dbg.gz=.gz)_PATH)/base_image_files/$(file)))"

	j2 -f env files/initramfs-tools/union-mount.j2 onie-image.conf > files/initramfs-tools/union-mount
	j2 -f env files/initramfs-tools/arista-convertfs.j2 onie-image.conf > files/initramfs-tools/arista-convertfs

	$(if $($*_DOCKERS),
		j2 files/build_templates/sonic_debian_extension.j2 > sonic_debian_extension.sh
		chmod +x sonic_debian_extension.sh,
	)

	export RFS_SPLIT_FIRST_STAGE=n
	export RFS_SPLIT_LAST_STAGE=y

	# Build images for the MACHINE, DEPENDENT_MACHINE defined.
	$(foreach dep_machine, $($*_MACHINE) $($*_DEPENDENT_MACHINE), \
		DEBUG_IMG="$(INSTALL_DEBUG_TOOLS)" \
		DEBUG_SRC_ARCHIVE_DIRS="$(DBG_SRC_ARCHIVE)" \
		DEBUG_SRC_ARCHIVE_FILE="$(DBG_SRC_ARCHIVE_FILE)" \
			scripts/dbg_files.sh

		RFS_SQUASHFS_NAME=$*__$(dep_machine)__rfs.squashfs \
		DEBUG_IMG="$(INSTALL_DEBUG_TOOLS)" \
		DEBUG_SRC_ARCHIVE_FILE="$(DBG_SRC_ARCHIVE_FILE)" \
		USERNAME="$(USERNAME)" \
		PASSWORD="$(PASSWORD)" \
		CHANGE_DEFAULT_PASSWORD="$(CHANGE_DEFAULT_PASSWORD)" \
		BMC_NOS_ACCOUNT_USERNAME="$(BMC_NOS_ACCOUNT_USERNAME)" \
		BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD="$(BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD)" \
		TARGET_MACHINE=$(dep_machine) \
		IMAGE_TYPE=$($*_IMAGE_TYPE) \
		TARGET_PATH=$(TARGET_PATH) \
		ONIE_IMAGE_PART_SIZE=$(ONIE_IMAGE_PART_SIZE) \
		SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		DBGOPT='$(DBGOPT)' \
		SONIC_VERSION_CACHE=$(SONIC_VERSION_CACHE) \
		MULTIARCH_QEMU_ENVIRON=$(MULTIARCH_QEMU_ENVIRON) \
		CROSS_BUILD_ENVIRON=$(CROSS_BUILD_ENVIRON) \
		BUILD_REDUCE_IMAGE_SIZE=$(BUILD_REDUCE_IMAGE_SIZE) \
		MASTER_KUBERNETES_VERSION=$(MASTER_KUBERNETES_VERSION) \
		MASTER_KUBERNETES_CONTAINER_IMAGE_VERSION=$(MASTER_KUBERNETES_CONTAINER_IMAGE_VERSION) \
		MASTER_PAUSE_VERSION=$(MASTER_PAUSE_VERSION) \
		MASTER_COREDNS_VERSION=$(MASTER_COREDNS_VERSION) \
		MASTER_ETCD_VERSION=$(MASTER_ETCD_VERSION) \
		MASTER_CRI_DOCKERD=$(MASTER_CRI_DOCKERD) \
		MASTER_UI_METRIC_VERSION=$(MASTER_UI_METRIC_VERSION) \
		MASTER_UI_DASH_VERSION=$(MASTER_UI_DASH_VERSION) \
		MASTER_MDM_VERSION=$(MASTER_MDM_VERSION) \
		MASTER_MDS_VERSION=$(MASTER_MDS_VERSION) \
		MASTER_FLUENTD_VERSION=$(MASTER_FLUENTD_VERSION) \
			./build_debian.sh $(LOG)

		USERNAME="$(USERNAME)" \
		PASSWORD="$(PASSWORD)" \
		BMC_NOS_ACCOUNT_USERNAME="$(BMC_NOS_ACCOUNT_USERNAME)" \
		BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD="$(BMC_ROOT_ACCOUNT_DEFAULT_PASSWORD)" \
		TARGET_MACHINE=$(dep_machine) \
		IMAGE_TYPE=$($*_IMAGE_TYPE) \
		ONIE_IMAGE_PART_SIZE=$(ONIE_IMAGE_PART_SIZE) \
		SONIC_ENABLE_IMAGE_SIGNATURE="$(SONIC_ENABLE_IMAGE_SIGNATURE)" \
		SECURE_UPGRADE_MODE="$(SECURE_UPGRADE_MODE)" \
		SECURE_UPGRADE_DEV_SIGNING_KEY="$(SECURE_UPGRADE_DEV_SIGNING_KEY)" \
		SECURE_UPGRADE_SIGNING_CERT="$(SECURE_UPGRADE_SIGNING_CERT)" \
		SECURE_UPGRADE_KERNEL_CAFILE="$(SECURE_UPGRADE_KERNEL_CAFILE)" \
		SECURE_UPGRADE_PROD_SIGNING_TOOL="$(SECURE_UPGRADE_PROD_SIGNING_TOOL)" \
		SECURE_UPGRADE_PROD_TOOL_ARGS="$(SECURE_UPGRADE_PROD_TOOL_ARGS)" \
		SIGNING_KEY="$(SIGNING_KEY)" \
		SIGNING_CERT="$(SIGNING_CERT)" \
		CA_CERT="$(CA_CERT)" \
		TARGET_PATH="$(TARGET_PATH)" \
			./build_image.sh $(LOG)
	)

	$(foreach docker, $($*_DOCKERS), \
		rm -f *$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh
		rm -f $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service
		rm -f $($(docker:-dbg.gz=.gz)_CONTAINER_NAME)@.service
	)

	$(if $($*_DOCKERS),
		rm sonic_debian_extension.sh,
	)

	chmod a+x $@
	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(SONIC_INSTALLERS))

###############################################################################
## Clean targets
###############################################################################

SONIC_CLEAN_DEBS = $(addsuffix -clean,$(addprefix $(DEBS_PATH)/, \
		   $(SONIC_ONLINE_DEBS) \
		   $(SONIC_COPY_DEBS) \
		   $(SONIC_MAKE_DEBS) \
		   $(SONIC_DPKG_DEBS) \
		   $(SONIC_DERIVED_DEBS) \
		   $(SONIC_EXTRA_DEBS)))

SONIC_CLEAN_FILES = $(addsuffix -clean,$(addprefix $(FILES_PATH)/, \
		   $(SONIC_ONLINE_FILES) \
		   $(SONIC_COPY_FILES) \
		   $(SONIC_MAKE_FILES)))

SONIC_CLEAN_PHONIES = $(addsuffix -clean,$(addprefix $(PHONY_PATH)/, \
		   $(SONIC_PHONIES)))

$(SONIC_CLEAN_DEBS) :: $(DEBS_PATH)/%-clean : .platform $$(addsuffix -clean,$$(addprefix $(DEBS_PATH)/,$$($$*_MAIN_DEB)))
	$(Q)# remove derived or extra targets if main one is removed, because we treat them
	$(Q)# as part of one package
	$(Q)rm -f $(addprefix $(DEBS_PATH)/, $* $($*_DERIVED_DEBS) $($*_EXTRA_DEBS))

$(SONIC_CLEAN_FILES) :: $(FILES_PATH)/%-clean : .platform
	$(Q)rm -f $(FILES_PATH)/$*

$(SONIC_CLEAN_PHONIES) :: $(PHONY_PATH)/%-clean : .platform
	$(Q)rm -f $(PHONY_PATH)/$*

SONIC_CLEAN_TARGETS += $(addsuffix -clean,$(addprefix $(TARGET_PATH)/, \
		       $(SONIC_DOCKER_IMAGES) \
		       $(SONIC_DOCKER_DBG_IMAGES) \
		       $(SONIC_SIMPLE_DOCKER_IMAGES) \
		       $(SONIC_INSTALLERS) \
		       $(SONIC_RFS_TARGETS)))

$(SONIC_CLEAN_TARGETS) :: $(TARGET_PATH)/%-clean : .platform
	$(Q)rm -rf $(TARGET_PATH)/$* target/versions/dockers/$(subst .gz,,$*)

SONIC_CLEAN_STDEB_DEBS = $(addsuffix -clean,$(addprefix $(PYTHON_DEBS_PATH)/, \
		     $(SONIC_PYTHON_STDEB_DEBS)))
$(SONIC_CLEAN_STDEB_DEBS) :: $(PYTHON_DEBS_PATH)/%-clean : .platform
	$(Q)rm -f $(PYTHON_DEBS_PATH)/$*

SONIC_CLEAN_WHEELS = $(addsuffix -clean,$(addprefix $(PYTHON_WHEELS_PATH)/, \
		     $(SONIC_PYTHON_WHEELS)))
$(SONIC_CLEAN_WHEELS) :: $(PYTHON_WHEELS_PATH)/%-clean : .platform
	$(Q)rm -f $(PYTHON_WHEELS_PATH)/$*

clean-logs :: .platform
	$(Q)rm -f $(TARGET_PATH)/*.log $(DEBS_PATH)/*.log $(FILES_PATH)/*.log $(PYTHON_DEBS_PATH)/*.log $(PYTHON_WHEELS_PATH)/*.log
clean-versions :: .platform
	@rm -rf target/versions/*

vclean:: .platform
	@sudo rm -rf target/vcache/* target/baseimage*

clean :: .platform clean-logs clean-versions $$(SONIC_CLEAN_DEBS) $$(SONIC_CLEAN_FILES) $$(SONIC_CLEAN_PHONIES) $$(SONIC_CLEAN_TARGETS) $$(SONIC_CLEAN_STDEB_DEBS) $$(SONIC_CLEAN_WHEELS)

###############################################################################
## all
###############################################################################

all : .platform $$(addprefix $(TARGET_PATH)/,$$(SONIC_ALL))

bookworm : $$(addprefix $(TARGET_PATH)/,$$(BOOKWORM_DOCKER_IMAGES)) \
          $$(addprefix $(TARGET_PATH)/,$$(BOOKWORM_DBG_DOCKER_IMAGES))

bullseye : $$(addprefix $(TARGET_PATH)/,$$(BULLSEYE_DOCKER_IMAGES)) \
          $$(addprefix $(TARGET_PATH)/,$$(BULLSEYE_DBG_DOCKER_IMAGES))

buster : $$(addprefix $(TARGET_PATH)/,$$(BUSTER_DOCKER_IMAGES)) \
          $$(addprefix $(TARGET_PATH)/,$$(BUSTER_DBG_DOCKER_IMAGES))

stretch : $$(addprefix $(TARGET_PATH)/,$$(STRETCH_DOCKER_IMAGES)) \
          $$(addprefix $(TARGET_PATH)/,$$(STRETCH_DBG_DOCKER_IMAGES))

jessie : $$(addprefix $(TARGET_PATH)/,$$(JESSIE_DOCKER_IMAGES)) \
         $$(addprefix $(TARGET_PATH)/,$$(JESSIE_DBG_DOCKER_IMAGES))

###############################################################################
## Standard targets
###############################################################################

.PHONY : $(SONIC_CLEAN_DEBS) $(SONIC_CLEAN_FILES) $(SONIC_CLEAN_PHONIES) $(SONIC_CLEAN_TARGETS) $(SONIC_CLEAN_STDEB_DEBS) $(SONIC_CLEAN_WHEELS) $(SONIC_PHONY_TARGETS) clean distclean configure

.INTERMEDIATE : $(SONIC_INSTALL_DEBS) $(SONIC_INSTALL_WHEELS) $(DOCKER_LOAD_TARGETS) docker-start .platform

## To build some commonly used libs. Some submodules depend on these libs.
## It is used in component pipelines. For example: swss needs libnl, libyang
lib-packages: $(addprefix $(DEBS_PATH)/,$(LIBNL3) $(LIBYANG) $(LIBYANG3) $(PROTOBUF) $(LIB_SONIC_DASH_API))
