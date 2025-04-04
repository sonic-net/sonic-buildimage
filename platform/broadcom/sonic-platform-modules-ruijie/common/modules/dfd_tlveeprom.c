/*
 * Copyright (C) 2003-2014 FreeIPMI Core Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
/*****************************************************************************\
 *  Copyright (C) 2007-2014 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Albert Chu <chu11@llnl.gov>
 *  UCRL-CODE-232183
 *
 *  This file is part of Ipmi-fru, a tool used for retrieving
 *  motherboard field replaceable unit (FRU) information. For details,
 *  see http://www.llnl.gov/linux/.
 *
 *  Ipmi-fru is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 3 of the License, or (at your
 *  option) any later version.
 *
 *  Ipmi-fru is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with Ipmi-fru.  If not, see <http://www.gnu.org/licenses/>.
\*****************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include "platform_common.h"
#include "dfd_tlveeprom.h"

/* using in is_valid_tlvinfo_header */
static u_int32_t eeprom_size;

/*
 *  List of TLV codes and names.
 */
static const struct tlv_code_desc tlv_code_list[] = {
    { TLV_CODE_PRODUCT_NAME      , "Product Name"},
    { TLV_CODE_PART_NUMBER       , "Part Number"},
    { TLV_CODE_SERIAL_NUMBER     , "Serial Number"},
    { TLV_CODE_MAC_BASE          , "Base MAC Address"},
    { TLV_CODE_MANUF_DATE        , "Manufacture Date"},
    { TLV_CODE_DEVICE_VERSION    , "Device Version"},
    { TLV_CODE_LABEL_REVISION    , "Label Revision"},
    { TLV_CODE_PLATFORM_NAME     , "Platform Name"},
    { TLV_CODE_ONIE_VERSION      , "ONIE Version"},
    { TLV_CODE_MAC_SIZE          , "MAC Addresses"},
    { TLV_CODE_MANUF_NAME        , "Manufacturer"},
    { TLV_CODE_MANUF_COUNTRY     , "Country Code"},
    { TLV_CODE_VENDOR_NAME       , "Vendor Name"},
    { TLV_CODE_DIAG_VERSION      , "Diag Version"},
    { TLV_CODE_SERVICE_TAG       , "Service Tag"},
    { TLV_CODE_VENDOR_EXT        , "Vendor Extension"},
    { TLV_CODE_CRC_32            , "CRC-32"},
};

#if 0
#define OPENBMC_VPD_KEY_INVAIL_VAL 0

static const tlv_code_map_t tlv_code_map[] = {
    { TLV_CODE_PRODUCT_NAME   , OPENBMC_VPD_KEY_PRODUCT_NAME},
    { TLV_CODE_PART_NUMBER    , OPENBMC_VPD_KEY_PRODUCT_PART_MODEL_NUM},
    { TLV_CODE_SERIAL_NUMBER  , OPENBMC_VPD_KEY_PRODUCT_SERIAL_NUM},
    { TLV_CODE_MAC_BASE       , OPENBMC_VPD_KEY_INVAIL_VAL},
    { TLV_CODE_MANUF_DATE     , OPENBMC_VPD_KEY_BOARD_MFG_DATE},
    { TLV_CODE_DEVICE_VERSION , OPENBMC_VPD_KEY_PRODUCT_VER},
    { TLV_CODE_LABEL_REVISION , OPENBMC_VPD_KEY_PRODUCT_CUSTOM7},
    { TLV_CODE_PLATFORM_NAME  , OPENBMC_VPD_KEY_PRODUCT_CUSTOM1},
    { TLV_CODE_ONIE_VERSION   , OPENBMC_VPD_KEY_PRODUCT_CUSTOM2},
    { TLV_CODE_MAC_SIZE       , OPENBMC_VPD_KEY_INVAIL_VAL},
    { TLV_CODE_MANUF_NAME     , OPENBMC_VPD_KEY_PRODUCT_MFR},
    { TLV_CODE_MANUF_COUNTRY  , OPENBMC_VPD_KEY_PRODUCT_CUSTOM3},
    { TLV_CODE_VENDOR_NAME    , OPENBMC_VPD_KEY_PRODUCT_CUSTOM4},
    { TLV_CODE_DIAG_VERSION   , OPENBMC_VPD_KEY_PRODUCT_CUSTOM8},
    { TLV_CODE_SERVICE_TAG    , OPENBMC_VPD_KEY_PRODUCT_CUSTOM5},
    { TLV_CODE_VENDOR_EXT     , OPENBMC_VPD_KEY_PRODUCT_CUSTOM6},
    { TLV_CODE_CRC_32         , OPENBMC_VPD_KEY_INVAIL_VAL},
};
#endif

