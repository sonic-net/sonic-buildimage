#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <wb_bsp_kernel_debug.h>

#define NCS23322_PAGE_REG                   (0x01)
#define NCS23322_OUT_CTRL_EN                 (0x0001)
#define NCS23322_OUT_CTRL_DIS                (0x0002)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define DESIGNED_ID_SIZE                (8)

#define SYSCLK_UNLOCK_OFFSET            (0)
#define SYSCLK_UNLOCK_FLG_OFFSET        (1)
#define EFUSE_ERR_OFFSEET               (0)
#define FW_ERR_OFFSEET                  (4)
#define RAM_ERR_FLG_OFFSEET             (0)
#define DFX_EN_OFFSET                   (0)

enum chips {
    ncs23322
};

struct ncs23322_data {
    struct i2c_client   *client;
    struct mutex        update_lock;
    struct attribute_group *sysfs_group;
    int page_reg;
};

static int ncs23322_set_page(struct i2c_client *client, int page)
{
    struct ncs23322_data *data = i2c_get_clientdata(client);
    int rv;

    rv = i2c_smbus_write_byte_data(client, data->page_reg, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 wirte page failed page_reg 0x%x target page %d, errno: %d\n", data->page_reg, page, rv);
        return rv;
    }

    rv = i2c_smbus_read_byte_data(client, data->page_reg);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read page failed page_reg 0x%x target page %d, errno: %d\n", data->page_reg, page, rv);
        return rv;
    }

    if (rv != page) {
        DEBUG_ERROR("ncs23322 current page %d != target page %d \n", rv, page);
        return -EIO;
    }
    DEBUG_INFO("ncs23322 set to page: %d success \n", page);
    return 0;
}

static s32 ncs23322_read_bit_data(struct i2c_client *client, int page, u8 reg, int bit)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read bit data set page%d failed, rv: %d\n", page, rv);
        return rv;
    }

    rv = i2c_smbus_read_byte_data(client, reg);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read bit data read byte data failed, reg:0x%x, rv: %d\n", reg, rv);
        return rv;
    }

    return ((rv >> bit) & 0x1);
}

#if 0
static u8 ncs23322_read_byte_data(struct i2c_client *client, int page, u8 reg)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_read_byte_data(client, reg);
}
#endif

static s32 ncs23322_read_word_data(struct i2c_client *client, int page, u8 reg)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read word setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_read_word_data(client, reg);
}

static s32 ncs23322_read_i2c_block_data(struct i2c_client *client, int page, u8 reg, u8 length,
            u8* values)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read word setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_read_i2c_block_data(client, reg, length, values);
}

static s32 ncs23322_write_bit_data(struct i2c_client *client, int page, u8 reg, u8 value, int bit)
{
    int rv;
    u8 ori_value;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read word setting page failed errno: %d\n", rv);
        return rv;
    }

    ori_value = i2c_smbus_read_byte_data(client, reg);
    if (value) {
        ori_value |= (1 << bit);
    } else {
        ori_value &= ~(1 << bit);
    }

    return i2c_smbus_write_byte_data(client, reg, ori_value);
}

#if 0
static int ncs23322_write_byte_data(struct i2c_client *client, int page, u8 reg, u8 value)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 write byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_write_byte_data(client, reg, value);
}
#endif

static int ncs23322_write_word_data(struct i2c_client *client, int page, u8 reg, u16 value)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 write byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_write_word_data(client, reg, value);
}

