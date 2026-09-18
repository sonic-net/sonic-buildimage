// SPDX-License-Identifier: GPL-2.0+
/*
 * Hardware monitoring driver for Renesas Digital Multiphase Voltage Regulators
 *
 * Copyright (c) 2017 Google Inc
 * Copyright (c) 2020 Renesas Electronics America
 *
 */

#include <linux/err.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>

#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

#define ISL68137_VOUT_AVS       (0x30)
#define RAA_DMPVR2_READ_VMON    (0xc8)
#define WRITE_PROTECT_CLOSE     (0x00)
#define WRITE_PROTECT_OPEN      (0x40)

/******* FW_UPG common register define ********/
#define FW_UPG_CYCLING_VCC_ADDR         (0xE6)
#define FW_UPG_CYCLING_VCC_VAL          (0x0007)
#define FW_UPG_RESTORE_CFG_ADDR         (0xF2)    /* Configuration ID 0~15 */
#define FW_UPG_DMA_ADDRESS              (0xC7)
#define FW_UPG_DMA_DATA_ADDR            (0xC5)
#define FW_UPG_PRODUCTION_HEX_MAGIC     (0xFFFE0001)
#define FW_UPG_MAX_CHIP_NVM_TIME        (28)
#define FW_UPG_ENABLE_DEVICE_DATA       (0x00000009)
#define FW_UPG_RESTORE_DEVICE_DATA      (0x00000001)

/******* FW_UPG_TYPE1 register define *********/
#define FW_UPG_TYPE1_CRC_ADDR           (0x0094)
#define FW_UPG_TYPE1_NVM_TIME_ADDR      (0x0035)
#define FW_UPG_TYPE1_HEX_FILE_ADDR      (0xBD87)
#define FW_UPG_TYPE1_UPG_STATUS_ADDR    (0x007E)
#define FW_UPG_TYPE1_BANK_STATUS_ADDR   (0x007F)
#define FW_UPG_TYPE1_DEV_DATA_ADDR      (0xECF0)
#define FW_UPG_TYPE1_MCUFLT_ADDR        (0xEC01)
#define FW_UPG_TYPE1_PARTID_ADDR        (0x00E8)

/******* FW_UPG_TYPE2 register define *********/
#define FW_UPG_TYPE2_CRC_ADDR           (0x00F8)
#define FW_UPG_TYPE2_NVM_TIME_ADDR      (0x0035)
#define FW_UPG_TYPE2_UPG_STATUS_ADDR    (0x0083)
#define FW_UPG_TYPE2_BANK_STATUS_ADDR   (0x0084)
#define FW_UPG_TYPE2_DEV_DATA_ADDR      (0xECF0)
#define FW_UPG_TYPE2_MCUFLT_ADDR        (0xEC01)
#define FW_UPG_TYPE2_PARTID_ADDR        (0x0184)

#define PHASE_CURR_PAGE          (0X80)
#define PHASE_VAL_REG            (0XE4)
#define PHASE1_CURR_REG          (0X00)
#define PHASE2_CURR_REG          (0X01)
#define PHASE3_CURR_REG          (0X02)
#define PHASE4_CURR_REG          (0X03)
#define PHASE5_CURR_REG          (0X04)
#define PHASE6_CURR_REG          (0X05)
#define PHASE7_CURR_REG          (0X06)
#define PHASE8_CURR_REG          (0X07)
#define PHASE9_CURR_REG          (0X08)
#define PHASE10_CURR_REG         (0X09)
#define PHASE11_CURR_REG         (0X0A)
#define PHASE12_CURR_REG         (0X0B)
#define PHASE13_CURR_REG         (0X0C)
#define PHASE14_CURR_REG         (0X0D)
#define PHASE15_CURR_REG         (0X0E)
#define PHASE16_CURR_REG         (0X0F)
#define PER_LSB_100MA            (100)


static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static pmbus_info_t isl68137_dfx_infos[] = {
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_BYTE", .pmbus_reg = PMBUS_STATUS_BYTE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_WORD", .pmbus_reg = PMBUS_STATUS_WORD, .width = WORD_DATA, .data_type = RAWDATA_WORD},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_VOUT", .pmbus_reg = PMBUS_STATUS_VOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_IOUT", .pmbus_reg = PMBUS_STATUS_IOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_INPUT", .pmbus_reg = PMBUS_STATUS_INPUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_TEMP", .pmbus_reg = PMBUS_STATUS_TEMPERATURE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_CML", .pmbus_reg = PMBUS_STATUS_CML, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_MFR_SPEC", .pmbus_reg = PMBUS_STATUS_MFR_SPECIFIC, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_VOUT", .pmbus_reg = PMBUS_READ_VOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_IOUT", .pmbus_reg = PMBUS_READ_IOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
};

typedef enum {
    FW_UPG_TYPE_FAIL = 0,   /* Get firmware upgrade type failed */
    FW_UPG_TYPE_UNSUPPORT,  /* Unsupport firmware upgrade */
    FW_UPG_TYPE1,           /* refer to <Renesas_DMP_Gen3_Production_Programming_rev1p3> */
    FW_UPG_TYPE2,           /* refer to <Renesas_DMP_Gen3p5_Production_Programming_rev1p1> */
} fw_upg_type_t;

typedef enum {
    FW_UPG_CHIP_ID_FAIL = 0,    /* Get ic_device_id failed */
    FW_UPG_CHIP_ID_UNKNOWN,     /* Get ic_device_id success, but unsupport to upgrade this chip id */
    FW_UPG_HEX_TYPE_IGNORE,     /* Get ic_device_id success, and don't care about the hex file type */
    FW_UPG_HEX_TYPE_FAIL,       /* Get hex file type failed */
    FW_UPG_HEX_TYPE_UNKNOWN,    /* Unknown hex file type */
    FW_UPG_HEX_TYPE_LEGACY,     /* Legacy hex Files */
    FW_UPG_HEX_TYPE_PRODUCTION, /* Production hex Files */
} fw_upg_hex_type_t;

typedef struct {
    uint32_t ic_device_id;
    fw_upg_type_t fw_upg_type;
} chip_info_t;

typedef struct {
    uint16_t crc_addr;
    uint16_t nvm_time_addr;
    uint16_t hex_file_addr;
    uint16_t upg_status_addr;
    uint16_t bank_status_addr;
    uint16_t dev_data_addr;
    uint16_t mcuflt_addr;
    uint16_t partid_addr;
} fw_upg_reg_addr_t;

typedef struct {
    chip_info_t chip_info;
    fw_upg_reg_addr_t fw_upg_reg;
    struct pmbus_driver_info info;
    struct i2c_client *client;
    struct miscdevice misc_dev;
    char misc_dev_name[DEV_NAME_LEN];
} renesas_info_t;
#define to_renesas_data(_info) container_of(_info, renesas_info_t, info)

