/*
* $Copyright: (c) 2024 Broadcom.
* Broadcom Proprietary and Confidential. All rights reserved.$
*
*/

#include "smbus_access.h"
#include "pmbus_commands.h"
#include "pmbus_regulator.h"

static double u16_to_hpf(uint16_t x)
{
    return ((x & 0x8000) ? -1.0 : 1.0) * ldexp(1.0 + (x & 0x3FF)/1024.0, (int)((x >> 10) & 0x1F) - 15) ;
}

static double u11_to_hpf(uint16_t r)
{   int val, sca;

    val = (int)(r & 0x7FF);
    val <<= 21;
    val >>= 21;
    sca = (int)(r & 0xF800);
    sca <<= 16;
    sca >>= 27;
    return ldexp((double)(val), sca);
}

double Vout_in_V(uint8_t vout_mode, uint16_t vout)
{
    switch ((vout_mode >> 5) & 0x3) {  /* vout_mode[6:5] */
    case 0 :  /* Linear16 format, Exp in vout_mode, Man in vout */
        return ldexp((double)(vout), (int)(vout_mode & 0x1F) - 32);
    case 3 :  /* IEEE 754 half-precission format, both Exp and Man in vout */
        return u16_to_hpf(vout);
    default :
        LOGX(LOG_ERROR, "Unsupported VOUT_MODE %02X", vout_mode);
        return -1.1111;
    }
}

enum num_format {
    NFRMT_DEFAULT = 0,
    NFRMT_VOUT,
    NFRMT_L11,
    NFRMT_UL16,
    NFRMT_IEEE_HPFP,
    NFRMT_UINT16
};

