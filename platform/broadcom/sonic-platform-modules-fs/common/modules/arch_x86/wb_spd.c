/*
 * wb_spd.c
 *
 * This module create sysfs to get mem info
 * through spd pci id.
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2024-09-08                  Initial version
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/delay.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/mutex.h>

#include "wb_spd.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

typedef struct wb_spd_data {
    struct pci_dev *pci_dev;
    struct device  *hwmon_dev;
    struct mutex    update_lock;
    int cpu_type;
} wb_spd_data_t;

static int intel_d1500_spd_rdata_valid(struct pci_dev *pdev, u32 *read_value)
{
    int timeout, ret;

    timeout = 0;
    while (1) {
        ret = pci_read_config_dword(pdev, INTEL_D1500_SMB_STAT, read_value);
        if (ret) {
            DEBUG_ERROR("intel_d1500_spd_rdata_valid pci_read_config_dword failed, reg: 0x%x, ret %d.\n", INTEL_D1500_SMB_STAT, ret);
            return ret;
        }

        if (!(*read_value & INTEL_D1500_SMBUS_RDO)) {
            DEBUG_VERBOSE("intel_d1500_spd rdata invalid\n");
            if (timeout > WAIT_TIME) {
                DEBUG_ERROR("intel_d1500_spd wait rdata valid timeout\n");
                return -ETIMEDOUT;
            }
            usleep_range(SLEEP_TIME, SLEEP_TIME+1);
            timeout++;
        } else {
            break;
        }
    }
    return 0;
}

static int intel_d1500_spd_set_page(int spd_page, struct pci_dev *pdev)
{
    u32 write_val, read_value;
    u32 smb_sa;
    int timeout, ret;

    timeout = 0;
    smb_sa = (spd_page == 0) ? INTEL_D1500_EE_PAGE_SEL0 : INTEL_D1500_EE_PAGE_SEL1;
    /* Send CMD one */
    write_val = INTEL_D1500_CLOCK_OVERRIDE | INTEL_D1500_DTI_WP_EEPROM;
    DEBUG_VERBOSE("intel_d1500_spd_set_page INTEL_D1500_SMB_CNTL write_val: 0x%x\n", write_val);
    ret = pci_write_config_dword(pdev, INTEL_D1500_SMB_CNTL, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1500_spd_set_page pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1500_SMB_CNTL, write_val, ret);
        return ret;
    }
    /* Send CMD two */
    write_val = INTEL_D1500_CMD_SEND | INTEL_D1500_CMD_SMB_WRITE | (smb_sa & 0x7) << 24;
    DEBUG_VERBOSE("intel_d1500_spd_set_page INTEL_D1500_SMB_CMD write_val: 0x%x\n", write_val);
    ret = pci_write_config_dword(pdev, INTEL_D1500_SMB_CMD, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1700_spd_set_page pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1500_SMB_CMD, write_val, ret);
        return ret;
    }

    usleep_range(WAIT_TIME, WAIT_TIME + 1);
    read_value = 0;
    ret = intel_d1500_spd_rdata_valid(pdev, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1500_spd_set_page wait rdata valid fail, ret %d.\n", ret);
        return ret;
    }
    return 0;
}

