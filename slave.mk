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
	override SONIC_IMAGE_VERSION = $(shell export BUILD_TIMESTAMP=$(BUILD_TIMESTAMP) && export BUILD_NUMBER=$(BUILD_NUMBER) && . functions.sh && sonic_get_version)
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
BLDENV = $(shell lsb_release -cs)
DEBS_PATH = $(TARGET_PATH)/debs/$(BLDENV)
FILES_PATH = $(TARGET_PATH)/files/$(BLDENV)
PYTHON_DEBS_PATH = $(TARGET_PATH)/python-debs
PYTHON_WHEELS_PATH = $(TARGET_PATH)/python-wheels
PROJECT_ROOT = $(shell pwd)
JESSIE_DEBS_PATH = $(TARGET_PATH)/debs/jessie
JESSIE_FILES_PATH = $(TARGET_PATH)/files/jessie
STRETCH_DEBS_PATH = $(TARGET_PATH)/debs/stretch
STRETCH_FILES_PATH = $(TARGET_PATH)/files/stretch
BUSTER_DEBS_PATH = $(TARGET_PATH)/debs/buster
BUSTER_FILES_PATH = $(TARGET_PATH)/files/buster
DBG_IMAGE_MARK = dbg
DBG_SRC_ARCHIVE_FILE = $(TARGET_PATH)/sonic_src.tar.gz
DPKG_ADMINDIR_PATH = /sonic/dpkg

CONFIGURED_PLATFORM := $(shell [ -f .platform ] && cat .platform || echo generic)
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
ifeq ($(BLDENV),bullseye)
IMAGE_DISTRO := bullseye
else
IMAGE_DISTRO := buster
endif
IMAGE_DISTRO_DEBS_PATH = $(TARGET_PATH)/debs/$(IMAGE_DISTRO)
IMAGE_DISTRO_FILES_PATH = $(TARGET_PATH)/files/$(IMAGE_DISTRO)

export BUILD_NUMBER
export BUILD_TIMESTAMP
export SONIC_IMAGE_VERSION
export CONFIGURED_PLATFORM
export CONFIGURED_ARCH
export PYTHON_WHEELS_PATH
export IMAGE_DISTRO
export IMAGE_DISTRO_DEBS_PATH
export MULTIARCH_QEMU_ENVIRON
export DOCKER_BASE_ARCH
export MIRROR_SNAPSHOT

###############################################################################
## Utility rules
## Define configuration, help etc.
###############################################################################

.platform :
ifneq ($(CONFIGURED_PLATFORM),generic)
	@echo Build system is not configured, please run make configure
	@exit 1
endif

configure :
	@mkdir -p $(DEBS_PATH)
	@mkdir -p $(JESSIE_DEBS_PATH)
	@mkdir -p $(STRETCH_DEBS_PATH)
	@mkdir -p $(BUSTER_DEBS_PATH)
	@mkdir -p $(FILES_PATH)
	@mkdir -p $(JESSIE_FILES_PATH)
	@mkdir -p $(STRETCH_FILES_PATH)
	@mkdir -p $(BUSTER_FILES_PATH)
	@mkdir -p $(PYTHON_DEBS_PATH)
	@mkdir -p $(PYTHON_WHEELS_PATH)
	@mkdir -p $(DPKG_ADMINDIR_PATH)
	@echo $(PLATFORM) > .platform
	@echo $(PLATFORM_ARCH) > .arch

distclean : .platform clean
	@rm -f .platform
	@rm -f .arch

list :
	@$(foreach target,$(SONIC_TARGET_LIST),echo $(target);)

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


ifeq ($(SONIC_INCLUDE_MUX),y)
INCLUDE_MUX = y
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

###############################################################################
## Routing stack related exports
###############################################################################