void SMBus_read_regulator_status(char *chip_name, bcm_plp_access_t plp_info, uint8_t regulator_i2c_addr)
{
    uint8_t vout_mode;
    uint32_t model;
    ial_config_t ialConfig ;

    struct vreg_info {
        const char* name;
        uint8_t cmd, len;
        char  info[31];
    } regulator_info[] = {
        {"MFR_ID",               PMBUS_MFR_ID,        0, {0}},
        {"MFR_MODEL",            PMBUS_MFR_MODEL,     0, {0}},
    }
#if 0
    ,
    regulator_xinfo_LTM7812[] = {
        {"MFR_REVISION",         PMBUS_MFR_REVISION,  0, {0}},
        {"MFR_SERIAL",           PMBUS_MFR_SERIAL,    0, {0}},
        {"IC_DEVICE_ID",         PMBUS_IC_DEVICE_ID,  0, {0}},
        {"IC_DEVICE_REV",        PMBUS_IC_DEVICE_REV, 0, {0}},
    }
#endif
    ;

    struct {
        const char* name;
        uint8_t cmd, val;
    } regulator_cap[] = {
        {"PMBUS_REVISION",       PMBUS_PMBUS_REVISION,   0},
        {"CAPABILITY",           PMBUS_CAPABILITY,       0},
        {"PMBUS_VOUT_MODE",      PMBUS_PMBUS_VOUT_MODE,  0},
    };

    struct rsw {
        const char *name, *unit, *fmt;
        uint8_t cmd;
        enum num_format nfmt;
        uint16_t val[2];
    } regulator_status_words[] = {
        {"STATUS_WORD",        "",     "  %04X",   PMBUS_STATUS_WORD,        NFRMT_UINT16,  {0}},
        {"READ_VIN",           "(V)",   "%6.3f",   PMBUS_READ_VIN,           NFRMT_DEFAULT, {0}},
        {"READ_IIN",           "(A)",   "%6.3f",   PMBUS_READ_IIN,           NFRMT_DEFAULT, {0}},
        {"VOUT_COMMAND",       "(V)",   "%6.3f",   PMBUS_VOUT_COMMAND,       NFRMT_VOUT,    {0}},
        {"READ_VOUT",          "(V)",   "%6.3f",   PMBUS_READ_VOUT,          NFRMT_VOUT,    {0}},
        {"READ_IOUT",          "(A)",   "%6.3f",   PMBUS_READ_IOUT,          NFRMT_DEFAULT, {0}},
        {"READ_TEMPERATURE_1", "(C)",   "%6.2f",   PMBUS_READ_TEMPERATURE_1, NFRMT_DEFAULT, {0}},
        {"READ_FREQUENCY",     "(Hz)",  "%6.1f",   PMBUS_READ_FREQUENCY,     NFRMT_DEFAULT, {0}},
        {"READ_POUT",          "(W)",   "%6.3f",   PMBUS_READ_POUT,          NFRMT_DEFAULT, {0}},
    };


    struct rsb {
        const char *name;
        uint8_t cmd;
        uint8_t val[2];
    } regulator_status_bytes[] = {
        {"PMBUS_OPERATION",      PMBUS_OPERATION,          {0}},
        {"PMBUS_ON_OFF_CONFIG",  PMBUS_ON_OFF_CONFIG,      {0}},
        {"STATUS_BYTE",          PMBUS_STATUS_BYTE,        {0}},
        {"STATUS_VOUT",          PMBUS_STATUS_VOUT,        {0}},
        {"STATUS_IOUT",          PMBUS_STATUS_IOUT,        {0}},
        {"STATUS_INPUT",         PMBUS_STATUS_INPUT,       {0}},
        {"STATUS_TEMPERATURE",   PMBUS_STATUS_TEMPERATURE, {0}},
        {"STATUS_CML",           PMBUS_STATUS_CML,         {0}},
    };

    uint8_t k, page;
    LOGX(LOG_MILESTONE, "======>> SMBus_read_regulator_status @ Log level %s <<======", getLogLevelStr(getLogLevel()));

    memset(&ialConfig, 0, sizeof(ial_config_t));
    configSmbusParam(chip_name, plp_info, &ialConfig, regulator_i2c_addr, /*PEC_enable=*/1, /*smbus_addr_mode=*/0);

    LOGX(LOG_INFO," **** Information for voltage regulator @ %02X : ****", regulator_i2c_addr);
    for (k=0; k<sizeof(regulator_info)/sizeof(regulator_info[0]); ++k) {
        uint8_t cmd = regulator_info[k].cmd;
        uint8_t rbuf[68] = {0};
        ialConfig.connection.Bits.PecEnabled = 0;
        smbusWrRdBlock(chip_name, plp_info, &ialConfig, &cmd, 1, rbuf, 1);
        if (rbuf[0] == 0xFF) {
            LOGX(LOG_ERROR, "Connection to SMBus slave is bad.");
            exit(0x400);
        }
        regulator_info[k].len = rbuf[0];
        ialConfig.connection.Bits.PecEnabled = 1;
        smbusWrRdBlock(chip_name, plp_info, &ialConfig, &cmd, 1, rbuf, rbuf[0]+1);
        snprintf(regulator_info[k].info, rbuf[0] + 1, "%s", (const char*)(rbuf+1));
        LOGX(LOG_INFO,"%-20s [%02X]      :  %s ", regulator_info[k].name, regulator_info[k].cmd, regulator_info[k].info);
    }

    sscanf(regulator_info[1].info, "%*[^0-9]%d", &model);

    LOGX(LOG_INFO,"");
    LOGX(LOG_INFO," **** Capability for voltage regulator @ %02X : **** ", regulator_i2c_addr);
    for (k=0; k<sizeof(regulator_cap)/sizeof(regulator_cap[0]); ++k) {
        regulator_cap[k].val = smbusReadByte(chip_name, plp_info, &ialConfig, regulator_cap[k].cmd);
        LOGX(LOG_INFO,"%-20s [%02X]      :  %02X ", regulator_cap[k].name, regulator_cap[k].cmd, regulator_cap[k].val);
    }

    for (page=0; page<2; ++page) {
        smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_PAGE, page);
        for (k=0; k<sizeof(regulator_status_words)/sizeof(regulator_status_words[0]); ++k) {
            regulator_status_words[k].val[page] = smbusReadWord(chip_name, plp_info, &ialConfig, regulator_status_words[k].cmd);
        }
        for (k=0; k<sizeof(regulator_status_bytes)/sizeof(regulator_status_bytes[0]); ++k) {
            regulator_status_bytes[k].val[page] = smbusReadByte(chip_name, plp_info, &ialConfig, regulator_status_bytes[k].cmd);
        }
    }

    vout_mode = regulator_cap[2].val;

    LOGX(LOG_INFO,"");
    LOGX(LOG_INFO," **** Status for voltage regulator @ %02X : ****", regulator_i2c_addr);
    LOGX(LOG_INFO,"%-20s [%02X]      :  %04X          %04X ", \
            regulator_status_words[0].name, regulator_status_words[0].cmd, \
            regulator_status_words[0].val[0], regulator_status_words[0].val[1]);
    for (k=0; k<sizeof(regulator_status_bytes)/sizeof(regulator_status_bytes[0]); ++k) {
        const struct rsb *sb = regulator_status_bytes+k;
        LOGX(LOG_INFO,"%-20s [%02X]      :    %02X            %02X ", sb->name, sb->cmd, sb->val[0], sb->val[1]);
    }

    LOGX(LOG_INFO,"");
    LOGX(LOG_INFO," **** Telemetrics for voltage regulator @ %02X : ****", regulator_i2c_addr);
    for (k=1; k<sizeof(regulator_status_words)/sizeof(regulator_status_words[0]); ++k) {
        const struct rsw *sw = regulator_status_words+k;
        double fval[2] = {-1.0, -1.0};
        if (strstr(sw->name, "VOUT")) {
            fval[0] = Vout_in_V(vout_mode, sw->val[0]);
            fval[1] = Vout_in_V(vout_mode, sw->val[1]);
        } else {
            if (model == 4678) {
                fval[0] = u11_to_hpf(sw->val[0]);
                fval[1] = u11_to_hpf(sw->val[1]);
            } else if (model == 7182) {
                fval[0] = u16_to_hpf(sw->val[0]);
                fval[1] = u16_to_hpf(sw->val[1]);
            }
        }

        LOGX(LOG_RAW, "{INFO } %-20s [%02X] %4s :", sw->name, sw->cmd, sw->unit);
        LOGX(LOG_RAW ,  "  %04X(", sw->val[0]);
        LOGX(LOG_RAW , sw->fmt, fval[0]);
        LOGX(LOG_RAW , ")  %04X(", sw->val[1]);
        LOGX(LOG_RAW , sw->fmt, fval[1]);
        LOGX(LOG_RAW , ")\n");
    }

    LOGX(LOG_MILESTONE, "<<====== SMBus_read_regulator_status ======>>");
}