#define TLV_CODE_NUM (sizeof(tlv_code_list) / sizeof(tlv_code_list[0]))

#if 0
#define TLV_CODE_MAP_NUM (sizeof(tlv_code_map) / sizeof(tlv_code_map[0]))
#endif

const unsigned long crc_table[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d,
};

static unsigned long crc32(unsigned long crc, const unsigned char *buf, unsigned len)
{
    unsigned i;
    if (len < 1)
        return 0xffffffff;

    for (i = 0; i != len; ++i)
    {
        crc = crc_table[(crc ^ buf[i]) & 0xff] ^ (crc >> 8);
    }

    crc = crc ^ 0xffffffff;

    return crc;
}

/*
 *  is_valid_tlv
 *
 *  Perform basic sanity checks on a TLV field. The TLV is pointed to
 *  by the parameter provided.
 *      1. The type code is not reserved (0x00 or 0xFF)
 */
static inline bool is_valid_tlv(tlvinfo_tlv_t *tlv)
{
    return ((tlv->type != 0x00) && (tlv->type != 0xFF));
}

/*
 *  is_valid_tlvinfo_header
 *
 *  Perform sanity checks on the first 11 bytes of the TlvInfo EEPROM
 *  data pointed to by the parameter:
 *      1. First 8 bytes contain null-terminated ASCII string "TlvInfo"
 *      2. Version byte is 1
 *      3. Total length bytes contain value which is less than or equal
 *         to the allowed maximum (2048-11)
 *
 */
static inline bool is_valid_tlvinfo_header(tlvinfo_header_t *hdr)
{
    int max_size = eeprom_size;
    return((strcmp(hdr->signature, TLV_INFO_ID_STRING) == 0) &&
       (hdr->version == TLV_INFO_VERSION) &&
       (be16_to_cpu(hdr->totallen) <= max_size) );
}

/*
 *  decode_tlv_value
 *
 *  Decode a single TLV value into a string.

 *  The validity of EEPROM contents and the TLV field have been verified
 *  prior to calling this function.
 */
static void decode_tlv_value(tlvinfo_tlv_t *tlv, tlv_decode_value_t *decode_value)
{
    int i;
    char *value;
    u_int32_t length;

    value = (char *)decode_value->value;

    switch (tlv->type) {
    case TLV_CODE_PRODUCT_NAME:
    case TLV_CODE_PART_NUMBER:
    case TLV_CODE_SERIAL_NUMBER:
    case TLV_CODE_MANUF_DATE:
    case TLV_CODE_LABEL_REVISION:
    case TLV_CODE_PLATFORM_NAME:
    case TLV_CODE_ONIE_VERSION:
    case TLV_CODE_MANUF_NAME:
    case TLV_CODE_MANUF_COUNTRY:
    case TLV_CODE_VENDOR_NAME:
    case TLV_CODE_DIAG_VERSION:
    case TLV_CODE_SERVICE_TAG:
    case TLV_CODE_VENDOR_EXT:
        memcpy(value, tlv->value, tlv->length);
        value[tlv->length] = 0;
        length = tlv->length;
        break;
    case TLV_CODE_MAC_BASE:
        length = sprintf(value, "%02X:%02X:%02X:%02X:%02X:%02X",
                tlv->value[0], tlv->value[1], tlv->value[2],
                tlv->value[3], tlv->value[4], tlv->value[5]);
        break;
    case TLV_CODE_DEVICE_VERSION:
        length = sprintf(value, "%u", tlv->value[0]);
        break;
    case TLV_CODE_MAC_SIZE:
        length = sprintf(value, "%u", (tlv->value[0] << 8) | tlv->value[1]);
        break;
    #if 0
    case TLV_CODE_VENDOR_EXT:
        value[0] = 0;
        length = 0;
        for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length); i++) {
            length += sprintf(value, "%s 0x%02X", value, tlv->value[i]);
        }
        break;
    #endif
    case TLV_CODE_CRC_32:
        length = sprintf(value, "0x%02X%02X%02X%02X", tlv->value[0],
                tlv->value[1], tlv->value[2], tlv->value[3]);
        break;
    default:
        value[0] = 0;
        length = 0;
        for (i = 0; (i < (TLV_DECODE_VALUE_MAX_LEN/5)) && (i < tlv->length); i++) {
            length += sprintf(value, "%s 0x%02X", value, tlv->value[i]);
        }
        break;
    }

    decode_value->length = length;
}

/*
 *  is_checksum_valid
 *
 *  Validate the checksum in the provided TlvInfo EEPROM data. First,
 *  verify that the TlvInfo header is valid, then make sure the last
 *  TLV is a CRC-32 TLV. Then calculate the CRC over the EEPROM data
 *  and compare it to the value stored in the EEPROM CRC-32 TLV.
 */