export SONIC_ROUTING_STACK
export FRR_USER_UID
export FRR_USER_GID

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
$(info "SONIC_USE_DOCKER_BUILDKIT"       : "$(SONIC_USE_DOCKER_BUILDKIT)")
$(info "USERNAME"                        : "$(USERNAME)")
$(info "PASSWORD"                        : "$(PASSWORD)")
$(info "ENABLE_DHCP_GRAPH_SERVICE"       : "$(ENABLE_DHCP_GRAPH_SERVICE)")
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
$(info "ENABLE_ZTP"                      : "$(ENABLE_ZTP)")
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
$(info "INCLUDE_SYSTEM_TELEMETRY"        : "$(INCLUDE_SYSTEM_TELEMETRY)")
$(info "INCLUDE_HOST_SERVICE"            : "$(INCLUDE_HOST_SERVICE)")
$(info "INCLUDE_RESTAPI"                 : "$(INCLUDE_RESTAPI)")
$(info "INCLUDE_SFLOW"                   : "$(INCLUDE_SFLOW)")
$(info "INCLUDE_NAT"                     : "$(INCLUDE_NAT)")
$(info "INCLUDE_KUBERNETES"              : "$(INCLUDE_KUBERNETES)")
$(info "INCLUDE_MUX"                     : "$(INCLUDE_MUX)")
$(info "TELEMETRY_WRITABLE"              : "$(TELEMETRY_WRITABLE)")
$(info "PDDF_SUPPORT"                    : "$(PDDF_SUPPORT)")
$(info "MULTIARCH_QEMU_ENVIRON"          : "$(MULTIARCH_QEMU_ENVIRON)")
$(info "SONIC_VERSION_CONTROL_COMPONENTS": "$(SONIC_VERSION_CONTROL_COMPONENTS)")
$(info "DEFAULT_CONTAINER_REGISTRY"      : "$(SONIC_DEFAULT_CONTAINER_REGISTRY)")
$(info )
else
$(info SONiC Build System for $(CONFIGURED_PLATFORM):$(CONFIGURED_ARCH))
endif

# Overwrite the buildinfo in slave container
$(shell sudo scripts/prepare_slave_container_buildinfo.sh $(SLAVE_DIR) $(CONFIGURED_ARCH) $(BLDENV))

include Makefile.cache

ifeq ($(SONIC_USE_DOCKER_BUILDKIT),y)
$(warning "Using SONIC_USE_DOCKER_BUILDKIT will produce larger installable SONiC image because of a docker bug (more details: https://github.com/moby/moby/issues/38903)")
export DOCKER_BUILDKIT=1
endif


###############################################################################
## Generic rules section
## All rules must go after includes for propper targets expansion
###############################################################################

export kernel_procure_method=$(KERNEL_PROCURE_METHOD)
export vs_build_prepare_mem=$(VS_PREPARE_MEM)

###############################################################################
## Local targets
###############################################################################

