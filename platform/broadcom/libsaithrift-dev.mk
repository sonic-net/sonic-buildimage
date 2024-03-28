# libsaithrift-dev package

SAI_VER = 0.9.4

LIBSAITHRIFT_DEV = libsaithrift$(SAITHRIFT_VER)-dev_$(SAI_VER)_amd64.deb
$(LIBSAITHRIFT_DEV)_SRC_PATH = $(SRC_PATH)/sonic-sairedis/SAI
#Support two different versions of thrift
ifeq ($(SAITHRIFT_V2),y)
$(LIBSAITHRIFT_DEV)_DEPENDS += $(LIBTHRIFT_0_14_1) $(LIBTHRIFT_0_14_1_DEV) $(PYTHON3_THRIFT_0_14_1) $(THRIFT_0_14_1_COMPILER)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(LIBTHRIFT_0_14_1)
$(LIBSAITHRIFT_DEV)_BUILD_ENV = SAITHRIFTV2=true SAITHRIFT_VER=v2
else
$(LIBSAITHRIFT_DEV)_DEPENDS += $(LIBTHRIFT) $(LIBTHRIFT_DEV) $(PYTHON_THRIFT) $(THRIFT_COMPILER)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(LIBTHRIFT)
endif
$(LIBSAITHRIFT_DEV)_DEPENDS += $(BRCM_XGS_SAI) $(BRCM_XGS_SAI_DEV)
$(LIBSAITHRIFT_DEV)_RDEPENDS += $(BRCM_XGS_SAI)
SONIC_DPKG_DEBS += $(LIBSAITHRIFT_DEV)

PYTHON_SAITHRIFT = python-saithrift$(SAITHRIFT_VER)_$(SAI_VER)_amd64.deb
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(PYTHON_SAITHRIFT)))

SAISERVER = saiserver$(SAITHRIFT_VER)_$(SAI_VER)_amd64.deb
$(SAISERVER)_RDEPENDS += $(LIBSAITHRIFT_DEV)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER)))

SAISERVER_DBG = saiserver$(SAITHRIFT_VER)-dbg_$(SAI_VER)_amd64.deb
$(SAISERVER_DBG)_RDEPENDS += $(SAISERVER)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER_DBG)))

# A workaround to fix the SAI header files not checkout issue
# Once the SAI upgraded to the version that contains the patch, then the following change can be removed
PATCH_DEPEND = $(shell cd $(SRC_PATH)/sonic-sairedis; QUILT_PATCHES=../sonic-sairedis.patch quilt push -a > /dev/null)
$(eval $(echo $(PATCH_DEPEND)))