static DEFINE_SPINLOCK(dev_array_lock);
static renesas_info_t* renesas_dev_arry[MAX_DEV_NUM];

static const chip_info_t renesas_device_infos[] = {
    {.ic_device_id = 0x49D28100, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29000, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2AF00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2AE00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28F00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28E00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B400, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B200, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B300, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B500, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B600, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28200, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29700, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29800, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29600, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28800, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28A00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29B00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29C00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28B00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28D00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2A400, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2A200, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D29D00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D27F00, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B100, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28000, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28600, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D28700, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2B800, .fw_upg_type = FW_UPG_TYPE1},
    {.ic_device_id = 0x49D2BA00, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2BB00, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2BC00, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2D600, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2BD00, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2BE00, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2D500, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C000, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2D700, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C100, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2D800, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C200, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2D900, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C400, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C500, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C600, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C700, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C800, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2C900, .fw_upg_type = FW_UPG_TYPE2},
    {.ic_device_id = 0x49D2CA00, .fw_upg_type = FW_UPG_TYPE2},
};

enum chips {
    isl68137,
    isl68220,
    isl68221,
    isl68222,
    isl68223,
    isl68224,
    isl68225,
    isl68226,
    isl68227,
    isl68229,
    isl68233,
    isl68239,
    isl69222,
    isl69223,
    isl69224,
    isl69225,
    isl69227,
    isl69228,
    isl69234,
    isl69236,
    isl69239,
    isl69242,
    isl69243,
    isl69247,
    isl69248,
    isl69254,
    isl69255,
    isl69256,
    isl69259,
    isl69260,
    isl69268,
    isl69269,
    isl69298,
    raa228000,
    raa228004,
    raa228006,
    raa228228,
    raa229001,
    raa229004,
};

enum variants {
    raa_dmpvr1_2rail,
    raa_dmpvr2_1rail,
    raa_dmpvr2_2rail,
    raa_dmpvr2_2rail_nontc,
    raa_dmpvr2_3rail,
    raa_dmpvr2_hv,
};

static const struct i2c_device_id raa_dmpvr_id[];

static ssize_t isl68137_avs_enable_show_page(struct i2c_client *client,
                         int page,
                         char *buf)
{
    int val = wb_pmbus_read_byte_data(client, page, PMBUS_OPERATION);

    return sprintf(buf, "%d\n",
               (val & ISL68137_VOUT_AVS) == ISL68137_VOUT_AVS ? 1 : 0);
}

static ssize_t isl68137_avs_enable_store_page(struct i2c_client *client,
                          int page,
                          const char *buf, size_t count)
{
    int rc, op_val;
    bool result;

    result = false;
    rc = kstrtobool(buf, &result);
    if (rc)
        return rc;

    op_val = result ? ISL68137_VOUT_AVS : 0;

    /*
     * Writes to VOUT setpoint over AVSBus will persist after the VRM is
     * switched to PMBus control. Switching back to AVSBus control
     * restores this persisted setpoint rather than re-initializing to
     * PMBus VOUT_COMMAND. Writing VOUT_COMMAND first over PMBus before
     * enabling AVS control is the workaround.
     */
    if (op_val == ISL68137_VOUT_AVS) {
        rc = wb_pmbus_read_word_data(client, page, 0xff,
                      PMBUS_VOUT_COMMAND);
        if (rc < 0)
            return rc;

        rc = wb_pmbus_write_word_data(client, page, PMBUS_VOUT_COMMAND,
                       rc);
        if (rc < 0)
            return rc;
    }

    rc = wb_pmbus_update_byte_data(client, page, PMBUS_OPERATION,
                    ISL68137_VOUT_AVS, op_val);

    return (rc < 0) ? rc : count;
}

static ssize_t isl68137_avs_enable_show(struct device *dev,
                    struct device_attribute *devattr,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

    return isl68137_avs_enable_show_page(client, attr->index, buf);
}

static ssize_t isl68137_avs_enable_store(struct device *dev,
                struct device_attribute *devattr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

    return isl68137_avs_enable_store_page(client, attr->index, buf, count);
}

static ssize_t isl68137_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_cmd, vout;

    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable isl68137_avs_vout_show failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }
    vout = vout_cmd * 1000;
    DEBUG_VERBOSE("%d-%04x: page%d, vout: %d, vout_cmd: 0x%x\n", client->adapter->nr,
        client->addr, attr->index, vout, vout_cmd);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", vout);
}

static ssize_t isl68137_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout, vout_max, vout_min;
    int ret, vout_cmd, vout_cmd_set;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout = 0;
    ret = kstrtoint(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %d \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        DEBUG_ERROR("%d-%04x: vout value: %d, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    /* calc VOUT_COMMAND set value */
    vout_cmd_set = vout / 1000;
    if (vout_cmd_set > 0xffff) {
        DEBUG_ERROR("%d-%04x: invalid value, vout %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable isl68137_avs_vout_store failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }

    /* close write protect */
    ret = wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_CLOSE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: close page%d write protect failed, ret: %d\n", client->adapter->nr,
            client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        DEBUG_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, ret);
        goto error;
    }

    /* compare vout_cmd and vout_cmd_set */
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        DEBUG_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;
    }

    /* open write protect */
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: set page%d vout cmd success, vout %d, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
error:
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    return ret;
}

static int read_renesas_device_id(struct i2c_client *client, uint32_t *device_id)
{
    int block_size;
    u8 block_data[I2C_SMBUS_BLOCK_MAX + 2] = { 0 };

    block_size = wb_pmbus_read_block_data(client, 0, PMBUS_IC_DEVICE_ID, block_data);
    if (block_size < 0) {
        DEBUG_VERBOSE("%d-%04x: read renesas ic_device_id failed, ret: %d\n",
            client->adapter->nr, client->addr, block_size);
        return block_size;
    }

    if (block_size != WIDTH_4Byte) {
        DEBUG_VERBOSE("%d-%04x: Invalid ic_device_id len %d\n",
            client->adapter->nr, client->addr, block_size);
        return -EINVAL;
    }

    *device_id = ((block_data[3] << 24) |(block_data[2] << 16) | (block_data[1] << 8) | block_data[0]);
    DEBUG_VERBOSE("%d-%04x: renesas ic_device_id: 0x%08x\n",
        client->adapter->nr, client->addr, *device_id);
    return 0;
}

