#ifndef _DFD_SFPPARSE_H_
#define _DFD_SFPPARSE_H_

struct rj_e2_enum_type_s {
    char key[32];
    char value[128];
};

struct sff_e2_spec_comp_s {
    int offset;
    char attr_name[128];
    struct rj_e2_enum_type_s *attr_value;
};

typedef enum {
    E2_STR,
    E2_INT,
    E2_HEX,
    E2_DATE,
    E2_BITVALUE,
    E2_BITMAP,
    E2_ENUM,
    E2_FUNC,
    E2_BITVALUE4,
} rj_eeprom_type_t;

struct rj_e2_enum_type_s sff8472_type_of_transceiver[] = {
    {"00","Unknown"},
    {"01","GBIC"},
    {"02", "Module soldered to motherboard"},
    {"03", "SFP or SFP Plus"},
    {"04", "300 pin XBI"},
    {"05", "XENPAK"},
    {"06", "XFP"},
    {"07", "XFF"},
    {"08", "XFP-E"},
    {"09", "XPAK"},
    {"0a", "X2"},
    {"0b", "DWDM-SFP"},
    {"0d", "QSFP"},
    {"NULL", ""}
};

struct rj_e2_enum_type_s sff8472_exttypeoftransceiver[] = {{"00", "GBIC def not specified"},
        {"01","GBIC is compliant with MOD_DEF 1"},
        {"02","GBIC is compliant with MOD_DEF 2"},
        {"03","GBIC is compliant with MOD_DEF 3"},
        {"04","GBIC/SFP defined by twowire interface ID"},
        {"05","GBIC is compliant with MOD_DEF 5"},
        {"06","GBIC is compliant with MOD_DEF 6"},
        {"07","GBIC is compliant with MOD_DEF 7"},
        {"NULL", ""}
};

struct rj_e2_enum_type_s sff8472_connector[] = {{"00", "Unknown"},
             {"01", "SC"},
             {"02", "Fibre Channel Style 1 copper connector"},
             {"03", "Fibre Channel Style 2 copper connector"},
             {"04", "BNC/TNC"},
             {"05", "Fibre Channel coaxial headers"},
             {"06", "FibreJack"},
             {"07", "LC"},
             {"08", "MT-RJ"},
             {"09", "MU"},
             {"0a", "SG"},
             {"0b", "Optical pigtail"},
             {"0C", "MPO Parallel Optic"},
             {"20", "HSSDCII"},
             {"21", "CopperPigtail"},
             {"22", "RJ45"},
             {"NULL", ""}
     };

struct rj_e2_enum_type_s sff8472_encoding_codes[] = {{"00","Unspecified"},
              {"01","8B/10B"},
              {"02","4B/5B"},
              {"03","NRZ"},
              {"04","Manchester"},
              {"05", "SONET Scrambled"},
              {"06","64B/66B"},
              {"NULL", ""}
      };

struct rj_e2_enum_type_s sff8472_rate_identifier[] = {{"00","Unspecified"},
               {"01","Defined for SFF-8079 (4/2/1G Rate_Select & AS0/AS1)"},
               {"02", "Defined for SFF-8431 (8/4/2G Rx Rate_Select only)"},
               {"03","Unspecified"},
               {"04", "Defined for SFF-8431 (8/4/2G Tx Rate_Select only)"},
               {"05","Unspecified"},
               {"06","Defined for SFF-8431 (8/4/2G Independent Rx & Tx Rate_select)"},
               {"07","Unspecified"},
               {"08", "Defined for FC-PI-5 (16/8/4G Rx Rate_select only) High=16G only, Low=8G/4G"},
               {"09", "Unspecified"},
               {"0a", "Defined for FC-PI-5 (16/8/4G Independent Rx, Tx Rate_select) High=16G only, Low=8G/4G"},
               {"NULL", ""}
       };

struct rj_e2_enum_type_s sff8436_type_of_transceiver[] = {
    {"00","Unknown or unspecified"},
    {"01","GBIC"},
    {"02", "Module/connector soldered to motherboard"},
    {"03", "SFP"},
    {"04", "300 pin XBI"},
    {"05", "XENPAK"},
    {"06", "XFP"},
    {"07", "XFF"},
    {"08", "XFP-E"},
    {"09", "XPAK"},
    {"0a", "X2"},
    {"0b", "DWDM-SFP"},
    {"0c", "QSFP"},
    {"0d", "QSFP+"},
    {"11", "QSFP28"},
    {"NULL", ""}
};

