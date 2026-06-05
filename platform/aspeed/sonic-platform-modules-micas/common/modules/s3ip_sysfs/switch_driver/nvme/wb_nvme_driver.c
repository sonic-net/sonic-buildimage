/*
 * Copyright(C) 2001-2025 whitebox Network. All rights reserved.
 */
/*
 * wb_nvme_driver.c
 * Original Author: [Your Name] [Date]
 *
 * NVME (GPU Computing Unit) related properties read and write function
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *   *  v1.0                                    2025-10-24        Initial version
 */

#include <linux/module.h>
#include <linux/slab.h>

#include "wb_module.h"
#include "dfd_cfg.h"
#include "dfd_cfg_info.h"
#include "dfd_cfg_adapter.h"
#include "wb_nvme_driver.h"

int g_dfd_nvme_dbg_level = 0x0;
module_param(g_dfd_nvme_dbg_level, int, S_IRUGO | S_IWUSR);

static nvme_device_type_t nvme_id_map_table[NVME_MAX_NUMBER] = {
    [0 ... NVME_MAX_NUMBER - 1] = { -1 }
};

static nvme_func_attr_t intel_d7_p5520_p5620_nvme_basic_attr_table[DFD_NVME_MAX_E] = {
    [DFD_NVME_STATUS_E] = {"Device Status", 0, 7, 1, 1, STORAGE_OUTPUT_HEXADECIMAL, STORAGE_ENDIAN_BE},
    [DFD_NVME_HEALTH_STATUS_E] = {"Device health status", 0, 7, 2, 1, STORAGE_OUTPUT_HEXADECIMAL, STORAGE_ENDIAN_BE},
    [DFD_NVME_PDLU_E] = {"Device PDLU", 0, 7, 4, 1, STORAGE_OUTPUT_HEXADECIMAL, STORAGE_ENDIAN_BE},
    [DFD_NVME_PCIE_VENDOR_ID_E] = {"PCIe Vendor ID", 8, 24, 1, 2, STORAGE_OUTPUT_HEXADECIMAL , STORAGE_ENDIAN_BE},
    [DFD_NVME_SERIAL_NUMBER_E] = {"Serial Number", 8, 24, 3, 20, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_FIRMWARE_VERSION_E] = {"Firmware Version", 32, 32, 7, 8, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_BOOTLOADER_VERSION_E] = {"Bootloader Version", 32, 32, 15, 8, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_PCIE_DEVICE_ID_E] = {"PCIe Device ID", 64, 32, 2, 2, STORAGE_OUTPUT_HEXADECIMAL , STORAGE_ENDIAN_BE},
    [DFD_NVME_PCIE_SUB_VENDOR_ID_E] = {"PCIe Sub Vendor ID", 64, 32, 4, 2, STORAGE_OUTPUT_HEXADECIMAL , STORAGE_ENDIAN_BE},
    [DFD_NVME_PCIE_SUB_DEVICE_ID_E] = {"PCIe Sub Device ID", 64, 32, 6, 2, STORAGE_OUTPUT_HEXADECIMAL , STORAGE_ENDIAN_BE},
    [DFD_NVME_MFR_NAME_E] = {"Manufacturer Name", -1, -1, 0xc, 8, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_PART_NUMBER_E] = {"Part Number", -1, -1, 0x1b, 24, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_PSN_E] = {"Product Serial Number", -1, -1, 0x36, 20, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_PRODUCT_NAME_E] = {"Product Name", -1, -1, 0x15, 5, STORAGE_OUTPUT_ASCII, STORAGE_ENDIAN_BE},
    [DFD_NVME_CAPACITY_E] = {"NVME Capacity", -1, -1, 0x68, 13, STORAGE_OUTPUT_INT, STORAGE_ENDIAN_LE},
    [DFD_NVME_INTERFACE_TYPE_E] = {"Interface Type", -1, -1, -1, 1, -1, -1},
    [DFD_NVME_MEDIA_TYPE_E] = {"Media Type", -1, -1, -1, 1, -1, -1},
    [DFD_NVME_TEMP_NUM_E] = {"Temp Number", -1, -1, -1, 1, -1, -1},
    [DFD_NVME_POWER_NUM_E] = {"Power Number", -1, -1, -1, 1, -1, -1},
};

