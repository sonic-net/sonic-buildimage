
/**************************************************************************************/
/*                                                                                    */
/*  Description   :     aperta_reg_access.h                                           */
/*                                                                                    */
/*  $Copyright: (c) 2020 Broadcom. */
/*  Broadcom Proprietary and Confidential. All rights reserved.$ */
/*                                                                                    */
/**************************************************************************************/


#ifndef __APERTA_REG_ACCESS__H__
#define __APERTA_REG_ACCESS__H__

#include <phymod/phymod_acc.h>
#include <phymod/phymod_reg.h>
#include "aperta_cfg_seq.h"
#include "aperta_pm_seq.h"
#include <tier1/bcmi_aperta_d_defs.h>

#ifdef BE_HOST
#define IS_BIG_ENDIAN    (1)
#define IS_LITTLE_ENDIAN (0)
#else
#define IS_BIG_ENDIAN    (0)
#define IS_LITTLE_ENDIAN (1)
#endif

#define PHYMOD_REG_APERTA_TSCBHAWK      0x18000000
typedef union APERTA_TSC_ADDR_TYPE_U {
    uint32_t data;
    struct  {
        #if IS_BIG_ENDIAN
            uint32_t  device         : 5  ; /* [31:27]*/
            uint32_t  reserved_26    : 1  ; /* [26:24]*/
            uint32_t  mpp1           : 1  ; /* [25:25]*/
            uint32_t  mpp0           : 1  ; /* [24:24]*/
            uint32_t  port_id        : 5  ; /* [23:19]*/
            uint32_t  lane_mode      : 3  ; /* [18:16]*/
            uint32_t  reg_addr       : 16 ; /* [15:00]*/
        #endif /*IS_BIG_ENDIAN*/
        #if IS_LITTLE_ENDIAN
            uint32_t  reg_addr       : 16 ; /* [15:00]*/
            uint32_t  lane_mode      : 3  ; /* [18:16]*/
            uint32_t  port_id        : 5  ; /* [23:19]*/
            uint32_t  mpp0           : 1  ; /* [24:24]*/
            uint32_t  mpp1           : 1  ; /* [25:25]*/
            uint32_t  reserved_26    : 1  ; /* [26:26]*/
            uint32_t  device         : 5  ; /* [31:27]*/
        #endif /*IS_LITTLE_ENDIAN*/
    } fields;
 } APERTA_TSC_ADDR_TYPE_T;

typedef union APERTA_PMD_ADDR_TYPE_U{
    uint32_t data;
    struct  {
        #if IS_BIG_ENDIAN
            uint32_t  device           : 5  ; /* [31:27] */
            uint32_t  pll_micro_brdcst : 1  ; /* [26:26] */
            uint32_t  pll_micro_sel    : 2  ; /* [25:24] */
            uint32_t  lane_mode        : 8  ; /* [23:16] */
            uint32_t  reg_addr         : 16 ; /* [15:00] */
        #endif /*IS_BIG_ENDIAN*/
        #if IS_LITTLE_ENDIAN
            uint32_t  reg_addr         : 16 ; /* [15:00] */
            uint32_t  lane_mode        : 8  ; /* [23:16] */
            uint32_t  pll_micro_sel    : 2  ; /* [25:24] */
            uint32_t  pll_micro_brdcst : 1  ; /* [26:26] */
            uint32_t  device           : 5  ; /* [31:27] */
        #endif /*IS_LITTLE_ENDIAN*/
    } fields;
} APERTA_PMD_ADDR_TYPE_T;

typedef enum APERTA_REG_ACCESS_E{
    APERTA_REG_MDIO = 0,
    APERTA_REG_LMI  = 1
} APERTA_REG_ACCESS_T;

typedef enum APERTA_TSC_DEVICES_E{
  APERTA_TSC = 0,
  APERTA_PMD = 1
} APERTA_TSC_DEVICES_T;

typedef enum APERTA_PM_BLOCK_SEL_TYPE_E{
    APERTA_BLK_PM_PORT,
    APERTA_BLK_PM_CDMAC_0,
    APERTA_BLK_PM_CDMAC_1
} APERTA_PM_BLOCK_SEL_TYPE_T;

typedef struct APERTA_PM_MEM_ACCESS_S{
    int               pm_mem_sel;
    phymod_mem_type_t pm_mem_type;
    int               pm_mem_addr;
    int               pm_mem_rw; /* 0 - Read, 1 - Write*/
    int               pm_mem_len;
    uint32_t          pm_mem_data[20]; /*incresing to 20 for residual*/
} APERTA_PM_MEM_ACCESS_T;

typedef enum APERTA_SLAVE_SEL_E {
    APERTA_LINE_SIDE_PM,
    APERTA_SYS_SIDE_PM,
    APERTA_BOTH_PM
} APERTA_SLAVE_SEL_T;

/* LMI Slave Type*/
typedef enum APERTA_SLAVE_TYPE_E {
   APERTA_SLAVE_NONE,        /* Chip Top Direct Register*/
   APERTA_PM_PORT,
   APERTA_PM_MAC,
   APERTA_PM_TSC,
   APERTA_EIP163_INGRESS,
   APERTA_EIP164_INGRESS,
   APERTA_EIP218_INGRESS,
   APERTA_CHIP_RDB,    /* Chip Top Indirect Register*/
   APERTA_EIP163_EGRESS,
   APERTA_EIP164_EGRESS,
   APERTA_EIP218_EGRESS
} APERTA_SLAVE_TYPE_T;