static s32 ncs23322_write_i2c_block_data(struct i2c_client *client, int page, u8 reg, u8 length,
            u8* values)
{
    int rv;

    rv = ncs23322_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("ncs23322 read word setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_write_i2c_block_data(client, reg, length, values);
}

static ssize_t ncs23322_general_word_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    s32 status;

    status = -1;
    mutex_lock(&data->update_lock);
    status = ncs23322_read_word_data(client, page, reg);
    if (status < 0) {
        DEBUG_ERROR("ncs23322 show value failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }
    DEBUG_INFO("ncs23322 show value success page [%d] reg[0x%x] read value[0x%x]\n", page,
      reg, status);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%x\n", status & 0xffff);
}

static ssize_t ncs23322_general_word_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    u16 val;
    int ret;

    val = 0;
    ret = kstrtou16(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    DEBUG_INFO("buf: %s, val = 0x%x\n", buf, val);

    mutex_lock(&data->update_lock);
    ret = ncs23322_write_word_data(client, page, reg, val);
    if (ret < 0) {
        DEBUG_ERROR("ncs23322 set value failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page,
            reg, val, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_INFO("ncs23322 set value success page [%d] reg[0x%x] value[0x%x]\n", page, reg, val);
    mutex_unlock(&data->update_lock);

    return count;
}

#if 0
static ssize_t ncs23322_general_byte_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    s32 status;

    status = -1;
    mutex_lock(&data->update_lock);
    status = ncs23322_read_byte_data(client, page, reg);
    if (status < 0) {
        DEBUG_ERROR("ncs23322 show value failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }
    DEBUG_INFO("ncs23322 show value success page [%d] reg[0x%x] read value[0x%x]\n", page, reg,
        status);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%x\n", status & 0xff);
}

static ssize_t ncs23322_general_byte_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    u8 val;
    int ret;

    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    DEBUG_INFO("buf: %s, val = 0x%x\n", buf, val);

    mutex_lock(&data->update_lock);
    ret = ncs23322_write_byte_data(client, page, reg, val);
    if (ret < 0) {
        DEBUG_ERROR("ncs23322 set value failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page,
            reg, val, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_INFO("ncs23322 set value success page [%d] reg[0x%x] value[0x%x]\n", page, reg, val);
    mutex_unlock(&data->update_lock);

    return count;
}
#endif

static ssize_t ncs23322_general_bit_show(struct device *dev, struct device_attribute *da,
                char *buf, int bit)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    int status;

    mutex_lock(&data->update_lock);
    status = ncs23322_read_bit_data(client, page, reg, bit);
    if (status < 0) {
        DEBUG_ERROR("ncs23322 show value failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }

    DEBUG_INFO("ncs23322 show value success page [%d] reg[0x%x] read value[0x%x]\n", page, reg, status);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%x\n", status & 0xff);
}

static ssize_t ncs23322_general_bit_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count, int bit)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    u8 val;
    int ret;

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    DEBUG_INFO("buf: %s, val = 0x%x\n", buf, val);

    mutex_lock(&data->update_lock);
    ret = ncs23322_write_bit_data(client, page, reg, val, bit);
    if (ret < 0) {
        DEBUG_ERROR("ncs23322 set value failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page,
            reg, val, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_INFO("ncs23322 set value success page [%d] reg[0x%x] value[0x%x]\n", page, reg, val);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t ncs23322_design_id_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    int i, ret;
    u8 values[DESIGNED_ID_SIZE];
    u64 val, tmp;

    memset(values, 0, sizeof(values));

    mutex_lock(&data->update_lock);
    ret = ncs23322_read_i2c_block_data(client, page, reg, DESIGNED_ID_SIZE, values);
    if (ret < 0) {
        DEBUG_ERROR("ncs23322 read i2c block data failed page [%d] reg [0x%x], errno: %d\n", page, reg, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    mutex_unlock(&data->update_lock);
    val = 0ull;
    for (i = 0; i < DESIGNED_ID_SIZE; i++) {
        tmp = (u64)values[i];
        val |= (tmp & 0xff) << ((DESIGNED_ID_SIZE - i - 1) * 8);
    }

    return snprintf(buf, PAGE_SIZE, "0x%llx\n", val);
}

static ssize_t ncs23322_design_id_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    struct ncs23322_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    int i, ret;
    u8 values[DESIGNED_ID_SIZE];
    u64 val;

    val = 0;
    ret = kstrtou64(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }
    DEBUG_INFO("buf: %s, val = 0x%llx\n", buf, val);

    for (i = 0; i < DESIGNED_ID_SIZE; i++) {
        values[DESIGNED_ID_SIZE - i - 1] = (u8)((val >> (i * 8)) & 0xff);
    }
    mutex_lock(&data->update_lock);
    ret = ncs23322_write_i2c_block_data(client, page, reg, DESIGNED_ID_SIZE, values);
    if (ret < 0) {
        DEBUG_ERROR("ncs23322 write i2c block data failed page [%d] reg [0x%x], errno: %d\n",
            page, reg, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    mutex_unlock(&data->update_lock);
    return count;
}

static ssize_t ncs23322_sysclk_unlock_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, SYSCLK_UNLOCK_OFFSET);
}

static ssize_t ncs23322_sysclk_unlock_flg_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, SYSCLK_UNLOCK_FLG_OFFSET);
}

static ssize_t ncs23322_sysclk_unlock_flg_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    return ncs23322_general_bit_store(dev, da, buf, count, SYSCLK_UNLOCK_FLG_OFFSET);
}

static ssize_t ncs23322_efuse_err_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, EFUSE_ERR_OFFSEET);
}

static ssize_t ncs23322_fw_err_show(struct device *dev, struct device_attribute *da, char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, FW_ERR_OFFSEET);
}

static ssize_t ncs23322_ram_err_flg_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, RAM_ERR_FLG_OFFSEET);
}

static ssize_t ncs23322_ram_err_flg_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    return ncs23322_general_bit_store(dev, da, buf, count, RAM_ERR_FLG_OFFSEET);
}

static ssize_t ncs23322_dfx_en_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ncs23322_general_bit_show(dev, da, buf, DFX_EN_OFFSET);
}

static ssize_t ncs23322_dfx_en_store(struct device *dev, struct device_attribute *da,
                const char *buf, size_t count)
{
    return ncs23322_general_bit_store(dev, da, buf, count, DFX_EN_OFFSET);
}

