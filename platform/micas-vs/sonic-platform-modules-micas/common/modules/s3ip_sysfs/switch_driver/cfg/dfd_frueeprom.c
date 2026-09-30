/*
 * An dfd_frueeprom driver for frueeprom devcie function
 *
 * Copyright (C) 2024 Micas Networks Inc.
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

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "dfd_frueeprom.h"
#include "dfd_cfg_adapter.h"
#include "wb_module.h"

int g_dfd_fru_dbg_level = 0;
module_param(g_dfd_fru_dbg_level, int, S_IRUGO | S_IWUSR);

/**
 * Takes the pointer to stream of bytes and length
 * and returns the 8 bit checksum
 * This algo is per IPMI V2.0 spec
 */
static unsigned char ipmi_calculate_crc(const unsigned char *data, size_t len)
{
    char crc = 0;
    size_t byte = 0;

    for (byte = 0; byte < len; byte++) {
        crc += *data++;
    }

    return(-crc);
}

/* Validates the data for crc and mandatory fields */
static int ipmi_verify_fru_data(const uint8_t *data, const size_t len)
{
    uint8_t checksum = 0;
    int rc = -DFD_RV_TYPE_ERR;

    /* Validate for first byte to always have a value of [1] */
    if (data[0] != IPMI_FRU_HDR_BYTE_ZERO) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid entry:[%d] in byte-0\n",data[0]);
        return rc;
    } else {
        DBG_FRU_DEBUG(DBG_VERBOSE, "SUCCESS: Validated [0x%X] in entry_1 of fru_data\n",data[0]);
    }

    /* See if the calculated CRC matches with the embedded one.
     * CRC to be calculated on all except the last one that is CRC itself.*/
    checksum = ipmi_calculate_crc(data, len - 1);
    if (checksum != data[len-1]) {
        DBG_FRU_DEBUG(DBG_ERROR, "Checksum mismatch."
                      " Calculated:[0x%X], Embedded:[0x%X]\n",
                      checksum, data[len - 1]);
        return rc;
    } else {
        DBG_FRU_DEBUG(DBG_VERBOSE, "SUCCESS: Checksum matches:[0x%X]\n",checksum);
    }

    return 0;
}

/* private method to parse type/length */
static int ipmi_parse_type_length (const void *areabuf,
                                   unsigned int areabuflen,
                                   unsigned int current_area_offset,
                                   uint8_t *number_of_data_bytes,
                                   ipmi_fru_field_t *field)
{
    const uint8_t *areabufptr = (const uint8_t*) areabuf;
    uint8_t type_length;
    uint8_t type_code;

    type_length = areabufptr[current_area_offset];

    /* ipmi workaround
     *
     * dell p weredge r610
     *
     * my reading of the fru spec is that all non-custom fields are
     * required to be listed by the vendor.  however, on this
     * motherboard, some areas list this, indicating that there is
     * no more data to be parsed.  so now, for "required" fields, i
     * check to see if the type-length field is a sentinel before
     * calling this function.
     */

    type_code = (type_length & IPMI_FRU_TYPE_LENGTH_TYPE_CODE_MASK) >> IPMI_FRU_TYPE_LENGTH_TYPE_CODE_SHIFT;
    (*number_of_data_bytes) = type_length & IPMI_FRU_TYPE_LENGTH_NUMBER_OF_DATA_BYTES_MASK;

    /* special case: this shouldn't be a length of 0x01 (see type/length
     * byte format in fru information storage definition).
     */
    DBG_FRU_DEBUG(DBG_VERBOSE, "areabuflen:%d, current_area_offset:0x%x, type_code:0x%x, number_of_data_bytes:%d\n",
                  areabuflen, current_area_offset, type_code, *number_of_data_bytes);
#if 0
    if (type_code == IPMI_FRU_TYPE_LENGTH_TYPE_CODE_LANGUAGE_CODE
        && (*number_of_data_bytes) == 0x01) {
        DBG_FRU_DEBUG(DBG_ERROR, "fru type length error.value:0x%x\n", type_length);
        return (-1);
    }
#endif
    if ((current_area_offset + 1 + (*number_of_data_bytes)) > areabuflen) {
        DBG_FRU_DEBUG(DBG_ERROR, "buf length error. current_area_offset:0x%x, need length:%d, total length:0x%x\n",
                      current_area_offset, *number_of_data_bytes, areabuflen);
        return (-1);
    }

    if (field) {
        mem_clear (field->type_length_field, IPMI_FRU_AREA_TYPE_LENGTH_FIELD_MAX);
        memcpy (field->type_length_field, &areabufptr[current_area_offset + 1], *number_of_data_bytes);
        DBG_FRU_DEBUG(DBG_VERBOSE, "fru parse ok. value:%s\n", field->type_length_field);
        field->type_length_field_length = *number_of_data_bytes;
    }

    return (0);
}

