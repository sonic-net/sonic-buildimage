/********************************************************************************
 * Copyright(C) 2026 Micas Network. All rights reserved.
 ********************************************************************************
 * bcm_sdk_stubs.c - APERTA low-level register access
 * All register access uses _phy_sdk_bus_read/_phy_sdk_bus_write.
 ********************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "aperta_phy_sdk.h"

/* ==========================================================================
 * Register access helpers
 * ========================================================================== */
extern int _phy_sdk_bus_read(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t *data);
extern int _phy_sdk_bus_write(const plp_aperta_phymod_access_t *pa, uint32_t reg, uint32_t data);

/* ==========================================================================
 * PM info global array
 * ========================================================================== */
aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];

/* ==========================================================================
 * Indirect (LMI/FW) register access for non-direct slaves (PM_MAC, PM_PORT,
 * PM_TSC). it computes port/lane/pll/micro selects and routes by LMI slave:
 *   - APERTA_SLAVE_NONE                -> direct bus access
 *   - FW loaded (is_fw_dloaded==1)      -> firmware mailbox message
 *        (TSC_REGS for PM_TSC, PM_REGS for PM_MAC/PM_PORT)
 *   - FW not loaded                     -> LMI indirect access
 * A raw bus write to a TSC register (e.g. 0x1800d184) decodes to the wrong
 * register and corrupts the PHY (observed: all reads return 0x100 after a
 * direct core-dp-reset write in PASS2).
 * ========================================================================== */
extern int _pm_msg_send(const plp_aperta_phymod_phy_access_t *phy,
                        uint8_t function, uint8_t operation,
                        uint8_t *tx_msg, uint8_t *rx_msg, uint8_t *result);

#define APERTA_SLAVE_NONE       0
#define APERTA_PM_PORT          1
#define APERTA_PM_MAC           2
#define APERTA_PM_TSC           3

#define APERTA_LMI_CMDr         (0x19001 | 0x1000000)
#define APERTA_LMI_CMD_SEQr     (0x19006 | 0x1000000)
#define APERTA_LMI_STATUSr      (0x1900a | 0x1000000)
#define APERTA_LMI_RETRY_CNT    100
#define APERTA_LMI_SLEEP_TIME   100

#define APERTA_LINE_SIDE_PM     1
#define APERTA_SYS_SIDE_PM      9
#define APERTA_FUNC_PM_REGS     0x10
#define APERTA_FUNC_TSC_REGS    0x11
#define APERTA_FUNC_CHIP_IND_REGS 0x13
#define APERTA_OP_WRITE         0x0
#define APERTA_OP_READ          0x1
#define APERTA_OP_SUCCESS       0xE

static int _pm_count_no_bits(uint32_t v)
{
    int n = 0;
    while(v)
    {
        v &= (v - 1);
        n++;
    }
    return n;
}

static int _pm_log2n(uint32_t v)
{
    int n = 0;
    while(v > 1)
    {
        v >>= 1;
        n++;
    }
    return n;
}

static int _pm_lmi_slave_get(uint32_t reg_addr)
{
    switch (reg_addr >> 24) {
        case 0x10:
        case 0x11:
            return APERTA_PM_PORT;
        case 0x14:
        case 0x15:
            return APERTA_PM_MAC;
        case 0x18:
            return APERTA_PM_TSC;
        default:
            return APERTA_SLAVE_NONE;
    }
}

static int _pm_lmi_data_length(uint32_t reg_addr)
{
    switch (reg_addr >> 24) {
        case 0x10:
        case 0x11:
        case 0x14:
        case 0x15:
            return 32;
        case 0x18:
            return 16;
        default:
            return 16;
    }
}

static int _pm_lmi_slave_sel(int slave, int port_loc)
{
    if (slave == APERTA_PM_PORT || slave == APERTA_PM_MAC || slave == APERTA_PM_TSC) {
        return (port_loc == phymodPortLocSys) ? APERTA_SYS_SIDE_PM : APERTA_LINE_SIDE_PM;
    }
    return 0;
}

static int _pm_lmi_access_prog(uint32_t addr, int mac, int data_length, int reg_mask)
{
    int slave = _pm_lmi_slave_get(addr);
    int access_type = 0, stage_id = 0, per_port;

    if (slave == APERTA_PM_PORT || slave == APERTA_PM_MAC) {
        per_port = (addr & (1 << 17)) ? 0 : 1;
        if (per_port == 0 && data_length == 32) {
            access_type = 0;
        }
        else if (per_port == 1 && data_length == 32) {
            access_type = 1;
        }
        else if (per_port == 0 && data_length == 48) {
            access_type = 2;
        }
        else if (per_port == 1 && data_length == 48) {
            access_type = 3;
        }
        else if (per_port == 0 && data_length == 64) {
            access_type = 4;
        }
        else if (per_port == 1 && data_length == 64) {
            access_type = 5;
        }
    } else if (slave == APERTA_PM_TSC) {
        access_type = 6;
    }
    if (slave == APERTA_PM_PORT) {
        stage_id = 0;
    }
    else if (slave == APERTA_PM_MAC) {
        stage_id = (mac == 0) ? 1 : 2;
    }
    else if (slave == APERTA_PM_TSC) {
        stage_id = (reg_mask == 0) ? 0 : 1;
    }

    return (access_type & 7) | ((stage_id & 0xF) << 3);
}

/* local copy */
void _pm_get_port_from_lm_sp(uint32_t speed, uint32_t lane_rate, uint32_t lm, uint32_t *prt, uint32_t *ln_sel)
{
    if ((speed == APERTA_SPEED_100G || speed == APERTA_SPEED_40G) &&
        (lane_rate == 25781 || lane_rate == 25000 ||
         lane_rate == 10312 || lane_rate == 10000)) {
        if (lm == 0x0 || lm == 0xF) {
            *prt = 0;
            *ln_sel = 0xF;
        }
        else if (lm == 0xF0) {
            *prt = 4;
            *ln_sel = 0xF0;
        }
        else {
            *prt = (lm & 0xF) ? 0 : 4;
            *ln_sel = lm;
        }
    } else if ((speed == APERTA_SPEED_10G || speed == APERTA_SPEED_25G) ||
               ((speed == APERTA_SPEED_50G) && (lane_rate == 53125 || lane_rate == 50000))) {
        if (lm == 0x0 || lm == 0x1) 
        {
            *ln_sel = 1;
            *prt = 0;
        }
        else if (lm == 0x2) {
            *ln_sel = lm;
            *prt = 1;
        }
        else if (lm == 0x4) {
            *ln_sel = lm;
            *prt = 2;
        }
        else if (lm == 0x8) {
            *ln_sel = lm;
            *prt = 3;
        }
        else if (lm == 0x10) {
            *ln_sel = lm;
            *prt = 4;
        }
        else if (lm == 0x20) {
            *ln_sel = lm;
            *prt = 5;
        }
        else if (lm == 0x40) {
            *ln_sel = lm;
            *prt = 6;
        }
        else if (lm == 0x80) {
            *ln_sel = lm;
            *prt = 7;
        }
        else {
            *ln_sel = 1;
            *prt = 0;
        }
    } else if (speed == APERTA_SPEED_400G) {
        *ln_sel = lm;
        *prt = 0;
    } else if (speed == APERTA_SPEED_200G) {
        if (lm == 0x0 || lm == 0xF) {
            *ln_sel = 0xF;
            *prt = 0;
        }
        else if (lm == 0xF0) {
            *ln_sel = lm;
            *prt = 4;
        }
        else {
            *ln_sel = lm;
            *prt = 0;
        }
    } else if ((speed == APERTA_SPEED_50G && (lane_rate == 25781 || lane_rate == 25000)) ||
               (speed == APERTA_SPEED_100G && (lane_rate == 53125 || lane_rate == 50000)) ||
               (speed == APERTA_SPEED_40G && (lane_rate == 20000))) {
        if (lm == 0x3 || lm == 0) 
        {
            *prt = 0;
            *ln_sel = 0x3;
        }
        else if (lm == 0xC) {
            *ln_sel = lm;
            *prt = 2;
        }
        else if (lm == 0x30) {
            *ln_sel = lm;
            *prt = 4;
        }
        else if (lm == 0xC0) {
            *ln_sel = lm;
            *prt = 6;
        }
        else {
            *ln_sel = lm;
            *prt = (lm & 0x3) ? 0 : ((lm & 0xC) ? 2 : ((lm & 0x30) ? 4 : 6));
        }
    } else {
        *ln_sel = 1;
        *prt = 0;
    }
}

/* _pm_pm_info_speed_get + APERTA_GET_PORT_SPEED_LDR */
static int _pm_pm_info_speed_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *speed, uint32_t *lane_rate)
{
    unsigned short cnt, lane_index;
    uint32_t data = 0, lane_map = 0xF;

    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy->access.addr) {
            for (lane_index = 0; lane_index < APERTA_PM_NUM_LANES; lane_index++) {
                if (phy->access.lane_mask & (1 << lane_index)) {
                    data = (phy->port_loc == phymodPortLocLine) ?
                           _plp_aperta_pm_info[cnt].speed[lane_index] :
                           _plp_aperta_pm_info[cnt].sys_speed[lane_index];
                    *speed = data & 0xFFFFFF;
                    lane_map = (data >> 24) & 0xFF;
                    if (lane_map == 0x0) {
                        lane_map = (phy->access.lane_mask & 0xF) ? 0xF : 0xF0;
                    }
                    if (lane_map == 0x0) {
                        lane_map = 0xF;
                    }
                    *lane_rate = (*speed) / _pm_count_no_bits(lane_map);
                    return PHYMOD_E_NONE;
                }
            }
        }
    }
    *speed = 100000;
    *lane_rate = 25000;
    return PHYMOD_E_NONE;
}