/* param _nr is page , param _index is reg addr */
static SENSOR_DEVICE_ATTR_2_RO(chip_id, ncs23322_general_word, 0x0, 0x02);
static SENSOR_DEVICE_ATTR_2_RO(ver_id, ncs23322_general_word, 0x0, 0x0c);
static SENSOR_DEVICE_ATTR_2_RW(config_id, ncs23322_general_word, 0x0, 0xd0);
static SENSOR_DEVICE_ATTR_2_RW(design_id, ncs23322_design_id, 0x0, 0xd4);
static SENSOR_DEVICE_ATTR_2_RO(sysclk_unlock, ncs23322_sysclk_unlock, 0x32, 0x20);
static SENSOR_DEVICE_ATTR_2_RW(sysclk_unlock_flg, ncs23322_sysclk_unlock_flg, 0x32, 0x21);
static SENSOR_DEVICE_ATTR_2_RO(efuse_err, ncs23322_efuse_err, 0x31, 0x0b);
static SENSOR_DEVICE_ATTR_2_RO(fw_err, ncs23322_fw_err, 0x31, 0x0b);
static SENSOR_DEVICE_ATTR_2_RW(ram_err_flg, ncs23322_ram_err_flg, 0x32, 0x1f);
static SENSOR_DEVICE_ATTR_2_RW(dfx_en, ncs23322_dfx_en, 0x32, 0x04);
static SENSOR_DEVICE_ATTR_2_RW(reg_0x3204, ncs23322_general_word, 0x32, 0x04);
static SENSOR_DEVICE_ATTR_2_RW(reg_0x3220, ncs23322_general_word, 0x32, 0x20);
static SENSOR_DEVICE_ATTR_2_RW(reg_0x3221, ncs23322_general_word, 0x32, 0x21);
static SENSOR_DEVICE_ATTR_2_RW(reg_0x310b, ncs23322_general_word, 0x31, 0x0b);
static SENSOR_DEVICE_ATTR_2_RW(reg_0x321f, ncs23322_general_word, 0x32, 0x1f);

static struct attribute *ncs23322_sysfs_attrs[] = {
    &sensor_dev_attr_chip_id.dev_attr.attr,
    &sensor_dev_attr_ver_id.dev_attr.attr,
    &sensor_dev_attr_config_id.dev_attr.attr,
    &sensor_dev_attr_design_id.dev_attr.attr,
    &sensor_dev_attr_sysclk_unlock.dev_attr.attr,
    &sensor_dev_attr_sysclk_unlock_flg.dev_attr.attr,
    &sensor_dev_attr_efuse_err.dev_attr.attr,
    &sensor_dev_attr_fw_err.dev_attr.attr,
    &sensor_dev_attr_ram_err_flg.dev_attr.attr,
    &sensor_dev_attr_dfx_en.dev_attr.attr,
    &sensor_dev_attr_reg_0x3204.dev_attr.attr,
    &sensor_dev_attr_reg_0x3220.dev_attr.attr,
    &sensor_dev_attr_reg_0x3221.dev_attr.attr,
    &sensor_dev_attr_reg_0x310b.dev_attr.attr,
    &sensor_dev_attr_reg_0x321f.dev_attr.attr,
    NULL
};

static struct attribute_group  ncs23322_sysfs_group = {
    .attrs = ncs23322_sysfs_attrs,
};

static int ncs23322_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct ncs23322_data *data;
    int ret;

    dev_dbg(&client->dev, "ncs23322 enter probe\n");
    data = devm_kzalloc(&client->dev, sizeof(struct ncs23322_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "ncs23322 alloc memory failed\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    switch (id->driver_data) {
    case ncs23322:
        data->sysfs_group = &ncs23322_sysfs_group;
        data->page_reg = NCS23322_PAGE_REG;
        break;
    default:
        dev_err(&client->dev, "Unknown chip id %ld\n", id->driver_data);
        return -ENODEV;
    }

    ret = sysfs_create_group(&client->dev.kobj, data->sysfs_group);
    if (ret < 0) {
        dev_err(&client->dev, "ncs23322 sysfs_create_group failed %d\n", ret);
    }
    return 0;
}

static void ncs23322_remove(struct i2c_client *client)
{
    struct ncs23322_data *data = i2c_get_clientdata(client);

    if (data->sysfs_group) {
        dev_info(&client->dev, "ncs23322 unregister sysfs group\n");
        sysfs_remove_group(&client->dev.kobj, (const struct attribute_group *)data->sysfs_group);
    }
    dev_info(&client->dev, "ncs23322 removed\n");
    return;
}

static const struct i2c_device_id ncs23322_id[] = {
    { "wb_ncs23322", ncs23322 },
    {}
};
MODULE_DEVICE_TABLE(i2c, ncs23322_id);

static struct i2c_driver wb_ncs23322_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "wb_ncs23322",
    },
    .probe      = ncs23322_probe,
    .remove     = ncs23322_remove,
    .id_table   = ncs23322_id,
};

module_i2c_driver(wb_ncs23322_driver);
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("ncs23322 driver");
MODULE_LICENSE("GPL");