static void renesas_fw_upg_reg_init(renesas_info_t *renesas_info)
{

    if (renesas_info->chip_info.fw_upg_type == FW_UPG_TYPE1) {
        renesas_info->fw_upg_reg.crc_addr = FW_UPG_TYPE1_CRC_ADDR;
        renesas_info->fw_upg_reg.nvm_time_addr = FW_UPG_TYPE1_NVM_TIME_ADDR;
        renesas_info->fw_upg_reg.hex_file_addr = FW_UPG_TYPE1_HEX_FILE_ADDR;
        renesas_info->fw_upg_reg.upg_status_addr = FW_UPG_TYPE1_UPG_STATUS_ADDR;
        renesas_info->fw_upg_reg.bank_status_addr = FW_UPG_TYPE1_BANK_STATUS_ADDR;
        renesas_info->fw_upg_reg.dev_data_addr = FW_UPG_TYPE1_DEV_DATA_ADDR;
        renesas_info->fw_upg_reg.mcuflt_addr = FW_UPG_TYPE1_MCUFLT_ADDR;
        renesas_info->fw_upg_reg.partid_addr = FW_UPG_TYPE1_PARTID_ADDR;
    } else {
        renesas_info->fw_upg_reg.crc_addr = FW_UPG_TYPE2_CRC_ADDR;
        renesas_info->fw_upg_reg.nvm_time_addr = FW_UPG_TYPE2_NVM_TIME_ADDR;
        renesas_info->fw_upg_reg.upg_status_addr = FW_UPG_TYPE2_UPG_STATUS_ADDR;
        renesas_info->fw_upg_reg.bank_status_addr = FW_UPG_TYPE2_BANK_STATUS_ADDR;
        renesas_info->fw_upg_reg.dev_data_addr = FW_UPG_TYPE2_DEV_DATA_ADDR;
        renesas_info->fw_upg_reg.mcuflt_addr = FW_UPG_TYPE2_MCUFLT_ADDR;
        renesas_info->fw_upg_reg.partid_addr = FW_UPG_TYPE2_PARTID_ADDR;
    }
    return;
}

static void renesas_fw_upg_init(renesas_info_t *renesas_info)
{
    uint32_t device_id;
    struct i2c_client *client;
    struct pmbus_data *data;
    int ret, i;

    client = renesas_info->client;
    data = i2c_get_clientdata(client);
    device_id = 0;
    mutex_lock(&data->update_lock);
    ret = read_renesas_device_id(client, &device_id);
    mutex_unlock(&data->update_lock);

    if (ret < 0) {
        renesas_info->chip_info.fw_upg_type = FW_UPG_TYPE_FAIL;
        return;
    }
    renesas_info->chip_info.ic_device_id = device_id;
    for (i = 0; i < ARRAY_SIZE(renesas_device_infos); i++) {
        if (renesas_device_infos[i].ic_device_id == device_id) {
            DEBUG_VERBOSE("%d-%04x: renesas device id: 0x%08x match, fw_upg_type: %d\n",
                client->adapter->nr, client->addr, device_id, renesas_device_infos[i].fw_upg_type);
            if ((renesas_device_infos[i].fw_upg_type != FW_UPG_TYPE1) &&
                (renesas_device_infos[i].fw_upg_type != FW_UPG_TYPE2)) {
                DEBUG_VERBOSE("%d-%04x: renesas device id: 0x%08x invalid fw_upg_type: %d\n",
                    client->adapter->nr, client->addr, device_id, renesas_device_infos[i].fw_upg_type);
                renesas_info->chip_info.fw_upg_type = FW_UPG_TYPE_UNSUPPORT;
                return;
            }
            renesas_info->chip_info.fw_upg_type = renesas_device_infos[i].fw_upg_type;
            renesas_fw_upg_reg_init(renesas_info);
            return;
        }
    }
    DEBUG_VERBOSE("%d-%04x: Unsupport renesas device id: 0x%08x \n",
        client->adapter->nr, client->addr, device_id);
    renesas_info->chip_info.fw_upg_type = FW_UPG_TYPE_UNSUPPORT;
    return;
}

static int renesas_fw_upg_reinit(struct i2c_client *client)
{
    fw_upg_type_t fw_upg_type;
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));

    fw_upg_type = renesas_info->chip_info.fw_upg_type;
    if (fw_upg_type == FW_UPG_TYPE_FAIL) {
        DEBUG_VERBOSE("%d-%04x: fw_upg_type failed, try to get fw_upg_type\n",
            client->adapter->nr, client->addr);
        renesas_fw_upg_init(renesas_info);
        fw_upg_type = renesas_info->chip_info.fw_upg_type;
        if (fw_upg_type == FW_UPG_TYPE_FAIL) {
            DEBUG_VERBOSE("%d-%04x: get fw_upg_type failed\n", client->adapter->nr, client->addr);
            return -EIO;
        }
    }
    if (fw_upg_type == FW_UPG_TYPE_UNSUPPORT) {
        return -ENODEV;
    }
    return 0;
}


static int renesas_dma_data_read(struct i2c_client *client, uint16_t dma_addr, uint32_t *value)
{
    int ret;
    u8 block_data[WIDTH_4Byte];

    /* set dma addr */
    ret = wb_pmbus_write_word_data(client, 0, FW_UPG_DMA_ADDRESS, dma_addr);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set dma address: 0x%04x failed, ret: %d \n",
            client->adapter->nr, client->addr, dma_addr, ret);
        return ret;
    }
    /* read data */
    mem_clear(block_data, sizeof(block_data));
    ret = wb_pmbus_read_i2c_block_data(client, 0, FW_UPG_DMA_DATA_ADDR, sizeof(block_data), block_data);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read dma address: 0x%04x data failed, ret: %d\n",
            client->adapter->nr, client->addr, dma_addr, ret);
        return ret;
    }

    *value = ((block_data[3] << 24) |(block_data[2] << 16) | (block_data[1] << 8) | block_data[0]);
    DEBUG_VERBOSE("%d-%04x: read dma addr: 0x%04x, value: 0x%08x success\n",
        client->adapter->nr, client->addr, dma_addr, *value);
    return 0;
}

static int renesas_dma_data_write(struct i2c_client *client, uint16_t dma_addr, uint32_t value)
{
    int ret;
    u8 block_data[WIDTH_4Byte];

    /* set dma addr */
    ret = wb_pmbus_write_word_data(client, 0, FW_UPG_DMA_ADDRESS, dma_addr);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set dma address: 0x%04x failed, ret: %d \n",
            client->adapter->nr, client->addr, dma_addr, ret);
        return ret;
    }
    /* write data */
    mem_clear(block_data, sizeof(block_data));
    block_data[3] = (value >> 24) & 0xff;
    block_data[2] = (value >> 16) & 0xff;
    block_data[1] = (value >> 8) & 0xff;
    block_data[0] = (value) & 0xff;
    ret = wb_pmbus_write_i2c_block_data(client, 0, FW_UPG_DMA_DATA_ADDR, sizeof(block_data), block_data);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: write dma address: 0x%04x, value: 0x%08xfailed, ret: %d\n",
            client->adapter->nr, client->addr, dma_addr, value, ret);
        return ret;
    }
    DEBUG_VERBOSE("%d-%04x: write dma addr: 0x%04x, value: 0x%08x success\n",
        client->adapter->nr, client->addr, dma_addr, value);
    return 0;
}


