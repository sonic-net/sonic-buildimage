WNC_OSW1800_PLATFORM = bfnplatformwnc_1.0.0_amd64.deb
$(WNC_OSW1800_PLATFORM)_URL = "https://github.com/YaoTien/download/raw/master/sonic/sde/7_0_0_18/bfnplatformwnc_1.0.0_amd64.deb"

SONIC_ONLINE_DEBS += $(WNC_OSW1800_PLATFORM) # $(BFN_SAI_DEV)
$(BFN_SAI_DEV)_DEPENDS += $(WNC_OSW1800_PLATFORM)