static ssize_t dfd_nvme_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_oob_command_basic_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_vpd_i2c_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_interface_type(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_media_type(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_id_detect(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_pcie_vendor_id(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_get_pcie_device_id(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_oob_command_temp_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_oob_command_power_data(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_get_monitor_flag(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count);
static ssize_t dfd_nvme_get_temp_num(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);
static ssize_t dfd_nvme_get_power_num(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count);

dfd_sysfs_func_map_t nvme_basic_func_table[DFD_NVME_MAX_E] = {
    [DFD_NVME_ALIAS_E] = {dfd_nvme_get_alias, NULL},
    [DFD_NVME_STATUS_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_HEALTH_STATUS_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_PDLU_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_PCIE_VENDOR_ID_E] = {dfd_get_pcie_vendor_id, NULL},
    [DFD_NVME_SERIAL_NUMBER_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_FIRMWARE_VERSION_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_BOOTLOADER_VERSION_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_PCIE_DEVICE_ID_E] = {dfd_get_pcie_device_id, NULL},
    [DFD_NVME_PCIE_SUB_VENDOR_ID_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_PCIE_SUB_DEVICE_ID_E] = {dfd_nvme_get_oob_command_basic_data, NULL},
    [DFD_NVME_MFR_NAME_E] = {dfd_nvme_get_vpd_i2c_data, NULL},
    [DFD_NVME_PART_NUMBER_E] = {dfd_nvme_get_vpd_i2c_data, NULL},
    [DFD_NVME_PSN_E] = {dfd_nvme_get_vpd_i2c_data, NULL},
    [DFD_NVME_PRODUCT_NAME_E] = {dfd_nvme_get_vpd_i2c_data, NULL},
    [DFD_NVME_CAPACITY_E] = {dfd_nvme_get_vpd_i2c_data, NULL},
    [DFD_NVME_INTERFACE_TYPE_E] = {dfd_nvme_get_interface_type, NULL},
    [DFD_NVME_MEDIA_TYPE_E] = {dfd_nvme_get_media_type, NULL},
    [DFD_NVME_ID_DETECT_E] = {dfd_nvme_get_id_detect, NULL},
    [DFD_NVME_MONITOR_FLAG_E] = {dfd_get_monitor_flag, NULL},
    [DFD_NVME_TEMP_NUM_E] = {dfd_nvme_get_temp_num, NULL},
    [DFD_NVME_POWER_NUM_E] = {dfd_nvme_get_power_num, NULL},
};

static nvme_func_attr_t intel_d7_p5520_p5620_nvme_temp_attr_table[DFD_NVME_TEMP_TYPE_MAX] = {
    [DFD_NVME_TEMP_E] = {"TEMP", 0, 7, 0x3, 1, STORAGE_OUTPUT_INT, STORAGE_ENDIAN_BE, 1000},
    [DFD_NVME_TEMP_MAX_E] = {"CCTEMP", 32, 32, 0x4, 1, STORAGE_OUTPUT_INT, STORAGE_ENDIAN_BE, 1000},
};

dfd_sysfs_func_map_t nvme_temp_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_HIGH_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_LOW_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_HIGH_E] = {dfd_nvme_get_oob_command_temp_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_NOTICE_LOW_E] = {dfd_nvme_get_oob_command_temp_data, NULL},

};

static nvme_func_attr_t intel_d7_p5520_p5620_nvme_power_attr_table[DFD_NVME_POWER_TYPE_MAX] = {
    [DFD_NVME_POWER_E] = {"PWR_MEA", 32, 32, 0x2, 1, STORAGE_OUTPUT_INT, STORAGE_ENDIAN_BE, 1000000},
};

dfd_sysfs_func_map_t nvme_power_func_table[DFD_BMC_MANAGED_SENSOR_TYPE_MAX_E] = {
    [DFD_BMC_MANAGED_SENSOR_INPUT_E] = {dfd_nvme_get_oob_command_power_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_ALIAS_E] = {dfd_nvme_get_oob_command_power_data , NULL},
    [DFD_BMC_MANAGED_SENSOR_MAX_E] = {dfd_nvme_get_oob_command_power_data, NULL},
    [DFD_BMC_MANAGED_SENSOR_MIN_E] = {dfd_nvme_get_oob_command_power_data, NULL},
};

nvme_list_t nvme_list[WB_MIAN_NVME_TYPE_MAX] = {
    {
        "intel_d7_p5520_p5620",                         /* name */
        WB_MAIN_NVME_DEV_INTEL_D7_P5520_P5620,          /* nvme_main_id */
        {"vendor_id", 0x8, 24, 1, 2, STORAGE_OUTPUT_HEXADECIMAL },                   /* vendor_attr */
        {"dev_id", 64, 32, 2, 2, STORAGE_OUTPUT_HEXADECIMAL },                     /* dev_attr */
        0x025e,                                         /* vendor_id */
        0x0B60,                                         /* device_id */
        intel_d7_p5520_p5620_nvme_basic_attr_table,     /* basic_attr_table */
        intel_d7_p5520_p5620_nvme_temp_attr_table,      /* temp_attr_table */
        2,                                              /* temp_number */
        intel_d7_p5520_p5620_nvme_power_attr_table,     /* power_attr_table */
        1,                                              /* power_number */
        STORAGE_INTERFACE_NVME,                         /* interface_type */
        STORAGE_MEDIA_SSD,                              /* storage_type */
    }
};

static int find_nvme_by_ids(unsigned int slot, int *nvme_type, int *bus_num, uint32_t *oob_addr, uint32_t *vpd_addr);

static int dfd_nvme_get_pcie_device_id_via_index(int index, int bus_num,
    int32_t addr, uint8_t val[])
{
    int ret;

    if (index >= WB_MIAN_NVME_TYPE_MAX || index < 0) {
        DBG_DEBUG(DBG_ERROR, "Invalid NVME index %d\n", index);
        ret = -DFD_RV_DEV_FAIL;
        goto out;
    }

    DBG_DEBUG(DBG_VERBOSE, "NVME index %d, bus_num: %d, addr: %d, command_code: %d, block_length: %d, offset: %d, size: %d\n",
        index, bus_num, addr, nvme_list[index].dev_attr.command_code, nvme_list[index].dev_attr.block_length,
        nvme_list[index].dev_attr.offset, nvme_list[index].dev_attr.size);
    ret = dfd_ko_i2c_dev_smbus_block_offset_read(
        bus_num, addr, nvme_list[index].dev_attr.command_code, nvme_list[index].dev_attr.block_length,
        nvme_list[index].dev_attr.offset, val, nvme_list[index].dev_attr.size);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "Failed to read NVME device id via index %d, bus_num: %d, addr: %d\n",
            index, bus_num, addr);
        ret = -DFD_RV_DEV_FAIL;
    }

out:
    return ret;
}

static int dfd_nvme_get_pcie_vendor_id_via_index(int index, int bus_num, int32_t addr, uint8_t val[])
{
    int ret;

    if (index >= WB_MIAN_NVME_TYPE_MAX || index < 0) {
        DBG_DEBUG(DBG_ERROR, "Invalid NVME index %d\n", index);
        ret = -DFD_RV_DEV_FAIL;
        goto out;
    }

    DBG_DEBUG(DBG_VERBOSE, "NVME index %d, bus_num: %d, addr: %d, command_code: %d, block_length: %d, offset: %d, size: %d\n",
        index, bus_num, addr, nvme_list[index].vendor_attr.command_code, nvme_list[index].vendor_attr.block_length,
        nvme_list[index].vendor_attr.offset, nvme_list[index].vendor_attr.size);
    ret = dfd_ko_i2c_dev_smbus_block_offset_read(
        bus_num, addr, nvme_list[index].vendor_attr.command_code, nvme_list[index].vendor_attr.block_length,
        nvme_list[index].vendor_attr.offset, val, nvme_list[index].vendor_attr.size);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "Failed to read NVME vendor id via index %d, bus_num: %d, addr: %d\n",
            index, bus_num, addr);
        ret = -DFD_RV_DEV_FAIL;
    }

out:
    return ret;
}

