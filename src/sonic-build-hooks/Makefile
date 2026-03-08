SONIC_BUILD_HOOKS = sonic-build-hooks
SONIC_BUILD_HOOKS_VERSION = 1.0
SONIC_BUILD_HOOKS_PACKAGE = $(SONIC_BUILD_HOOKS)_$(SONIC_BUILD_HOOKS_VERSION)_all.deb

BUILDINFO_DIR = buildinfo
TMP_DIR = tmp
SYMBOL_LINKS_SRC_DIR = ../../usr/local/share/buildinfo/scripts
SYMBOL_LINKS = symlink_build_hooks post_run_buildinfo pre_run_buildinfo collect_version_files post_run_cleanup
SONIC_BUILD_HOOKS_TARGET = $(BUILDINFO_DIR)/$(SONIC_BUILD_HOOKS_PACKAGE)
BUILD_ROOT_DIR = $(TMP_DIR)/$(SONIC_BUILD_HOOKS)
DEBIAN_DIR = $(BUILD_ROOT_DIR)/DEBIAN
TRUSTED_GPG_PATH = $(BUILD_ROOT_DIR)/etc/apt/trusted.gpg.d
INSTALL_PATH = $(BUILD_ROOT_DIR)/usr/local/share/buildinfo
SYMBOL_LINK_PATH = $(BUILD_ROOT_DIR)/usr/sbin
SCRIPTS_PATH = $(INSTALL_PATH)/scripts
HOOKS_PATH = $(INSTALL_PATH)/hooks
DPKGTOOL = $(shell which dpkg-deb)


# If the depk-deb not installed, use the docker container to make the debian package
ifeq ($(shell which dpkg-deb),)
BUILD_COMMAND=docker run --user $(shell id -u):$(shell id -g) --rm -v $(shell pwd):/build debian:buster bash -c 'cd /build; dpkg-deb --build $(TMP_DIR)/$(SONIC_BUILD_HOOKS) $(SONIC_BUILD_HOOKS_TARGET)'
else
BUILD_COMMAND=dpkg-deb -Zxz --build $(TMP_DIR)/$(SONIC_BUILD_HOOKS) $(SONIC_BUILD_HOOKS_TARGET)
endif

DEPENDS := $(shell find scripts hooks debian -type f)
$(SONIC_BUILD_HOOKS_TARGET): $(DEPENDS)
	@rm -rf $(BUILDINFO_DIR)/$(SONIC_BUILD_HOOKS) $(TMP_DIR)
	@mkdir -p $(DEBIAN_DIR) $(SCRIPTS_PATH) $(HOOKS_PATH) $(SYMBOL_LINK_PATH) $(TRUSTED_GPG_PATH) $(BUILDINFO_DIR)
	@chmod 0775 $(DEBIAN_DIR)
	@cp debian/* $(DEBIAN_DIR)/
	@cp scripts/* $(SCRIPTS_PATH)/
	@cp hooks/* $(HOOKS_PATH)/
	@set -e; for url in $$(echo $(TRUSTED_GPG_URLS) | sed 's/[,;]/ /g'); do wget -q "$$url" -P "$(TRUSTED_GPG_PATH)/"; done
	@for f in $(SYMBOL_LINKS); do ln -s $(SYMBOL_LINKS_SRC_DIR)/$$f $(SYMBOL_LINK_PATH)/$$f; done
	@$(BUILD_COMMAND)

all: $(SONIC_BUILD_HOOKS_TARGET)
