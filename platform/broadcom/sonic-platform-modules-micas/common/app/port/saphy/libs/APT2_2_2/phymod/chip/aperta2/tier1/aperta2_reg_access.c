#include "aperta2_reg_access.h"
#include <tier1/aperta2_msg_task.h>

/*
 * Function:
 *  plp_aperta2_phymod_aperta2_field_get
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
plp_aperta2_phymod_aperta2_field_get(const uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
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
 *  phymod_aperta_field_set
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
plp_aperta2_phymod_aperta2_field_set(uint32_t *entbuf, int sbit, int ebit, uint32_t *fbuf)
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
 *  phymod_aperta_field32_get
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
plp_aperta2_phymod_aperta2_field32_get(const uint32_t *entbuf, int sbit, int ebit)
{
    uint32_t fval[1];

    plp_aperta2_phymod_aperta2_field_get(entbuf, sbit, ebit, fval);

    return fval[0];
}

/*
 * Function:
 *  phymod_aperta_field32_set
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
plp_aperta2_phymod_aperta2_field32_set(uint32_t *entbuf, int sbit, int ebit, uint32_t fval)
{
    uint32_t fval_loc[1];
    fval_loc[0] = fval;
    plp_aperta2_phymod_aperta2_field_set(entbuf, sbit, ebit, fval_loc);
}

/*!
 *  Function to read 32-bit register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Register address
 *  @param data            output, register value
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_reg32_read(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint32_t *data)
{
    aperta2_register_read_t read_param;
    int speed = 0, lane_data_rate = 0;

    PHYMOD_MEMSET(&read_param, 0, sizeof(aperta2_register_read_t));

    APERTA2_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA2_GET_PLL_MICRO_SEL(phy, read_param.pll_sel, read_param.micro_sel);
    APERTA2_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, read_param.port_number, read_param.lane_sel, read_param.oct_sel);
    APERTA2_GET_PM_SEL(phy, read_param.oct_sel, read_param.pm_sel)

    
    read_param.lane_sel = read_param.lane_sel & (-read_param.lane_sel);
    read_param.reg_addr=reg_addr;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_register_read(phy, &read_param));
    COMPILER_64_TO_32_LO (*data, read_param.reg_data);


    return PHYMOD_E_NONE;
}

/*!
 *  Function to write 32-bit register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Register address
 *  @param data            register value to be written
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_reg32_write(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint32_t data)
{
    aperta2_register_write_t write_param;
    int speed = 0, lane_data_rate = 0;

    PHYMOD_MEMSET(&write_param, 0, sizeof(aperta2_register_write_t));

    APERTA2_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA2_GET_PLL_MICRO_SEL(phy, write_param.pll_sel, write_param.micro_sel);
    APERTA2_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, write_param.port_number, write_param.lane_sel, write_param.oct_sel);
    APERTA2_GET_PM_SEL(phy, write_param.oct_sel, write_param.pm_sel)
    write_param.reg_addr=reg_addr;
    COMPILER_64_SET(write_param.reg_data, 0, data);
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_register_write(phy, &write_param));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to read 64-bit register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Register address
 *  @param data            output, register value 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_reg64_read(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint64_t *data)
{
    aperta2_register_read_t read_param;
    int speed = 0, lane_data_rate = 0;

    PHYMOD_MEMSET(&read_param, 0, sizeof(aperta2_register_read_t));

    APERTA2_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA2_GET_PLL_MICRO_SEL(phy, read_param.pll_sel, read_param.micro_sel);
    APERTA2_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, read_param.port_number, read_param.lane_sel, read_param.oct_sel);
    APERTA2_GET_PM_SEL(phy, read_param.oct_sel, read_param.pm_sel)
    
    read_param.lane_sel = read_param.lane_sel & (-read_param.lane_sel);
    read_param.reg_addr=reg_addr;

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_register_read(phy, &read_param));
    COMPILER_64_COPY (*data, read_param.reg_data);

    return PHYMOD_E_NONE;
}

/*!
 *  Function to write 64-bit register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Register address
 *  @param data            Register value to be written
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_reg64_write(const plp_aperta2_phymod_phy_access_t *phy, unsigned int reg_addr, uint64_t data)
{
    aperta2_register_write_t write_param;
    int speed = 0, lane_data_rate = 0;

    PHYMOD_MEMSET(&write_param, 0, sizeof(aperta2_register_write_t));

    APERTA2_GET_PORT_SPEED_LDR(phy, speed, lane_data_rate);
    APERTA2_GET_PLL_MICRO_SEL(phy, write_param.pll_sel, write_param.micro_sel);
    APERTA2_GET_PORT_FROM_LM_SP(speed, lane_data_rate, phy->access.lane_mask, write_param.port_number, write_param.lane_sel, write_param.oct_sel);
    APERTA2_GET_PM_SEL(phy, write_param.oct_sel, write_param.pm_sel)
    write_param.reg_addr=reg_addr;

    COMPILER_64_COPY(write_param.reg_data, data);
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_register_write(phy, &write_param));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to write in to memory 
 *
 *  @param phy             phy access configuration
 *  @param mem_type        Memory to access
 *  @param mem_index       Memory index to access
 *  @param data            Data array to be written on the memory index 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_mem_write(const plp_aperta2_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data)
{
    int oct_sel = APERTA2_GET_OCTAL(phy->access.lane_mask);
    int get_init_state = 0;
    aperta2_pm_reg_t pm_reg ;

    PHYMOD_MEMSET(&pm_reg, 0, sizeof(aperta2_pm_reg_t));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_init_state(phy->access.addr, &get_init_state));

    if (get_init_state == APERTA2_INIT_STATE_START) {
        pm_reg.pm_sel = 0xF;
    } else {
        APERTA2_GET_PM_SEL(phy, oct_sel, pm_reg.pm_sel)
    }

    (void)oct_sel;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_mem_access_type_len(mem_type, &pm_reg.datalen, &pm_reg.access_type));
    pm_reg.addr = mem_index;
    pm_reg.mem_data = data;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_mem_access(phy, &pm_reg, 1 /*write_en*/));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to read from memory 
 *
 *  @param phy             phy access configuration
 *  @param mem_type        Memory to access
 *  @param mem_index       Memory index to access
 *  @param data            Read Data  
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_mem_read(const plp_aperta2_phymod_phy_access_t *phy, phymod_mem_type_t mem_type, uint32_t mem_index, uint32_t* data)
{
    int oct_sel = APERTA2_GET_OCTAL(phy->access.lane_mask);
    aperta2_pm_reg_t pm_reg ;

    PHYMOD_MEMSET(&pm_reg, 0, sizeof(aperta2_pm_reg_t));

    APERTA2_GET_PM_SEL(phy, oct_sel, pm_reg.pm_sel)
    (void)oct_sel;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_mem_access_type_len(mem_type, &pm_reg.datalen, &pm_reg.access_type));
    pm_reg.addr = mem_index;
    pm_reg.mem_data = data;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_fw_mem_access(phy, &pm_reg, 0 /*write_en*/));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to write direct register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Direct register address
 *  @param reg_data        Data to write
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_direct_reg_write(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t reg_data)
{
    if (((reg_addr & 0xFFFF) >= (APERTA2_FWREG_BASEADDR_RANGE_START & 0xFFFF)) && ((reg_addr & 0xFFFF) <= (APERTA2_FWREG_BASEADDR_RANGE_END & 0xFFFF))) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_reg32_write(phy, reg_addr, reg_data));
    } else {
        if(phy->access.devad == 30) { /* User specific device address */
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_WRITE(&phy->access, ((30<<16) | (reg_addr & 0xFFFF)), reg_data));
        } else { /* For PMA and PMD device address */
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_WRITE(&phy->access, ((1<<16) | (reg_addr & 0xFFFF)), reg_data));
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to read direct register
 *
 *  @param phy             phy access configuration
 *  @param reg_addr        Direct register address
 *  @param reg_data        Read Data
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_direct_reg_read(const plp_aperta2_phymod_phy_access_t* phy, uint32_t reg_addr, uint32_t *reg_data)
{
   if (((reg_addr & 0xFFFF) >= (APERTA2_FWREG_BASEADDR_RANGE_START & 0xFFFF)) && ((reg_addr & 0xFFFF) <= (APERTA2_FWREG_BASEADDR_RANGE_END & 0xFFFF))) {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_reg32_read(phy, reg_addr, reg_data));
    } else {
        if(phy->access.devad == 30) {
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(&phy->access, ((30<<16) | (reg_addr & 0xFFFF)), reg_data));
        } else {
            PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(&phy->access, ((1<<16) | (reg_addr & 0xFFFF)), reg_data));
        }
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to update TSC register address
 *
 *  @param reg_addr        TSC register address
 *  @param lane_sel        TSC lane select
 *  @param pll_sel         TSC PLL select
 *  @param tsc_addr        Address to be written 
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_get_tsc_reg(uint32_t reg_addr, uint32_t lane_sel, uint32_t pll_sel, uint32_t *tsc_addr)
{
    uint8_t mpp0 = 0, mpp1= 0, tsc_lane_sel = 0, device_id = 0;                                         

    lane_sel = (lane_sel & 0xFF00) ? (lane_sel >> 8) : lane_sel;

    if ((reg_addr >> APERTA2_REG_BASE_SHIFT) == (APERTA2_PM_TSCP_16B_BASEADR >> APERTA2_REG_BASE_SHIFT)) {   
        device_id     = 0;
        mpp1          = (lane_sel & 0xF0) ? 1 : 0;
        mpp0          = (lane_sel & 0x0F) ? 1 : 0;

        if (mpp1 == 0 && mpp0 == 0) {
            PHYMOD_DEBUG_ERROR(("MPP0 and MPP1 cannot be 0:lanesel:%x\n", lane_sel));
            return PHYMOD_E_PARAM;
        }

        if (lane_sel & 0xF0) {
            lane_sel = lane_sel >> 4;
        }

        /* Per Lane register:
           Translate lane_sel code to TSC port_lane_mode*/
        switch (lane_sel) {
            case 0x1:      tsc_lane_sel = 0x0; break; /* Single lane 0*/
            case 0x2:      tsc_lane_sel = 0x1; break; /* Single lane 1*/
            case 0x4:      tsc_lane_sel = 0x2; break; /* Single lane 2*/
            case 0x8:      tsc_lane_sel = 0x3; break; /* Single lane 3*/
            case 0x3:      tsc_lane_sel = 0x4; break; /* Dual lane: lanes 0 and 1*/
            case 0xC:      tsc_lane_sel = 0x5; break; /* Dual lane: lanes 2 and 3*/
            case 0xF:      tsc_lane_sel = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0xFF:     tsc_lane_sel = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0xFFFF:   tsc_lane_sel = 0x6; break; /* Quad lane: lanes 0, 1, 2, 3*/
            case 0x0:      tsc_lane_sel = 0x6; break; /* LANE_NONE: Select lanes 0, 1, 2, 3 when lane is don't care*/
            default:
                PHYMOD_DEBUG_ERROR(("lane_sel=%x is not supported by PortMacro TSCP", lane_sel));
                return PHYMOD_E_PARAM;
        }
        *tsc_addr = (device_id << 27) | (mpp1 << 25 )| (mpp0 << 24) | ((tsc_lane_sel & 7) << 16) |(reg_addr & 0xFFFF);
    } else {
        device_id     = 1;
        reg_addr      = reg_addr & 0xFFFF;
        switch (lane_sel) {
            case 0x0 : tsc_lane_sel = 0x00; break; /* LANE_NONE: Select lane 0 when lane is don't care*/
            case 0x1 : tsc_lane_sel = 0x00; break;
            case 0x2 : tsc_lane_sel = 0x01; break;
            case 0x4 : tsc_lane_sel = 0x02; break;
            case 0x8 : tsc_lane_sel = 0x03; break;
            case 0x10: tsc_lane_sel = 0x04; break;
            case 0x20: tsc_lane_sel = 0x05; break;
            case 0x40: tsc_lane_sel = 0x06; break;
            case 0x80: tsc_lane_sel = 0x07; break;
            case 0x3 : tsc_lane_sel = 0x20; break; /* Multicast_0_1         3'b001 5'h00*/
            case 0xC : tsc_lane_sel = 0x21; break; /* Multicast_2_3         3'b001 5'h01*/
            case 0x30: tsc_lane_sel = 0x22; break; /* Multicast_4_5         3'b001 5'h02*/
            case 0xC0: tsc_lane_sel = 0x23; break; /* Multicast_6_7         3'b001 5'h03*/
            case 0xF : tsc_lane_sel = 0x40; break; /* Multicast_0_1_2_3     3'b010 5'h00*/
            case 0xF0: tsc_lane_sel = 0x41; break; /* Multicast_4_5_6_7     3'b010 5'h01*/
            case 0xFF: tsc_lane_sel = 0xFF; break; /* Broadcast All Lanes               */
            default:
                       PHYMOD_DEBUG_ERROR(("aperta2_get_tsc_addr::lane_sel=%x is not supported by PortMacro PMD", lane_sel));
                       return PHYMOD_E_PARAM;
        }
        *tsc_addr = (device_id << 27) | (pll_sel << 24) | ((tsc_lane_sel & 7) << 16) |(reg_addr);
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Function to write register using FW message
 *
 *  @param phy             phy access structure
 *  @param write_param     Information to write register
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_register_write(const plp_aperta2_phymod_phy_access_t* phy, aperta2_register_write_t *write_param)
{
    uint32_t fw_slave, data_length = 0, tsc_addr = 0, data = 0;
    uint8_t access_type = 0;
    aperta2_pm_reg_t pm_reg ;
    aperta2_tsc_reg_t tsc_reg ;
    aperta2_ind_reg_t ind_reg ;
    aperta2_macsec_reg_t macsec_reg ;
    aperta2_ahb_reg_t ahb_reg;
    int get_init_state = 0;
    uint32_t egr_port = 0, ing_port=0, temp = 0;

    /* Get LMI Slave*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_lmi_slave_length(write_param->reg_addr, &fw_slave, &data_length));
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_init_state(phy->access.addr, &get_init_state));

    if (fw_slave == APERTA2_DIRECT_ACCESS) { /* To handle Direct access*/
        uint32_t dir_write = COMPILER_64_LO(write_param->reg_data);
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_write(phy, write_param->reg_addr, dir_write));
    } else if (fw_slave == APERTA2_FW_PM_TSC) {     /* To handle PCS and PMD*/
        PHYMOD_MEMSET(&tsc_reg, 0, sizeof(tsc_reg));
        write_param->pll_sel = write_param->micro_sel;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_tsc_reg(write_param->reg_addr, write_param->lane_sel, write_param->pll_sel, &tsc_addr));
        if (get_init_state == APERTA2_INIT_STATE_START) {
            tsc_reg.pm_sel = 0xF;
        } else {
            tsc_reg.pm_sel = write_param->pm_sel;
        }
        tsc_reg.addr = tsc_addr;
        data = COMPILER_64_LO(write_param->reg_data);
        tsc_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_TSC_REGS, &tsc_reg));

    } else if ((fw_slave == APERTA2_FW_PM_PORT) || (fw_slave == APERTA2_FW_PM_MAC)) {  /* To handle MAC & port*/
        PHYMOD_MEMSET(&pm_reg, 0, sizeof(pm_reg));
        if (write_param->port_number >= 8) {
            if ((write_param->pm_sel == APERTA2_PM_SYS_OCTAL1) ||
                    (write_param->pm_sel == APERTA2_PM_LINE_OCTAL1)) {
                write_param->port_number -= 8;
            }
        }
        APERTA2_FW_PM_REG_ACCESS_TYPE(write_param->reg_addr, write_param->port_number, fw_slave, access_type)
        if (get_init_state == APERTA2_INIT_STATE_START) {
            pm_reg.pm_sel = 0xF;
        } else {
            pm_reg.pm_sel = write_param->pm_sel; 
        }
        pm_reg.access_type = access_type;
        pm_reg.datalen = (data_length/8); /*bit to byte conversion*/
        pm_reg.addr = (write_param->reg_addr & 0xFFFF);
        pm_reg.data = &write_param->reg_data;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_PM_REGS, &pm_reg));
    } else if (fw_slave == APERTA2_FW_MACSEC) { /*to handle MACSEC*/
        PHYMOD_MEMSET(&macsec_reg, 0, sizeof(macsec_reg));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a005 + (16 *  write_param->port_number), &temp)); /* ingress port and egress port */
        macsec_reg.oct_sel = write_param->oct_sel;
        APERTA2_UPDATE_REG_ADDR(fw_slave, write_param->reg_addr, temp);
        macsec_reg.addr = write_param->reg_addr;
        data = COMPILER_64_LO(write_param->reg_data);
        macsec_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_MACSEC_REGS, &macsec_reg));
    } else if (fw_slave == APERTA2_FW_INDIRECT) {
        PHYMOD_MEMSET(&ind_reg, 0, sizeof(ind_reg));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a005 + (16 *  write_param->port_number), &temp)); /* ingress port and egress port */
        ind_reg.oct_sel = write_param->oct_sel;
        APERTA2_UPDATE_REG_ADDR(fw_slave, write_param->reg_addr, temp);
        ind_reg.addr = (write_param->reg_addr & 0xFFFF);
        data = COMPILER_64_LO(write_param->reg_data);
        ind_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_CHIP_IND_REGS, &ind_reg));
    } else if (fw_slave == APERTA2_FW_AHB) { /* to handle AHB*/
        PHYMOD_MEMSET(&ahb_reg, 0, sizeof(ahb_reg));
        ahb_reg.addr = write_param->reg_addr;
        data = COMPILER_64_LO(write_param->reg_data);
        ahb_reg.data = &data;
        if (((ahb_reg.addr & 0xFFFF) >= (APERTA2_FWREG_BASEADDR_RANGE_START & 0xFFFF)) && ((ahb_reg.addr & 0xFFFF) <= (APERTA2_FWREG_BASEADDR_RANGE_END & 0xFFFF))) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_CHIP_DIR_REGS, &ahb_reg));
        } else {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_WRITE, APERTA2_FUNC_AHB_REGS, &ahb_reg));
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid SLAVE:0x%x\n", write_param->reg_addr));
        return PHYMOD_E_PARAM;
    }



    return PHYMOD_E_NONE;
}