struct rj_e2_enum_type_s sff8436_ext_type_of_transceiver[] = {
            {"00", "Power Class 1(1.5W max)"},
            {"04", "Power Class 1(1.5W max), CDR present in Tx"},
            {"08", "Power Class 1(1.5W max), CDR present in Rx"},
            {"0c", "Power Class 1(1.5W max), CDR present in Rx Tx"},
            {"10", "Power Class 1(1.5W max), CLEI present"},
            {"14", "Power Class 1(1.5W max), CLEI present, CDR present in Tx"},
            {"18", "Power Class 1(1.5W max), CLEI present, CDR present in Rx"},
            {"1c", "Power Class 1(1.5W max), CLEI present, CDR present in Rx Tx"},

            {"40", "Power Class 2(2.0W max)"},
            {"44", "Power Class 2(2.0W max), CDR present in Rx"},
            {"48", "Power Class 2(2.0W max), CDR present in Tx"},
            {"4c", "Power Class 2(2.0W max), CDR present in Rx Tx"},
            {"50", "Power Class 2(2.0W max), CLEI present"},
            {"54", "Power Class 2(2.0W max), CLEI present, CDR present in Rx"},
            {"58", "Power Class 2(2.0W max), CLEI present, CDR present in Tx"},
            {"5c", "Power Class 2(2.0W max), CLEI present, CDR present in Rx Tx"},

            {"80", "Power Class 3(2.5W max)"},
            {"84", "Power Class 3(2.5W max), CDR present in Rx"},
            {"88", "Power Class 3(2.5W max), CDR present in Tx"},
            {"8c", "Power Class 3(2.5W max), CDR present in Rx Tx"},
            {"90", "Power Class 3(2.5W max), CLEI present"},
            {"94", "Power Class 3(2.5W max), CLEI present, CDR present in Rx"},
            {"98", "Power Class 3(2.5W max), CLEI present, CDR present in Tx"},
            {"9c", "Power Class 3(2.5W max), CLEI present, CDR present in Rx Tx"},

            {"c0", "Power Class 4(3.5W max)"},
            {"c4", "Power Class 4(3.5W max), CDR present in Rx"},
            {"c8", "Power Class 4(3.5W max), CDR present in Tx"},
            {"cc", "Power Class 4(3.5W max), CDR present in Rx Tx"},
            {"d0", "Power Class 4(3.5W max), CLEI present"},
            {"d4", "Power Class 4(3.5W max), CLEI present, CDR present in Rx"},
            {"d8", "Power Class 4(3.5W max), CLEI present, CDR present in Tx"},
            {"dc", "Power Class 4(3.5W max), CLEI present, CDR present in Rx Tx"},
            {"NULL", ""}
        };

struct rj_e2_enum_type_s sff8436_connector[] = {
            {"00", "Unknown or unspecified"},
            {"01", "SC"},
            {"02", "FC Style 1 copper connector"},
            {"03", "FC Style 2 copper connector"},
            {"04", "BNC/TNC"},
            {"05", "FC coax headers"},
            {"06", "Fiberjack"},
            {"07", "LC"},
            {"08", "MT-RJ"},
            {"09", "MU"},
            {"0a", "SG"},
            {"0b", "Optical Pigtail"},
            {"0c", "MPOx12"},
            {"0d", "MPOx16"},
            {"20", "HSSDC II"},
            {"21", "Copper pigtail"},
            {"22", "RJ45"},
            {"23", "No separable connector"},
            {"NULL", ""}
};

struct rj_e2_enum_type_s sff8436_encoding_codes[] = {
            {"00","Unspecified"},
            {"01", "8B10B"},
            {"02", "4B5B"},
            {"03", "NRZ"},
            {"04", "SONET Scrambled"},
            {"05", "64B66B"},
            {"06", "Manchester"},
            {"07", "256B257B"},
            {"NULL", ""}
        };

struct rj_e2_enum_type_s sff8436_rate_identifier[] = {
        {"00","QSFP+ Rate Select Version 1"},
        {"NULL", ""}
    };

/* qsfp sepecification compliance*/
/* byte131 10/40G Ethernet Compliance Code */
struct rj_e2_enum_type_s qsfp_comp_10_ethcc_bitmap[8] = {
       {"0", "40G Active Cable (XLPPI)"},
       {"1", "40GBASE-LR4"},
       {"2", "40GBASE-SR4"},
       {"3", "40GBASE-CR4"},
       {"4", "10GBase-SR"},
       {"5", "10GBase-LR"},
       {"6", "10GBase-LRM"},
       {"NULL", ""},
};

