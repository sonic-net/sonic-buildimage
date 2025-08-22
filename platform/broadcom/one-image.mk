# sonic broadcom one image installer

SONIC_ONE_IMAGE = sonic-broadcom.bin
$(SONIC_ONE_IMAGE)_MACHINE = broadcom
$(SONIC_ONE_IMAGE)_DEPENDENT_MACHINE = broadcom-dnx
$(SONIC_ONE_IMAGE)_IMAGE_TYPE = onie
$(SONIC_ONE_IMAGE)_INSTALLS += $(PDDF_PLATFORM_MODULE)
$(SONIC_ONE_IMAGE)_INSTALLS += $(SYSTEMD_SONIC_GENERATOR)
$(SONIC_ONE_IMAGE)_INSTALLS += $(FLASHROM)
$(SONIC_ONE_IMAGE)_LAZY_INSTALLS += $(DELL_S6000_PLATFORM_MODULE) \
                               $(DELL_Z9264F_PLATFORM_MODULE) \
                               $(DELL_S5212F_PLATFORM_MODULE) \
                               $(DELL_S5224F_PLATFORM_MODULE) \
                               $(DELL_S5232F_PLATFORM_MODULE) \
                               $(DELL_S5248F_PLATFORM_MODULE) \
                               $(DELL_Z9332F_PLATFORM_MODULE) \
                               $(DELL_Z9432F_PLATFORM_MODULE) \
                               $(DELL_S5296F_PLATFORM_MODULE) \
                               $(DELL_Z9100_PLATFORM_MODULE) \
                               $(DELL_S6100_PLATFORM_MODULE) \
                               $(DELL_N3248PXE_PLATFORM_MODULE) \
                               $(DELL_N3248TE_PLATFORM_MODULE) \
                               $(DELL_E3224F_PLATFORM_MODULE) \
			       $(DELL_Z9664F_PLATFORM_MODULE) \
                               $(INGRASYS_S8900_54XC_PLATFORM_MODULE) \
                               $(INGRASYS_S8900_64XC_PLATFORM_MODULE) \
                               $(INGRASYS_S9100_PLATFORM_MODULE) \
                               $(INGRASYS_S8810_32Q_PLATFORM_MODULE) \
                               $(INGRASYS_S9200_64X_PLATFORM_MODULE) \
                               $(ACCTON_AS7712_32X_PLATFORM_MODULE) \
                               $(ACCTON_AS5712_54X_PLATFORM_MODULE) \
                               $(ACCTON_AS7816_64X_PLATFORM_MODULE) \
                               $(ACCTON_AS7716_32X_PLATFORM_MODULE) \
                               $(ACCTON_AS7312_54X_PLATFORM_MODULE) \
                               $(ACCTON_AS7326_56X_PLATFORM_MODULE) \
                               $(ACCTON_AS7716_32XB_PLATFORM_MODULE) \
                               $(ACCTON_AS6712_32X_PLATFORM_MODULE) \
                               $(ACCTON_AS7726_32X_PLATFORM_MODULE) \
                               $(ACCTON_AS4630_54PE_PLATFORM_MODULE) \
                               $(ACCTON_AS4630_54TE_PLATFORM_MODULE) \
                               $(ACCTON_MINIPACK_PLATFORM_MODULE) \
                               $(ACCTON_AS5812_54X_PLATFORM_MODULE) \
                               $(ACCTON_AS5812_54T_PLATFORM_MODULE) \
                               $(ACCTON_AS5835_54X_PLATFORM_MODULE) \
                               $(ACCTON_AS9716_32D_PLATFORM_MODULE) \
                               $(ACCTON_AS9726_32D_PLATFORM_MODULE) \
                               $(ACCTON_AS5835_54T_PLATFORM_MODULE) \
                               $(ACCTON_AS7312_54XS_PLATFORM_MODULE) \
                               $(ACCTON_AS7315_27XB_PLATFORM_MODULE) \
                               $(INVENTEC_D7032Q28B_PLATFORM_MODULE) \
                               $(INVENTEC_D7054Q28B_PLATFORM_MODULE) \
                               $(INVENTEC_D7264Q28B_PLATFORM_MODULE) \
                               $(INVENTEC_D6356_PLATFORM_MODULE) \
                               $(INVENTEC_D6332_PLATFORM_MODULE) \
                               $(CEL_DX010_PLATFORM_MODULE) \
                               $(CEL_HALIBURTON_PLATFORM_MODULE) \
                               $(CEL_SEASTONE2_PLATFORM_MODULE) \
                               $(CEL_DS3000_PLATFORM_MODULE) \
                               $(CEL_DS1000_PLATFORM_MODULE) \
                               $(CEL_QUESTONE2_PLATFORM_MODULE) \
                               $(CEL_SILVERSTONE_V2_PLATFORM_MODULE) \
                               $(CEL_DS2000_PLATFORM_MODULE) \
                               $(DELTA_AG9032V1_PLATFORM_MODULE) \
                               $(DELTA_AG9064_PLATFORM_MODULE) \
                               $(DELTA_AG5648_PLATFORM_MODULE) \
                               $(DELTA_ET6248BRB_PLATFORM_MODULE) \
                               $(QUANTA_IX1B_32X_PLATFORM_MODULE) \
                               $(QUANTA_IX7_32X_PLATFORM_MODULE) \
                               $(QUANTA_IX7_BWDE_32X_PLATFORM_MODULE) \
                               $(QUANTA_IX8_56X_PLATFORM_MODULE) \
                               $(QUANTA_IX8A_BWDE_56X_PLATFORM_MODULE) \
                               $(QUANTA_IX8C_56X_PLATFORM_MODULE) \
                               $(QUANTA_IX9_32X_PLATFORM_MODULE) \
                               $(MITAC_LY1200_32X_PLATFORM_MODULE) \
                               $(ALPHANETWORKS_SNH60A0_320FV2_PLATFORM_MODULE) \
                               $(ALPHANETWORKS_SNH60B0_640F_PLATFORM_MODULE) \
                               $(ALPHANETWORKS_SNJ60D0_320F_PLATFORM_MODULE) \
                               $(ALPHANETWORKS_BES2348T_PLATFORM_MODULE) \
                               $(BRCM_XLR_GTS_PLATFORM_MODULE) \
                               $(DELTA_AG9032V2A_PLATFORM_MODULE) \
                               $(JUNIPER_QFX5210_PLATFORM_MODULE) \
                               $(CEL_SILVERSTONE_PLATFORM_MODULE) \
                               $(JUNIPER_QFX5200_PLATFORM_MODULE) \
                               $(DELTA_AGC032_PLATFORM_MODULE) \
                               $(RUIJIE_B6510_48VS8CQ_PLATFORM_MODULE) \
                               $(RAGILE_RA_B6510_48V8C_PLATFORM_MODULE) \
                               $(NOKIA_IXR7250_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220D4_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H3_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H4_32D_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H4_64D_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H5_64O_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H5_64D_PLATFORM_MODULE) \
                               $(NOKIA_IXR7220H5_32D_PLATFORM_MODULE) \
                               $(NOKIA_IXR7250X1B_PLATFORM_MODULE) \
                               $(NOKIA_IXR7250X3B_PLATFORM_MODULE) \
                               $(TENCENT_TCS8400_PLATFORM_MODULE) \
                               $(TENCENT_TCS9400_PLATFORM_MODULE) \
                               $(UFISPACE_S6301_56ST_PLATFORM_MODULE) \
                               $(UFISPACE_S7801_54XS_PLATFORM_MODULE) \
                               $(UFISPACE_S8901_54XC_PLATFORM_MODULE) \
                               $(UFISPACE_S9110_32X_PLATFORM_MODULE) \
                               $(UFISPACE_S9300_32D_PLATFORM_MODULE) \
                               $(UFISPACE_S9301_32D_PLATFORM_MODULE) \
                               $(UFISPACE_S9301_32DB_PLATFORM_MODULE) \
                               $(UFISPACE_S9321_64E_PLATFORM_MODULE) \
                               $(UFISPACE_S9321_64EO_PLATFORM_MODULE) \
                               $(MICAS_M2_W6510_48V8C_PLATFORM_MODULE) \
			       $(MICAS_M2_W6510_48GT4V_PLATFORM_MODULE) \
			       $(MICAS_M2_W6520_24DC8QC_PLATFORM_MODULE) \
			       $(MICAS_M2_W6940_128QC_PLATFORM_MODULE) \
			       $(MICAS_M2_W6930_64QC_PLATFORM_MODULE) \
			       $(MICAS_M2_W6940_64OC_PLATFORM_MODULE) \
			       $(MICAS_M2_W6920_32QC2X_PLATFORM_MODULE) \
			       $(MICAS_M2_W6510_32C_PLATFORM_MODULE) \
                   $(MICAS_M2_W6520_48C8QC_PLATFORM_MODULE)

$(SONIC_ONE_IMAGE)_LAZY_BUILD_INSTALLS = $(BRCM_OPENNSL_KERNEL) $(BRCM_DNX_OPENNSL_KERNEL)
ifeq ($(INSTALL_DEBUG_TOOLS),y)
$(SONIC_ONE_IMAGE)_DOCKERS += $(SONIC_INSTALL_DOCKER_DBG_IMAGES)
$(SONIC_ONE_IMAGE)_DOCKERS += $(filter-out $(patsubst %-$(DBG_IMAGE_MARK).gz,%.gz, $(SONIC_INSTALL_DOCKER_DBG_IMAGES)), $(SONIC_INSTALL_DOCKER_IMAGES))
else
$(SONIC_ONE_IMAGE)_DOCKERS = $(SONIC_INSTALL_DOCKER_IMAGES)
endif

SONIC_INSTALLERS += $(SONIC_ONE_IMAGE)

