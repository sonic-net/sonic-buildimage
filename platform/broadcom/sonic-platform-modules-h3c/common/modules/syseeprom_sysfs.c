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
/*公有文件引入*/
#include <linux/init.h>
#include <linux/module.h>
#include <asm/io.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include<linux/fs.h>
#include<linux/uaccess.h>
#include <asm/byteorder.h>
//#include <stdbool.h>
/*私有文件*/
#include "pub.h"
#include "bsp_base.h"
#include "i2c_dev_reg.h"
#include "syseeprom.h"

module_param_named(syseeprom_debug_level, bsp_module_debug_level[BSP_SYSEEPROM_MODULE], int, 0644);
MODULE_PARM_DESC(syseeprom_debug_level, "DEBUG 0x4, ERROR 0x2, INFO 0x1, ALL 0x7, DEBUG_OFF 0x0; Default value is ERROR");

static struct kobject *kobj_syseeprom = NULL;
enum EEPROM_ATTR
{
    PRODUCT_NAME,
    PART_NUMBER,
    SERIAL_NUMBER,
    MANUFACTURE_DATE,
    DEVICE_VERSION,
    BASE_MAC_ADDRESS,
    LABEL_REVISION,
    PLATFORM_NAME,
    ONIE_VERSION,
    NUM_MACS,
    MANUFACTURER,
    MANUFACTURE_COUNTRY,
    VENDOR_NAME,
    DIAG_VERSION,
    SERVICE_TAG,
    VENDOR_EXTENSION,
    CRC_32,
    EEPROM,
    BSP_VERSION
};

ssize_t bsp_syseeprom_sysfs_read(struct device *kobjs, struct device_attribute *da, char *buf)
{
    ssize_t len = 0;
    //char tlv_info_string[TLV_DECODE_VALUE_MAX_LEN]= {0};
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    switch (attr->index)
    {
        case PRODUCT_NAME:
            len = bsp_syseeprom_sysfs_read_model_name(buf);
            break;
        case PART_NUMBER:
            len = bsp_syseeprom_sysfs_read_part_number(buf);
            break;
        case MANUFACTURE_DATE:
            len = bsp_syseeprom_sysfs_read_manuf_date(buf);
            break;
        case DEVICE_VERSION:
            len = bsp_syseeprom_sysfs_read_device_version(buf);
            break;
        case BASE_MAC_ADDRESS:
            len = bsp_syseeprom_sysfs_read_mac_address(buf);
            break;
        case LABEL_REVISION:
            len = bsp_syseeprom_sysfs_read_label_revision(buf);
            break;
        case PLATFORM_NAME:
            len = bsp_syseeprom_sysfs_read_platform_name(buf);
            break;
        case ONIE_VERSION:
            len = bsp_syseeprom_sysfs_read_onie_version(buf);
            break;
        case MANUFACTURER:
            len = bsp_syseeprom_sysfs_read_manufacturer(buf);
            break;
        case MANUFACTURE_COUNTRY:
            len = bsp_syseeprom_sysfs_read_manuf_country(buf);
            break;
        case VENDOR_NAME:
            len = bsp_syseeprom_sysfs_read_vendor_name(buf);
            break;
        case DIAG_VERSION:
            len = bsp_syseeprom_sysfs_read_diag_version(buf);
            break;
        case SERVICE_TAG:
            len = bsp_syseeprom_sysfs_read_service_tag(buf);
            break;
        case VENDOR_EXTENSION:
            len = bsp_syseeprom_sysfs_read_vendor_ext(buf);
            break;
        case NUM_MACS:
            len = bsp_syseeprom_sysfs_read_mac_nums(buf);
            break;
        case SERIAL_NUMBER:
            len = bsp_syseeprom_sysfs_read_serial_number(buf);
            break;
        case CRC_32:
            len = bsp_syseeprom_sysfs_read_crc32(buf);
            break;
        case EEPROM:
            len = bsp_syseeprom_sysfs_read_eeprom(buf);
            break;
        case BSP_VERSION:
            len = sprintf(buf, "%s\n", H3C_BSP_VERSION);
            break;
        default:
            len = -ENOSYS;
            break;
    }
    return len;
}