/* byte132 SONET Compliance codes */
struct rj_e2_enum_type_s qsfp_comp_socc_bitmap[8] = {
       {"0", "OC 48 short reach"},
       {"1", "OC 48, intermediate reach"},
       {"2", "OC 48, long reach"},
       {"3", "40G OTN (OTU3B/OTU3C)"},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte133 SAS/SATA compliance codes */
struct rj_e2_enum_type_s qsfp_comp_sscc_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"4", "SAS 3.0G"},
       {"5", "SAS 6.0G"},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte134 Gigabit Ethernet Compliant codes */
struct rj_e2_enum_type_s qsfp_comp_gethcc_bitmap[8] = {
       {"0", "1000BASE-SX"},
       {"1", "1000BASE-LX"},
       {"2", "1000BASE-CX"},
       {"3", "1000BASE-T"},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte135 Fibre Channel link length */
struct rj_e2_enum_type_s qsfp_comp_fcll_bitmap[8] = {
       {"0", "Electrical inter-enclosure (EL)"},
       {"1", "Longwave laser (LC)"},
       {"NULL", ""},
       {"3", "Medium (M)"},
       {"4", "Long distance (L)"},
       {"5", "Intermediate distance (I)"},
       {"6", "Short distance (S)"},
       {"7", "Very long distance (V)"},
};

/* byte136 Transmitter Technology */
struct rj_e2_enum_type_s qsfp_comp_tt_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"4", "Longwave Laser (LL)"},
       {"5", "Shortwave laser w OFC (SL)"},
       {"6", "Shortwave laser w/o OFC (SN)"},
       {"7", "Electrical intra-enclosure"},
};

/* byte137 Fibre Channel transmission media */
struct rj_e2_enum_type_s qsfp_comp_fctm_bitmap[8] = {
       {"0", "Single Mode (SM)"},
       {"1", "Multi-mode 50um (OM3)"},
       {"2", "Multi-mode 50m (M5)"},
       {"3", "Multi-mode 62.5m (M6)"},
       {"4", "Video Coax (TV)"},
       {"5", "Miniature Coax (MI)"},
       {"6", "Shielded Twisted Pair (TP)"},
       {"7", "Twin Axial Pair (TW)"},
};

/* byte138 Fibre Channel Speed */
struct rj_e2_enum_type_s qsfp_comp_fcs_bitmap[8] = {
       {"0", "100 Mbytes/Sec"},
       {"NULL", ""},
       {"2", "200 Mbytes/Sec"},
       {"NULL", ""},
       {"4", "400 Mbytes/Sec"},
       {"5", "1600 Mbytes/Sec"},
       {"6", "800 Mbytes/Sec"},
       {"7", "1200 Mbytes/Sec"},
};

/* sfp sepecification compliance*/
/* byte3 10G Ethernet Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_10_ethcc_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"4", "10G Base-SR"},
       {"5", "10G Base-LR"},
       {"6", "10G Base-LRM"},
       {"7", "10G Base-ER"},
};

/* byte3 Infiniband Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_infcc_bitmap[8] = {
       {"0", "1X Copper Passive"},
       {"1", "1X Copper Active"},
       {"2", "1X LX"},
       {"3", "1X SX"},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte4 ESCON Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_escc_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"6", "ESCON SMF, 1310nm Laser"},
       {"7", "ESCON MMF, 1310nm LED"},
};

/* byte4 SONET Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_socc1_bitmap[8] = {
       {"0", "OC-48, short reach"},
       {"1", "OC-48, intermediate reach"},
       {"2", "OC-48, long reach"},
       {"3", "SONET reach specifier bit 2"},
       {"4", "SONET reach specifier bit 1"},
       {"5", "OC-192, short reach"},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte5 SONET Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_socc2_bitmap[8] = {
       {"0", "OC-3, short reach"},
       {"1", "OC-3, single mode, inter reach"},
       {"2", "OC-3, single mode, long reach"},
       {"NULL", ""},
       {"4", "OC-12, short reach"},
       {"5", "OC-12, single mode, inter reach"},
       {"6", "OC-12, single mode, long reach"},
       {"NULL", ""},
};

/* byte6 Ethernet Compliance Codes */
struct rj_e2_enum_type_s sfp_comp_ethcc_bitmap[8] = {
       {"0", "1000BASE-SX"},
       {"1", "1000BASE-LX"},
       {"2", "1000BASE-CX"},
       {"3", "1000BASE-T"},
       {"4", "100BASE-LX/LX10"},
       {"5", "100BASE-FX"},
       {"6", "BASE-BX10"},
       {"7", "BASE-PX"},
};