static int ipmi_fru_product_info_area(const void *areabuf,
                                      unsigned int areabuflen, ipmi_product_info_t *ipmi_product_info)
{
    const uint8_t *areabufptr = (const uint8_t*) areabuf;
    unsigned int area_offset = 2;
    uint8_t number_of_data_bytes;
    int rv;
    ipmi_fru_field_t **ipmi_fru_field_point;
    int ipmi_fru_field_len, i;

    if (!areabuf || !areabuflen || !ipmi_product_info) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid Parameter.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    /* Verify the crc and size */
    rv = ipmi_verify_fru_data(areabuf, areabuflen);
    if (rv < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Failed to validate fru product info data\n");
        return rv;
    }

    ipmi_fru_field_len = (sizeof(ipmi_product_info_t) - sizeof(uint8_t *)) /(sizeof(ipmi_fru_field_t *));

    if (ipmi_product_info->language_code) {
        (*ipmi_product_info->language_code) = areabufptr[area_offset];
    }
    area_offset++;
    ipmi_fru_field_point = (ipmi_fru_field_t **)((uint8_t *)ipmi_product_info + sizeof(uint8_t *));
    for (i = 0; i < ipmi_fru_field_len; i++) {
        if (*ipmi_fru_field_point) {
            mem_clear(*ipmi_fru_field_point, sizeof(ipmi_fru_field_t));
        }

        if (((areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE) && (i >= IPMI_FRU_PRODUCT_AREA_MIN_LEN))
            || (area_offset == areabuflen - 1)) {
            rv = 0;
            break;
        }

        rv = ipmi_parse_type_length(areabufptr, areabuflen, area_offset, &number_of_data_bytes, *ipmi_fru_field_point);
        if (rv < 0) {
            DBG_FRU_DEBUG(DBG_ERROR, "[%d] _parse_type_length area_offset[%d] rv=%d \n", i, area_offset, rv);
            break;
        }

        area_offset += 1;          /* type/length byte */
        area_offset += number_of_data_bytes;
        ipmi_fru_field_point++;
    }

    return (rv);
}

static int ipmi_fru_board_info_area(const void *areabuf,
                                    unsigned int areabuflen, ipmi_board_info_t *ipmi_board_info)
{
    const uint8_t *areabufptr = (const uint8_t*) areabuf;
    unsigned int area_offset = 2;
    uint8_t number_of_data_bytes;
    int rv;
    ipmi_fru_field_t **ipmi_fru_field_point;
    int ipmi_fru_field_len, i;

    if (!areabuf || !areabuflen || !ipmi_board_info) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid Parameter.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    /* Verify the crc and size */
    rv = ipmi_verify_fru_data(areabuf, areabuflen);
    if (rv < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Failed to validate fru product info data\n");
        return rv;
    }

    ipmi_fru_field_len = (sizeof(ipmi_board_info_t) - sizeof(uint8_t *) - sizeof(uint8_t *)) /(sizeof(ipmi_fru_field_t *));

    if (ipmi_board_info->language_code) {
        (*ipmi_board_info->language_code) = areabufptr[area_offset];
    }
    area_offset++;

    if (ipmi_board_info->mfg_time) {
        memcpy(ipmi_board_info->mfg_time, &areabufptr[area_offset], IPMI_FRU_BOARD_INFO_MFG_TIME_LENGTH);
    }
    area_offset += IPMI_FRU_BOARD_INFO_MFG_TIME_LENGTH;
    ipmi_fru_field_point = (ipmi_fru_field_t **)((uint8_t *)ipmi_board_info + sizeof(uint8_t *) + sizeof(uint8_t *));
    for (i = 0; i < ipmi_fru_field_len; i++) {
        if (*ipmi_fru_field_point) {
            mem_clear(*ipmi_fru_field_point, sizeof(ipmi_fru_field_t));
        }

        if (((areabufptr[area_offset] == IPMI_FRU_SENTINEL_VALUE) && (i >= IPMI_FRU_BOARD_AREA_MIN_LEN))
            || (area_offset == areabuflen - 1)) {
            rv = 0;
            break;
        }

        rv = ipmi_parse_type_length(areabufptr, areabuflen, area_offset, &number_of_data_bytes, *ipmi_fru_field_point);
        if (rv < 0) {
            DBG_FRU_DEBUG(DBG_ERROR, "[%d] _parse_type_length area_offset[%d] rv=%d \n", i, area_offset, rv);
            break;
        }

        area_offset += 1;          /* type/length byte */
        area_offset += number_of_data_bytes;
        ipmi_fru_field_point++;
    }

    return (rv);
}

