#include "aperta_reg_access.h"
#include "aperta_msg_tasks.h"

#ifdef ATE_PRINT_ENABLED
#define APERTA_LMI_REG_DUMP         1
#else
#define APERTA_LMI_REG_DUMP         0
#endif
#define APERTA_FW_READ_WRITE        1

int
plp_aperta_mem_read(const plp_aperta_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* val)
{
    APERTA_PM_MEM_ACCESS_T mem_access;
    int len = 0 , rv = 0, index = 0;

    PHYMOD_MEMSET(&mem_access, 0, sizeof(APERTA_PM_MEM_ACCESS_T));
#if !APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_reg32_write(phy, 0x10020015, 1));
#endif
    mem_access.pm_mem_sel = (phy->port_loc ==phymodPortLocLine) ? APERTA_LINE_SIDE_PM : APERTA_SYS_SIDE_PM ;
    mem_access.pm_mem_type = mem_type;
    mem_access.pm_mem_addr = mem_index;
    mem_access.pm_mem_rw = 0; /* 0 - Read, 1 - Write*/
    plp_aperta_prg_mem_access_prog_val_len(mem_type, &len);
    rv = plp_aperta_pm_tsc_mem_access(phy, &mem_access);
    if (rv != PHYMOD_E_NONE) {
#if !APERTA_FW_READ_WRITE
        PHYMOD_IF_ERR_RETURN(
              plp_aperta_reg32_write(phy, 0x10020015, 0));
#endif        
        return rv;
    }
#if !APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_reg32_write(phy, 0x10020015, 0));
#endif    
 
    PHYMOD_MEMCPY(val, mem_access.pm_mem_data, (((len*2)/4) * sizeof(uint32_t)));
    if ((len*2) % 4) {
        val[(len*2)/4] = 0;
        for(index = 0; index < ((len*2) %4); index ++) {
            val[(len*2)/4] |= (mem_access.pm_mem_data[(len*2)/4] & (0xFF << index*8));
        }
    }

    return PHYMOD_E_NONE;
}

int
plp_aperta_mem_write(const plp_aperta_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data)
{
    APERTA_PM_MEM_ACCESS_T mem_access;
    int len = 0, rv = 0, index = 0;

    PHYMOD_MEMSET(&mem_access, 0, sizeof(APERTA_PM_MEM_ACCESS_T));
#if !APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
          plp_aperta_reg32_write(phy, 0x10020015, 1));
#endif 
    mem_access.pm_mem_sel = (phy->port_loc ==phymodPortLocLine) ? APERTA_LINE_SIDE_PM : APERTA_SYS_SIDE_PM ;
    mem_access.pm_mem_type = mem_type;
    mem_access.pm_mem_addr = mem_index;
    mem_access.pm_mem_rw = 1; /* 0 - Read, 1 - Write*/
    plp_aperta_prg_mem_access_prog_val_len(mem_type, &len);
    PHYMOD_MEMCPY(mem_access.pm_mem_data, data, (((len*2)/4) * sizeof(uint32_t)));
    if ((len*2) % 4) {
        for(index = 0; index < ((len*2) %4); index ++) {
            mem_access.pm_mem_data[(len*2)/4] |= (data[(len*2)/4] & (0xFF << index*8));
        }
    }
    rv = plp_aperta_pm_tsc_mem_access(phy, &mem_access);
    if (rv != PHYMOD_E_NONE) {
#if !APERTA_FW_READ_WRITE
        PHYMOD_IF_ERR_RETURN(
              plp_aperta_reg32_write(phy, 0x10020015, 0));
#endif        
        return rv;
    }
#if !APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
      plp_aperta_reg32_write(phy, 0x10020015, 0));
#endif    
 
 
    return PHYMOD_E_NONE;
}


int plp_aperta_reg32_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data)
{
    uint64_t reg_data;
    int speed = 0, lane_data_rate = 0, port_sel = 0, lane_sel = 0;
    int pll_sel = 0, micro_sel = 0;

    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PLL_MICRO_SEL(phy, pll_sel, micro_sel);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port_sel, lane_sel);
    if (lane_sel == 0xff || lane_sel == 0xF) {
        lane_sel = 1;
    } else {
        lane_sel = lane_sel & ~(lane_sel-1);
    }

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg_read(phy, port_sel, lane_sel,
                 pll_sel, micro_sel, reg_addr, &reg_data));
    COMPILER_64_TO_32_LO (*data, reg_data);

    return PHYMOD_E_NONE;
}

int plp_aperta_reg32_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data)
{
    uint64_t reg_data;
    int speed = 0, lane_data_rate = 0, port_sel = 0, lane_sel = 0;
    int pll_sel = 0, micro_sel = 0;

    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PLL_MICRO_SEL(phy, pll_sel, micro_sel);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port_sel, lane_sel);
    COMPILER_64_SET(reg_data, 0, data);
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_register_write(phy, port_sel, lane_sel,
                 pll_sel, micro_sel, reg_addr, reg_data, 0));

    return PHYMOD_E_NONE;
}

int plp_aperta_reg64_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint64_t *data)
{
    int speed = 0, lane_data_rate = 0, port_sel = 0, lane_sel = 0;
    int pll_sel = 0, micro_sel = 0;

    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PLL_MICRO_SEL(phy, pll_sel, micro_sel);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port_sel, lane_sel);
    if (lane_sel == 0xff || lane_sel == 0xF) {
        lane_sel = 1;
    } else {
        lane_sel = lane_sel & ~(lane_sel-1);
    }

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_reg_read(phy, port_sel, lane_sel,
                 pll_sel, micro_sel, reg_addr, data));

    return PHYMOD_E_NONE;
}

int plp_aperta_reg64_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint64_t data)
{
    int speed = 0, lane_data_rate = 0, port_sel = 0, lane_sel = 0;
    int pll_sel = 0, micro_sel = 0;

    APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA_GET_PLL_MICRO_SEL(phy, pll_sel, micro_sel);
    APERTA_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, port_sel, lane_sel);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_register_write(phy, port_sel, lane_sel,
                 pll_sel, micro_sel, reg_addr, data, 0));

    return PHYMOD_E_NONE;
}