# Copy debian packages from local directory
# Add new package for copy:
#     SOME_NEW_DEB = some_new_deb.deb
#     $(SOME_NEW_DEB)_PATH = path/to/some_new_deb.deb
#     SONIC_COPY_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_COPY_DEBS)) : $(DEBS_PATH)/% : .platform
	$(HEADER)
	$(foreach deb,$* $($*_DERIVED_DEBS), \
	    { cp $($(deb)_PATH)/$(deb) $(DEBS_PATH)/ $(LOG) || exit 1 ; } ; )
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
			{ curl -L -f -o $(DEBS_PATH)/$(deb) $($(deb)_CURL_OPTIONS) $($(deb)_URL) $(LOG) || { exit 1 ; } } ; )

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
	curl -L -f -o $@ $($*_CURL_OPTIONS) $($*_URL) $(LOG)
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
#     SONIC_MAKE_FILES += $(SOME_NEW_FILE)
$(addprefix $(FILES_PATH)/, $(SONIC_MAKE_FILES)) : $(FILES_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
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
#     SONIC_MAKE_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_MAKE_DEBS)) : $(DEBS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
			$$(addprefix $(DEBS_PATH)/,$$($$*_AFTER)) \
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
		DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC}" make DEST=$(shell pwd)/$(DEBS_PATH) -C $($*_SRC_PATH) $(shell pwd)/$(DEBS_PATH)/$* $(LOG)
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
#     SONIC_DPKG_DEBS += $(SOME_NEW_DEB)
$(addprefix $(DEBS_PATH)/, $(SONIC_DPKG_DEBS)) : $(DEBS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) \
			$$(addprefix $(DEBS_PATH)/,$$($$*_AFTER)) \
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
			${$*_BUILD_ENV} DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC} ${$*_DEB_BUILD_OPTIONS}" dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --as-root -T$($*_DPKG_TARGET) --admindir $$mergedir $(LOG),
			${$*_BUILD_ENV} DEB_BUILD_OPTIONS="${DEB_BUILD_OPTIONS_GENERIC} ${$*_DEB_BUILD_OPTIONS}" dpkg-buildpackage -rfakeroot -b -us -uc -j$(SONIC_CONFIG_MAKE_JOBS) --admindir $$mergedir $(LOG)
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
$(addprefix $(DEBS_PATH)/, $(SONIC_DERIVED_DEBS)) : $(DEBS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS)))
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
$(SONIC_INSTALL_DEBS) : $(DEBS_PATH)/%-install : .platform $$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEPENDS))) $(DEBS_PATH)/$$*
	$(HEADER)
	[ -f $(DEBS_PATH)/$* ] || { echo $(DEBS_PATH)/$* does not exist $(LOG) && false $(LOG) }
	while true; do
		# wait for conflicted packages to be uninstalled
		$(foreach deb, $($*_CONFLICT_DEBS), \
			{ while dpkg -s $(firstword $(subst _, ,$(basename $(deb)))) | grep "^Version: $(word 2, $(subst _, ,$(basename $(deb))))" &> /dev/null; do echo "waiting for $(deb) to be uninstalled" $(LOG); sleep 1; done } )
		# put a lock here because dpkg does not allow installing packages in parallel
		if mkdir $(DEBS_PATH)/dpkg_lock &> /dev/null; then
			{ sudo DEBIAN_FRONTEND=noninteractive dpkg -i $(DEBS_PATH)/$* $(LOG) && rm -d $(DEBS_PATH)/dpkg_lock && break; } || { set +e; rm -d $(DEBS_PATH)/dpkg_lock; sudo lsof /var/lib/dpkg/lock-frontend; ps aux; exit 1 ; }
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
#     SONIC_PYTHON_STDEB_DEBS += $(SOME_NEW_DEB)
$(addprefix $(PYTHON_DEBS_PATH)/, $(SONIC_PYTHON_STDEB_DEBS)) : $(PYTHON_DEBS_PATH)/% : .platform \
		$$(addsuffix -install,$$(addprefix $(DEBS_PATH)/,$$($$*_DEBS_DEPENDS))) \
		$$(addsuffix -install,$$(addprefix $(PYTHON_DEBS_PATH)/,$$($$*_DEPENDS))) \
		$$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_WHEEL_DEPENDS))) \
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
#     SONIC_PYTHON_WHEELS += $(SOME_NEW_WHL)
$(addprefix $(PYTHON_WHEELS_PATH)/, $(SONIC_PYTHON_WHEELS)) : $(PYTHON_WHEELS_PATH)/% : .platform $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_DEPENDS))) \
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
		# Use pip instead of later setup.py to install dependencies into user home, but uninstall self
		pip$($*_PYTHON_VERSION) install . && pip$($*_PYTHON_VERSION) uninstall --yes `python setup.py --name`
		if [ ! "$($*_TEST)" = "n" ]; then python$($*_PYTHON_VERSION) setup.py test $(LOG); fi
		python$($*_PYTHON_VERSION) setup.py bdist_wheel $(LOG)
		# clean up
		if [ -f ../$(notdir $($*_SRC_PATH)).patch/series ]; then quilt pop -a -f; [ -d .pc ] && rm -rf .pc; fi
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
$(SONIC_INSTALL_WHEELS) : $(PYTHON_WHEELS_PATH)/%-install : .platform $$(addsuffix -install,$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*_DEPENDS))) $(PYTHON_WHEELS_PATH)/$$*
	$(HEADER)
	[ -f $(PYTHON_WHEELS_PATH)/$* ] || { echo $(PYTHON_WHEELS_PATH)/$* does not exist $(LOG) && exit 1; }
	# put a lock here to avoid race conditions
	while true; do
	if mkdir $(PYTHON_WHEELS_PATH)/pip_lock &> /dev/null; then
	{ sudo -E pip$($*_PYTHON_VERSION) install $(PYTHON_WHEELS_PATH)/$* $(LOG) && rm -d $(PYTHON_WHEELS_PATH)/pip_lock && break; } || { rm -d $(PYTHON_WHEELS_PATH)/pip_lock && exit 1 ; }
	fi
	done
	$(FOOTER)

###############################################################################
## Docker images related targets
###############################################################################

# start docker daemon
docker-start :
	@sudo sed -i '/http_proxy/d' /etc/default/docker
	@sudo bash -c "echo \"export http_proxy=$$http_proxy\" >> /etc/default/docker"
	@test x$(SONIC_CONFIG_USE_NATIVE_DOCKERD_FOR_BUILD) != x"y" && sudo service docker status &> /dev/null || ( sudo service docker start &> /dev/null && ./scripts/wait_for_docker.sh 60 )

