/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg_adapter.h"
#include "../rg_dev_sysfs/include/rg_sysfs_common.h"
#include "./include/dfd_sfpparse.h"

#define DFD_TEMP_BUFF_SIZE                     (128)
#define DFD_SFP_OPTOE_ADDR                     (0x50)
#define DFD_SFP_PAGE_SIZE                      (256)
#define DFD_FPATH_MAX_LEN                      (64)
#define DFD_QSFP_DOM_SIZE                      (8)
#define SFP_DIAGNOSTIC_MONITORING_TYPE_ADDR    (92)

/* sfp cable length */
static char *g_sfp_cable_length_type[] = {
    "LengthSMFkm-UnitsOfKm",        /* byte14 */
    "LengthSMF(UnitsOf100m)",       /* byte15 */
    "Length50um(UnitsOf10m)",       /* byte16 */
    "Length62.5um(UnitsOfm)",       /* byte17 */
    "LengthCable(UnitsOfm)",        /* byte18 */
    "LengthOM3(UnitsOf10m)",        /* byte19 */
};

/* qsfp cable length */
static char *g_qsfp_cable_length_type[] = {
    "Length(km)",                   /* byte142 */
    "Length OM3(2m)",               /* byte143 */
    "Length OM2(m)",                /* byte144 */
    "Length OM1(m)",                /* byte145 */
    "Length Cable Assembly(m)",     /* byte146 */
};

/* qsfp sepecification compliance*/
static struct sff_e2_spec_comp_s g_qsfp_spec_comp[] = {
    {0, "10/40G Ethernet Compliance Code", qsfp_comp_10_ethcc_bitmap}, /* byte131 */
    {1, "SONET Compliance codes" , qsfp_comp_socc_bitmap},             /* byte132 */
    {2, "SAS/SATA compliance codes", qsfp_comp_sscc_bitmap},           /* byte133 */
    {3, "Gigabit Ethernet Compliant codes", qsfp_comp_gethcc_bitmap},  /* byte134 */
    {4, "Fibre Channel link length", qsfp_comp_fcll_bitmap},           /* byte135 */
    {5, "Transmitter Technology", qsfp_comp_tt_bitmap},                /* byte136 */
    {6, "Fibre Channel transmission media", qsfp_comp_fctm_bitmap},    /* byte137 */
    {7, "Fibre Channel Speed", qsfp_comp_fcs_bitmap},                  /* byte138 */
};

/* sfp sepecification compliance*/
static struct sff_e2_spec_comp_s g_sfp_spec_comp[] = {
    {0, "10GEthernetComplianceCode", sfp_comp_10_ethcc_bitmap},   /* byte3 */
    {0, "InfinibandComplianceCode" , sfp_comp_infcc_bitmap},      /* byte3 */
    {1, "ESCONComplianceCodes", sfp_comp_escc_bitmap},            /* byte4 */
    {1, "SONETComplianceCodes", sfp_comp_socc1_bitmap},           /* byte4 */
    {2, "SONETComplianceCodes", sfp_comp_socc2_bitmap},           /* byte5 */
    {3, "EthernetComplianceCodes", sfp_comp_ethcc_bitmap},        /* byte6 */
    {4, "FibreChannelLinkLength", sfp_comp_fcll_bitmap},          /* byte7 */
    {4, "FibreChannelTechnology", sfp_comp_fct1_bitmap},          /* byte7 */
    {5, "FibreChannelTechnology", sfp_comp_fct2_bitmap},          /* byte8 */
    {5, "SFP+CableTechnology", sfp_comp_sfpct_bitmap},            /* byte8 */
    {6, "FibreChannelTransmissionMedia", sfp_comp_fctm_bitmap},   /* byte9 */
    {7, "FibreChannelSpeed", sfp_comp_fcs_bitmap},                /* byte10 */
};

static void dump_date(unsigned char *data, struct rj_eeprom_parse_t *eep_parse)
{
    int i = 0;
    DFD_SFF_DEBUG(DBG_VERBOSE, "eeprom return data, addr 0x%x, offset %d, suboff %d, size %d, type %d:\n",
            eep_parse->addr, eep_parse->offset, eep_parse->sub_offset,
            eep_parse->size, eep_parse->type);
    for (i = 0; i < eep_parse->size; i++) {
        DFD_SFF_DEBUG(DBG_VERBOSE, "    data[%d]=0x%x=\n", i, data[i]);
    }
}

static int twos_comp(int num, int bits)
{
    if ((num & (1 << (bits - 1))) != 0) {
        num = num - (1 << bits);
    }
    return num;
}

