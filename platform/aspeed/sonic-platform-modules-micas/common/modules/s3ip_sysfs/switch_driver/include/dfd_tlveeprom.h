#ifndef _DFD_TLVEEPROM_H_
#define _DFD_TLVEEPROM_H_

#include "wb_module.h"

#ifndef be16_to_cpu
#define be16_to_cpu(x)   ntohs(x)
#endif

#ifndef cpu_to_be16
#define cpu_to_be16(x)   htons(x)
#endif

#define TLV_CODE_NAME_LEN 64

#define TLV_CODE_PRODUCT_NAME   (0x21)
#define TLV_CODE_PART_NUMBER    (0x22)
#define TLV_CODE_SERIAL_NUMBER  (0x23)
#define TLV_CODE_MAC_BASE       (0x24)
#define TLV_CODE_MANUF_DATE     (0x25)
#define TLV_CODE_DEVICE_VERSION (0x26)
#define TLV_CODE_LABEL_REVISION (0x27)
#define TLV_CODE_PLATFORM_NAME  (0x28)
#define TLV_CODE_ONIE_VERSION   (0x29)
#define TLV_CODE_MAC_SIZE       (0x2A)
#define TLV_CODE_MANUF_NAME     (0x2B)
#define TLV_CODE_MANUF_COUNTRY  (0x2C)
#define TLV_CODE_VENDOR_NAME    (0x2D)
#define TLV_CODE_DIAG_VERSION   (0x2E)
#define TLV_CODE_SERVICE_TAG    (0x2F)
#define TLV_CODE_VENDOR_EXT     (0xFD)
#define TLV_CODE_CRC_32         (0xFE)
#define TLV_CODE_NO_DEFINED     (-1)

/*
 *  Struct for displaying the TLV codes and names.
 */
struct tlv_code_desc {
    uint8_t m_code;
    char m_name[TLV_CODE_NAME_LEN];
};

struct tlv_type_cmd_table {
    int info_type;    /* dev_info_type */
    int tlv_type;     /* TLV TYPE type */
};

static const struct tlv_type_cmd_table tlv_type_cmd_table_list[] = {
    {DFD_DEV_INFO_TYPE_MAC, TLV_CODE_MAC_BASE},
    {DFD_DEV_INFO_TYPE_NAME, TLV_CODE_PRODUCT_NAME},
    {DFD_DEV_INFO_TYPE_SN, TLV_CODE_SERIAL_NUMBER},
};

/* ONIE TLV Type Type and extended TLV type definition */
typedef struct dfd_tlv_type_s {
    uint8_t main_type;    /* ONIE standard TLV TYPE */
    uint8_t ext_type;     /* Extended TLV TYPE type */
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

int dfd_tlvinfo_get_e2prom_info(uint8_t *eeprom, uint32_t size, int cmd, uint8_t* buf, uint32_t *buf_len);

#endif /* endif _DFD_TLVEEPROM_H_ */