/*  Register Write task - High Level Task*/
int plp_aperta_register_write(const plp_aperta_phymod_phy_access_t* phy, int port_number, int lane_sel,
                          int pll_sel, int micro_sel, int reg_addr,
                          uint64_t reg_data, int reg_mask) {
    uint64_t tmp_data, temp_reg_mask;
    uint32_t tmp_data_32 = 0;
    APERTA_SLAVE_TYPE_T lmi_slave;
    int data_width;
    int port_sel = 0;
    uint64_t lmi_reg_data[1];
    int speed = 0, lane_data_rate = 0;

    /* Get LMI Slave*/
    lmi_slave = plp_aperta_get_lmi_slave(reg_addr);

    /*Clean up unused upper bits of reg_mask*/
    data_width  = plp_aperta_get_lmi_slave_data_length(reg_addr);
    reg_mask   &= (((int) (1) << data_width) - 1);
    COMPILER_64_SET(temp_reg_mask, 0, reg_mask);

    /* Check Port Selection for writting*/
    if (port_number >= APERTA_MAX_PORT) {
        PHYMOD_DEBUG_ERROR(("reg_wr_task :: Invalid Port Selection: %0d. num_of_ports=%0d", port_number, APERTA_MAX_PORT));
        return PHYMOD_E_PARAM;
    }

    port_sel = port_number;
    /* Calculate port_sel and lane_sel based on Port number*/
    switch (lmi_slave) {
        case APERTA_PM_PORT:
        case APERTA_PM_MAC:
        case APERTA_PM_TSC:
            /* Set Port_lane_sel to the port number inside the die
            port_sel = port_number;*/
            /* Set Port_lane_sel select all lanes of given port
            lane_sel = lane_sel;*/
        break;
        case APERTA_EIP218_EGRESS:
        case APERTA_EIP218_INGRESS:
        {
            reg_addr = (reg_addr & ~(0xF << 8)) | (port_sel << 8);
        }
        break;
        case APERTA_CHIP_RDB:
        {
            /* Special case for PTP registers*/
            if (((reg_addr >> 12) == ( APERTA_P1588_BASEADR >> 12))) {
                /* set reg_addr[11:8] with port offset
                   reg_addr[11:8] = port_sel;*/
                APERTA_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
                (void) lane_data_rate;
                if (speed == 400000) {
                    reg_addr = (reg_addr | (0xF << 8));
                } else {
                    reg_addr = (reg_addr & ~(0xF << 8)) | (port_sel << 8);
                }
            }
            else if (((reg_addr >> 12) == (APERTA_EGR_SF_BASEADR >> 12)) ||
                     ((reg_addr >> 12) == (APERTA_ING_FC_BASEADR >> 12)) ||
                     ((reg_addr >> 12) == (APERTA_EGR_FC_BASEADR >> 12))  ) {
                if (((reg_addr >> 11) & 1) == 0) {
                    reg_addr = (reg_addr & ~(0xF << 8)) | (port_sel << 8);
                } else {
                    port_sel = 0; /* Use Port 0 by default*/
                }
            } else {
                /* Use Port 0 by default*/
                port_sel = 0;
            }
        }
        break;
        default:
           port_sel = 0;
           break;
    } /* case (lmi_slave)*/
    if (lmi_slave == APERTA_SLAVE_NONE) {
        /* Chip top Direct Register Access*/
        if (reg_mask == 0) {
            /* Write register*/
            COMPILER_64_TO_32_LO(tmp_data_32, reg_data);
            PHYMOD_IF_ERR_RETURN(
                plp_aperta_direct_reg_write(phy, reg_addr, tmp_data_32));
        } else {
            int reg_data_32 = 0;
            /* read modify write - Chip top Direct Register Access*/
            PHYMOD_IF_ERR_RETURN(
               plp_aperta_direct_reg_read(phy, reg_addr, &tmp_data_32));
            /* Apply Mask*/
            tmp_data_32 = tmp_data_32 & reg_mask;
            COMPILER_64_TO_32_LO(reg_data_32, reg_data);
            tmp_data_32 = tmp_data_32 | ((~reg_mask) & reg_data_32);
            /* Write back*/
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(
                                  phy, reg_addr, tmp_data_32));
        }
    } else if (lmi_slave == APERTA_PM_TSC) {
        /* LMI Indirect TSC Register Write - using reg_mask*/
        lmi_reg_data[0] = reg_data;
        PHYMOD_IF_ERR_RETURN(plp_aperta_indirect_reg_access(phy,
                            port_sel, lane_sel, pll_sel, micro_sel,
                            reg_addr, lmi_reg_data, &temp_reg_mask,
                            1, 1));
    } else {
        /* LMI Indirect Register Write*/
        if (reg_mask == 0) {
            lmi_reg_data[0] = reg_data;
            PHYMOD_IF_ERR_RETURN(plp_aperta_indirect_reg_access(phy,
                                port_sel, lane_sel, pll_sel, micro_sel,
                                reg_addr, lmi_reg_data, &temp_reg_mask,
                                1, 1));
        } else {
            int reg_data_32 = 0;
            /* LMI Indirect Reg Read*/
            PHYMOD_IF_ERR_RETURN(plp_aperta_indirect_reg_access(phy,
                                port_sel, lane_sel, pll_sel, micro_sel,
                                reg_addr, lmi_reg_data, &temp_reg_mask,
                                1, 0));
            tmp_data = lmi_reg_data[0];
            /* Apply Mask*/
            COMPILER_64_AND(tmp_data, temp_reg_mask); 
			COMPILER_64_TO_32_LO(reg_data_32, reg_data);
            COMPILER_64_SET(temp_reg_mask,0, (((~reg_mask) & reg_data_32))); 
            COMPILER_64_OR(tmp_data, temp_reg_mask); 
    	    
            /* LMI Indirect Reg Write*/
            lmi_reg_data[0] = tmp_data;
            PHYMOD_IF_ERR_RETURN(plp_aperta_indirect_reg_access(phy,
                                  port_sel, lane_sel, pll_sel, micro_sel,
                                  reg_addr, lmi_reg_data, 0,
                                  1, 1));
        }
    }
    return PHYMOD_E_NONE;
}

/* Write Register*/
int plp_aperta_direct_reg_write(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t reg_data)
{
    PHYMOD_IF_ERR_RETURN(
       PHYMOD_BUS_WRITE(&phy->access, ((1<<16) | (reg_addr & 0xFFFF)), reg_data));

    return PHYMOD_E_NONE;
}

/* Read Register - Low Level Task*/
int plp_aperta_direct_reg_read(const plp_aperta_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t *reg_data) {
    PHYMOD_IF_ERR_RETURN(
       PHYMOD_BUS_READ(&phy->access, ((1<<16) | (reg_addr & 0xFFFF)), reg_data));

    return PHYMOD_E_NONE;
}

int plp_aperta_reg_read(const plp_aperta_phymod_phy_access_t* phy, int port_number, int lane_sel,
                   int pll_sel, int micro_sel,
                   uint32_t reg_addr, uint64_t *reg_data) {

    APERTA_SLAVE_TYPE_T    lmi_slave;
    int port_sel = 0;

    /* Get LMI Slave*/
    lmi_slave = plp_aperta_get_lmi_slave(reg_addr);

    /* Check Port Selection for read*/
    if (port_number >= APERTA_MAX_PORT) {
        PHYMOD_DEBUG_ERROR(("read_rd_task Invalid Port Selection: %0d. num_of_ports=%0d", port_number, APERTA_MAX_PORT));
        return PHYMOD_E_PARAM;
    }
    port_sel = port_number;
    /* Calculate port_sel and lane_sel*/
    switch (lmi_slave) {
        case APERTA_PM_PORT:
        case APERTA_PM_MAC:
        case APERTA_PM_TSC:
          /*if (lmi_slave == APERTA_PM_TSC && lane_sel == LANE_NONE) {
              lane_sel = (1<<port_sel);
          }*/
        break;
        case APERTA_EIP218_EGRESS:
        case APERTA_EIP218_INGRESS:
             reg_addr = (reg_addr & ~(0xF << 8)) | (port_sel << 8);
        break;
        case  APERTA_CHIP_RDB:
            /* Special case for PTP registers*/
            if ((reg_addr>>12) == (APERTA_P1588_BASEADR>>12)) {
              /* set reg_addr[11:8] with port offset*/
              reg_addr &= (~(0xF << 8));
              reg_addr |= ((port_sel & 0x000F) << 8);
            } else if (((reg_addr >> 12) == (APERTA_EGR_SF_BASEADR >> 12)) ||
                       ((reg_addr >> 12) == (APERTA_ING_FC_BASEADR >> 12)) ||
                       ((reg_addr >> 12) == (APERTA_EGR_FC_BASEADR >> 12))
                       ) {
                if (((reg_addr >> 11) & 0x1) == 0) {
                    /* reg_addr[11] == 1'b0 -> Per Port register*/
                    reg_addr = (reg_addr & ~(0xF << 8)) | (port_sel << 8);
                } else {
                    port_sel = 0; /* Use Port 0 by default*/
                }
            } else {
                /* Use Port 0 by default*/
                port_sel = 0;
            }
        break;
        default:
            port_sel = 0;
            break;
    }
    /* do the register access*/
    if (lmi_slave == APERTA_SLAVE_NONE) {
        uint32_t dir_read = 0;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_direct_reg_read(phy, reg_addr, &dir_read));
        COMPILER_64_SET(*reg_data, 0, dir_read);
    } else {
        uint64_t lmi_reg_data[1];
        uint64_t mask;
        COMPILER_64_ZERO(mask);

        PHYMOD_IF_ERR_RETURN(
            plp_aperta_indirect_reg_access(phy,
                          port_sel, lane_sel, pll_sel, micro_sel,
                          reg_addr, lmi_reg_data, &mask,
                          1, 0));
        *reg_data = lmi_reg_data[0];
    }
    return PHYMOD_E_NONE;
}

