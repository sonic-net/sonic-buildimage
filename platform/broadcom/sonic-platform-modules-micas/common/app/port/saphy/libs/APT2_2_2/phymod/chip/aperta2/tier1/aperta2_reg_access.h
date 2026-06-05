
/**************************************************************************************/
/*                                                                                    */
/*  Description   :     aperta2_reg_access.h                                           */
/*                                                                                    */
/*  $Copyright: (c) 2022 Broadcom. */
/*  Broadcom Proprietary and Confidential. All rights reserved.$ */
/*                                                                                    */
/**************************************************************************************/


#ifndef __APERTA2_REG_ACCESS__H__
#define __APERTA2_REG_ACCESS__H__

#include <phymod/phymod_acc.h>
#include <phymod/phymod_reg.h>

#include <tier1/aperta2_pm_seq.h>

   
#define APERTA2_PM_DC3PORT_16B_BASEADR                   (0xA0000000)
#define APERTA2_PM_DC3PORT_32B_BASEADR                   (0xA1000000)
#define APERTA2_PM_DC3PORT_48B_BASEADR                   (0xA2000000)
#define APERTA2_PM_DC3PORT_64B_BASEADR                   (0xA3000000)
#define APERTA2_PM_DC3MAC_16B_BASEADR                    (0xA4000000)
#define APERTA2_PM_DC3MAC_32B_BASEADR                    (0xA5000000)
#define APERTA2_PM_DC3MAC_48B_BASEADR                    (0xA6000000)
#define APERTA2_PM_DC3MAC_64B_BASEADR                    (0xA7000000)
#define APERTA2_PM_TSCP_16B_BASEADR                      (0xA8000000)
#define APERTA2_PM_TSC_PERIGRINE_BASE                    (0xA9000000)
#define APERTA2_CHIP_IND_BASEADR                         (0xAA000000)
#define APERTA2_EIP218_P2M_INGRESS_BASEADR               (0x50700000)
#define APERTA2_EIP218_P2M_EGRESS_BASEADR                (0x50300000)
#define APERTA2_EIP218_RC_EGRESS_BASEADR                 (0x50400000)
#define APERTA2_EIP163_INGRESS_BASEADR                   (0x50500000)
#define APERTA2_EIP164_INGRESS_BASEADR                   (0x50600000)
#define APERTA2_EIP163_EGRESS_BASEADR                    (0x50100000)
#define APERTA2_EIP164_EGRESS_BASEADR                    (0x50200000)
#define APERTA2_PKT_INJ_EGRESS_BASEADR                   (0x50800000)
#define APERTA2_PKT_EXT_EGRESS_BASEADR                   (0x50900000)
#define APERTA2_PKT_INJ_INGRESS_BASEADR                  (0x50A00000)
#define APERTA2_PKT_EXT_INGRESS_BASEADR                  (0x50B00000)
#define APERTA2_M0_MST_BASEADR                           (0x40030000)
#define APERTA2_VTMON_AVS_I2C_PMON_BASEADR               (0x52000000)
/* Using FW message for the following direct FW registers */
#define APERTA2_FWREG_BASEADDR_RANGE_START               (0x0101A000)
#define APERTA2_FWREG_BASEADDR_RANGE_END                 (0x0101A3FF)
#define APERTA2_REG_BASE_SHIFT                           20
#define APERTA2_REG_READ                                 0
#define APERTA2_REG_WRITE                                1

#define APERTA2_FW_PM_REG_ACCESS_TYPE(REG_ADDR, PORT_SEL,FW_S, ACCESS_TYPE)  \
    {uint8_t per_port = (REG_ADDR & (1 << 17)) ? 0 : 1;                      \
          if (per_port == 0) {                                               \
              if ( FW_S == APERTA2_FW_PM_PORT) {                             \
                  ACCESS_TYPE = 0; /* For general reg port*/                 \
              } else {                                                       \
                  ACCESS_TYPE = 0x20; /*For General reg mac*/                \
              }                                                              \
          } else {                                                           \
              if ( FW_S == APERTA2_FW_PM_PORT) {                             \
                  ACCESS_TYPE = 0x80 | (PORT_SEL);                           \
              } else {                                                       \
                  ACCESS_TYPE = 0xA0 | (PORT_SEL);                           \
              }                                                              \
          }                                                                  \
    }