static ssize_t renesas_dma_data_show(struct i2c_client *client, uint16_t dma_addr, char *buf, size_t count)
{
    int ret;
    struct pmbus_data *data;
    uint32_t value;

    value = 0;
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret =renesas_dma_data_read(client, dma_addr, &value);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        return ret;
    }
    mem_clear(buf, count);
    return snprintf(buf, count, "0x%08x\n", value);
}

static ssize_t chip_crc_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.crc_addr, buf, PAGE_SIZE);
}

static ssize_t chip_nvm_time_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.nvm_time_addr, buf, PAGE_SIZE);
}

static int get_hex_file_type(struct i2c_client *client, renesas_info_t *renesas_info)
{
    uint32_t chip_nvm_time, hex_file_flag;
    int ret;

    /* get chip_nvm_time */
    chip_nvm_time = 0;
    ret = renesas_dma_data_read(client, renesas_info->fw_upg_reg.nvm_time_addr, &chip_nvm_time);
    if (ret < 0) {
        return ret;
    }

    /* get hex file flag */
    hex_file_flag = 0;
    ret = renesas_dma_data_read(client, renesas_info->fw_upg_reg.hex_file_addr, &hex_file_flag);
    if (ret < 0) {
        return ret;
    }

    DEBUG_VERBOSE("%d-%04x: chip_nvm_time: %d, hex_file_flag: 0x%08x\n",
        client->adapter->nr, client->addr, chip_nvm_time, hex_file_flag);
    if ((hex_file_flag == 0) && (chip_nvm_time < FW_UPG_MAX_CHIP_NVM_TIME)) {
        return FW_UPG_HEX_TYPE_LEGACY;
    }

    if ((hex_file_flag == FW_UPG_PRODUCTION_HEX_MAGIC) ||
        (chip_nvm_time == FW_UPG_MAX_CHIP_NVM_TIME)) {
            return FW_UPG_HEX_TYPE_PRODUCTION;
    }

    return FW_UPG_HEX_TYPE_UNKNOWN;
}

static ssize_t hex_file_type_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    fw_upg_type_t fw_upg_type;
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret == -EIO) {  /* get chip id failed */
        return snprintf(buf, PAGE_SIZE, "%d\n", FW_UPG_CHIP_ID_FAIL);
    }
    if (ret == -ENODEV) {  /* unsupport chip id */
        return snprintf(buf, PAGE_SIZE, "%d\n", FW_UPG_CHIP_ID_UNKNOWN);
    }
    /* get fw_upg_type success */
    fw_upg_type = renesas_info->chip_info.fw_upg_type;
    if (fw_upg_type == FW_UPG_TYPE2) {
        return snprintf(buf, PAGE_SIZE, "%d\n", FW_UPG_HEX_TYPE_IGNORE);
    }
    /* FW_UPG_TYPE1, get hex file type */
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = get_hex_file_type(client, renesas_info);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get_hex_file_type failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        return snprintf(buf, PAGE_SIZE, "%d\n", FW_UPG_HEX_TYPE_FAIL);
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static ssize_t upg_status_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.upg_status_addr, buf, PAGE_SIZE);
}

static ssize_t bank_status_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.bank_status_addr, buf, PAGE_SIZE);
}

static ssize_t en_device_data_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;
    struct pmbus_data *data;
    uint32_t value;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    value = 0;
    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret =renesas_dma_data_read(client, renesas_info->fw_upg_reg.dev_data_addr, &value);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        return ret;
    }
    mem_clear(buf, PAGE_SIZE);
    if (value == FW_UPG_ENABLE_DEVICE_DATA) {
        return snprintf(buf, PAGE_SIZE, "1\n");
    }
    if (value == FW_UPG_RESTORE_DEVICE_DATA) {
        return snprintf(buf, PAGE_SIZE, "0\n");
    }

    return snprintf(buf, PAGE_SIZE, "UNKNOWN: 0x%08x\n", value);
}

static ssize_t en_device_data_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret, val;
    struct pmbus_data *data;
    uint32_t wr_val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if ((val != 0) && (val != 1)) {
        DEBUG_ERROR("%d-%04x: Invalid en_device_data value: %d, can't do en_device_data operation\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    if (val == 0) {
        wr_val = FW_UPG_RESTORE_DEVICE_DATA;
    } else {
        wr_val = FW_UPG_ENABLE_DEVICE_DATA;
    }

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret =renesas_dma_data_write(client, renesas_info->fw_upg_reg.dev_data_addr, wr_val);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        return ret;
    }

    return count;
}

static ssize_t mcu_fault_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.mcuflt_addr, buf, PAGE_SIZE);
}

static ssize_t part_id_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));
    int ret;

    ret = renesas_fw_upg_reinit(client);
    if (ret < 0) {
        return ret;
    }

    return renesas_dma_data_show(client, renesas_info->fw_upg_reg.partid_addr, buf, PAGE_SIZE);
}

static ssize_t cycling_vcc_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret, val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if (val != 1) {
        DEBUG_ERROR("%d-%04x: Invalid val: %d, can't do vcc cycling operation\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = wb_pmbus_write_word_data(client, 0, FW_UPG_CYCLING_VCC_ADDR, FW_UPG_CYCLING_VCC_VAL);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set vcc cycling failed, addr 0x%02x, write value: 0x%04x \n",
            client->adapter->nr, client->addr, FW_UPG_CYCLING_VCC_ADDR, FW_UPG_CYCLING_VCC_VAL);
        return ret;
    }
    return count;
}

static ssize_t restore_cfg_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int ret;

    ret = wb_pmbus_read_byte_data(client, 0, FW_UPG_RESTORE_CFG_ADDR);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read restore cfg failed, reg addr 0x%02x, ret: %d \n",
            client->adapter->nr, client->addr, FW_UPG_RESTORE_CFG_ADDR, ret);
        return ret;
    }
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "0x%02x\n", ret);
}

static ssize_t restore_cfg_store(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret, val;

    val = 0;
    ret = kstrtoint(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: Invalid val: %s, kstrtoint failed, ret: %d\n",
            client->adapter->nr, client->addr, buf, ret);
        return ret;
    }

    if ((val < 0) || (val >= 16)) {
        DEBUG_ERROR("%d-%04x: Invalid Configuration ID: %d, can't do restore cfg operation\n",
            client->adapter->nr, client->addr, val);
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    ret = wb_pmbus_write_byte_data(client, 0, FW_UPG_RESTORE_CFG_ADDR, val);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: restore cfg failed, addr 0x%02x, write value: 0x%04x\n",
            client->adapter->nr, client->addr, FW_UPG_RESTORE_CFG_ADDR, val);
        return ret;
    }
    return count;
}