void set_rail_address(char *chip_name, bcm_plp_access_t plp_info, uint8_t rail_addr, const uint16_t* r, uint8_t L)
{
    ial_config_t ialConfig ;

    memset(&ialConfig, 0, sizeof(ial_config_t));
    while (L--) {
        uint8_t ch, addr;
        ch = (*r)  & 0xF;
        addr = (*r) >> 4;

        configSmbusParam(chip_name, plp_info, &ialConfig, addr, /*PEC_enable=*/1, /*smbus_addr_mode=*/0);
        smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_PAGE, ch);
        smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_MFR_RAIL_ADDRESS, rail_addr);
        ++r;
    }

    configSmbusParam(chip_name, plp_info, &ialConfig, rail_addr, /*PEC_enable=*/1, /*smbus_addr_mode=*/0);
    smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_PAGE, 0);
    smbusWriteWord(chip_name, plp_info, &ialConfig, PMBUS_VOUT_UV_WARN_LIMIT,  2270); /* 2457/4096 = 0.59985 */
    smbusWriteWord(chip_name, plp_info, &ialConfig, PMBUS_VOUT_UV_FAULT_LIMIT, 2130); /* 2130/4096 = 0.52002 */
}

uint16_t set_DVDD_voltage(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr, uint8_t ch, uint16_t mV)
{
    ial_config_t ialConfig ;

    memset(&ialConfig, 0, sizeof(ial_config_t));
    uint16_t vout_code;
    vout_code = (uint16_t) (mV*4.096 + 0.5); /* specific to Linear regulators */

    configSmbusParam(chip_name, plp_info, &ialConfig, addr, /*PEC_enable=*/1, /*smbus_addr_mode=*/0);
    smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_PAGE, ch);
    smbusWriteWord(chip_name, plp_info, &ialConfig, PMBUS_VOUT_COMMAND, vout_code);

    LOGX(LOG_INFO, "   PMB[%02X.%d] <== %04X (%03d mV)", addr, ch, vout_code, mV);
    return vout_code;
}

uint16_t get_DVDD_voltage(char *chip_name, bcm_plp_access_t plp_info, uint8_t addr, uint8_t ch)
{
    ial_config_t ialConfig ;
    uint16_t vout_code, mV;

    memset(&ialConfig, 0, sizeof(ial_config_t));

    configSmbusParam(chip_name, plp_info, &ialConfig, addr, /*PEC_enable=*/1, /*smbus_addr_mode=*/0);

    smbusWriteByte(chip_name, plp_info, &ialConfig, PMBUS_PAGE, ch);
    vout_code = smbusReadWord(chip_name, plp_info, &ialConfig, PMBUS_READ_VOUT);
    mV =  vout_code * 1000 / 4096; /* specific to Linear regulators */
    LOGX(LOG_INFO, "   PMB[%02X.%d] ==> %04X (%03d mV)", addr, ch, vout_code, mV);
    return mV;
}