/*!
 *  Function to read register using FW message
 *
 *  @param phy             phy access structure
 *  @param read_param      Information to read register
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_register_read(const plp_aperta2_phymod_phy_access_t* phy, aperta2_register_read_t *read_param)
{
    uint32_t fw_slave, data_length = 0, tsc_addr = 0, data = 0;
    uint8_t access_type = 0;
    aperta2_pm_reg_t pm_reg ;
    aperta2_tsc_reg_t tsc_reg ;
    aperta2_ind_reg_t ind_reg ;
    aperta2_macsec_reg_t macsec_reg ;
    aperta2_ahb_reg_t ahb_reg;
    uint32_t egr_port = 0, ing_port=0, temp = 0;

    /* Get LMI Slave*/
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_lmi_slave_length(read_param->reg_addr, &fw_slave, &data_length));

    if (fw_slave == APERTA2_DIRECT_ACCESS) { /* To handle Direct access*/
        uint32_t dir_read = 0;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_direct_reg_read(phy, read_param->reg_addr, &dir_read));
        COMPILER_64_SET(read_param->reg_data, 0, dir_read);
    } else if (fw_slave == APERTA2_FW_PM_TSC) {     /* To handle PCS and PMD*/
        PHYMOD_MEMSET(&tsc_reg, 0, sizeof(tsc_reg));
        read_param->pll_sel = read_param->micro_sel;
        PHYMOD_IF_ERR_RETURN(
            plp_aperta2_get_tsc_reg(read_param->reg_addr, read_param->lane_sel, read_param->pll_sel, &tsc_addr));
        tsc_reg.pm_sel = read_param->pm_sel;
        tsc_reg.addr = tsc_addr;
        tsc_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_TSC_REGS, &tsc_reg));
        COMPILER_64_SET(read_param->reg_data, 0, data);

    } else if ((fw_slave == APERTA2_FW_PM_PORT) || (fw_slave == APERTA2_FW_PM_MAC)) {  /* To handle MAC & port*/
        PHYMOD_MEMSET(&pm_reg, 0, sizeof(pm_reg));
        if (read_param->port_number >= 8) {
            if ((read_param->pm_sel == APERTA2_PM_SYS_OCTAL1) ||
                    (read_param->pm_sel == APERTA2_PM_LINE_OCTAL1)) {
                read_param->port_number -= 8;
            }
        }
        APERTA2_FW_PM_REG_ACCESS_TYPE(read_param->reg_addr, read_param->port_number, fw_slave, access_type)
        pm_reg.pm_sel = read_param->pm_sel; 
        pm_reg.access_type = access_type;
        pm_reg.datalen = (data_length/8); /*bit to byte conversion*/
        pm_reg.addr = (read_param->reg_addr & 0xFFFF);
        pm_reg.data = &read_param->reg_data;
        PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_PM_REGS, &pm_reg));
    } else if (fw_slave == APERTA2_FW_MACSEC) { /*to handle MACSEC*/
        PHYMOD_MEMSET(&macsec_reg, 0, sizeof(macsec_reg));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a005 + (16 *  read_param->port_number), &temp)); /* ingress port and egress port */
        macsec_reg.oct_sel = read_param->oct_sel;
        APERTA2_UPDATE_REG_ADDR(fw_slave, read_param->reg_addr, temp);
        macsec_reg.addr = read_param->reg_addr;
        macsec_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_MACSEC_REGS, &macsec_reg));
        COMPILER_64_SET(read_param->reg_data, 0, data);
    } else if (fw_slave == APERTA2_FW_INDIRECT) {
        PHYMOD_MEMSET(&ind_reg, 0, sizeof(ind_reg));
        PHYMOD_IF_ERR_RETURN(plp_aperta2_direct_reg_read(phy, 0x0100a005 + (16 *  read_param->port_number), &temp)); /* ingress port and egress port */
        ind_reg.oct_sel = read_param->oct_sel;
        APERTA2_UPDATE_REG_ADDR(fw_slave, read_param->reg_addr, temp);
        ind_reg.addr = (read_param->reg_addr & 0xFFFF);
        ind_reg.data = &data;
        PHYMOD_IF_ERR_RETURN(
             plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_CHIP_IND_REGS, &ind_reg));
        COMPILER_64_SET(read_param->reg_data, 0, data);
    } else if (fw_slave == APERTA2_FW_AHB) { /* to handle AHB*/
        PHYMOD_MEMSET(&ahb_reg, 0, sizeof(ahb_reg));
        ahb_reg.addr = read_param->reg_addr;
        ahb_reg.data = &data;
        if (((ahb_reg.addr & 0xFFFF) >= (APERTA2_FWREG_BASEADDR_RANGE_START & 0xFFFF)) && ((ahb_reg.addr & 0xFFFF) <= (APERTA2_FWREG_BASEADDR_RANGE_END & 0xFFFF))) {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_CHIP_DIR_REGS, &ahb_reg));
        } else {
            PHYMOD_IF_ERR_RETURN(
                plp_aperta2_fw_reg_access(phy, APERTA2_REG_READ, APERTA2_FUNC_AHB_REGS, &ahb_reg));
        }
        COMPILER_64_SET(read_param->reg_data, 0, data);
    } else {
        PHYMOD_DEBUG_ERROR(("Invalid SLAVE:0x%x\n", read_param->reg_addr));
        return PHYMOD_E_PARAM;
    }



    return PHYMOD_E_NONE;
}