/* _pm_pm_is_fw_dloaded_get - local copy */
static int _pm_is_fw_dloaded(uint32_t phy_id, uint32_t *active)
{
    unsigned short cnt;
    *active = 0;
    for (cnt = 0; cnt < APERTA_MAX_PM_INFO; cnt++) {
        if (_plp_aperta_pm_info[cnt].phy_id == phy_id) {
            *active = _plp_aperta_pm_info[cnt].is_fw_dloaded;
            break;
        }
    }
    return PHYMOD_E_NONE;
}

/* _pm_get_tsc_addr - compose TSC (dev 0) / PMD (dev 1) address */
static void _pm_get_tsc_addr(uint32_t *tsc_addr, uint32_t *pmd_addr,
                             uint32_t port_sel, uint32_t lane_sel,
                             uint32_t pll_sel, uint32_t micro_sel,
                             uint32_t reg_addr, int *device_type)
{
    uint32_t lane_mode;
    (void)port_sel;

    if ((reg_addr >= 0x18000090) && (reg_addr <= 0x1800009f)) {
        *device_type = 1; /* APERTA_PMD */
    } else {
        *device_type = (((reg_addr >> 12) & 0xF) == 0xD) ? 1 : 0;
    }
    if (*device_type == 0) {  /* TSC */
        uint32_t ls = lane_sel;
        if (ls & 0xF0) ls >>= 4;   /* high MPP lanes map to lane 0-3 mode */
        lane_mode = 0x6;
        if (ls == 0x1) lane_mode = 0x0;
        else if (ls == 0x2) lane_mode = 0x1;
        else if (ls == 0x4) lane_mode = 0x2;
        else if (ls == 0x8) lane_mode = 0x3;
        else if (ls == 0x3) lane_mode = 0x4;
        else if (ls == 0xC) lane_mode = 0x5;
        else lane_mode = 0x6;
        /* SDK APERTA_TSC_ADDR_TYPE_T (little-endian field order):
         * device[31:27] | reserved[26] | mpp1[25] | mpp0[24] | port_id[23:19]
         * | lane_mode[18:16] | reg_addr[15:0] */
        *tsc_addr = ((uint32_t)(*device_type) << 27) |
                    (((lane_sel & 0xF0) ? 1 : 0) << 25) |   /* mpp1 */
                    (((lane_sel & 0x0F) ? 1 : 0) << 24) |   /* mpp0 */
                    (0 << 19) |                              /* port_id */
                    (lane_mode << 16) |
                    (reg_addr & 0xFFFF);
        *pmd_addr = 0;
    } else {  /* PMD */
        lane_mode = 0x00;
        if (lane_sel == 0x2) lane_mode = 0x01;
        else if (lane_sel == 0x4) lane_mode = 0x02;
        else if (lane_sel == 0x8) lane_mode = 0x03;
        else if (lane_sel == 0x10) lane_mode = 0x04;
        else if (lane_sel == 0x20) lane_mode = 0x05;
        else if (lane_sel == 0x40) lane_mode = 0x06;
        else if (lane_sel == 0x80) lane_mode = 0x07;
        else if (lane_sel == 0x3) lane_mode = 0x20;
        else if (lane_sel == 0xC) lane_mode = 0x21;
        else if (lane_sel == 0x30) lane_mode = 0x22;
        else if (lane_sel == 0xC0) lane_mode = 0x23;
        else if (lane_sel == 0xF) lane_mode = 0x40;
        else if (lane_sel == 0xF0) lane_mode = 0x41;
        else if (lane_sel == 0xFF) lane_mode = 0xFF;
        else lane_mode = 0x00;
        (void)micro_sel;
        /* SDK APERTA_PMD_ADDR_TYPE_T (little-endian field order):
         * device[31:27] | pll_micro_brdcst[26] | pll_micro_sel[25:24]
         * | lane_mode[23:16] | reg_addr[15:0] */
        *pmd_addr = ((uint32_t)(*device_type) << 27) |
                    (0 << 26) |                      /* pll_micro_brdcst */
                    ((pll_sel & 0x3) << 24) |        /* pll_micro_sel */
                    (lane_mode << 16) |
                    (reg_addr & 0xFFFF);
        *tsc_addr = 0;
    }
}

/* LMI indirect register access */
static int _pm_lmi_reg_access(const plp_aperta_phymod_phy_access_t *phy,
                              uint32_t port_sel, uint32_t lane_sel,
                              uint32_t pll_sel, uint32_t micro_sel,
                              uint32_t reg_addr, uint32_t *data, int write_en)
{
    uint32_t lmi_cmd = 0, done = 0, retry_cnt = APERTA_LMI_RETRY_CNT;
    uint32_t slave = _pm_lmi_slave_get(reg_addr);
    int lmi_data_size = _pm_lmi_data_length(reg_addr);
    int mac = 0, tsc_addr = 0, pmd_addr = 0, device_type = 0;
    int access_prog, slave_data_count;
    uint32_t rd = 0;

    if (phy->access.lane_mask & 0xf) mac = 0; else mac = 1;

    access_prog = _pm_lmi_access_prog(reg_addr, mac, lmi_data_size, 0);
    lmi_cmd = ((uint32_t)_pm_lmi_slave_sel((int)slave, phy->port_loc) & 0xF) << 8;
    lmi_cmd |= ((uint32_t)access_prog & 0x7F) << 1;
    lmi_cmd |= (port_sel & 0x7) << 12;
    lmi_cmd |= (write_en & 1);

    if (slave == APERTA_PM_TSC) {
        _pm_get_tsc_addr((uint32_t*)&tsc_addr, (uint32_t*)&pmd_addr,
                         port_sel, lane_sel, pll_sel, micro_sel, reg_addr,
                         &device_type);
    }

    /* Wait DONE==0 */
    do {
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done));
        PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
    } while (((done & 1) != 0) && (--retry_cnt));
    if (retry_cnt == 0) return PHYMOD_E_TIMEOUT;
    retry_cnt = APERTA_LMI_RETRY_CNT;

    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, lmi_cmd));

    if (write_en) {
        for (slave_data_count = 0; slave_data_count < (lmi_data_size / 16); slave_data_count++) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr,
                (*data >> (16 * slave_data_count)) & 0xFFFF));
        }
    }

    if (slave == APERTA_PM_TSC) {
        uint32_t a = (device_type == 0) ? (uint32_t)tsc_addr : (uint32_t)pmd_addr;
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, a & 0xFFFF));
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, (a >> 16) & 0xFFFF));
    } else {
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_LMI_CMD_SEQr, reg_addr & 0xFFFF));
    }

    /* Wait DONE==1 */
    do {
        PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_LMI_STATUSr, &done));
        PHYMOD_USLEEP(APERTA_LMI_SLEEP_TIME);
    } while (((done & 1) != 1) && (--retry_cnt));
    if (retry_cnt == 0) return PHYMOD_E_TIMEOUT;

    if (!write_en) {
        for (slave_data_count = 0; slave_data_count < (lmi_data_size / 16); slave_data_count++) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_LMI_CMD_SEQr, &rd));
            rd = (rd & 0xFFFF) << (16 * slave_data_count);
            if (slave_data_count == 0) *data = rd;
            else *data |= rd;
        }
    }
    return PHYMOD_E_NONE;
}

/* FW message byte-buffer helpers (LITTLE-ENDIAN, matching plp_aperta_put_*: the
 * SDK's put_half_word/put_word write the LSB first). This is critical - the
 * FW mailbox protocol is little-endian; using big-endian scrambled every
 * message's side/cnt/addr fields. */
static void _pm_msg_put_byte(uint8_t **buf, uint8_t val)
{
    *(*buf)++ = val;
}

static void _pm_msg_put_half_word(uint8_t **buf, uint16_t val)
{
    *(*buf)++ = (uint8_t)(val & 0xFF);
    *(*buf)++ = (uint8_t)(val >> 8);
}

static void _pm_msg_put_word(uint8_t **buf, uint32_t val)
{
    *(*buf)++ = (uint8_t)(val & 0xFF);
    *(*buf)++ = (uint8_t)((val >> 8) & 0xFF);
    *(*buf)++ = (uint8_t)((val >> 16) & 0xFF);
    *(*buf)++ = (uint8_t)((val >> 24) & 0xFF);
}