static bool is_checksum_valid(u_int8_t *eeprom)
{
    tlvinfo_header_t *eeprom_hdr;
    tlvinfo_tlv_t *eeprom_crc;
    unsigned int calc_crc;
    unsigned int stored_crc;

    eeprom_hdr = (tlvinfo_header_t *) eeprom;

    // Is the eeprom header valid?
    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
        return false;
    }

    // Is the last TLV a CRC?
    eeprom_crc = (tlvinfo_tlv_t *) &eeprom[sizeof(tlvinfo_header_t) +
        be16_to_cpu(eeprom_hdr->totallen) - (sizeof(tlvinfo_tlv_t) + 4)];
    if ((eeprom_crc->type != TLV_CODE_CRC_32) || (eeprom_crc->length != 4)) {
        return false;
    }

    // Calculate the checksum
    calc_crc = crc32(0xffffffffL, (const unsigned char *)eeprom, sizeof(tlvinfo_header_t) +
             be16_to_cpu(eeprom_hdr->totallen) - 4);
    stored_crc = ((eeprom_crc->value[0] << 24) | (eeprom_crc->value[1] << 16) |
          (eeprom_crc->value[2] <<  8) | eeprom_crc->value[3]);

    return (calc_crc == stored_crc);
}

/*
 *  tlvinfo_find_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM.
 *  An offset from the beginning of the EEPROM is returned in the
 *  eeprom_index parameter if the TLV is found.
 */
static bool tlvinfo_find_tlv(u_int8_t *eeprom, u_int8_t tcode, int *eeprom_index)
{
    tlvinfo_header_t *eeprom_hdr;
    tlvinfo_tlv_t    *eeprom_tlv;
    int eeprom_end;

    eeprom_hdr = (tlvinfo_header_t *) eeprom;

    // Search through the TLVs, looking for the first one which matches the
    // supplied type code.
    *eeprom_index = sizeof(tlvinfo_header_t);
    eeprom_end = sizeof(tlvinfo_header_t) + be16_to_cpu(eeprom_hdr->totallen);
    while (*eeprom_index < eeprom_end) {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[*eeprom_index];
        if (!is_valid_tlv(eeprom_tlv)) {
            return false;
        }

        if (eeprom_tlv->type == tcode) {
            return true;
        }

        *eeprom_index += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }

    return false;
}

/*
 *  tlvinfo_decode_tlv
 *
 *  This function finds the TLV with the supplied code in the EERPOM
 *  and decodes the value into the buffer provided.
 */
static bool tlvinfo_decode_tlv(u_int8_t *eeprom, u_int8_t tcode, tlv_decode_value_t *decode_value)
{
    int eeprom_index;
    tlvinfo_tlv_t *eeprom_tlv;

    // Find the TLV and then decode it
    if (tlvinfo_find_tlv(eeprom, tcode, &eeprom_index)) {
        eeprom_tlv = (tlvinfo_tlv_t *) &eeprom[eeprom_index];
        decode_tlv_value(eeprom_tlv, decode_value);
        return true;
    }

    return false;
}

/*
 *  parse_tlv_eeprom
 *
 *  parse the EEPROM into memory, if it hasn't already been read.
 */
int parse_tlv_eeprom(u_int8_t *eeprom, u_int32_t size)
{
    unsigned int i;
    bool ret;
    tlvinfo_header_t *eeprom_hdr;
    //tlv_info_vec_t tlv_info;
    tlv_decode_value_t decode_value;
    int j;

    eeprom_hdr = (tlvinfo_header_t *) eeprom;
    eeprom_size = size; /* eeprom real size */

    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
        DBG_ERROR("Failed to check tlv header.\n");
        return -1;
    }

    if (!is_checksum_valid(eeprom)) {
        DBG_ERROR("Failed to check tlv crc.\n");
        return -1;
    }

    for (i = 0; i < TLV_CODE_NUM; i++) {
        mem_clear((void *)&decode_value, sizeof(tlv_decode_value_t));
        ret = tlvinfo_decode_tlv(eeprom, tlv_code_list[i].m_code, &decode_value);
        if (!ret) {
            DBG_ERROR("No found type: %s\n", tlv_code_list[i].m_name);
            continue;
        }

        DBG_DEBUG("i: %d,Found type: %s tlv[%d]:%s\n", i, tlv_code_list[i].m_name, tlv_code_list[i].m_code,
            decode_value.value);
        for (j = 0; j < decode_value.length; j++) {
            if ((j % 16) == 0) {
                DBG_DEBUG("\n");
            }
            DBG_DEBUG("%02x ", decode_value.value[j]);
        }
        DBG_DEBUG("\n\n");
    }
    return 0;
}
static int dfd_parse_tlv_eeprom(u_int8_t *eeprom, u_int32_t size, u_int8_t main_type, tlv_decode_value_t *decode_value)
{
    bool ret;
    tlvinfo_header_t *eeprom_hdr;
    //tlv_info_vec_t tlv_info;
    int j;

    eeprom_hdr = (tlvinfo_header_t *) eeprom;
    eeprom_size = size; /* eeprom real size */

    if (!is_valid_tlvinfo_header(eeprom_hdr)) {
        DBG_ERROR("Failed to check tlv header.\n");
        return -1;
    }

    if (!is_checksum_valid(eeprom)) {
        DBG_ERROR("Failed to check tlv crc.\n");
        return -1;
    }

    ret = tlvinfo_decode_tlv(eeprom, main_type, decode_value);
    if (!ret) {
        DBG_ERROR("No found type: %d\n", main_type);
        return -1;
    }

    DBG_DEBUG("Found type: %d, value: %s\n", main_type,decode_value->value);
    for (j = 0; j < decode_value->length; j++) {
        if ((j % 16) == 0) {
            DBG_DEBUG("\n");
        }
        DBG_DEBUG("%02x ", decode_value->value[j]);
    }
    DBG_DEBUG("\n\n");

    return 0;
}