static int read_optoe_eeprom_data(const unsigned int sfp_bus,
        const unsigned char sfp_addr,
        const unsigned int sfp_offset, char *buf, int read_bytes)
{
    char filename[DFD_FPATH_MAX_LEN];
    int ret;

    memset(filename, 0, sizeof(filename));
    snprintf(filename, sizeof(filename), "/sys/bus/i2c/devices/%d-%04x/eeprom",sfp_bus,sfp_addr);
    DFD_SFF_DEBUG(DBG_VERBOSE, "read optoe eeprom, fpath:%s, offset:0x%x, read bytes:%d.\n", filename, sfp_offset, read_bytes);
    ret = dfd_ko_read_file(filename, sfp_offset, buf, read_bytes);
    if ( ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "read optoe eeprom error.\n");
        return ret;
    }
    DFD_SFF_DEBUG(DBG_VERBOSE, " read_optoe_eeprom_data ok.\n");

    return ret;
}

static int read_one_eeprom_data(const unsigned int sfp_bus,
        const unsigned char sfp_addr,
        const unsigned int sfp_offset)
{
    char filename[DFD_FPATH_MAX_LEN];
    unsigned char data;
    int ret;

    memset(filename, 0, sizeof(filename));
    snprintf(filename, sizeof(filename), "/sys/bus/i2c/devices/%d-%04x/eeprom",sfp_bus,sfp_addr);
    DFD_SFF_DEBUG(DBG_VERBOSE, "read one eeprom date, fpath:%s, offset:0x%x.\n", filename, sfp_offset);
    ret = dfd_ko_read_file(filename, sfp_offset, &data, 1);
    if ( ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "read one eeprom error.\n");
        return ret;
    }
    DFD_SFF_DEBUG(DBG_VERBOSE, " read_one_eeprom_data get data 0x%x\n", data);

    return data;

}

static int get_calibration_type(const unsigned int sfp_bus,
        rj_dom_type_t dom_type)
{
    int calb_type;
    int tmp_data;

    if (dom_type == DOM_SFF8436) {
        calb_type = 1;
    } else {
        tmp_data = read_one_eeprom_data(sfp_bus, DFD_SFP_OPTOE_ADDR, SFP_DIAGNOSTIC_MONITORING_TYPE_ADDR);
        if ((tmp_data>>5)&1) {
            calb_type = 1;
        } else if ((tmp_data>>4)&1) {
            calb_type = 2;
        } else {
            calb_type = 0;
        }
    }
    return calb_type;
}

static int get_dom_ext_calibration_constants(rj_dom_ext_calibration_t dom_ext_type)
{
    int off;

    switch (dom_ext_type) {
        case EXTCALB_RX_PWR_4:
            off = 56;
            break;
        case EXTCALB_RX_PWR_3:
            off = 60;
            break;
        case EXTCALB_RX_PWR_2:
            off = 64;
            break;
        case EXTCALB_RX_PWR_1:
            off = 68;
            break;
        case EXTCALB_RX_PWR_0:
            off = 72;
            break;
        case EXTCALB_TX_I_Slope:
            off = 76;
            break;
        case EXTCALB_TX_I_Offset:
            off = 78;
            break;
        case EXTCALB_TX_PWR_Slope:
            off = 80;
            break;
        case EXTCALB_TX_PWR_Offset:
            off = 82;
            break;
        case EXTCALB_T_Slope:
            off = 84;
            break;
        case EXTCALB_T_Offset:
            off = 86;
            break;
        case EXTCALB_V_Slope:
            off = 88;
            break;
        case EXTCALB_V_Offset:
            off = 90;
            break;
        default:
            off = 0;
            DFD_SFF_DEBUG(DBG_ERROR, "Not support type dom_ext_type\n");
            break;
    }

    return off;
}

static void get_div_mod_4(int idata, int divdata,
        int *out_div, int *out_mod)
{
    *out_div = idata/divdata;
    *out_mod = (idata*10000/divdata)%10000;
}

static void power_in_dbm_str(int result, char *out_buf, int buf_len)
{
    int div_result, mod_result;

    if (result == 0) {
        snprintf(out_buf, buf_len, "%s\n", "-inf");
    } else if (result < 0) {
        snprintf(out_buf, buf_len, "%s\n", "NaN");
    } else {
        get_div_mod_4(result, 10000, &div_result, &mod_result);
        snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    }
}