/* Read or Write Indirect Register */
int plp_aperta_indirect_reg_access(const plp_aperta_phymod_phy_access_t* phy,
                         uint16_t port_sel, uint16_t lane_sel, int pll_sel, int micro_sel,
                         uint32_t reg_addr, uint64_t* reg_data, uint64_t* tsc_data_mask,
                         int num_words, int write_en)
{
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_lmi_reg_access(phy, port_sel, lane_sel, pll_sel, micro_sel,
                 reg_addr, reg_data, tsc_data_mask,
                 num_words, write_en));

    return PHYMOD_E_NONE;
}

/* Read or Write Indirect Register through LMI*/
int plp_aperta_lmi_reg_access(const plp_aperta_phymod_phy_access_t* phy,
                    uint16_t port_sel, uint16_t lane_sel, int pll_sel, int micro_sel,
                    uint32_t reg_addr, uint64_t* reg_data, uint64_t *tsc_data_mask,
                    int num_words, int write_en)
{

    BCMI_APERTA_D_LMI_LMI_CMDr_t     lmi_cmd ;
    BCMI_APERTA_D_LMI_LMI_CMD_EXTr_t lmi_cmd_ext;
    BCMI_APERTA_D_LMI_LMI_STATUSr_t  lmi_sts;
    BCMI_APERTA_D_LMI_LMI_CMD_SEQr_t lmi_cmd_seq;
    APERTA_TSC_ADDR_TYPE_T           tsc_addr;
    APERTA_PMD_ADDR_TYPE_T           pmd_addr;
    APERTA_SLAVE_TYPE_T              lmi_slave;
    int slave_data_size = 0;
    int done = 0, retry_cnt = APERTA_LMI_RETRY_CNT;
    int word_count = 0, slave_data_count = 0;
    int device_type = 0;
    int reg_mask_32 = 0;
#if IS_LITTLE_ENDIAN
    uint64_t temp_data_64;
#endif
#if APERTA_FW_READ_WRITE
    uint32_t fw_dloaded = 0;
    int pcs_pmd_addr = 0;
#endif    

    COMPILER_64_SET(*tsc_data_mask, 0, 0);
    COMPILER_64_TO_32_LO(reg_mask_32, *tsc_data_mask);
    /* Get Lmi Slave*/
    lmi_slave = plp_aperta_get_lmi_slave(reg_addr);

#if APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_is_fw_dloaded_get(phy->access.addr, &fw_dloaded));
#endif    
    /* Compose LMI command*/
    PHYMOD_MEMSET(&lmi_cmd, 0, sizeof(BCMI_APERTA_D_LMI_LMI_CMDr_t));
    PHYMOD_MEMSET(&lmi_cmd_ext, 0, sizeof(BCMI_APERTA_D_LMI_LMI_CMD_EXTr_t));
    PHYMOD_MEMSET(&lmi_sts, 0, sizeof(BCMI_APERTA_D_LMI_LMI_STATUSr_t));
    PHYMOD_MEMSET(&lmi_cmd_seq, 0, sizeof(BCMI_APERTA_D_LMI_LMI_CMD_SEQr_t));

    slave_data_size = plp_aperta_get_lmi_cmd(&lmi_cmd, phy->port_loc, port_sel, reg_addr, reg_mask_32, write_en, phy->access.lane_mask);
    slave_data_size = (slave_data_size > 64)? 64: slave_data_size;

    if (num_words > 1) {
        BCMI_APERTA_D_LMI_LMI_CMDr_USE_CMD_EXTf_SET(lmi_cmd, 1);
        BCMI_APERTA_D_LMI_LMI_CMD_EXTr_BURST_LENf_SET(lmi_cmd_ext, num_words);
        BCMI_APERTA_D_LMI_LMI_CMD_EXTr_BURST_ADR_INCRf_SET(lmi_cmd_ext, 1);
    }
    /* Compose TSC Address [31:0]*/
    if (lmi_slave == APERTA_PM_TSC) {
        tsc_addr.data = 0x0;
        pmd_addr.data = 0x0;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta_get_tsc_addr(&tsc_addr, &pmd_addr, port_sel, lane_sel, pll_sel, micro_sel, reg_addr, &device_type ));
    }
#if APERTA_LMI_REG_DUMP
#ifdef ATE_PRINT_ENABLED
	if (write_en) {
        PHYMOD_DEBUG_INFO(("ATE PLP_LMI_WRITE Complete: LMI_SLAVE %s Phy:%d %s port=0x%0x lane=0x%x (addr=0x%x) data=0x%llx num_words=%d\n",
           ((lmi_slave ==0) ? "APERTA_SLAVE_NONE" : (lmi_slave==1) ? "APERTA_PM_PORT" : (lmi_slave==2)?"APERTA_PM_MAC":(lmi_slave==3)?"APERTA_PM_TSC":(lmi_slave==4)?"APERTA_EIP163_INGRESS":
		   (lmi_slave==5)?"APERTA_EIP164_INGRESS":(lmi_slave==6)?"APERTA_EIP218_INGRESS":(lmi_slave==7)?"APERTA_CHIP_RDB":(lmi_slave==8)?"APERTA_EIP163_EGRESS":(lmi_slave==9)?"APERTA_EIP164_EGRESS":
		   "APERTA_EIP218_EGRESS"),phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel, 
           reg_addr, (uint64_t)*reg_data, num_words));
    } else{
        PHYMOD_DEBUG_INFO(("ATE PLP_LMI_READ Complete: LMI_SLAVE %s Phy:%d %s port=0x%x lane=0x%x (addr=0x%x) num_words=%d data:0x%llx\n",
           ((lmi_slave ==0) ? "APERTA_SLAVE_NONE" : (lmi_slave==1) ? "APERTA_PM_PORT" : (lmi_slave==2)?"APERTA_PM_MAC":(lmi_slave==3)?"APERTA_PM_TSC":(lmi_slave==4)?"APERTA_EIP163_INGRESS":
		   (lmi_slave==5)?"APERTA_EIP164_INGRESS":(lmi_slave==6)?"APERTA_EIP218_INGRESS":(lmi_slave==7)?"APERTA_CHIP_RDB":(lmi_slave==8)?"APERTA_EIP163_EGRESS":(lmi_slave==9)?"APERTA_EIP164_EGRESS":
		   "APERTA_EIP218_EGRESS"),phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel, 
           reg_addr, num_words, *reg_data));
    }
#else
    if (write_en) {
        PHYMOD_DEBUG_INFO(("LMI_WRITE Starting: Phy:%d %s port=%0x lane=%0x (addr=%0x) data=%llx num_words=%0d\n",
           phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel,
           reg_addr, (uint64_t)*reg_data, num_words));
    } else {
        PHYMOD_DEBUG_INFO(("LMI_READ Starting: Phy:%d %s port=%0x lane=%0x (addr=%0x) num_words=%0d\n",
           phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel,
           reg_addr, num_words));
    }