static int tlvinfo_find_wb_ext_tlv(tlv_decode_value_t *ext_tlv_value, u_int8_t ext_type,
    u_int8_t *buf, u_int8_t *buf_len)
{
    tlvinfo_tlv_t    *eeprom_tlv;
    int eeprom_end, eeprom_index;

    // Search through the TLVs, looking for the first one which matches the
    // supplied type code.
    DBG_DEBUG("ext_tlv_value->length: %d.\n", ext_tlv_value->length);
    for (eeprom_index = 0; eeprom_index < ext_tlv_value->length; eeprom_index++) {
        if ((eeprom_index % 16) == 0) {
            DBG_DEBUG("\n");
        }
        DBG_DEBUG("%02x ", ext_tlv_value->value[eeprom_index]);
    }

    DBG_DEBUG("\n");

    eeprom_index = 0;
    eeprom_end = ext_tlv_value->length;
    while (eeprom_index < eeprom_end) {
        eeprom_tlv = (tlvinfo_tlv_t *) &(ext_tlv_value->value[eeprom_index]);
        if (!is_valid_tlv(eeprom_tlv)) {
            DBG_ERROR("tlv is not valid, eeprom_tlv->type 0x%x.\n", eeprom_tlv->type);
            return -1;
        }

        DBG_DEBUG("eeprom_tlv->length %d.\n", eeprom_tlv->length);
        if (eeprom_tlv->type == ext_type) {
            if (*buf_len >= eeprom_tlv->length) {
                memcpy(buf, eeprom_tlv->value, eeprom_tlv->length);
                DBG_DEBUG("eeprom_tlv->length %d.\n", eeprom_tlv->length);
                *buf_len = eeprom_tlv->length;
                return 0;
            }
            DBG_ERROR("buf_len %d small than info_len %d.\n", *buf_len, eeprom_tlv->length);
            return -1;
        }

        eeprom_index += sizeof(tlvinfo_tlv_t) + eeprom_tlv->length;
    }

    DBG_ERROR("ext_type %d: tlv is not found.\n", ext_type);
    return -1;
}

int dfd_tlvinfo_get_e2prom_info(u_int8_t *eeprom, u_int32_t size, dfd_tlv_type_t *tlv_type, u_int8_t* buf, u_int8_t *buf_len)
{
    tlv_decode_value_t decode_value;
    int ret;

    if (eeprom == NULL || tlv_type == NULL || buf == NULL) {
        DBG_ERROR("Input para invalid.\n");
        return -1;
    }

    mem_clear((void *)&decode_value, sizeof(tlv_decode_value_t));
    ret = dfd_parse_tlv_eeprom(eeprom, size, tlv_type->main_type, &decode_value);
    if (ret) {
        DBG_ERROR("dfd_parse_tlv_eeprom failed ret %d.\n", ret);
        return ret;
    }

    if (tlv_type->main_type != TLV_CODE_VENDOR_EXT) {
        if (*buf_len >= decode_value.length) {
            memcpy(buf, decode_value.value, decode_value.length);
            *buf_len = decode_value.length;
            return 0;
        }
        DBG_ERROR("buf_len %d small than info_len %d.\n", *buf_len, decode_value.length);
        return -1;
    }
    DBG_DEBUG("info_len %d.\n", decode_value.length);

    return tlvinfo_find_wb_ext_tlv(&decode_value, tlv_type->ext_type, buf, buf_len);
}
