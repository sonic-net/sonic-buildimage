#ifndef BMC8140_REGMAP_H
#define BMC8140_REGMAP_H

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO
#include <linux/regmap.h>
#endif
#else
#include <stddef.h>
#include <inttypes.h>
#endif

struct Bmc8140_dev_reg
{
    uint8_t pad1[8192];

    uint32_t    BMC8140_IDP_DATA;           /* System IDP data (4096 bytes) */
    uint8_t pad2[4092];

    uint32_t    BMC8140_IDP_THEEND;         /* Reserved */
    uint8_t pad3[4092];

    uint32_t    BMC8140_BASE_FID;           /* 8140 BMC FPGA ID */
    uint32_t    BMC8140_BASE_MJR;           /* Major revision number */
    uint32_t    BMC8140_BASE_MNR;           /* Minor revision number */
    uint32_t    BMC8140_BASE_BLD;           /* Build revision number */
    uint32_t    BMC8140_BASE_SCRATCHPAD;    /* Scratchpad test register */
    uint32_t    BMC8140_BASE_BMC_DATE;      /* Used for development purposes. */
    uint32_t    BMC8140_BASE_RESET_HISTORY_0; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_0; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_1; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_1; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_2; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_2; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_3; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_3; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_4; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_4; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_5; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_5; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_6; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_6; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_7; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_7; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_8; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_8; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_9; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_9; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_10; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_10; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_11; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_11; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_12; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_12; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_13; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_13; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_14; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_14; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_RESET_HISTORY_15; /* Reset History */
    uint32_t    BMC8140_BASE_RESET_HISTORY_TS_UPR_15; /* Reset History Timestamp, upper bits 48-16 */
    uint32_t    BMC8140_BASE_MSI_CTRL;      /* MSI Control Register */
    uint32_t    BMC8140_BASE_ISR_MASTER_EVENT; /* Master Interrupt Event Register */
    uint32_t    BMC8140_BASE_ISM_MASTER_EVENT; /* Master Interrupt Mask Register */
    uint32_t    BMC8140_BASE_ISR_TEMP;      /* Temperature Interrupt */
    uint32_t    BMC8140_BASE_ISM_TEMP;      /* Mask Temperature Interrupt */
    uint32_t    BMC8140_BASE_STATUS_TEMP;   /* Status Temperature */
    uint32_t    BMC8140_BASE_ISR_FAN;       /* Fan Interrupt */
    uint32_t    BMC8140_BASE_ISM_FAN;       /* Mask Fan Interrupt */
    uint32_t    BMC8140_BASE_STATUS_FAN;    /* Status Fan */
    uint32_t    BMC8140_BASE_ISR_WDT;       /* Watchdog Timer Interrupt */
    uint32_t    BMC8140_BASE_ISM_WDT;       /* Mask Watchdog Timer Interrupt */
    uint32_t    BMC8140_BASE_STATUS_WDT;    /* Status Watchdog Timer */
    uint8_t pad4[824];

    uint32_t    BMC8140_ADC_0;              /* Value extracted from BMC ADC channel BMC_ADC0, for rail P1V2_VDDQ_AB */
    uint32_t    BMC8140_ADC_1;              /* Value extracted from BMC ADC channel BMC_ADC1, for rail 0_9V_PCIEX_DPOL */
    uint32_t    BMC8140_ADC_2;              /* Value extracted from BMC ADC channel BMC_ADC2, for rail P0V6_VTT_AB */
    uint32_t    BMC8140_ADC_3;              /* Value extracted from BMC ADC channel BMC_ADC3, for rail 0_9V_PCIEX_LDO */
    uint32_t    BMC8140_ADC_4;              /* Value extracted from BMC ADC channel BMC_ADC4, for rail P2V5_VPP_AB */
    uint32_t    BMC8140_ADC_5;              /* Value extracted from BMC ADC channel BMC_ADC5, for rail 1_8V_PCIE */
    uint32_t    BMC8140_ADC_6;              /* Value extracted from BMC ADC channel BMC_ADC6, for rail PVCCIN_CPU */
    uint32_t    BMC8140_ADC_7;              /* Value extracted from BMC ADC channel BMC_ADC7, for rail P1V8_AUX */
    uint32_t    BMC8140_ADC_8;              /* Value extracted from BMC ADC channel BMC_ADC8, for rail P3V3 */
    uint32_t    BMC8140_ADC_9;              /* Value extracted from BMC ADC channel BMC_ADC9, for rail P1V05 */
    uint32_t    BMC8140_ADC_10;             /* Value extracted from BMC ADC channel BMC_ADC10, for rail PVNN_PCH */
    uint32_t    BMC8140_ADC_11;             /* Value extracted from BMC ADC channel BMC_ADC11, for rail PVCCANA_CPU */
    uint32_t    BMC8140_ADC_12;             /* Value extracted from BMC ADC channel BMC_ADC12, for rail P3V3_CPU */
    uint32_t    BMC8140_ADC_13;             /* Value extracted from BMC ADC channel BMC_ADC13, for rail 1V_PHY */
    uint32_t    BMC8140_ADC_14;             /* Value extracted from BMC ADC channel BMC_ADC14, for rail PVNN_NAC */
    uint32_t    BMC8140_ADC_15;             /* Value extracted from BMC ADC channel BMC_ADC15, for rail P3V3_BMC_BATT */
    uint32_t    BMC8140_ADC_THRESH_WARN_0;  /* ADC_0 (P1V2_VDDQ_AB) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_1;  /* ADC_1 (0_9V_PCIEX_DPOL) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_2;  /* ADC_2 (P0V6_VTT_AB) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_3;  /* ADC_3 (0_9V_PCIEX_LDO) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_4;  /* ADC_4 (P2V5_VPP_AB) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_5;  /* ADC_5 (1_8V_PCIE) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_6;  /* ADC_6 (PVCCIN_CPU) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_7;  /* ADC_7 (P1V8_AUX) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_8;  /* ADC_8 (P3V3) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_9;  /* ADC_9 (P1V05) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_10; /* ADC_10 (PVNN_PCH) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_11; /* ADC_11 (PVCCANA_CPU) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_12; /* ADC_12 (P3V3_CPU) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_13; /* ADC_13 (1V_PHY) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_14; /* ADC_14 (PVNN_NAC) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_WARN_15; /* ADC_15 (P3V3_BMC_BATT) voltage warning thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_0;  /* ADC_0 (P1V2_VDDQ_AB) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_1;  /* ADC_1 (0_9V_PCIEX_DPOL) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_2;  /* ADC_2 (P0V6_VTT_AB) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_3;  /* ADC_3 (0_9V_PCIEX_LDO) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_4;  /* ADC_4 (P2V5_VPP_AB) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_5;  /* ADC_5 (1_8V_PCIE) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_6;  /* ADC_6 (PVCCIN_CPU) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_7;  /* ADC_7 (P1V8_AUX) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_8;  /* ADC_8 (P3V3) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_9;  /* ADC_9 (P1V05) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_10; /* ADC_10 (PVNN_PCH) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_11; /* ADC_11 (PVCCANA_CPU) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_12; /* ADC_12 (P3V3_CPU) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_13; /* ADC_13 (1V_PHY) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_14; /* ADC_14 (PVNN_NAC) voltage critical thresholds for SEL logging */
    uint32_t    BMC8140_ADC_THRESH_CRIT_15; /* ADC_15 (P3V3_BMC_BATT) voltage critical thresholds for SEL logging */
    uint8_t pad5[832];

    uint32_t    BMC8140_TMP_0;              /* Remote/Local values extracted from TMP421 sensor: Near DIMMS */
    uint32_t    BMC8140_TMP_1;              /* Remote/Local values extracted from TMP421 sensor: Near SSD */
    uint32_t    BMC8140_TMP_2;              /* Remote/Local values extracted from TMP421 sensor: CPU Exhaust */
    uint32_t    BMC8140_TMP_3;              /* Remote/Local values extracted from TMP421 sensor: Near Pcie Switch */
    uint32_t    BMC8140_TMP_4;              /* Remote/Local values extracted from TMP421 sensor: MB Right */
    uint32_t    BMC8140_TMP_5;              /* Remote/Local values extracted from TMP421 sensor: Inlet */
    uint32_t    BMC8140_TMP_6;              /* Remote/Local values extracted from TMP421 sensor: Between J2C */
    uint32_t    BMC8140_TMP_7;              /* Remote/Local values extracted from TMP421 sensor: MB Exhaust */
    uint32_t    BMC8140_TMP_8;              /* Remote/Local values extracted from TMP421 sensor: MB Left */
    uint32_t    BMC8140_TMP_THRESH_WARN_0;  /* TMP_0 (Near DIMMS) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_1;  /* TMP_1 (Near SSD) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_2;  /* TMP_2 (CPU Exhaust) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_3;  /* TMP_3 (Near Pcie Switch) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_4;  /* TMP_4 (MB Right) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_5;  /* TMP_5 (Inlet) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_6;  /* TMP_6 (Between J2C) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_7;  /* TMP_7 (MB Exhaust) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_WARN_8;  /* TMP_8 (MB Left) Remote/Local Temperature warning thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_0;  /* TMP_0 (Near DIMMS) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_1;  /* TMP_1 (Near SSD) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_2;  /* TMP_2 (CPU Exhaust) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_3;  /* TMP_3 (Near Pcie Switch) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_4;  /* TMP_4 (MB Right) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_5;  /* TMP_5 (Inlet) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_6;  /* TMP_6 (Between J2C) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_7;  /* TMP_7 (MB Exhaust) Remote/Local Temperature critical thresholds for SEL Logging */
    uint32_t    BMC8140_TMP_THRESH_CRIT_8;  /* TMP_8 (MB Left) Remote/Local Temperature critical thresholds for SEL Logging */
    uint8_t pad6[916];