static void reverse_bytes_inplace(char *source, int source_size) {
    int i, j;
    uint8_t t;

    if (!source || source_size <= 1)
        return;

    for (i = 0, j = source_size - 1; i < j; ++i, --j) {
        t = source[i];
        source[i] = source[j];
        source[j] = t;
    }
}

static int convert_output_data(char *buf, unsigned int count, char *source, nvme_func_attr_t *tmp_attr)
{
    int i, bit_index;
    int total_bits = tmp_attr->size * 8;
    int hex_needed = 2 + (tmp_attr->size) * 2;
    int max_bits = (int)(sizeof(long long) * 8);
    uint8_t tmp_c;
    long long target_int_value;
    static const char HEX[] = "0123456789ABCDEF";

    bit_index = 0;
    target_int_value = 0;

    if (total_bits > max_bits)
        total_bits = max_bits;

    if (tmp_attr->endian == STORAGE_ENDIAN_LE)
        reverse_bytes_inplace(source, tmp_attr->size);

    switch(tmp_attr->output_type) {
        case STORAGE_OUTPUT_ASCII:
            snprintf(buf, count, "%s\n", source);
            if (buf[0] == '\0') {
                DBG_NVME_DEBUG(DBG_VERBOSE, "target is empty string!\n");
                snprintf(buf, count, "%s\n", SWITCH_DEV_NO_SUPPORT);
            }

            break;
        case STORAGE_OUTPUT_INT:
            i = 0;
            while (i < tmp_attr->size && (unsigned char)source[i] == 0)
                ++i;

            for (; i < tmp_attr->size; ++i) {
                target_int_value = (target_int_value << 8) | (uint8_t)source[i];
                DBG_NVME_DEBUG(DBG_VERBOSE, "i: %d, source[i]: 0x%x target_int_value = %lld\n", i, source[i], target_int_value);
            }

            if (tmp_attr->conv_factor > 0)
                target_int_value = target_int_value * tmp_attr->conv_factor;

            DBG_NVME_DEBUG(DBG_VERBOSE, "target_int_value = %lld\n", target_int_value);
            snprintf(buf, count, "%lld\n", target_int_value);
            break;
        case STORAGE_OUTPUT_HEXADECIMAL :
            /* hex + \n + \0 */
            if (count < hex_needed + 1 + 1)
                return -DFD_RV_INVALID_VALUE;
            buf[0] = '0';
            buf[1] = 'x';
            for (i = 0; i < tmp_attr->size; ++i) {
                tmp_c = (unsigned char)source[i];
                buf[2 + 2*i]     = HEX[(tmp_c >> 4) & 0xF];
                buf[2 + 2*i + 1] = HEX[tmp_c & 0xF];
                DBG_NVME_DEBUG(DBG_VERBOSE, "STORAGE_OUTPUT_HEXADECIMAL  i: %d, char: 0x%x, buf[2 + 2*i]: %c, buf[2 + 2*i + 1]: %c !\r\n",
                    i, tmp_c, buf[2 + 2*i], buf[2 + 2*i + 1]);
            }
            buf[hex_needed] = '\n';
            buf[hex_needed + 1] = '\0';
            DBG_NVME_DEBUG(DBG_VERBOSE, "Buf is %s !\r\n", buf);
            break;
        default:
            return -DFD_RV_INVALID_VALUE;
            break;
    }

    return DFD_RV_OK;
}