static int calc_rx_power_value(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        int msb, int lsb, int *result)
{
    int off, value;
    unsigned int cal_type;
    int rx_pwr_byte0, rx_pwr_byte1, rx_pwr_byte2, rx_pwr_byte3;
    int rx_pwr_4, rx_pwr_3, rx_pwr_2, rx_pwr_1, rx_pwr_0, rx_pwr;
    int bus_addr;

    bus_addr = DFD_SFP_OPTOE_ADDR;
    cal_type = get_calibration_type(sfp_bus, eep_parse->dom_type);
    value = (msb << 8) | (lsb & 0xff);

    if (cal_type == 1) {
        /* Internal Calibration */
        *result = value;
        return DFD_RV_OK;
    }
    if (cal_type == 2) {
       /* XXX: External Calibration
        # RX_PWR(uW) = RX_PWR_4 * RX_PWR_AD +
        #          RX_PWR_3 * RX_PWR_AD +
        #          RX_PWR_2 * RX_PWR_AD +
        #          RX_PWR_1 * RX_PWR_AD +
        #          RX_PWR(0)*/
        off = get_dom_ext_calibration_constants(EXTCALB_RX_PWR_4);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        rx_pwr_byte3 = read_one_eeprom_data(sfp_bus, bus_addr, off);
        rx_pwr_byte2 = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        rx_pwr_byte1 = read_one_eeprom_data(sfp_bus, bus_addr, off+2);
        rx_pwr_byte0 = read_one_eeprom_data(sfp_bus, bus_addr, off+3);
        rx_pwr_4 = (rx_pwr_byte3 << 24) | (rx_pwr_byte2 << 16) | (rx_pwr_byte1 << 8) | (rx_pwr_byte0 & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_RX_PWR_3);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        rx_pwr_byte3 = read_one_eeprom_data(sfp_bus, bus_addr, off);
        rx_pwr_byte2 = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        rx_pwr_byte1 = read_one_eeprom_data(sfp_bus, bus_addr, off+2);
        rx_pwr_byte0 = read_one_eeprom_data(sfp_bus, bus_addr, off+3);
        rx_pwr_3 = (rx_pwr_byte3 << 24) | (rx_pwr_byte2 << 16) | (rx_pwr_byte1 << 8) | (rx_pwr_byte0 & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_RX_PWR_2);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        rx_pwr_byte3 = read_one_eeprom_data(sfp_bus, bus_addr, off);
        rx_pwr_byte2 = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        rx_pwr_byte1 = read_one_eeprom_data(sfp_bus, bus_addr, off+2);
        rx_pwr_byte0 = read_one_eeprom_data(sfp_bus, bus_addr, off+3);
        rx_pwr_2 = (rx_pwr_byte3 << 24) | (rx_pwr_byte2 << 16) | (rx_pwr_byte1 << 8) | (rx_pwr_byte0 & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_RX_PWR_1);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        rx_pwr_byte3 = read_one_eeprom_data(sfp_bus, bus_addr, off);
        rx_pwr_byte2 = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        rx_pwr_byte1 = read_one_eeprom_data(sfp_bus, bus_addr, off+2);
        rx_pwr_byte0 = read_one_eeprom_data(sfp_bus, bus_addr, off+3);
        rx_pwr_1 = (rx_pwr_byte3 << 24) | (rx_pwr_byte2 << 16) | (rx_pwr_byte1 << 8) | (rx_pwr_byte0 & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_RX_PWR_0);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        rx_pwr_byte3 = read_one_eeprom_data(sfp_bus, bus_addr, off);
        rx_pwr_byte2 = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        rx_pwr_byte1 = read_one_eeprom_data(sfp_bus, bus_addr, off+2);
        rx_pwr_byte0 = read_one_eeprom_data(sfp_bus, bus_addr, off+3);
        rx_pwr_0 = (rx_pwr_byte3 << 24) | (rx_pwr_byte2 << 16) | (rx_pwr_byte1 << 8) | (rx_pwr_byte0 & 0xff);

        rx_pwr = (rx_pwr_4 * value) + (rx_pwr_3 * value) + (rx_pwr_2 * value) + (rx_pwr_1 * value) + rx_pwr_0;

        *result = rx_pwr;
        return DFD_RV_OK;
    }
    return -DFD_RV_TYPE_ERR;
}

static void sfp_calc_rx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int ret, result;

    ret = calc_rx_power_value(sfp_bus, eep_parse, data[0], data[1], &result);
    if(ret < 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
        return ;
    }
    power_in_dbm_str(result, out_buf, buf_len);
    return ;
}

static void qsfp_calc_rx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, ret, result, div_result, mod_result ;

    for (i = 0; i < DFD_QSFP_DOM_SIZE; i += 2) {
        ret = calc_rx_power_value(sfp_bus, eep_parse, data[i], data[i + 1], &result);
        if (ret < 0) {
            snprintf(out_buf, buf_len, "%srx%dpower: Unknown\n", out_buf, (i/2 + 1));
        } else {
            if (result == 0) {
                snprintf(out_buf, buf_len, "%srx%dpower: -inf\n", out_buf, (i/2 + 1));
            } else if (result < 0) {
                snprintf(out_buf, buf_len, "%srx%dpower: NaN\n", out_buf, (i/2 + 1));
            } else {
                get_div_mod_4(result, 10000, &div_result, &mod_result);
                snprintf(out_buf, buf_len, "%srx%dpower: %d.%04d\n", out_buf, (i/2 + 1), div_result, mod_result);
            }
        }
    }
    return ;

}