    uint32_t    BMC8140_FAN_STATUS_0;       /* Status for FAN_0 of Fan FRU 0 */
    uint32_t    BMC8140_FAN_STATUS_1;       /* Status for FAN_1 of Fan FRU 0 */
    uint32_t    BMC8140_FAN_STATUS_2;       /* Status for FAN_2 of Fan FRU 1 */
    uint32_t    BMC8140_FAN_STATUS_3;       /* Status for FAN_3 of Fan FRU 1 */
    uint32_t    BMC8140_FAN_STATUS_4;       /* Status for FAN_4 of Fan FRU 2 */
    uint32_t    BMC8140_FAN_STATUS_5;       /* Status for FAN_5 of Fan FRU 2 */
    uint32_t    BMC8140_FAN_STATUS_6;       /* Status for FAN_6 of Fan FRU 3 */
    uint32_t    BMC8140_FAN_STATUS_7;       /* Status for FAN_7 of Fan FRU 3 */
    uint32_t    BMC8140_FAN_STATUS_8;       /* Status for FAN_8 of Fan FRU 4 */
    uint32_t    BMC8140_FAN_STATUS_9;       /* Status for FAN_9 of Fan FRU 4 */
    uint32_t    BMC8140_FAN_STATUS_10;      /* Status for FAN_10 of Fan FRU 5 */
    uint32_t    BMC8140_FAN_STATUS_11;      /* Status for FAN_11 of Fan FRU 5 */
    uint32_t    BMC8140_FAN_TACH_0;         /* Tachometer readings for dual rotors: FAN_0 */
    uint32_t    BMC8140_FAN_TACH_1;         /* Tachometer readings for dual rotors: FAN_1 */
    uint32_t    BMC8140_FAN_TACH_2;         /* Tachometer readings for dual rotors: FAN_2 */
    uint32_t    BMC8140_FAN_TACH_3;         /* Tachometer readings for dual rotors: FAN_3 */
    uint32_t    BMC8140_FAN_TACH_4;         /* Tachometer readings for dual rotors: FAN_4 */
    uint32_t    BMC8140_FAN_TACH_5;         /* Tachometer readings for dual rotors: FAN_5 */
    uint32_t    BMC8140_FAN_TACH_6;         /* Tachometer readings for dual rotors: FAN_6 */
    uint32_t    BMC8140_FAN_TACH_7;         /* Tachometer readings for dual rotors: FAN_7 */
    uint32_t    BMC8140_FAN_TACH_8;         /* Tachometer readings for dual rotors: FAN_8 */
    uint32_t    BMC8140_FAN_TACH_9;         /* Tachometer readings for dual rotors: FAN_9 */
    uint32_t    BMC8140_FAN_TACH_10;        /* Tachometer readings for dual rotors: FAN_10 */
    uint32_t    BMC8140_FAN_TACH_11;        /* Tachometer readings for dual rotors: FAN_11 */
    uint32_t    BMC8140_FAN_TARGET_0;       /* Target RPM for FAN_0 */
    uint32_t    BMC8140_FAN_TARGET_1;       /* Target RPM for FAN_1 */
    uint32_t    BMC8140_FAN_TARGET_2;       /* Target RPM for FAN_2 */
    uint32_t    BMC8140_FAN_TARGET_3;       /* Target RPM for FAN_3 */
    uint32_t    BMC8140_FAN_TARGET_4;       /* Target RPM for FAN_4 */
    uint32_t    BMC8140_FAN_TARGET_5;       /* Target RPM for FAN_5 */
    uint32_t    BMC8140_FAN_TARGET_6;       /* Target RPM for FAN_6 */
    uint32_t    BMC8140_FAN_TARGET_7;       /* Target RPM for FAN_7 */
    uint32_t    BMC8140_FAN_TARGET_8;       /* Target RPM for FAN_8 */
    uint32_t    BMC8140_FAN_TARGET_9;       /* Target RPM for FAN_9 */
    uint32_t    BMC8140_FAN_TARGET_10;      /* Target RPM for FAN_10 */
    uint32_t    BMC8140_FAN_TARGET_11;      /* Target RPM for FAN_11 */
    uint32_t    BMC8140_FAN_THRESH_0;       /* FAN_0 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_1;       /* FAN_1 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_2;       /* FAN_2 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_3;       /* FAN_3 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_4;       /* FAN_4 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_5;       /* FAN_5 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_6;       /* FAN_6 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_7;       /* FAN_7 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_8;       /* FAN_8 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_9;       /* FAN_9 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_10;      /* FAN_10 maximum/minimum RPM thresholds */
    uint32_t    BMC8140_FAN_THRESH_11;      /* FAN_11 maximum/minimum RPM thresholds */
    uint8_t pad7[832];

    uint32_t    BMC8140_RESERVED_THESTART;  /* Reserved */
    uint8_t pad8[2040];

    uint32_t    BMC8140_RESERVED_THEEND;    /* Reserved */
    uint8_t pad9[10240];

    uint32_t    BMC8140_FAN_FRU_IDP_0;      /* Fan FRU 0 IDP data (256 bytes) */
    uint8_t pad10[252];

    uint32_t    BMC8140_FAN_FRU_IDP_1;      /* Fan FRU 1 IDP data (256 bytes) */
    uint8_t pad11[252];

    uint32_t    BMC8140_FAN_FRU_IDP_2;      /* Fan FRU 2 IDP data (256 bytes) */
    uint8_t pad12[252];

    uint32_t    BMC8140_FAN_FRU_IDP_3;      /* Fan FRU 3 IDP data (256 bytes) */
    uint8_t pad13[252];

    uint32_t    BMC8140_FAN_FRU_IDP_4;      /* Fan FRU 4 IDP data (256 bytes) */
    uint8_t pad14[252];

    uint32_t    BMC8140_FAN_FRU_IDP_5;      /* Fan FRU 5 IDP data (256 bytes) */
    uint8_t pad15[252];

    uint32_t    BMC8140_FAN_FRU_IDP_THEEND; /* Reserved */
} __attribute__ ((__packed__, __aligned__(4)));

#define BMC8140_REG_PTR(base, reg)           (&(base)->reg)
#define BMC8140_REG_OFFSET(reg)              offsetof(struct Bmc8140_dev_reg, reg)
#define BMC8140_REG_INDEX(reg)               (BMC8140_REG_OFFSET(reg)/sizeof(uint32_t))
#define BMC8140_REG_WIDTH(reg)               BMC8140_JOIN(reg,, _WIDTH)
#define BMC8140_REG_TYPE(reg)                BMC8140_JOIN(reg,, _TYPE)
#define BMC8140_REG_VALUE(reg, val)          BMC8140_JOIN(reg,, val)
#define BMC8140_FIELD_MASK(reg, field)       BMC8140_JOIN(reg, field, _MASK)
#define BMC8140_FIELD_SHIFT(reg, field)      BMC8140_JOIN(reg, field, _SHIFT)
#define BMC8140_FIELD_VALUE(reg, field, val) BMC8140_JOIN(reg, field, val)
#define BMC8140_JOIN(reg, field, suffix) reg ## _ ## field ## __ ## suffix

#define BMC8140_GET_BITFIELD(regval, mask, shift) \
    ( ((regval)&(mask)) >> (shift) )

#define BMC8140_SET_BITFIELD(regval, mask, shift, value) \
    ( ((regval) & ~(mask)) | (((value)<<(shift)) & (mask)) )


/* ---- BMC8140_IDP_DATA ---- */
#define BMC8140_IDP_DATA____WIDTH	32
#define BMC8140_IDP_DATA____TYPE 	uint32_t

#define BMC8140_IDP_DATA____REGMASK	UINT32_C(0)

/* ---- BMC8140_IDP_THEEND ---- */
#define BMC8140_IDP_THEEND____WIDTH	32
#define BMC8140_IDP_THEEND____TYPE 	uint32_t

#define BMC8140_IDP_THEEND____REGMASK	UINT32_C(0)

/* ---- BMC8140_BASE_FID ---- */
#define BMC8140_BASE_FID____WIDTH	32
#define BMC8140_BASE_FID____TYPE 	uint32_t

#define BMC8140_BASE_FID_Unused_16___MASK 	UINT32_C(0xffff0000)
#define BMC8140_BASE_FID_Unused_16___SHIFT	16
#define BMC8140_BASE_FID_PID___MASK       	UINT32_C(0xff00)
#define BMC8140_BASE_FID_PID___SHIFT      	8
#define BMC8140_BASE_FID_DID___MASK       	UINT32_C(0xff)
#define BMC8140_BASE_FID_DID___SHIFT      	0
#define BMC8140_BASE_FID____REGMASK	UINT32_C(65535)

/* ---- BMC8140_BASE_MJR ---- */
#define BMC8140_BASE_MJR____WIDTH	32
#define BMC8140_BASE_MJR____TYPE 	uint32_t

#define BMC8140_BASE_MJR____REGMASK	UINT32_C(0)

/* ---- BMC8140_BASE_MNR ---- */
#define BMC8140_BASE_MNR____WIDTH	32
#define BMC8140_BASE_MNR____TYPE 	uint32_t

#define BMC8140_BASE_MNR____REGMASK	UINT32_C(0)

/* ---- BMC8140_BASE_BLD ---- */
#define BMC8140_BASE_BLD____WIDTH	32
#define BMC8140_BASE_BLD____TYPE 	uint32_t

#define BMC8140_BASE_BLD____REGMASK	UINT32_C(0)

/* ---- BMC8140_BASE_SCRATCHPAD ---- */
#define BMC8140_BASE_SCRATCHPAD____WIDTH	32
#define BMC8140_BASE_SCRATCHPAD____TYPE 	uint32_t

#define BMC8140_BASE_SCRATCHPAD____REGMASK	UINT32_C(0)

/* ---- BMC8140_BASE_BMC_DATE ---- */
#define BMC8140_BASE_BMC_DATE____WIDTH	32
#define BMC8140_BASE_BMC_DATE____TYPE 	uint32_t