/* byte7 Fibre Channel Link Length */
struct rj_e2_enum_type_s sfp_comp_fcll_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"3", "medium distance (M)"},
       {"4", "Long distance (L)"},
       {"5", "Intermediate distance (I)"},
       {"6", "short distance (S)"},
       {"7", "very long distance (V)"},
};

/* byte7 Fibre Channel Technology */
struct rj_e2_enum_type_s sfp_comp_fct1_bitmap[8] = {
       {"0", "Electrical inter-enclosure (EL)"},
       {"1", "Longwave Laser (LC)"},
       {"2", "Shortwave laser, linear Rx (SA)"},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte8 Fibre Channel Technology */
struct rj_e2_enum_type_s sfp_comp_fct2_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"4", "Longwave laser (LL)"},
       {"5", "Shortwave laser with OFC (SL)"},
       {"6", "Shortwave laser w/o OFC (SN)"},
       {"7", "Electrical intra-enclosure (EL)"},
};

/* byte8 SFP+ Cable Technology */
struct rj_e2_enum_type_s sfp_comp_sfpct_bitmap[8] = {
       {"NULL", ""},
       {"NULL", ""},
       {"2", "Passive Cable"},
       {"3", "Active Cable"},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
       {"NULL", ""},
};

/* byte9 Fibre Channel Transmission Media */
struct rj_e2_enum_type_s sfp_comp_fctm_bitmap[8] = {
       {"0", "Single Mode (SM)"},
       {"NULL", ""},
       {"2", "Multimode, 50um (M5, M5E)"},
       {"3", "Multimode, 62.5um (M6)"},
       {"4", "Video Coax (TV)"},
       {"5", "Miniature Coax (MI)"},
       {"6", "Twisted Pair (TP)"},
       {"7", "Twin Axial Pair (TW)"},
};

/* byte10 Fibre Channel Speed */
struct rj_e2_enum_type_s sfp_comp_fcs_bitmap[8] = {
       {"0", "100 MBytes/sec"},
       {"NULL", ""},
       {"2", "200 MBytes/sec"},
       {"NULL", ""},
       {"4", "400 MBytes/sec"},
       {"5", "1600 MBytes/sec"},
       {"6", "800 MBytes/sec"},
       {"7", "1200 MBytes/sec"},
};

typedef enum {
    DOM_NONE,
    DOM_SFF8436,
    DOM_SFF8472,
} rj_dom_type_t;

typedef enum {
    EXTCALB_RX_PWR_4,
    EXTCALB_RX_PWR_3,
    EXTCALB_RX_PWR_2,
    EXTCALB_RX_PWR_1,
    EXTCALB_RX_PWR_0,
    EXTCALB_TX_I_Slope,
    EXTCALB_TX_I_Offset,
    EXTCALB_TX_PWR_Slope,
    EXTCALB_TX_PWR_Offset,
    EXTCALB_T_Slope,
    EXTCALB_T_Offset,
    EXTCALB_V_Slope,
    EXTCALB_V_Offset,
} rj_dom_ext_calibration_t;

struct rj_eeprom_parse_t_subaddr {
    int offset;
    int sub_offset;
    int size;
};

struct rj_eeprom_parse_t_ext {
    int subaddr_num;
    char prefix[10];
    struct rj_eeprom_parse_t_subaddr subaddr[4];
};

struct rj_eeprom_parse_t {
    char attr_name[32];
    int addr;
    int offset;
    int sub_offset;
    int size;
    rj_eeprom_type_t type;
    struct rj_e2_enum_type_s *enum_str;
    rj_dom_type_t dom_type;
    void (*calc_func) ( int sfp_bus,
            struct rj_eeprom_parse_t *eep_parse,
            unsigned char *data, char *out_buf, int buf_len);
   struct rj_eeprom_parse_t_ext ext;
};