static int intel_d1500_smb_read_dword(struct pci_dev *pdev, int type, int reg_offset, int slot_id, unsigned int *spd_result)
{
    u32 write_val, read_value;
    int timeout, ret, spd_page;

    timeout = 0;
    spd_page = 0;

    if ((reg_offset > 0xFF) && (type == SMB_EEPROM)) {
        reg_offset = reg_offset - 0x100;
        spd_page = 1;
    }
    if (type == SMB_EEPROM) {
        ret = intel_d1500_spd_set_page(spd_page, pdev);
        if (ret < 0) {
            DEBUG_ERROR("intel_d1500_smb_read_dword SPD set page %d failed\n", spd_page);
            return ret;
        }
    }
    /* Send CMD one */
    write_val = INTEL_D1500_CLOCK_OVERRIDE;
    write_val = (type == SMB_EEPROM) ? write_val | INTEL_D1500_SELECT_EEPROM : write_val | INTEL_D1500_SELECT_TSOD;
    DEBUG_VERBOSE("intel_d1500_smb_read_dword INTEL_D1500_SMB_CNTL write_val:0x%x\n", write_val);

    ret = pci_write_config_dword(pdev, INTEL_D1500_SMB_CNTL, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1500_smb_read_dword pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1500_SMB_CNTL, write_val, ret);
        return ret;
    }

    read_value = 0;
    ret = pci_read_config_dword(pdev, INTEL_D1500_SMB_CNTL, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1500_smb_read_dword pci_read_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1500_SMB_CNTL, ret);
        return ret;
    }

    DEBUG_VERBOSE("intel_d1500_smb_read_dword SMB_CNTL rd_value: 0x%08X\n", read_value);
    /* Send CMD two */
    write_val = INTEL_D1500_CMD_SEND | (reg_offset & MASK_16) << 16 | (slot_id & 0x7) << 24;
    write_val = (type == SMB_EEPROM) ? write_val : write_val | INTEL_D1500_CMD_WORD_MODE;
    DEBUG_VERBOSE("intel_d1500_smb_read_dword INTEL_D1500_SMB_CMD write_val:0x%x\n", write_val);

    ret = pci_write_config_dword(pdev, INTEL_D1500_SMB_CMD, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1500_smb_read_dword pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1500_SMB_CMD, write_val, ret);
        return ret;
    }

    ret = pci_read_config_dword(pdev, INTEL_D1500_SMB_CMD, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1500_smb_read_dword pci_read_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1500_SMB_CMD, ret);
        return ret;
    }
    DEBUG_VERBOSE("intel_d1500_smb_read_dword SMB_CNTL rd_value: 0x%08X\n", read_value);

    ret = intel_d1500_spd_rdata_valid(pdev, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1500_smb_read_dword wait rdata valid fail, ret %d.\n", ret);
        return ret;
    }

    DEBUG_VERBOSE("intel_d1500_smb_read_dword SMB_STAT rd_value: 0x%08X\n", read_value);
    if (read_value & INTEL_D1500_SMBUS_ERROR) {
        DEBUG_ERROR("intel_d1500_smb_read_dword SMB_STAT rdata error, value: 0x%08X\n", read_value);
        return -ENODATA;
    }
    *spd_result = read_value;
    return 0;
}

static void intel_d1700_do_tsod_en(struct pci_dev *pdev)
{
    u32 read_value;
    int ret;

    read_value = 0;
    ret = pci_read_config_dword(pdev, INTEL_D1700_SMB_CMD_CFG, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1700_do_tsod_en pci_read_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1700_SMB_CMD_CFG, ret);
        return;
    }

    ret = pci_write_config_dword(pdev, INTEL_D1700_SMB_CMD_CFG, (read_value | INTEL_D1700_SMBUS_TSOD_EN));
    if (ret) {
        DEBUG_ERROR("intel_d1700_do_tsod_en pci_write_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1700_SMB_CMD_CFG, ret);
        return;
    }

    return;
}

static int intel_d1700_spd_is_busy(struct pci_dev *pdev, u32 *read_value)
{
    int timeout, ret;

    timeout = 0;
    while (1) {
        ret = pci_read_config_dword(pdev, INTEL_D1700_SMB_STAT_CFG, read_value);
        if (ret) {
            DEBUG_ERROR("intel_d1700_spd_is_busy pci_read_config_dword failed, reg: 0x%x, ret %d.\n", INTEL_D1700_SMB_STAT_CFG, ret);
            return ret;
        }

        if (*read_value & INTEL_D1700_SMBUS_BUSY) {
            DEBUG_VERBOSE("intel_d1700_spd is busy\n");
            if (timeout > WAIT_TIME) {
                DEBUG_ERROR("intel_d1700_spd wait spd busy timeout\n");
                return -ETIMEDOUT;
            }
            usleep_range(SLEEP_TIME, SLEEP_TIME+1);
            timeout++;
        } else {
            break;
        }
    }
    return 0;
}