ssize_t bsp_syseeprom_sysfs_memory_read(struct kobject *kobjs, struct kobj_attribute *attr, char *buf)
{
    unsigned char temp1[1024] = {0};
    ssize_t len = 0;
    board_static_data *bdata = bsp_get_board_data();
    int ret = ERROR_SUCCESS;

    ret = bsp_i2c_24LC128_eeprom_read_bytes(bdata->i2c_addr_eeprom, 0, bdata->eeprom_used_size, temp1, I2C_DEV_EEPROM);
    if (ERROR_SUCCESS != ret)
    {
        DBG_ECHO(DEBUG_ERR, "bsp_i2c_24LC128_eeprom_read_bytes failed\n");
        len = ret;
    }
    else
    {
        len += bsp_print_memory(temp1, bdata->eeprom_used_size, buf + len, 4096 - len, 0x0, 4);
    }

    return len;
}

ssize_t  bsp_syseeprom_sysfs_memory_write(struct kobject *kobjs, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret = bsp_syseeprom_write_buf((u8 *)buf, count);

    if (ERROR_SUCCESS != ret)
    {
        DBG_ECHO(DEBUG_ERR, "eeprom sysfs write failed, ret=%d\n", ret);
        count = ret;
    }
    return count;
}

SENSOR_DEVICE_ATTR(product_name, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, PRODUCT_NAME);
SENSOR_DEVICE_ATTR(part_number, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, PART_NUMBER);
SENSOR_DEVICE_ATTR(serial_number, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, SERIAL_NUMBER);
SENSOR_DEVICE_ATTR(base_MAC_address, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, BASE_MAC_ADDRESS);
SENSOR_DEVICE_ATTR(manufacture_date, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, MANUFACTURE_DATE);
SENSOR_DEVICE_ATTR(device_version, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, DEVICE_VERSION);
SENSOR_DEVICE_ATTR(label_revision, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, LABEL_REVISION);
SENSOR_DEVICE_ATTR(platform_name, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, PLATFORM_NAME);
SENSOR_DEVICE_ATTR(ONIE_version, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, ONIE_VERSION);
SENSOR_DEVICE_ATTR(num_MACs, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, NUM_MACS);
SENSOR_DEVICE_ATTR(manufacturer, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, MANUFACTURER);
SENSOR_DEVICE_ATTR(manufacture_country, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, MANUFACTURE_COUNTRY);
SENSOR_DEVICE_ATTR(vendor_name, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, VENDOR_NAME);
SENSOR_DEVICE_ATTR(diag_version, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, DIAG_VERSION);
SENSOR_DEVICE_ATTR(service_tag, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, SERVICE_TAG);
SENSOR_DEVICE_ATTR(vendor_extension, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, VENDOR_EXTENSION);
SENSOR_DEVICE_ATTR(crc_32, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, CRC_32);
SENSOR_DEVICE_ATTR(eeprom, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, EEPROM);
SENSOR_DEVICE_ATTR(bsp_version, S_IRUGO, bsp_syseeprom_sysfs_read, NULL, BSP_VERSION);

//提供的eeprom直接读写
SYSFS_RW_ATTR_DEF(eeprom, bsp_syseeprom_sysfs_memory_read, bsp_syseeprom_sysfs_memory_write);

BSPMODULE_DEBUG_ATTR_DEF(debug, BSP_SYSEEPROM_MODULE);

BSPMODULE_DEBUG_RW_ATTR_DEF(loglevel, BSP_SYSEEPROM_MODULE);