/* FW register access */
static int _pm_fw_reg_access(const plp_aperta_phymod_phy_access_t *phy,
                             uint32_t port_sel, uint32_t lane_sel,
                             uint32_t reg_addr, uint32_t *data, int write_en)
{
    uint8_t tx_buf[256], rx_buf[256];
    uint8_t *tx_msg, result = 0;
    uint32_t slave = _pm_lmi_slave_get(reg_addr);
    uint8_t block;
    uint32_t fw_lane, gen_access, reg_data, side;
    int tsc_addr = 0, pmd_addr = 0, device_type = 0, rv;
    int i;

    (void)port_sel;
    PHYMOD_MEMSET(tx_buf, 0, sizeof(tx_buf));
    PHYMOD_MEMSET(rx_buf, 0, sizeof(rx_buf));
    tx_msg = &tx_buf[0];
    side = (phy->port_loc == phymodPortLocSys) ? 0 : 1;
    reg_data = *data;

    if (slave == APERTA_PM_TSC) {
        _pm_get_tsc_addr((uint32_t*)&tsc_addr, (uint32_t*)&pmd_addr,
                         port_sel, lane_sel, phy->access.pll_idx, 0,
                         reg_addr, &device_type);
        block = APERTA_FUNC_TSC_REGS;
        if (write_en) {
            _pm_msg_put_half_word(&tx_msg, 9);   /* write: side+mask+cnt+addr+data16 */
            _pm_msg_put_byte(&tx_msg, (uint8_t)side);
            _pm_msg_put_byte(&tx_msg, 0x00);   /* Mask */
            _pm_msg_put_byte(&tx_msg, 0x01);   /* Cnt */
            _pm_msg_put_word(&tx_msg, (device_type == 0) ? (uint32_t)tsc_addr : (uint32_t)pmd_addr);
            _pm_msg_put_half_word(&tx_msg, (uint16_t)(reg_data & 0xFFFF));
        } else {
            _pm_msg_put_half_word(&tx_msg, 6);   /* read: side+cnt+addr */
            _pm_msg_put_byte(&tx_msg, (uint8_t)side);
            _pm_msg_put_byte(&tx_msg, 0x01);   /* Cnt */
            _pm_msg_put_word(&tx_msg, (device_type == 0) ? (uint32_t)tsc_addr : (uint32_t)pmd_addr);
        }
    } else if (slave == APERTA_PM_PORT || slave == APERTA_PM_MAC) {
        uint8_t access_type = 0;
        gen_access = (reg_addr & (1 << 17)) ? 1 : 0;
        fw_lane = _pm_log2n(lane_sel & ~(lane_sel - 1));
        if (slave == APERTA_PM_MAC) {
            if (gen_access) {
                access_type = (phy->access.lane_mask & 0xF) ? 0x10 : 0x20;
            } else {
                if (phy->access.lane_mask & 0xF) {
                    access_type = (uint8_t)(0x90 | fw_lane);
                } else {
                    access_type = (uint8_t)(0xA0 | (fw_lane - 4));
                }
            }
        } else { /* PM_PORT */
            access_type = gen_access ? 0x0 : (uint8_t)(0x80 | fw_lane);
        }
        block = APERTA_FUNC_PM_REGS;
        if (write_en) {
            _pm_msg_put_half_word(&tx_msg, 0x0007 + 4);
            _pm_msg_put_byte(&tx_msg, (uint8_t)side);
            _pm_msg_put_byte(&tx_msg, access_type);
            _pm_msg_put_byte(&tx_msg, 4);    /* DataLen */
            _pm_msg_put_byte(&tx_msg, 0x00); /* Mask */
            _pm_msg_put_byte(&tx_msg, 0x01); /* Cnt */
            _pm_msg_put_half_word(&tx_msg, (uint16_t)(reg_addr & 0xFFFF));
            for (i = 0; i < 4; i++) _pm_msg_put_byte(&tx_msg, (reg_data >> (i * 8)) & 0xFF);
        } else {
            _pm_msg_put_half_word(&tx_msg, 6);   /* read: side+access_type+datalen+cnt+addr */
            _pm_msg_put_byte(&tx_msg, (uint8_t)side);
            _pm_msg_put_byte(&tx_msg, access_type);
            _pm_msg_put_byte(&tx_msg, 4);    /* DataLen */
            _pm_msg_put_byte(&tx_msg, 0x01); /* Cnt */
            _pm_msg_put_half_word(&tx_msg, (uint16_t)(reg_addr & 0xFFFF));
        }
    } else {
        return PHYMOD_E_UNAVAIL;
    }

    rv = _pm_msg_send(phy, block, write_en ? APERTA_OP_WRITE : APERTA_OP_READ,
                      tx_buf, rx_buf, &result);
    if (rv != PHYMOD_E_NONE) {
        return rv;
    }
    if (APERTA_OP_SUCCESS != result) {
        return PHYMOD_E_INTERNAL;
    }

    /* Parse the read response back into *data.
     * rx_buf is filled LITTLE-ENDIAN by _pm_msg_put_half_word (matching the
     * FW protocol): rx_buf layout = [len_lo, len_hi, data_lo, data_hi, ...]. */
    if (!write_en) {
        uint16_t rlen = (uint16_t)(rx_buf[0] | ((uint16_t)rx_buf[1] << 8));
        if (block == APERTA_FUNC_TSC_REGS) {
            if (rlen == 2) {
                *data = (uint32_t)(rx_buf[2] | ((uint16_t)rx_buf[3] << 8));
            } else {
                LOG_INFO("FW TSC read len=%d unexpected", rlen);
                return PHYMOD_E_INTERNAL;
            }
        } else { /* PM_REGS */
            if (rlen == 4) {
                *data = (uint32_t)rx_buf[2] | ((uint32_t)rx_buf[3] << 8) |
                        ((uint32_t)rx_buf[4] << 16) | ((uint32_t)rx_buf[5] << 24);
            } else {
                LOG_INFO("FW PM read len=%d unexpected", rlen);
                return PHYMOD_E_INTERNAL;
            }
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_reg32_read(const plp_aperta_phymod_phy_access_t *phy,
                          uint32_t reg_addr, uint32_t *data)
{
    uint32_t speed = 0, lane_rate = 0, port_sel = 0, lane_sel = 0;
    uint32_t pll_sel, micro_sel = 0, is_dloaded = 0;
    uint32_t slave;

    if (phy == NULL || data == NULL) {
        return PHYMOD_E_PARAM;
    }

    slave = _pm_lmi_slave_get(reg_addr);
    if (slave == APERTA_SLAVE_NONE) {
        return _phy_sdk_bus_read(&phy->access, reg_addr, data);
    }
    _pm_pm_info_speed_get(phy, &speed, &lane_rate);
    pll_sel = phy->access.pll_idx;
    _pm_get_port_from_lm_sp(speed, lane_rate, phy->access.lane_mask, &port_sel, &lane_sel);
    if (lane_sel == 0xFF || lane_sel == 0xF) {
        lane_sel = 1;
    } else {
        lane_sel = lane_sel & ~(lane_sel - 1);
    }

    _pm_is_fw_dloaded(phy->access.addr, &is_dloaded);
    if (is_dloaded) {
        return _pm_fw_reg_access(phy, port_sel, lane_sel, reg_addr, data, 0);
    }
    return _pm_lmi_reg_access(phy, port_sel, lane_sel, pll_sel, micro_sel, reg_addr, data, 0);
}

int plp_aperta_reg32_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t data)
{
    uint32_t speed = 0, lane_rate = 0, port_sel = 0, lane_sel = 0;
    uint32_t pll_sel, micro_sel = 0, is_dloaded = 0;
    uint32_t slave;

    if (phy == NULL) {
        return PHYMOD_E_PARAM;
    }

    slave = _pm_lmi_slave_get(reg_addr);
    if (slave == APERTA_SLAVE_NONE) {
        return _phy_sdk_bus_write(&phy->access, reg_addr, data);
    }
    _pm_pm_info_speed_get(phy, &speed, &lane_rate);
    pll_sel = phy->access.pll_idx;
    _pm_get_port_from_lm_sp(speed, lane_rate, phy->access.lane_mask, &port_sel, &lane_sel);
    if (lane_sel == 0xFF || lane_sel == 0xF) {
        lane_sel = 1;
    } else {
        lane_sel = lane_sel & ~(lane_sel - 1);
    }

    _pm_is_fw_dloaded(phy->access.addr, &is_dloaded);
    if (is_dloaded) {
        return _pm_fw_reg_access(phy, port_sel, lane_sel, reg_addr, &data, 1);
    }
    return _pm_lmi_reg_access(phy, port_sel, lane_sel, pll_sel, micro_sel,
                              reg_addr, &data, 1);
}

/* ==========================================================================
 * plp_aperta_direct_reg_read / plp_aperta_direct_reg_write
 * Direct register access (passthrough to bus read/write).
 * ========================================================================== */
int plp_aperta_direct_reg_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *reg_data)
{
    if (phy == NULL || reg_data == NULL) {
        return PHYMOD_E_PARAM;
    }
    return _phy_sdk_bus_read(&phy->access, reg_addr, reg_data);
}

int plp_aperta_direct_reg_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t reg_data)
{
    if (phy == NULL) {
        return PHYMOD_E_PARAM;
    }
    return _phy_sdk_bus_write(&phy->access, reg_addr, reg_data);
}

/* ==========================================================================
 * plp_aperta_phymod_util_lane_config_get - resolve lane config from lane_mask
 * ========================================================================== */