/* NVMe get alias */
static ssize_t dfd_nvme_get_alias(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    info_ctrl_t *info_ctrl;
    uint64_t key;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_alias has been called, main_dev_id: %d, sub_dev_id: %d, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[tmp_nvme_dev->nvme_type].nvme_main_id,
            DFD_GET_NVME_KEY2(main_dev_id, DFD_NVME_ALIAS_E));
    DBG_NVME_DEBUG(DBG_VERBOSE, "alias key: 0x%08llx\n", key);

    info_ctrl = dfd_ko_cfg_get_item(key);
    if (info_ctrl == NULL) {
        DBG_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return (size_t)snprintf(buf, count, "%s\n", info_ctrl->str_cons);
}

/* NVMe OOB Command Response for Basic data */
static ssize_t dfd_nvme_get_oob_command_basic_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    uint64_t key;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (sub_dev_id < 0 || sub_dev_id >= DFD_NVME_MAX_E) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME sub_dev_id %d\n", sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_status has been called, main_dev_id: %d, sub_dev_id: %d, count: %zu.\n",
        main_dev_id, sub_dev_id, count);

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);

    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_oob_addr;

    if (sub_dev_id == DFD_NVME_ALIAS_E) {
        return (ssize_t)snprintf(buf, count, "%s\n",
            nvme_list[nvme_type].basic_attr_table[sub_dev_id].name);
    }

    ret = dfd_ko_i2c_dev_smbus_block_offset_read(
        bus_num, addr, nvme_list[nvme_type].basic_attr_table[sub_dev_id].command_code, nvme_list[nvme_type].basic_attr_table[sub_dev_id].block_length,
        nvme_list[nvme_type].basic_attr_table[sub_dev_id].offset, val, nvme_list[nvme_type].basic_attr_table[sub_dev_id].size);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_ko_i2c_dev_smbus_block_offset_read, bus: %d, addr: 0x%x, command_code: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, addr, nvme_list[nvme_type].basic_attr_table[sub_dev_id].command_code, nvme_list[nvme_type].basic_attr_table[sub_dev_id].offset, ret);
        return ret;
    }

    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].basic_attr_table[sub_dev_id]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }

    return strlen(buf);
}

/* NVMe OOB Command Response for Temp data*/
static ssize_t dfd_nvme_get_oob_command_temp_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    int tmp_dev_id, tmp_temp_id;
    uint64_t key;
    nvme_device_type_t *tmp_nvme_dev;

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_oob_command_temp_data has been called, main_dev_id: 0x%x, sub_dev_id: %d.\n",
        main_dev_id, sub_dev_id);

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    tmp_dev_id = main_dev_id >> NVME_SENSOR_INDEX_OFFSET;
    tmp_temp_id = main_dev_id & 0xFFFF;

    if ((tmp_dev_id < 1 || tmp_dev_id > NVME_MAX_NUMBER)
        || (tmp_temp_id < 1 || tmp_temp_id > DFD_NVME_TEMP_TYPE_MAX)) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[tmp_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE_TEMP,
        DFD_GET_NVME_SENSOR_KEY1(nvme_list[nvme_type].nvme_main_id, tmp_dev_id),
        DFD_GET_NVME_SENSOR_KEY2(sub_dev_id, tmp_temp_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    } else if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "key %llx, nvme_type %x main_dev_id %x sub_dev_id %x", key, nvme_type, main_dev_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);

    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_oob_addr;

    if (sub_dev_id == DFD_NVME_SENSOR_ALIAS_E) {
        DBG_NVME_DEBUG(DBG_ERROR, "nvme_type: %d, tmp_temp_id: %d, Alias is %s\n",
            nvme_type, tmp_temp_id, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].name);
        return (ssize_t)snprintf(buf, count, "%s\n",
            nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].name);
    }

    ret = dfd_ko_i2c_dev_smbus_block_offset_read(
        bus_num, addr, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].command_code, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].block_length,
        nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].offset, val, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].size);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_nvme_get_oob_command_temp_data, bus: %d, addr: 0x%x, command_code: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, addr, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].command_code, nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1].offset, ret);
        return ret;
    }

    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].temp_attr_table[tmp_temp_id - 1]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }
    return strlen(buf);
}