#define BMC8140_BASE_BMC_DATE_build_year___MASK  	UINT32_C(0xffff0000)
#define BMC8140_BASE_BMC_DATE_build_year___SHIFT 	16
#define BMC8140_BASE_BMC_DATE_build_month___MASK 	UINT32_C(0xff00)
#define BMC8140_BASE_BMC_DATE_build_month___SHIFT	8
#define BMC8140_BASE_BMC_DATE_build_day___MASK   	UINT32_C(0xff)
#define BMC8140_BASE_BMC_DATE_build_day___SHIFT  	0
#define BMC8140_BASE_BMC_DATE____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_0 ---- */
#define BMC8140_BASE_RESET_HISTORY_0____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_0____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_0_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_0_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_0_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_0_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_0_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_0_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_0_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_0_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_0_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_0_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_0_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_0_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_0_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_0_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_0 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_0____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_0____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_0_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_0_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_1 ---- */
#define BMC8140_BASE_RESET_HISTORY_1____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_1____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_1_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_1_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_1_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_1_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_1_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_1_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_1_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_1_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_1_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_1_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_1_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_1_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_1_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_1_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_1 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_1____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_1____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_1_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_1_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_2 ---- */
#define BMC8140_BASE_RESET_HISTORY_2____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_2____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_2_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_2_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_2_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_2_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_2_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_2_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_2_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_2_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_2_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_2_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_2_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_2_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_2_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_2_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_2 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_2____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_2____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_2_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_2_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_3 ---- */
#define BMC8140_BASE_RESET_HISTORY_3____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_3____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_3_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_3_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_3_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_3_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_3_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_3_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_3_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_3_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_3_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_3_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_3_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_3_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_3_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_3_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_3 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_3____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_3____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_3_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_3_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_4 ---- */
#define BMC8140_BASE_RESET_HISTORY_4____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_4____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_4_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_4_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_4_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_4_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_4_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_4_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_4_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_4_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_4_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_4_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_4_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_4_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_4_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_4_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_4 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_4____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_4____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_4_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_4_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_5 ---- */
#define BMC8140_BASE_RESET_HISTORY_5____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_5____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_5_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_5_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_5_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_5_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_5_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_5_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_5_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_5_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_5_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_5_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_5_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_5_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_5_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_5_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_5 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_5____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_5____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_5_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_5_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_6 ---- */
#define BMC8140_BASE_RESET_HISTORY_6____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_6____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_6_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_6_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_6_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_6_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_6_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_6_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_6_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_6_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_6_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_6_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_6_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_6_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_6_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_6_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_6 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_6____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_6____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_6_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_6_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_7 ---- */
#define BMC8140_BASE_RESET_HISTORY_7____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_7____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_7_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_7_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_7_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_7_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_7_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_7_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_7_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_7_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_7_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_7_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_7_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_7_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_7_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_7_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_7 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_7____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_7____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_7_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_7_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_8 ---- */
#define BMC8140_BASE_RESET_HISTORY_8____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_8____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_8_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_8_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_8_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_8_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_8_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_8_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_8_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_8_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_8_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_8_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_8_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_8_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_8_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_8_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_8 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_8____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_8____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_8_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_8_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_9 ---- */
#define BMC8140_BASE_RESET_HISTORY_9____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_9____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_9_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_9_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_9_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_9_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_9_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_9_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_9_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_9_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_9_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_9_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_9_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_9_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_9_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_9_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_9 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_9____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_9____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_9_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_9_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_10 ---- */
#define BMC8140_BASE_RESET_HISTORY_10____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_10____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_10_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_10_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_10_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_10_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_10_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_10_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_10_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_10_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_10_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_10_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_10_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_10_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_10_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_10_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_10 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_10____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_10____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_10_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_10_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_11 ---- */
#define BMC8140_BASE_RESET_HISTORY_11____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_11____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_11_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_11_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_11_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_11_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_11_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_11_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_11_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_11_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_11_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_11_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_11_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_11_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_11_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_11_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_11 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_11____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_11____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_11_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_11_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_12 ---- */
#define BMC8140_BASE_RESET_HISTORY_12____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_12____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_12_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_12_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_12_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_12_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_12_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_12_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_12_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_12_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_12_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_12_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_12_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_12_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_12_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_12_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_12____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_12 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_12____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_12____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_12_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_12_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_12____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_13 ---- */
#define BMC8140_BASE_RESET_HISTORY_13____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_13____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_13_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_13_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_13_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_13_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_13_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_13_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_13_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_13_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_13_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_13_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_13_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_13_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_13_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_13_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_13____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_13 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_13____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_13____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_13_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_13_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_13____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_14 ---- */
#define BMC8140_BASE_RESET_HISTORY_14____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_14____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_14_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_14_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_14_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_14_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_14_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_14_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_14_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_14_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_14_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_14_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_14_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_14_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_14_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_14_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_14____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_14 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_14____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_14____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_14_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_14_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_14____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_15 ---- */
#define BMC8140_BASE_RESET_HISTORY_15____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_15____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_15_TIMESTAMPL___MASK              	UINT32_C(0xffff0000)
#define BMC8140_BASE_RESET_HISTORY_15_TIMESTAMPL___SHIFT             	16
#define BMC8140_BASE_RESET_HISTORY_15_RSVD0___MASK                   	UINT32_C(0xc000)
#define BMC8140_BASE_RESET_HISTORY_15_RSVD0___SHIFT                  	14
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_REBOOT___MASK        	UINT32_C(0x2000)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_REBOOT___SHIFT       	13
#define BMC8140_BASE_RESET_HISTORY_15_RESET_BMC_WDT_EXPIRY___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_BMC_WDT_EXPIRY___SHIFT   	12
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_COLD___MASK      	UINT32_C(0x800)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_COLD___SHIFT     	11
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_HOT___MASK       	UINT32_C(0x400)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_HOT___SHIFT      	10
#define BMC8140_BASE_RESET_HISTORY_15_RESET_RUDRA_A5A5___MASK        	UINT32_C(0x200)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_RUDRA_A5A5___SHIFT       	9
#define BMC8140_BASE_RESET_HISTORY_15_RESET_SUTRA_A5A5___MASK        	UINT32_C(0x100)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_SUTRA_A5A5___SHIFT       	8
#define BMC8140_BASE_RESET_HISTORY_15_RESET_RUDRA_5A5A___MASK        	UINT32_C(0x80)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_RUDRA_5A5A___SHIFT       	7
#define BMC8140_BASE_RESET_HISTORY_15_RESET_SUTRA_5A5A___MASK        	UINT32_C(0x40)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_SUTRA_5A5A___SHIFT       	6
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_ERROR2___MASK        	UINT32_C(0x20)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_ERROR2___SHIFT       	5
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_CATERR___MASK        	UINT32_C(0x10)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_CATERR___SHIFT       	4
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_MSMI___MASK          	UINT32_C(0x8)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_CPU_MSMI___SHIFT         	3
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_THERMTRIP___MASK 	UINT32_C(0x4)
#define BMC8140_BASE_RESET_HISTORY_15_RESET_THERMAL_THERMTRIP___SHIFT	2
#define BMC8140_BASE_RESET_HISTORY_15_BUTTON_SHORT_PRESS___MASK      	UINT32_C(0x2)
#define BMC8140_BASE_RESET_HISTORY_15_BUTTON_SHORT_PRESS___SHIFT     	1
#define BMC8140_BASE_RESET_HISTORY_15_BUTTON_LONG_PRESS___MASK       	UINT32_C(0x1)
#define BMC8140_BASE_RESET_HISTORY_15_BUTTON_LONG_PRESS___SHIFT      	0
#define BMC8140_BASE_RESET_HISTORY_15____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_RESET_HISTORY_TS_UPR_15 ---- */
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_15____WIDTH	32
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_15____TYPE 	uint32_t

#define BMC8140_BASE_RESET_HISTORY_TS_UPR_15_TIMESTAMPU___MASK 	UINT32_C(0xffffffff)
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_15_TIMESTAMPU___SHIFT	0
#define BMC8140_BASE_RESET_HISTORY_TS_UPR_15____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_BASE_MSI_CTRL ---- */
#define BMC8140_BASE_MSI_CTRL____WIDTH	32
#define BMC8140_BASE_MSI_CTRL____TYPE 	uint32_t

#define BMC8140_BASE_MSI_CTRL_Unused_5___MASK     	UINT32_C(0xffffffe0)
#define BMC8140_BASE_MSI_CTRL_Unused_5___SHIFT    	5
#define BMC8140_BASE_MSI_CTRL_MSI_INTERVAL___MASK 	UINT32_C(0x1f)
#define BMC8140_BASE_MSI_CTRL_MSI_INTERVAL___SHIFT	0
#define BMC8140_BASE_MSI_CTRL____REGMASK	UINT32_C(31)

/* ---- BMC8140_BASE_ISR_MASTER_EVENT ---- */
#define BMC8140_BASE_ISR_MASTER_EVENT____WIDTH	32
#define BMC8140_BASE_ISR_MASTER_EVENT____TYPE 	uint32_t

#define BMC8140_BASE_ISR_MASTER_EVENT_Unused_3___MASK 	UINT32_C(0xfffffff8)
#define BMC8140_BASE_ISR_MASTER_EVENT_Unused_3___SHIFT	3
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_WDT___MASK  	UINT32_C(0x4)
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_WDT___SHIFT 	2
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_FANS___MASK 	UINT32_C(0x2)
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_FANS___SHIFT	1
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_TEMP___MASK 	UINT32_C(0x1)
#define BMC8140_BASE_ISR_MASTER_EVENT_ANY_TEMP___SHIFT	0
#define BMC8140_BASE_ISR_MASTER_EVENT____REGMASK	UINT32_C(7)

/* ---- BMC8140_BASE_ISM_MASTER_EVENT ---- */
#define BMC8140_BASE_ISM_MASTER_EVENT____WIDTH	32
#define BMC8140_BASE_ISM_MASTER_EVENT____TYPE 	uint32_t

#define BMC8140_BASE_ISM_MASTER_EVENT_Unused_3___MASK 	UINT32_C(0xfffffff8)
#define BMC8140_BASE_ISM_MASTER_EVENT_Unused_3___SHIFT	3
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_WDT___MASK  	UINT32_C(0x4)
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_WDT___SHIFT 	2
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_FANS___MASK 	UINT32_C(0x2)
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_FANS___SHIFT	1
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_TEMP___MASK 	UINT32_C(0x1)
#define BMC8140_BASE_ISM_MASTER_EVENT_ANY_TEMP___SHIFT	0
#define BMC8140_BASE_ISM_MASTER_EVENT____REGMASK	UINT32_C(7)

/* ---- BMC8140_BASE_ISR_TEMP ---- */
#define BMC8140_BASE_ISR_TEMP____WIDTH	32
#define BMC8140_BASE_ISR_TEMP____TYPE 	uint32_t

#define BMC8140_BASE_ISR_TEMP_SYS_TEMP_WITHIN_OTR___MASK  	UINT32_C(0x80000000)
#define BMC8140_BASE_ISR_TEMP_SYS_TEMP_WITHIN_OTR___SHIFT 	31
#define BMC8140_BASE_ISR_TEMP_Unused_20___MASK            	UINT32_C(0x7ff00000)
#define BMC8140_BASE_ISR_TEMP_Unused_20___SHIFT           	20
#define BMC8140_BASE_ISR_TEMP_tmp8_sensor_error___MASK    	UINT32_C(0x80000)
#define BMC8140_BASE_ISR_TEMP_tmp8_sensor_error___SHIFT   	19
#define BMC8140_BASE_ISR_TEMP_tmp8_sensor_fault___MASK    	UINT32_C(0x40000)
#define BMC8140_BASE_ISR_TEMP_tmp8_sensor_fault___SHIFT   	18
#define BMC8140_BASE_ISR_TEMP_tmp7_sensor_error___MASK    	UINT32_C(0x20000)
#define BMC8140_BASE_ISR_TEMP_tmp7_sensor_error___SHIFT   	17
#define BMC8140_BASE_ISR_TEMP_tmp7_sensor_fault___MASK    	UINT32_C(0x10000)
#define BMC8140_BASE_ISR_TEMP_tmp7_sensor_fault___SHIFT   	16
#define BMC8140_BASE_ISR_TEMP_tmp6_sensor_error___MASK    	UINT32_C(0x8000)
#define BMC8140_BASE_ISR_TEMP_tmp6_sensor_error___SHIFT   	15
#define BMC8140_BASE_ISR_TEMP_tmp6_sensor_fault___MASK    	UINT32_C(0x4000)
#define BMC8140_BASE_ISR_TEMP_tmp6_sensor_fault___SHIFT   	14
#define BMC8140_BASE_ISR_TEMP_tmp5_sensor_error___MASK    	UINT32_C(0x2000)
#define BMC8140_BASE_ISR_TEMP_tmp5_sensor_error___SHIFT   	13
#define BMC8140_BASE_ISR_TEMP_tmp5_sensor_fault___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_ISR_TEMP_tmp5_sensor_fault___SHIFT   	12
#define BMC8140_BASE_ISR_TEMP_tmp4_sensor_error___MASK    	UINT32_C(0x800)
#define BMC8140_BASE_ISR_TEMP_tmp4_sensor_error___SHIFT   	11
#define BMC8140_BASE_ISR_TEMP_tmp4_sensor_fault___MASK    	UINT32_C(0x400)
#define BMC8140_BASE_ISR_TEMP_tmp4_sensor_fault___SHIFT   	10
#define BMC8140_BASE_ISR_TEMP_tmp3_sensor_error___MASK    	UINT32_C(0x200)
#define BMC8140_BASE_ISR_TEMP_tmp3_sensor_error___SHIFT   	9
#define BMC8140_BASE_ISR_TEMP_tmp3_sensor_fault___MASK    	UINT32_C(0x100)
#define BMC8140_BASE_ISR_TEMP_tmp3_sensor_fault___SHIFT   	8
#define BMC8140_BASE_ISR_TEMP_tmp2_sensor_error___MASK    	UINT32_C(0x80)
#define BMC8140_BASE_ISR_TEMP_tmp2_sensor_error___SHIFT   	7
#define BMC8140_BASE_ISR_TEMP_tmp2_sensor_fault___MASK    	UINT32_C(0x40)
#define BMC8140_BASE_ISR_TEMP_tmp2_sensor_fault___SHIFT   	6
#define BMC8140_BASE_ISR_TEMP_tmp1_sensor_error___MASK    	UINT32_C(0x20)
#define BMC8140_BASE_ISR_TEMP_tmp1_sensor_error___SHIFT   	5
#define BMC8140_BASE_ISR_TEMP_tmp1_sensor_fault___MASK    	UINT32_C(0x10)
#define BMC8140_BASE_ISR_TEMP_tmp1_sensor_fault___SHIFT   	4
#define BMC8140_BASE_ISR_TEMP_tmp0_sensor_error___MASK    	UINT32_C(0x8)
#define BMC8140_BASE_ISR_TEMP_tmp0_sensor_error___SHIFT   	3
#define BMC8140_BASE_ISR_TEMP_tmp0_sensor_fault___MASK    	UINT32_C(0x4)
#define BMC8140_BASE_ISR_TEMP_tmp0_sensor_fault___SHIFT   	2
#define BMC8140_BASE_ISR_TEMP_under_temp_threshold___MASK 	UINT32_C(0x2)
#define BMC8140_BASE_ISR_TEMP_under_temp_threshold___SHIFT	1
#define BMC8140_BASE_ISR_TEMP_over_temp_threshold___MASK  	UINT32_C(0x1)
#define BMC8140_BASE_ISR_TEMP_over_temp_threshold___SHIFT 	0
#define BMC8140_BASE_ISR_TEMP____REGMASK	UINT32_C(2148532223)

