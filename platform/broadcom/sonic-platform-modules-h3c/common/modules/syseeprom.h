/*
* Copyright (c) 2019  <sonic@h3c.com>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __SYSEEPROM_H__
#define __SYSEEPROM_H__

#define DECODE_NAME_MAX             20
#define TLV_VALUE_MAX_LEN           255
#define TLV_DECODE_VALUE_MAX_LEN    ((5 * TLV_VALUE_MAX_LEN) + 1)
#define TLV_INFO_ID_STRING          "TlvInfo"
#define TLV_INFO_VERSION            0x01
#define TLV_TOTAL_LEN_MAX           (SYS_EEPROM_SIZE - sizeof(tlvinfo_header_t))
#define CONFIG_SYS_EEPROM_SIZE      2048
#define SYS_EEPROM_SIZE             CONFIG_SYS_EEPROM_SIZE

#define DBG_ECHO(level, fmt, args...) DEBUG_PRINT(bsp_module_debug_level[BSP_SYSEEPROM_MODULE], level, BSP_LOG_FILE, fmt, ##args)

#define MODULE_NAME                 "syseeprom"

#define H3C_BSP_VERSION             "1.1"

enum fan_eeprom_attributes
{
    FAN_VENDOR,
    FAN_PRODUCT_NAME,
    FAN_SN,
    FAN_PN,
    FAN_HW_VERSION,
};

struct    __attribute__((__packed__))  tlvinfo_header_s
{
    char    signature[8];   /* 0x00 - 0x07 EEPROM Tag "TlvInfo" */
    char      version;  /* 0x08        Structure version */
    unsigned short int    totallen; /* 0x09 - 0x0A Length of all data which follows */
};

struct __attribute__((__packed__)) tlvinfo_tlv_s
{
    unsigned  char type;
    u_int8_t length;
    unsigned  char  value[0];
};

extern int bsp_syseeprom_write_buf(u8 *buf, size_t count);
extern ssize_t bsp_syseeprom_sysfs_read_model_name(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_part_number(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_manuf_date(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_device_version(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_mac_address(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_label_revision(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_platform_name(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_switch_mac_address(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_onie_version(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_manufacturer(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_manuf_country(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_vendor_name(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_vendor_ext(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_mac_nums(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_eeprom(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_serial_number(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_crc32(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_service_tag(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_vendor_name(char *buf);
extern ssize_t bsp_syseeprom_sysfs_read_diag_version(char *buf);

#endif /* __SYSEEPROM_H__ */