/**
 * Validates the fru data per ipmi common header constructs.
 * Returns with updated common_hdr and also file_size
 */
static int ipmi_validate_common_hdr(const uint8_t *fru_data, const size_t data_len)
{
    int rc = -1;

    uint8_t common_hdr[sizeof(fru_common_header_t)] = {0};
    if (data_len >= sizeof(common_hdr)) {
        memcpy(common_hdr, fru_data, sizeof(common_hdr));
    } else {
        DBG_FRU_DEBUG(DBG_ERROR, "Incomplete fru data file. Size:[%zd]\n", data_len);
        return rc;
    }

    /* Verify the crc and size */
    rc = ipmi_verify_fru_data(common_hdr, sizeof(common_hdr));
    if (rc < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Failed to validate common header\n");
        return rc;
    }

    return 0;
}

/* Header information acquisition */
static int dfd_get_frue2prom_info(int bus, int dev_addr, fru_common_header_t *info, const char *sysfs_name)
{
    int ret;
    uint8_t fru_common_header_info[sizeof(fru_common_header_t)];

    if (info == NULL) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid parameter!\n");
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_ko_i2c_read(bus, dev_addr, 0, (uint8_t *)info, sizeof(fru_common_header_t), sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Read eeprom head info error(bus: %d, addr: 0x%02x).\n", bus, dev_addr);
        return ret;
    }

    memcpy(fru_common_header_info, (uint8_t *)info, sizeof(fru_common_header_t));

    if (ipmi_validate_common_hdr(fru_common_header_info, sizeof(fru_common_header_t)) != 0) {
        return -DFD_RV_TYPE_ERR;
    }

    return DFD_RV_OK;
}

