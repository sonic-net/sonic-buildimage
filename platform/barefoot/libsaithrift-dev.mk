# libsaithrift-dev package
SAI_VER = 0.9.4
LIBSAITHRIFT_DEV = libsaithrift$(SAITHRIFT_VER)-dev_$(SAI_VER)_$(CONFIGURED_ARCH).deb
$(LIBSAITHRIFT_DEV)_SRC_PATH = $(SRC_PATH)/sonic-sairedis/SAI
$(LIBSAITHRIFT_DEV)_DEPENDS += $(LIBTHRIFT_0_14_1) $(LIBTHRIFT_0_14_1_DEV) \
                               $(PYTHON3_THRIFT_0_14_1) $(THRIFT_0_14_1_COMPILER)  \
                               $(BFN_SAI)
# Support two version of saithift for syncd-rpc
# Support two different versions of thrift
# Only saithriftv2 will build saithriftv2
ifeq ($(SAITHRIFT_V2),y)
$(LIBSAITHRIFT_DEV)_BUILD_ENV = SAITHRIFTV2=true SAITHRIFT_VER=v2 GEN_SAIRPC_OPTS="--adapter_logger"
endif

#$(LIBSAIVS) $(LIBSAIVS_DEV) $(LIBSAIMETADATA) $(LIBSAIMETADATA_DEV)

# $(LIBSAITHRIFT_DEV)_BUILD_ENV = platform=v

$(LIBSAITHRIFT_DEV)_RDEPENDS += $(LIBTHRIFT_0_14_1) $(BFN_SAI)
SONIC_DPKG_DEBS += $(LIBSAITHRIFT_DEV)

PYTHON_SAITHRIFT = python-saithrift$(SAITHRIFT_VER)_$(SAI_VER)_amd64.deb
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(PYTHON_SAITHRIFT)))

SAISERVER = saiserver$(SAITHRIFT_VER)_$(SAI_VER)_amd64.deb
$(SAISERVER)_RDEPENDS += $(LIBTHRIFT_0_14_1) $(BFN_SAI)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER)))

SAISERVER_DBG = saiserver$(SAITHRIFT_VER)-dbg_$(SAI_VER)_amd64.deb
$(SAISERVER_DBG)_RDEPENDS += $(SAISERVER)
$(eval $(call add_extra_package,$(LIBSAITHRIFT_DEV),$(SAISERVER_DBG)))
