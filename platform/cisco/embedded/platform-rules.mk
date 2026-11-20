# Copyright 2015-2026 Cisco Systems, Inc.
# SPDX-License-Identifier: BSD-3-Clause
# See platform/cisco/COPYRIGHT for the full license text.

# Cisco platform groups.

# Cisco binaries are published as independent GitHub Releases.  Keep the
# release tag derived from the Debian version so a package can be updated
# without coupling it to a SONiC release or to another Cisco package family.
CISCO_RELEASES_URL ?= https://github.com/cisco/sonic-releases/releases/download
CISCO_MODULES_RELEASES_URL ?= https://github.com/cisco/sonic-modules/releases/download
CISCO_PLATFORM_RELEASES_URL ?= https://github.com/cisco/sonic-platform/releases/download
CISCO_ARTIFACT_ROOT ?=

# Cisco's internal GitHub Enterprise instance returns 404 for the browser
# style /releases/download URL.  Resolve internal Release assets through the
# authenticated REST API; public GitHub continues to use direct URLs.
CISCO_RELEASES_API_BASE ?= https://wwwin-github.cisco.com/api/v3
CISCO_RELEASES_API_REPO ?= cisco/sonic-releases
CISCO_MODULES_API_REPO ?= cisco/sonic-modules
CISCO_PLATFORM_API_REPO ?= cisco/sonic-platform
CISCO_RELEASES_USE_API ?= $(if $(findstring wwwin-github.cisco.com,$(CISCO_RELEASES_URL) $(CISCO_MODULES_RELEASES_URL) $(CISCO_PLATFORM_RELEASES_URL)),y,n)

# The normal SONiC online-deb rule uses curl directly.  Internal GitHub
# Enterprise Release assets require authentication, so create a curl config
# inside the build workspace when a token file is supplied.  The config keeps
# the token out of the curl command line and is generated only in the build
# environment; it is not a source artifact.
CISCO_ARTIFACT_CURL_OPTIONS ?=
CISCO_ARTIFACT_CURL_CONFIG ?= $(CURDIR)/target/.cisco-artifact-curl.conf

ifneq ($(strip $(CISCO_ARTIFACT_TOKEN_FILE)),)
ifneq ($(wildcard $(CISCO_ARTIFACT_TOKEN_FILE)),)
$(shell mkdir -p "$(dir $(CISCO_ARTIFACT_CURL_CONFIG))"; \
	token=$$(tr -d '\r\n' < "$(CISCO_ARTIFACT_TOKEN_FILE)"); \
	printf 'header = "Authorization: Bearer %s"\n' "$$token" > "$(CISCO_ARTIFACT_CURL_CONFIG)"; \
	chmod 600 "$(CISCO_ARTIFACT_CURL_CONFIG)")
CISCO_ARTIFACT_CURL_OPTIONS += --config $(CISCO_ARTIFACT_CURL_CONFIG)
else
$(error CISCO_ARTIFACT_TOKEN_FILE does not exist: $(CISCO_ARTIFACT_TOKEN_FILE))
endif
endif

ifeq ($(strip $(CISCO_ARTIFACT_ROOT)),)
ifeq ($(CISCO_RELEASES_USE_API),y)
CISCO_ARTIFACT_URL_MODE := api
else
CISCO_ARTIFACT_URL_MODE := direct
endif
else
CISCO_ARTIFACT_URL_MODE := local
endif

ifeq ($(CISCO_ARTIFACT_URL_MODE),api)
CISCO_ARTIFACT_PACKAGE_CURL_OPTIONS := \
	$(CISCO_ARTIFACT_CURL_OPTIONS) -H 'Accept: application/octet-stream'
else
CISCO_ARTIFACT_PACKAGE_CURL_OPTIONS := $(CISCO_ARTIFACT_CURL_OPTIONS)
endif

define cisco_release_asset_url
$(shell \
	tag_uri=$$(jq -rn --arg value "$(2)" '$$value | @uri'); \
	release=$$(curl --config "$(CISCO_ARTIFACT_CURL_CONFIG)" --fail --silent --show-error --location \
		-H 'Accept: application/vnd.github+json' \
		"$(CISCO_RELEASES_API_BASE)/repos/$(1)/releases/tags/$$tag_uri"); \
	asset_id=$$(printf '%s' "$$release" | jq -r --arg name "$(3)" \
		'.assets[] | select(.name == $$name) | .id' | head -n 1); \
	test -n "$$asset_id" || { echo "Cisco Release asset not found: repo=$(1) tag=$(2) name=$(3)" >&2; exit 1; }; \
	printf '%s/repos/%s/releases/assets/%s' \
		"$(CISCO_RELEASES_API_BASE)" "$(1)" "$$asset_id")
endef