/* ---- BMC8140_BASE_ISM_TEMP ---- */
#define BMC8140_BASE_ISM_TEMP____WIDTH	32
#define BMC8140_BASE_ISM_TEMP____TYPE 	uint32_t

#define BMC8140_BASE_ISM_TEMP_SYS_TEMP_WITHIN_OTR___MASK  	UINT32_C(0x80000000)
#define BMC8140_BASE_ISM_TEMP_SYS_TEMP_WITHIN_OTR___SHIFT 	31
#define BMC8140_BASE_ISM_TEMP_Unused_20___MASK            	UINT32_C(0x7ff00000)
#define BMC8140_BASE_ISM_TEMP_Unused_20___SHIFT           	20
#define BMC8140_BASE_ISM_TEMP_tmp8_sensor_error___MASK    	UINT32_C(0x80000)
#define BMC8140_BASE_ISM_TEMP_tmp8_sensor_error___SHIFT   	19
#define BMC8140_BASE_ISM_TEMP_tmp8_sensor_fault___MASK    	UINT32_C(0x40000)
#define BMC8140_BASE_ISM_TEMP_tmp8_sensor_fault___SHIFT   	18
#define BMC8140_BASE_ISM_TEMP_tmp7_sensor_error___MASK    	UINT32_C(0x20000)
#define BMC8140_BASE_ISM_TEMP_tmp7_sensor_error___SHIFT   	17
#define BMC8140_BASE_ISM_TEMP_tmp7_sensor_fault___MASK    	UINT32_C(0x10000)
#define BMC8140_BASE_ISM_TEMP_tmp7_sensor_fault___SHIFT   	16
#define BMC8140_BASE_ISM_TEMP_tmp6_sensor_error___MASK    	UINT32_C(0x8000)
#define BMC8140_BASE_ISM_TEMP_tmp6_sensor_error___SHIFT   	15
#define BMC8140_BASE_ISM_TEMP_tmp6_sensor_fault___MASK    	UINT32_C(0x4000)
#define BMC8140_BASE_ISM_TEMP_tmp6_sensor_fault___SHIFT   	14
#define BMC8140_BASE_ISM_TEMP_tmp5_sensor_error___MASK    	UINT32_C(0x2000)
#define BMC8140_BASE_ISM_TEMP_tmp5_sensor_error___SHIFT   	13
#define BMC8140_BASE_ISM_TEMP_tmp5_sensor_fault___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_ISM_TEMP_tmp5_sensor_fault___SHIFT   	12
#define BMC8140_BASE_ISM_TEMP_tmp4_sensor_error___MASK    	UINT32_C(0x800)
#define BMC8140_BASE_ISM_TEMP_tmp4_sensor_error___SHIFT   	11
#define BMC8140_BASE_ISM_TEMP_tmp4_sensor_fault___MASK    	UINT32_C(0x400)
#define BMC8140_BASE_ISM_TEMP_tmp4_sensor_fault___SHIFT   	10
#define BMC8140_BASE_ISM_TEMP_tmp3_sensor_error___MASK    	UINT32_C(0x200)
#define BMC8140_BASE_ISM_TEMP_tmp3_sensor_error___SHIFT   	9
#define BMC8140_BASE_ISM_TEMP_tmp3_sensor_fault___MASK    	UINT32_C(0x100)
#define BMC8140_BASE_ISM_TEMP_tmp3_sensor_fault___SHIFT   	8
#define BMC8140_BASE_ISM_TEMP_tmp2_sensor_error___MASK    	UINT32_C(0x80)
#define BMC8140_BASE_ISM_TEMP_tmp2_sensor_error___SHIFT   	7
#define BMC8140_BASE_ISM_TEMP_tmp2_sensor_fault___MASK    	UINT32_C(0x40)
#define BMC8140_BASE_ISM_TEMP_tmp2_sensor_fault___SHIFT   	6
#define BMC8140_BASE_ISM_TEMP_tmp1_sensor_error___MASK    	UINT32_C(0x20)
#define BMC8140_BASE_ISM_TEMP_tmp1_sensor_error___SHIFT   	5
#define BMC8140_BASE_ISM_TEMP_tmp1_sensor_fault___MASK    	UINT32_C(0x10)
#define BMC8140_BASE_ISM_TEMP_tmp1_sensor_fault___SHIFT   	4
#define BMC8140_BASE_ISM_TEMP_tmp0_sensor_error___MASK    	UINT32_C(0x8)
#define BMC8140_BASE_ISM_TEMP_tmp0_sensor_error___SHIFT   	3
#define BMC8140_BASE_ISM_TEMP_tmp0_sensor_fault___MASK    	UINT32_C(0x4)
#define BMC8140_BASE_ISM_TEMP_tmp0_sensor_fault___SHIFT   	2
#define BMC8140_BASE_ISM_TEMP_under_temp_threshold___MASK 	UINT32_C(0x2)
#define BMC8140_BASE_ISM_TEMP_under_temp_threshold___SHIFT	1
#define BMC8140_BASE_ISM_TEMP_over_temp_threshold___MASK  	UINT32_C(0x1)
#define BMC8140_BASE_ISM_TEMP_over_temp_threshold___SHIFT 	0
#define BMC8140_BASE_ISM_TEMP____REGMASK	UINT32_C(2148532223)

/* ---- BMC8140_BASE_STATUS_TEMP ---- */
#define BMC8140_BASE_STATUS_TEMP____WIDTH	32
#define BMC8140_BASE_STATUS_TEMP____TYPE 	uint32_t

#define BMC8140_BASE_STATUS_TEMP_SYS_TEMP_WITHIN_OTR___MASK  	UINT32_C(0x80000000)
#define BMC8140_BASE_STATUS_TEMP_SYS_TEMP_WITHIN_OTR___SHIFT 	31
#define BMC8140_BASE_STATUS_TEMP_Unused_20___MASK            	UINT32_C(0x7ff00000)
#define BMC8140_BASE_STATUS_TEMP_Unused_20___SHIFT           	20
#define BMC8140_BASE_STATUS_TEMP_tmp8_sensor_error___MASK    	UINT32_C(0x80000)
#define BMC8140_BASE_STATUS_TEMP_tmp8_sensor_error___SHIFT   	19
#define BMC8140_BASE_STATUS_TEMP_tmp8_sensor_fault___MASK    	UINT32_C(0x40000)
#define BMC8140_BASE_STATUS_TEMP_tmp8_sensor_fault___SHIFT   	18
#define BMC8140_BASE_STATUS_TEMP_tmp7_sensor_error___MASK    	UINT32_C(0x20000)
#define BMC8140_BASE_STATUS_TEMP_tmp7_sensor_error___SHIFT   	17
#define BMC8140_BASE_STATUS_TEMP_tmp7_sensor_fault___MASK    	UINT32_C(0x10000)
#define BMC8140_BASE_STATUS_TEMP_tmp7_sensor_fault___SHIFT   	16
#define BMC8140_BASE_STATUS_TEMP_tmp6_sensor_error___MASK    	UINT32_C(0x8000)
#define BMC8140_BASE_STATUS_TEMP_tmp6_sensor_error___SHIFT   	15
#define BMC8140_BASE_STATUS_TEMP_tmp6_sensor_fault___MASK    	UINT32_C(0x4000)
#define BMC8140_BASE_STATUS_TEMP_tmp6_sensor_fault___SHIFT   	14
#define BMC8140_BASE_STATUS_TEMP_tmp5_sensor_error___MASK    	UINT32_C(0x2000)
#define BMC8140_BASE_STATUS_TEMP_tmp5_sensor_error___SHIFT   	13
#define BMC8140_BASE_STATUS_TEMP_tmp5_sensor_fault___MASK    	UINT32_C(0x1000)
#define BMC8140_BASE_STATUS_TEMP_tmp5_sensor_fault___SHIFT   	12
#define BMC8140_BASE_STATUS_TEMP_tmp4_sensor_error___MASK    	UINT32_C(0x800)
#define BMC8140_BASE_STATUS_TEMP_tmp4_sensor_error___SHIFT   	11
#define BMC8140_BASE_STATUS_TEMP_tmp4_sensor_fault___MASK    	UINT32_C(0x400)
#define BMC8140_BASE_STATUS_TEMP_tmp4_sensor_fault___SHIFT   	10
#define BMC8140_BASE_STATUS_TEMP_tmp3_sensor_error___MASK    	UINT32_C(0x200)
#define BMC8140_BASE_STATUS_TEMP_tmp3_sensor_error___SHIFT   	9
#define BMC8140_BASE_STATUS_TEMP_tmp3_sensor_fault___MASK    	UINT32_C(0x100)
#define BMC8140_BASE_STATUS_TEMP_tmp3_sensor_fault___SHIFT   	8
#define BMC8140_BASE_STATUS_TEMP_tmp2_sensor_error___MASK    	UINT32_C(0x80)
#define BMC8140_BASE_STATUS_TEMP_tmp2_sensor_error___SHIFT   	7
#define BMC8140_BASE_STATUS_TEMP_tmp2_sensor_fault___MASK    	UINT32_C(0x40)
#define BMC8140_BASE_STATUS_TEMP_tmp2_sensor_fault___SHIFT   	6
#define BMC8140_BASE_STATUS_TEMP_tmp1_sensor_error___MASK    	UINT32_C(0x20)
#define BMC8140_BASE_STATUS_TEMP_tmp1_sensor_error___SHIFT   	5
#define BMC8140_BASE_STATUS_TEMP_tmp1_sensor_fault___MASK    	UINT32_C(0x10)
#define BMC8140_BASE_STATUS_TEMP_tmp1_sensor_fault___SHIFT   	4
#define BMC8140_BASE_STATUS_TEMP_tmp0_sensor_error___MASK    	UINT32_C(0x8)
#define BMC8140_BASE_STATUS_TEMP_tmp0_sensor_error___SHIFT   	3
#define BMC8140_BASE_STATUS_TEMP_tmp0_sensor_fault___MASK    	UINT32_C(0x4)
#define BMC8140_BASE_STATUS_TEMP_tmp0_sensor_fault___SHIFT   	2
#define BMC8140_BASE_STATUS_TEMP_under_temp_threshold___MASK 	UINT32_C(0x2)
#define BMC8140_BASE_STATUS_TEMP_under_temp_threshold___SHIFT	1
#define BMC8140_BASE_STATUS_TEMP_over_temp_threshold___MASK  	UINT32_C(0x1)
#define BMC8140_BASE_STATUS_TEMP_over_temp_threshold___SHIFT 	0
#define BMC8140_BASE_STATUS_TEMP____REGMASK	UINT32_C(2148532223)