int plp_aperta_phymod_util_lane_config_get(const plp_aperta_phymod_access_t *phys, int *start_lane, int *num_of_lane)
{
    switch (phys->lane_mask) {
        case 0x1:   *start_lane = 0; *num_of_lane = 1; break;
        case 0x2:   *start_lane = 1; *num_of_lane = 1; break;
        case 0x4:   *start_lane = 2; *num_of_lane = 1; break;
        case 0x8:   *start_lane = 3; *num_of_lane = 1; break;
        case 0x10:  *start_lane = 4; *num_of_lane = 1; break;
        case 0x20:  *start_lane = 5; *num_of_lane = 1; break;
        case 0x40:  *start_lane = 6; *num_of_lane = 1; break;
        case 0x80:  *start_lane = 7; *num_of_lane = 1; break;
        case 0x3:   *start_lane = 0; *num_of_lane = 2; break;
        case 0xc:   *start_lane = 2; *num_of_lane = 2; break;
        case 0x30:  *start_lane = 4; *num_of_lane = 2; break;
        case 0xc0:  *start_lane = 6; *num_of_lane = 2; break;
        case 0x7:   *start_lane = 0; *num_of_lane = 3; break;
        case 0xf:   *start_lane = 0; *num_of_lane = 4; break;
        case 0xf0:  *start_lane = 4; *num_of_lane = 4; break;
        case 0xff:  *start_lane = 0; *num_of_lane = 8; break;
        default:
            *start_lane = 0;
            *num_of_lane = 1;
            break;
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * TSCBH register base address
 * ========================================================================== */
#define PHYMOD_REG_APERTA_TSCBH     0x18000000
#define APERTA_TSCBH_NOF_LANES_IN_CORE      8

/* Blackhawk TSC register base.
 * TSC digital regs (0xd0xx-0xd2xx) live at 0x1800d000; low PMD uC regs
 * (<=0x9c, e.g. link-training 0x96-0x9b) live at 0x18000000. */
#define APERTA_PM_TSC_BLACKHAWK_BASEADR   0x1800d000u

/* Compose the full bus address for a Blackhawk TSC register, matching the */
static uint32_t _bh_tsc_addr(uint16_t addr)
{
    if (addr > 0x9c) {
        return APERTA_PM_TSC_BLACKHAWK_BASEADR | addr;
    }
    return (APERTA_PM_TSC_BLACKHAWK_BASEADR & ~(0xF000u)) | addr;
}

/* ---- TX_LN_SWP register (0x18009200) ---- */
#define TX_X1_TX_LN_SWPr                   (0x00009200 | PHYMOD_REG_APERTA_TSCBH)
#define TX_X1_TX_LN_SWPr_SET(r,d)          (r).tx_x1_tx_ln_swp[0] = d
/* NOTE: fields are 3-bit each (bits 0/3/6/9), NOT nibbles */
#define TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_GET(r) (((r).tx_x1_tx_ln_swp[0]) & 0x7)
#define TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_GET(r) ((((r).tx_x1_tx_ln_swp[0]) >> 3) & 0x7)
#define TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_GET(r) ((((r).tx_x1_tx_ln_swp[0]) >> 6) & 0x7)
#define TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_GET(r) ((((r).tx_x1_tx_ln_swp[0]) >> 9) & 0x7)

/* Blackhawk low-level register helpers */
static int __bh_rdt_reg(const plp_aperta_phymod_phy_access_t *phy, uint16_t addr, uint16_t *val)
{
    uint32_t rd;
    uint32_t full_addr = _bh_tsc_addr(addr);
    int rv = plp_aperta_reg32_read(phy, full_addr, &rd);
    if (rv == PHYMOD_E_NONE) {
        *val = (uint16_t)(rd & 0xFFFF);
    }
    return rv;
}

static int __bh_wr_reg(const plp_aperta_phymod_phy_access_t *phy, uint16_t addr, uint16_t val)
{
    uint32_t full_addr = _bh_tsc_addr(addr);
    return plp_aperta_reg32_write(phy, full_addr, val);
}

/* ==========================================================================
 * Blackhawk TSC low-level helpers
 * ========================================================================== */
#define APERTA_BH_CORE_DP_S_RSTBr        0xd184   /* bit13 core dp reset */
#define APERTA_BH_UC_ACTIVEr             0xd101   /* bit1  uc active     */
#define APERTA_BH_AMS_TX_VERSION_IDr     0xd0d9   /* [7:0] ams tx version */
#define APERTA_BH_REVID_MULTIPLICITYr    0xd10a   /* [15:12] multiplicity */
#define APERTA_BH_MICRO_NUM_UC_CORESr    0xd21a   /* [15:12] #micros, [3:0] sdk status */
#define APERTA_BH_MICRO_MASTER_CLK_ENr   0xd200
#define APERTA_BH_MICRO_MASTER_RSTBr     0xd201
#define APERTA_BH_MICRO_CR_ACCESS_ENr    0xd227
#define APERTA_BH_MICRO_CORE_CLK_ENr     0xd240
#define APERTA_BH_MICRO_CORE_RSTBr       0xd241

/* PCS lane-swap / lane-addr registers */
#define APERTA_BH_TX_LN_SWPr             (0x00009200 | PHYMOD_REG_APERTA_TSCBH)
#define APERTA_BH_RX_LN_SWPr             (0x00009225 | PHYMOD_REG_APERTA_TSCBH)
/* lane-addr table: tx sel in [12:8], rx sel in [4:0] of 0xd190+n */
#define APERTA_BH_TX_LANE_ADDRr(n)       (0xd190 + (n))

/* Valid-bit pattern written alongside the 3-bit lane selects (macros:
 * field<n> |= 7 << (16 + 3*n) => 0x0F8F0000). */
#define APERTA_BH_LN_SWP_VALID           0x0F8F0000u

/* Read field: val = (reg << shift_left) >> shift_right (SDK rde_field_byte) */
static int _bh_rde_field_byte(const plp_aperta_phymod_phy_access_t *phy,
                              uint16_t addr, uint8_t shift_left, uint8_t shift_right, uint8_t *val)
{
    uint32_t reg;
    uint16_t v16;
    if (plp_aperta_reg32_read(phy, _bh_tsc_addr(addr), &reg) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    /* so bits shifted above bit15 are DISCARDED (e.g. (reg<<2)>>15 == bit13
     * only, NOT bits[15:13]). */
    v16 = (uint16_t)(reg & 0xFFFF);
    v16 = (uint16_t)(v16 << shift_left);
    v16 = (uint16_t)(v16 >> shift_right);
    *val = (uint8_t)v16;
    return PHYMOD_E_NONE;
}

/* Read-modify-write a field once per lane in lane_mask */
static int _bh_mwr_reg_byte(const plp_aperta_phymod_phy_access_t *phy,
                            uint16_t addr, uint16_t mask, uint8_t lsb, uint16_t val)
{
    plp_aperta_phymod_phy_access_t pc;
    uint32_t full_addr = _bh_tsc_addr(addr);
    uint32_t i, reg_val;
    uint16_t tmp;

    val = (val << lsb) & mask;
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    for (i = 1; i <= 0x80; i <<= 1) {
        if (i & phy->access.lane_mask) {
            pc.access.lane_mask = i;
            if (plp_aperta_reg32_read(&pc, full_addr, &reg_val) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            tmp = (uint16_t)(reg_val & 0xFFFF);
            tmp &= (uint16_t)~mask;
            tmp |= (uint16_t)val;
            if (plp_aperta_reg32_write(&pc, full_addr, tmp) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
    }
    return PHYMOD_E_NONE;
}

/* Direct 16-bit reg write to TSC (SDK pmd_wr_reg / acc_wr_reg) */
static int _bh_acc_wr_reg(const plp_aperta_phymod_phy_access_t *phy, uint16_t addr, uint16_t val)
{
    return plp_aperta_reg32_write(phy, _bh_tsc_addr(addr), val & 0xFFFF);
}

static uint8_t _bh_get_micro_idx(const plp_aperta_phymod_phy_access_t *phy)
{
    return (uint8_t)(phy->access.pll_idx & 0xF);
}

static void _bh_set_micro_idx(plp_aperta_phymod_phy_access_t *phy, uint8_t idx)
{
    phy->access.pll_idx = idx;
}

static int _bh_get_micro_num_uc_cores(const plp_aperta_phymod_phy_access_t *phy, uint8_t *num)
{
    return _bh_rde_field_byte(phy, APERTA_BH_MICRO_NUM_UC_CORESr, 0, 12, num);
}

/* Core data-path reset: assert(enable=1) -> core_dp_s_rstb=0 */
static int _bh_core_dp_reset(const plp_aperta_phymod_phy_access_t *phy, uint8_t enable)
{
    return _bh_mwr_reg_byte(phy, APERTA_BH_CORE_DP_S_RSTBr, 0x2000, 13, enable ? 0 : 1);
}

static int _bh_ip_version_check(const plp_aperta_phymod_phy_access_t *phy, uint8_t *i_am_b0)
{
    uint8_t v;
    if (_bh_rde_field_byte(phy, APERTA_BH_AMS_TX_VERSION_IDr, 8, 8, &v) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    *i_am_b0 = ((v & 0xF0) >= 0xC0) ? 1 : 0;
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * _bh_uc_reset_with_info - assert/release TSC microcontroller reset
 * ========================================================================== */
static int _bh_uc_reset_with_info(plp_aperta_phymod_phy_access_t *phy, uint8_t enable, uint16_t stack_size, uint32_t ucode_size)
{
    uint8_t micro_orig, num_micros = 0;
    int micro_idx;

    if (enable) {
        /* Assert micro reset and reset all micro registers to defaults */
        if (_bh_mwr_reg_byte(phy, APERTA_BH_UC_ACTIVEr, 0x0002, 1, 0) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        micro_orig = _bh_get_micro_idx(phy);
        if (_bh_get_micro_num_uc_cores(phy, &num_micros) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        for (micro_idx = 0; micro_idx < (int)num_micros; micro_idx++) {
            _bh_set_micro_idx(phy, (uint8_t)micro_idx);
            if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_CORE_RSTBr, 0x0001, 0, 0) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_CORE_CLK_ENr, 0x0001, 0, 0) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
        _bh_set_micro_idx(phy, micro_orig);
        if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_MASTER_CLK_ENr, 0x0001, 0, 0) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }

        {
            static const struct { uint16_t a, v; } w[] = {
                {0xD200,0x0000},{0xD201,0x4000},{0xD202,0x0000},{0xD204,0x0000},
                {0xD205,0x0000},{0xD206,0x0000},{0xD207,0x0000},{0xD208,0x0000},
                {0xD209,0x0000},{0xD20A,0x0000},{0xD20B,0x0000},{0xD20C,0x0000},
                {0xD20D,0x0000},{0xD20E,0x0000},{0xD211,0x0000},{0xD212,0x0000},
                {0xD213,0x0000},{0xD214,0x0000},{0xD215,0x0000},{0xD216,0x0000},
                {0xD217,0x0003},{0xD218,0xffff},{0xD219,0x0e01},{0xD220,0x0000},
                {0xD221,0x0000},{0xD225,0x0000},{0xD226,0x0000},{0xD227,0x8382},
                {0xD229,0x0000},{0xD22B,0x0000},{0xD22C,0x0000},{0xD230,0x0000},
                {0xD231,0x0000},{0xD232,0x0000},{0xD233,0x0000},
            };
            unsigned int i;
            for (i = 0; i < sizeof(w) / sizeof(w[0]); i++) {
                if (_bh_acc_wr_reg(phy, w[i].a, w[i].v) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
            }
        }
        /* Per-micro registers */
        micro_orig = _bh_get_micro_idx(phy);
        if (_bh_get_micro_num_uc_cores(phy, &num_micros) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        for (micro_idx = 0; micro_idx < (int)num_micros; micro_idx++) {
            static const uint16_t pm[] = {0xD240,0xD241,0xD244,0xD245,0xD24D,0xD24E};
            static const uint16_t pv[] = {0x0000,0x8000,0x0000,0x000e,0x0000,0x0000};
            unsigned int i;
            _bh_set_micro_idx(phy, (uint8_t)micro_idx);
            for (i = 0; i < sizeof(pm) / sizeof(pm[0]); i++) {
                if (_bh_acc_wr_reg(phy, pm[i], pv[i]) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
            }
        }
        _bh_set_micro_idx(phy, micro_orig);
        if (_bh_acc_wr_reg(phy, 0xD22B, 0x0000) != PHYMOD_E_NONE) { /* fast_read_en=0 */
            return PHYMOD_E_FAIL;
        }

        /* Stack size (default 0x7f4 = 8 lanes) */
        {
            uint16_t stk = (stack_size != 0) ? stack_size : 0x7f4;
            if (_bh_acc_wr_reg(phy, 0xD22E, (uint16_t)(0x8000 | (stk << 2))) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
        /* CODE/DATA RAM config based on ucode size + IP version */
        {
            uint8_t i_am_b0 = 0;
            uint16_t ram_config_val = 0;
            if (_bh_ip_version_check(phy, &i_am_b0) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            if (i_am_b0) {
                if (ucode_size <= 0x14000) {
                    ram_config_val = 0x0000;
                }
                else if (ucode_size <= 0x15000) {
                    ram_config_val = 0x0400;
                }
                else {
                    return PHYMOD_E_CONFIG;
                }
            } else {
                if (ucode_size <= 0x10000) {
                    ram_config_val = 0x0000;
                }
                else if (ucode_size <= 0x10800) {
                    ram_config_val = 0x0200;
                }
                else if (ucode_size <= 0x11000) {
                    ram_config_val = 0x0400;
                }
                else if (ucode_size <= 0x11C00) {
                    ram_config_val = 0x0700;
                }
                else {
                    return PHYMOD_E_CONFIG;
                }
            }
            if (_bh_acc_wr_reg(phy, 0xD228, ram_config_val) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
    } else {
        /* De-assert micro reset - start executing code */
        if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_MASTER_CLK_ENr, 0x0001, 0, 1) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_MASTER_RSTBr, 0x0001, 0, 1) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_CR_ACCESS_ENr, 0x0001, 0, 1) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        micro_orig = _bh_get_micro_idx(phy);
        if (_bh_get_micro_num_uc_cores(phy, &num_micros) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        for (micro_idx = (int)(num_micros - 1); micro_idx >= 0; micro_idx--) {
            _bh_set_micro_idx(phy, (uint8_t)micro_idx);
            if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_CORE_CLK_ENr, 0x0001, 0, 1) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            if (_bh_mwr_reg_byte(phy, APERTA_BH_MICRO_CORE_RSTBr, 0x0001, 0, 1) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
        _bh_set_micro_idx(phy, micro_orig);
    }
    return PHYMOD_E_NONE;
}

/* Wait for uC to become active */
static int _bh_wait_uc_active(const plp_aperta_phymod_phy_access_t *phy)
{
    uint32_t loop, rddata;
    for (loop = 0; loop < 10000; loop++) {
        uint8_t core_uc_active = 0;
        if (_bh_rde_field_byte(phy, APERTA_BH_UC_ACTIVEr, 14, 15, &core_uc_active) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        if (plp_aperta_reg32_read(phy, PHYMOD_REG_APERTA_TSCBH | APERTA_BH_MICRO_NUM_UC_CORESr, &rddata) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        if (((rddata & 0xF) == 0xF) && (core_uc_active == 1)) {
            return PHYMOD_E_NONE;
        }
        if ((loop % 2000) == 0 || loop == 9999) {
            LOG_INFO("wait_uc_active: loop=%u uc_active(0xd101 bit1)=%d micro_status(0xd21a)=0x%x",
                     (unsigned)loop, core_uc_active, (unsigned)(rddata & 0xFFFF));
        }
        if (loop > 10) {
            PHYMOD_USLEEP(1);
        }
    }
    return PHYMOD_E_TIMEOUT;
}

/* PCS TX lane swap: MPP0/MPP1, Registers at 0x18009200; lane_mask selects MPP0(0x1)/MPP1(0x10). */
static int _bh_pcs_tx_lane_swap(const plp_aperta_phymod_phy_access_t *phy,
                                uint32_t tx_lane_swap)
{
    plp_aperta_phymod_phy_access_t pc;
    uint8_t physical[APERTA_TSCBH_NOF_LANES_IN_CORE], lane;
    uint32_t val;

    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        physical[((tx_lane_swap >> (lane * 4)) & 0xf)] = lane;
    }
    /* Adjust MPP1 logical lane offset */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE / 2; lane++) {
        if (physical[4 + lane] < 4) {
            physical[4 + lane] += 4;
        } else {
            physical[4 + lane] -= 4;
        }
    }
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    /* MPP0: physical lanes 0-3 */
    pc.access.lane_mask = 0x1 << 0;
    val = (physical[0] & 0x7) | ((physical[1] & 0x7) << 3) |
          ((physical[2] & 0x7) << 6) | ((physical[3] & 0x7) << 9) |
          APERTA_BH_LN_SWP_VALID;
    if (plp_aperta_reg32_write(&pc, APERTA_BH_TX_LN_SWPr, val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    /* MPP1: physical lanes 4-7 */
    pc.access.lane_mask = 0x1 << 4;
    val = (physical[4] & 0x7) | ((physical[5] & 0x7) << 3) |
          ((physical[6] & 0x7) << 6) | ((physical[7] & 0x7) << 9) |
          APERTA_BH_LN_SWP_VALID;
    if (plp_aperta_reg32_write(&pc, APERTA_BH_TX_LN_SWPr, val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    return PHYMOD_E_NONE;
}

/* PCS RX lane swap: MPP0/MPP1 0x18009225. */
static int _bh_pcs_rx_lane_swap(const plp_aperta_phymod_phy_access_t *phy, uint32_t rx_lane_swap)
{
    plp_aperta_phymod_phy_access_t pc;
    uint8_t physical[APERTA_TSCBH_NOF_LANES_IN_CORE], lane;
    uint32_t val;

    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        physical[((rx_lane_swap >> (lane * 4)) & 0xf)] = lane;
    }
    PHYMOD_MEMCPY(&pc, phy, sizeof(pc));
    pc.access.lane_mask = 0x1 << 0;
    val = (physical[0] & 0x7) | ((physical[1] & 0x7) << 3) |
          ((physical[2] & 0x7) << 6) | ((physical[3] & 0x7) << 9) |
          APERTA_BH_LN_SWP_VALID;
    if (plp_aperta_reg32_write(&pc, APERTA_BH_RX_LN_SWPr, val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    pc.access.lane_mask = 0x1 << 4;
    val = (physical[4] & 0x7) | ((physical[5] & 0x7) << 3) |
          ((physical[6] & 0x7) << 6) | ((physical[7] & 0x7) << 9) |
          APERTA_BH_LN_SWP_VALID;
    if (plp_aperta_reg32_write(&pc, APERTA_BH_RX_LN_SWPr, val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    return PHYMOD_E_NONE;
}

/* Write PMD lane-address map: 0xd190+n,
 * tx sel [12:8], rx sel [4:0]. Requires core DP reset + micros held in reset. */
static int _bh_map_lanes(plp_aperta_phymod_phy_access_t *phy,
                         uint8_t num_lanes, const uint8_t *tx_lane_map,
                         const uint8_t *rx_lane_map)
{
    uint8_t rd_val = 0, micro_orig, num_micros = 0, micro_idx;
    uint8_t i1, i2;

    /* Verify core data path is held in reset */
    if (_bh_rde_field_byte(phy, APERTA_BH_CORE_DP_S_RSTBr, 2, 15, &rd_val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    if (rd_val != 0) {
        LOG_INFO("lane_map: core DP reset not held (core_dp_s_rstb=0x%x)", rd_val);
        return PHYMOD_E_FAIL;
    }

    /* Verify all micros are held in reset */
    micro_orig = _bh_get_micro_idx(phy);
    if (_bh_get_micro_num_uc_cores(phy, &num_micros) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    rd_val = 0;
    for (micro_idx = 0; micro_idx < num_micros; micro_idx++) {
        uint8_t v;
        _bh_set_micro_idx(phy, micro_idx);
        if (_bh_rde_field_byte(phy, APERTA_BH_MICRO_CORE_RSTBr, 15, 15, &v) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        rd_val |= v;
    }
    _bh_set_micro_idx(phy, micro_orig);
    if (rd_val != 0) {
        LOG_INFO("lane_map: micro reset not held (micro_core_rstb=0x%x)", rd_val);
        return PHYMOD_E_FAIL;
    }

    /* Verify multiplicity */
    if (_bh_rde_field_byte(phy, APERTA_BH_REVID_MULTIPLICITYr, 0, 12, &rd_val) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    if (rd_val != num_lanes) {
        LOG_INFO("lane_map: revid_multiplicity=0x%x != %d", rd_val, num_lanes);
        return PHYMOD_E_FAIL;
    }

    /* Validate lane maps */
    for (i1 = 0; i1 < num_lanes; i1++) {
        if ((tx_lane_map[i1] >= num_lanes) || (rx_lane_map[i1] >= num_lanes)) {
            return PHYMOD_E_CONFIG;
        }
        for (i2 = (uint8_t)(i1 + 1); i2 < num_lanes; i2++) {
            if ((tx_lane_map[i1] == tx_lane_map[i2]) ||(rx_lane_map[i1] == rx_lane_map[i2])) {
                return PHYMOD_E_CONFIG;
            }
        }
    }

    /* Write lane-addr bitfields */
    for (i1 = 0; i1 < num_lanes; i1++) {
        if (_bh_mwr_reg_byte(phy, APERTA_BH_TX_LANE_ADDRr(i1), 0x1f00, 8, tx_lane_map[i1]) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        if (_bh_mwr_reg_byte(phy, APERTA_BH_TX_LANE_ADDRr(i1), 0x001f, 0, rx_lane_map[i1]) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
    }
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_core_lane_map_set - write TSCBH TX/RX lane swap registers
 * (full path: core DP reset -> micro reset -> PCS TX/RX lane swap -> PMD lane map ->
 * micro release -> wait uc active -> core DP release).
 * ========================================================================== */
int plp_aperta_tscbh_core_lane_map_set(const plp_aperta_phymod_phy_access_t *core, const plp_aperta_phymod_lane_map_t *lane_map)
{
    plp_aperta_phymod_phy_access_t phy_copy;
    uint32_t lane, pcs_tx_swap = 0, pcs_rx_swap = 0, reg_val = 0;
    uint8_t pmd_tx_addr[APERTA_TSCBH_NOF_LANES_IN_CORE];
    uint8_t pmd_rx_addr[APERTA_TSCBH_NOF_LANES_IN_CORE];
    uint16_t stack_size;
    uint32_t ucode_size;  /* 83921 (0x147D1) overflows uint16_t! */

    if (lane_map->num_of_lanes != APERTA_TSCBH_NOF_LANES_IN_CORE) {
        return PHYMOD_E_CONFIG;
    }

    PHYMOD_MEMCPY(&phy_copy, core, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1;

    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        if ((lane_map->lane_map_tx[lane] >= APERTA_TSCBH_NOF_LANES_IN_CORE) ||
            (lane_map->lane_map_rx[lane] >= APERTA_TSCBH_NOF_LANES_IN_CORE)) {
            return PHYMOD_E_CONFIG;
        }

        pcs_tx_swap += lane_map->lane_map_tx[lane] << (lane * 4);
        pcs_rx_swap += lane_map->lane_map_rx[lane] << (lane * 4);
    }
    /* PMD lane addr is based on PCS logical-to-physical mapping */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        pmd_tx_addr[(pcs_tx_swap >> (lane * 4)) & 0xf] = lane;
        pmd_rx_addr[(pcs_rx_swap >> (lane * 4)) & 0xf] = lane;
    }

    /* ucode info (SDK reads live stack-size / RAM-config regs) */
    {
        uint32_t d22e = 0, d228 = 0;
        if (plp_aperta_reg32_read(&phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xd22e,
                                  &d22e) != PHYMOD_E_NONE) {
            LOG_INFO("lane_map_set: read 0xd22e failed");
            return PHYMOD_E_FAIL;
        }
        if (plp_aperta_reg32_read(&phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xd228,
                                  &d228) != PHYMOD_E_NONE) {
            LOG_INFO("lane_map_set: read 0xd228 failed");
            return PHYMOD_E_FAIL;
        }
        stack_size = (uint16_t)((d22e & 0x3FFC) >> 2);
        ucode_size = (d228 == 0) ? 75000 : 83921;
        LOG_INFO("lane_map_set: ucode_info d22e=0x%x d228=0x%x stack_size=0x%x ucode_size=0x%x",
                 (unsigned)d22e, (unsigned)d228, stack_size, ucode_size);
    }

    /* Assert core DP reset (both PLLs) */
    phy_copy.access.pll_idx = 1;
    if (_bh_core_dp_reset(&phy_copy, 1) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: core_dp_reset assert (pll1) failed");
        return PHYMOD_E_FAIL;
    }
    phy_copy.access.pll_idx = 0;
    if (_bh_core_dp_reset(&phy_copy, 1) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: core_dp_reset assert (pll0) failed");
        return PHYMOD_E_FAIL;
    }
    {
        /* Read back core DP reset state right after assert (pll0). If this is
         * 0, the write lands and any later 1 means uc_reset disturbed it. */
        uint8_t dp = 0xff;
        _bh_rde_field_byte(&phy_copy, APERTA_BH_CORE_DP_S_RSTBr, 2, 15, &dp);
        LOG_INFO("lane_map_set: after core_dp assert pll0 dp_rstb=%d", dp);
    }

    /* Micros held in reset */
    if (_bh_uc_reset_with_info(&phy_copy, 1, stack_size, ucode_size)
        != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: uc_reset assert failed");
        return PHYMOD_E_FAIL;
    }

    /* Program lane maps */
    if (_bh_pcs_tx_lane_swap(&phy_copy, pcs_tx_swap) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: pcs_tx_lane_swap failed");
        return PHYMOD_E_FAIL;
    }
    if (_bh_pcs_rx_lane_swap(&phy_copy, pcs_rx_swap) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: pcs_rx_lane_swap failed");
        return PHYMOD_E_FAIL;
    }
    {
        uint8_t dp = 0xff, mr = 0xff, mp = 0xff;
        uint32_t raw = 0, tsc_a = 0, pmd_a = 0;
        int rv, dev = 0;
        _bh_rde_field_byte(&phy_copy, APERTA_BH_CORE_DP_S_RSTBr, 2, 15, &dp);
        _bh_rde_field_byte(&phy_copy, APERTA_BH_MICRO_CORE_RSTBr, 15, 15, &mr);
        _bh_rde_field_byte(&phy_copy, APERTA_BH_REVID_MULTIPLICITYr, 0, 12, &mp);
        rv = plp_aperta_reg32_read(&phy_copy, PHYMOD_REG_APERTA_TSCBH | APERTA_BH_CORE_DP_S_RSTBr, &raw);
        _pm_get_tsc_addr(&tsc_a, &pmd_a, 0, 0x1, phy_copy.access.pll_idx, 0,
                         PHYMOD_REG_APERTA_TSCBH | APERTA_BH_CORE_DP_S_RSTBr, &dev);
        LOG_INFO("lane_map_set: pre-map_lanes dp=%d mr=%d mp=%d raw_d184=0x%x rv=%d pll=%d dev=%d pmd=0x%08x",
                 dp, mr, mp, (unsigned)(raw & 0xFFFF), rv, phy_copy.access.pll_idx, dev, (unsigned)pmd_a);
    }
    if (_bh_map_lanes(&phy_copy, APERTA_TSCBH_NOF_LANES_IN_CORE, pmd_tx_addr, pmd_rx_addr) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: map_lanes failed");
        return PHYMOD_E_FAIL;
    }
    if (_bh_uc_reset_with_info(&phy_copy, 0, stack_size, ucode_size) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: uc_reset release failed");
        return PHYMOD_E_FAIL;
    }
    {
        uint8_t mrst = 0xff, mclk = 0xff;
        _bh_rde_field_byte(&phy_copy, APERTA_BH_MICRO_MASTER_RSTBr, 15, 15, &mrst);
        _bh_rde_field_byte(&phy_copy, APERTA_BH_MICRO_MASTER_CLK_ENr, 15, 15, &mclk);
        LOG_INFO("lane_map_set: after uc_reset release master_rstb(0xd201)=%d master_clk_en(0xd200)=%d pll=%d",
                 mrst, mclk, phy_copy.access.pll_idx);
    }

    /* Wait for uC to become active */
    if (_bh_wait_uc_active(&phy_copy) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: wait_uc_active failed");
        return PHYMOD_E_FAIL;
    }

    /* Release core DP reset (both PLLs) */
    phy_copy.access.lane_mask = 0x1;
    phy_copy.access.pll_idx = 1;
    if (_bh_core_dp_reset(&phy_copy, 0) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: core_dp_reset release (pll1) failed");
        return PHYMOD_E_FAIL;
    }
    phy_copy.access.pll_idx = 0;
    if (_bh_core_dp_reset(&phy_copy, 0) != PHYMOD_E_NONE) {
        LOG_INFO("lane_map_set: core_dp_reset release (pll0) failed");
        return PHYMOD_E_FAIL;
    }

    /* Diagnostic: poll the TVCO PLL1 lock (0xd148 bit8) now that the uC is
     * active and DP reset is released. This decides whether PASS2's PLL config
     * locks once the micro firmware runs (vs. needing further PLL
     * programming). Up to 2s @ 10ms. */
    {
        uint8_t lock = 0;
        uint32_t poll, raw = 0;
        phy_copy.access.pll_idx = 1;
        phy_copy.access.lane_mask = 0x1;
        for (poll = 0; poll < 200; poll++) {
            _bh_rde_field_byte(&phy_copy, 0xd148, 7, 15, &lock);
            if (lock) {
                break;
            }
            PHYMOD_USLEEP(10000);
        }
        plp_aperta_reg32_read(&phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xd148, &raw);
        LOG_INFO("lane_map_set: PLL1 lock=%d after %u x10ms raw_d148=0x%x (uc_active side=%d)\n",
                 lock, poll, (unsigned)(raw & 0xFFFF), phy_copy.port_loc);
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_core_lane_map_get - read back TSCBH TX lane swap registers
 * ========================================================================== */
int plp_aperta_tscbh_core_lane_map_get(const plp_aperta_phymod_phy_access_t *core, plp_aperta_phymod_lane_map_t *lane_map)
{
    plp_aperta_phymod_phy_access_t phy;
    uint32_t reg_val;
    uint8_t tx_lane_map_physical[8];
    uint32_t lane, lane_outer, lane_inner, pcs_lane_swap = 0;

    PHYMOD_MEMCPY(&phy, core, sizeof(phy));

    /* MPP0: physical lanes 0-3 */
    phy.access.lane_mask = 0x1 << 0;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, TX_X1_TX_LN_SWPr, &reg_val));
    tx_lane_map_physical[0] = (reg_val) & 0x7;
    tx_lane_map_physical[1] = (reg_val >> 3) & 0x7;
    tx_lane_map_physical[2] = (reg_val >> 6) & 0x7;
    tx_lane_map_physical[3] = (reg_val >> 9) & 0x7;

    /* MPP1: physical lanes 4-7 */
    phy.access.lane_mask = 0x1 << 4;
    PHYMOD_IF_ERR_RETURN(plp_aperta_reg32_read(&phy, TX_X1_TX_LN_SWPr, &reg_val));
    tx_lane_map_physical[4] = (reg_val) & 0x7;
    tx_lane_map_physical[5] = (reg_val >> 3) & 0x7;
    tx_lane_map_physical[6] = (reg_val >> 6) & 0x7;
    tx_lane_map_physical[7] = (reg_val >> 9) & 0x7;

    /* Adjust the MPP1 logical lane offset (0-3 <-> 4-7) */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE / 2; lane++) {
        if (tx_lane_map_physical[4 + lane] > 4) {
            tx_lane_map_physical[4 + lane] -= 4;
        } else {
            tx_lane_map_physical[4 + lane] += 4;
        }
        tx_lane_map_physical[4 + lane] %= 8;
    }

    /* Translate logical lane based on physical lane based lane map */
    for (lane_outer = 0; lane_outer < APERTA_TSCBH_NOF_LANES_IN_CORE; lane_outer++) {
        for (lane_inner = 0; lane_inner < APERTA_TSCBH_NOF_LANES_IN_CORE; lane_inner++) {
            if (lane_outer == tx_lane_map_physical[lane_inner]) {
                pcs_lane_swap |= lane_inner << (lane_outer * 4);
                break;
            }
        }
    }

    lane_map->num_of_lanes = APERTA_TSCBH_NOF_LANES_IN_CORE;
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++) {
        lane_map->lane_map_tx[lane] = (pcs_lane_swap >> (lane * 4)) & 0xF;
        lane_map->lane_map_rx[lane] = (pcs_lane_swap >> (lane * 4)) & 0xF;
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_phy_polarity_set - set TX/RX polarity per lane
 * The PMD polarity bits (TSC 0xd173 TX / 0xd163 RX, bit0) are only reliably
 * applied while the lane datapath soft reset (0xd0b1 bit0) is held, and the
 * PCS datapath (SC_X4_CTL SW_SPEED_CHANGE) must be disabled / re-enabled
 * around the change. The previous stub skipped all of that, so the programmed
 * polarity could be ignored by the PMD.
 * ========================================================================== */
int plp_aperta_tscbh_phy_polarity_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_polarity_t *polarity)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t phy_copy, pm_phy_copy;
    uint32_t lane_reset, pcs_lane_enable, link_live, rd32;
    uint16_t rd;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    PHYMOD_MEMCPY(&pm_phy_copy, phy, sizeof(pm_phy_copy));
    if (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }

    /* (a) lane soft-reset status: LN_DP_S_RSTB (0xd0b1 bit0) is active-low.
     * lane_reset=1 => lane is currently held in datapath reset (bit=0). */
    lane_reset = 0;
    if (__bh_rdt_reg(&pm_phy_copy, 0xd0b1, &rd) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    lane_reset = (rd & 0x1) ? 0 : 1;

    /* (b) PCS lane enable: SC_X4_CTL (PCS domain 0x1800c050) bit8
     * SW_SPEED_CHANGE. NOTE: PCS-domain register, MUST be addressed as
     * PHYMOD_REG_APERTA_TSCBH|0xc050, NOT via _bh_tsc_addr() (which maps to
     * the 0x1800d000 TSC block and would produce 0x1800dc50). */
    pcs_lane_enable = 0;
    if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, &rd32) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }
    pcs_lane_enable = (rd32 >> 8) & 0x1;
    LOG_INFO("POL-SET start=%d num=%d lane_reset=%u pcs_en=%u SC_X4_CTL=0x%04x\n",
             start_lane, num_lane, (unsigned)lane_reset,
             (unsigned)pcs_lane_enable, (unsigned)(rd32 & 0xFFFF));

    /* (c) if the PCS lane was enabled: disable the PCS datapath
     * (SW_SPEED_CHANGE=0, SDK tbhmod_disable_set) and apply the SDK's
     * PCS-reset SW WAR (tbhmod_pcs_reset_sw_war). */
    if (pcs_lane_enable) {
        for (i = 0; i < num_lane; i++) {
            if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                continue;
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, &rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd32 &= ~0x100u;              /* SW_SPEED_CHANGE=0 */
            if (plp_aperta_reg32_write(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
        /* PCS reset SW WAR: only needed if the PCS link is live
         * (RX_X4_PCS_LATCH_STS1 0x1800c160 bit10). */
        pm_phy_copy.access.lane_mask = 1u << start_lane;
        if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc160, &rd32) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        link_live = (rd32 >> 10) & 0x1;
        LOG_INFO("POL-SET PCS_LINK_LIVE=%u (0xc160=0x%04x)\n", (unsigned)link_live, (unsigned)(rd32 & 0xFFFF));
        if (link_live) {
            /* disable all lane TX: TXFIR_MISC_CTL0 (0xd131, TSC domain)
             * bit0 SDK_TX_DISABLE=1 */
            for (i = 0; i < num_lane; i++) {
                if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                    continue;
                pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
                if (__bh_rdt_reg(&pm_phy_copy, 0xd131, &rd) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
                rd = (rd & ~0x1) | 0x1;
                if (__bh_wr_reg(&pm_phy_copy, 0xd131, rd) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
            }
            /* toggle SW_SPEED_CHANGE 1 -> wait -> 0 */
            pm_phy_copy.access.lane_mask = 1u << start_lane;
            if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, &rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd32 = (rd32 & ~0x100u) | 0x100u;
            if (plp_aperta_reg32_write(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            PHYMOD_USLEEP(5);
            if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, &rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd32 &= ~0x100u;
            if (plp_aperta_reg32_write(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            PHYMOD_USLEEP(5);
            /* restore all lane TX */
            for (i = 0; i < num_lane; i++) {
                if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                    continue;
                pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
                if (__bh_rdt_reg(&pm_phy_copy, 0xd131, &rd) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
                rd = rd & ~0x1;
                if (__bh_wr_reg(&pm_phy_copy, 0xd131, rd) != PHYMOD_E_NONE) {
                    return PHYMOD_E_FAIL;
                }
            }
        }
    }

    /* (d) if the lane was not in reset, assert the lane datapath soft reset
     * (0xd0b1 bit0=0) before touching the PMD polarity registers. */
    if (!lane_reset) {
        for (i = 0; i < num_lane; i++) {
            if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                continue;
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (__bh_rdt_reg(&pm_phy_copy, 0xd0b1, &rd) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd = rd & ~0x1;               /* assert lane soft reset */
            if (__bh_wr_reg(&pm_phy_copy, 0xd0b1, rd) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
    }

    /* (e) program TX/RX polarity per lane (TSC 0xd173 / 0xd163 bit0) */
    for (i = 0; i < num_lane; i++) {
        if (!(phy->access.lane_mask & (1 << (start_lane + i))))
            continue;
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);

        uint32_t tx_val = (polarity->tx_polarity >> i) & 0x1;
        uint32_t rx_val = (polarity->rx_polarity >> i) & 0x1;

        /* TX polarity: TSC reg 0xd173 bit0 */
        if (__bh_rdt_reg(&phy_copy, 0xd173, &rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        rd = (rd & ~0x1) | (tx_val & 0x1);
        if (__bh_wr_reg(&phy_copy, 0xd173, rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        LOG_INFO("POL-SET lane=%d tx_pol=%u 0xd173=0x%04x\n", start_lane + i, (unsigned)tx_val, (unsigned)rd);

        /* RX polarity: TSC reg 0xd163 bit0 */
        if (__bh_rdt_reg(&phy_copy, 0xd163, &rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        rd = (rd & ~0x1) | (rx_val & 0x1);
        if (__bh_wr_reg(&phy_copy, 0xd163, rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        LOG_INFO("POL-SET lane=%d rx_pol=%u 0xd163=0x%04x\n", start_lane + i, (unsigned)rx_val, (unsigned)rd);
    }

    /* (f) release the lane soft reset if we asserted it */
    if (!lane_reset) {
        for (i = 0; i < num_lane; i++) {
            if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                continue;
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (__bh_rdt_reg(&pm_phy_copy, 0xd0b1, &rd) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd = (rd & ~0x1) | 0x1;       /* release lane soft reset */
            if (__bh_wr_reg(&pm_phy_copy, 0xd0b1, rd) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
    }

    /* (g) re-enable the PCS lane if it was enabled (SW_SPEED_CHANGE=1,
     * SDK tbhmod_enable_set) */
    if (pcs_lane_enable) {
        for (i = 0; i < num_lane; i++) {
            if (!(phy->access.lane_mask & (1 << (start_lane + i))))
                continue;
            pm_phy_copy.access.lane_mask = 1 << (start_lane + i);
            if (plp_aperta_reg32_read(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, &rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
            rd32 = (rd32 & ~0x100u) | 0x100u;   /* SW_SPEED_CHANGE=1 */
            if (plp_aperta_reg32_write(&pm_phy_copy, PHYMOD_REG_APERTA_TSCBH | 0xc050, rd32) != PHYMOD_E_NONE) {
                return PHYMOD_E_FAIL;
            }
        }
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_phy_polarity_get - get TX/RX polarity per lane
 * ========================================================================== */
int plp_aperta_tscbh_phy_polarity_get(const plp_aperta_phymod_phy_access_t *phy, plp_aperta_phymod_polarity_t *polarity)
{
    int start_lane, num_lane, i;
    plp_aperta_phymod_phy_access_t phy_copy;

    PHYMOD_MEMCPY(&phy_copy, phy, sizeof(phy_copy));
    if (plp_aperta_phymod_util_lane_config_get(&phy->access, &start_lane, &num_lane) != PHYMOD_E_NONE) {
        return PHYMOD_E_FAIL;
    }

    polarity->tx_polarity = 0;
    polarity->rx_polarity = 0;

    for (i = 0; i < num_lane; i++) {
        if (!(phy->access.lane_mask & (1 << (start_lane + i))))
            continue;
        phy_copy.access.lane_mask = 0x1 << (i + start_lane);

        uint32_t rd;
        if (__bh_rdt_reg(&phy_copy, 0xd173, (uint16_t*)&rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        LOG_INFO("POL-GET lane=%d 0xd173=0x%04x (tx bit=%d)", start_lane + i, (unsigned)(rd & 0xFFFF), (int)(rd & 0x1));
        polarity->tx_polarity |= ((rd & 0x1) << i);

        if (__bh_rdt_reg(&phy_copy, 0xd163, (uint16_t*)&rd) != PHYMOD_E_NONE) {
            return PHYMOD_E_FAIL;
        }
        LOG_INFO("POL-GET lane=%d 0xd163=0x%04x (rx bit=%d)", start_lane + i, (unsigned)(rd & 0xFFFF), (int)(rd & 0x1));
        polarity->rx_polarity |= ((rd & 0x1) << i);
    }

    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_tscbh_phy_cl72_set - enable/disable CL72 (stub)
 * ========================================================================== */
int plp_aperta_tscbh_phy_cl72_set(const plp_aperta_phymod_phy_access_t *phy,
                                  uint32_t cl72_en)
{
    (void)phy;
    (void)cl72_en;
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * Interface type <-> numeric conversion
 * SWGPREG stores the index into APERTA_IF_TYPE_LIST_ELEMENTS (0..17).
 * ========================================================================== */
static const int aperta_if_type_list[] = {
    PHYMOD_APERTA_INTF_SR,        /* 0 */
    PHYMOD_APERTA_INTF_KR,        /* 1 */
    PHYMOD_APERTA_INTF_CR,        /* 2 */
    PHYMOD_APERTA_INTF_XFI,       /* 3 */
    PHYMOD_APERTA_INTF_SFI,       /* 4 */
    PHYMOD_APERTA_INTF_XLAUI,     /* 5 */
    PHYMOD_APERTA_INTF_LR,        /* 6 */
    PHYMOD_APERTA_INTF_ER,        /* 7 */
    PHYMOD_APERTA_INTF_VSR,       /* 8 */
    PHYMOD_APERTA_INTF_CAUI4_C2C, /* 9 */
    PHYMOD_APERTA_INTF_AUI_C2C,   /* 10 */
    PHYMOD_APERTA_INTF_AUI_C2M,   /* 11 */
    PHYMOD_APERTA_INTF_KR4,       /* 12 */
    PHYMOD_APERTA_INTF_CR4,       /* 13 */
    PHYMOD_APERTA_INTF_CAUI4_C2M, /* 14 */
    PHYMOD_APERTA_INTF_XLPPI,     /* 15 */
    PHYMOD_APERTA_INTF_CEIMR,     /* 16 */
    PHYMOD_APERTA_INTF_CEILR      /* 17 */
};

static int _aperta_convert_interface_type_to_numeric_value(int if_type)
{
    unsigned int i;
    for (i = 0; i < sizeof(aperta_if_type_list) / sizeof(aperta_if_type_list[0]); i++) {
        if (aperta_if_type_list[i] == if_type) {
            return (int)i;
        }
    }
    return 0;
}

static int _aperta_convert_numeric_value_to_interface_type(uint32_t numeric_val)
{
    unsigned int count = sizeof(aperta_if_type_list) / sizeof(aperta_if_type_list[0]);
    if (numeric_val < count) {
        return aperta_if_type_list[numeric_val];
    }
    return PHYMOD_APERTA_INTF_BYPASS;
}

/* SW interface type save registers */
#define APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR       BCMI_APERTA_D_CTRL_SWGPREG00r
#define APERTA_IF_TYPE_PER_LANE_STORAGE_MASK        0xF
#define APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET 2
#define APERTA_IF_TYPE_SAVE_SWGPREG_BIT4            BCMI_APERTA_D_CTRL_SWGPREG1Er

#define APERTA_IS_LINE_SIDE(PHY) \
    ((PHY)->port_loc == phymodPortLocLine)

/* ==========================================================================
 * plp_aperta_sw_intf_set - save SW interface type into SWGPREG
 * ========================================================================== */
int plp_aperta_sw_intf_set(const plp_aperta_phymod_phy_access_t *phy, int if_type)
{
    uint32_t if_type_numeric_val = 0, if_type_numeric_val_bit_4 = 0;
    uint32_t if_type_rd = 0, if_type_wr = 0;
    uint8_t lane_index = 0, sys_side = 0;

    if (phy == NULL) {
        return PHYMOD_E_PARAM;
    }

    if_type_numeric_val = _aperta_convert_interface_type_to_numeric_value(if_type);
    if_type_numeric_val_bit_4 = (if_type_numeric_val & 0x10) >> 4;
    if_type_numeric_val &= 0xF;

    sys_side = APERTA_IS_LINE_SIDE(phy) ? 0 : 1;

    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                 (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) + (lane_index / 4)), &if_type_rd));
            if_type_wr  = if_type_rd & ~(APERTA_IF_TYPE_PER_LANE_STORAGE_MASK << ((lane_index & 0x3) << 0x2));
            if_type_wr |= ((if_type_numeric_val & APERTA_IF_TYPE_PER_LANE_STORAGE_MASK) << ((lane_index & 0x3) << 0x2));
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
                 (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) + (lane_index / 4)), if_type_wr));
            break;
        }
    }
    /* one more bit to support more interface types */
    for (lane_index = 0; lane_index < APERTA_MAX_LANES; lane_index++) {
        if (phy->access.lane_mask & (1 << lane_index)) {
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, &if_type_rd));
            if_type_wr  = if_type_rd & ~(1 << (lane_index + (sys_side * 8)));
            if_type_wr |= (if_type_numeric_val_bit_4 & 1) << (lane_index + (sys_side * 8));
            PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_write(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, if_type_wr));
            break;
        }
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_sw_intf_get(const plp_aperta_phymod_phy_access_t *phy, uint8_t lane_index, int *if_type)
{
    uint32_t if_type_rd = 0, if_type_numeric_val_bit_4 = 0;
    uint8_t sys_side = 0;
    uint32_t numeric_val;

    if (phy == NULL || if_type == NULL) {
        return PHYMOD_E_PARAM;
    }
    sys_side = APERTA_IS_LINE_SIDE(phy) ? 0 : 1;

    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, (APERTA_IF_TYPE_SAVE_SWGPREG_BASE_ADDR +
         (sys_side * APERTA_LINE_SYS_IF_TYPE_SAVE_SWGPREG_OFFSET) + (lane_index / 4)), &if_type_rd));
    PHYMOD_IF_ERR_RETURN(plp_aperta_direct_reg_read(phy, APERTA_IF_TYPE_SAVE_SWGPREG_BIT4, &if_type_numeric_val_bit_4));
    if_type_numeric_val_bit_4 = (if_type_numeric_val_bit_4 >> (lane_index + (sys_side * 8))) & 1;
    numeric_val = (if_type_rd >> ((lane_index & 0x3) << 2)) & APERTA_IF_TYPE_PER_LANE_STORAGE_MASK;
    numeric_val |= (if_type_numeric_val_bit_4 << 4);
    *if_type = _aperta_convert_numeric_value_to_interface_type(numeric_val);
    return PHYMOD_E_NONE;
}

/* ==========================================================================
 * plp_aperta_phy_interface_config_set - configure PHY interface
 * ========================================================================== */
int plp_aperta_phy_interface_config_set(const plp_aperta_phymod_phy_access_t *phy, const plp_aperta_phymod_phy_inf_config_t *config)
{
    plp_aperta_phymod_phy_inf_config_t config_temp;
    aperta_device_aux_modes_t auxmode;
    int pif;

    if (phy == NULL || config == NULL) {
        return PHYMOD_E_PARAM;
    }
    if (!config->device_aux_modes) {
        LOG_INFO("Aux mode cannot be NULL\n");
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMCPY(&auxmode, config->device_aux_modes, sizeof(auxmode));
    PHYMOD_MEMCPY(&config_temp, config, sizeof(config_temp));

    pif = config_temp.interface_type;
    /* Keep config->interface_modes as-is ;
     * it only ORs in the FIBER/COPPER/BACKPLANE bits below. */
    if ((pif == PHYMOD_APERTA_INTF_SFI) || (pif == PHYMOD_APERTA_INTF_SR) ||
        (pif == PHYMOD_APERTA_INTF_XFI) || (pif == PHYMOD_APERTA_INTF_XLAUI) ||
        (pif == PHYMOD_APERTA_INTF_LR) || (pif == PHYMOD_APERTA_INTF_ER) ||
        (pif == PHYMOD_APERTA_INTF_VSR) || (pif == PHYMOD_APERTA_INTF_AUI_C2C) ||
        (pif == PHYMOD_APERTA_INTF_AUI_C2M) || (pif == PHYMOD_APERTA_INTF_CAUI4_C2C) ||
        (pif == PHYMOD_APERTA_INTF_CAUI4_C2M) || (pif == PHYMOD_APERTA_INTF_XLPPI) ||
        (pif == PHYMOD_APERTA_INTF_CEIMR) || (pif == PHYMOD_APERTA_INTF_CEILR)) {
        config_temp.interface_modes |= PHYMOD_INTF_MODES_FIBER;
    } else if (pif == PHYMOD_APERTA_INTF_CR || pif == PHYMOD_APERTA_INTF_CR4) {
        config_temp.interface_modes |= PHYMOD_INTF_MODES_COPPER;
    } else {
        config_temp.interface_modes |= PHYMOD_INTF_MODES_BACKPLANE;
    }

    /* PM configuration */
    PHYMOD_IF_ERR_RETURN(plp_aperta_pm_interface_config_set(phy, &config_temp));
    PHYMOD_IF_ERR_RETURN(plp_aperta_sw_intf_set(phy, config->interface_type));
    if (auxmode.failover_config.lane_map != 0) {
        plp_aperta_phymod_phy_access_t phy_cpy;
        PHYMOD_MEMCPY(&phy_cpy, phy, sizeof(phy_cpy));
        phy_cpy.access.lane_mask = auxmode.failover_config.lane_map;
        PHYMOD_IF_ERR_RETURN(plp_aperta_sw_intf_set(&phy_cpy, config->interface_type));
    }
    return PHYMOD_E_NONE;
}