#endif
#endif
#if APERTA_FW_READ_WRITE
   if (fw_dloaded == 1) {
       if (lmi_slave == APERTA_PM_TSC) {
           if (device_type == APERTA_TSC) { 
               pcs_pmd_addr = tsc_addr.data; 
           } else if (device_type == APERTA_PMD) {
               pcs_pmd_addr = pmd_addr.data;
           } else {
               pcs_pmd_addr = 0;
           }
       }
       PHYMOD_IF_ERR_RETURN(
               plp_aperta_fw_reg_access(phy, port_sel, lane_sel, reg_addr, reg_data, write_en, pcs_pmd_addr));
    } else {
#endif        
        PHYMOD_DEBUG_INFO(("LMI ACCESS : Before FW download !!!!\n"));
        done = 0;
        /* Wait for done==0 */
        do {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_LMI_LMI_STATUSr(&phy->access, &lmi_sts));
            done = BCMI_APERTA_D_LMI_LMI_STATUSr_DONEf_GET(lmi_sts);
            PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
        } while ((done != 0) && --retry_cnt);
        if (retry_cnt <= 0) {
            PHYMOD_DEBUG_ERROR((" LMI Access timeout(waiting for done==0.1): phy:%d\n", phy->access.addr));
            return PHYMOD_E_TIMEOUT;
        }
        retry_cnt = APERTA_LMI_RETRY_CNT;
        BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, lmi_cmd.v[0]);
        PHYMOD_IF_ERR_RETURN(
              BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));

#if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
    PHYMOD_DIAG_OUT(("CMD:%x\n", lmi_cmd_seq.v[0]));
#endif
#endif
        /* Write CMD_EXT*/
        if (BCMI_APERTA_D_LMI_LMI_CMDr_USE_CMD_EXTf_GET(lmi_cmd)) {
            BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, lmi_cmd_ext.v[0]);
            PHYMOD_IF_ERR_RETURN(
                 BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
        }
        /* Write Data*/
        if (write_en) {
        /* Write TSC DATA Mask*/
            if ((lmi_slave == APERTA_PM_TSC) && (reg_mask_32 != 0)) {
                BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, reg_mask_32);
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                    PHYMOD_DIAG_OUT(("CMD:%x\n", lmi_cmd_seq.v[0]));
                #endif
#endif
            }
            for (word_count=0; word_count < num_words; word_count ++) {
                for (slave_data_count = 0; slave_data_count < (slave_data_size/16); slave_data_count ++) {
                    uint16_t data;
                    #if IS_LITTLE_ENDIAN
                        COMPILER_64_SET(temp_data_64 , 0, (slave_data_count ? 16 : 0));
                        data = (COMPILER_64_SHR(*reg_data, temp_data_64) & 0xFFFF); 
                        /*data = ((uint64_t)(*reg_data) >> ( 16 * slave_data_count)) & 0xFFFF;*/
                    #endif
                    #if IS_BIG_ENDIAN
                        data = ((uint64_t)(*reg_data) >> (16*( 3 - slave_data_count))) & 0xFFFF;
                    #endif
                    BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, data);
                    PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                    PHYMOD_DIAG_OUT(("CMD:%x\n", lmi_cmd_seq.v[0]));
                #endif
#endif
                }
            }
        }
        /* Write Address*/
        if (lmi_slave == APERTA_PM_TSC) {
            if (device_type == APERTA_TSC) {
                  BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, (tsc_addr.data & 0xFFFF));
                  PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                  #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                  PHYMOD_DIAG_OUT(("Addr:%x\n", lmi_cmd_seq.v[0]));
                  #endif
#endif
                  BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, ((tsc_addr.data >> 16) & 0xFFFF));
                  PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                  #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                  PHYMOD_DIAG_OUT(("Addr:%x\n", lmi_cmd_seq.v[0]));
#endif
                  #endif
            } else { /*PMD*/
                BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, (pmd_addr.data & 0xFFFF));
                PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                PHYMOD_DIAG_OUT(("Addr:%x\n", lmi_cmd_seq.v[0]));
#endif
                #endif
                  BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, ((pmd_addr.data >> 16) & 0xFFFF));
                  PHYMOD_IF_ERR_RETURN(
                        BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                PHYMOD_DIAG_OUT(("Addr:%x\n", lmi_cmd_seq.v[0]));
#endif
                #endif
            }
        } else {
            BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, (reg_addr & 0xFFFF));
            PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
            #if  APERTA_LMI_REG_DUMP
#ifndef ATE_PRINT_ENABLED
                PHYMOD_DIAG_OUT(("CMD:%x\n", lmi_cmd_seq.v[0]));
#endif
            #endif
        }

        do {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_LMI_LMI_STATUSr(&phy->access, &lmi_sts));
            done = BCMI_APERTA_D_LMI_LMI_STATUSr_DONEf_GET(lmi_sts);
            PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
        } while ((done != 1) && --retry_cnt);
        if (retry_cnt <= 0) {
            PHYMOD_DEBUG_ERROR((" LMI Access timeout(waiting for done==1.2): phy:%d addr:%x\n", phy->access.addr, reg_addr));
            return PHYMOD_E_TIMEOUT;
        }
        if (BCMI_APERTA_D_LMI_LMI_STATUSr_ACCESS_ERRORf_GET(lmi_sts) ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_DED_ERRf_GET(lmi_sts) ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_WRONG_SLAVE_SELf_GET(lmi_sts) ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_UNDERRUNf_GET(lmi_sts)  ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_OVERRUNf_GET(lmi_sts) ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_ERRf_GET(lmi_sts) ||
              BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_TIME_OUTf_GET(lmi_sts)) {
             PHYMOD_DEBUG_ERROR((" LMI Access Error:%x: phy:%d addr:%x side:%d\n", lmi_sts.v[0],phy->access.addr,
                         reg_addr, phy->port_loc));
             return PHYMOD_E_INTERNAL;
        }
        /* Collect Read Data*/
        if (!write_en) {
            for (word_count = 0; word_count < num_words; word_count ++) {
                COMPILER_64_ZERO(*reg_data);
                for (slave_data_count = 0; slave_data_count < slave_data_size/16; slave_data_count ++) {
                    uint16_t tmp_data;

                    PHYMOD_IF_ERR_RETURN (
                            BCMI_APERTA_D_READ_LMI_LMI_CMD_SEQr(&phy->access, &lmi_cmd_seq));
                      tmp_data = BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_GET(lmi_cmd_seq);
                    #if IS_LITTLE_ENDIAN
                        if (slave_data_count < 2) {
                            COMPILER_64_SET(temp_data_64 , 0, ((tmp_data & 0xFFFF) << (16*slave_data_count)));
                        } else {
                            COMPILER_64_SET(temp_data_64 , ((tmp_data & 0xFFFF) << (16*(slave_data_count-2))), 0);
                        }
                        COMPILER_64_OR(*reg_data, temp_data_64); 
                        /**reg_data |= (((uint64_t)tmp_data & 0xFFFF) << (16*slave_data_count));*/
                    #endif
                    #if IS_BIG_ENDIAN
                        *reg_data |= (((uint64_t)tmp_data & 0xFFFF) << (16*(3-slave_data_count)));
                    #endif
                }
            }
        }
#if APERTA_FW_READ_WRITE
    }