/* ---- BMC8140_BASE_ISR_FAN ---- */
#define BMC8140_BASE_ISR_FAN____WIDTH	32
#define BMC8140_BASE_ISR_FAN____TYPE 	uint32_t

#define BMC8140_BASE_ISR_FAN_Unused_30___MASK     	UINT32_C(0xc0000000)
#define BMC8140_BASE_ISR_FAN_Unused_30___SHIFT    	30
#define BMC8140_BASE_ISR_FAN_fan_present___MASK   	UINT32_C(0x3f000000)
#define BMC8140_BASE_ISR_FAN_fan_present___SHIFT  	24
#define BMC8140_BASE_ISR_FAN_fan_powergood___MASK 	UINT32_C(0xfff000)
#define BMC8140_BASE_ISR_FAN_fan_powergood___SHIFT	12
#define BMC8140_BASE_ISR_FAN_fan_failure___MASK   	UINT32_C(0xfff)
#define BMC8140_BASE_ISR_FAN_fan_failure___SHIFT  	0
#define BMC8140_BASE_ISR_FAN____REGMASK	UINT32_C(1073741823)

/* ---- BMC8140_BASE_ISM_FAN ---- */
#define BMC8140_BASE_ISM_FAN____WIDTH	32
#define BMC8140_BASE_ISM_FAN____TYPE 	uint32_t

#define BMC8140_BASE_ISM_FAN_Unused_30___MASK     	UINT32_C(0xc0000000)
#define BMC8140_BASE_ISM_FAN_Unused_30___SHIFT    	30
#define BMC8140_BASE_ISM_FAN_fan_present___MASK   	UINT32_C(0x3f000000)
#define BMC8140_BASE_ISM_FAN_fan_present___SHIFT  	24
#define BMC8140_BASE_ISM_FAN_fan_powergood___MASK 	UINT32_C(0xfff000)
#define BMC8140_BASE_ISM_FAN_fan_powergood___SHIFT	12
#define BMC8140_BASE_ISM_FAN_fan_failure___MASK   	UINT32_C(0xfff)
#define BMC8140_BASE_ISM_FAN_fan_failure___SHIFT  	0
#define BMC8140_BASE_ISM_FAN____REGMASK	UINT32_C(1073741823)

/* ---- BMC8140_BASE_STATUS_FAN ---- */
#define BMC8140_BASE_STATUS_FAN____WIDTH	32
#define BMC8140_BASE_STATUS_FAN____TYPE 	uint32_t

#define BMC8140_BASE_STATUS_FAN_Unused_30___MASK     	UINT32_C(0xc0000000)
#define BMC8140_BASE_STATUS_FAN_Unused_30___SHIFT    	30
#define BMC8140_BASE_STATUS_FAN_fan_present___MASK   	UINT32_C(0x3f000000)
#define BMC8140_BASE_STATUS_FAN_fan_present___SHIFT  	24
#define BMC8140_BASE_STATUS_FAN_fan_powergood___MASK 	UINT32_C(0xfff000)
#define BMC8140_BASE_STATUS_FAN_fan_powergood___SHIFT	12
#define BMC8140_BASE_STATUS_FAN_fan_failure___MASK   	UINT32_C(0xfff)
#define BMC8140_BASE_STATUS_FAN_fan_failure___SHIFT  	0
#define BMC8140_BASE_STATUS_FAN____REGMASK	UINT32_C(1073741823)

/* ---- BMC8140_BASE_ISR_WDT ---- */
#define BMC8140_BASE_ISR_WDT____WIDTH	32
#define BMC8140_BASE_ISR_WDT____TYPE 	uint32_t

#define BMC8140_BASE_ISR_WDT_Unused_1___MASK         	UINT32_C(0xfffffffe)
#define BMC8140_BASE_ISR_WDT_Unused_1___SHIFT        	1
#define BMC8140_BASE_ISR_WDT_host_wdt_timeout___MASK 	UINT32_C(0x1)
#define BMC8140_BASE_ISR_WDT_host_wdt_timeout___SHIFT	0
#define BMC8140_BASE_ISR_WDT____REGMASK	UINT32_C(1)

/* ---- BMC8140_BASE_ISM_WDT ---- */
#define BMC8140_BASE_ISM_WDT____WIDTH	32
#define BMC8140_BASE_ISM_WDT____TYPE 	uint32_t

#define BMC8140_BASE_ISM_WDT_Unused_1___MASK         	UINT32_C(0xfffffffe)
#define BMC8140_BASE_ISM_WDT_Unused_1___SHIFT        	1
#define BMC8140_BASE_ISM_WDT_host_wdt_timeout___MASK 	UINT32_C(0x1)
#define BMC8140_BASE_ISM_WDT_host_wdt_timeout___SHIFT	0
#define BMC8140_BASE_ISM_WDT____REGMASK	UINT32_C(1)

/* ---- BMC8140_BASE_STATUS_WDT ---- */
#define BMC8140_BASE_STATUS_WDT____WIDTH	32
#define BMC8140_BASE_STATUS_WDT____TYPE 	uint32_t

#define BMC8140_BASE_STATUS_WDT_Unused_1___MASK         	UINT32_C(0xfffffffe)
#define BMC8140_BASE_STATUS_WDT_Unused_1___SHIFT        	1
#define BMC8140_BASE_STATUS_WDT_host_wdt_timeout___MASK 	UINT32_C(0x1)
#define BMC8140_BASE_STATUS_WDT_host_wdt_timeout___SHIFT	0
#define BMC8140_BASE_STATUS_WDT____REGMASK	UINT32_C(1)

/* ---- BMC8140_ADC_0 ---- */
#define BMC8140_ADC_0____WIDTH	32
#define BMC8140_ADC_0____TYPE 	uint32_t

#define BMC8140_ADC_0_Unused_17___MASK    	UINT32_C(0xfffe0000)
#define BMC8140_ADC_0_Unused_17___SHIFT   	17
#define BMC8140_ADC_0_present___MASK      	UINT32_C(0x10000)
#define BMC8140_ADC_0_present___SHIFT     	16
#define BMC8140_ADC_0_P1V2_VDDQ_AB___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_0_P1V2_VDDQ_AB___SHIFT	0
#define BMC8140_ADC_0____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_1 ---- */
#define BMC8140_ADC_1____WIDTH	32
#define BMC8140_ADC_1____TYPE 	uint32_t

#define BMC8140_ADC_1_Unused_17___MASK       	UINT32_C(0xfffe0000)
#define BMC8140_ADC_1_Unused_17___SHIFT      	17
#define BMC8140_ADC_1_present___MASK         	UINT32_C(0x10000)
#define BMC8140_ADC_1_present___SHIFT        	16
#define BMC8140_ADC_1_0_9V_PCIEX_DPOL___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_1_0_9V_PCIEX_DPOL___SHIFT	0
#define BMC8140_ADC_1____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_2 ---- */
#define BMC8140_ADC_2____WIDTH	32
#define BMC8140_ADC_2____TYPE 	uint32_t

#define BMC8140_ADC_2_Unused_17___MASK   	UINT32_C(0xfffe0000)
#define BMC8140_ADC_2_Unused_17___SHIFT  	17
#define BMC8140_ADC_2_present___MASK     	UINT32_C(0x10000)
#define BMC8140_ADC_2_present___SHIFT    	16
#define BMC8140_ADC_2_P0V6_VTT_AB___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_2_P0V6_VTT_AB___SHIFT	0
#define BMC8140_ADC_2____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_3 ---- */
#define BMC8140_ADC_3____WIDTH	32
#define BMC8140_ADC_3____TYPE 	uint32_t

#define BMC8140_ADC_3_Unused_17___MASK      	UINT32_C(0xfffe0000)
#define BMC8140_ADC_3_Unused_17___SHIFT     	17
#define BMC8140_ADC_3_present___MASK        	UINT32_C(0x10000)
#define BMC8140_ADC_3_present___SHIFT       	16
#define BMC8140_ADC_3_0_9V_PCIEX_LDO___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_3_0_9V_PCIEX_LDO___SHIFT	0
#define BMC8140_ADC_3____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_4 ---- */
#define BMC8140_ADC_4____WIDTH	32
#define BMC8140_ADC_4____TYPE 	uint32_t

#define BMC8140_ADC_4_Unused_17___MASK   	UINT32_C(0xfffe0000)
#define BMC8140_ADC_4_Unused_17___SHIFT  	17
#define BMC8140_ADC_4_present___MASK     	UINT32_C(0x10000)
#define BMC8140_ADC_4_present___SHIFT    	16
#define BMC8140_ADC_4_P2V5_VPP_AB___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_4_P2V5_VPP_AB___SHIFT	0
#define BMC8140_ADC_4____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_5 ---- */
#define BMC8140_ADC_5____WIDTH	32
#define BMC8140_ADC_5____TYPE 	uint32_t

#define BMC8140_ADC_5_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_5_Unused_17___SHIFT	17
#define BMC8140_ADC_5_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_5_present___SHIFT  	16
#define BMC8140_ADC_5_1_8V_PCIE___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_5_1_8V_PCIE___SHIFT	0
#define BMC8140_ADC_5____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_6 ---- */
#define BMC8140_ADC_6____WIDTH	32
#define BMC8140_ADC_6____TYPE 	uint32_t

#define BMC8140_ADC_6_Unused_17___MASK  	UINT32_C(0xfffe0000)
#define BMC8140_ADC_6_Unused_17___SHIFT 	17
#define BMC8140_ADC_6_present___MASK    	UINT32_C(0x10000)
#define BMC8140_ADC_6_present___SHIFT   	16
#define BMC8140_ADC_6_PVCCIN_CPU___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_6_PVCCIN_CPU___SHIFT	0
#define BMC8140_ADC_6____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_7 ---- */
#define BMC8140_ADC_7____WIDTH	32
#define BMC8140_ADC_7____TYPE 	uint32_t

#define BMC8140_ADC_7_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_7_Unused_17___SHIFT	17
#define BMC8140_ADC_7_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_7_present___SHIFT  	16
#define BMC8140_ADC_7_P1V8_AUX___MASK  	UINT32_C(0xffff)
#define BMC8140_ADC_7_P1V8_AUX___SHIFT 	0
#define BMC8140_ADC_7____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_8 ---- */
#define BMC8140_ADC_8____WIDTH	32
#define BMC8140_ADC_8____TYPE 	uint32_t

#define BMC8140_ADC_8_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_8_Unused_17___SHIFT	17
#define BMC8140_ADC_8_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_8_present___SHIFT  	16
#define BMC8140_ADC_8_P3V3___MASK      	UINT32_C(0xffff)
#define BMC8140_ADC_8_P3V3___SHIFT     	0
#define BMC8140_ADC_8____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_9 ---- */
#define BMC8140_ADC_9____WIDTH	32
#define BMC8140_ADC_9____TYPE 	uint32_t

#define BMC8140_ADC_9_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_9_Unused_17___SHIFT	17
#define BMC8140_ADC_9_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_9_present___SHIFT  	16
#define BMC8140_ADC_9_P1V05___MASK     	UINT32_C(0xffff)
#define BMC8140_ADC_9_P1V05___SHIFT    	0
#define BMC8140_ADC_9____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_10 ---- */
#define BMC8140_ADC_10____WIDTH	32
#define BMC8140_ADC_10____TYPE 	uint32_t

#define BMC8140_ADC_10_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_10_Unused_17___SHIFT	17
#define BMC8140_ADC_10_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_10_present___SHIFT  	16
#define BMC8140_ADC_10_PVNN_PCH___MASK  	UINT32_C(0xffff)
#define BMC8140_ADC_10_PVNN_PCH___SHIFT 	0
#define BMC8140_ADC_10____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_11 ---- */
#define BMC8140_ADC_11____WIDTH	32
#define BMC8140_ADC_11____TYPE 	uint32_t