static int intel_d1700_spd_set_page(int spd_page, struct pci_dev *pdev)
{
    u32 write_val, read_value;
    u32 smb_sa;
    int timeout, ret;

    timeout = 0;
    smb_sa = (spd_page == 0) ? INTEL_D1700_EE_PAGE_SEL0 : INTEL_D1700_EE_PAGE_SEL1;
    write_val = INTEL_D1700_CLOCK_OVERRIDE | (smb_sa & 0x7) << 8 | INTEL_D1700_CMD_SEND | (INTEL_D1700_CMD_SMB_WRITE) | INTEL_D1700_DTI_WP_EEPROM;
    DEBUG_VERBOSE("intel_d1700_spd_set_page SMB_CMD wr_value 0x%x\n", write_val);

    ret = pci_write_config_dword(pdev, INTEL_D1700_SMB_DATA_CFG, 0x0);
    if (ret) {
        DEBUG_ERROR("intel_d1700_spd_set_page pci_write_config_dword failed, reg: 0x%x, wr_value:0x0, ret %d.\n", INTEL_D1700_SMB_DATA_CFG, ret);
        return ret;
    }
    ret = pci_write_config_dword(pdev, INTEL_D1700_SMB_CMD_CFG, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1700_spd_set_page pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1700_SMB_DATA_CFG, write_val, ret);
        return ret;
    }

    usleep_range(WAIT_TIME, WAIT_TIME + 1);
    read_value = 0;
    ret = intel_d1700_spd_is_busy(pdev, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1700_spd_set_page wait spd busy fail, ret %d.\n", ret);
        return ret;
    }

    intel_d1700_do_tsod_en(pdev);
    return 0;
}

static int intel_d1700_smb_read_dword(struct pci_dev *pdev, int type, int reg_offset, int slot_id, u32 *spd_result)
{
    u32 write_val, read_value;
    int timeout, ret, spd_page;

    timeout = 0;
    spd_page = 0;
    if ((reg_offset > 0xFF) && (type == SMB_EEPROM)) {
        reg_offset = reg_offset - 0x100;
        spd_page = 1;
    }
    if (type == SMB_EEPROM) {
        ret = intel_d1700_spd_set_page(spd_page, pdev);
        if (ret < 0) {
            DEBUG_ERROR("intel_d1700_smb_read_dword SPD set page %d failed\n", spd_page);
            goto do_tsod_en;
        }
    }
    /* Send CMD one */
    write_val = INTEL_D1700_CLOCK_OVERRIDE | (slot_id & 0x7) << 8 | (reg_offset & MASK_16) | INTEL_D1700_CMD_SEND;
    write_val = (type == SMB_EEPROM) ?  write_val | INTEL_D1700_SELECT_EEPROM: write_val | INTEL_D1700_SELECT_TSOD | INTEL_D1700_CMD_WORD_MODE;
    DEBUG_VERBOSE("intel_d1700_smb_read_dword INTEL_D1700_SMB_CMD_CFG write_val:0x%x\n", write_val);

    ret = pci_write_config_dword(pdev, INTEL_D1700_SMB_CMD_CFG, write_val);
    if (ret) {
        DEBUG_ERROR("intel_d1700_smb_read_dword pci_write_config_dword failed, reg: 0x%x, wr_value:0x%x, ret %d.\n", INTEL_D1700_SMB_CMD_CFG, write_val, ret);
        goto do_tsod_en;
    }

    read_value = 0;
    ret = pci_read_config_dword(pdev, INTEL_D1700_SMB_CMD_CFG, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1700_smb_read_dword pci_read_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1700_SMB_CMD_CFG, ret);
        goto do_tsod_en;
    }

    DEBUG_VERBOSE("intel_d1700_smb_read_dword SMB_CMD_CFG rd_value: 0x%08X\n", read_value);
    ret = intel_d1700_spd_is_busy(pdev, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1700_smb_read_dword wait spd busy fail, ret %d.\n", ret);
        goto do_tsod_en;
    }

    if (read_value & INTEL_D1700_SMBUS_ERROR) {
        DEBUG_ERROR("intel_d1700_smb_read_dword SMB_STAT rdata error, value: 0x%08X\n", read_value);
        ret = -ENODATA;
        goto do_tsod_en;
    }

    ret = pci_read_config_dword(pdev, INTEL_D1700_SMB_DATA_CFG, &read_value);
    if (ret) {
        DEBUG_ERROR("intel_d1700_smb_read_dword pci_read_config_dword failed, reg: 0x%x ret %d.\n", INTEL_D1700_SMB_DATA_CFG, ret);
        goto do_tsod_en;
    }

    DEBUG_VERBOSE("intel_d1700_smb_read_dword SMB_DATA rd_value: 0x%08X\n", read_value);
    *spd_result = read_value;
