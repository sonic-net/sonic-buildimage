BFN_UFISPACE_PLATFORM = bfnplatform-ufispace_1.0.0_amd64.deb
$(BFN_UFISPACE_PLATFORM)_URL = "https://github.com/ufispace-dev/bf_sde_bsp/raw/master/sonic/9.12.0/$(BFN_UFISPACE_PLATFORM)"

SONIC_ONLINE_DEBS += $(BFN_UFISPACE_PLATFORM) # $(BFN_SAI_DEV)
$(BFN_SAI_DEV)_DEPENDS += $(BFN_UFISPACE_PLATFORM)