# targets for building simple docker images that do not depend on any debian packages
$(addprefix $(TARGET_PATH)/, $(SONIC_SIMPLE_DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform docker-start $$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$($$*.gz_LOAD_DOCKERS)))
	$(HEADER)
	# Apply series of patches if exist
	if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*.gz_PATH)).patch quilt push -a; popd; fi $(LOG)
	# Prepare docker build info
	scripts/prepare_docker_buildinfo.sh $* $($*.gz_PATH)/Dockerfile $(CONFIGURED_ARCH) $(TARGET_DOCKERFILE)/Dockerfile.buildinfo
	docker info $(LOG)
	docker build --squash --no-cache \
		--build-arg http_proxy=$(HTTP_PROXY) \
		--build-arg https_proxy=$(HTTPS_PROXY) \
		--build-arg user=$(USER) \
		--build-arg uid=$(UID) \
		--build-arg guid=$(GUID) \
		--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
		--label Tag=$(SONIC_IMAGE_VERSION) \
		-f $(TARGET_DOCKERFILE)/Dockerfile.buildinfo \
		-t $* $($*.gz_PATH) $(LOG)
	scripts/collect_docker_version_files.sh $* $(TARGET_PATH)
	docker save $* | gzip -c > $@
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
	DOCKER_IMAGES = $(filter-out $(SONIC_JESSIE_DOCKERS) $(SONIC_STRETCH_DOCKERS),$(SONIC_DOCKER_IMAGES))
	DOCKER_DBG_IMAGES = $(filter-out $(SONIC_JESSIE_DBG_DOCKERS) $(SONIC_STRETCH_DBG_DOCKERS), $(SONIC_DOCKER_DBG_IMAGES))
endif
endif

$(foreach IMAGE,$(DOCKER_IMAGES), $(eval $(IMAGE)_DEBS_PATH := $(DEBS_PATH)))
$(foreach IMAGE,$(DOCKER_IMAGES), $(eval $(IMAGE)_FILES_PATH := $(FILES_PATH)))
$(foreach IMAGE,$(DOCKER_DBG_IMAGES), $(eval $(IMAGE)_DEBS_PATH := $(DEBS_PATH)))
$(foreach IMAGE,$(DOCKER_DBG_IMAGES), $(eval $(IMAGE)_FILES_PATH := $(FILES_PATH)))

# Targets for building docker images
$(addprefix $(TARGET_PATH)/, $(DOCKER_IMAGES)) : $(TARGET_PATH)/%.gz : .platform docker-start \
		$$(addprefix $$($$*.gz_DEBS_PATH)/,$$($$*.gz_DEPENDS)) \
		$$(addprefix $$($$*.gz_FILES_PATH)/,$$($$*.gz_FILES)) \
		$$(addprefix $(PYTHON_DEBS_PATH)/,$$($$*.gz_PYTHON_DEBS)) \
		$$(addprefix $(PYTHON_WHEELS_PATH)/,$$($$*.gz_PYTHON_WHEELS)) \
		$$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$($$*.gz_LOAD_DOCKERS))) \
		$$($$*.gz_PATH)/Dockerfile.j2 \
		$(call dpkg_depend,$(TARGET_PATH)/%.gz.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*.gz,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*.gz_CACHE_LOADED)' ] ; then

		# Apply series of patches if exist
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && ( quilt pop -a -f 1>/dev/null 2>&1 || true ) && QUILT_PATCHES=../$(notdir $($*.gz_PATH)).patch quilt push -a; popd; fi $(LOG)
		mkdir -p $($*.gz_PATH)/debs $(LOG)
		mkdir -p $($*.gz_PATH)/files $(LOG)
		mkdir -p $($*.gz_PATH)/python-debs $(LOG)
		mkdir -p $($*.gz_PATH)/python-wheels $(LOG)
		sudo mount --bind $($*.gz_DEBS_PATH) $($*.gz_PATH)/debs $(LOG)
		sudo mount --bind $($*.gz_FILES_PATH) $($*.gz_PATH)/files $(LOG)
		sudo mount --bind $(PYTHON_DEBS_PATH) $($*.gz_PATH)/python-debs $(LOG)
		sudo mount --bind $(PYTHON_WHEELS_PATH) $($*.gz_PATH)/python-wheels $(LOG)
		# Export variables for j2. Use path for unique variable names, e.g. docker_orchagent_debs
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_debs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DEPENDS),RDEPENDS))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_pydebs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_PYTHON_DEBS)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_whls=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_PYTHON_WHEELS)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_PACKAGES)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_pkgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_APT_PACKAGES)))\n" | awk '!a[$$0]++'))
		j2 $($*.gz_PATH)/Dockerfile.j2 > $($*.gz_PATH)/Dockerfile
		# Prepare docker build info
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		scripts/prepare_docker_buildinfo.sh $* $($*.gz_PATH)/Dockerfile $(CONFIGURED_ARCH)
		docker info $(LOG)
		docker build --squash --no-cache \
			--build-arg http_proxy=$(HTTP_PROXY) \
			--build-arg https_proxy=$(HTTPS_PROXY) \
			--build-arg user=$(USER) \
			--build-arg uid=$(UID) \
			--build-arg guid=$(GUID) \
			--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
			--build-arg frr_user_uid=$(FRR_USER_UID) \
			--build-arg frr_user_gid=$(FRR_USER_GID) \
			--build-arg image_version=$(SONIC_IMAGE_VERSION) \
			--label Tag=$(SONIC_IMAGE_VERSION) \
			-t $* $($*.gz_PATH) $(LOG)
		scripts/collect_docker_version_files.sh $* $(TARGET_PATH)
		docker save $* | gzip -c > $@
		# Clean up
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*.gz,$@)
	fi

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(DOCKER_IMAGES))

