SAI_INIT_CONFIG_FILE=/usr/share/sonic/hwsku/th-a7060-cx32s-8x100G+48x50G.config.bcm
SAI_NUM_ECMP_MEMBERS=64
# BROADCOM_LEGACY_SAI_COMPAT: TH1 (BCM56960) has no streaming telemetry platform driver;
# sai_query_stats_st_capability crashes in brcm_sai_st_pd_ctr_cap_list_get.
SAI_STATS_ST_CAPABILITY_SUPPORTED=0

# BROADCOM_LEGACY_SAI_COMPAT: sai_get_stats_ext is not supported for switch objects on TH1 (BCM56960).
# Setting to 0 disables use_sai_stats_ext in FlexCounter for COUNTER_TYPE_SWITCH.
SAI_STATS_EXT_SWITCH_SUPPORTED=0
