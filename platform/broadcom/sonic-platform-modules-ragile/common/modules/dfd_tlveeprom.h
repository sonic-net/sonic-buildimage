#ifndef DFD_OPENBMC_TLVEEPROM_H
#define DFD_OPENBMC_TLVEEPROM_H

#ifndef u_int8_t
#define u_int8_t unsigned char
#endif

#ifndef u_int16_t
#define u_int16_t unsigned short
#endif

#ifndef u_int32_t
#define u_int32_t unsigned int
#endif

#ifndef be16_to_cpu
#define be16_to_cpu(x)   ntohs(x)
#endif

#ifndef cpu_to_be16
#define cpu_to_be16(x)   htons(x)
#endif

/**
 *  The TLV Types.
 *
 *  Keep these in sync with tlv_code_list in cmd_sys_eeprom.c
 */
#define TLV_CODE_PRODUCT_NAME   0x21
#define TLV_CODE_PART_NUMBER    0x22
#define TLV_CODE_SERIAL_NUMBER  0x23
#define TLV_CODE_MAC_BASE       0x24
#define TLV_CODE_MANUF_DATE     0x25
#define TLV_CODE_DEVICE_VERSION 0x26
#define TLV_CODE_LABEL_REVISION 0x27
#define TLV_CODE_PLATFORM_NAME  0x28
#define TLV_CODE_ONIE_VERSION   0x29
#define TLV_CODE_MAC_SIZE       0x2A
#define TLV_CODE_MANUF_NAME     0x2B
#define TLV_CODE_MANUF_COUNTRY  0x2C
#define TLV_CODE_VENDOR_NAME    0x2D
#define TLV_CODE_DIAG_VERSION   0x2E
#define TLV_CODE_SERVICE_TAG    0x2F
#define TLV_CODE_VENDOR_EXT     0xFD
#define TLV_CODE_CRC_32         0xFE

#define TLV_CODE_NAME_LEN 64
/*
 *  Struct for displaying the TLV codes and names.
 */
struct tlv_code_desc {
    u_int8_t m_code;
    char m_name[TLV_CODE_NAME_LEN];
};

typedef struct dfd_tlv_type_s {
    u_int8_t main_type;
    u_int8_t ext_type;
} dfd_tlv_type_t;

// Header Field Constants
#define TLV_INFO_ID_STRING      "TlvInfo"
#define TLV_INFO_VERSION        0x01
/*#define TLV_TOTAL_LEN_MAX       (XXXXXXXX - sizeof(tlvinfo_header_t))*/

struct __attribute__ ((__packed__)) tlvinfo_header_s {
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    u_int8_t      version;  /* 0x08        Structure version */
    u_int16_t     totallen; /* 0x09 - 0x0A Length of all data which follows */
};
typedef struct tlvinfo_header_s tlvinfo_header_t;

/*
 * TlvInfo TLV: Layout of a TLV field
 */
struct __attribute__ ((__packed__)) tlvinfo_tlv_s {
    u_int8_t  type;
    u_int8_t  length;
    u_int8_t  value[0];
};
typedef struct tlvinfo_tlv_s tlvinfo_tlv_t;

#define TLV_VALUE_MAX_LEN        255
/*
 * The max decode value is currently for the 'raw' type or the 'vendor
 * extension' type, both of which have the same decode format.  The
 * max decode string size is computed as follows:
 *
 *   strlen(" 0xFF") * TLV_VALUE_MAX_LEN + 1
 *
 */
#define TLV_DECODE_VALUE_MAX_LEN    ((5 * TLV_VALUE_MAX_LEN) + 1)

typedef struct tlv_decode_value_s {
    u_int8_t value[TLV_DECODE_VALUE_MAX_LEN];
    u_int32_t length;
} tlv_decode_value_t;

typedef enum dfd_tlvinfo_ext_tlv_type_e {
    DFD_TLVINFO_EXT_TLV_TYPE_DEV_TYPE   = 1,
} dfd_tlvinfo_ext_tlv_type_t;

#if 0
#define TLV_TIME_LEN 64

int ipmi_tlv_validate_fru_area(const uint8_t fruid, const char *fru_file_name,
                           sd_bus *bus_type, const bool bmc_fru);

extern const char *get_vpd_key_names(int key_id);
extern std::string getService(sdbusplus::bus::bus& bus,
                         const std::string& intf,
                         const std::string& path);
extern std::string getFRUValue(const std::string& section,
                        const std::string& key,
                        const std::string& delimiter,
                        IPMIFruInfo& fruData);
#endif

int dfd_tlvinfo_get_e2prom_info(u_int8_t *eeprom, u_int32_t size, dfd_tlv_type_t *tlv_type, u_int8_t* buf, u_int8_t *buf_len);

#endif /* endif DFD_OPENBMC_TLVEEPROM_H */
