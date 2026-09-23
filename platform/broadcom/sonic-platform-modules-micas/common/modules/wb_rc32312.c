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

#define RC32312_PAGE_REG                    (0xfd)
#define RC32312_APLL_EVENT_REG              (0x6e)
#define RC32312_APLL_CLEAR_VAL              (0x03)
#define RC32312_APLL_CLEAR_MASK             (0x03)
#define RC32312_APLL_LOL_EVENT_REG          (0x6f)
#define RC32312_APLL_LOL_EVENT_CLEAR_VAL    (0x0)
#define RC32312_APLL_LOL_EVENT_CLEAR_MASK   (0x0f)
#define RC32312_APLL_EVENT_CLEAR_VAL        (0x7f)
#define RC32312_APLL_EVENT_CLEAR_MASK       (0x7f)
#define RC32312_APLL_EXT_EVENT_REG          (0x70)
#define RC32312_XTAL_LOS_CNT_REG            (0x79)

#define RC32312_APLL_PAGE                   (2)
#define RC32312_APLL_STS_REG                (0x71)
#define RC32312_OUT_CTRL_EN                 (0x0001)
#define RC32312_OUT_CTRL_DIS                (0x0002)

#define RC32312_XTAL_LOS_PAGE               (1)
#define RC32312_XTAL_LOS_EVT_REG            (0x78)
#define RC32312_XTAL_LOS_STS_REG            (0x7a)

#define RC32312_CLOCK_REG_INFO_MASK         (0x1)
#define RC32312_CLOCK_FAULT_XTAL            (0)
#define RC32312_CLOCK_FAULT_BUS             (1)
#define RC32312_CLOCK_FAULT_APLL            (4)

#define RC32312_VENDOR_ID_PAGE              (0)
#define RC32312_VENDOR_ID_REG               (0x0)
#define RC32312_VENDOR_ID                   (0x1033)

#define RC32312_CLOCK_STATUS_NORMAL         (0)
#define RC32312_CLOCK_STATUS_ABNORMAL       (1)

#define RC21012_LOSMON_PAGE                 (0)
#define RC21012_LOSMON4_STS_REG             (0x99)
#define RC21012_LOSMON4_CNT_REG             (0x9b)
#define RC21012_LOSMON4_CNT_CLEAR_VAL       (0x0)
#define RC21012_LOSMON4_CNT_CLEAR_MASK      (0x0f)

#define RC21012_APLL_PAGE                   (1)
#define RC21012_APLL_STS_REG                (0x3f)
#define RC21012_APLL_LOL_CNT_REG            (0x41)
#define RC21012_APLL_LOL_CNT_CLEAR_VAL      (0x0)
#define RC21012_APLL_LOL_CNT_CLEAR_MASK     (0x0f)

/* RC38112 register definitions */
#define RC38112_XTAL_PAGE                   (8)     /* Page number for RC38112 registers */
#define RC38112_XTAL_LOS_EVT_REG            (0x08)
#define RC38112_XTAL_LOS_EVT_CLEAR_VAL      (0x3)
#define RC38112_XTAL_LOS_EVT_CLEAR_MASK     (0x3)

#define RC38112_XTAL_LOS_CNT_REG            (0x09)
#define RC38112_XTAL_LOS_CNT_CLEAR_VAL      (0x0)
#define RC38112_XTAL_LOS_CNT_CLEAR_MASK     (0x0f)

#define RC38112_APLL_EVENT_REG              (0xba)
#define RC38112_APLL_EVENT_CLEAR_VAL        (0x7f)
#define RC38112_APLL_EVENT_CLEAR_MASK       (0x7f)

#define RC38112_APLL_LOL_EVENT_REG          (0xbb)
#define RC38112_APLL_LOL_EVENT_CLEAR_VAL    (0x0)
#define RC38112_APLL_LOL_EVENT_CLEAR_MASK   (0x0f)

#define RC38112_APLL_LOL_CLEAR_REG          (0xbc)
#define RC38112_APLL_LOL_CLEAR_CLEAR_VAL    (0x1)
#define RC38112_APLL_LOL_CLEAR_CLEAR_MASK   (0x0f)

#define RC38112_APLL_STS_REG                (0xbd)
#define RC38112_APLL_STS_MASK               (0x1)