define cisco_package_url
$(if $(filter api,$(CISCO_ARTIFACT_URL_MODE)),$(call cisco_release_asset_url,$(3),$(notdir $(2)),$(1)),$(2)/$(1))
endef

CISCO_PLATFORM_ID := cisco
CISCO_PLATFORM_HW_SKUS := \
    x86_64-8102_64h_o-r0 \
    x86_64-8101_32fh_o-r0 \
    x86_64-8101_32fh_o_c01-r0

CISCO_8102_64H_O_VARIANT_PLATFORMS := \
    x86_64-8102_64h_o-r0
CISCO_8201_32FH_O_VARIANT_PLATFORMS := \
    x86_64-8101_32fh_o-r0
CISCO_8101_32FH_O_C01_VARIANT_PLATFORMS := \
    x86_64-8101_32fh_o_c01-r0

CISCO_8000_PLATFORMS := $(CISCO_PLATFORM_HW_SKUS)
CISCO_PLATFORMS_GIBRALTAR := $(CISCO_8000_PLATFORMS)
CISCO_PLATFORM_HW_SKU ?= $(firstword $(CISCO_PLATFORM_HW_SKUS))
PLATFORM_HW_SKU ?= $(CISCO_PLATFORM_HW_SKU)

# Platform capabilities.
CISCO_PLATFORM_CAPABILITIES :=

# Release tags are intentionally package-family specific.  The Debian version
# remains the source of truth for both the URL and the package filename.
CISCO_SDK_RELEASE_TAG ?= cisco-gibraltar-$(CISCO_Q200_SILICON_ONE_VERSION)
CISCO_PLATFORM_MODULE_RELEASE_TAG ?= sonic-platform-cisco-8000-$(CISCO_8000_PLATFORM_MODULE_VERSION)
CISCO_KMOD_RELEASE_TAG ?= sonic-platform-cisco-8000-kmod-$(CISCO_8000_PLATFORM_MODULE_VERSION)
CISCO_S1_Q200_RELEASE_TAG ?= sonic-platform-cisco-8000-s1-q200-$(CISCO_8000_PLATFORM_MODULE_VERSION)
CISCO_WB_BSP_RELEASE_TAG ?= cisco-wb-bsp-$(CISCO_WB_BSP_VERSION)
CISCO_WB_BSP2_RELEASE_TAG ?= cisco-wb-bsp2-$(CISCO_WB_BSP2_VERSION)
CISCO_WB_FPD_RELEASE_TAG ?= cisco-wb-fpd-$(CISCO_WB_FPD_VERSION)
CISCO_SECURITY_RELEASE_TAG ?= cisco-security-$(CISCO_SECURITY_VERSION)

ifeq ($(strip $(CISCO_ARTIFACT_ROOT)),)
CISCO_SDK_URL := $(CISCO_RELEASES_URL)/$(CISCO_SDK_RELEASE_TAG)
CISCO_PLATFORM_MODULE_URL := $(CISCO_PLATFORM_RELEASES_URL)/$(CISCO_PLATFORM_MODULE_RELEASE_TAG)
CISCO_KMOD_URL := $(CISCO_MODULES_RELEASES_URL)/$(CISCO_KMOD_RELEASE_TAG)
CISCO_S1_Q200_URL := $(CISCO_MODULES_RELEASES_URL)/$(CISCO_S1_Q200_RELEASE_TAG)
CISCO_WB_URL := $(CISCO_RELEASES_URL)/$(CISCO_WB_BSP_RELEASE_TAG)
CISCO_WB_BSP2_URL := $(CISCO_RELEASES_URL)/$(CISCO_WB_BSP2_RELEASE_TAG)
CISCO_WB_FPD_URL := $(CISCO_RELEASES_URL)/$(CISCO_WB_FPD_RELEASE_TAG)
CISCO_SECURITY_URL := $(CISCO_RELEASES_URL)/$(CISCO_SECURITY_RELEASE_TAG)
else
# A flat local cache is useful for offline builds and release validation.
CISCO_SDK_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_PLATFORM_MODULE_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_KMOD_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_S1_Q200_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_WB_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_WB_BSP2_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_WB_FPD_URL := $(CISCO_ARTIFACT_ROOT)
CISCO_SECURITY_URL := $(CISCO_ARTIFACT_ROOT)
endif

define add_prebuilt_debs
CISCO_EMBEDDED_PREBUILT_DEBS += $(1)
SONIC_ONLINE_DEBS += $(1)
$(1)_URL := $(call cisco_package_url,$(1),$(2),$(if $(3),$(3),$(CISCO_RELEASES_API_REPO)))
$(1)_CURL_OPTIONS := $(CISCO_ARTIFACT_PACKAGE_CURL_OPTIONS)
endef