# Targets for building docker images
$(addprefix $(TARGET_PATH)/, $(DOCKER_DBG_IMAGES)) : $(TARGET_PATH)/%-$(DBG_IMAGE_MARK).gz : .platform docker-start \
		$$(addprefix $$($$*.gz_DEBS_PATH)/,$$($$*.gz_DBG_DEPENDS)) \
		$$(addsuffix -load,$$(addprefix $(TARGET_PATH)/,$$*.gz)) \
		$(call dpkg_depend,$(TARGET_PATH)/%-$(DBG_IMAGE_MARK).gz.dep)
	$(HEADER)

	# Load the target deb from DPKG cache
	$(call LOAD_CACHE,$*-$(DBG_IMAGE_MARK).gz,$@)

	# Skip building the target if it is already loaded from cache
	if [ -z '$($*-$(DBG_IMAGE_MARK).gz_CACHE_LOADED)' ] ; then

		mkdir -p $($*.gz_PATH)/debs $(LOG)
		sudo mount --bind $($*.gz_DEBS_PATH) $($*.gz_PATH)/debs $(LOG)
		# Export variables for j2. Use path for unique variable names, e.g. docker_orchagent_debs
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_debs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_DEPENDS),RDEPENDS))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_image_dbgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_IMAGE_PACKAGES)))\n" | awk '!a[$$0]++'))
		$(eval export $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_pkgs=$(shell printf "$(subst $(SPACE),\n,$(call expand,$($*.gz_DBG_APT_PACKAGES),RDEPENDS))\n" | awk '!a[$$0]++'))
		./build_debug_docker_j2.sh $* $(subst -,_,$(notdir $($*.gz_PATH)))_dbg_debs $(subst -,_,$(notdir $($*.gz_PATH)))_image_dbgs > $($*.gz_PATH)/Dockerfile-dbg.j2
		j2 $($*.gz_PATH)/Dockerfile-dbg.j2 > $($*.gz_PATH)/Dockerfile-dbg
		# Prepare docker build info
		PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
		SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
		TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
		scripts/prepare_docker_buildinfo.sh $* $($*.gz_PATH)/Dockerfile-dbg $(CONFIGURED_ARCH)
		docker info $(LOG)
		docker build \
			$(if $($*.gz_DBG_DEPENDS), --squash --no-cache, --no-cache) \
			--build-arg http_proxy=$(HTTP_PROXY) \
			--build-arg https_proxy=$(HTTPS_PROXY) \
			--build-arg docker_container_name=$($*.gz_CONTAINER_NAME) \
			--label Tag=$(SONIC_IMAGE_VERSION) \
			--file $($*.gz_PATH)/Dockerfile-dbg \
			-t $*-dbg $($*.gz_PATH) $(LOG)
		scripts/collect_docker_version_files.sh $*-dbg $(TARGET_PATH)
		docker save $*-dbg | gzip -c > $@
		# Clean up
		if [ -f $($*.gz_PATH).patch/series ]; then pushd $($*.gz_PATH) && quilt pop -a -f; [ -d .pc ] && rm -rf .pc; popd; fi

		# Save the target deb into DPKG cache
		$(call SAVE_CACHE,$*-$(DBG_IMAGE_MARK).gz,$@)
	fi

	$(FOOTER)

SONIC_TARGET_LIST += $(addprefix $(TARGET_PATH)/, $(DOCKER_DBG_IMAGES))

DOCKER_LOAD_TARGETS = $(addsuffix -load,$(addprefix $(TARGET_PATH)/, \
		      $(SONIC_SIMPLE_DOCKER_IMAGES) \
		      $(DOCKER_IMAGES)))

$(DOCKER_LOAD_TARGETS) : $(TARGET_PATH)/%.gz-load : .platform docker-start $$(TARGET_PATH)/$$*.gz
	$(HEADER)
	docker load -i $(TARGET_PATH)/$*.gz $(LOG)
	$(FOOTER)