#define RC38112_XTAL_LOS_STS_REG            (0x0a)
#define RC38112_XTAL_LOS_STS_MASK           (0x1)

#define RC38112_APLL_PAGE                   (0)     /* Page number for RC38112 registers */

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

enum attrs {
    APLL_STATUS,
    APLL_COUNT,
    APLL_EVENT,
    XTAL_LOS_EVT,
    XTAL_LOS_CNT,
    XTAL_LOS_STS,
    LOSMON4_STS,
    LOSMON4_CNT,
};

typedef struct {
    int attr;
    int page;
    int reg;
} rc32312_attr_match_t;

rc32312_attr_match_t rc21012_attr_infos[] = {
    {APLL_STATUS, RC21012_APLL_PAGE, RC21012_APLL_STS_REG},
    {APLL_COUNT, RC21012_APLL_PAGE, RC21012_APLL_LOL_CNT_REG},
    {LOSMON4_STS, RC21012_LOSMON_PAGE, RC21012_LOSMON4_STS_REG},
    {LOSMON4_CNT, RC21012_LOSMON_PAGE, RC21012_LOSMON4_CNT_REG},
};

rc32312_attr_match_t rc32312_attr_infos[] = {
    {APLL_STATUS, RC32312_APLL_PAGE, RC32312_APLL_STS_REG},
    {APLL_COUNT, RC32312_APLL_PAGE, RC32312_APLL_LOL_EVENT_REG},
    {APLL_EVENT, RC32312_APLL_PAGE, RC32312_APLL_EVENT_REG},
    {XTAL_LOS_EVT, RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_EVT_REG},
    {XTAL_LOS_CNT, RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_CNT_REG},
    {XTAL_LOS_STS, RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_STS_REG},
};

rc32312_attr_match_t rc38112_attr_infos[] = {
    {APLL_STATUS, RC38112_APLL_PAGE, RC38112_APLL_STS_REG},
    {APLL_COUNT, RC38112_APLL_PAGE, RC38112_APLL_LOL_EVENT_REG},
    {APLL_EVENT, RC38112_APLL_PAGE, RC38112_APLL_EVENT_REG},
    {XTAL_LOS_EVT, RC38112_XTAL_PAGE, RC38112_XTAL_LOS_EVT_REG},
    {XTAL_LOS_CNT, RC38112_XTAL_PAGE, RC38112_XTAL_LOS_CNT_REG},
    {XTAL_LOS_STS, RC38112_XTAL_PAGE, RC38112_XTAL_LOS_STS_REG},
};

enum chips {
    rc32312,
    rc21012,
    rc38112,
};

struct rc32312_data {
    struct i2c_client   *client;
    struct mutex        update_lock;
    struct attribute_group *sysfs_group;
    int page_reg;
    int chip;
    int vendor_id;
};

struct rc32312_clear_op {
    int page;
    u8 reg;
    u8 val;
    u8 mask;
};

static const struct rc32312_clear_op rc32312_clear_ops[] = {
    { RC32312_APLL_PAGE, RC32312_APLL_EVENT_REG, RC32312_APLL_EVENT_CLEAR_VAL, RC32312_APLL_EVENT_CLEAR_MASK },
    { RC32312_APLL_PAGE, RC32312_APLL_LOL_EVENT_REG, RC32312_APLL_LOL_EVENT_CLEAR_VAL, RC32312_APLL_LOL_EVENT_CLEAR_MASK },
    { RC32312_APLL_PAGE, RC32312_APLL_EXT_EVENT_REG, RC32312_APLL_CLEAR_VAL, RC32312_APLL_LOL_EVENT_CLEAR_MASK },
    { RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_EVT_REG, RC32312_APLL_CLEAR_VAL, RC32312_APLL_LOL_EVENT_CLEAR_MASK },
    { RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_CNT_REG, RC32312_APLL_LOL_EVENT_CLEAR_VAL, RC32312_APLL_LOL_EVENT_CLEAR_MASK },
};