/* NVMe OOB Command Response for Power data*/
static ssize_t dfd_nvme_get_oob_command_power_data(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    int tmp_dev_id, tmp_power_id;
    uint64_t key;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_oob_command_power_data has been called, main_dev_id: 0x%x, sub_dev_id: %d.\n",
        main_dev_id, sub_dev_id);

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    tmp_dev_id = main_dev_id >> NVME_SENSOR_INDEX_OFFSET;
    tmp_power_id = main_dev_id & 0xffff;

    if ((tmp_dev_id < 1 || tmp_dev_id > NVME_MAX_NUMBER)
        || (tmp_power_id < 1 || tmp_power_id > DFD_NVME_POWER_TYPE_MAX)) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME id %d, %d\n", tmp_dev_id, tmp_power_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[tmp_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE_POWER,
        DFD_GET_NVME_SENSOR_KEY1(nvme_list[nvme_type].nvme_main_id, tmp_dev_id),
        DFD_GET_NVME_SENSOR_KEY2(sub_dev_id, tmp_power_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    } else if (IS_TARGET_SENSOR_STATIC(sub_dev_id)) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "key %llx, nvme_type %x main_dev_id %x sub_dev_id %x", key, nvme_type, main_dev_id, sub_dev_id);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_oob_addr;

    if (sub_dev_id == DFD_NVME_SENSOR_ALIAS_E) {
        return (ssize_t)snprintf(buf, count, "%s\n",
            nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].name);
    }

    ret = dfd_ko_i2c_dev_smbus_block_offset_read(
        bus_num, addr, nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].command_code, nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].block_length,
        nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].offset, val, nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].size);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_nvme_get_oob_command_power_data, bus: %d, addr: 0x%x, command_code: 0x%x, offset: 0x%x, ret: %d\n",
            bus_num, addr, nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].command_code, nvme_list[nvme_type].power_attr_table[tmp_power_id - 1].offset, ret);
        return ret;
    }

    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].power_attr_table[tmp_power_id - 1]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }
    return strlen(buf);
}

/* PCIe Vendor ID */
static ssize_t dfd_get_pcie_vendor_id(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    nvme_device_type_t *tmp_nvme_dev;
    uint64_t key;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (sub_dev_id < 0 || sub_dev_id >= DFD_NVME_MAX_E) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME sub_dev_id %d\n", sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_status has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);


    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_oob_addr;

    ret = dfd_nvme_get_pcie_vendor_id_via_index(nvme_type, bus_num, addr, val);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_get_pcie_vendor_id, nvme_type: %d bus: %d, addr: 0x%x, ret: %d\n",
            nvme_type, bus_num, addr, ret);
        return ret;
    }
    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].basic_attr_table[sub_dev_id]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }
    return strlen(buf);
}

/* PCIe Device ID */
static ssize_t dfd_get_pcie_device_id(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    nvme_device_type_t *tmp_nvme_dev;
    uint64_t key;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (sub_dev_id < 0 || sub_dev_id >= DFD_NVME_MAX_E) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME sub_dev_id %d\n", sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_status has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);

    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_oob_addr;

    ret = dfd_nvme_get_pcie_device_id_via_index(nvme_type, bus_num, addr, val);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_nvme_get_pcie_device_id_via_index, nvme_type: %d bus: %d, addr: 0x%x, ret: %d\n",
            nvme_type, bus_num, addr, ret);
        return ret;
    }

    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].basic_attr_table[sub_dev_id]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }
    return strlen(buf);
}

/* NVMe VPD Data read via I2C */
static ssize_t dfd_nvme_get_vpd_i2c_data(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint32_t addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;
    nvme_device_type_t *tmp_nvme_dev;
    uint64_t key;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    if (sub_dev_id < 0 || sub_dev_id >= DFD_NVME_MAX_E) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME sub_dev_id %d\n", sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_status has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);

    bus_num = tmp_nvme_dev->i2c_bus_num;
    addr = tmp_nvme_dev->i2c_vpd_addr;

    if (sub_dev_id == DFD_NVME_SENSOR_ALIAS_E) {
        return (ssize_t)snprintf(buf, count, "%s\n",
            nvme_list[nvme_type].basic_attr_table[sub_dev_id].name);
    }

    ret = dfd_ko_i2c_read(
        bus_num, addr, nvme_list[nvme_type].basic_attr_table[sub_dev_id].offset, val,
        nvme_list[nvme_type].basic_attr_table[sub_dev_id].size, NULL
    );

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute dfd_nvme_get_vpd_i2c_data, bus: %d, addr: 0x%x, offset: 0x%x, size: %d, ret: %d\n",
            bus_num, addr, nvme_list[nvme_type].basic_attr_table[sub_dev_id].offset, nvme_list[nvme_type].basic_attr_table[sub_dev_id].size, ret);
        return ret;
    }

    ret = convert_output_data(buf, count, val, &nvme_list[nvme_type].basic_attr_table[sub_dev_id]);

    if (ret < 0) {
        DBG_DEBUG(DBG_ERROR, "failed to execute convert_output_data, ret: %d\n", ret);
        return ret;
    }
    return strlen(buf);
}

