#ifndef ANLT_H
#define ANLT_H

#include "common/common_display.h"

/* ANLT requires hive "AutoNegIEEE" and "AutoNegCustom" defined. */
extern const RegHive_t AutoNegIEEE[], AutoNegCustom[];

#if HAL_SUPPORT_ANLT
#define ANI HIVE(AutoNegIEEE)
#define ANC HIVE(AutoNegCustom)

/* Auto Neg IEEE */
#define REG_AUTONEG_IEEE_MODE     REGBITR(ANC, 0x00, 12)
#define REG_AUTONEG_IEEE_STATE    REGBITR(ANC, 0x01, 3, 0)
#define REG_AUTONEG_BP_15_0       REGBITR(ANI, 0x10, 15, 0)
#define REG_AUTONEG_BP_31_16      REGBITR(ANI, 0x11, 15, 0)
#define REG_AUTONEG_BP_47_32      REGBITR(ANI, 0x12, 15, 0)
#define REG_AUTONEG_NP1_15_0      REGBITR(ANI, 0xF0, 15, 0)
#define REG_AUTONEG_NP1_31_16     REGBITR(ANI, 0xF1, 15, 0)
#define REG_AUTONEG_NP1_47_32     REGBITR(ANI, 0xF2, 15, 0)
#define REG_AUTONEG_NP2_15_0      REGBITR(ANI, 0xF3, 15, 0)
#define REG_AUTONEG_NP2_31_16     REGBITR(ANI, 0xF4, 15, 0)
#define REG_AUTONEG_NP2_47_32     REGBITR(ANI, 0xF5, 15, 0)
#define REG_AUTONEG_PAGE_RECEIVED REGBITR(ANI, 0x01, 6)
#define REG_AUTONEG_PAGE_NUMBER   REGBITR(ANC, 0x10, 2, 0)
#define REG_AUTONEG_TX_PAGE0_15_0 REGBITR(ANC, 0x11, 15, 0)
#define REG_AUTONEG_RX_PAGE0_15_0 REGBITR(ANC, 0x30, 15, 0)

CredoError_t common_set_autoneg_pages(CredoSlice_t* slice, int lane, int page_id, uint64_t page);
CredoError_t common_get_autoneg_exchanged_pages(CredoSlice_t* slice, int lane, int* page_count,
                                                uint64_t transmitted_pages[9], uint64_t received_pages[9]);
CredoError_t common_dump_anlt_pages(CredoSlice_t* slice, int* lane_list, unsigned* an_state, uint64_t tx_pages[][9],
                                    uint64_t rx_pages[][9], const DisplayState_t* D);
CredoError_t common_dump_anlt_detail(CredoSlice_t* slice, int* lane_list, unsigned* an_state, uint64_t tx_pages[][9],
                                     uint64_t rx_pages[][9], const DisplayState_t* D);
#endif  // HAL_SUPPORT_ANLT

#endif