static const struct rc32312_clear_op rc38112_clear_ops[] = {
    { RC38112_XTAL_PAGE, RC38112_XTAL_LOS_EVT_REG, RC38112_XTAL_LOS_EVT_CLEAR_VAL, RC38112_XTAL_LOS_EVT_CLEAR_MASK },
    { RC38112_XTAL_PAGE, RC38112_XTAL_LOS_CNT_REG, RC38112_XTAL_LOS_CNT_CLEAR_VAL, RC38112_XTAL_LOS_CNT_CLEAR_MASK },
    { RC38112_APLL_PAGE, RC38112_APLL_EVENT_REG, RC38112_APLL_EVENT_CLEAR_VAL, RC38112_APLL_EVENT_CLEAR_MASK },
    { RC38112_APLL_PAGE, RC38112_APLL_LOL_EVENT_REG, RC38112_APLL_LOL_EVENT_CLEAR_VAL, RC38112_APLL_LOL_EVENT_CLEAR_MASK },
    { RC38112_APLL_PAGE, RC38112_APLL_LOL_CLEAR_REG, RC38112_APLL_LOL_CLEAR_CLEAR_VAL, RC38112_APLL_LOL_CLEAR_CLEAR_MASK },
};


int rc32312_set_page(struct i2c_client *client, int page)
{
    struct rc32312_data *data = i2c_get_clientdata(client);
    int rv;

    rv = i2c_smbus_write_byte_data(client, data->page_reg, page);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 wirte page failed page_reg 0x%x target page %d, errno: %d\n", data->page_reg, page, rv);
        return rv;
    }

    rv = i2c_smbus_read_byte_data(client, data->page_reg);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 read page failed page_reg 0x%x target page %d, errno: %d\n", data->page_reg, page, rv);
        return rv;
    }

    if (rv != page) {
        DEBUG_ERROR("rc32312 current page %d != target page %d \n", rv, page);
        return -EIO;
    }
    DEBUG_VERBOSE("rc32312 set to page: %d success \n", page);
    return 0;
}

static s32 rc32312_read_byte_data(struct i2c_client *client, int page, u8 reg)
{
    int rv;

    rv = rc32312_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 read byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_read_byte_data(client, reg);
}

static s32 rc32312_read_word_data(struct i2c_client *client, int page, u8 reg)
{
    int rv;

    rv = rc32312_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 read word setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_read_word_data(client, reg);
}

static int rc32312_write_byte_data(struct i2c_client *client, int page, u8 reg, u8 value)
{
    int rv;

    rv = rc32312_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 write byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_write_byte_data(client, reg, value);
}

static int rc32312_write_word_data(struct i2c_client *client, int page, u8 reg, u16 value)
{
    int rv;

    rv = rc32312_set_page(client, page);
    if (rv < 0) {
        DEBUG_ERROR("rc32312 write byte setting page failed errno: %d\n", rv);
        return rv;
    }

    return i2c_smbus_write_word_data(client, reg, value);
}