static ssize_t phase_curr_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret;

    if ((attr->index < PHASE1_CURR_REG) || (attr->index > PHASE16_CURR_REG)) {
        DEBUG_ERROR("%d-%04x: invalid phase index: %d\n", client->adapter->nr,
            client->addr, attr->index);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    if (data->dev_available == WB_DEV_UNAVAILABLE_FLAG) {
        DEBUG_VERBOSE("%d-%04x: dev unavailable phase_curr_show failed\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return -EBUSY;
    }
        /* close write protect */
    ret = wb_pmbus_write_byte_data(client, 0, PMBUS_WRITE_PROTECT, WRITE_PROTECT_CLOSE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: close write protect failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    ret = wb_pmbus_write_byte_data(client, PHASE_CURR_PAGE, PMBUS_PHASE, attr->index);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: write phase_reg:0x%x failed, wr_value: 0x%x, ret: %d\n", client->adapter->nr,
            client->addr, PMBUS_PHASE, attr->index, ret);
        goto error;
    }

    ret = wb_pmbus_read_word_data(client, PHASE_CURR_PAGE, attr->index, PHASE_VAL_REG);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read phase_curr fail, phase_reg: 0x%x, ret: %d\n", client->adapter->nr,
            client->addr, attr->index, ret);
        goto error;
    }

    ret = (s16)(ret);

    DEBUG_VERBOSE("%d-%04x: read phase_curr success, reg: 0x%x, value: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, ret);
    /* open write protect */
    wb_pmbus_write_byte_data(client, 0, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", (ret * PER_LSB_100MA));
error:
    wb_pmbus_write_byte_data(client, 0, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    return ret;
}

static SENSOR_DEVICE_ATTR_RW(avs0_enable, isl68137_avs_enable, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_enable, isl68137_avs_enable, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout, isl68137_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, isl68137_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, pmbus_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, pmbus_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, pmbus_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, pmbus_avs_vout_min, 1);

static SENSOR_DEVICE_ATTR(phase1_curr, S_IRUGO, phase_curr_show, NULL, PHASE1_CURR_REG);
static SENSOR_DEVICE_ATTR(phase2_curr, S_IRUGO, phase_curr_show, NULL, PHASE2_CURR_REG);
static SENSOR_DEVICE_ATTR(phase3_curr, S_IRUGO, phase_curr_show, NULL, PHASE3_CURR_REG);
static SENSOR_DEVICE_ATTR(phase4_curr, S_IRUGO, phase_curr_show, NULL, PHASE4_CURR_REG);
static SENSOR_DEVICE_ATTR(phase5_curr, S_IRUGO, phase_curr_show, NULL, PHASE5_CURR_REG);
static SENSOR_DEVICE_ATTR(phase6_curr, S_IRUGO, phase_curr_show, NULL, PHASE6_CURR_REG);
static SENSOR_DEVICE_ATTR(phase7_curr, S_IRUGO, phase_curr_show, NULL, PHASE7_CURR_REG);
static SENSOR_DEVICE_ATTR(phase8_curr, S_IRUGO, phase_curr_show, NULL, PHASE8_CURR_REG);
static SENSOR_DEVICE_ATTR(phase9_curr, S_IRUGO, phase_curr_show, NULL, PHASE9_CURR_REG);
static SENSOR_DEVICE_ATTR(phase10_curr, S_IRUGO, phase_curr_show, NULL, PHASE10_CURR_REG);
static SENSOR_DEVICE_ATTR(phase11_curr, S_IRUGO, phase_curr_show, NULL, PHASE11_CURR_REG);
static SENSOR_DEVICE_ATTR(phase12_curr, S_IRUGO, phase_curr_show, NULL, PHASE12_CURR_REG);
static SENSOR_DEVICE_ATTR(phase13_curr, S_IRUGO, phase_curr_show, NULL, PHASE13_CURR_REG);
static SENSOR_DEVICE_ATTR(phase14_curr, S_IRUGO, phase_curr_show, NULL, PHASE14_CURR_REG);
static SENSOR_DEVICE_ATTR(phase15_curr, S_IRUGO, phase_curr_show, NULL, PHASE15_CURR_REG);
static SENSOR_DEVICE_ATTR(phase16_curr, S_IRUGO, phase_curr_show, NULL, PHASE16_CURR_REG);

static struct attribute *enable_attrs[] = {
    &sensor_dev_attr_avs0_enable.dev_attr.attr,
    &sensor_dev_attr_avs1_enable.dev_attr.attr,
    NULL,
};

static struct attribute *avs_ctrl_attrs[] = {
    &sensor_dev_attr_avs0_vout.dev_attr.attr,
    &sensor_dev_attr_avs1_vout.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_min.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_min.dev_attr.attr,
    NULL,
};

static struct attribute *phase_curr_attrs[] = {
    &sensor_dev_attr_phase1_curr.dev_attr.attr,
    &sensor_dev_attr_phase2_curr.dev_attr.attr,
    &sensor_dev_attr_phase3_curr.dev_attr.attr,
    &sensor_dev_attr_phase4_curr.dev_attr.attr,
    &sensor_dev_attr_phase5_curr.dev_attr.attr,
    &sensor_dev_attr_phase6_curr.dev_attr.attr,
    &sensor_dev_attr_phase7_curr.dev_attr.attr,
    &sensor_dev_attr_phase8_curr.dev_attr.attr,
    &sensor_dev_attr_phase9_curr.dev_attr.attr,
    &sensor_dev_attr_phase10_curr.dev_attr.attr,
    &sensor_dev_attr_phase11_curr.dev_attr.attr,
    &sensor_dev_attr_phase12_curr.dev_attr.attr,
    &sensor_dev_attr_phase13_curr.dev_attr.attr,
    &sensor_dev_attr_phase14_curr.dev_attr.attr,
    &sensor_dev_attr_phase15_curr.dev_attr.attr,
    &sensor_dev_attr_phase16_curr.dev_attr.attr,
    NULL,
};

static const struct attribute_group enable_group = {
    .attrs = enable_attrs,
};

static const struct attribute_group avs_ctrl_group = {
    .attrs = avs_ctrl_attrs,
};

static const struct attribute_group phase_curr_group = {
    .attrs = phase_curr_attrs,
};

static const struct attribute_group *isl68137_attribute_groups[] = {
    /* &enable_group, */
    &avs_ctrl_group,
    &phase_curr_group,
    NULL,
};

/******************** sysfs attr ***********************/
static SENSOR_DEVICE_ATTR(dfx_info, S_IRUGO, show_pmbus_dfx_info, NULL, -1);
static SENSOR_DEVICE_ATTR(dfx_info0, S_IRUGO, show_pmbus_dfx_info, NULL, 0);
static SENSOR_DEVICE_ATTR(dfx_info1, S_IRUGO, show_pmbus_dfx_info, NULL, 1);
static SENSOR_DEVICE_ATTR_RO(device_name, pmbus_device_name, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(device_id, pmbus_block_data_reverse, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(ic_device_rev, pmbus_block_data_reverse, PMBUS_IC_DEVICE_REV);
static SENSOR_DEVICE_ATTR_RO(mfr_date, pmbus_block_data_reverse, PMBUS_MFR_DATE);
static SENSOR_DEVICE_ATTR_RO(version, pmbus_block_data_reverse, PMBUS_MFR_DATE);


static SENSOR_DEVICE_ATTR_RW(dev_available, pmbus_dev_available, 0);
static SENSOR_DEVICE_ATTR_RO(status0_byte, pmbus_get_status_byte, 0);
static SENSOR_DEVICE_ATTR_RO(status1_byte, pmbus_get_status_byte, 1);
static SENSOR_DEVICE_ATTR_RO(status0_word, pmbus_get_status_word, 0);
static SENSOR_DEVICE_ATTR_RO(status1_word, pmbus_get_status_word, 1);
static SENSOR_DEVICE_ATTR(chip_crc, S_IRUGO, chip_crc_show, NULL, 0);
static SENSOR_DEVICE_ATTR(chip_nvm_time, S_IRUGO, chip_nvm_time_show, NULL, 0);
static SENSOR_DEVICE_ATTR(hex_file_type, S_IRUGO, hex_file_type_show, NULL, 0);
static SENSOR_DEVICE_ATTR(upg_status, S_IRUGO, upg_status_show, NULL, 0);
static SENSOR_DEVICE_ATTR(bank_status, S_IRUGO, bank_status_show, NULL, 0);
static SENSOR_DEVICE_ATTR(en_device_data, S_IRUGO | S_IWUSR, en_device_data_show, en_device_data_store, 0);
static SENSOR_DEVICE_ATTR(mcu_fault, S_IRUGO, mcu_fault_show, NULL, 0);
static SENSOR_DEVICE_ATTR(part_id, S_IRUGO, part_id_show, NULL, 0);
static SENSOR_DEVICE_ATTR(cycling_vcc, S_IWUSR, NULL, cycling_vcc_store, 0);
static SENSOR_DEVICE_ATTR(restore_cfg, S_IRUGO | S_IWUSR, restore_cfg_show, restore_cfg_store, 0);

static struct attribute *isl68137_sysfs_attrs[] = {
    &sensor_dev_attr_device_id.dev_attr.attr,
    &sensor_dev_attr_device_name.dev_attr.attr,
    &sensor_dev_attr_ic_device_rev.dev_attr.attr,
    &sensor_dev_attr_mfr_date.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_dfx_info.dev_attr.attr,
    &sensor_dev_attr_dfx_info0.dev_attr.attr,
    &sensor_dev_attr_dfx_info1.dev_attr.attr,
    &sensor_dev_attr_dev_available.dev_attr.attr,
    &sensor_dev_attr_chip_crc.dev_attr.attr,
    &sensor_dev_attr_chip_nvm_time.dev_attr.attr,
    &sensor_dev_attr_hex_file_type.dev_attr.attr,
    &sensor_dev_attr_upg_status.dev_attr.attr,
    &sensor_dev_attr_bank_status.dev_attr.attr,
    &sensor_dev_attr_en_device_data.dev_attr.attr,
    &sensor_dev_attr_mcu_fault.dev_attr.attr,
    &sensor_dev_attr_part_id.dev_attr.attr,
    &sensor_dev_attr_cycling_vcc.dev_attr.attr,
    &sensor_dev_attr_restore_cfg.dev_attr.attr,
    &sensor_dev_attr_status0_byte.dev_attr.attr,
    &sensor_dev_attr_status1_byte.dev_attr.attr,
    &sensor_dev_attr_status0_word.dev_attr.attr,
    &sensor_dev_attr_status1_word.dev_attr.attr,
    NULL,
};

static const struct attribute_group isl68137_sysfs_attrs_group = {
    .attrs = isl68137_sysfs_attrs,
};

static int raa_dmpvr2_read_word_data(struct i2c_client *client, int page,
                     int phase, int reg)
{
    int ret;

    switch (reg) {
    case PMBUS_VIRT_READ_VMON:
        ret = wb_pmbus_read_word_data(client, page, phase,
                       RAA_DMPVR2_READ_VMON);
        break;
    default:
        ret = -ENODATA;
        break;
    }

    return ret;
}

static struct pmbus_driver_info raa_dmpvr_info = {
    .pages = 3,
    .format[PSC_VOLTAGE_IN] = direct,
    .format[PSC_VOLTAGE_OUT] = direct,
    .format[PSC_CURRENT_IN] = direct,
    .format[PSC_CURRENT_OUT] = direct,
    .format[PSC_POWER] = direct,
    .format[PSC_TEMPERATURE] = direct,
    .m[PSC_VOLTAGE_IN] = 1,
    .b[PSC_VOLTAGE_IN] = 0,
    .R[PSC_VOLTAGE_IN] = 2,
    .m[PSC_VOLTAGE_OUT] = 1,
    .b[PSC_VOLTAGE_OUT] = 0,
    .R[PSC_VOLTAGE_OUT] = 3,
    .m[PSC_CURRENT_IN] = 1,
    .b[PSC_CURRENT_IN] = 0,
    .R[PSC_CURRENT_IN] = 2,
    .m[PSC_CURRENT_OUT] = 1,
    .b[PSC_CURRENT_OUT] = 0,
    .R[PSC_CURRENT_OUT] = 1,
    .m[PSC_POWER] = 1,
    .b[PSC_POWER] = 0,
    .R[PSC_POWER] = 0,
    .m[PSC_TEMPERATURE] = 1,
    .b[PSC_TEMPERATURE] = 0,
    .R[PSC_TEMPERATURE] = 0,
    .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
        | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT
        | PMBUS_HAVE_VMON,
    .func[1] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
        | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
        | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
    .func[2] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
        | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
        | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
};

static loff_t renesas_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;

    switch (origin) {
    case SEEK_SET:        /* Offset relative to the beginning of the file. */
        if (offset < 0 || offset > 0xff) {
            DEBUG_VERBOSE("SEEK_SET, offset: %lld, invalid.\n", offset);
            ret = -EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_VERBOSE("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

static ssize_t renesas_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    u8 reg_offset;
    int ret;
    struct pmbus_data *data;
    struct i2c_client *client;
    renesas_info_t *renesas_info;
    char block_data[I2C_SMBUS_BLOCK_MAX + 2] = { 0 };

    if ((file == NULL) || (buf == NULL) || (offset == NULL)) {
        DEBUG_ERROR("Invalid param, read failed\n");
        return -EINVAL;
    }

    if ((*offset < 0) || (*offset > 0xff)) {
        DEBUG_ERROR("Invalid offset: %lld.\n", *offset);
        return -EINVAL;
    }

    if ((count == 0) ||(count > I2C_SMBUS_BLOCK_MAX)) {
        DEBUG_ERROR("Invalid read conut %zu\n", count);
        return -EINVAL;
    }

    renesas_info = file->private_data;
    if (renesas_info == NULL) {
        DEBUG_ERROR("Invalid renesas_info, renesas_info is NULL\n");
        return -EINVAL;
    }
    client = renesas_info->client;
    if (client == NULL) {
        DEBUG_ERROR("Invalid i2c client, i2c client is NULL\n");
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    reg_offset = (*offset) & 0xff;
    DEBUG_VERBOSE("%d-%04x: reg offset: 0x%x read count %zu\n",
        client->adapter->nr, client->addr, reg_offset, count);

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_read_i2c_block_data(client, 0, reg_offset, count, block_data);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: reg offset: 0x%x read count %zu, read failed, ret: %d\n",
            client->adapter->nr, client->addr, reg_offset, count, ret);
        return ret;
    }

    if (copy_to_user(buf, block_data, count)) {
        DEBUG_ERROR("copy_to_user error\n");
        return -EFAULT;
    }

    *offset += count;
    return count;
}

static ssize_t renesas_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    u8 reg_offset;
    int ret, i;
    struct pmbus_data *data;
    struct i2c_client *client;
    renesas_info_t *renesas_info;
    char block_data[I2C_SMBUS_BLOCK_MAX + 2] = { 0 };

    if ((file == NULL) || (buf == NULL) || (offset == NULL)) {
        DEBUG_ERROR("Invalid param, write failed\n");
        return -EINVAL;
    }

    if ((*offset < 0) || (*offset > 0xff)) {
        DEBUG_ERROR("Invalid offset: %lld.\n", *offset);
        return -EINVAL;
    }

    if ((count == 0) ||(count > I2C_SMBUS_BLOCK_MAX)) {
        DEBUG_ERROR("Invalid write conut %zu\n", count);
        return -EINVAL;
    }

    renesas_info = file->private_data;
    if (renesas_info == NULL) {
        DEBUG_ERROR("Invalid renesas_info, renesas_info is NULL\n");
        return -EINVAL;
    }
    client = renesas_info->client;
    if (client == NULL) {
        DEBUG_ERROR("Invalid i2c client, i2c client is NULL\n");
        return -EINVAL;
    }

    if (copy_from_user(block_data, buf, count)) {
        DEBUG_ERROR("copy_from_user failed.\n");
        return -EFAULT;
    }

    data = i2c_get_clientdata(client);
    reg_offset = (*offset) & 0xff;
    DEBUG_VERBOSE("%d-%04x: reg offset: 0x%x write count %zu\n",
        client->adapter->nr, client->addr, reg_offset, count);
    for (i = 0; i < count; i++) {
        DEBUG_VERBOSE("buf[%d] : 0x%02x\n", i, block_data[i]);
    }

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_write_i2c_block_data(client, 0, reg_offset, count, block_data);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: reg offset: 0x%x write count %zu, write failed, ret: %d\n",
            client->adapter->nr, client->addr, reg_offset, count, ret);
        return ret;
    }

    *offset += count;
    return count;
}

static long renesas_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int minor_to_dev(int minor, renesas_info_t **renesas_dev)
{
    int i;

    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (renesas_dev_arry[i] == NULL) {
            continue;
        }
        if (renesas_dev_arry[i]->misc_dev.minor == minor) {
            *renesas_dev = renesas_dev_arry[i];
            return 0;
        }
    }
    return -ENODEV;
}

static int add_dev_to_g_dev_list(renesas_info_t *renesas_dev)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (renesas_dev_arry[i] == NULL) {
            renesas_dev_arry[i] = renesas_dev;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -EBUSY;
}

static int remove_dev_from_g_dev_list(int minor)
{
    int i;
    unsigned long flags;

    spin_lock_irqsave(&dev_array_lock, flags);
    for (i = 0; i < MAX_DEV_NUM; i++) {
        if (renesas_dev_arry[i] == NULL) {
            continue;
        }
        if (renesas_dev_arry[i]->misc_dev.minor == minor) {
            renesas_dev_arry[i] = NULL;
            spin_unlock_irqrestore(&dev_array_lock, flags);
            return 0;
        }
    }
    spin_unlock_irqrestore(&dev_array_lock, flags);
    return -ENODEV ;
}

static int renesas_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    renesas_info_t *renesas_info;
    int ret;

    DEBUG_VERBOSE("inode: %p, file: %p, minor: %u", inode, file, minor);

    ret = minor_to_dev(minor, &renesas_info);
    if (ret) {
        return ret;
    }
    file->private_data = renesas_info;
    return 0;
}