static int calc_tx_power_value(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        int msb, int lsb, int *result)
{
    int off, value;
    unsigned int cal_type;
    int msb_tx_pwr, lsb_tx_pwr, tx_pwr_slope, tx_pwr_offset;
    int bus_addr;

    bus_addr = DFD_SFP_OPTOE_ADDR;
    cal_type = get_calibration_type(sfp_bus, eep_parse->dom_type);
    value = (msb << 8) | (lsb & 0xff);
    DFD_SFF_DEBUG(DBG_VERBOSE, "calc_tx_power. sfp_bus:%d, value:0x%x, cal_type:%d.\n", sfp_bus, value, cal_type);

    if (cal_type == 1) {
        *result = value;
        return DFD_RV_OK;
    }
    if (cal_type == 2) {
        /* XXX: TX_PWR(uW) = TX_PWR_Slope * TX_PWR_AD + TX_PWR_Offset */
        off = get_dom_ext_calibration_constants(EXTCALB_TX_PWR_Slope);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_tx_pwr = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_tx_pwr = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        tx_pwr_slope = (msb_tx_pwr << 8) | (lsb_tx_pwr & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_TX_PWR_Offset);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_tx_pwr = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_tx_pwr = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        tx_pwr_offset = (msb_tx_pwr << 8) | (lsb_tx_pwr & 0xff);
        tx_pwr_offset = twos_comp(tx_pwr_offset, 16);

        *result = tx_pwr_slope * value + tx_pwr_offset;
        return DFD_RV_OK;
    }
    return -DFD_RV_TYPE_ERR;
}

static void sfp_calc_tx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int ret, result;

    ret = calc_tx_power_value(sfp_bus, eep_parse, data[0], data[1], &result);
    if(ret < 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
        return ;
    }
    power_in_dbm_str(result, out_buf, buf_len);
    return ;
}

static void qsfp_calc_tx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, ret, result, div_result, mod_result ;

    for (i = 0; i < DFD_QSFP_DOM_SIZE; i += 2) {
        ret = calc_tx_power_value(sfp_bus, eep_parse, data[i], data[i + 1], &result);
        if (ret < 0) {
            snprintf(out_buf, buf_len, "%stx%dpower: Unknown\n", out_buf, (i/2 + 1));
        } else {
            if (result == 0) {
                snprintf(out_buf, buf_len, "%stx%dpower: -inf\n", out_buf, (i/2 + 1));
            } else if (result < 0) {
                snprintf(out_buf, buf_len, "%stx%dpower: NaN\n", out_buf, (i/2 + 1));
            } else {
                get_div_mod_4(result, 10000, &div_result, &mod_result);
                snprintf(out_buf, buf_len, "%stx%dpower: %d.%04d\n", out_buf, (i/2 + 1), div_result, mod_result);
            }
        }
    }
    return ;
}

static int calc_bias_value(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        int msb, int lsb, int *div_result, int *mod_result)
{
    int off;
    unsigned int cal_type, result;
    int msb_i, lsb_i, i_slope, i_offset;
    int bus_addr;

    bus_addr = DFD_SFP_OPTOE_ADDR;
    cal_type = get_calibration_type(sfp_bus, eep_parse->dom_type);
    result = (msb << 8) | (lsb & 0xff);

    if (cal_type == 1) {
        /* Internal Calibration */
        get_div_mod_4(result, 500, div_result, mod_result);
        return DFD_RV_OK;
    }
    if (cal_type == 2) {
        /* XXX: External Calibration */
        /* I(uA) = I_Slope * I_AD + I_Offset */
        off = get_dom_ext_calibration_constants(EXTCALB_TX_I_Slope);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_i = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_i = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        i_slope = (msb_i << 8) | (lsb_i & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_TX_I_Offset);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_i = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_i = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        i_offset = (msb_i << 8) | (lsb_i & 0xff);
        i_offset = twos_comp(i_offset, 16);

        result = i_slope * result + i_offset;
        get_div_mod_4(result, 500, div_result, mod_result);
        return DFD_RV_OK;
    }
    return -DFD_RV_TYPE_ERR;
}

static void sfp_calc_tx_bias(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int ret, div_result, mod_result;

    ret = calc_bias_value(sfp_bus, eep_parse, data[0], data[1], &div_result, &mod_result);
    if(ret < 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
        return ;
    }
    snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    return ;
}

static void qsfp_calc_tx_bias(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, ret, div_result, mod_result;

    for (i = 0; i < DFD_QSFP_DOM_SIZE; i += 2) {
        ret = calc_bias_value(sfp_bus, eep_parse, data[i], data[i + 1], &div_result, &mod_result);
        if (ret < 0) {
            snprintf(out_buf, buf_len, "%stx%dbias: Unknown\n", out_buf, (i/2 + 1));
        } else {
            snprintf(out_buf, buf_len, "%stx%dbias: %d.%04d\n", out_buf, (i/2 + 1), div_result, mod_result);
        }
    }
    return ;
}