static ssize_t show_rc32312_value_word(struct device *dev, struct device_attribute *da, char *buf)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    s32 status;

    status = -1;
    mutex_lock(&data->update_lock);
    status = rc32312_read_word_data(client, page, reg);
    if (status < 0) {
        DEBUG_ERROR("rc32312 show value failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }
    DEBUG_VERBOSE("rc32312 show value success page [%d] reg[0x%x] read value[0x%x]\n", page, reg, status);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%x\n", status & 0xffff);
}

int rc32312_get_page_reg_from_attr(struct rc32312_data *data, int attr, int *page, int *reg)
{
    int i;
    rc32312_attr_match_t *tmp_attr_infos;
    int attr_infos_len;

    switch (data->chip) {
    case rc32312:
        tmp_attr_infos = rc32312_attr_infos;
        attr_infos_len = ARRAY_SIZE(rc32312_attr_infos);
        break;
    case rc21012:
        tmp_attr_infos = rc21012_attr_infos;
        attr_infos_len = ARRAY_SIZE(rc21012_attr_infos);
        break;
    case rc38112:
        tmp_attr_infos = rc38112_attr_infos;
        attr_infos_len = ARRAY_SIZE(rc38112_attr_infos);
        break;
    default:
        DEBUG_ERROR("Unknown chip %d\n", data->chip);
        return -ENODEV;
    }
    for (i = 0; i < attr_infos_len; i++) {
        if (tmp_attr_infos[i].attr == attr) {
            *reg = tmp_attr_infos[i].reg;
            *page = tmp_attr_infos[i].page;
            DEBUG_VERBOSE("get_page_reg_from_attr success, chip[%d], attr[%d], page[%d], reg[%d]\n", 
                data->chip, attr, *page, *reg);
            return 0;
        }
    }

    DEBUG_ERROR("get_page_reg_from_attr fail, chip[%d], attr[%d]\n", data->chip, attr);
    return -ENODEV;
}

static ssize_t show_common_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int attr = to_sensor_dev_attr_2(da)->nr;
    int reg, page;
    int ret;
    s32 status;

    ret = rc32312_get_page_reg_from_attr(data, attr, &page, &reg);
    if  (ret < 0) {
        return ret;
    }

    status = -1;
    mutex_lock(&data->update_lock);
    status = rc32312_read_byte_data(client, page, reg);
    if (status < 0) {
        DEBUG_ERROR("rc32312 show value failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }
    DEBUG_VERBOSE("rc32312 show value success page [%d] reg[0x%x] read value[0x%x]\n", page, reg, status);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%x\n", status & 0xff);
}

static ssize_t set_common_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int attr = to_sensor_dev_attr_2(da)->nr;
    int reg, page;
    u8 val;
    int ret;

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    ret = rc32312_get_page_reg_from_attr(data, attr, &page, &reg);
    if  (ret < 0) {
        return ret;
    }

    mutex_lock(&data->update_lock);
    ret = rc32312_write_byte_data(client, page, reg, val);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 set value failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page, reg, val, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_VERBOSE("rc32312 set value success page [%d] reg[0x%x] value[0x%x]\n", page, reg, val);
    mutex_unlock(&data->update_lock);

    return count;
}

static ssize_t show_rc32312_out_ctrl_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    s32 status;

    status = -1;
    mutex_lock(&data->update_lock);
    status = rc32312_read_word_data(client, page, reg);
    if (status < 0) {
        DEBUG_ERROR("rc32312 read word data failed page [%d] reg [0x%x], errno: %d\n", page, reg, status);
        mutex_unlock(&data->update_lock);
        return status;
    }
    DEBUG_VERBOSE("rc32312 read word data success page [%d] reg[0x%x] read value[0x%x]\n", page, reg, status);

    mutex_unlock(&data->update_lock);
    if (status == RC32312_OUT_CTRL_EN) {
        return snprintf(buf, PAGE_SIZE, "1\n");
    }

    if (status == RC32312_OUT_CTRL_DIS) {
        return snprintf(buf, PAGE_SIZE, "0\n");
    }

    return snprintf(buf, PAGE_SIZE, "0x%04x\n", status & 0xffff);
}

static ssize_t show_rc32312_bus_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client;
    s32 read_vendor_id;
    int bus_status;

    if (!data || !data->client) {
        DEBUG_ERROR("rc32312 invalid data or client pointer\n");
        return -EINVAL;
    }

    if (data->vendor_id == 0) {
        DEBUG_ERROR("rc32312 vendor_id is 0\n");
        return -EINVAL;
    }

    client = data->client;
    mutex_lock(&data->update_lock);
    read_vendor_id = rc32312_read_word_data(client, RC32312_VENDOR_ID_PAGE, RC32312_VENDOR_ID_REG);
    mutex_unlock(&data->update_lock);
    if (read_vendor_id < 0) {
        DEBUG_ERROR("rc32312 read vendor_id failed errno: %d\n", read_vendor_id);
        return read_vendor_id;
    }
    
    bus_status = ((read_vendor_id & 0xffff) == data->vendor_id) ?
        RC32312_CLOCK_STATUS_NORMAL : RC32312_CLOCK_STATUS_ABNORMAL;

    return snprintf(buf, PAGE_SIZE, "%d\n", bus_status);
}

static ssize_t set_rc32312_out_ctrl_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    u16 val;
    int ret;

    val = 0;
    ret = kstrtou16(buf, 0, &val);
    if (ret) {
        dev_err(&client->dev, "Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if ((val != 0) && (val != 1)) {
        dev_err(&client->dev, "Unsupport value: %d, please enter 0 or 1\n", val);
        return -EINVAL;
    }

    if (val == 0) {
        val = RC32312_OUT_CTRL_DIS;
    } else {
        val = RC32312_OUT_CTRL_EN;
    }

    mutex_lock(&data->update_lock);
    ret = rc32312_write_word_data(client, page, reg, val);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 write word data failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page, reg, val, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_VERBOSE("rc32312 write word data success page [%d] reg[0x%x] value[0x%x]\n", page, reg, val);
    mutex_unlock(&data->update_lock);
    return count;
}