#define APERTA2_UPDATE_REG_ADDR(fw_slave, reg_addr, port_sel)           \
    ing_port = (port_sel & 0xFF);                      \
    egr_port = (port_sel >> 8) & 0xFF;                 \
    ing_port = (ing_port>7)?(ing_port-8):ing_port;                      \
    egr_port = (egr_port>7)?(egr_port-8):egr_port;                      \
    if (fw_slave == APERTA2_FW_MACSEC) {                                \
        if ((reg_addr >> APERTA2_REG_BASE_SHIFT) == (APERTA2_EIP218_P2M_INGRESS_BASEADR >> APERTA2_REG_BASE_SHIFT)){ \
                macsec_reg.oct_sel =  ((port_sel & 0xFF) > 7) ? APERTA2_PM_OCTAL2 : APERTA2_PM_OCTAL1; \
                reg_addr = (reg_addr & ~(0xF << 8)) | (ing_port << 8); \
        } else if (((reg_addr >> APERTA2_REG_BASE_SHIFT) == (APERTA2_EIP218_P2M_EGRESS_BASEADR >> APERTA2_REG_BASE_SHIFT)) ||\
                ((reg_addr >> APERTA2_REG_BASE_SHIFT) == (APERTA2_EIP218_RC_EGRESS_BASEADR >> APERTA2_REG_BASE_SHIFT))) {\
                macsec_reg.oct_sel =  (((port_sel >>8)& 0xFF) > 7) ? APERTA2_PM_OCTAL2 : APERTA2_PM_OCTAL1; \
                reg_addr = (reg_addr & ~(0xF << 8)) | (egr_port << 8);\
        }\
    } else if (fw_slave == APERTA2_FW_INDIRECT) {\
        /* Special case for PTP registers*/\
        if (((reg_addr & 0xFFFF) >= 0x6000) && ((reg_addr & 0xFFFF) < 0x709C)) {\
            reg_addr &= (~(0xF << 8));\
            if (((reg_addr & 0xFFFF) <= 0x6037) || \
                (((reg_addr & 0xFFFF) >= 0x7000) && (((reg_addr & 0xFFFF) <= 0x700C)))) {\
            /* set reg_addr[11:8] with port offset*/\
                ind_reg.oct_sel =  (((port_sel >>8)& 0xFF) > 7) ? APERTA2_PM_OCTAL2 : APERTA2_PM_OCTAL1; \
                reg_addr |= ((egr_port & 0x000F) << 8);\
            } else {                                   \
                ind_reg.oct_sel =  ((port_sel & 0xFF) > 7) ? APERTA2_PM_OCTAL2 : APERTA2_PM_OCTAL1; \
                reg_addr |= ((ing_port & 0x000F) << 8);\
            }                                          \
        } \
    }

/*Slaves based on FW messages read/write messages*/
typedef enum APERTA2_SLAVE_TYPE_E 
{
    APERTA2_DIRECT_ACCESS = 0,      /* Direct access register*/
    APERTA2_FW_PM_PORT,             /* PM port Register Block */ 
    APERTA2_FW_PM_MAC,              /* PM MAC  Register Block*/ 
    APERTA2_FW_PM_TSC,              /* PM TSC  Register Block*/
    APERTA2_FW_MACSEC,              /* MACSEC  Register block */
    APERTA2_FW_INDIRECT,            /* Indirect Register Block*/
    APERTA2_FW_AHB                  /* AHB register block*/
} APERTA2_SLAVE_TYPE_T;

/* Datastructure for FW register read/write*/
typedef struct aperta2_register_read_s {
    uint32_t port_number;            /* Port number*/
    uint32_t lane_sel;               /* Lane Select*/ 
    uint32_t pll_sel;                /* PLL select*/
    uint32_t micro_sel;              /* Micro Select*/
    uint32_t reg_addr;               /* Register address to read*/
    uint32_t oct_sel;                /* Octal Select*/
    uint32_t pm_sel;                 /* PM select*/
    uint64_t reg_data;               /* Register Read data*/
} aperta2_register_read_t;

typedef struct aperta2_register_write_s {
    uint32_t port_number;            /* Port number*/
    uint32_t lane_sel;               /* Lane Select*/ 
    uint32_t pll_sel;                /* PLL select*/
    uint32_t micro_sel;              /* Micro Select*/
    uint32_t reg_addr;               /* Register address to read*/
    uint32_t mask;                   /* Data Mask for read-modify-write*/ 
    uint32_t oct_sel;                /* Octal Select*/
    uint32_t pm_sel;                 /* PM select*/
    uint64_t reg_data;               /* Register Read data*/
} aperta2_register_write_t;

int plp_aperta2_reg32_read(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint32_t *data);
int plp_aperta2_reg32_write(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint32_t data);
int plp_aperta2_reg64_read(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint64_t *data);
int plp_aperta2_reg64_write(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint64_t data);
int plp_aperta2_mem_write(const plp_aperta2_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data);
int plp_aperta2_mem_read(const plp_aperta2_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data);
int plp_aperta2_direct_reg_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t reg_data);

int plp_aperta2_direct_reg_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t *reg_data);

int plp_aperta2_register_read(const plp_aperta2_phymod_phy_access_t* phy, aperta2_register_read_t *read_param);

int plp_aperta2_register_write(const plp_aperta2_phymod_phy_access_t* phy, aperta2_register_write_t *write_param );

uint32_t plp_aperta2_phymod_aperta2_field32_get(const uint32_t *entbuf, int sbit, int ebit);
void plp_aperta2_phymod_aperta2_field32_set(uint32_t *entbuf, int sbit, int ebit, uint32_t fval);
void plp_aperta2_phymod_aperta2_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf);
uint32_t * plp_aperta2_phymod_aperta2_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf);
int plp_aperta2_get_lmi_slave_length(uint32_t reg_addr, uint32_t *slave, uint32_t *length);
int plp_aperta2_mem_access_type_len(phymod_mem_type_t memory_type, uint8_t *len, uint8_t *access_type);
int aperta2_get_octal_crossing(const plp_aperta2_phymod_phy_access_t *phy, aperta2_octal_crossing_t *crossing) ;

#endif