static void calc_temperature(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int off;
    unsigned int cal_type, msb, lsb, result;
    int msb_t, lsb_t, t_slope, t_offset;
    int bus_addr;
    int div_result, mod_result;

    bus_addr = DFD_SFP_OPTOE_ADDR;
    cal_type = get_calibration_type(sfp_bus, eep_parse->dom_type);
    msb = data[0];
    lsb = data[1];

    result = (msb << 8) | (lsb & 0xff);
    result = twos_comp(result, 16);
    if (cal_type == 1) {
        /* Internal calibration */
        get_div_mod_4(result, 256, &div_result, &mod_result);
        snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    } else if (cal_type == 2) {
        /* XXX:External calibration */
        /* T(C) = T_Slope * T_AD + T_Offset */
        off = get_dom_ext_calibration_constants(EXTCALB_T_Slope);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_t = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_t = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        t_slope = (msb_t << 8) | (lsb_t & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_T_Offset);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_t = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_t = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        t_offset = (msb_t << 8) | (lsb_t & 0xff);
        t_offset = twos_comp(t_offset, 16);

        result = t_slope * result + t_offset;
        get_div_mod_4(result, 256, &div_result, &mod_result);
        snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    } else {
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
    }
}

static void calc_voltage(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int off;
    unsigned int cal_type, msb, lsb, result;
    int msb_v, lsb_v, v_slope, v_offset;
    int bus_addr;
    int div_result, mod_result;

    bus_addr = DFD_SFP_OPTOE_ADDR;
    cal_type = get_calibration_type(sfp_bus, eep_parse->dom_type);
    msb = data[0];
    lsb = data[1];
    result = (msb << 8) | (lsb & 0xff);

    if (cal_type == 1) {
        /* Internal Calibration */
        get_div_mod_4(result, 10000, &div_result, &mod_result);
        snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    } else if (cal_type == 2) {
        /* XXX:External Calibration */
        /*V(uV) = V_Slope * VAD + V_Offset  */
        off = get_dom_ext_calibration_constants(EXTCALB_V_Slope);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_v = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_v = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        v_slope = (msb_v << 8) | (lsb_v & 0xff);

        off = get_dom_ext_calibration_constants(EXTCALB_V_Offset);
        if (eep_parse->dom_type == DOM_SFF8472) {
            off += DFD_SFP_PAGE_SIZE;
        }
        msb_v = read_one_eeprom_data(sfp_bus, bus_addr, off);
        lsb_v = read_one_eeprom_data(sfp_bus, bus_addr, off+1);
        v_offset = (msb_v << 8) | (lsb_v & 0xff);
        v_offset = twos_comp(v_offset, 16);

        result = v_slope * result + v_offset;
        get_div_mod_4(result, 10000, &div_result, &mod_result);
        snprintf(out_buf, buf_len, "%d.%04d\n", div_result, mod_result);
    } else {
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
    }
}

static void convert_date_to_string(unsigned char *inchar, unsigned char *outtxt)
{
    snprintf(outtxt, DFD_TEMP_BUFF_SIZE, "%s%c%c-%c%c-%c%c %c%c",
            "20", inchar[0], inchar[1], inchar[2], inchar[3],
            inchar[4], inchar[5], inchar[6], inchar[7]);
    DFD_SFF_DEBUG(DBG_VERBOSE, "==outtxt %s\n", outtxt);
}

static void convert_int_to_str(int idata, unsigned char *outtxt)
{
    snprintf(outtxt, DFD_TEMP_BUFF_SIZE, "%d", idata);
    DFD_SFF_DEBUG(DBG_VERBOSE, "==outtxt %s\n", outtxt);
}

static void convert_hex_to_str(unsigned char *inchar, unsigned int len, unsigned char *outtxt, int need_spc)
{
    unsigned char hbit, lbit;
    unsigned int i, cnt;
    cnt = 0;
    for (i=0; i < len; i++) {
        hbit = (*(inchar+i)&0xf0)>>4;
        lbit = *(inchar+i)&0x0f;
        if (hbit > 9) {
            outtxt[cnt] = 'a'+hbit-10;
        } else {
            outtxt[cnt]='0'+hbit;
        }
        cnt++;
        if (lbit > 9) {
            outtxt[cnt]='a'+lbit-10;
        } else{
            outtxt[cnt]='0'+lbit;
        }
        cnt++;
        if (need_spc && i != len-1) {
            outtxt[cnt]='-';
            cnt++;
        }
    }
    outtxt[cnt] = 0;

    DFD_SFF_DEBUG(DBG_VERBOSE, "hex_to_str outtxt %s\n", outtxt);
}

static bool test_bitmapvalue(unsigned char inchar,
        struct rj_e2_enum_type_s *enum_str, int setoff)
{
    if (strcmp(enum_str[setoff].key, "BITVALUE") == 0) {
        return true;
    }
    if (strcmp(enum_str[setoff].key, "NULL") == 0) {
        return false;
    }
    if (((inchar >> setoff)  & 1) == 1) {
        return true;
    } else {
        return false;
    }
}