static int clear_rc32312_value(struct rc32312_data *data, int page, u8 reg, u8 val, u8 mask)
{
    struct i2c_client *client;
    s32 ori_status;
    u8 clear_value;
    int ret;

    if (!data || !data->client) {
        DEBUG_ERROR("Invalid data or client pointer\n");
        return -EINVAL;
    }
    client = data->client;

    mutex_lock(&data->update_lock);
    ori_status = rc32312_read_byte_data(client, page, reg);
    if (ori_status < 0) {
        DEBUG_ERROR("rc32312 read value failed page [%d] reg [0x%x], errno: %d\n", page, reg, ori_status);
        mutex_unlock(&data->update_lock);
        return ori_status;
    }
    clear_value = (((u8)ori_status & ~(mask)) | (val & (mask)));

    ret = rc32312_write_byte_data(client, page, reg, clear_value);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 write value failed page [%d] reg [0x%x] val [0x%x], errno: %d\n", page, reg, clear_value, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    DEBUG_VERBOSE("rc32312 clear value success page [%d] reg [0x%x] value [0x%x]\n", page, reg, clear_value);
    mutex_unlock(&data->update_lock);

    return 0;
}

static int clear_common_values(struct rc32312_data *data)
{
    const struct rc32312_clear_op *clear_ops = NULL;
    size_t clear_ops_size;
    const struct rc32312_clear_op *op;
    int ret;
    int i;

    /* Select the appropriate clear operations based on chip type */
    switch (data->chip) {
    case rc32312:
        clear_ops = rc32312_clear_ops;
        clear_ops_size = ARRAY_SIZE(rc32312_clear_ops);
        break;
    case rc38112:
        clear_ops = rc38112_clear_ops;
        clear_ops_size = ARRAY_SIZE(rc38112_clear_ops);
        break;
    case rc21012:
    default:
        DEBUG_ERROR("not support chip type %d\n", data->chip);
        return -ENODEV;
    }

    for (i = 0; i < clear_ops_size; i++) {
        op = &clear_ops[i];
        ret = clear_rc32312_value(data, op->page, op->reg, op->val, op->mask);
        if (ret < 0) {
            DEBUG_ERROR("rc32312 clear reg[%d] failed, errno: %d\n", i, ret);
            return ret;
        }
    }

    return 0;
}

static int rc32312_get_status(struct rc32312_data *data, int *status)
{
    struct i2c_client *client;
    s32 read_vendor_id;
    s32 xtal_los_evt;
    s32 xtal_los_sts;
    s32 apll_los_evt;
    s32 apll_los_sts;
    int bus_status;
    int xtal_status;
    int apll_status;

    if (!data || !data->client || !status) {
        DEBUG_ERROR("rc32312 invalid data, client or status pointer\n");
        return -EINVAL;
    }

    if (data->vendor_id == 0) {
        DEBUG_ERROR("rc32312 vendor_id is 0\n");
        return -EINVAL;
    }

    client = data->client;
    read_vendor_id = rc32312_read_word_data(client, RC32312_VENDOR_ID_PAGE, RC32312_VENDOR_ID_REG);
    if (read_vendor_id < 0) {
        DEBUG_ERROR("rc32312 read vendor_id failed errno: %d\n", read_vendor_id);
        return read_vendor_id;
    }

    xtal_los_evt = rc32312_read_byte_data(client, RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_EVT_REG);
    if (xtal_los_evt < 0) {
        DEBUG_ERROR("rc32312 read xtal_los_evt failed errno: %d\n", xtal_los_evt);
        return xtal_los_evt;
    }

    xtal_los_sts = rc32312_read_byte_data(client, RC32312_XTAL_LOS_PAGE, RC32312_XTAL_LOS_STS_REG);
    if (xtal_los_sts < 0) {
        DEBUG_ERROR("rc32312 read xtal_los_sts failed errno: %d\n", xtal_los_sts);
        return xtal_los_sts;
    }

    apll_los_evt = rc32312_read_byte_data(client, RC32312_APLL_PAGE, RC32312_APLL_EVENT_REG);
    if (apll_los_evt < 0) {
        DEBUG_ERROR("rc32312 read apll_event failed errno: %d\n", apll_los_evt);
        return apll_los_evt;
    }

    apll_los_sts = rc32312_read_byte_data(client, RC32312_APLL_PAGE, RC32312_APLL_STS_REG);
    if (apll_los_sts < 0) {
        DEBUG_ERROR("rc32312 read apll_los_sts failed errno: %d\n", apll_los_sts);
        return apll_los_sts;
    }

    bus_status = ((read_vendor_id & 0xffff) == data->vendor_id) ?
        RC32312_CLOCK_STATUS_NORMAL : RC32312_CLOCK_STATUS_ABNORMAL;

    xtal_status = ((xtal_los_evt & RC32312_CLOCK_REG_INFO_MASK) |
        (xtal_los_sts & RC32312_CLOCK_REG_INFO_MASK)) ?
        RC32312_CLOCK_STATUS_ABNORMAL : RC32312_CLOCK_STATUS_NORMAL;

    apll_status = ((apll_los_evt & RC32312_CLOCK_REG_INFO_MASK) |
        !(apll_los_sts & RC32312_CLOCK_REG_INFO_MASK)) ?
        RC32312_CLOCK_STATUS_ABNORMAL : RC32312_CLOCK_STATUS_NORMAL;

    *status = (bus_status << RC32312_CLOCK_FAULT_BUS) |
        (xtal_status << RC32312_CLOCK_FAULT_XTAL) |
        (apll_status << RC32312_CLOCK_FAULT_APLL);

    return 0;
}