###############################################################################
## Installers
###############################################################################

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
        $(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(INITRAMFS_TOOLS) \
                $(LINUX_KERNEL) \
                $(SONIC_DEVICE_DATA) \
                $(IFUPDOWN2) \
                $(KDUMP_TOOLS) \
                $(NTP) \
                $(LIBPAM_TACPLUS) \
                $(LIBNSS_TACPLUS) \
                $(MONIT) \
                $(OPENSSH_SERVER) \
                $(PYTHON_SWSSCOMMON) \
                $(PYTHON3_SWSSCOMMON) \
                $(SONIC_UTILITIES_DATA) \
                $(SONIC_HOST_SERVICES_DATA) \
                $(AUDISP_TACPLUS)) \
        $$(addprefix $(TARGET_PATH)/,$$($$*_DOCKERS)) \
        $$(addprefix $(FILES_PATH)/,$$($$*_FILES)) \
        $(if $(findstring y,$(ENABLE_ZTP)),$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(SONIC_ZTP))) \
        $(if $(findstring y,$(INCLUDE_HOST_SERVICE)),$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$(SONIC_HOST_SERVICE))) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_UTILITIES_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_COMMON_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_COMMON_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(REDIS_DUMP_LOAD_PY2)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_API_PY2)) \
        $(if $(findstring y,$(PDDF_SUPPORT)),$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY2))) \
        $(if $(findstring y,$(PDDF_SUPPORT)),$(addprefix $(PYTHON_WHEELS_PATH)/,$(PDDF_PLATFORM_API_BASE_PY3))) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MODELS_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CTRMGRD)) \
        $(addprefix $(FILES_PATH)/,$($(SONIC_CTRMGRD)_FILES)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_YANG_MGMT_PY3)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SYSTEM_HEALTH)) \
        $(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_HOST_SERVICES_PY3))
	$(HEADER)
	# Pass initramfs and linux kernel explicitly. They are used for all platforms
	export debs_path="$(IMAGE_DISTRO_DEBS_PATH)"
	export files_path="$(FILES_PATH)"
	export python_debs_path="$(PYTHON_DEBS_PATH)"
	export initramfs_tools="$(IMAGE_DISTRO_DEBS_PATH)/$(INITRAMFS_TOOLS)"
	export linux_kernel="$(IMAGE_DISTRO_DEBS_PATH)/$(LINUX_KERNEL)"
	export onie_recovery_image="$(FILES_PATH)/$(ONIE_RECOVERY_IMAGE)"
	export kversion="$(KVERSION)"
	export image_type="$($*_IMAGE_TYPE)"
	export sonicadmin_user="$(USERNAME)"
	export sonic_asic_platform="$(patsubst %-$(CONFIGURED_ARCH),%,$(CONFIGURED_PLATFORM))"
	export enable_organization_extensions="$(ENABLE_ORGANIZATION_EXTENSIONS)"
	export enable_dhcp_graph_service="$(ENABLE_DHCP_GRAPH_SERVICE)"
	export enable_ztp="$(ENABLE_ZTP)"
	export include_system_telemetry="$(INCLUDE_SYSTEM_TELEMETRY)"
	export include_restapi="$(INCLUDE_RESTAPI)"
	export include_host_service="$(INCLUDE_HOST_SERVICE)"
	export include_nat="$(INCLUDE_NAT)"
	export include_sflow="$(INCLUDE_SFLOW)"
	export include_mgmt_framework="$(INCLUDE_MGMT_FRAMEWORK)"
	export include_iccpd="$(INCLUDE_ICCPD)"
	export pddf_support="$(PDDF_SUPPORT)"
	export shutdown_bgp_on_start="$(SHUTDOWN_BGP_ON_START)"
	export default_buffer_model="$(SONIC_BUFFER_MODEL)"
	export include_kubernetes="$(INCLUDE_KUBERNETES)"
	export kube_docker_proxy="$(KUBE_DOCKER_PROXY)"
	export enable_pfcwd_on_start="$(ENABLE_PFCWD_ON_START)"
	export installer_debs="$(addprefix $(IMAGE_DISTRO_DEBS_PATH)/,$($*_INSTALLS))"
	export lazy_installer_debs="$(foreach deb, $($*_LAZY_INSTALLS),$(foreach device, $($(deb)_PLATFORM),$(addprefix $(device)@, $(IMAGE_DISTRO_DEBS_PATH)/$(deb))))"
	export installer_images="$(addprefix $(TARGET_PATH)/,$($*_DOCKERS))"
	export sonic_py_common_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY2))"
	export sonic_py_common_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PY_COMMON_PY3))"
	export config_engine_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY2))"
	export config_engine_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_CONFIG_ENGINE_PY3))"
	export swsssdk_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SWSSSDK_PY2))"
	export swsssdk_py3_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SWSSSDK_PY3))"
	export platform_common_py2_wheel_path="$(addprefix $(PYTHON_WHEELS_PATH)/,$(SONIC_PLATFORM_COMMON_PY2))"
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
	export include_mux="$(INCLUDE_MUX)"
	$(foreach docker, $($*_DOCKERS),\
		export docker_image="$(docker)"
		export docker_image_name="$(basename $(docker))"
		export docker_container_name="$($(docker:-dbg.gz=.gz)_CONTAINER_NAME)"
		$(eval $(docker:-dbg.gz=.gz)_RUN_OPT += $($(docker:-dbg.gz=.gz)_$($*_IMAGE_TYPE)_RUN_OPT))
		export docker_image_run_opt="$($(docker:-dbg.gz=.gz)_RUN_OPT)"

		if [ -f files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 ]; then
			j2 files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service

			# Set the flag GLOBAL for all the global system-wide dockers.
			$(if $(shell ls files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).service.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_GLOBAL = yes)
			)
		fi
		if [ -f files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 ]; then
			j2 files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer

			# Set the flag GLOBAL_TIMER for all the global system-wide dockers timers.
			$(if $(shell ls files/build_templates/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_GLOBAL_TIMER = yes)
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
		# Any timer template, inside instance directory, will be used to generate .timer and @.timer file.
		if [ -f files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 ]; then
			export multi_instance="true"
			j2 files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME)@.timer
			$(if $(shell ls files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 2>/dev/null),\
				$(eval $(docker:-dbg.gz=.gz)_TEMPLATE_TIMER = yes)
			)
			export multi_instance="false"
			j2 files/build_templates/per_namespace/$($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer.j2 > $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).timer
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
	# Marks template timers with an "@" according to systemd convention
	# If the $($docker)_TEMPLATE_TIMER) variable is set, the timer will be treated as a template
	# If the $($docker)_GLOBAL_TIMER) and $($docker)_TEMPLATE_TIMER) variables are set the timer will be added both as a global and template timer.
	$(foreach docker, $($*_DOCKERS),\
		$(if $($(docker:-dbg.gz=.gz)_TEMPLATE_TIMER),\
			$(if $($(docker:-dbg.gz=.gz)_GLOBAL_TIMER),\
				$(eval TIMERS += "$(addsuffix .timer, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))")\
			)\
			$(eval TIMERS += "$(addsuffix @.timer, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))"),\
			$(eval TIMERS += "$(addsuffix .timer, $($(docker:-dbg.gz=.gz)_CONTAINER_NAME))")
		)
	)
	export installer_services="$(SERVICES)"
	export installer_timers="$(TIMERS)"

	export installer_extra_files="$(foreach docker, $($*_DOCKERS), $(foreach file, $($(docker:-dbg.gz=.gz)_BASE_IMAGE_FILES), $($(docker:-dbg.gz=.gz)_PATH)/base_image_files/$(file)))"

	j2 -f env files/initramfs-tools/union-mount.j2 onie-image.conf > files/initramfs-tools/union-mount
	j2 -f env files/initramfs-tools/arista-convertfs.j2 onie-image.conf > files/initramfs-tools/arista-convertfs

	$(if $($*_DOCKERS),
		j2 files/build_templates/sonic_debian_extension.j2 > sonic_debian_extension.sh
		chmod +x sonic_debian_extension.sh,
	)

	DEBUG_IMG="$(INSTALL_DEBUG_TOOLS)" \
	DEBUG_SRC_ARCHIVE_DIRS="$(DBG_SRC_ARCHIVE)" \
	DEBUG_SRC_ARCHIVE_FILE="$(DBG_SRC_ARCHIVE_FILE)" \
		scripts/dbg_files.sh

	DEBUG_IMG="$(INSTALL_DEBUG_TOOLS)" \
	DEBUG_SRC_ARCHIVE_FILE="$(DBG_SRC_ARCHIVE_FILE)" \
	USERNAME="$(USERNAME)" \
	PASSWORD="$(PASSWORD)" \
	IMAGE_TYPE=$($*_IMAGE_TYPE) \
	TARGET_PATH=$(TARGET_PATH) \
	SONIC_ENFORCE_VERSIONS=$(SONIC_ENFORCE_VERSIONS) \
	TRUSTED_GPG_URLS=$(TRUSTED_GPG_URLS) \
	PACKAGE_URL_PREFIX=$(PACKAGE_URL_PREFIX) \
	MULTIARCH_QEMU_ENVIRON=$(MULTIARCH_QEMU_ENVIRON) \
		./build_debian.sh $(LOG)

	USERNAME="$(USERNAME)" \
	PASSWORD="$(PASSWORD)" \
	TARGET_MACHINE=$($*_MACHINE) \
	IMAGE_TYPE=$($*_IMAGE_TYPE) \
	SONIC_ENABLE_IMAGE_SIGNATURE="$(SONIC_ENABLE_IMAGE_SIGNATURE)" \
	SIGNING_KEY="$(SIGNING_KEY)" \
	SIGNING_CERT="$(SIGNING_CERT)" \
	CA_CERT="$(CA_CERT)" \
	TARGET_PATH="$(TARGET_PATH)" \
		./build_image.sh $(LOG)

	$(foreach docker, $($*_DOCKERS), \
		rm -f $($(docker:-dbg.gz=.gz)_CONTAINER_NAME).sh
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

$(SONIC_CLEAN_DEBS) :: $(DEBS_PATH)/%-clean : .platform $$(addsuffix -clean,$$(addprefix $(DEBS_PATH)/,$$($$*_MAIN_DEB)))
	@# remove derived or extra targets if main one is removed, because we treat them
	@# as part of one package
	@rm -f $(addprefix $(DEBS_PATH)/, $* $($*_DERIVED_DEBS) $($*_EXTRA_DEBS))

$(SONIC_CLEAN_FILES) :: $(FILES_PATH)/%-clean : .platform
	@rm -f $(FILES_PATH)/$*

SONIC_CLEAN_TARGETS += $(addsuffix -clean,$(addprefix $(TARGET_PATH)/, \
		       $(SONIC_DOCKER_IMAGES) \
		       $(SONIC_DOCKER_DBG_IMAGES) \
		       $(SONIC_SIMPLE_DOCKER_IMAGES) \
		       $(SONIC_INSTALLERS)))
$(SONIC_CLEAN_TARGETS) :: $(TARGET_PATH)/%-clean : .platform
	@rm -f $(TARGET_PATH)/$*

SONIC_CLEAN_STDEB_DEBS = $(addsuffix -clean,$(addprefix $(PYTHON_DEBS_PATH)/, \
		     $(SONIC_PYTHON_STDEB_DEBS)))
$(SONIC_CLEAN_STDEB_DEBS) :: $(PYTHON_DEBS_PATH)/%-clean : .platform
	@rm -f $(PYTHON_DEBS_PATH)/$*

SONIC_CLEAN_WHEELS = $(addsuffix -clean,$(addprefix $(PYTHON_WHEELS_PATH)/, \
		     $(SONIC_PYTHON_WHEELS)))
$(SONIC_CLEAN_WHEELS) :: $(PYTHON_WHEELS_PATH)/%-clean : .platform
	@rm -f $(PYTHON_WHEELS_PATH)/$*

clean-logs :: .platform
	@rm -f $(TARGET_PATH)/*.log $(DEBS_PATH)/*.log $(FILES_PATH)/*.log $(PYTHON_DEBS_PATH)/*.log $(PYTHON_WHEELS_PATH)/*.log

clean :: .platform clean-logs $$(SONIC_CLEAN_DEBS) $$(SONIC_CLEAN_FILES) $$(SONIC_CLEAN_TARGETS) $$(SONIC_CLEAN_STDEB_DEBS) $$(SONIC_CLEAN_WHEELS)

###############################################################################
## all
###############################################################################

all : .platform $$(addprefix $(TARGET_PATH)/,$$(SONIC_ALL))

stretch : $$(addprefix $(TARGET_PATH)/,$$(STRETCH_DOCKER_IMAGES)) \
          $$(addprefix $(TARGET_PATH)/,$$(STRETCH_DBG_DOCKER_IMAGES))

jessie : $$(addprefix $(TARGET_PATH)/,$$(JESSIE_DOCKER_IMAGES)) \
         $$(addprefix $(TARGET_PATH)/,$$(JESSIE_DBG_DOCKER_IMAGES))

###############################################################################
## Standard targets
###############################################################################

.PHONY : $(SONIC_CLEAN_DEBS) $(SONIC_CLEAN_FILES) $(SONIC_CLEAN_TARGETS) $(SONIC_CLEAN_STDEB_DEBS) $(SONIC_CLEAN_WHEELS) $(SONIC_PHONY_TARGETS) clean distclean configure

.INTERMEDIATE : $(SONIC_INSTALL_DEBS) $(SONIC_INSTALL_WHEELS) $(DOCKER_LOAD_TARGETS) docker-start .platform

## To build some commonly used libs. Some submodules depend on these libs.
## It is used in component pipelines. For example: swss needs libnl, libyang
lib-packages: $(addprefix $(DEBS_PATH)/,$(LIBNL3) $(LIBYANG))
