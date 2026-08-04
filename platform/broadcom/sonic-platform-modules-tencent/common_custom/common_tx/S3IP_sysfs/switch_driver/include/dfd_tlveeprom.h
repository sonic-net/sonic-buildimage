#ifndef _DFD_TLVEEPROM_H_
#define _DFD_TLVEEPROM_H_

#ifndef be16_to_cpu
#define be16_to_cpu(x)   ntohs(x)
#endif

#ifndef cpu_to_be16
#define cpu_to_be16(x)   htons(x)
#endif

#define TLV_CODE_NAME_LEN 64

/*
 *  Struct for displaying the TLV codes and names.
 */
struct tlv_code_desc {
    uint8_t m_code;
    char m_name[TLV_CODE_NAME_LEN];
};

typedef struct dfd_tlv_type_s {
    uint8_t main_type;
    uint8_t ext_type;
} dfd_tlv_type_t;

/* Header Field Constants */
#define TLV_INFO_ID_STRING      "TlvInfo"
#define TLV_INFO_VERSION        0x01

struct __attribute__ ((__packed__)) tlvinfo_header_s {
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    uint8_t      version;  /* 0x08        Structure version */
    uint16_t     totallen; /* 0x09 - 0x0A Length of all data which follows */
};
typedef struct tlvinfo_header_s tlvinfo_header_t;

/*
 * TlvInfo TLV: Layout of a TLV field
 */
struct __attribute__ ((__packed__)) tlvinfo_tlv_s {
    uint8_t  type;
    uint8_t  length;
    uint8_t  value[0];
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
    uint8_t value[TLV_DECODE_VALUE_MAX_LEN];
    uint32_t length;
} tlv_decode_value_t;

typedef enum dfd_tlvinfo_ext_tlv_type_e {
    DFD_TLVINFO_EXT_TLV_TYPE_DEV_TYPE   = 1,
} dfd_tlvinfo_ext_tlv_type_t;

int dfd_tlvinfo_get_e2prom_info(uint8_t *eeprom, uint32_t size, dfd_tlv_type_t *tlv_type, uint8_t* buf, uint32_t *buf_len);

#endif /* endif _DFD_TLVEEPROM_H_ */