static ssize_t show_rc32312_status(struct device *dev, struct device_attribute *da, char *buf)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    int status;
    int ret;

    if (!data || !data->client) {
        DEBUG_ERROR("rc32312 invalid data or client pointer\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    ret = rc32312_get_status(data, &status);
    mutex_unlock(&data->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 get status failed, errno: %d\n", ret);
        return ret;
    }

    ret = clear_common_values(data);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 clear values failed, errno: %d\n", ret);
        return ret;
    }

    return snprintf(buf, PAGE_SIZE, "0x%x\n", status);
}

static ssize_t clear_rc32312_reg_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct rc32312_data *data = dev_get_drvdata(dev);
    u16 val;
    int ret;

    if (!data || !data->client) {
        return -EINVAL;
    }

    val = 0;
    ret = kstrtou16(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if (val != 1) {
        DEBUG_ERROR("Unsupport value: %d, please enter 1\n", val);
        return -EINVAL;
    }

    ret = clear_common_values(data);
    if (ret < 0) {
        DEBUG_ERROR("rc32312 clear values failed, errno: %d\n", ret);
        return ret;
    }

    return count;
}

static int rc32312_init(struct rc32312_data *data)
{
    int ret = 0;

    if (!data) {
        DEBUG_ERROR("Invalid data pointer\n");
        return -EINVAL;
    }

    switch (data->chip) {
    case rc32312:
        ret = clear_rc32312_value(data, RC32312_APLL_PAGE, RC32312_APLL_EVENT_REG, RC32312_APLL_CLEAR_VAL, RC32312_APLL_CLEAR_MASK);
        ret += clear_rc32312_value(data, RC32312_APLL_PAGE, RC32312_APLL_LOL_EVENT_REG, RC32312_APLL_LOL_EVENT_CLEAR_VAL, RC32312_APLL_LOL_EVENT_CLEAR_MASK);
        break;
    case rc21012:
        ret = clear_rc32312_value(data, RC21012_LOSMON_PAGE, RC21012_LOSMON4_CNT_REG, RC21012_LOSMON4_CNT_CLEAR_VAL, RC21012_LOSMON4_CNT_CLEAR_MASK);
        ret += clear_rc32312_value(data, RC21012_APLL_PAGE, RC21012_APLL_LOL_CNT_REG, RC21012_APLL_LOL_CNT_CLEAR_VAL, RC21012_APLL_LOL_CNT_CLEAR_MASK);
        break;
    case rc38112:
        ret += clear_rc32312_value(data, RC38112_APLL_PAGE, RC38112_APLL_EVENT_REG, RC38112_APLL_EVENT_CLEAR_VAL, RC38112_APLL_EVENT_CLEAR_MASK);
        ret += clear_rc32312_value(data, RC38112_APLL_PAGE, RC38112_APLL_LOL_EVENT_REG, RC38112_APLL_LOL_EVENT_CLEAR_VAL, RC38112_APLL_LOL_EVENT_CLEAR_MASK);
        break;
    default:
        dev_dbg(&data->client->dev, "Unknown chip id %d, skip\n", data->chip);
        return -ENODEV;
    }

    if (ret < 0) {
        dev_warn(&data->client->dev, "RC32312 init fail\n");
        return ret;
    }
    dev_dbg(&data->client->dev, "RC32312 init success.\n");
    return ret;
}

