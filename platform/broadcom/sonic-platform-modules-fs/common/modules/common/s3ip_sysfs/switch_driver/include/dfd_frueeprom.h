#ifndef _DFD_FRUEEPROM_H_
#define _DFD_FRUEEPROM_H_

/* Per IPMI v2.0 FRU specification */
typedef struct fru_common_header_s {
    uint8_t fixed;
    uint8_t internal_offset;
    uint8_t chassis_offset;
    uint8_t board_offset;
    uint8_t product_offset;
    uint8_t multi_offset;
    uint8_t pad;
    uint8_t crc;
} __attribute__((packed)) fru_common_header_t;

/* first byte in header is 1h per IPMI V2 spec. */

#define IPMI_FRU_HDR_BYTE_ZERO   1
#define IPMI_EIGHT_BYTES         8
#define IPMI_FRU_PRODUCT_AREA_MIN_LEN       (7)
#define IPMI_FRU_BOARD_AREA_MIN_LEN         (5)

#define IPMI_INVALID_BUS_ADDR               (-1)

#define IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX            512
#define IPMI_FRU_BOARD_INFO_MFG_TIME_LENGTH            3
#define IPMI_FRU_SENTINEL_VALUE                        0xC1
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK            0xC0
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT           0x06
#define IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK 0x3F
#define IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE   0x03

typedef enum {
    FRU_PRODUCT_AREA = 0,
    FRU_BOARD_AREA = 1
} ipmi_fru_area_flag_t;

struct ipmi_fru_field {
    uint8_t type_length_field[IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX];
    /* store length of data stored in buffer */
    unsigned int type_length_field_length;
};

typedef struct ipmi_fru_field ipmi_fru_field_t;

typedef struct ipmi_product_info_s {
    uint8_t *language_code;
    ipmi_fru_field_t *product_manufacturer_name;
    ipmi_fru_field_t *product_name;
    ipmi_fru_field_t *product_part_model_number;
    ipmi_fru_field_t *product_version;
    ipmi_fru_field_t *product_serial_number;
    ipmi_fru_field_t *product_asset_tag;
    ipmi_fru_field_t *product_fru_file_id;
    ipmi_fru_field_t *product_extra1;
    ipmi_fru_field_t *product_extra2;
    ipmi_fru_field_t *product_extra3;
    ipmi_fru_field_t *product_extra4;
} ipmi_product_info_t;

typedef struct ipmi_board_info_s {
    uint8_t *language_code;
    uint8_t *mfg_time;
    ipmi_fru_field_t *board_manufacturer;
    ipmi_fru_field_t *board_product_name;
    ipmi_fru_field_t *board_serial_number;
    ipmi_fru_field_t *board_part_number;
    ipmi_fru_field_t *board_fru_file_id;
    ipmi_fru_field_t *board_custom_fields;  /*hw version */
} ipmi_board_info_t;

/**
 * dfd_get_fru_data - Obtain product area FRU information
 * @bus:FRU E2 bus number
 * @dev_addr:FRU E2 Device address
 * @type: 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @buf:Data is stored in buf
 * @buf_len:buf length
 * @sysfs_name:sysfs attribute name
 * @returns: 0 success, negative value: failed
 */
int dfd_get_fru_data(int bus, int dev_addr, int type, uint8_t *buf, uint32_t buf_len, const char *sysfs_name);

/**
 * dfd_get_fru_product_data_by_file - Obtain product area FRU information by file
 * @file_path:sysfs file path
 * @type: 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @buf:Data is stored in buf
 * @buf_len:buf length
 * @returns: 0 success, negative value: failed
 */
int dfd_get_fru_product_data_by_file(char *file_path, int type, uint8_t *buf, uint32_t buf_len);
#if 0
/**
 * dfd_get_fru_board_data_by_file - Obtain board area FRU information by file
 * @file_path:sysfs file path
 * @type: 2: board name, 3: board serial number 5: hardware version number 6: board ID
 * @buf:Data is stored in buf
 * @buf_len:buf length
 * @returns: 0 success, negative value: failed
 */
int dfd_get_fru_board_data_by_file(const char *file_path, int type, uint8_t *buf, uint32_t buf_len);
#endif

/**
 * dfd_get_fru_board_data - Obtain the FRU information of the board area
 * @bus:FRU E2 bus number
 * @dev_addr:FRU E2 Device address
 * @type: 2: Product name, 3: product serial number 5: hardware version number
 * @buf:Data is stored in buf
 * @buf_len:buf length
 * @sysfs_name:sysfs attribute name
 * @returns: 0 success, negative value: failed
 */
int dfd_get_fru_board_data(int bus, int dev_addr, int type, uint8_t *buf, uint32_t buf_len, const char *sysfs_name);

#endif /* endif _DFD_FRUEEPROM_H_ */