#endif   
#if APERTA_LMI_REG_DUMP
#ifdef ATE_PRINT_ENABLED
	if (write_en) {
        PHYMOD_DEBUG_INFO(("ATE PLP_LMI_WRITE Complete: LMI_SLAVE %s Phy:%d %s port=0x%0x lane=0x%x (addr=0x%x) data=0x%llx num_words=%d\n",
           ((lmi_slave ==0) ? "APERTA_SLAVE_NONE" : (lmi_slave==1) ? "APERTA_PM_PORT" : (lmi_slave==2)?"APERTA_PM_MAC":(lmi_slave==3)?"APERTA_PM_TSC":(lmi_slave==4)?"APERTA_EIP163_INGRESS":
		   (lmi_slave==5)?"APERTA_EIP164_INGRESS":(lmi_slave==6)?"APERTA_EIP218_INGRESS":(lmi_slave==7)?"APERTA_CHIP_RDB":(lmi_slave==8)?"APERTA_EIP163_EGRESS":(lmi_slave==9)?"APERTA_EIP164_EGRESS":
		   "APERTA_EIP218_EGRESS"),phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel, 
           reg_addr, (uint64_t)*reg_data, num_words));
    } else{
        PHYMOD_DEBUG_INFO(("ATE PLP_LMI_READ Complete: LMI_SLAVE %s Phy:%d %s port=0x%x lane=0x%x (addr=0x%x) num_words=%d data:0x%llx\n",
           ((lmi_slave ==0) ? "APERTA_SLAVE_NONE" : (lmi_slave==1) ? "APERTA_PM_PORT" : (lmi_slave==2)?"APERTA_PM_MAC":(lmi_slave==3)?"APERTA_PM_TSC":(lmi_slave==4)?"APERTA_EIP163_INGRESS":
		   (lmi_slave==5)?"APERTA_EIP164_INGRESS":(lmi_slave==6)?"APERTA_EIP218_INGRESS":(lmi_slave==7)?"APERTA_CHIP_RDB":(lmi_slave==8)?"APERTA_EIP163_EGRESS":(lmi_slave==9)?"APERTA_EIP164_EGRESS":
		   "APERTA_EIP218_EGRESS"),phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel, 
           reg_addr, num_words, *reg_data));
    }
#else

    if (write_en) {
        PHYMOD_DEBUG_INFO(("LMI_WRITE Complete: Phy:%d %s port=%0x lane=%x (addr=%x) data=%llx num_words=%d\n",
           phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel,
           reg_addr, (uint64_t)*reg_data, num_words));
    } else{
        PHYMOD_DEBUG_INFO(("LMI_READ Complete: Phy:%d %s port=%x lane=%x (addr=%x) num_words=%d data:%llx\n",
           phy->access.addr, (phy->port_loc == phymodPortLocLine) ? "LINE": "SYS" , port_sel, lane_sel,
           reg_addr, num_words, *reg_data));
    }
#endif
#endif
    return PHYMOD_E_NONE;
}

/* Get Lmi Slave type*/
APERTA_SLAVE_TYPE_T plp_aperta_get_lmi_slave(int reg_addr)
{
    APERTA_SLAVE_TYPE_T slave;
    switch (reg_addr >> 24) {
        case (APERTA_PM_CDPORT_16B_BASEADR      >> 24):  { slave = APERTA_PM_PORT; break; }
        case (APERTA_PM_CDPORT_32B_BASEADR      >> 24):  { slave = APERTA_PM_PORT; break; }
        case (APERTA_PM_CDPORT_48B_BASEADR      >> 24):  { slave = APERTA_PM_PORT; break; }
        case (APERTA_PM_CDPORT_64B_BASEADR      >> 24):  { slave = APERTA_PM_PORT; break; }
        case (APERTA_PM_CDMAC_16B_BASEADR       >> 24):  { slave = APERTA_PM_MAC; break; }
        case (APERTA_PM_CDMAC_32B_BASEADR       >> 24):  { slave = APERTA_PM_MAC; break; }
        case (APERTA_PM_CDMAC_48B_BASEADR       >> 24):  { slave = APERTA_PM_MAC; break; }
        case (APERTA_PM_CDMAC_64B_BASEADR       >> 24):  { slave = APERTA_PM_MAC; break; }
        case (APERTA_PM_TSC_BLACKHAWK_BASEADR   >> 24):  { slave = APERTA_PM_TSC; break; }
        case (APERTA_EIP163_INGRESS_BASEADR     >> 24):  { slave = APERTA_EIP163_INGRESS; break; }
        case (APERTA_EIP164_INGRESS_BASEADR     >> 24):  { slave = APERTA_EIP164_INGRESS; break; }
        case (APERTA_EIP218_INGRESS_BASEADR     >> 24):  { slave = APERTA_EIP218_INGRESS; break; }
        case (APERTA_EIP163_EGRESS_BASEADR      >> 24):  { slave = APERTA_EIP163_EGRESS; break; }
        case (APERTA_EIP164_EGRESS_BASEADR      >> 24):  { slave = APERTA_EIP164_EGRESS; break; }
        case (APERTA_EIP218_EGRESS_BASEADR      >> 24):  { slave = APERTA_EIP218_EGRESS; break; }
        case (APERTA_CHIP_IND_BASEADR           >> 24):  { slave = APERTA_CHIP_RDB; break; }
        default:
            slave = APERTA_SLAVE_NONE;
            if ((reg_addr >> 24) != 1) {
                PHYMOD_DEBUG_INFO(("######Caution invalid slave:0x%x\n", reg_addr));
            }
            break;
    }
    return(slave);
}

/* Get Lmi Data Length based on base address*/
int plp_aperta_get_lmi_slave_data_length(int reg_addr)
{
    int data_length = 0;
    switch (reg_addr >> 24) {
        case (APERTA_PM_CDPORT_16B_BASEADR      >> 24): { data_length = 32; break; }
        case (APERTA_PM_CDPORT_32B_BASEADR      >> 24): { data_length = 32; break; }
        case (APERTA_PM_CDPORT_48B_BASEADR      >> 24): { data_length = 48; break; }
        case (APERTA_PM_CDPORT_64B_BASEADR      >> 24): { data_length = 64; break; }
        case (APERTA_PM_CDMAC_16B_BASEADR       >> 24): { data_length = 32; break; }
        case (APERTA_PM_CDMAC_32B_BASEADR       >> 24): { data_length = 32; break; }
        case (APERTA_PM_CDMAC_48B_BASEADR       >> 24): { data_length = 48; break; }
        case (APERTA_PM_CDMAC_64B_BASEADR       >> 24): { data_length = 64; break; }
        case (APERTA_PM_TSC_BLACKHAWK_BASEADR   >> 24): { data_length = 16; break; }
        case (APERTA_EIP163_INGRESS_BASEADR     >> 24): { data_length = 32; break; }
        case (APERTA_EIP164_INGRESS_BASEADR     >> 24): { data_length = 32; break; }
        case (APERTA_EIP218_INGRESS_BASEADR     >> 24): { data_length = 32; break; }
        case (APERTA_EIP163_EGRESS_BASEADR      >> 24): { data_length = 32; break; }
        case (APERTA_EIP164_EGRESS_BASEADR      >> 24): { data_length = 32; break; }
        case (APERTA_EIP218_EGRESS_BASEADR      >> 24): { data_length = 32; break; }
        case (APERTA_CHIP_IND_BASEADR           >> 24): { data_length = 16; break; }
        default:
            data_length = 16;
            break;
    }
    return (data_length);
}