#define BMC8140_ADC_11_Unused_17___MASK   	UINT32_C(0xfffe0000)
#define BMC8140_ADC_11_Unused_17___SHIFT  	17
#define BMC8140_ADC_11_present___MASK     	UINT32_C(0x10000)
#define BMC8140_ADC_11_present___SHIFT    	16
#define BMC8140_ADC_11_PVCCANA_CPU___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_11_PVCCANA_CPU___SHIFT	0
#define BMC8140_ADC_11____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_12 ---- */
#define BMC8140_ADC_12____WIDTH	32
#define BMC8140_ADC_12____TYPE 	uint32_t

#define BMC8140_ADC_12_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_12_Unused_17___SHIFT	17
#define BMC8140_ADC_12_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_12_present___SHIFT  	16
#define BMC8140_ADC_12_P3V3_CPU___MASK  	UINT32_C(0xffff)
#define BMC8140_ADC_12_P3V3_CPU___SHIFT 	0
#define BMC8140_ADC_12____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_13 ---- */
#define BMC8140_ADC_13____WIDTH	32
#define BMC8140_ADC_13____TYPE 	uint32_t

#define BMC8140_ADC_13_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_13_Unused_17___SHIFT	17
#define BMC8140_ADC_13_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_13_present___SHIFT  	16
#define BMC8140_ADC_13_1V_PHY___MASK    	UINT32_C(0xffff)
#define BMC8140_ADC_13_1V_PHY___SHIFT   	0
#define BMC8140_ADC_13____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_14 ---- */
#define BMC8140_ADC_14____WIDTH	32
#define BMC8140_ADC_14____TYPE 	uint32_t

#define BMC8140_ADC_14_Unused_17___MASK 	UINT32_C(0xfffe0000)
#define BMC8140_ADC_14_Unused_17___SHIFT	17
#define BMC8140_ADC_14_present___MASK   	UINT32_C(0x10000)
#define BMC8140_ADC_14_present___SHIFT  	16
#define BMC8140_ADC_14_PVNN_NAC___MASK  	UINT32_C(0xffff)
#define BMC8140_ADC_14_PVNN_NAC___SHIFT 	0
#define BMC8140_ADC_14____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_15 ---- */
#define BMC8140_ADC_15____WIDTH	32
#define BMC8140_ADC_15____TYPE 	uint32_t

#define BMC8140_ADC_15_Unused_17___MASK     	UINT32_C(0xfffe0000)
#define BMC8140_ADC_15_Unused_17___SHIFT    	17
#define BMC8140_ADC_15_present___MASK       	UINT32_C(0x10000)
#define BMC8140_ADC_15_present___SHIFT      	16
#define BMC8140_ADC_15_P3V3_BMC_BATT___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_15_P3V3_BMC_BATT___SHIFT	0
#define BMC8140_ADC_15____REGMASK	UINT32_C(131071)

/* ---- BMC8140_ADC_THRESH_WARN_0 ---- */
#define BMC8140_ADC_THRESH_WARN_0____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_0____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_0_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_0_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_0_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_0_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_1 ---- */
#define BMC8140_ADC_THRESH_WARN_1____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_1____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_1_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_1_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_1_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_1_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_2 ---- */
#define BMC8140_ADC_THRESH_WARN_2____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_2____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_2_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_2_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_2_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_2_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_3 ---- */
#define BMC8140_ADC_THRESH_WARN_3____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_3____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_3_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_3_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_3_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_3_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_4 ---- */
#define BMC8140_ADC_THRESH_WARN_4____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_4____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_4_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_4_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_4_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_4_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_5 ---- */
#define BMC8140_ADC_THRESH_WARN_5____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_5____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_5_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_5_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_5_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_5_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_6 ---- */
#define BMC8140_ADC_THRESH_WARN_6____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_6____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_6_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_6_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_6_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_6_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_7 ---- */
#define BMC8140_ADC_THRESH_WARN_7____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_7____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_7_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_7_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_7_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_7_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_8 ---- */
#define BMC8140_ADC_THRESH_WARN_8____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_8____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_8_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_8_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_8_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_8_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_9 ---- */
#define BMC8140_ADC_THRESH_WARN_9____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_9____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_9_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_9_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_9_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_9_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_10 ---- */
#define BMC8140_ADC_THRESH_WARN_10____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_10____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_10_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_10_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_10_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_10_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_11 ---- */
#define BMC8140_ADC_THRESH_WARN_11____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_11____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_11_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_11_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_11_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_11_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_12 ---- */
#define BMC8140_ADC_THRESH_WARN_12____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_12____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_12_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_12_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_12_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_12_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_12____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_13 ---- */
#define BMC8140_ADC_THRESH_WARN_13____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_13____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_13_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_13_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_13_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_13_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_13____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_14 ---- */
#define BMC8140_ADC_THRESH_WARN_14____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_14____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_14_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_14_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_14_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_14_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_14____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_WARN_15 ---- */
#define BMC8140_ADC_THRESH_WARN_15____WIDTH	32
#define BMC8140_ADC_THRESH_WARN_15____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_WARN_15_warnhi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_WARN_15_warnhi___SHIFT	16
#define BMC8140_ADC_THRESH_WARN_15_warnlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_WARN_15_warnlo___SHIFT	0
#define BMC8140_ADC_THRESH_WARN_15____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_0 ---- */
#define BMC8140_ADC_THRESH_CRIT_0____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_0____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_0_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_0_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_0_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_0_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_1 ---- */
#define BMC8140_ADC_THRESH_CRIT_1____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_1____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_1_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_1_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_1_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_1_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_2 ---- */
#define BMC8140_ADC_THRESH_CRIT_2____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_2____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_2_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_2_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_2_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_2_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_3 ---- */
#define BMC8140_ADC_THRESH_CRIT_3____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_3____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_3_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_3_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_3_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_3_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_4 ---- */
#define BMC8140_ADC_THRESH_CRIT_4____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_4____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_4_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_4_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_4_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_4_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_5 ---- */
#define BMC8140_ADC_THRESH_CRIT_5____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_5____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_5_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_5_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_5_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_5_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_6 ---- */
#define BMC8140_ADC_THRESH_CRIT_6____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_6____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_6_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_6_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_6_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_6_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_7 ---- */
#define BMC8140_ADC_THRESH_CRIT_7____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_7____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_7_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_7_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_7_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_7_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_8 ---- */
#define BMC8140_ADC_THRESH_CRIT_8____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_8____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_8_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_8_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_8_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_8_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_9 ---- */
#define BMC8140_ADC_THRESH_CRIT_9____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_9____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_9_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_9_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_9_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_9_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_10 ---- */
#define BMC8140_ADC_THRESH_CRIT_10____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_10____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_10_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_10_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_10_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_10_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_11 ---- */
#define BMC8140_ADC_THRESH_CRIT_11____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_11____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_11_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_11_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_11_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_11_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_12 ---- */
#define BMC8140_ADC_THRESH_CRIT_12____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_12____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_12_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_12_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_12_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_12_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_12____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_13 ---- */
#define BMC8140_ADC_THRESH_CRIT_13____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_13____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_13_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_13_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_13_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_13_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_13____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_14 ---- */
#define BMC8140_ADC_THRESH_CRIT_14____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_14____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_14_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_14_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_14_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_14_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_14____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_ADC_THRESH_CRIT_15 ---- */
#define BMC8140_ADC_THRESH_CRIT_15____WIDTH	32
#define BMC8140_ADC_THRESH_CRIT_15____TYPE 	uint32_t

#define BMC8140_ADC_THRESH_CRIT_15_crithi___MASK 	UINT32_C(0xffff0000)
#define BMC8140_ADC_THRESH_CRIT_15_crithi___SHIFT	16
#define BMC8140_ADC_THRESH_CRIT_15_critlo___MASK 	UINT32_C(0xffff)
#define BMC8140_ADC_THRESH_CRIT_15_critlo___SHIFT	0
#define BMC8140_ADC_THRESH_CRIT_15____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_0 ---- */
#define BMC8140_TMP_0____WIDTH	32
#define BMC8140_TMP_0____TYPE 	uint32_t

#define BMC8140_TMP_0_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_0_Unused_21___SHIFT     	21
#define BMC8140_TMP_0_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_0_local_present___SHIFT 	20
#define BMC8140_TMP_0_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_0_LOCAL___SHIFT         	12
#define BMC8140_TMP_0_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_0_Unused_9___SHIFT      	9
#define BMC8140_TMP_0_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_0_remote_present___SHIFT	8
#define BMC8140_TMP_0_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_0_REMOTE___SHIFT        	0
#define BMC8140_TMP_0____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_1 ---- */
#define BMC8140_TMP_1____WIDTH	32
#define BMC8140_TMP_1____TYPE 	uint32_t

#define BMC8140_TMP_1_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_1_Unused_21___SHIFT     	21
#define BMC8140_TMP_1_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_1_local_present___SHIFT 	20
#define BMC8140_TMP_1_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_1_LOCAL___SHIFT         	12
#define BMC8140_TMP_1_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_1_Unused_9___SHIFT      	9
#define BMC8140_TMP_1_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_1_remote_present___SHIFT	8
#define BMC8140_TMP_1_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_1_REMOTE___SHIFT        	0
#define BMC8140_TMP_1____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_2 ---- */
#define BMC8140_TMP_2____WIDTH	32
#define BMC8140_TMP_2____TYPE 	uint32_t

#define BMC8140_TMP_2_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_2_Unused_21___SHIFT     	21
#define BMC8140_TMP_2_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_2_local_present___SHIFT 	20
#define BMC8140_TMP_2_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_2_LOCAL___SHIFT         	12
#define BMC8140_TMP_2_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_2_Unused_9___SHIFT      	9
#define BMC8140_TMP_2_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_2_remote_present___SHIFT	8
#define BMC8140_TMP_2_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_2_REMOTE___SHIFT        	0
#define BMC8140_TMP_2____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_3 ---- */
#define BMC8140_TMP_3____WIDTH	32
#define BMC8140_TMP_3____TYPE 	uint32_t

#define BMC8140_TMP_3_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_3_Unused_21___SHIFT     	21
#define BMC8140_TMP_3_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_3_local_present___SHIFT 	20
#define BMC8140_TMP_3_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_3_LOCAL___SHIFT         	12
#define BMC8140_TMP_3_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_3_Unused_9___SHIFT      	9
#define BMC8140_TMP_3_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_3_remote_present___SHIFT	8
#define BMC8140_TMP_3_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_3_REMOTE___SHIFT        	0
#define BMC8140_TMP_3____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_4 ---- */
#define BMC8140_TMP_4____WIDTH	32
#define BMC8140_TMP_4____TYPE 	uint32_t

#define BMC8140_TMP_4_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_4_Unused_21___SHIFT     	21
#define BMC8140_TMP_4_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_4_local_present___SHIFT 	20
#define BMC8140_TMP_4_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_4_LOCAL___SHIFT         	12
#define BMC8140_TMP_4_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_4_Unused_9___SHIFT      	9
#define BMC8140_TMP_4_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_4_remote_present___SHIFT	8
#define BMC8140_TMP_4_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_4_REMOTE___SHIFT        	0
#define BMC8140_TMP_4____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_5 ---- */
#define BMC8140_TMP_5____WIDTH	32
#define BMC8140_TMP_5____TYPE 	uint32_t