static int renesas_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static const struct file_operations renesas_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = renesas_dev_llseek,
    .read           = renesas_dev_read,
    .write          = renesas_dev_write,
    .unlocked_ioctl = renesas_dev_ioctl,
    .open           = renesas_dev_open,
    .release        = renesas_dev_release,
};

static int isl68137_probe(struct i2c_client *client)
{
    struct pmbus_driver_info *info;
    struct pmbus_data *data;
    renesas_info_t *renesas_info;
    int status;
    struct miscdevice *misc;

    renesas_info = devm_kzalloc(&client->dev, sizeof(renesas_info_t), GFP_KERNEL);
    if (!renesas_info) {
        dev_err(&client->dev, "devm_kzalloc renesas_info error.\n");
        return -ENOMEM;
    }
    renesas_info->client = client;
    info = &renesas_info->info;

    memcpy(info, &raa_dmpvr_info, sizeof(*info));

    switch (i2c_match_id(raa_dmpvr_id, client)->driver_data) {
    case raa_dmpvr1_2rail:
        info->pages = 2;
        info->R[PSC_VOLTAGE_IN] = 3;
        info->func[0] &= ~PMBUS_HAVE_VMON;
        info->func[1] = PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
            | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
            | PMBUS_HAVE_POUT;
        break;
    case raa_dmpvr2_1rail:
        info->pages = 1;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_2rail_nontc:
        info->func[0] &= ~PMBUS_HAVE_TEMP3;
        info->func[1] &= ~PMBUS_HAVE_TEMP3;
        fallthrough;
    case raa_dmpvr2_2rail:
        info->pages = 2;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_3rail:
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_hv:
        info->pages = 1;
        info->R[PSC_VOLTAGE_IN] = 1;
        info->m[PSC_VOLTAGE_OUT] = 2;
        info->R[PSC_VOLTAGE_OUT] = 2;
        info->m[PSC_CURRENT_IN] = 2;
        info->m[PSC_POWER] = 2;
        info->R[PSC_POWER] = -1;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    default:
        return -ENODEV;
    }
    info->groups = isl68137_attribute_groups;

    status = wb_pmbus_do_probe(client, info);
    if (status != 0) {
        dev_info(&client->dev, "isl68137 pmbus probe error %d\n", status);
        return status;
    }

    data = i2c_get_clientdata(client);
    data->pmbus_info_array = isl68137_dfx_infos;
    data->pmbus_info_array_size = ARRAY_SIZE(isl68137_dfx_infos);

    status = sysfs_create_group(&client->dev.kobj, &isl68137_sysfs_attrs_group);
    if (status != 0) {
        dev_info(&client->dev, "Create isl68137 sysfs failed, status: %d\n", status);
        wb_pmbus_do_remove(client);
        return status;
    }

    snprintf(renesas_info->misc_dev_name, sizeof(renesas_info->misc_dev_name), "renesas_%d_0x%02x",
        client->adapter->nr, client->addr);
    misc = &renesas_info->misc_dev;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = renesas_info->misc_dev_name;
    misc->fops = &renesas_dev_fops;
    if (misc_register(misc) != 0) {
        dev_err(&client->dev, "Failed to register %s\n", misc->name);
        sysfs_remove_group(&client->dev.kobj, &isl68137_sysfs_attrs_group);
        wb_pmbus_do_remove(client);
        return -ENXIO;
    }

    status = add_dev_to_g_dev_list(renesas_info);
    if (status) {
        dev_err(&client->dev, "Failed to add_dev_to_g_dev_list, ret: %d\n", status);
        misc_deregister(misc);
        sysfs_remove_group(&client->dev.kobj, &isl68137_sysfs_attrs_group);
        wb_pmbus_do_remove(client);
        return -EINVAL;
    }

    renesas_fw_upg_init(renesas_info);

    dev_info(&client->dev, "Register %s with minor: %d success\n", misc->name, misc->minor);
    return status;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int isl68137_remove(struct i2c_client *client)
#else
static void isl68137_remove(struct i2c_client *client)
#endif
{
    int ret, minor;
    renesas_info_t *renesas_info = to_renesas_data(wb_pmbus_get_driver_info(client));

    DEBUG_VERBOSE("bus: %d, addr: 0x%02x do remove\n", client->adapter->nr, client->addr);

    minor = renesas_info->misc_dev.minor;

    DEBUG_VERBOSE("misc_deregister %s, minor: %d\n", renesas_info->misc_dev.name, minor);
    misc_deregister(&renesas_info->misc_dev);
    remove_dev_from_g_dev_list(minor);


    sysfs_remove_group(&client->dev.kobj, &isl68137_sysfs_attrs_group);
    ret = wb_pmbus_do_remove(client);
    if (ret != 0){
        dev_err(&client->dev, "Failed to remove isl68137 pmbus, ret: %d\n", ret);
    } else {
        dev_info(&client->dev, "Remove renesas pmbus device success.\n");
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return ret;
#endif
}

static const struct i2c_device_id raa_dmpvr_id[] = {
    {"wb_isl68127", raa_dmpvr1_2rail},
    {"wb_isl68137", raa_dmpvr1_2rail},
    {"wb_isl68220", raa_dmpvr2_2rail},
    {"wb_isl68221", raa_dmpvr2_3rail},
    {"wb_isl68222", raa_dmpvr2_2rail},
    {"wb_isl68223", raa_dmpvr2_2rail},
    {"wb_isl68224", raa_dmpvr2_3rail},
    {"wb_isl68225", raa_dmpvr2_2rail},
    {"wb_isl68226", raa_dmpvr2_3rail},
    {"wb_isl68227", raa_dmpvr2_1rail},
    {"wb_isl68229", raa_dmpvr2_3rail},
    {"wb_isl68233", raa_dmpvr2_2rail},
    {"wb_isl68239", raa_dmpvr2_3rail},

    {"wb_isl69222", raa_dmpvr2_2rail},
    {"wb_isl69223", raa_dmpvr2_3rail},
    {"wb_isl69224", raa_dmpvr2_2rail},
    {"wb_isl69225", raa_dmpvr2_2rail},
    {"wb_isl69227", raa_dmpvr2_3rail},
    {"wb_isl69228", raa_dmpvr2_3rail},
    {"wb_isl69234", raa_dmpvr2_2rail},
    {"wb_isl69236", raa_dmpvr2_2rail},
    {"wb_isl69239", raa_dmpvr2_3rail},
    {"wb_isl69242", raa_dmpvr2_2rail},
    {"wb_isl69243", raa_dmpvr2_1rail},
    {"wb_isl69247", raa_dmpvr2_2rail},
    {"wb_isl69248", raa_dmpvr2_2rail},
    {"wb_isl69254", raa_dmpvr2_2rail},
    {"wb_isl69255", raa_dmpvr2_2rail},
    {"wb_isl69256", raa_dmpvr2_2rail},
    {"wb_isl69259", raa_dmpvr2_2rail},
    {"wb_isl69260", raa_dmpvr2_2rail},
    {"wb_isl69268", raa_dmpvr2_2rail},
    {"wb_isl69269", raa_dmpvr2_3rail},
    {"wb_isl69298", raa_dmpvr2_2rail},

    {"wb_raa228000", raa_dmpvr2_hv},
    {"wb_raa228004", raa_dmpvr2_hv},
    {"wb_raa228006", raa_dmpvr2_hv},
    {"wb_raa228228", raa_dmpvr2_2rail_nontc},
    {"wb_raa229001", raa_dmpvr2_2rail},
    {"wb_raa229004", raa_dmpvr2_2rail},
    {}
};

MODULE_DEVICE_TABLE(i2c, raa_dmpvr_id);

/* This is the driver that will be inserted */
static struct i2c_driver isl68137_driver = {
    .driver = {
           .name = "wb_isl68137",
           },
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    .probe = isl68137_probe,
#else
    .probe_new = isl68137_probe,
#endif
    .remove = isl68137_remove,
    .id_table = raa_dmpvr_id,
};

module_i2c_driver(isl68137_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for Renesas digital multiphase voltage regulators");
MODULE_LICENSE("GPL");
