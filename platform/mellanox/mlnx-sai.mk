# Mellanox SAI

MLNX_SAI_VERSION = SAIBuild2605.37.0.31
MLNX_SAI_ASSETS_GITHUB_URL = https://github.com/Mellanox/Spectrum-SDK-Drivers-SONiC-Bins
MLNX_SAI_ASSETS_RELEASE_TAG = sai-$(MLNX_SAI_VERSION)-$(BLDENV)-$(CONFIGURED_ARCH)
MLNX_SAI_ASSETS_URL = $(MLNX_SAI_ASSETS_GITHUB_URL)/releases/download/$(MLNX_SAI_ASSETS_RELEASE_TAG)
MLNX_SAI_DEB_VERSION = $(subst -,.,$(subst _,.,$(MLNX_SAI_VERSION)))

# Place here URL where SAI sources exist
MLNX_SAI_SOURCE_BASE_URL = 

ifneq ($(MLNX_SAI_SOURCE_BASE_URL), )
SAI_FROM_SRC = y
else
SAI_FROM_SRC = n
endif

export MLNX_SAI_VERSION MLNX_SAI_SOURCE_BASE_URL

MLNX_SAI = mlnx-sai_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb
$(MLNX_SAI)_SRC_PATH = $(PLATFORM_PATH)/mlnx-sai
$(MLNX_SAI)_RDEPENDS += $(MLNX_SDK_RDEBS) $(LIBNL_ROUTE3)
$(eval $(call add_conflict_package,$(MLNX_SAI),$(LIBSAIVS_DEV)))
MLNX_SAI_DBGSYM = mlnx-sai-dbgsym_1.mlnx.$(MLNX_SAI_VERSION)_$(CONFIGURED_ARCH).deb

define make_url
	$(1)_URL = $(MLNX_SAI_ASSETS_URL)/$(1)

endef

# Where debug symbols live depends on how SAI is obtained. Registering a
# dbgsym target that the recipe cannot produce turns into a missing
# prerequisite, so each branch only declares what it can actually deliver.
#
# Built from source (SAI_FROM_SRC=y):
#   debuild honors DEB_BUILD_OPTIONS exported by slave.mk. With SPLIT_DBGSYM=y
#   (the default) dh_strip moves symbols into a separate dbgsym package and
#   the runtime deb ships stripped, so that package is registered as a derived
#   package of the runtime deb - one recipe run emits both. With SPLIT_DBGSYM=n
#   the nostrip option keeps symbols inside the runtime deb and no dbgsym
#   package is ever emitted, so nothing is registered and the -dbg image just
#   reuses the runtime deb. _DEPENDS is the compile set: SDK -dev headers and
#   libnl-route headers; -dev still pulls the runtime SDK via its own _DEPENDS.
#
# Downloaded (SAI_FROM_SRC=n):
#   Both debs are prebuilt release assets that always exist, independent of
#   SPLIT_DBGSYM, so the flag plays no part here. They are registered as two
#   independent online debs, each with its own URL, which keeps the dbgsym
#   download lazy: it is fetched only when something actually depends on it,
#   i.e. when the -dbg image is built. Attaching it to the runtime deb with
#   add_derived_package instead would drag it down on every build. _DEPENDS is
#   the dpkg -i set: runtime SDK and libnl-route, not -dev, so a full image
#   that only installs SAI does not fetch SDK headers.
ifeq ($(SAI_FROM_SRC), y)
$(MLNX_SAI)_DEPENDS += $(MLNX_SDK_DEBS) $(LIBNL_ROUTE3_DEV)
SONIC_MAKE_DEBS += $(MLNX_SAI)
ifeq ($(SPLIT_DBGSYM),y)
$(eval $(call add_derived_package,$(MLNX_SAI),$(MLNX_SAI_DBGSYM)))
endif
else
$(MLNX_SAI)_DEPENDS += $(MLNX_SDK_RDEBS) $(LIBNL_ROUTE3)
SONIC_ONLINE_DEBS += $(MLNX_SAI) $(MLNX_SAI_DBGSYM)
$(eval $(foreach deb,$(MLNX_SAI) $(MLNX_SAI_DBGSYM),$(call make_url,$(deb))))
endif