#define BMC8140_TMP_5_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_5_Unused_21___SHIFT     	21
#define BMC8140_TMP_5_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_5_local_present___SHIFT 	20
#define BMC8140_TMP_5_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_5_LOCAL___SHIFT         	12
#define BMC8140_TMP_5_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_5_Unused_9___SHIFT      	9
#define BMC8140_TMP_5_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_5_remote_present___SHIFT	8
#define BMC8140_TMP_5_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_5_REMOTE___SHIFT        	0
#define BMC8140_TMP_5____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_6 ---- */
#define BMC8140_TMP_6____WIDTH	32
#define BMC8140_TMP_6____TYPE 	uint32_t

#define BMC8140_TMP_6_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_6_Unused_21___SHIFT     	21
#define BMC8140_TMP_6_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_6_local_present___SHIFT 	20
#define BMC8140_TMP_6_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_6_LOCAL___SHIFT         	12
#define BMC8140_TMP_6_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_6_Unused_9___SHIFT      	9
#define BMC8140_TMP_6_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_6_remote_present___SHIFT	8
#define BMC8140_TMP_6_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_6_REMOTE___SHIFT        	0
#define BMC8140_TMP_6____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_7 ---- */
#define BMC8140_TMP_7____WIDTH	32
#define BMC8140_TMP_7____TYPE 	uint32_t

#define BMC8140_TMP_7_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_7_Unused_21___SHIFT     	21
#define BMC8140_TMP_7_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_7_local_present___SHIFT 	20
#define BMC8140_TMP_7_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_7_LOCAL___SHIFT         	12
#define BMC8140_TMP_7_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_7_Unused_9___SHIFT      	9
#define BMC8140_TMP_7_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_7_remote_present___SHIFT	8
#define BMC8140_TMP_7_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_7_REMOTE___SHIFT        	0
#define BMC8140_TMP_7____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_8 ---- */
#define BMC8140_TMP_8____WIDTH	32
#define BMC8140_TMP_8____TYPE 	uint32_t

#define BMC8140_TMP_8_Unused_21___MASK      	UINT32_C(0xffe00000)
#define BMC8140_TMP_8_Unused_21___SHIFT     	21
#define BMC8140_TMP_8_local_present___MASK  	UINT32_C(0x100000)
#define BMC8140_TMP_8_local_present___SHIFT 	20
#define BMC8140_TMP_8_LOCAL___MASK          	UINT32_C(0xff000)
#define BMC8140_TMP_8_LOCAL___SHIFT         	12
#define BMC8140_TMP_8_Unused_9___MASK       	UINT32_C(0xe00)
#define BMC8140_TMP_8_Unused_9___SHIFT      	9
#define BMC8140_TMP_8_remote_present___MASK 	UINT32_C(0x100)
#define BMC8140_TMP_8_remote_present___SHIFT	8
#define BMC8140_TMP_8_REMOTE___MASK         	UINT32_C(0xff)
#define BMC8140_TMP_8_REMOTE___SHIFT        	0
#define BMC8140_TMP_8____REGMASK	UINT32_C(2093567)

/* ---- BMC8140_TMP_THRESH_WARN_0 ---- */
#define BMC8140_TMP_THRESH_WARN_0____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_0____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_0_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_0_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_0_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_0_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_0_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_0_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_0_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_0_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_1 ---- */
#define BMC8140_TMP_THRESH_WARN_1____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_1____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_1_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_1_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_1_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_1_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_1_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_1_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_1_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_1_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_2 ---- */
#define BMC8140_TMP_THRESH_WARN_2____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_2____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_2_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_2_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_2_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_2_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_2_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_2_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_2_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_2_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_3 ---- */
#define BMC8140_TMP_THRESH_WARN_3____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_3____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_3_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_3_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_3_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_3_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_3_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_3_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_3_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_3_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_4 ---- */
#define BMC8140_TMP_THRESH_WARN_4____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_4____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_4_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_4_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_4_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_4_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_4_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_4_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_4_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_4_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_5 ---- */
#define BMC8140_TMP_THRESH_WARN_5____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_5____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_5_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_5_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_5_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_5_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_5_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_5_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_5_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_5_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_6 ---- */
#define BMC8140_TMP_THRESH_WARN_6____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_6____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_6_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_6_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_6_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_6_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_6_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_6_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_6_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_6_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_7 ---- */
#define BMC8140_TMP_THRESH_WARN_7____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_7____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_7_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_7_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_7_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_7_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_7_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_7_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_7_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_7_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_WARN_8 ---- */
#define BMC8140_TMP_THRESH_WARN_8____WIDTH	32
#define BMC8140_TMP_THRESH_WARN_8____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_WARN_8_local_warnhi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_WARN_8_local_warnhi___SHIFT 	24
#define BMC8140_TMP_THRESH_WARN_8_local_warnlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_WARN_8_local_warnlo___SHIFT 	16
#define BMC8140_TMP_THRESH_WARN_8_remote_warnhi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_WARN_8_remote_warnhi___SHIFT	8
#define BMC8140_TMP_THRESH_WARN_8_remote_warnlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_WARN_8_remote_warnlo___SHIFT	0
#define BMC8140_TMP_THRESH_WARN_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_0 ---- */
#define BMC8140_TMP_THRESH_CRIT_0____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_0____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_0_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_0_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_0_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_0_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_0_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_0_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_0_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_0_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_1 ---- */
#define BMC8140_TMP_THRESH_CRIT_1____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_1____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_1_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_1_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_1_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_1_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_1_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_1_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_1_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_1_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_2 ---- */
#define BMC8140_TMP_THRESH_CRIT_2____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_2____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_2_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_2_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_2_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_2_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_2_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_2_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_2_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_2_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_3 ---- */
#define BMC8140_TMP_THRESH_CRIT_3____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_3____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_3_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_3_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_3_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_3_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_3_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_3_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_3_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_3_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_4 ---- */
#define BMC8140_TMP_THRESH_CRIT_4____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_4____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_4_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_4_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_4_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_4_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_4_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_4_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_4_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_4_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_5 ---- */
#define BMC8140_TMP_THRESH_CRIT_5____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_5____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_5_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_5_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_5_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_5_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_5_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_5_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_5_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_5_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_6 ---- */
#define BMC8140_TMP_THRESH_CRIT_6____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_6____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_6_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_6_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_6_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_6_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_6_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_6_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_6_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_6_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_7 ---- */
#define BMC8140_TMP_THRESH_CRIT_7____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_7____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_7_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_7_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_7_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_7_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_7_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_7_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_7_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_7_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_TMP_THRESH_CRIT_8 ---- */
#define BMC8140_TMP_THRESH_CRIT_8____WIDTH	32
#define BMC8140_TMP_THRESH_CRIT_8____TYPE 	uint32_t

#define BMC8140_TMP_THRESH_CRIT_8_local_crithi___MASK  	UINT32_C(0xff000000)
#define BMC8140_TMP_THRESH_CRIT_8_local_crithi___SHIFT 	24
#define BMC8140_TMP_THRESH_CRIT_8_local_critlo___MASK  	UINT32_C(0xff0000)
#define BMC8140_TMP_THRESH_CRIT_8_local_critlo___SHIFT 	16
#define BMC8140_TMP_THRESH_CRIT_8_remote_crithi___MASK 	UINT32_C(0xff00)
#define BMC8140_TMP_THRESH_CRIT_8_remote_crithi___SHIFT	8
#define BMC8140_TMP_THRESH_CRIT_8_remote_critlo___MASK 	UINT32_C(0xff)
#define BMC8140_TMP_THRESH_CRIT_8_remote_critlo___SHIFT	0
#define BMC8140_TMP_THRESH_CRIT_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_STATUS_0 ---- */
#define BMC8140_FAN_STATUS_0____WIDTH	32
#define BMC8140_FAN_STATUS_0____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_0_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_0_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_0_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_0_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_0_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_0_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_0____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_1 ---- */
#define BMC8140_FAN_STATUS_1____WIDTH	32
#define BMC8140_FAN_STATUS_1____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_1_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_1_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_1_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_1_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_1_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_1_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_1____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_2 ---- */
#define BMC8140_FAN_STATUS_2____WIDTH	32
#define BMC8140_FAN_STATUS_2____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_2_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_2_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_2_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_2_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_2_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_2_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_2____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_3 ---- */
#define BMC8140_FAN_STATUS_3____WIDTH	32
#define BMC8140_FAN_STATUS_3____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_3_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_3_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_3_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_3_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_3_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_3_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_3____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_4 ---- */
#define BMC8140_FAN_STATUS_4____WIDTH	32
#define BMC8140_FAN_STATUS_4____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_4_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_4_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_4_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_4_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_4_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_4_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_4____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_5 ---- */
#define BMC8140_FAN_STATUS_5____WIDTH	32
#define BMC8140_FAN_STATUS_5____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_5_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_5_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_5_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_5_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_5_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_5_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_5____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_6 ---- */
#define BMC8140_FAN_STATUS_6____WIDTH	32
#define BMC8140_FAN_STATUS_6____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_6_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_6_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_6_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_6_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_6_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_6_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_6____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_7 ---- */
#define BMC8140_FAN_STATUS_7____WIDTH	32
#define BMC8140_FAN_STATUS_7____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_7_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_7_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_7_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_7_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_7_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_7_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_7____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_8 ---- */
#define BMC8140_FAN_STATUS_8____WIDTH	32
#define BMC8140_FAN_STATUS_8____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_8_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_8_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_8_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_8_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_8_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_8_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_8____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_9 ---- */
#define BMC8140_FAN_STATUS_9____WIDTH	32
#define BMC8140_FAN_STATUS_9____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_9_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_9_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_9_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_9_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_9_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_9_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_9____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_10 ---- */
#define BMC8140_FAN_STATUS_10____WIDTH	32
#define BMC8140_FAN_STATUS_10____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_10_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_10_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_10_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_10_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_10_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_10_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_10____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_STATUS_11 ---- */
#define BMC8140_FAN_STATUS_11____WIDTH	32
#define BMC8140_FAN_STATUS_11____TYPE 	uint32_t

#define BMC8140_FAN_STATUS_11_Unused_2___MASK    	UINT32_C(0xfffffffc)
#define BMC8140_FAN_STATUS_11_Unused_2___SHIFT   	2
#define BMC8140_FAN_STATUS_11_fan_failure___MASK 	UINT32_C(0x2)
#define BMC8140_FAN_STATUS_11_fan_failure___SHIFT	1
#define BMC8140_FAN_STATUS_11_fan_present___MASK 	UINT32_C(0x1)
#define BMC8140_FAN_STATUS_11_fan_present___SHIFT	0
#define BMC8140_FAN_STATUS_11____REGMASK	UINT32_C(3)

/* ---- BMC8140_FAN_TACH_0 ---- */
#define BMC8140_FAN_TACH_0____WIDTH	32
#define BMC8140_FAN_TACH_0____TYPE 	uint32_t

#define BMC8140_FAN_TACH_0_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_0_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_0_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_0_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_1 ---- */
#define BMC8140_FAN_TACH_1____WIDTH	32
#define BMC8140_FAN_TACH_1____TYPE 	uint32_t

#define BMC8140_FAN_TACH_1_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_1_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_1_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_1_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_2 ---- */
#define BMC8140_FAN_TACH_2____WIDTH	32
#define BMC8140_FAN_TACH_2____TYPE 	uint32_t

#define BMC8140_FAN_TACH_2_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_2_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_2_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_2_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_3 ---- */
#define BMC8140_FAN_TACH_3____WIDTH	32
#define BMC8140_FAN_TACH_3____TYPE 	uint32_t