static void calc_temperature(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void calc_voltage(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void sfp_calc_rx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void sfp_calc_tx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void sfp_calc_tx_bias(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void qsfp_calc_tx_bias(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void qsfp_calc_rx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void qsfp_calc_tx_power(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void sfp_cable_length_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void qsfp_cable_length_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void qsfp_spec_comp_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);
static void sfp_spec_comp_display(int sfp_bus, struct rj_eeprom_parse_t *eep_parse,
        unsigned char *data, char *out_buf, int buf_len);

/* QSFP DOM EEPROM is also at addr 0x50 */
struct rj_eeprom_parse_t ssf8436_interface_id[] = {
        {"type", 0x50, 128, 0, 1, E2_ENUM, sff8436_type_of_transceiver, DOM_NONE, NULL},
        {"hw_version", 0x50, 184, 0, 2, E2_STR, NULL, DOM_NONE, NULL},
        {"serial_num", 0x50, 196, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"manufacture_name", 0x50, 148, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"model_name", 0x50, 168, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"connector", 0x50, 130, 0, 1, E2_ENUM, sff8436_connector, DOM_NONE, NULL},
        {"encoding", 0x50, 139, 0, 1, E2_ENUM, sff8436_encoding_codes, DOM_NONE, NULL},
        {"ext_identifier", 0x50, 129, 0, 1, E2_ENUM, sff8436_ext_type_of_transceiver, DOM_NONE, NULL},
        {"ext_rateselect_compliance", 0x50, 141, 0, 1, E2_ENUM, sff8436_rate_identifier, DOM_NONE, NULL},
        {"cable_length", 0x50, 142, 0, 5, E2_FUNC, NULL, DOM_NONE, qsfp_cable_length_display},    /* 142 - 146*/
        {"nominal_bit_rate", 0x50, 140, 0, 1, E2_INT, NULL, DOM_NONE, NULL},
        {"sepecification_compliance", 0x50, 131, 0, 8, E2_FUNC, NULL, DOM_NONE, qsfp_spec_comp_display},  /* 131 - 138 */
        {"vendor_date", 0x50, 212, 0, 8, E2_DATE, NULL, DOM_NONE, NULL},
        {"vendor_oui", 0x50, 165, 0, 3, E2_HEX, NULL, DOM_NONE, NULL},

        {"temperature", 0x50, 22, 0, 2, E2_FUNC, NULL, DOM_SFF8436, calc_temperature},
        {"voltage", 0x50, 26, 0, 2, E2_FUNC, NULL, DOM_SFF8436, calc_voltage},
        {"rxpower", 0x50, 34, 0, 8, E2_FUNC, NULL, DOM_SFF8436, qsfp_calc_rx_power},
        {"txbias", 0x50, 42, 0, 8, E2_FUNC, NULL, DOM_SFF8436, qsfp_calc_tx_bias},
        {"txpower", 0x50, 50, 0, 8, E2_FUNC, NULL, DOM_SFF8436, qsfp_calc_tx_power},
};

struct rj_eeprom_parse_t ssf8472_interface_id[] = {
        {"type", 0x50, 0, 0, 1, E2_ENUM, sff8472_type_of_transceiver, DOM_NONE, NULL},
        {"hw_version", 0x50, 56, 0, 4, E2_STR, NULL, DOM_NONE, NULL},
        {"serial_num", 0x50, 68, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"manufacture_name", 0x50, 20, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"model_name", 0x50, 40, 0, 16, E2_STR, NULL, DOM_NONE, NULL},
        {"connector", 0x50, 2, 0, 1, E2_ENUM, sff8472_connector, DOM_NONE, NULL},
        {"encoding", 0x50, 11, 0, 1, E2_ENUM, sff8472_encoding_codes, DOM_NONE, NULL},
        {"ext_identifier", 0x50, 1, 0, 1, E2_ENUM, sff8472_exttypeoftransceiver, DOM_NONE, NULL},
        {"ext_rateselect_compliance", 0x50, 13, 0, 1, E2_ENUM, sff8472_rate_identifier, DOM_NONE, NULL},
        {"cable_length", 0x50, 14, 0, 6, E2_FUNC, NULL, DOM_NONE, sfp_cable_length_display}, /* 14~19 */
        {"nominal_bit_rate", 0x50, 12, 0, 1, E2_INT, NULL, DOM_NONE, NULL},
        {"sepecification_compliance", 0x50, 3, 0, 8, E2_FUNC, NULL, DOM_NONE, sfp_spec_comp_display},    /* 3~10 */
        {"vendor_date", 0x50, 84, 0, 8, E2_DATE, NULL, DOM_NONE, NULL},
        {"vendor_oui", 0x50, 37, 0, 3, E2_HEX, NULL, DOM_NONE, NULL},

        {"temperature", 0x50, 352, 0, 2, E2_FUNC, NULL, DOM_SFF8472, calc_temperature},
        {"voltage", 0x50, 354, 0, 2, E2_FUNC, NULL, DOM_SFF8472, calc_voltage},
        {"rxpower", 0x50, 360, 0, 2, E2_FUNC, NULL, DOM_SFF8472, sfp_calc_rx_power},
        {"txbias", 0x50, 356, 0, 2, E2_FUNC, NULL, DOM_SFF8472, sfp_calc_tx_bias},
        {"txpower", 0x50, 358, 0, 2, E2_FUNC, NULL, DOM_SFF8472, sfp_calc_tx_power},
};

#endif /* _DFD_SFPPARSE_H_ */