/* Get LMI Cmd */
int plp_aperta_get_lmi_cmd(BCMI_APERTA_D_LMI_LMI_CMDr_t* lmi_cmd, int side, int port_sel,
                 int reg_addr, int reg_mask, int write_en, uint32_t lane_mask) {
    int mac = 0, lmi_data_size = 0;
    APERTA_SLAVE_TYPE_T slave;

    /* Get Slave type*/
    slave = plp_aperta_get_lmi_slave(reg_addr);

    /* Get data size for LMI Transaction*/
    lmi_data_size = plp_aperta_get_lmi_slave_data_length(reg_addr);

    /* Get MAC*/
    if (lane_mask & 0xf) {
        mac = 0;
    } else {
        mac = 1;
    }

    /* Compose LMI cmd*/
    BCMI_APERTA_D_LMI_LMI_CMDr_SLAVE_SELf_SET(*lmi_cmd, plp_aperta_get_lmi_slave_sel(slave, side, write_en));
    BCMI_APERTA_D_LMI_LMI_CMDr_ACCESS_PROGf_SET(*lmi_cmd, plp_aperta_get_lmi_access_prog(reg_addr, mac, lmi_data_size, reg_mask));
    BCMI_APERTA_D_LMI_LMI_CMDr_PORT_SELf_SET(*lmi_cmd, port_sel);
    BCMI_APERTA_D_LMI_LMI_CMDr_WRITE_ENf_SET(*lmi_cmd, write_en);
    BCMI_APERTA_D_LMI_LMI_CMDr_USE_CMD_EXTf_SET(*lmi_cmd, 0);

    return lmi_data_size;
}

/*Get Lmi Slave Sel*/
int plp_aperta_get_lmi_slave_sel(APERTA_SLAVE_TYPE_T slave, plp_aperta_phymod_port_loc_t port_loc, int write_en)
{
    int slave_sel = 0;
    switch (slave) {
        case APERTA_CHIP_RDB:       slave_sel = 0x6; break;
        case APERTA_EIP163_INGRESS: slave_sel = 0x2; break;
        case APERTA_EIP164_INGRESS: slave_sel = 0x3; break;
        case APERTA_EIP218_INGRESS: slave_sel = 0x4; break;
        case APERTA_EIP163_EGRESS: slave_sel = 0xA; break;
        case APERTA_EIP164_EGRESS: slave_sel = 0xB; break;
        case APERTA_EIP218_EGRESS: slave_sel = 0xC; break;
        case APERTA_PM_PORT:
        case APERTA_PM_MAC:
        case APERTA_PM_TSC:
            if (port_loc == phymodPortLocLine) {
                slave_sel = 0x1;
            } else if (port_loc == phymodPortLocSys) {
                slave_sel = 0x9;
            } else {
                if (write_en) {
                slave_sel = 0xF;
                } else {
                    slave_sel = 0x1;
                }
            }
            break;
        default: slave_sel = 0x0;
            break;
    }
    return(slave_sel);
}

/* Get Lmi access_prog*/
int plp_aperta_get_lmi_access_prog(int addr, int mac, int data_length, int reg_mask)
{
    APERTA_SLAVE_TYPE_T slave;
    int access_type = 0;
    int stage_id    = 0;
    int access_prog = 0;
    int per_port;

    slave = plp_aperta_get_lmi_slave(addr);
    /* Getting access type*/
    switch (slave) {
        case APERTA_PM_PORT:
        case APERTA_PM_MAC:
          per_port = (addr & (1<<17))? 0 : 1;
          if (per_port==0 && data_length==32) { access_type = 0;
          } else if (per_port==1 && data_length==32) { access_type = 1;
          } else if (per_port==0 && data_length==48) { access_type = 2;
          } else if (per_port==1 && data_length==48) { access_type = 3;
          } else if (per_port==0 && data_length==64) { access_type = 4;
          } else if (per_port==1 && data_length==64) { access_type = 5;
          }
        break;
        case APERTA_PM_TSC: access_type = 6; break;
        case APERTA_EIP163_INGRESS:
        case APERTA_EIP163_EGRESS:
        case APERTA_EIP164_INGRESS:
        case APERTA_EIP164_EGRESS:
        case APERTA_EIP218_INGRESS:
        case APERTA_EIP218_EGRESS:
        {
            /*  [3:1] - MSB address [18:16] of EIP164*/
            access_type = ((addr>>16) & 7);
        }
        break;
        default:
            access_type = 0;
            break;
    }
    /* Getting stage_id*/
    switch (slave) {
        case APERTA_PM_PORT: stage_id = 0; break;
        case APERTA_PM_MAC: {
            if (mac == 0) { stage_id = 1;
            } else if (mac == 1) { stage_id = 2;
            }
        }
        break;
        case APERTA_PM_TSC:
        {
          stage_id = (reg_mask == 0)? 0 : 1;
        }
        break;
        default:
            stage_id = 0;
            break;

    }
    access_prog  = access_type & 7;
    access_prog |= ((stage_id & 0xF) << 3);

    return (access_prog);
}

int plp_aperta_get_tsc_addr(APERTA_TSC_ADDR_TYPE_T *tsc_addr, APERTA_PMD_ADDR_TYPE_T *pmd_addr, uint16_t port_sel,
                  uint16_t lane_sel, int pll_sel, int micro_sel, uint32_t reg_addr, int *device_type)
{

    /* Get device_type*/
    if ((reg_addr >= 0x18000090) && (reg_addr <= (0x18000090 + 0xf))) {
        *device_type = APERTA_PMD; /* 0:PCS 1:PMA/PMD*/
    } else {
        *device_type = (((reg_addr>>12) & 0xF) == 0xD)? APERTA_PMD : APERTA_TSC; /* 0:PCS 1:PMA/PMD*/
    }

    /* Compose TSC Address*/
    if (*device_type == 0) {
        tsc_addr->fields.device        = *device_type;
        tsc_addr->fields.mpp1          = (lane_sel & 0xF0) ? 1 : 0;
        tsc_addr->fields.mpp0          = (lane_sel & 0x0F) ? 1 : 0;
        tsc_addr->fields.reg_addr      = reg_addr & 0xFFFF;
        if (tsc_addr->fields.mpp1 == 0 &&
                tsc_addr->fields.mpp0 == 0) {
            PHYMOD_DEBUG_ERROR(("MPP0 and MPP1 cannot be 0:lanesel:%x\n", lane_sel));
            return PHYMOD_E_PARAM;
        }

        if (lane_sel & 0xF0) {
            lane_sel = lane_sel >> 4;
        }

        /* Per Lane register:
           Translate lane_sel code to TSC port_lane_mode*/
        switch (lane_sel) {
            case 0x1:      tsc_addr->fields.lane_mode = 0x0; break; /* Single lane 0*/
            case 0x2:      tsc_addr->fields.lane_mode = 0x1; break; /* Single lane 1*/
            case 0x4:      tsc_addr->fields.lane_mode = 0x2; break; /* Single lane 2*/
            case 0x8:      tsc_addr->fields.lane_mode = 0x3; break; /* Single lane 3*/
            case 0x3:      tsc_addr->fields.lane_mode = 0x4; break; /* Dual lane: lanes 0 and 1*/
            case 0xC:      tsc_addr->fields.lane_mode = 0x5; break; /* Dual lane: lanes 2 and 3*/
            case 0xF:      tsc_addr->fields.lane_mode = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0xFF:     tsc_addr->fields.lane_mode = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0xFFFF:   tsc_addr->fields.lane_mode = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0x0:      tsc_addr->fields.lane_mode = 0x6; break; /* LANE_NONE: Select lanes 0, 1, 2, 3 when lane is don't care*/
            default:
              PHYMOD_DEBUG_ERROR(("plp_aperta_get_tsc_addr: lane_sel=%x is not supported by PortMacro TSC", lane_sel));
              return PHYMOD_E_PARAM;
        }
    }  else {   /* Compose PMD Address*/
        pmd_addr->fields.device         = *device_type;
        pmd_addr->fields.reg_addr       = reg_addr & 0xFFFF;
        pmd_addr->fields.pll_micro_sel = pll_sel;
        switch (lane_sel) {
          case 0x0 : pmd_addr->fields.lane_mode = 0x00; break; /* LANE_NONE: Select lane 0 when lane is don't care*/
          case 0x1 : pmd_addr->fields.lane_mode = 0x00; break;
          case 0x2 : pmd_addr->fields.lane_mode = 0x01; break;
          case 0x4 : pmd_addr->fields.lane_mode = 0x02; break;
          case 0x8 : pmd_addr->fields.lane_mode = 0x03; break;
          case 0x10: pmd_addr->fields.lane_mode = 0x04; break;
          case 0x20: pmd_addr->fields.lane_mode = 0x05; break;
          case 0x40: pmd_addr->fields.lane_mode = 0x06; break;
          case 0x80: pmd_addr->fields.lane_mode = 0x07; break;
          case 0x3 : pmd_addr->fields.lane_mode = 0x20; break; /* Multicast_0_1         3'b001 5'h00*/
          case 0xC : pmd_addr->fields.lane_mode = 0x21; break; /* Multicast_2_3         3'b001 5'h01*/
          case 0x30: pmd_addr->fields.lane_mode = 0x22; break; /* Multicast_4_5         3'b001 5'h02*/
          case 0xC0: pmd_addr->fields.lane_mode = 0x23; break; /* Multicast_6_7         3'b001 5'h03*/
          case 0xF : pmd_addr->fields.lane_mode = 0x40; break; /* Multicast_0_1_2_3     3'b010 5'h00*/
          case 0xF0: pmd_addr->fields.lane_mode = 0x41; break; /* Multicast_4_5_6_7     3'b010 5'h01*/
          case 0xFF: pmd_addr->fields.lane_mode = 0xFF; break; /* Broadcast All Lanes               */
          default:
            PHYMOD_DEBUG_ERROR(("plp_aperta_get_tsc_addr::lane_sel=%x is not supported by PortMacro PMD", lane_sel));
            return PHYMOD_E_PARAM;
        }
    }
    return (PHYMOD_E_NONE);
}

