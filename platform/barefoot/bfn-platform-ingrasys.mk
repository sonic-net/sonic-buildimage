BFN_INGRASYS_PLATFORM = bfnplatform-ingrasys_8.2.0_amd64.deb
$(BFN_INGRASYS_PLATFORM)_URL = "https://github.com/Ingrasys-sonic/packages/raw/master/lib/bfnplatform-ingrasys_8.4.0_8.5.0_amd64.deb"

SONIC_ONLINE_DEBS += $(BFN_INGRASYS_PLATFORM) # $(BFN_SAI_DEV)
$(BFN_SAI_DEV)_DEPENDS += $(BFN_INGRASYS_PLATFORM)