do_tsod_en:
    intel_d1700_do_tsod_en(pdev);
    return ret;
}

static int smb_read_dword(struct pci_dev *pdev, int type, int reg_offset, int slot_id, u32 *spd_result, int cpu_type)
{
    switch (cpu_type) {
    case TYPE_INTEL_D1700:
        return intel_d1700_smb_read_dword(pdev, type, reg_offset, slot_id, spd_result);
    case TYPE_INTEL_D1500:
        return intel_d1500_smb_read_dword(pdev, type, reg_offset, slot_id, spd_result);
    default:
        DEBUG_ERROR("unsupport cpu type\n");
        return -EINVAL;
    }
}

static ssize_t spd_temp_show(struct device *dev, struct device_attribute *da, char *buf)
{
    uint32_t slot, reg, value;
    int ret;
    wb_spd_data_t  *spd_data = dev_get_drvdata(dev);
    int temp;
    int decimal_bits;
    int integer_bits;
    int flag = 1;

    reg = to_sensor_dev_attr_2(da)->index;
    slot = to_sensor_dev_attr_2(da)->nr;

    mutex_lock(&spd_data->update_lock);
    ret = smb_read_dword(spd_data->pci_dev, SMB_TSOD, reg, slot, &value, spd_data->cpu_type);
    if (ret) {
        DEBUG_ERROR("spd_temp_show read reg failed ret %d.\n", ret);
        mutex_unlock(&spd_data->update_lock);
        return -EIO;
    }
    mutex_unlock(&spd_data->update_lock);
    DEBUG_VERBOSE("spd_temp_show read reg success, rd_value: 0x%X\n", value);

    /* get valid value */
    value = (value & MEM_TEMP_VALID_VALUE_MASK) >> 2;
    /* check the sign bit */
    if  (value & MEM_TEMP_SIGN_BIT) {
        value = ~value + 1;
        flag = -1;
    }
    decimal_bits = value & MEM_TEMP_DECIMAL_BITS_MASK;
    integer_bits = (value >> 2) & MEM_TEMP_INTEGER_BITS_MASK;
    temp = (integer_bits * 1000 + (decimal_bits * 250)) * flag;

    return snprintf(buf, PAGE_SIZE, "%d\n", temp);
}

static ssize_t spd_date_code_show(struct device *dev, struct device_attribute *da, char *buf)
{
    uint32_t slot, value;
    int ret;
    wb_spd_data_t  *spd_data = dev_get_drvdata(dev);

    slot = to_sensor_dev_attr_2(da)->nr;

    mutex_lock(&spd_data->update_lock);
    ret = smb_read_dword(spd_data->pci_dev, SMB_EEPROM, MEM_DATE_CODE_REG, slot, &value, spd_data->cpu_type);
    if (ret) {
        DEBUG_ERROR("spd_date_code_show read reg failed ret %d.\n", ret);
        mutex_unlock(&spd_data->update_lock);
        return -EIO;
    }
    mutex_unlock(&spd_data->update_lock);
    DEBUG_VERBOSE("spd_date_code_show read reg success, rd_value: 0x%X\n", value);

    return snprintf(buf, PAGE_SIZE, "0x%04X\n", value);
}