int plp_aperta_pm_tsc_mem_access(const plp_aperta_phymod_phy_access_t*phy, APERTA_PM_MEM_ACCESS_T* pm_mem_access)
{
    BCMI_APERTA_D_LMI_LMI_CMDr_t  lmi_cmd;
    BCMI_APERTA_D_LMI_LMI_STATUSr_t lmi_sts;
    BCMI_APERTA_D_LMI_LMI_CMD_SEQr_t lmi_cmd_seq;
    int access_prog = 0;
    int retry_cnt = APERTA_LMI_RETRY_CNT, done = 0, count = 0;
#if APERTA_FW_READ_WRITE
    uint32_t is_dloaded = 0;
#endif
    PHYMOD_MEMSET(&lmi_cmd, 0, sizeof(BCMI_APERTA_D_LMI_LMI_CMDr_t));
    PHYMOD_MEMSET(&lmi_sts, 0, sizeof(BCMI_APERTA_D_LMI_LMI_STATUSr_t));
    PHYMOD_MEMSET(&lmi_cmd_seq, 0, sizeof(BCMI_APERTA_D_LMI_LMI_CMD_SEQr_t));
#if APERTA_FW_READ_WRITE
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_pm_is_fw_dloaded_get(phy->access.addr, &is_dloaded));
#endif    
    access_prog = plp_aperta_prg_mem_access_prog_val_len(pm_mem_access->pm_mem_type, &pm_mem_access->pm_mem_len);
#if APERTA_FW_READ_WRITE
    if (is_dloaded == 1 ) {
        PHYMOD_IF_ERR_RETURN(
                plp_aperta_fw_mem_access(phy, pm_mem_access));
    } else {    /* Wait for done==0 */
#endif
        if (pm_mem_access->pm_mem_type == phymodMemPmMib) {
            PHYMOD_DEBUG_ERROR(("PM MIB memory cannot be accessed before Intialization\n"));
            return PHYMOD_E_UNAVAIL;
        }
        do {
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_READ_LMI_LMI_STATUSr(&phy->access, &lmi_sts));
            done = BCMI_APERTA_D_LMI_LMI_STATUSr_DONEf_GET(lmi_sts);
            PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
        } while ((done != 0) && --retry_cnt);
        if (retry_cnt <= 0) {
            PHYMOD_DEBUG_ERROR((" LMI Access timeout\n"));
            return PHYMOD_E_TIMEOUT;
        }
        retry_cnt = APERTA_LMI_RETRY_CNT;

        BCMI_APERTA_D_LMI_LMI_CMDr_SLAVE_SELf_SET(lmi_cmd,   (pm_mem_access->pm_mem_sel == APERTA_LINE_SIDE_PM)? 0x1: (pm_mem_access->pm_mem_sel == APERTA_SYS_SIDE_PM)? 0x9: 0xF);
        BCMI_APERTA_D_LMI_LMI_CMDr_ACCESS_PROGf_SET(lmi_cmd, access_prog);
        BCMI_APERTA_D_LMI_LMI_CMDr_PORT_SELf_SET   (lmi_cmd, 0);
        BCMI_APERTA_D_LMI_LMI_CMDr_WRITE_ENf_SET   (lmi_cmd, pm_mem_access->pm_mem_rw);
        BCMI_APERTA_D_LMI_LMI_CMDr_USE_CMD_EXTf_SET(lmi_cmd, 0);

        BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, lmi_cmd.v[0]);
        PHYMOD_IF_ERR_RETURN(
            BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));

        /* Check whether we are performing read/write*/
        if (pm_mem_access->pm_mem_rw == 1) { /* Write operation*/
            uint16_t write_count = 0;
            for (count = 0; count < pm_mem_access->pm_mem_len; count++) {
                /* Since 16 bit*/
                /* Write LSB*/
                BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, (pm_mem_access->pm_mem_data[count] & 0xFFFF));
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                if (++write_count >= pm_mem_access->pm_mem_len) {
                    break;
                }

                /* Write MSB*/
                BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, (pm_mem_access->pm_mem_data[count] >> 16) & 0xFFFF);
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
                if (++write_count >= pm_mem_access->pm_mem_len) {
                    break;
                }
            }
            BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, pm_mem_access->pm_mem_addr);
            PHYMOD_IF_ERR_RETURN(
               BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
            /* Wait for done==1*/
            do {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_LMI_LMI_STATUSr(&phy->access, &lmi_sts));
                done = BCMI_APERTA_D_LMI_LMI_STATUSr_DONEf_GET(lmi_sts);
                PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
            } while ((done != 1) && --retry_cnt);
            if (retry_cnt <= 0) {
                PHYMOD_DEBUG_ERROR((" LMI Access timeout\n"));
                return PHYMOD_E_TIMEOUT;
            }
            if (BCMI_APERTA_D_LMI_LMI_STATUSr_ACCESS_ERRORf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_DED_ERRf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_WRONG_SLAVE_SELf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_UNDERRUNf_GET(lmi_sts)  ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_OVERRUNf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_ERRf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_TIME_OUTf_GET(lmi_sts)) {
                   PHYMOD_DEBUG_ERROR((" LMI mem Access Error:%x: phy:%d\n", lmi_sts.v[0],phy->access.addr));
                return PHYMOD_E_INTERNAL;
            }
        } else { /* Read operation*/
            uint16_t read_count = 0;
            BCMI_APERTA_D_LMI_LMI_CMD_SEQr_SEQ_DATAf_SET(lmi_cmd_seq, pm_mem_access->pm_mem_addr);
            PHYMOD_IF_ERR_RETURN(
                BCMI_APERTA_D_WRITE_LMI_LMI_CMD_SEQr(&phy->access, lmi_cmd_seq));
            /* Wait for data ready==1*/
            do {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_LMI_LMI_STATUSr(&phy->access, &lmi_sts));
                done = BCMI_APERTA_D_LMI_LMI_STATUSr_RD_DATA_RDYf_GET(lmi_sts);
                PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
            } while ((done != 1) && --retry_cnt);
            if (retry_cnt <= 0) {
                PHYMOD_DEBUG_ERROR((" LMI Access timeout\n"));
                return PHYMOD_E_TIMEOUT;
            }
            if (BCMI_APERTA_D_LMI_LMI_STATUSr_ACCESS_ERRORf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_DED_ERRf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_WRONG_SLAVE_SELf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_UNDERRUNf_GET(lmi_sts)  ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_FIFO_OVERRUNf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_ERRf_GET(lmi_sts) ||
                BCMI_APERTA_D_LMI_LMI_STATUSr_SLV_TIME_OUTf_GET(lmi_sts)) {
                   PHYMOD_DEBUG_ERROR((" LMI mem Access Error:%x: phy:%d\n", lmi_sts.v[0],phy->access.addr));
                return PHYMOD_E_INTERNAL;
            }

            for (count = 0; count < pm_mem_access->pm_mem_len; count++) {
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_LMI_LMI_CMD_SEQr(&phy->access, &lmi_cmd_seq));
                pm_mem_access->pm_mem_data[count] = lmi_cmd_seq.v[0];
                if (++read_count >= pm_mem_access->pm_mem_len) {
                    break;
                }
                PHYMOD_IF_ERR_RETURN(
                    BCMI_APERTA_D_READ_LMI_LMI_CMD_SEQr(&phy->access, &lmi_cmd_seq));
                pm_mem_access->pm_mem_data[count] |= (lmi_cmd_seq.v[0] << 16);
                if (++read_count >= pm_mem_access->pm_mem_len) {
                    break;
                }
            }

        }