#define APERTA_LMI_RETRY_CNT                           100

#ifdef WIN32
#define APERTA_LMI_SLEEP_TIME                          1
#else
#define APERTA_LMI_SLEEP_TIME                          100
#endif

#define APERTA_MAX_PORT                                8

#define APERTA_PM_CDPORT_16B_BASEADR                   (0x10000000)
#define APERTA_PM_CDPORT_32B_BASEADR                   (0x11000000)
#define APERTA_PM_CDPORT_48B_BASEADR                   (0x12000000)
#define APERTA_PM_CDPORT_64B_BASEADR                   (0x13000000)
#define APERTA_PM_CDMAC_16B_BASEADR                    (0x14000000)
#define APERTA_PM_CDMAC_32B_BASEADR                    (0x15000000)
#define APERTA_PM_CDMAC_48B_BASEADR                    (0x16000000)
#define APERTA_PM_CDMAC_64B_BASEADR                    (0x17000000)
#define APERTA_PM_TSCBH_16B_BASEADR                    (0x18000000)
#define APERTA_PM_TSC_BLACKHAWK_BASEADR                (0x1800d000)
#define APERTA_EIP218_INGRESS_BASEADR                  (0x21000000)
#define APERTA_EIP218_EGRESS_BASEADR                   (0x22000000)
#define APERTA_EIP163_INGRESS_BASEADR                  (0x43000000)
#define APERTA_EIP164_INGRESS_BASEADR                  (0x44000000)
#define APERTA_EIP163_EGRESS_BASEADR                   (0x45000000)
#define APERTA_EIP164_EGRESS_BASEADR                   (0x46000000)
#define APERTA_CHIP_IND_BASEADR                        (0x49000000)
#define APERTA_P1588_BASEADR                           (0x49007000)
#define APERTA_EGR_SF_BASEADR                          (0x49009000)
#define APERTA_ING_FC_BASEADR                          (0x4900a000)
#define APERTA_EGR_FC_BASEADR                          (0x4900b000)


#define APERTA_ALL_PLL  (-1) /* Selects all plls for writting*/
#define APERTA_PLL_NONE (-2) /* PLL selection is don't care*/


int plp_aperta_register_write(const plp_aperta_phymod_phy_access_t* phy, int port_number, int lane_sel,
                          int pll_sel, int micro_sel, int reg_addr,
                              uint64_t reg_data, int reg_mask);

int plp_aperta_direct_reg_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t reg_data);

int plp_aperta_direct_reg_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t *reg_data);

int plp_aperta_reg_read(const plp_aperta_phymod_phy_access_t* phy, int port_number, int lane_sel,
                   int pll_sel, int micro_sel,
                   uint32_t reg_addr, uint64_t *reg_data);

int plp_aperta_indirect_reg_access(const plp_aperta_phymod_phy_access_t* phy,
                         uint16_t port_sel, uint16_t lane_sel, int pll_sel, int micro_sel,
                         uint32_t reg_addr, uint64_t* reg_data, uint64_t* tsc_data_mask,
                         int num_words, int write_en);

int plp_aperta_lmi_reg_access(const plp_aperta_phymod_phy_access_t* phy,
                    uint16_t port_sel, uint16_t lane_sel, int pll_sel, int micro_sel,
                    uint32_t reg_addr, uint64_t* reg_data, uint64_t *tsc_data_mask,
                    int num_words, int write_en);

APERTA_SLAVE_TYPE_T plp_aperta_get_lmi_slave(int reg_addr);

int plp_aperta_get_lmi_slave_data_length(int reg_addr) ;

int plp_aperta_get_lmi_cmd(BCMI_APERTA_D_LMI_LMI_CMDr_t* lmi_cmd, int side, int port_sel,
                 int reg_addr, int reg_mask, int write_en, uint32_t lane_mask);

int plp_aperta_get_lmi_slave_sel(APERTA_SLAVE_TYPE_T slave, plp_aperta_phymod_port_loc_t port_loc, int write_en);

int plp_aperta_get_lmi_access_prog(int addr, int mac, int data_length, int reg_mask);

int plp_aperta_get_tsc_addr(APERTA_TSC_ADDR_TYPE_T* tsc_addr, APERTA_PMD_ADDR_TYPE_T* pmd_addr, uint16_t port_sel,
                  uint16_t lane_sel, int pll_sel, int micro_sel, uint32_t reg_addr, int *device_type);

int plp_aperta_pm_tsc_mem_access(const plp_aperta_phymod_phy_access_t *phy, APERTA_PM_MEM_ACCESS_T* pm_mem_access);

int plp_aperta_prg_mem_access_prog_val_len(phymod_mem_type_t memory_type, int *len);

int plp_aperta_reg64_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint64_t *data);

int plp_aperta_reg64_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint64_t data);

int plp_aperta_reg32_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data);

int plp_aperta_reg32_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data);

void plp_aperta_phymod_aperta_field32_set(uint32_t *entbuf, int sbit, int ebit, uint32_t fval);

uint32_t plp_aperta_phymod_aperta_field32_get(const uint32_t *entbuf, int sbit, int ebit);

void
plp_aperta_phymod_aperta_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf);

uint32_t *
plp_aperta_phymod_aperta_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf);

int
plp_aperta_mem_read(const plp_aperta_phymod_phy_access_t *pa, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* val);
int
plp_aperta_mem_write(const plp_aperta_phymod_phy_access_t *pa, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data);

 #endif