/* NVMe interface Type */
static ssize_t dfd_nvme_get_interface_type(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, nvme_type;
    nvme_device_type_t *tmp_nvme_dev;
    uint64_t key;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_interface_type has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);


    val[0] = nvme_list[nvme_type].interface_type;

    snprintf(buf, count, "0x%x\n", val[0]);
    return strlen(buf);
}

/* NVMe Media Type */
static ssize_t dfd_nvme_get_media_type(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int nvme_type, ret;
    uint64_t key;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_media_type has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;
    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[nvme_type].nvme_main_id, DFD_GET_NVME_KEY2(main_dev_id, sub_dev_id));
    ret = dfd_get_value_from_info(key, val, INFO_INT_MAX_LEN);
    if (ret >= 0) {
        DBG_NVME_DEBUG(DBG_VERBOSE, "%s", val);
        snprintf(buf, count, "%s", val);
        return strlen(buf);
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "cant get the cfg from nvme_type %d main_dev_id: %d, sub_dev_id: %d.\n",
        nvme_type, main_dev_id, sub_dev_id);

    val[0] = nvme_list[nvme_type].storage_type;

    snprintf(buf, count, "0x%x\n", val[0]);
    return strlen(buf);
}

/* NVMe get temp num */
static ssize_t dfd_nvme_get_temp_num(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int nvme_type;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_temp_num has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    val[0] = nvme_list[nvme_type].temp_number;

    snprintf(buf, count, "%d\n", val[0]);
    return strlen(buf);
}

/* NVMe get power num */
static ssize_t dfd_nvme_get_power_num(unsigned int main_dev_id, unsigned int sub_dev_id,  char *buf, size_t count)
{
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int nvme_type;
    nvme_device_type_t *tmp_nvme_dev;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, main_dev_id: %u, sub_dev_id: %u\n",
            main_dev_id, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_power_num has been called.\n");

    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    tmp_nvme_dev = &nvme_id_map_table[main_dev_id - 1];

    if (tmp_nvme_dev->nvme_type < 0 || tmp_nvme_dev->nvme_type >= WB_MIAN_NVME_TYPE_MAX) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME type %d\n", tmp_nvme_dev->nvme_type);
        return -DFD_RV_DEV_FAIL;
    }

    nvme_type = tmp_nvme_dev->nvme_type;

    val[0] = nvme_list[nvme_type].power_number;

    snprintf(buf, count, "%d\n", val[0]);
    return strlen(buf);
}