/*!
 *  Function to get LMI slave and its length
 *
 *  @param reg_addr         Address of the register
 *  @param slave            output, Slave address
 *  @param length           output, length of the address specified
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_get_lmi_slave_length(uint32_t reg_addr, uint32_t *slave, uint32_t *length)
{

    switch (reg_addr >> APERTA2_REG_BASE_SHIFT) {
        case (APERTA2_PM_DC3PORT_16B_BASEADR       >> APERTA2_REG_BASE_SHIFT):  { *length = 32; *slave = APERTA2_FW_PM_PORT; break; }
        case (APERTA2_PM_DC3PORT_32B_BASEADR       >> APERTA2_REG_BASE_SHIFT):  { *length = 32; *slave = APERTA2_FW_PM_PORT; break; }
        case (APERTA2_PM_DC3PORT_48B_BASEADR       >> APERTA2_REG_BASE_SHIFT):  { *length = 48; *slave = APERTA2_FW_PM_PORT; break; }
        case (APERTA2_PM_DC3PORT_64B_BASEADR       >> APERTA2_REG_BASE_SHIFT):  { *length = 64; *slave = APERTA2_FW_PM_PORT; break; }
        case (APERTA2_PM_DC3MAC_16B_BASEADR        >> APERTA2_REG_BASE_SHIFT):  { *length = 32; *slave = APERTA2_FW_PM_MAC; break; }
        case (APERTA2_PM_DC3MAC_32B_BASEADR        >> APERTA2_REG_BASE_SHIFT):  { *length = 32; *slave = APERTA2_FW_PM_MAC; break; }
        case (APERTA2_PM_DC3MAC_48B_BASEADR        >> APERTA2_REG_BASE_SHIFT):  { *length = 48; *slave = APERTA2_FW_PM_MAC; break; }
        case (APERTA2_PM_DC3MAC_64B_BASEADR        >> APERTA2_REG_BASE_SHIFT):  { *length = 64; *slave = APERTA2_FW_PM_MAC; break; }
        case (APERTA2_PM_TSCP_16B_BASEADR          >> APERTA2_REG_BASE_SHIFT):  { *length = 16; *slave = APERTA2_FW_PM_TSC; break; }
        case (APERTA2_PM_TSC_PERIGRINE_BASE        >> APERTA2_REG_BASE_SHIFT):  { *length = 16; *slave = APERTA2_FW_PM_TSC; break; }
        case (APERTA2_CHIP_IND_BASEADR             >> APERTA2_REG_BASE_SHIFT):  { *length = 16; *slave = APERTA2_FW_INDIRECT; break; }
        case (APERTA2_EIP218_P2M_INGRESS_BASEADR   >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP218_P2M_EGRESS_BASEADR    >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP218_RC_EGRESS_BASEADR     >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP163_INGRESS_BASEADR       >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP164_INGRESS_BASEADR       >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP163_EGRESS_BASEADR        >> APERTA2_REG_BASE_SHIFT):  
        case (APERTA2_EIP164_EGRESS_BASEADR        >> APERTA2_REG_BASE_SHIFT): 
            { *length = 32; *slave = APERTA2_FW_MACSEC; break; }
        case (APERTA2_VTMON_AVS_I2C_PMON_BASEADR   >> APERTA2_REG_BASE_SHIFT):
        case (APERTA2_PKT_INJ_EGRESS_BASEADR       >> APERTA2_REG_BASE_SHIFT): 
        case (APERTA2_PKT_EXT_EGRESS_BASEADR       >> APERTA2_REG_BASE_SHIFT):
        case (APERTA2_PKT_INJ_INGRESS_BASEADR      >> APERTA2_REG_BASE_SHIFT):
        case (APERTA2_PKT_EXT_INGRESS_BASEADR      >> APERTA2_REG_BASE_SHIFT):
        case (APERTA2_M0_MST_BASEADR               >> APERTA2_REG_BASE_SHIFT):
            { *length = 32; *slave = APERTA2_FW_AHB; break; }
        default:
            if (((reg_addr & 0xFFFF) >= (APERTA2_FWREG_BASEADDR_RANGE_START & 0xFFFF)) && ((reg_addr & 0xFFFF) <= (APERTA2_FWREG_BASEADDR_RANGE_END & 0xFFFF))) {
                *length = 32; *slave = APERTA2_FW_AHB;
            } else {
                *slave = APERTA2_DIRECT_ACCESS;
                if ((reg_addr >> 24) != 1) {
                    PHYMOD_DEBUG_INFO(("######Caution invalid slave:0x%x\n", reg_addr));
                }
            }
            break;
    }
    return PHYMOD_E_NONE;
}

/*!
 *  Get access type and length for memory
 *
 *  @param memory_type      Type of memory
 *  @param len              output, length of memory
 *  @param access_type      output, access type of the memory
 *  
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_mem_access_type_len(phymod_mem_type_t memory_type, uint8_t *len, uint8_t *access_type)
{
    switch (memory_type) {
        case phymodMemPmMib:
            {
                *access_type = 0xE0;
                *len = 64;
            }
            break;
        case phymodMemSpeedIdTable:
            {
                *access_type = 0xE2;
                *len = 18;
            }
            break;
        case phymodMemAMTable:
            {
                *access_type = 0xE3;
                *len = 10;
            }
            break;
        case phymodMemUMTable:
            {
                *access_type = 0xE4;
                *len = 8;
            }
            break;
        case phymodMemTxLkup1588Mpp0:
            {
                *access_type = 0xE5;
                *len = 10;
            }
            break;
        case phymodMemTxLkup1588Mpp1:
            {
                *access_type = 0xE6;
                *len = 10;
            }
            break;
        case phymodMemTxLkup1588100G_KR1_2XN:
            *access_type = 0xE7;
            *len = 10;
            break;

        case phymodMemRxLkup1588Mpp0:
            {
                *access_type = 0xE8;
                *len = 8;
            }
            break;
        case phymodMemRxLkup1588Mpp1:
            {
                *access_type = 0xE9;
                *len = 8;
            }
            break;
        case phymodMemSpeedPriorityMapTable:
            {
                *access_type = 0xEB;
                *len = 36;
            }
            break;
        case phymodMemTxTwostep1588Ts :
            *access_type = 0xEC;
            *len = 10;
            break;
        case phymodMemRsFecSymbErrMib:
            *access_type = 0xEF;
            *len = 16;
            break;
        case phymodMemFDR:
            *access_type=0xED;
            *len = 64;
            break;
        case phymodMemTxLkup1588400G:
        case phymodMemRxLkup1588400G:
        case phymodMemCount:
            return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}