static void convert_strsplit(char *ibuf, char split_c, char *out_buf_1, char *out_buf_2)
{
    int i;
    char *out_buf_p;

    out_buf_p = out_buf_1;
    for (i = 0; i < strlen(ibuf); i++) {
        if (ibuf[i] == split_c) {
            out_buf_p = out_buf_2;
            continue;
        }
        *out_buf_p = ibuf[i];
        out_buf_p++;
    }
}

static void convert_bitmapdate_to_string(unsigned char inchar,
        struct rj_e2_enum_type_s *enum_str, char *buf, int buf_len)
{
    char tmp_buf[DFD_TEMP_BUFF_SIZE];
    char split_buf_1[DFD_TEMP_BUFF_SIZE];
    char split_buf_2[DFD_TEMP_BUFF_SIZE];
    char *tmp_buf_p;
    int i;

    memset(tmp_buf, 0, sizeof(tmp_buf));
    memset(split_buf_1, 0, sizeof(split_buf_1));
    memset(split_buf_2, 0, sizeof(split_buf_2));
    tmp_buf_p = tmp_buf;
    for (i = 0; i < 8; i++) {
        if (test_bitmapvalue(inchar, enum_str, i) && ((tmp_buf_p-tmp_buf) < DFD_TEMP_BUFF_SIZE)) {
            if (tmp_buf_p != tmp_buf) {
                snprintf(tmp_buf_p, 2, "%c", ',');
                tmp_buf_p++;
            }
            if (strcmp(enum_str[i].key, "BITVALUE") == 0) {
                convert_strsplit(enum_str[i].value, ':', split_buf_1, split_buf_2);
                DFD_SFF_DEBUG(DBG_VERBOSE, "bit 0 value %s , bit 1 value %s \n", split_buf_1, split_buf_2);
                if ((inchar>>i & 1) == 0) {
                    snprintf(tmp_buf_p, strlen(split_buf_1)+1, "%s", split_buf_1);
                    tmp_buf_p += strlen(split_buf_1);
                } else {
                    snprintf(tmp_buf_p, strlen(split_buf_2)+1, "%s", split_buf_2);
                    tmp_buf_p += strlen(split_buf_2);
                }
            } else {
                snprintf(tmp_buf_p, strlen(enum_str[i].value)+1, "%s", enum_str[i].value);
                tmp_buf_p += strlen(enum_str[i].value);
            }
            DFD_SFF_DEBUG(DBG_VERBOSE, "====tmp_buf %s \n", tmp_buf);
            break;
        }
    }
    snprintf(buf, buf_len, "%s\n", tmp_buf);
}

static bool eep_parse_decode(struct rj_eeprom_parse_t *eep_parse, unsigned char *data, char *buf, int buf_len)
{
    bool ret;
    unsigned char tmp;
    struct rj_e2_enum_type_s *tmp_enum_str;
    char out_buf[DFD_TEMP_BUFF_SIZE];
    int i;

    ret = true;
    memset(out_buf, 0, sizeof(out_buf));
    dump_date(data, eep_parse);
    switch (eep_parse->type) {
        case E2_STR :
            snprintf(buf, buf_len, "%s\n", data);
            break;
        case E2_ENUM:
            convert_hex_to_str(data, eep_parse->size, out_buf, false);
            if (eep_parse->enum_str != NULL) {
                snprintf(buf, buf_len, "%s\n", "Unknown");
                tmp_enum_str = eep_parse->enum_str;
                i = 0;
                while (strcmp(tmp_enum_str->key, "NULL") != 0 && i < 100) {
                    DFD_SFF_DEBUG(DBG_VERBOSE, " key =%s, out_buf %s", tmp_enum_str->key, out_buf);
                    if (strcmp(tmp_enum_str->key, out_buf) == 0) {
                        DFD_SFF_DEBUG(DBG_VERBOSE, "====mach==\n");
                        snprintf(buf, buf_len, "%s\n", tmp_enum_str->value);
                        break;
                    }
                    i++;
                    tmp_enum_str++;
                }
            } else {
                snprintf(buf, buf_len, "0x%s\n", out_buf);
            }
            break;
        case E2_INT:
            convert_int_to_str(data[0], out_buf);
            snprintf(buf, buf_len, "%s\n", out_buf);
            break;
        case E2_HEX:
            convert_hex_to_str(data, eep_parse->size, out_buf, true);
            snprintf(buf, buf_len, "%s\n", out_buf);
            break;
        case E2_DATE:
            convert_date_to_string(data, out_buf);
            snprintf(buf, buf_len, "%s\n", out_buf);
            break;
        case E2_BITVALUE:
            tmp = data[0];
            if (((tmp >> eep_parse->sub_offset) & 1) == 1) {
                snprintf(buf, buf_len, "1\n");
            } else {
                snprintf(buf, buf_len, "0\n");
            }
            break;
        case E2_BITVALUE4:
            tmp = data[0];
            if (((tmp >> eep_parse->sub_offset) & 1) == 1) {
                snprintf(buf, buf_len, "1\n");
            } else {
                snprintf(buf, buf_len, "0\n");
            }
            break;
        case E2_BITMAP:
             tmp = data[0];
             if (eep_parse->enum_str != NULL) {
                 convert_bitmapdate_to_string(tmp, eep_parse->enum_str, buf, buf_len);
             } else {
                 snprintf(buf, buf_len, "0x%x\n", tmp);
             }
             break;
        default:
            ret = false;
            DFD_SFF_DEBUG(DBG_ERROR, "Not support type %d \n", eep_parse->type);
            break;
    }
    return ret;
}

