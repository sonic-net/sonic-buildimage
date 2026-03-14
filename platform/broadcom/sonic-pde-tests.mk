# sonic pde package

SONIC_PLATFORM_PDE = sonic-platform-pde_1.0_amd64.deb
$(SONIC_PLATFORM_PDE)_SRC_PATH = $(SRC_PATH)/sonic-platform-pde
$(SONIC_PLATFORM_PDE)_DEPENDS += $(BRCM_XGS_SAI) $(BRCM_XGS_SAI_DEV) $(SWIG)

SONIC_DPKG_DEBS += $(SONIC_PLATFORM_PDE)