static SENSOR_DEVICE_ATTR_2(apll_event, S_IRUGO | S_IWUSR, show_common_value, set_common_value, APLL_EVENT, 0);
static SENSOR_DEVICE_ATTR_2(apll_lol_event, S_IRUGO | S_IWUSR, show_common_value, set_common_value, APLL_COUNT, 0);
static SENSOR_DEVICE_ATTR_2(apll_sts, S_IRUGO, show_common_value, NULL, APLL_STATUS, 0);
static SENSOR_DEVICE_ATTR_2(xtal_los_evt, S_IRUGO | S_IWUSR, show_common_value, set_common_value, XTAL_LOS_EVT, 0);
static SENSOR_DEVICE_ATTR_2(xtal_los_cnt, S_IRUGO | S_IWUSR, show_common_value, set_common_value, XTAL_LOS_CNT, 0);
static SENSOR_DEVICE_ATTR_2(xtal_los_sts, S_IRUGO, show_common_value, NULL, XTAL_LOS_STS, 0);
static SENSOR_DEVICE_ATTR_2(clear_reg, S_IWUSR, NULL, clear_rc32312_reg_value, 0, 0);
static SENSOR_DEVICE_ATTR_2(vendor_id, S_IRUGO, show_rc32312_value_word, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(device_id, S_IRUGO, show_rc32312_value_word, NULL, 0, 0x2);
static SENSOR_DEVICE_ATTR_2(device_pgm, S_IRUGO, show_rc32312_value_word, NULL, 0, 0x6);
static SENSOR_DEVICE_ATTR_2(bus_status, S_IRUGO, show_rc32312_bus_status, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(status, S_IRUGO, show_rc32312_status, NULL, 0, 0);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_0, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x04);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_1, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x0c);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_2, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x14);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_3, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x1c);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_4, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x24);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_5, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x2c);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_6, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x34);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_7, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x3c);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_8, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x44);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_9, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x4c);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_10, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x54);
static SENSOR_DEVICE_ATTR_2(out_en_ctrl_11, S_IRUGO | S_IWUSR, show_rc32312_out_ctrl_value, set_rc32312_out_ctrl_value, 1, 0x5c);

static struct attribute *rc32312_sysfs_attrs[] = {
    &sensor_dev_attr_apll_event.dev_attr.attr,
    &sensor_dev_attr_apll_lol_event.dev_attr.attr,
    &sensor_dev_attr_apll_sts.dev_attr.attr,
    &sensor_dev_attr_xtal_los_evt.dev_attr.attr,
    &sensor_dev_attr_xtal_los_cnt.dev_attr.attr,
    &sensor_dev_attr_xtal_los_sts.dev_attr.attr,
    &sensor_dev_attr_clear_reg.dev_attr.attr,
    &sensor_dev_attr_vendor_id.dev_attr.attr,
    &sensor_dev_attr_device_id.dev_attr.attr,
    &sensor_dev_attr_device_pgm.dev_attr.attr,
    &sensor_dev_attr_bus_status.dev_attr.attr,
    &sensor_dev_attr_status.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_0.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_1.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_2.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_3.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_4.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_5.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_6.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_7.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_8.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_9.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_10.dev_attr.attr,
    &sensor_dev_attr_out_en_ctrl_11.dev_attr.attr,
    NULL
};

static struct attribute_group  rc32312_sysfs_group = {
    .attrs = rc32312_sysfs_attrs,
};

static SENSOR_DEVICE_ATTR_2(losmon4_sts, S_IRUGO, show_common_value, NULL, LOSMON4_STS, 0);
static SENSOR_DEVICE_ATTR_2(losmon4_cnt, S_IRUGO | S_IWUSR, show_common_value, set_common_value, LOSMON4_CNT, 0);