static void sfp_cable_length_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, c_flag;

    if (eep_parse->size <0 || eep_parse->size > ARRAY_SIZE(g_sfp_cable_length_type)) {
        DFD_SFF_DEBUG(DBG_ERROR, "size error. sfp_bus: %d, eeprom parse size:%d.\n", sfp_bus, eep_parse->size);
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
        return ;
    }

    c_flag = 0;
    for(i = 0; i < eep_parse->size; i++) {
        if(data[i] != 0) {
            c_flag = 1;
            snprintf(out_buf, buf_len, "%s%s: %d\n", out_buf, g_sfp_cable_length_type[i], data[i]);
        }
    }

    if (c_flag == 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unspecified");
    }

    return ;
}

static void qsfp_cable_length_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, c_flag;

    if (eep_parse->size <0 || eep_parse->size > ARRAY_SIZE(g_qsfp_cable_length_type)) {
        DFD_SFF_DEBUG(DBG_ERROR, "size error. sfp_bus: %d, eeprom parse size:%d.\n", sfp_bus, eep_parse->size);
        snprintf(out_buf, buf_len, "%s\n", "Unknown");
        return ;
    }
    c_flag = 0;
    for(i = 0; i < eep_parse->size; i++) {
        if(data[i] != 0) {
            c_flag = 1;
            snprintf(out_buf, buf_len, "%s%s: %d\n", out_buf, g_qsfp_cable_length_type[i], data[i]);
        }
    }
    if (c_flag == 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unspecified");
    }
    return ;
}

static void sfp_spec_comp_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, s_flag, reg_offset;
    int spec_comp_len;
    char buf_tmp[DFD_TEMP_BUFF_SIZE];

    spec_comp_len = ARRAY_SIZE(g_sfp_spec_comp);
    s_flag = 0;
    for(i = 0; i < spec_comp_len; i++) {
        reg_offset = g_sfp_spec_comp[i].offset;
        if (reg_offset < 0 || reg_offset >= eep_parse->size ||g_sfp_spec_comp[i].attr_value == NULL ) {
            DFD_SFF_DEBUG(DBG_ERROR, "g_qsfp_spec_comp config error. sfp_bus: %d, g_qsfp_spec_comp index:%d, offset:%d,  eeprom parse size:%d.\n",
                sfp_bus, i, reg_offset, eep_parse->size);
            continue;
        }
        if(data[reg_offset] != 0) {
            memset(buf_tmp, 0, sizeof(buf_tmp));
            convert_bitmapdate_to_string(data[reg_offset], g_sfp_spec_comp[i].attr_value, buf_tmp, sizeof(buf_tmp));
            if (strcmp(buf_tmp, "\n") == 0) {
                DFD_SFF_DEBUG(DBG_WARN, "qsfp sepecification_compliance.value reserved. sfp_bus:%d, reg offset:%d, value:0x%x\n",
                    sfp_bus, eep_parse->addr + i, data[i]);
                continue;
            }
            s_flag = 1;
            snprintf(out_buf, buf_len, "%s%s: %s", out_buf, g_sfp_spec_comp[i].attr_name, buf_tmp);
        }
    }
    if (s_flag == 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unspecified");
    }
    return ;
}

static void qsfp_spec_comp_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len)
{
    int i, s_flag, reg_offset;
    int spec_comp_len;
    char buf_tmp[DFD_TEMP_BUFF_SIZE];

    spec_comp_len = ARRAY_SIZE(g_qsfp_spec_comp);
    s_flag = 0;
    for(i = 0; i < spec_comp_len; i++) {
        reg_offset = g_qsfp_spec_comp[i].offset;
        if (reg_offset < 0 || reg_offset >= eep_parse->size ||g_qsfp_spec_comp[i].attr_value == NULL ) {
            DFD_SFF_DEBUG(DBG_ERROR, "g_qsfp_spec_comp config error. sfp_bus: %d, g_qsfp_spec_comp index:%d, offset:%d,  eeprom parse size:%d.\n",
                sfp_bus, i, reg_offset, eep_parse->size);
            continue;
        }
        if(data[reg_offset] != 0) {
            memset(buf_tmp, 0, sizeof(buf_tmp));
            convert_bitmapdate_to_string(data[reg_offset], g_qsfp_spec_comp[i].attr_value, buf_tmp, sizeof(buf_tmp));
            if (strcmp(buf_tmp, "\n") == 0) {
                DFD_SFF_DEBUG(DBG_WARN, "qsfp sepecification_compliance.value reserved. sfp_bus:%d, reg offset:%d, value:0x%x\n",
                    sfp_bus, eep_parse->addr + i, data[i]);
                continue;
            }
            s_flag = 1;
            snprintf(out_buf, buf_len, "%s%s: %s", out_buf, g_qsfp_spec_comp[i].attr_name, buf_tmp);
        }
    }
    if (s_flag == 0) {
        snprintf(out_buf, buf_len, "%s\n", "Unspecified");
    }
    return ;
}