static int dfd_set_fru_product_info(ipmi_product_info_t *ipmi_product_info, ipmi_fru_field_t *vpd_info, int type)
{
    int ret;
    ret = DFD_RV_OK;
    if (ipmi_product_info == NULL || vpd_info == NULL) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid parameter!\n");
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear((uint8_t *)ipmi_product_info, sizeof(ipmi_product_info_t));
    switch (type) {
    case DFD_DEV_INFO_TYPE_SN:
        ipmi_product_info->product_serial_number = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_NAME:
        ipmi_product_info->product_name = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_DEV_TYPE:
        ipmi_product_info->product_type_fields = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_HW_INFO:
        ipmi_product_info->product_version = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_VENDOR:
        ipmi_product_info->product_manufacturer_name = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_PART_NUMBER:
        ipmi_product_info->product_part_model_number = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_ASSET_TAG:
        ipmi_product_info->product_asset_tag = vpd_info;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

static int dfd_set_fru_board_info(ipmi_board_info_t *ipmi_board_info, ipmi_fru_field_t *vpd_info, int type)
{
    int ret;
    ret = DFD_RV_OK;
    if (ipmi_board_info == NULL || vpd_info == NULL) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid parameter!\n");
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear((uint8_t *)ipmi_board_info, sizeof(ipmi_board_info_t));
    switch (type) {
    case DFD_DEV_INFO_TYPE_SN:
        ipmi_board_info->board_serial_number = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_NAME:
        ipmi_board_info->board_product_name = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_HW_INFO:
        ipmi_board_info->board_custom_fields = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_PART_NUMBER:
        ipmi_board_info->board_part_number = vpd_info;
        break;
    case DFD_DEV_INFO_TYPE_VENDOR:
        ipmi_board_info->board_manufacturer = vpd_info;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

/**
 * dfd_get_fru_data - Obtain product area FRU information
 * @bus:FRU E2 bus number
 * @dev_addr:FRU E2 Device address
 * @type 2: Product name, 3: product serial number 5: hardware version number 6: product ID
 * @buf: Data is stored in buf
 * @buf_len:buf length
 * @sysfs_name:sysfs attribute name
 * @returns:0 success, negative value: failed
 */
int dfd_get_fru_data(int bus, int dev_addr, int type, uint8_t *buf, uint32_t buf_len, const char *sysfs_name)
{
    fru_common_header_t info;
    uint8_t *fru_data;
    int ret;
    uint8_t fru_len;
    ipmi_product_info_t ipmi_product_info;
    ipmi_fru_field_t vpd_info;
    int product_offset;
    int fru_len_tmp;

    if (buf == NULL || buf_len <= 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid parameter!\n");
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_FRU_DEBUG(DBG_VERBOSE, "Read fru eeprom (bus: %d, addr: 0x%02x, type:%d, buf: %p, len: %d).\n",
                  bus, dev_addr, type, buf, buf_len);

    ret = dfd_get_frue2prom_info(bus, dev_addr, &info, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Read eeprom info head error(bus: %d, addr: 0x%02x, buf: %p, len: %d).\n",
                      bus, dev_addr, buf, buf_len);
        return ret;
    }

    product_offset = info.product_offset * IPMI_EIGHT_BYTES;
    ret = dfd_ko_i2c_read(bus, dev_addr, product_offset + 1, &fru_len, 1, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "read eeprom info product_offset(bus: %d, addr: 0x%02x, product offset:%d).\n",
                      bus, dev_addr, info.product_offset);
        return -DFD_RV_DEV_FAIL;
    }

    fru_len_tmp = fru_len * IPMI_EIGHT_BYTES;
    fru_data = (uint8_t *)kmalloc(sizeof(uint8_t) * fru_len_tmp, GFP_KERNEL);
    if (fru_data == NULL) {
        DBG_FRU_DEBUG(DBG_ERROR, "Allocate buffer(len:%d) error!\n", fru_len_tmp);
        return -DFD_RV_NO_MEMORY;
    }

    ret = dfd_ko_i2c_read(bus, dev_addr, product_offset, fru_data, fru_len_tmp, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Get FRU data error.\n");
        kfree(fru_data);
        return ret;
    }

    mem_clear((uint8_t *)&vpd_info, sizeof(ipmi_fru_field_t));
    ret = dfd_set_fru_product_info(&ipmi_product_info, &vpd_info, type);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Not support to get info: %d.\n", type);
        kfree(fru_data);
        return ret;
    }

    ret = ipmi_fru_product_info_area(fru_data, fru_len_tmp, &ipmi_product_info);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "analysis FRU product info error.\n");
        kfree(fru_data);
        return ret;
    }

    kfree(fru_data);

    buf_len = buf_len < vpd_info.type_length_field_length ? buf_len : vpd_info.type_length_field_length;
    memcpy(buf, (uint8_t *)&vpd_info, buf_len);

    return DFD_RV_OK;
}

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
int dfd_get_fru_board_data(int bus, int dev_addr, int type, uint8_t *buf, uint32_t buf_len, const char *sysfs_name)
{
    fru_common_header_t info;
    uint8_t *fru_data;
    int ret;
    uint8_t fru_len;
    ipmi_board_info_t ipmi_board_info;
    ipmi_fru_field_t vpd_info;
    int board_offset;
    int fru_len_tmp;

    if (buf == NULL || buf_len <= 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Invalid parameter!\n");
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_FRU_DEBUG(DBG_VERBOSE, "Read fru eeprom (bus: %d, addr: 0x%02x, type:%d, buf: %p, len: %d).\n",
                  bus, dev_addr, type, buf, buf_len);

    ret = dfd_get_frue2prom_info(bus, dev_addr, &info, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Read eeprom info head error(bus: %d, addr: 0x%02x, buf: %p, len: %d).\n",
                      bus, dev_addr, buf, buf_len);
        return ret;
    }

    board_offset = info.board_offset * IPMI_EIGHT_BYTES;
    ret = dfd_ko_i2c_read(bus, dev_addr, board_offset + 1, &fru_len, 1, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "read eeprom info product_offset(bus: %d, addr: 0x%02x, product offset:%d).\n",
                      bus, dev_addr, info.board_offset);
        return -DFD_RV_DEV_FAIL;
    }

    fru_len_tmp = fru_len * IPMI_EIGHT_BYTES;
    fru_data = (uint8_t *)kmalloc(sizeof(uint8_t) * fru_len_tmp, GFP_KERNEL);
    if (fru_data == NULL) {
        DBG_FRU_DEBUG(DBG_ERROR, "Allocate buffer(len:%d) error!\n", fru_len_tmp);
        return -DFD_RV_NO_MEMORY;
    }

    ret = dfd_ko_i2c_read(bus, dev_addr, board_offset, fru_data, fru_len_tmp, sysfs_name);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Get FRU data error.\n");
        kfree(fru_data);
        return ret;
    }

    mem_clear((uint8_t *)&vpd_info, sizeof(ipmi_fru_field_t));
    ret = dfd_set_fru_board_info(&ipmi_board_info, &vpd_info, type);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "Not support to get info: %d.\n", type);
        kfree(fru_data);
        return ret;
    }

    ret = ipmi_fru_board_info_area(fru_data, fru_len_tmp, &ipmi_board_info);
    if (ret < 0) {
        DBG_FRU_DEBUG(DBG_ERROR, "analysis FRU product info error.\n");
        kfree(fru_data);
        return ret;
    }

    kfree(fru_data);

    buf_len = buf_len < vpd_info.type_length_field_length ? buf_len : vpd_info.type_length_field_length;
    memcpy(buf, (uint8_t *)&vpd_info, buf_len);

    return DFD_RV_OK;
}