#if APERTA_FW_READ_WRITE
    }
#endif    
    return PHYMOD_E_NONE;
}

int plp_aperta_prg_mem_access_prog_val_len(phymod_mem_type_t memory_type, int *len) {
    int access_prog = 0x7;
    switch (memory_type) {
        case phymodMemPmMib:
        {
          access_prog |= ((0xF & 0x0) << 3);
          *len = 32;
        }
        break;
        case phymodMemSpeedIdTable:
        {
          access_prog |= ((0xF & 0x2) << 3);
          *len = 10;
        }
        break;
        case phymodMemAMTable:
        {
          access_prog |= ((0xF & 0x3) << 3);
          *len = 5;
        }
        break;
        case phymodMemUMTable:
        {
          access_prog |= ((0xF & 0x4) << 3);
          *len = 4;
        }
        break;
        case phymodMemTxLkup1588Mpp0:
        {
          access_prog |= ((0xF & 0x5) << 3);
          *len = 5;
        }
        break;
        case phymodMemTxLkup1588Mpp1:
        {
          access_prog |= ((0xF & 0x6) << 3);
          *len = 5;
        }
        break;
        case phymodMemTxLkup1588400G:
        {
          access_prog |= ((0xF & 0x7) << 3);
          *len = 5;
        }
        break;
        case phymodMemRxLkup1588Mpp0:
        {
          access_prog |= ((0xF & 0x8) << 3);
          *len = 5;
        }
        break;
        case phymodMemRxLkup1588Mpp1:
        {
          access_prog |= ((0xF & 0x9) << 3);
          *len = 5;
        }
        break;
        case phymodMemRxLkup1588400G:
        {
          access_prog |= ((0xF & 0xA) << 3);
          *len = 5;
        }
        break;
        case phymodMemSpeedPriorityMapTable:
        {
          access_prog |= ((0xF & 0xB) << 3);
          *len = 18;
        }
        break;
        case phymodMemCount:
        {
          access_prog |= ((0xF & 0x1) << 3);
          *len = 8;
        }
        break;
        default:
            return access_prog;
            
    }
    return access_prog;
}

/*
 * Function:
 *  plp_aperta_phymod_aperta_field_get
 * Purpose:
 *  Extract multi-word field value from multi-word register/memory.
 * Parameters:
 *  entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer where to store extracted field value
 * Returns:
 *      Pointer to extracted field value.
 */
uint32_t *
plp_aperta_phymod_aperta_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    int i, wp, bp, len;


    bp = sbit;
    len = ebit - sbit + 1;

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            fbuf[i] = (entbuf[wp++] >> bp) & ((1 << (32 - bp)) - 1);
            if (len > (32 - bp)) {
                fbuf[i] |= entbuf[wp] << (32 - bp);
            }
        } else {
            fbuf[i] = entbuf[wp++];
        }
        if (len < 32) {
            fbuf[i] &= ((1 << len) - 1);
        }
    }

    return fbuf;
}
/*
 * Function:
 *  plp_aperta_phymod_aperta_field_set
 * Purpose:
 *  Assign multi-word field value in multi-word register/memory.
 * Parameters:
 *  entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 *      fbuf - buffer with new field value
 * Returns:
 *      Nothing.
 */
void
plp_aperta_phymod_aperta_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
{
    uint32_t mask;
    int i, wp, bp, len;


    bp = sbit;
    len = ebit - sbit + 1;

    wp = bp / 32;
    bp = bp & (32 - 1);
    i = 0;

    for (; len > 0; len -= 32, i++) {
        if (bp) {
            if (len < 32) {
                mask = (1 << len) - 1;
            } else {
                mask = ~0;
            }
            entbuf[wp] &= ~(mask << bp);
            entbuf[wp++] |= fbuf[i] << bp;
            if (len > (32 - bp)) {
                entbuf[wp] &= ~(mask >> (32 - bp));
                entbuf[wp] |= fbuf[i] >> (32 - bp) & ((1 << bp) - 1);
            }
        } else {
            if (len < 32) {
                mask = (1 << len) - 1;
                entbuf[wp] &= ~mask;
                entbuf[wp++] |= (fbuf[i] & mask) << bp;
            } else {
                entbuf[wp++] = fbuf[i];
            }
        }
    }
}

/*
 * Function:
 *  plp_aperta_phymod_aperta_field32_get
 * Purpose:
 *  Extract field value from multi-word register/memory.
 * Parameters:
 *  entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 * Returns:
 *      Extracted field value.
 */
uint32_t
plp_aperta_phymod_aperta_field32_get(const uint32_t *entbuf, int sbit, int ebit)
{
    uint32_t fval[1];

    plp_aperta_phymod_aperta_field_get(entbuf, sbit, ebit, fval);

    return fval[0];
}

/*
 * Function:
 *  plp_aperta_phymod_aperta_field32_set
 * Purpose:
 *  Assign field value in multi-word register/memory.
 * Parameters:
 *  entbuf - current contents of register/memory (word array)
 *      sbit - bit number of first bit of the field to extract
 *      ebit - bit number of last bit of the field to extract
 * Returns:
 *      Nothing.
 */
void
plp_aperta_phymod_aperta_field32_set(uint32_t *entbuf, int sbit, int ebit, uint32_t fval)
{
    uint32_t fval_loc[1];
    fval_loc[0] = fval;
    plp_aperta_phymod_aperta_field_set(entbuf, sbit, ebit, fval_loc);
}