/* ID Detect */
static ssize_t dfd_nvme_get_id_detect(unsigned int slot, unsigned int sub_dev_id, char *buf, size_t count)
{
    uint32_t oob_addr, vpd_addr;
    uint8_t val[INFO_INT_MAX_LEN + 1];
    int ret, bus_num, nvme_type;

    if (buf == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "param error, buf is NULL, slot: %u, sub_dev_id: %u\n",
            slot, sub_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_nvme_get_media_type has been called.\n");


    mem_clear(buf, count);
    mem_clear(val, sizeof(val));

    ret = find_nvme_by_ids(slot, &nvme_type, &bus_num, &oob_addr, &vpd_addr);
    if (nvme_type < 0) {
        DBG_NVME_DEBUG(DBG_ERROR, "slot: %u, sub_dev_id %u config error.\n",
            slot, sub_dev_id);

        return nvme_type;
    }

    DBG_NVME_DEBUG(DBG_VERBOSE, "slot: %u, type %s has been detect.\n",
            slot, nvme_list[nvme_type].name);

    nvme_id_map_table[slot - 1].nvme_type = nvme_type;
    nvme_id_map_table[slot - 1].i2c_bus_num = bus_num;
    nvme_id_map_table[slot - 1].i2c_oob_addr = oob_addr;
    nvme_id_map_table[slot - 1].i2c_vpd_addr = vpd_addr;

    mem_clear(buf, count);
    snprintf(buf, count, "%s\n", nvme_list[nvme_type].name);

    return strlen(buf);
}

static ssize_t dfd_get_monitor_flag(unsigned int main_dev_id, unsigned int sub_dev_id, char *buf, size_t count)
{
    int nvme_type;
    int *m_num, bus_num;
    int rv, data, tmp_data, i;
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint32_t oob_addr, vpd_addr;

    DBG_NVME_DEBUG(DBG_VERBOSE, "dfd_get_monitor_flag: main_dev_id: %u, sub_dev_id: %u\n",
        main_dev_id, sub_dev_id);

    if (main_dev_id < 1 || main_dev_id > NVME_MAX_NUMBER) {
        DBG_NVME_DEBUG(DBG_ERROR, "Invalid NVME main_dev_id %d\n", main_dev_id);
        return -DFD_RV_INVALID_VALUE;
    }

    mem_clear(buf, count);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_MONITOR_FLAG_NUM, main_dev_id, 0);
    m_num = dfd_ko_cfg_get_item(key);
    if (m_num == NULL) {
        DBG_NVME_DEBUG(DBG_ERROR, "failed to get monitor flag number, main_dev_id: %u key : %llx\n", main_dev_id, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    tmp_data = 1;
    for (i = 1; i <= *m_num; i++) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_MONITOR_FLAG, main_dev_id, i);
        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "get info ctrl failed, key=0x%08llx\n", key);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", WB_SENSOR_MONITOR_YES);
        }

        rv = dfd_info_get_int(key, &data, NULL);
        if (rv < 0) {
            DBG_NVME_DEBUG(DBG_ERROR, "get monitor flag error, rv: %d\n", rv);
            return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
        }

        DBG_NVME_DEBUG(DBG_VERBOSE, "main_dev_id: %u, data = %d\n", main_dev_id, data);

        tmp_data &= data;
        if (tmp_data == 0) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "tmp_data == 0 data = %d, break\n", data);
            break;
        }
    }

    if (nvme_id_map_table[main_dev_id - 1].monitor_last_flag == -1) {
        nvme_id_map_table[main_dev_id - 1].monitor_last_flag = tmp_data;
        DBG_NVME_DEBUG(DBG_VERBOSE, "first monitor main_dev_id: %u, data = %d last_flag = %d\n",
            main_dev_id, data, nvme_id_map_table[main_dev_id - 1].monitor_last_flag);
        rv = find_nvme_by_ids(main_dev_id, &nvme_type, &bus_num, &oob_addr, &vpd_addr);
        if (rv < 0) {
            DBG_NVME_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error.\n",
                main_dev_id, sub_dev_id);
            goto monitor_unready;
        }

        goto monitor_ready;

    } else {
        if (tmp_data == 1 && nvme_id_map_table[main_dev_id - 1].monitor_last_flag == 0) {
            rv = find_nvme_by_ids(main_dev_id, &nvme_type, &bus_num, &oob_addr, &vpd_addr);
            if (rv < 0) {
                DBG_NVME_DEBUG(DBG_ERROR, "main_dev_id: %u, sub_dev_id %u config error.\n",
                    main_dev_id, sub_dev_id);
                goto monitor_unready;
            }

            goto monitor_ready;
        } else if (tmp_data == 0) {
            DBG_NVME_DEBUG(DBG_ERROR, "dpu is offline, we cant monitor it.\n");
            goto monitor_unready;
        }
    }

    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);

monitor_ready:
    nvme_id_map_table[main_dev_id - 1].nvme_type = nvme_type;
    DBG_NVME_DEBUG(DBG_VERBOSE, "slot(%d) nvme_type is %d.\n", main_dev_id, nvme_id_map_table[main_dev_id - 1].nvme_type);
    nvme_id_map_table[main_dev_id - 1].monitor_last_flag = tmp_data;
    nvme_id_map_table[main_dev_id - 1].i2c_bus_num = bus_num;
    nvme_id_map_table[main_dev_id - 1].i2c_oob_addr = oob_addr;
    nvme_id_map_table[main_dev_id - 1].i2c_vpd_addr = vpd_addr;
    DBG_NVME_DEBUG(DBG_VERBOSE, "slot(%d) i2c_bus_num is %d.\n", main_dev_id, nvme_id_map_table[main_dev_id - 1].i2c_bus_num);
    DBG_NVME_DEBUG(DBG_VERBOSE, "slot(%d) i2c_oob_addr is %d, i2c_vpd_addr is %d.\n",
        main_dev_id, nvme_id_map_table[main_dev_id - 1].i2c_oob_addr, nvme_id_map_table[main_dev_id - 1].i2c_vpd_addr);
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 1);

monitor_unready:
    nvme_id_map_table[main_dev_id - 1].monitor_last_flag = 0;
    return (ssize_t)snprintf(buf, PAGE_SIZE, "%d\n", 0);
}

