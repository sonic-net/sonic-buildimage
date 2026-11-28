#ifndef _DFD_WBTLV_EEPROM_H_
#define _DFD_WBTLV_EEPROM_H_

#define WB_TLV_EEPROM_HEAD "\x01\x7e\x01\xf1"

typedef struct  dfd_dev_head_info_s {
    uint8_t   ver;                       /* The version number defined in the E2PROM file, initially 0x01 */
    uint8_t   flag;                      /* The new version E2PROM is identified as 0x7E */
    uint8_t   hw_ver;                    /* It consists of two parts: the main version number and the revised version */
    uint8_t   type;                      /* Hardware type definition information */
    int16_t   tlv_len;                   /* Valid data length (16 bits) */
} dfd_dev_head_info_t;

typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;                       /* Data type */
    uint8_t  len;                        /* Data length */
    uint8_t  data[0];                    /* Data */
} dfd_dev_tlv_info_t;

/**
 * dfd_wb_tlv_support_type - Check cmd is wb tlv support type or not
 * @cmd: Only suooprt 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @returns: true: support, false: not support
 */
bool dfd_wb_tlv_support_type(uint8_t cmd);

/**
 * dfd_wb_tlv_eeprom_read - Obtain wb tlv eeprom information
 * @bus: E2 bus number
 * @addr: E2 Device address
 * @cmd: 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @buf:Data is stored in buf
 * @len:buf length
 * @sysfs_name:sysfs attribute name
 * @returns: 0 success, negative value: failed
 */
int dfd_wb_tlv_eeprom_read(int bus, int addr, uint8_t cmd, char *buf, int len,
               const char *sysfs_name);


#endif /* endif _DFD_WBTLV_EEPROM_H_ */