//syseeprom node
static struct attribute *syseeprom_attributes[] =
{
    &sensor_dev_attr_product_name.dev_attr.attr,
    &sensor_dev_attr_part_number.dev_attr.attr,
    &sensor_dev_attr_serial_number.dev_attr.attr,
    &sensor_dev_attr_manufacture_date.dev_attr.attr,
    &sensor_dev_attr_base_MAC_address.dev_attr.attr,
    &sensor_dev_attr_device_version.dev_attr.attr,
    &sensor_dev_attr_label_revision.dev_attr.attr,
    &sensor_dev_attr_platform_name.dev_attr.attr,
    &sensor_dev_attr_ONIE_version.dev_attr.attr,
    &sensor_dev_attr_num_MACs.dev_attr.attr,
    &sensor_dev_attr_manufacturer.dev_attr.attr,
    &sensor_dev_attr_manufacture_country.dev_attr.attr,
    &sensor_dev_attr_vendor_name.dev_attr.attr,
    &sensor_dev_attr_diag_version.dev_attr.attr,
    &sensor_dev_attr_service_tag.dev_attr.attr,
    &sensor_dev_attr_vendor_extension.dev_attr.attr,
    &sensor_dev_attr_crc_32.dev_attr.attr,
    &sensor_dev_attr_eeprom.dev_attr.attr,
    &sensor_dev_attr_bsp_version.dev_attr.attr,
    &bspmodule_debug.attr,
    &bspmodule_loglevel.attr,
    NULL
};

static const struct attribute_group syseeprom_group =
{
    .attrs = syseeprom_attributes,
};

//debug node
static struct attribute *syseeprom_attributes_bebug[] =
{
    &eeprom.attr,
    NULL
};

static const struct attribute_group syseeprom_debug =
{
    .attrs = syseeprom_attributes_bebug,
};

int syseeprom_sysfs_init(void)
{
    int ret = ERROR_SUCCESS;

    //create node for syseeprom
    kobj_syseeprom = kobject_create_and_add("syseeprom", kobj_switch);

    if (kobj_syseeprom == NULL)
    {
        DBG_ECHO(DEBUG_ERR, "create kobj_syseeprom failed!\n");
        ret = -ENOMEM;
        goto exit;
    }

    ret = sysfs_create_group(kobj_syseeprom, &syseeprom_group);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create syseeprom group failed");

    //create eeprom for debug node
    ret = sysfs_create_group(kobj_debug, &syseeprom_debug);
    CHECK_IF_ERROR_GOTO_EXIT(ret, "create eeprom debug attribute group failed!");

exit:
    if (ret != ERROR_SUCCESS)
    {
        DBG_ECHO(DEBUG_ERR, "syseeprom init failed! result=%d", ret);
        if (kobj_syseeprom != NULL)
            kobject_put(kobj_syseeprom);
    }

    return ret;
}

void syseeprom_sysfs_exit(void)
{
    //释放debug结点
    if (kobj_debug != NULL)
    {
        sysfs_remove_group(kobj_debug, &syseeprom_debug);
    }

    //释放syseeprom结点
    if (kobj_syseeprom != NULL)
    {
        sysfs_remove_group(kobj_syseeprom, &syseeprom_group);
        kobject_put(kobj_syseeprom);
    }
    return;
}

static int __init syseeprom_init(void)
{
    int ret = ERROR_SUCCESS;
    INIT_PRINT("module init started");
    ret = syseeprom_sysfs_init();
    if (ERROR_SUCCESS == ret)
    {
        INIT_PRINT("module init finished and success!");
    }
    else
    {
        DBG_ECHO(DEBUG_ERR, "module init finished and failed!");
    }

    eeprom_table_init(I2C_DEV_EEPROM);

    return ret;
}

static void __exit syseeprom_exit(void)
{
    syseeprom_sysfs_exit();

    INIT_PRINT("module uninstalled !\n");
    return;
}

module_init(syseeprom_init);
module_exit(syseeprom_exit);

MODULE_AUTHOR("Wang Xue <wang.xue@h3c.com>");
MODULE_DESCRIPTION("h3c system eeprom driver");
MODULE_LICENSE("Dual BSD/GPL");