static int find_nvme_by_ids(unsigned int slot, int *nvme_type, int *bus_num, uint32_t *oob_addr, uint32_t *vpd_addr) {
    info_ctrl_t *info_ctrl;
    uint64_t key;
    uint16_t value;
    int bus_num_tmp;
    uint8_t val[INFO_INT_MAX_LEN + 1] = {0};
    const char *pos;
    int rv, i;

    DBG_NVME_DEBUG(DBG_VERBOSE, "find_nvme_by_ids called.\n");

    for (i = 0; i < sizeof(nvme_list) / sizeof(nvme_list[0]); i++) {
        /* 1. fetch oob data*/
        key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[i].nvme_main_id,
                DFD_GET_NVME_KEY2(slot, DFD_NVME_OOB_DATA_BUS_NUMBER));

        /* Entry check */
        if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
            DBG_NVME_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
            continue;
        }

        info_ctrl = dfd_ko_cfg_get_item(key);
        if (info_ctrl == NULL) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n nvme_list[i].nvme_main_id: 0x%x DFD_GET_NVME_KEY2(slot, DFD_NVME_OOB_DATA_BUS_NUMBER): 0x%x",
                    key, nvme_list[i].nvme_main_id, DFD_GET_NVME_KEY2(slot, DFD_NVME_OOB_DATA_BUS_NUMBER));
            continue;
        }

        if (info_ctrl->mode != INFO_CTRL_MODE_CFG || info_ctrl->src != INFO_SRC_FILE) {
            continue;
        }

        pos = strstr(info_ctrl->fpath, "i2c-");
        if (pos == NULL) {
            continue;
        }

        rv = kstrtouint(pos + 4, 10, &bus_num_tmp);
        if (rv < 0) {
            continue;
        }

        DBG_NVME_DEBUG(DBG_VERBOSE, "find nvme bus num, bus num is %d, addr: 0x%x\n", bus_num_tmp, info_ctrl->addr);

        /* Read vendor ID */
        rv = dfd_nvme_get_pcie_vendor_id_via_index(i, bus_num_tmp, info_ctrl->addr, val);
        if (rv < 0) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "bus num %d slave addr 0x%x get vendor id(0x%x) failed.\n", bus_num_tmp, info_ctrl->addr, nvme_list[i].vendor_attr.command_code);
            continue;
        }

        value = (val[0] << 8) | val[1];

        DBG_NVME_DEBUG(DBG_VERBOSE, "Read vendor ID succeed 0x%x.\n", value);
        if (nvme_list[i].vendor_id != value) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "nvme_list[i].vendor_id(0x%x) != value(0x%x).\n", nvme_list[i].vendor_id, value);
            continue;
        }

        /* Read device ID */
        rv = dfd_nvme_get_pcie_device_id_via_index(i, bus_num_tmp, info_ctrl->addr, val);

        if (rv < 0) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "bus num %d slave addr 0x%x, failed to get device id(0x%x).\n", bus_num_tmp, info_ctrl->addr, nvme_list[i].dev_attr.command_code);
            continue;
        }

        value = (val[0] << 8) | val[1];
        DBG_NVME_DEBUG(DBG_VERBOSE, "Read device ID succeed 0x%x.\n", value);
        if (nvme_list[i].device_id  != value) {
            DBG_NVME_DEBUG(DBG_VERBOSE, "nvme_list[i].device_id(0x%x) != value(0x%x).\n", nvme_list[i].device_id, value);
            continue;
        }
        if (oob_addr != NULL) {
            *oob_addr = info_ctrl->addr;

            /* fetch vpd data */
            key = DFD_CFG_KEY(DFD_CFG_ITEM_NVME_DEVICE, nvme_list[i].nvme_main_id,
                    DFD_GET_NVME_KEY2(slot, DFD_NVME_VPD_DATA_BUS_NUMBER));
            if (!DFD_CFG_ITEM_IS_INFO_CTRL(DFD_CFG_ITEM_ID(key))) {
                DBG_NVME_DEBUG(DBG_ERROR, "input arguments error, key: 0x%08llx\n", key);
                continue;
            }

            info_ctrl = dfd_ko_cfg_get_item(key);
            if (info_ctrl == NULL || info_ctrl->mode != INFO_CTRL_MODE_CFG) {
                DBG_NVME_DEBUG(DBG_VERBOSE, "can't find dfd config, key: 0x%08llx\n nvme_list[i].nvme_main_id: 0x%x DFD_GET_NVME_KEY2(slot, DFD_NVME_VPD_DATA_BUS_NUMBER): 0x%x",
                        key, nvme_list[i].nvme_main_id, DFD_GET_NVME_KEY2(slot, DFD_NVME_VPD_DATA_BUS_NUMBER));
            } else {
                DBG_NVME_DEBUG(DBG_VERBOSE, "find dfd config, key: 0x%08llx\n", key);
                *vpd_addr = info_ctrl->addr;
            }

        }
        if (bus_num != NULL)
            *bus_num = bus_num_tmp;
        *nvme_type = i;
        return 0;
    }

    return -ENOMEM;
}