static SENSOR_DEVICE_ATTR_2(mem_temp_1, S_IRUGO, spd_temp_show, NULL, 0, MEM_TEMP_REG);
static SENSOR_DEVICE_ATTR_2(mem_temp_2, S_IRUGO, spd_temp_show, NULL, 1, MEM_TEMP_REG);
static SENSOR_DEVICE_ATTR_2(mem_temp_3, S_IRUGO, spd_temp_show, NULL, 2, MEM_TEMP_REG);
static SENSOR_DEVICE_ATTR_2(mem_temp_4, S_IRUGO, spd_temp_show, NULL, 3, MEM_TEMP_REG);
static SENSOR_DEVICE_ATTR_2(mem_date_code_1, S_IRUGO, spd_date_code_show, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(mem_date_code_2, S_IRUGO, spd_date_code_show, NULL, 1, 0);
static SENSOR_DEVICE_ATTR_2(mem_date_code_3, S_IRUGO, spd_date_code_show, NULL, 2, 0);
static SENSOR_DEVICE_ATTR_2(mem_date_code_4, S_IRUGO, spd_date_code_show, NULL, 3, 0);

static struct attribute *spd_hwmon_attrs[] = {
    &sensor_dev_attr_mem_temp_1.dev_attr.attr,
    &sensor_dev_attr_mem_temp_2.dev_attr.attr,
    &sensor_dev_attr_mem_temp_3.dev_attr.attr,
    &sensor_dev_attr_mem_temp_4.dev_attr.attr,
    &sensor_dev_attr_mem_date_code_1.dev_attr.attr,
    &sensor_dev_attr_mem_date_code_2.dev_attr.attr,
    &sensor_dev_attr_mem_date_code_3.dev_attr.attr,
    &sensor_dev_attr_mem_date_code_4.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(spd_hwmon);

static int spd_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    wb_spd_data_t *wb_spd_data;

    DEBUG_VERBOSE("spd_probe.\n");
    wb_spd_data = devm_kzalloc(&pdev->dev, sizeof(wb_spd_data_t), GFP_KERNEL);
    if (!wb_spd_data) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    wb_spd_data->pci_dev = pdev;
    wb_spd_data->cpu_type = id->driver_data;
    wb_spd_data->hwmon_dev = hwmon_device_register_with_groups(&pdev->dev, pdev->driver->name, wb_spd_data, spd_hwmon_groups);
	if (IS_ERR(wb_spd_data->hwmon_dev)) {
        dev_err(&pdev->dev, "Failed to register spd hwmon\n");
        return PTR_ERR(wb_spd_data->hwmon_dev);
    }
    mutex_init(&wb_spd_data->update_lock);
    pci_set_drvdata(pdev, wb_spd_data);
    dev_info(&pdev->dev, "wb spd probe success.\n");
    return 0;
}

static void spd_remove(struct pci_dev *pdev)
{
    wb_spd_data_t *wb_spd_data;

    DEBUG_VERBOSE("spd_remove.\n");
    wb_spd_data = pci_get_drvdata(pdev);
    hwmon_device_unregister(wb_spd_data->hwmon_dev);
    dev_info(&pdev->dev, "wb spd remove success.\n");

    return;
}

static const struct pci_device_id spd_pci_ids[] = {
    { PCI_DEVICE(PCI_VENDOR_ID_INTEL, INTEL_D1500_SPD_DEVICE_ID), .driver_data = TYPE_INTEL_D1500},
    { PCI_DEVICE(PCI_VENDOR_ID_INTEL, INTEL_D1700_SPD_DEVICE_ID), .driver_data = TYPE_INTEL_D1700},
    {0}
};
MODULE_DEVICE_TABLE(pci, spd_pci_ids);

static struct pci_driver wb_spd_driver = {
    .name = "wb_spd",
    .id_table = spd_pci_ids,
    .probe = spd_probe,
    .remove = spd_remove,
};

static int __init wb_spd_init(void)
{
    DEBUG_VERBOSE("wb_spd_init enter!\n");
    return pci_register_driver(&wb_spd_driver);
}

static void __exit wb_spd_exit(void)
{
    DEBUG_VERBOSE("wb_spd_exit enter!\n");
    pci_unregister_driver(&wb_spd_driver);
    return;
}

module_init(wb_spd_init);
module_exit(wb_spd_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("spd Driver");
MODULE_LICENSE("GPL");