ssize_t sfp_show_atrr(int sfp_bus, int sfp_mode, const char *attr_name, char *buf, int buf_len)
{
    struct rj_eeprom_parse_t *eep_parse;
    unsigned char data[DFD_TEMP_BUFF_SIZE];
    int i, isqsfp, ret;
    char *buf_start;

    if(!attr_name || !buf || buf_len <=0 ) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error. attr_name or buf is NULL. sfp_bus:%d,buf_len:%d\n", sfp_bus, buf_len);
        return -DFD_RV_INVALID_VALUE;
    }
    eep_parse = NULL;
    if (sfp_mode == RG_SFF_SPEED_100GE) { /* is sff8436 */
        isqsfp = true;
        for (i = 0; i < sizeof(ssf8436_interface_id)/sizeof(struct rj_eeprom_parse_t); i++) {
            if (strcmp(ssf8436_interface_id[i].attr_name, attr_name) == 0) {
                DFD_SFF_DEBUG(DBG_VERBOSE, "match attribute.sfp_bus:%d, speed mode:%d, attr_name:%s.\n", sfp_bus, sfp_mode, attr_name);
                eep_parse = &ssf8436_interface_id[i];
                break;
            }
        }
    } else if (sfp_mode == RG_SFF_SPEED_25GE) {
        isqsfp = false;
        for (i = 0; i < sizeof(ssf8472_interface_id)/sizeof(struct rj_eeprom_parse_t); i++) {
            if (strcmp(ssf8472_interface_id[i].attr_name, attr_name) == 0) {
                DFD_SFF_DEBUG(DBG_VERBOSE, "match attribute.sfp_bus:%d, speed mode:%d, attr_name:%s.\n", sfp_bus, sfp_mode, attr_name);
                eep_parse = &ssf8472_interface_id[i];
                break;
            }
        }
    } else {
        DFD_SFF_DEBUG(DBG_ERROR, "sfp attrname:%s, sfp mode:%d, don't support.\n", attr_name, sfp_mode);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    if (eep_parse == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "not define attr(%s) error.\n", attr_name);
        return -DFD_RV_NODE_FAIL;
    }
    if (eep_parse->size > sizeof(data) ) {
        DFD_SFF_DEBUG(DBG_ERROR, "date length error.need size:%d, date size:%lud.\n", eep_parse->size, sizeof(data));
        return -DFD_RV_INVALID_VALUE;
    }
    memset(data, 0, sizeof(data));
    buf_start = buf;
    if (eep_parse->type != E2_BITVALUE4) {
        ret = read_optoe_eeprom_data(sfp_bus, eep_parse->addr, eep_parse->offset, data, eep_parse->size);
        dump_date(data, eep_parse);
        if(ret < 0) {
            DFD_SFF_DEBUG(DBG_ERROR, "read eeprom error.\n");
            return ret;
        }
        if (eep_parse->type == E2_FUNC) {
            if( !eep_parse->calc_func ) {
                DFD_SFF_DEBUG(DBG_ERROR, "eep_parse->calc_func is NULL.\n");
                return -DFD_RV_INVALID_VALUE;
            }
            eep_parse->calc_func(sfp_bus, eep_parse, data, buf, buf_len);
        } else {
            if (!eep_parse_decode(eep_parse, data, buf, buf_len)) {
                DFD_SFF_DEBUG(DBG_ERROR, "eep_parse_decode error.\n");
                return -DFD_RV_DEV_FAIL;
            }
        }
    } else {
        /* XXX */
        for (i = 0; i < eep_parse->ext.subaddr_num; i++) {
            ret = read_optoe_eeprom_data(sfp_bus, eep_parse->addr, eep_parse->ext.subaddr[i].offset, data, eep_parse->ext.subaddr[i].size);
            if (ret < 0) {
                DFD_SFF_DEBUG(DBG_ERROR, "read eeprom error.\n");
                return ret;
            }
            snprintf(buf, buf_len, "%s-%d:", eep_parse->ext.prefix, i);
            eep_parse->sub_offset = eep_parse->ext.subaddr[i].sub_offset;
            if (!eep_parse_decode(eep_parse, data, buf+(int)strlen(buf_start), buf_len - (int)strlen(buf_start))) {
                DFD_SFF_DEBUG(DBG_ERROR, "eep_parse_decode error.\n");
                return -DFD_RV_DEV_FAIL;
            }
        }
        buf = buf_start;
    }
    return (ssize_t)strlen(buf);
}