static struct attribute *rc21012_sysfs_attrs[] = {
    &sensor_dev_attr_vendor_id.dev_attr.attr,
    &sensor_dev_attr_device_pgm.dev_attr.attr,
    &sensor_dev_attr_apll_sts.dev_attr.attr,
    &sensor_dev_attr_apll_lol_event.dev_attr.attr,
    &sensor_dev_attr_losmon4_sts.dev_attr.attr,
    &sensor_dev_attr_losmon4_cnt.dev_attr.attr,
    NULL
};

static struct attribute_group  rc21012_sysfs_group = {
    .attrs = rc21012_sysfs_attrs,
};

static struct attribute *rc38112_sysfs_attrs[] = {
    &sensor_dev_attr_apll_event.dev_attr.attr,
    &sensor_dev_attr_apll_lol_event.dev_attr.attr,
    &sensor_dev_attr_apll_sts.dev_attr.attr,
    &sensor_dev_attr_xtal_los_evt.dev_attr.attr,
    &sensor_dev_attr_xtal_los_cnt.dev_attr.attr,
    &sensor_dev_attr_xtal_los_sts.dev_attr.attr,
    &sensor_dev_attr_clear_reg.dev_attr.attr,
    &sensor_dev_attr_vendor_id.dev_attr.attr,
    &sensor_dev_attr_device_id.dev_attr.attr,
    &sensor_dev_attr_device_pgm.dev_attr.attr,
    &sensor_dev_attr_bus_status.dev_attr.attr,
    &sensor_dev_attr_status.dev_attr.attr,
    NULL
};

static struct attribute_group  rc38112_sysfs_group = {
    .attrs = rc38112_sysfs_attrs,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static int rc32312_probe(struct i2c_client *client)
#else
static int rc32312_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
    struct rc32312_data *data;
    int ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct i2c_device_id *id = i2c_client_get_device_id(client);
#endif

    dev_dbg(&client->dev, "RC32312 enter probe\n");
    data = devm_kzalloc(&client->dev, sizeof(struct rc32312_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "RC32312 alloc memory failed\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);
    data->chip = id->driver_data;

    switch (id->driver_data) {
    case rc32312:
        data->sysfs_group = &rc32312_sysfs_group;
        data->page_reg = RC32312_PAGE_REG;
        data->vendor_id = RC32312_VENDOR_ID;
        break;
    case rc21012:
        data->sysfs_group = &rc21012_sysfs_group;
        data->page_reg = RC32312_PAGE_REG;
        data->vendor_id = RC32312_VENDOR_ID;
        break;
    case rc38112:
        data->sysfs_group = &rc38112_sysfs_group;
        data->page_reg = RC32312_PAGE_REG;
        data->vendor_id = RC32312_VENDOR_ID;
        break;
    default:
        dev_err(&client->dev, "Unknown chip id %ld\n", id->driver_data);
        return -ENODEV;
    }

    (void)rc32312_init(data);
    ret = sysfs_create_group(&client->dev.kobj, data->sysfs_group);
    if (ret < 0) {
        dev_err(&client->dev, "RC32312 sysfs_create_group failed %d\n", ret);
    }

    dev_info(&client->dev, "init %s success\n", client->name);
    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
static void rc32312_remove(struct i2c_client *client)
#else
static int rc32312_remove(struct i2c_client *client)
#endif
{
    struct rc32312_data *data = i2c_get_clientdata(client);

    if (data->sysfs_group) {
        dev_info(&client->dev, "RC32312 unregister sysfs group\n");
        sysfs_remove_group(&client->dev.kobj, (const struct attribute_group *)data->sysfs_group);
    }
    dev_info(&client->dev, "RC32312 removed\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct i2c_device_id rc32312_id[] = {
    { "wb_rc32312", rc32312 },
    { "wb_rc21012", rc21012 },
    { "wb_rc38112", rc38112 },
    {}
};
MODULE_DEVICE_TABLE(i2c, rc32312_id);

static struct i2c_driver wb_rc32312_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "wb_rc32312",
    },
    .probe      = rc32312_probe,
    .remove     = rc32312_remove,
    .id_table   = rc32312_id,
};

module_i2c_driver(wb_rc32312_driver);
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("RC32312 driver");
MODULE_LICENSE("GPL");