#define BMC8140_FAN_TACH_3_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_3_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_3_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_3_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_4 ---- */
#define BMC8140_FAN_TACH_4____WIDTH	32
#define BMC8140_FAN_TACH_4____TYPE 	uint32_t

#define BMC8140_FAN_TACH_4_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_4_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_4_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_4_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_5 ---- */
#define BMC8140_FAN_TACH_5____WIDTH	32
#define BMC8140_FAN_TACH_5____TYPE 	uint32_t

#define BMC8140_FAN_TACH_5_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_5_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_5_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_5_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_6 ---- */
#define BMC8140_FAN_TACH_6____WIDTH	32
#define BMC8140_FAN_TACH_6____TYPE 	uint32_t

#define BMC8140_FAN_TACH_6_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_6_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_6_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_6_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_7 ---- */
#define BMC8140_FAN_TACH_7____WIDTH	32
#define BMC8140_FAN_TACH_7____TYPE 	uint32_t

#define BMC8140_FAN_TACH_7_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_7_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_7_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_7_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_8 ---- */
#define BMC8140_FAN_TACH_8____WIDTH	32
#define BMC8140_FAN_TACH_8____TYPE 	uint32_t

#define BMC8140_FAN_TACH_8_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_8_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_8_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_8_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_9 ---- */
#define BMC8140_FAN_TACH_9____WIDTH	32
#define BMC8140_FAN_TACH_9____TYPE 	uint32_t

#define BMC8140_FAN_TACH_9_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_9_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_9_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_9_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_10 ---- */
#define BMC8140_FAN_TACH_10____WIDTH	32
#define BMC8140_FAN_TACH_10____TYPE 	uint32_t

#define BMC8140_FAN_TACH_10_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_10_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_10_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_10_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TACH_11 ---- */
#define BMC8140_FAN_TACH_11____WIDTH	32
#define BMC8140_FAN_TACH_11____TYPE 	uint32_t

#define BMC8140_FAN_TACH_11_fan_tach_1___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TACH_11_fan_tach_1___SHIFT	16
#define BMC8140_FAN_TACH_11_fan_tach_0___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_TACH_11_fan_tach_0___SHIFT	0
#define BMC8140_FAN_TACH_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_0 ---- */
#define BMC8140_FAN_TARGET_0____WIDTH	32
#define BMC8140_FAN_TARGET_0____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_0_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_0_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_0_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_0_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_0_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_0_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_1 ---- */
#define BMC8140_FAN_TARGET_1____WIDTH	32
#define BMC8140_FAN_TARGET_1____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_1_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_1_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_1_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_1_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_1_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_1_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_2 ---- */
#define BMC8140_FAN_TARGET_2____WIDTH	32
#define BMC8140_FAN_TARGET_2____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_2_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_2_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_2_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_2_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_2_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_2_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_3 ---- */
#define BMC8140_FAN_TARGET_3____WIDTH	32
#define BMC8140_FAN_TARGET_3____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_3_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_3_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_3_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_3_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_3_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_3_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_4 ---- */
#define BMC8140_FAN_TARGET_4____WIDTH	32
#define BMC8140_FAN_TARGET_4____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_4_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_4_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_4_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_4_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_4_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_4_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_5 ---- */
#define BMC8140_FAN_TARGET_5____WIDTH	32
#define BMC8140_FAN_TARGET_5____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_5_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_5_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_5_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_5_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_5_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_5_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_6 ---- */
#define BMC8140_FAN_TARGET_6____WIDTH	32
#define BMC8140_FAN_TARGET_6____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_6_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_6_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_6_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_6_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_6_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_6_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_7 ---- */
#define BMC8140_FAN_TARGET_7____WIDTH	32
#define BMC8140_FAN_TARGET_7____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_7_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_7_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_7_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_7_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_7_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_7_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_8 ---- */
#define BMC8140_FAN_TARGET_8____WIDTH	32
#define BMC8140_FAN_TARGET_8____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_8_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_8_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_8_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_8_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_8_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_8_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_9 ---- */
#define BMC8140_FAN_TARGET_9____WIDTH	32
#define BMC8140_FAN_TARGET_9____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_9_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_9_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_9_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_9_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_9_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_9_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_10 ---- */
#define BMC8140_FAN_TARGET_10____WIDTH	32
#define BMC8140_FAN_TARGET_10____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_10_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_10_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_10_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_10_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_10_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_10_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_TARGET_11 ---- */
#define BMC8140_FAN_TARGET_11____WIDTH	32
#define BMC8140_FAN_TARGET_11____TYPE 	uint32_t

#define BMC8140_FAN_TARGET_11_set_fan_target___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_TARGET_11_set_fan_target___SHIFT	16
#define BMC8140_FAN_TARGET_11_is_overridden___MASK  	UINT32_C(0x8000)
#define BMC8140_FAN_TARGET_11_is_overridden___SHIFT 	15
#define BMC8140_FAN_TARGET_11_fan_target___MASK     	UINT32_C(0x7fff)
#define BMC8140_FAN_TARGET_11_fan_target___SHIFT    	0
#define BMC8140_FAN_TARGET_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_0 ---- */
#define BMC8140_FAN_THRESH_0____WIDTH	32
#define BMC8140_FAN_THRESH_0____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_0_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_0_min___SHIFT	16
#define BMC8140_FAN_THRESH_0_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_0_max___SHIFT	0
#define BMC8140_FAN_THRESH_0____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_1 ---- */
#define BMC8140_FAN_THRESH_1____WIDTH	32
#define BMC8140_FAN_THRESH_1____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_1_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_1_min___SHIFT	16
#define BMC8140_FAN_THRESH_1_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_1_max___SHIFT	0
#define BMC8140_FAN_THRESH_1____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_2 ---- */
#define BMC8140_FAN_THRESH_2____WIDTH	32
#define BMC8140_FAN_THRESH_2____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_2_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_2_min___SHIFT	16
#define BMC8140_FAN_THRESH_2_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_2_max___SHIFT	0
#define BMC8140_FAN_THRESH_2____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_3 ---- */
#define BMC8140_FAN_THRESH_3____WIDTH	32
#define BMC8140_FAN_THRESH_3____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_3_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_3_min___SHIFT	16
#define BMC8140_FAN_THRESH_3_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_3_max___SHIFT	0
#define BMC8140_FAN_THRESH_3____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_4 ---- */
#define BMC8140_FAN_THRESH_4____WIDTH	32
#define BMC8140_FAN_THRESH_4____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_4_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_4_min___SHIFT	16
#define BMC8140_FAN_THRESH_4_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_4_max___SHIFT	0
#define BMC8140_FAN_THRESH_4____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_5 ---- */
#define BMC8140_FAN_THRESH_5____WIDTH	32
#define BMC8140_FAN_THRESH_5____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_5_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_5_min___SHIFT	16
#define BMC8140_FAN_THRESH_5_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_5_max___SHIFT	0
#define BMC8140_FAN_THRESH_5____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_6 ---- */
#define BMC8140_FAN_THRESH_6____WIDTH	32
#define BMC8140_FAN_THRESH_6____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_6_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_6_min___SHIFT	16
#define BMC8140_FAN_THRESH_6_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_6_max___SHIFT	0
#define BMC8140_FAN_THRESH_6____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_7 ---- */
#define BMC8140_FAN_THRESH_7____WIDTH	32
#define BMC8140_FAN_THRESH_7____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_7_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_7_min___SHIFT	16
#define BMC8140_FAN_THRESH_7_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_7_max___SHIFT	0
#define BMC8140_FAN_THRESH_7____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_8 ---- */
#define BMC8140_FAN_THRESH_8____WIDTH	32
#define BMC8140_FAN_THRESH_8____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_8_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_8_min___SHIFT	16
#define BMC8140_FAN_THRESH_8_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_8_max___SHIFT	0
#define BMC8140_FAN_THRESH_8____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_9 ---- */
#define BMC8140_FAN_THRESH_9____WIDTH	32
#define BMC8140_FAN_THRESH_9____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_9_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_9_min___SHIFT	16
#define BMC8140_FAN_THRESH_9_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_9_max___SHIFT	0
#define BMC8140_FAN_THRESH_9____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_10 ---- */
#define BMC8140_FAN_THRESH_10____WIDTH	32
#define BMC8140_FAN_THRESH_10____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_10_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_10_min___SHIFT	16
#define BMC8140_FAN_THRESH_10_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_10_max___SHIFT	0
#define BMC8140_FAN_THRESH_10____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_FAN_THRESH_11 ---- */
#define BMC8140_FAN_THRESH_11____WIDTH	32
#define BMC8140_FAN_THRESH_11____TYPE 	uint32_t

#define BMC8140_FAN_THRESH_11_min___MASK 	UINT32_C(0xffff0000)
#define BMC8140_FAN_THRESH_11_min___SHIFT	16
#define BMC8140_FAN_THRESH_11_max___MASK 	UINT32_C(0xffff)
#define BMC8140_FAN_THRESH_11_max___SHIFT	0
#define BMC8140_FAN_THRESH_11____REGMASK	UINT32_C(4294967295)

/* ---- BMC8140_RESERVED_THESTART ---- */
#define BMC8140_RESERVED_THESTART____WIDTH	32
#define BMC8140_RESERVED_THESTART____TYPE 	uint32_t

#define BMC8140_RESERVED_THESTART____REGMASK	UINT32_C(0)

/* ---- BMC8140_RESERVED_THEEND ---- */
#define BMC8140_RESERVED_THEEND____WIDTH	32
#define BMC8140_RESERVED_THEEND____TYPE 	uint32_t

#define BMC8140_RESERVED_THEEND____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_0 ---- */
#define BMC8140_FAN_FRU_IDP_0____WIDTH	32
#define BMC8140_FAN_FRU_IDP_0____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_0____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_1 ---- */
#define BMC8140_FAN_FRU_IDP_1____WIDTH	32
#define BMC8140_FAN_FRU_IDP_1____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_1____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_2 ---- */
#define BMC8140_FAN_FRU_IDP_2____WIDTH	32
#define BMC8140_FAN_FRU_IDP_2____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_2____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_3 ---- */
#define BMC8140_FAN_FRU_IDP_3____WIDTH	32
#define BMC8140_FAN_FRU_IDP_3____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_3____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_4 ---- */
#define BMC8140_FAN_FRU_IDP_4____WIDTH	32
#define BMC8140_FAN_FRU_IDP_4____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_4____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_5 ---- */
#define BMC8140_FAN_FRU_IDP_5____WIDTH	32
#define BMC8140_FAN_FRU_IDP_5____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_5____REGMASK	UINT32_C(0)

/* ---- BMC8140_FAN_FRU_IDP_THEEND ---- */
#define BMC8140_FAN_FRU_IDP_THEEND____WIDTH	32
#define BMC8140_FAN_FRU_IDP_THEEND____TYPE 	uint32_t

#define BMC8140_FAN_FRU_IDP_THEEND____REGMASK	UINT32_C(0)

#ifdef __KERNEL__
#ifdef CONFIG_REGMAP_MMIO

static const struct regmap_config bmc8140_regmap_config = {
        .reg_bits     = 32,
        .val_bits     = 32,
        .reg_stride   = 4,
        .max_register = sizeof(struct Bmc8140_dev_reg),
};

#ifndef UINT16_C
#define UINT16_C(c)  __UINT16_C(c)
#endif
#ifndef UINT32_C
#define UINT32_C(c)  __UINT32_C(c)
#endif

#endif /* CONFIG_REGMAP_MMIO */
#endif /* __KERNEL__ */
#endif
