#include <linux/init.h>
#include <linux/slab.h>
#include <linux/platform_data/pca954x.h>
#include <linux/i2c-mux.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>
#include <linux/err.h>

#define CPUPLD_ADDR      0x31
#define SWPLD1_ADDR      0x6a
#define SWPLD2_ADDR      0x73
#define SWPLD3_ADDR      0x75
#define MUX_VAL_SWPLD    0xff
#define MUX_VAL_IDEEPROM 0xfc
#define MUX_VAL_PCA9548  0xfd
#define DEF_DEV_NUM      1
#define BUS0_DEV_NUM     3
#define BUS0_BASE_NUM    1
#define BUS0_MUX_REG     0x14
#define BUS3_DEV_NUM     33
#define BUS3_BASE_NUM    20
#define BUS3_MUX_REG     0x1f
#define QSFP_PRESENCE_1  0x12
#define QSFP_PRESENCE_2  0x13
#define QSFP_PRESENCE_3  0x14
#define QSFP_PRESENCE_4  0x15
#define SFP_PRESENCE_5   0x02
#define QSFP_LPMODE_1    0x0E
#define QSFP_LPMODE_2    0x0F
#define QSFP_LPMODE_3    0x10
#define QSFP_LPMODE_4    0x11
#define QSFP_RESET_1     0x16
#define QSFP_RESET_2     0x17
#define QSFP_RESET_3     0x18
#define QSFP_RESET_4     0x19


struct mutex dni_lock;
void device_release(struct device *dev)
{
    return;
}

unsigned char dni_log2 (unsigned char num){
    unsigned char num_log2 = 0;
    while(num > 0){
        num = num >> 1;
        num_log2 += 1;
    }
    return num_log2 -1;
}

unsigned char reverse_8bits(unsigned char c)
{
        unsigned char s = 0;
        int i;
        for (i = 0; i < 8; ++i) {
                s <<= 1;
                s |= c & 1;
                c >>= 1;
        }
        return s;
}

enum{
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
    BUS8,
    BUS9,
    BUS10,
    BUS11,
    BUS12,
    BUS13,
    BUS14,
};

#define ag9032v2a_i2c_device_num(NUM){                                         \
        .name                   = "delta-ag9032v2a-i2c-device",                \
        .id                     = NUM,                                      \
        .dev                    = {                                         \
                    .platform_data = &ag9032v2a_i2c_device_platform_data[NUM], \
                    .release       = device_release,                        \
        },                                                                  \
}

struct cpld_attribute_data {
    uint8_t bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t mask;
    char note[200];
};

enum cpld_type {
    system_cpld,
};

enum swpld1_type {
    swpld1,
};

enum swpld2_type {
    swpld2,
};

enum swpld3_type {
    swpld3,
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

enum cpld_attributes {
    CPLD_REG_ADDR,
    CPLD_REG_VALUE,
    SWPLD1_REG_ADDR,
    SWPLD1_REG_VALUE,
    SWPLD2_REG_ADDR,
    SWPLD2_REG_VALUE,
    SWPLD3_REG_ADDR,
    SWPLD3_REG_VALUE,
};

enum ag9032v2a_sfp_sysfs_attributes 
{
    SFP_IS_PRESENT,
    QSFP_LPMODE,
    QSFP_RESET,
};

static struct cpld_attribute_data attribute_data[] = {
    [CPLD_REG_ADDR] = {
    },
    [CPLD_REG_VALUE] = {
    },
    [SWPLD1_REG_ADDR] = {
    },
    [SWPLD1_REG_VALUE] = {
    },
    [SWPLD2_REG_ADDR] = {
    },
    [SWPLD2_REG_VALUE] = {
    },
    [SWPLD3_REG_ADDR] = {
    },
};
struct i2c_device_platform_data {
    int parent;
    struct i2c_board_info info;
    struct i2c_client *client;
};

struct i2c_client * i2c_client_9548;

static struct cpld_platform_data ag9032v2a_cpld_platform_data[] = {
    [system_cpld] = {
        .reg_addr = CPUPLD_ADDR,
    },
};

static struct cpld_platform_data ag9032v2a_swpld1_platform_data[] = {
    [swpld1] = {
        .reg_addr = SWPLD1_ADDR,
    },
};

static struct cpld_platform_data ag9032v2a_swpld2_platform_data[] = {
    [swpld2] = {
        .reg_addr = SWPLD2_ADDR,
    },
};

static struct cpld_platform_data ag9032v2a_swpld3_platform_data[] = {
    [swpld3] = {
        .reg_addr = SWPLD3_ADDR,
    },
};

// pca9548 - add 8 bus 
static struct pca954x_platform_mode pca954x_mode[] = 
{
    { 
        .adap_id = 10,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 11,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 12,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 13,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 14,
        .deselect_on_exit = 1,
    },
    {   
        .adap_id = 15,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 16,
        .deselect_on_exit = 1,
    },
    { 
        .adap_id = 17,
        .deselect_on_exit = 1,
    },
};

static struct pca954x_platform_data pca954x_data = 
{
    .modes = pca954x_mode, 
    .num_modes = ARRAY_SIZE(pca954x_mode),
};

static struct i2c_board_info __initdata i2c_info_pca9548[] =
{
    {
        I2C_BOARD_INFO("pca9548", 0x71),
        .platform_data = &pca954x_data,
    },
};

/*----------------   I2C device   - start   ------------- */
static struct i2c_device_platform_data ag9032v2a_i2c_device_platform_data[] = {
    {
        // id eeprom
        .parent = 1,
        .info = { I2C_BOARD_INFO("24c02", 0x53) },
        .client = NULL,
    },
    {
        // qsfp 1 (0x50)
        .parent = 20,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 2 (0x50)
        .parent = 21,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 3 (0x50)
        .parent = 22,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 4 (0x50)
        .parent = 23,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 5 (0x50)
        .parent = 24,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 6 (0x50)
        .parent = 25,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 7 (0x50)
        .parent = 26,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 8 (0x50)
        .parent = 27,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 9 (0x50)
        .parent = 28,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 10 (0x50)
        .parent = 29,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 11 (0x50)
        .parent = 30,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 12 (0x50)
        .parent = 31,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 13 (0x50)
        .parent = 32,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 14 (0x50)
        .parent = 33,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 15 (0x50)
        .parent = 34,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 16 (0x50)
        .parent = 35,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 17 (0x50)
        .parent = 36,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 18 (0x50)
        .parent = 37,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 19 (0x50)
        .parent = 38,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 20 (0x50)
        .parent = 39,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 21 (0x50)
        .parent = 40,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 22 (0x50)
        .parent = 41,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 23 (0x50)
        .parent = 42,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 24 (0x50)
        .parent = 43,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 25 (0x50)
        .parent = 44,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 26 (0x50)
        .parent = 45,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 27 (0x50)
        .parent = 46,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 28 (0x50)
        .parent = 47,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 29 (0x50)
        .parent = 48,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 30 (0x50)
        .parent = 49,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 31 (0x50)
        .parent = 50,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // qsfp 32 (0x50)
        .parent = 51,
        .info = { .type = "optoe1", .addr = 0x50 },
        .client = NULL,
    },
    {
        // sfp 1 (0x50) 
        .parent = 52,
        .info = { .type = "optoe2", .addr = 0x50 },
        .client = NULL,
    },
};

static struct platform_device ag9032v2a_i2c_device[] = {
    ag9032v2a_i2c_device_num(0),
    ag9032v2a_i2c_device_num(1),
    ag9032v2a_i2c_device_num(2),
    ag9032v2a_i2c_device_num(3),
    ag9032v2a_i2c_device_num(4),
    ag9032v2a_i2c_device_num(5),
    ag9032v2a_i2c_device_num(6),
    ag9032v2a_i2c_device_num(7),
    ag9032v2a_i2c_device_num(8),
    ag9032v2a_i2c_device_num(9),
    ag9032v2a_i2c_device_num(10),
    ag9032v2a_i2c_device_num(11),
    ag9032v2a_i2c_device_num(12),
    ag9032v2a_i2c_device_num(13),
    ag9032v2a_i2c_device_num(14),
    ag9032v2a_i2c_device_num(15),
    ag9032v2a_i2c_device_num(16),
    ag9032v2a_i2c_device_num(17),
    ag9032v2a_i2c_device_num(18),
    ag9032v2a_i2c_device_num(19),
    ag9032v2a_i2c_device_num(20),
    ag9032v2a_i2c_device_num(21),
    ag9032v2a_i2c_device_num(22),
    ag9032v2a_i2c_device_num(23),
    ag9032v2a_i2c_device_num(24),
    ag9032v2a_i2c_device_num(25),
    ag9032v2a_i2c_device_num(26),
    ag9032v2a_i2c_device_num(27),
    ag9032v2a_i2c_device_num(28),
    ag9032v2a_i2c_device_num(29),
    ag9032v2a_i2c_device_num(30),
    ag9032v2a_i2c_device_num(31),
    ag9032v2a_i2c_device_num(32),
    ag9032v2a_i2c_device_num(33),
};

/*----------------   I2C device   - end   ------------- */

/*----------------   I2C driver   - start   ------------- */

static int __init i2c_device_probe(struct platform_device *pdev)
{
    struct i2c_device_platform_data *pdata;
    struct i2c_adapter *parent;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n",
            pdata->parent);
        return -ENODEV;
    }

    pdata->client = i2c_new_device(parent, &pdata->info);
    if (!pdata->client) {
        dev_err(&pdev->dev, "Failed to create i2c client %s at %d\n",
            pdata->info.type, pdata->parent);
        return -ENODEV;
    }

    return 0; 
}

static int __exit i2c_deivce_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent;
    struct i2c_device_platform_data *pdata;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    if (pdata->client) {
        parent = (pdata->client)->adapter;
        i2c_unregister_device(pdata->client);
        i2c_put_adapter(parent);
    }

    return 0;
}
static struct platform_driver i2c_device_driver = {
    .probe = i2c_device_probe,
    .remove = __exit_p(i2c_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-ag9032v2a-i2c-device",
    }
};

/*----------------   I2C driver   - end   ------------- */

/*----------------    SFP attribute read/write - start -------- */
static struct kobject *kobj_swpld1;
static struct kobject *kobj_swpld2;
static struct kobject *kobj_swpld3;

static ssize_t for_status(struct device *dev, struct device_attribute *dev_attr, char *buf){
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev_1 = kobj_to_dev(kobj_swpld1);
    struct device *i2cdev_2 = kobj_to_dev(kobj_swpld2);
    struct cpld_platform_data *pdata1 = i2cdev_1->platform_data;
    struct cpld_platform_data *pdata2 = i2cdev_2->platform_data;
    int ret;
    int ret_sfp;
    u64 data = 0;

    mutex_lock(&dni_lock);
    switch (attr->index) {
        case SFP_IS_PRESENT:
            /*QSFP25~32*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_PRESENCE_4);
            data = (u32)ret & 0xff;
            /*QSFP17~24*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_PRESENCE_3);
            data |= ((u32)ret & 0xff) << 8;
            /*QSFP9~16*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_PRESENCE_2);
            data |= (u32)(ret & 0xff) << 16;
            /*QSFP1~8*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_PRESENCE_1);
            data |= (u32)(ret & 0xff) << 24;

            ret = i2c_smbus_read_byte_data(pdata2[swpld2].client, SFP_PRESENCE_5);
            ret_sfp = (ret & (0x80));
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%x%02x\n", data, ret_sfp);

        case QSFP_LPMODE:
            /*QSFP25~32*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_LPMODE_4);
            data = (u32)(ret & 0xff);
            /*QSFP17~24*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_LPMODE_3);
            data |= (u32)(ret & 0xff) << 8;
            /*QSFP9~16*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_LPMODE_2);
            data |= (u32)(ret & 0xff) << 16;
            /*QSFP1~8*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_LPMODE_1);
            data |= (u32)(ret & 0xff) << 24;
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%x\n", data);

         case QSFP_RESET:
            /*QSFP25~32*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_RESET_4);
            data = (u32)(ret & 0xff);
            /*QSFP17~24*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_RESET_3);
            data |= (u32)(ret & 0xff) << 8;
            /*QSFP9~16*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_RESET_2);
            data |= (u32)(ret & 0xff) << 16;
            /*QSFP1~8*/
            ret = i2c_smbus_read_byte_data(pdata1[swpld1].client, QSFP_RESET_1);
            data |= (u32)(ret & 0xff) << 24;
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%x\n", data);

        default:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "%d not found", attr->index);
    }
}

static ssize_t set_lpmode_data(struct device *dev, struct device_attribute *dev_attr, const char *buf, size_t count)
{
    struct device *i2cdev = kobj_to_dev(kobj_swpld1);
    struct cpld_platform_data *pdata = i2cdev->platform_data;
    unsigned long long set_data;
    int err;
    int status = 0;
    unsigned char set_bytes;


    err = kstrtoull(buf, 16, &set_data);
    if (err){
        return err;
    }
    mutex_lock(&dni_lock);
    /*QSFP25~32*/
    set_bytes = set_data & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_LPMODE_4, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP17~24*/
    set_bytes = (set_data >> 8 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_LPMODE_3, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP9~16*/
    set_bytes = (set_data >> 16 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_LPMODE_2, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP1~8*/
    set_bytes = (set_data >> 24 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_LPMODE_1, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    mutex_unlock(&dni_lock);
    return count;

ERROR:
    mutex_unlock(&dni_lock);
    return status;

}

static ssize_t set_reset_data(struct device *dev, struct device_attribute *dev_attr, const char *buf, size_t count)
{
    struct device *i2cdev = kobj_to_dev(kobj_swpld1);
    struct cpld_platform_data *pdata = i2cdev->platform_data;
    unsigned long long set_data;
    int err;
    int status = 0;
    unsigned char set_bytes;

    err = kstrtoull(buf, 16, &set_data);
    if (err){
        return err;
    }

    mutex_lock(&dni_lock);
    /*QSFP25~32*/
    set_bytes = set_data & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_RESET_4, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP17~24*/
    set_bytes = (set_data >> 8 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_RESET_3, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP9~16*/
    set_bytes = (set_data >> 16 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_RESET_2, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    /*QSFP1~8*/
    set_bytes = (set_data >> 24 ) & 0xff;
    status = i2c_smbus_write_byte_data(pdata[swpld1].client, QSFP_RESET_1, set_bytes);
    if(status < 0)
    {
        goto ERROR;
    }

    mutex_unlock(&dni_lock);
    return count;
ERROR:
    mutex_unlock(&dni_lock);
    return status;
}


/*----------------    SFP attribute read/write - end   -------- */

/*----------------    CPLD  - start   ------------- */
unsigned char cpupld_reg_addr;
unsigned char swpld1_reg_addr;
unsigned char swpld2_reg_addr;
unsigned char swpld3_reg_addr;
/*    CPLD  -- device   */
static struct platform_device cpld_device = {
    .name               = "delta-ag9032v2a-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v2a_cpld_platform_data,
                .release         = device_release
    },

};

static struct platform_device swpld1_device = {
    .name               = "delta-ag9032v2a-swpld1",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v2a_swpld1_platform_data,
                .release         = device_release
    },
};

static struct platform_device swpld2_device = {
    .name               = "delta-ag9032v2a-swpld2",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v2a_swpld2_platform_data,
                .release         = device_release
    },
};

static struct platform_device swpld3_device = {
    .name               = "delta-ag9032v2a-swpld3",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v2a_swpld3_platform_data,
                .release         = device_release
    },
};

static ssize_t get_cpld_reg(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    int mask;
    int value;
    char note[200];
    unsigned char reg;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct cpld_platform_data *pdata = dev->platform_data;
    
    mutex_lock(&dni_lock);
    switch (attr->index) {
        case CPLD_REG_ADDR:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", cpupld_reg_addr);
        case SWPLD1_REG_ADDR:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", swpld1_reg_addr);
        case SWPLD2_REG_ADDR:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", swpld2_reg_addr);
        case SWPLD3_REG_ADDR:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", swpld3_reg_addr);
        case SWPLD1_REG_VALUE:
            ret = i2c_smbus_read_byte_data(pdata[swpld1].client, swpld1_reg_addr);
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", ret);
        case SWPLD2_REG_VALUE:
            ret = i2c_smbus_read_byte_data(pdata[swpld2].client, swpld2_reg_addr);
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", ret);
        case SWPLD3_REG_VALUE:
            ret = i2c_smbus_read_byte_data(pdata[swpld3].client, swpld3_reg_addr);
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x\n", ret);
        default:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "%d not found", attr->index);
    } 
    
    switch (mask) 
    {
        case 0xff:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%02x%s", value, note);
        case 0x0f:
        case 0x03:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "0x%01x%s", value, note);
        case 0x0c:
            mutex_unlock(&dni_lock);
            value = value >> 2;
            return sprintf(buf, "0x%01x%s", value, note);
        case 0xf0:
            mutex_unlock(&dni_lock);
            value = value >> 4;
            return sprintf(buf, "0x%01x%s", value, note);
        case 0xe0:
            mutex_unlock(&dni_lock);
            value = value >> 5;
            return sprintf(buf, "0x%01x%s", value, note);
        default :
            value = value >> dni_log2(mask);
            mutex_unlock(&dni_lock);
            return sprintf(buf, "%d%s", value, note);
    }
}

static ssize_t set_cpld_reg(struct device *dev, struct device_attribute *dev_attr,
            const char *buf, size_t count)
{
    int err;
    int value;
    int set_data;
    unsigned long set_data_ul;
    unsigned char reg;
    unsigned char mask;  
    unsigned char mask_out;      

    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 0, &set_data_ul);
    if (err){
        return err;
    }
    
    set_data = (int)set_data_ul;
    if (set_data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    mutex_lock(&dni_lock);
    switch (attr->index) 
   {
        case CPLD_REG_ADDR:
            cpupld_reg_addr = set_data;
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD1_REG_ADDR:
            swpld1_reg_addr = set_data;
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD2_REG_ADDR:
            swpld2_reg_addr = set_data;
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD3_REG_ADDR:
            swpld3_reg_addr = set_data;
            mutex_unlock(&dni_lock);
            return count;
        case CPLD_REG_VALUE:
            i2c_smbus_write_byte_data(pdata[system_cpld].client, cpupld_reg_addr, set_data);
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD1_REG_VALUE:
            i2c_smbus_write_byte_data(pdata[swpld1].client, swpld1_reg_addr, set_data);
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD2_REG_VALUE:
            i2c_smbus_write_byte_data(pdata[swpld2].client, swpld2_reg_addr, set_data);
            mutex_unlock(&dni_lock);
            return count;
        case SWPLD3_REG_VALUE:
            i2c_smbus_write_byte_data(pdata[swpld3].client, swpld3_reg_addr, set_data);
            mutex_unlock(&dni_lock);
            return count;
        default:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "%d not found", attr->index);
    }

    switch (mask) {
        case 0x03:
        case 0x0C:
        case 0x0F:
        case 0xFF:
            set_data = mask_out | (set_data & mask);
            break;
        case 0xF0:
            set_data = set_data << 4;
            set_data = mask_out | (set_data & mask);
            break;
        case 0xE0:
            set_data = set_data << 5;
            set_data = mask_out | (set_data & mask);
            break;
        default :
            set_data = mask_out | (set_data << dni_log2(mask) );        
    }   

    switch (attr->index) {
        default:
            mutex_unlock(&dni_lock);
            return sprintf(buf, "cpld not found"); 
    }
    mutex_unlock(&dni_lock);
    return count;
}

//address and value
static SENSOR_DEVICE_ATTR(cpld_reg_addr,    S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, CPLD_REG_ADDR);
static SENSOR_DEVICE_ATTR(cpld_reg_value,   S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, CPLD_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld1_reg_addr,  S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD1_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld1_reg_value, S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD1_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld2_reg_addr,  S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD2_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld2_reg_value, S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD2_REG_VALUE);
static SENSOR_DEVICE_ATTR(swpld3_reg_addr,  S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD3_REG_ADDR);
static SENSOR_DEVICE_ATTR(swpld3_reg_value, S_IRUGO | S_IWUSR, get_cpld_reg, set_cpld_reg, SWPLD3_REG_VALUE);

//QSFP
static SENSOR_DEVICE_ATTR(sfp_is_present,   S_IRUGO,           for_status,   NULL,            SFP_IS_PRESENT);
static SENSOR_DEVICE_ATTR(qsfp_lpmode,      S_IRUGO | S_IWUSR, for_status,   set_lpmode_data, QSFP_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_reset,       S_IRUGO | S_IWUSR, for_status,   set_reset_data,  QSFP_RESET);

static struct attribute *cpld_attrs[] = {
    &sensor_dev_attr_cpld_reg_value.dev_attr.attr,
    &sensor_dev_attr_cpld_reg_addr.dev_attr.attr,
    NULL,
};

static struct attribute *swpld1_attrs[] = {
    //SWPLD1
    &sensor_dev_attr_swpld1_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld1_reg_addr.dev_attr.attr, 
    &sensor_dev_attr_sfp_is_present.dev_attr.attr,
    &sensor_dev_attr_qsfp_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_reset.dev_attr.attr,
    NULL,
};

static struct attribute *swpld2_attrs[] = {
    &sensor_dev_attr_swpld2_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld2_reg_addr.dev_attr.attr, 
    NULL,
};

static struct attribute *swpld3_attrs[] = {
    &sensor_dev_attr_swpld3_reg_value.dev_attr.attr,
    &sensor_dev_attr_swpld3_reg_addr.dev_attr.attr, 
    NULL,
};

static struct attribute_group cpld_attr_grp = {
    .attrs = cpld_attrs,
};

static struct attribute_group swpld1_attr_grp = {
    .attrs = swpld1_attrs,
};

static struct attribute_group swpld2_attr_grp = {
    .attrs = swpld2_attrs,
};

static struct attribute_group swpld3_attr_grp = {
    .attrs = swpld3_attrs,
};

static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS0);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", BUS0);
        return -ENODEV;
    }

    pdata[system_cpld].client = i2c_new_dummy(parent, pdata[system_cpld].reg_addr);
    if (!pdata[system_cpld].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[system_cpld].reg_addr);
        goto error;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &cpld_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    return 0;

error:
    i2c_unregister_device(pdata[system_cpld].client);
    i2c_put_adapter(parent);

    return -ENODEV;
}

static int __init swpld1_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD1 platform data not found\n");
        return -ENODEV;
    }
    parent = i2c_get_adapter(BUS3);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", BUS1);
        return -ENODEV;
    }

    pdata[swpld1].client = i2c_new_dummy(parent, pdata[swpld1].reg_addr);
    if (!pdata[swpld1].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[swpld1].reg_addr);
        goto error;
    }

    kobj_swpld1 = &pdev->dev.kobj;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld1_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld attribute group");
        goto error;
    }
    return 0;

error:
    kobject_put(kobj_swpld1); 
    i2c_unregister_device(pdata[swpld1].client);
    i2c_put_adapter(parent);
    return -ENODEV;
}

static int __init swpld2_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD2 platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS3);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", BUS1);
        return -ENODEV;
    }

    pdata[swpld2].client = i2c_new_dummy(parent, pdata[swpld2].reg_addr);
    if (!pdata[swpld2].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[swpld2].reg_addr);
        goto error;
    }

    kobj_swpld2 = &pdev->dev.kobj;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld2_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld attribute group");
        goto error;
    }

    return 0;

error:
    kobject_put(kobj_swpld2); 
    i2c_unregister_device(pdata[swpld2].client);
    i2c_put_adapter(parent);

    return -ENODEV;
}

static int __init swpld3_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD3 platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS3);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", BUS1);
        return -ENODEV;
    }

    pdata[swpld3].client = i2c_new_dummy(parent, pdata[swpld3].reg_addr);
    if (!pdata[swpld3].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[swpld3].reg_addr);
        goto error;
    }

    kobj_swpld3 = &pdev->dev.kobj;
    ret = sysfs_create_group(&pdev->dev.kobj, &swpld3_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld attribute group");
        goto error;
    }

    return 0;

error:
    kobject_put(kobj_swpld3); 
    i2c_unregister_device(pdata[swpld3].client);
    i2c_put_adapter(parent);

    return -ENODEV;
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[system_cpld].client) {
            if (!parent) {
                parent = (pdata[system_cpld].client)->adapter;
            }
        i2c_unregister_device(pdata[system_cpld].client);
        }
    }
    i2c_put_adapter(parent);

    return 0;
}

static int __exit swpld1_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &swpld1_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[swpld1].client) {
            if (!parent) {
                parent = (pdata[swpld1].client)->adapter;
            }
        i2c_unregister_device(pdata[swpld1].client);
        }
    }
    i2c_put_adapter(parent);
    return 0;
}

static int __exit swpld2_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &swpld2_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[swpld2].client) {
            if (!parent) {
                parent = (pdata[swpld2].client)->adapter;
            }
        i2c_unregister_device(pdata[swpld2].client);
        }
    }
    i2c_put_adapter(parent);
    return 0;
}

static int __exit swpld3_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &swpld3_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else {
        if (pdata[swpld3].client) {
            if (!parent) {
                parent = (pdata[swpld3].client)->adapter;
            }
        i2c_unregister_device(pdata[swpld3].client);
        }
    }
    i2c_put_adapter(parent);
    return 0;
}

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-cpld",
    },
};

static struct platform_driver swpld1_driver = {
    .probe  = swpld1_probe,
    .remove = __exit_p(swpld1_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-swpld1",
    },
};

static struct platform_driver swpld2_driver = {
    .probe  = swpld2_probe,
    .remove = __exit_p(swpld2_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-swpld2",
    },
};

static struct platform_driver swpld3_driver = {
    .probe  = swpld3_probe,
    .remove = __exit_p(swpld3_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-swpld3",
    },
};
/*----------------    CPLD  - end   ------------- */
/*----------------    MUX   - start   ------------- */

struct cpld_mux_platform_data {
    int parent;
    int base_nr;
    struct i2c_client *cpld;
    int reg_addr;
};

struct cpld_mux {
    struct i2c_adapter *parent;
    struct i2c_adapter **child;
    struct cpld_mux_platform_data data;
};

static struct cpld_mux_platform_data ag9032v2a_cpld_mux_platform_data[] = {
    {
        .parent         = BUS0,
        .base_nr        = BUS0_BASE_NUM,
        .cpld           = NULL,
        .reg_addr       = BUS0_MUX_REG, 
    },
};

static struct cpld_mux_platform_data ag9032v2a_swpld_mux_platform_data[] = {
    {
        .parent         = BUS3,
        .base_nr        = BUS3_BASE_NUM,
        .cpld           = NULL,
        .reg_addr       = BUS3_MUX_REG, 
    },
};

static struct platform_device cpld_mux_device[] = 
{
    {
        .name           = "delta-ag9032v2a-cpld-mux",
        .id             = 0,
        .dev            = {
                .platform_data   = &ag9032v2a_cpld_mux_platform_data[0],
                .release         = device_release,
        },
    },
};

static struct platform_device swpld1_mux_device[] = 
{
    {
        .name           = "delta-ag9032v2a-swpld1-mux",
        .id             = 0,
        .dev            = {
                .platform_data   = &ag9032v2a_swpld_mux_platform_data[0],
                .release         = device_release,
        },
    },
};

static int cpld_reg_write_byte(struct i2c_client *client, u8 regaddr, u8 val)
{
    union i2c_smbus_data data;

    data.byte = val;
    return client->adapter->algo->smbus_xfer(client->adapter, client->addr,
                                             client->flags,
                                             I2C_SMBUS_WRITE,
                                             regaddr, I2C_SMBUS_BYTE_DATA, &data);
}

static int cpld_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_mux  *mux = i2c_mux_priv(muxc);
    u8 cpld_mux_val = 0;
    int ret = 0; 
    if ( mux->data.base_nr == BUS0_BASE_NUM ){
        switch (chan) {
            case 0:
                cpld_mux_val = MUX_VAL_IDEEPROM;
                break;
            case 1:
                cpld_mux_val = MUX_VAL_PCA9548;
                break;
            case 2:
                cpld_mux_val = MUX_VAL_SWPLD;
                break;
            default:
                cpld_mux_val = 0x0;
                break;
        }
    }
    else
    {
        printk(KERN_ERR "CPLD mux select error\n");
        return 0;
    }
    ret = cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(cpld_mux_val & 0xff));
    return ret;
}

static int swpld_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct cpld_mux  *mux = i2c_mux_priv(muxc);
    u8 swpld_mux_val = 0;
    int ret =0;

    if ( mux->data.base_nr == BUS3_BASE_NUM ){
        swpld_mux_val = chan;
    }
    else
    {
        printk(KERN_ERR "SWPLD mux select error\n");
        return 0;
    }
    ret = cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(swpld_mux_val & 0xff));
    return ret;
}

static int __init cpld_mux_probe(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc;
    struct cpld_mux *mux;
    struct cpld_mux_platform_data *pdata;
    struct i2c_adapter *parent;
    int i, ret, dev_num;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLD platform data not found\n");
        return -ENODEV;
    }
    mux = kzalloc(sizeof(*mux), GFP_KERNEL);
    if (!mux) {
        printk(KERN_ERR "Failed to allocate memory for mux\n");
        return -ENOMEM;
    }
    mux->data = *pdata;
    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        kfree(mux);
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n", pdata->parent);
        return -ENODEV;
    }
    /* Judge bus number to decide how many devices*/
    switch (pdata->parent) {
        case BUS0:
            dev_num = BUS0_DEV_NUM;
            break;
        default :
            dev_num = DEF_DEV_NUM;
            break;
    }
    muxc = i2c_mux_alloc(parent, &pdev->dev, dev_num, 0, 0, cpld_mux_select, NULL);
    if (!muxc) {
        ret = -ENOMEM;
        goto alloc_failed;
    }
    muxc->priv = mux;
    platform_set_drvdata(pdev, muxc);

    for (i = 0; i < dev_num; i++)
    {
        int nr = pdata->base_nr + i;
        unsigned int class = 0;
        ret = i2c_mux_add_adapter(muxc, nr, i, class);
        if (ret) {
            dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
            goto add_adapter_failed;
        }
    }
    dev_info(&pdev->dev, "%d port mux on %s adapter\n", dev_num, parent->name);
    return 0;

add_adapter_failed:
    i2c_mux_del_adapters(muxc);
alloc_failed:
    kfree(mux);
    i2c_put_adapter(parent);
    return ret;
}

static int __init swpld_mux_probe(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc;
    struct cpld_mux *mux;
    struct cpld_mux_platform_data *pdata;
    struct i2c_adapter *parent;
    int i, ret, dev_num;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD platform data not found\n");
        return -ENODEV;
    }
    mux = kzalloc(sizeof(*mux), GFP_KERNEL);
    if (!mux) {
        printk(KERN_ERR "Failed to allocate memory for mux\n");
        return -ENOMEM;
    }
    mux->data = *pdata;
    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        kfree(mux);
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n", pdata->parent);
        return -ENODEV;
    }
    /* Judge bus number to decide how many devices*/
    switch (pdata->parent) {
        case BUS3:
            dev_num = BUS3_DEV_NUM;
            break;
        default :
            dev_num = DEF_DEV_NUM;
            break;
    }

    muxc = i2c_mux_alloc(parent, &pdev->dev, dev_num, 0, 0, swpld_mux_select, NULL);
    if (!muxc) {
        ret = -ENOMEM;
        goto alloc_failed;
    }
    muxc->priv = mux;
    platform_set_drvdata(pdev, muxc);
    for (i = 0; i < dev_num; i++) 
    {
        int nr = pdata->base_nr + i;
        unsigned int class = 0;
       	ret = i2c_mux_add_adapter(muxc, nr, i, class);
        if (ret) {
            dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
            goto add_adapter_failed;
        }
    }
    dev_info(&pdev->dev, "%d port mux on %s adapter\n", dev_num, parent->name);
    return 0;

add_adapter_failed:
    i2c_mux_del_adapters(muxc);
alloc_failed:
    kfree(mux);
    i2c_put_adapter(parent);

    return ret;
}

static int __exit cpld_mux_remove(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc  = platform_get_drvdata(pdev);
    struct i2c_adapter *parent = muxc->parent;

    i2c_mux_del_adapters(muxc);
    i2c_put_adapter(parent);

    return 0;
}

static int __exit swpld_mux_remove(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc  = platform_get_drvdata(pdev);
    struct i2c_adapter *parent = muxc->parent;

    i2c_mux_del_adapters(muxc);
    i2c_put_adapter(parent);

    return 0;
}

static struct platform_driver cpld_mux_driver = {
    .probe  = cpld_mux_probe,
    .remove = __exit_p(cpld_mux_remove), /* TODO */
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-cpld-mux",
    },
};

static struct platform_driver swpld1_mux_driver = {
    .probe  = swpld_mux_probe,
    .remove = __exit_p(swpld_mux_remove), /* TODO */
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v2a-swpld1-mux",
    },
};
/*----------------    MUX   - end   ------------- */

/*----------------   module initialization     ------------- */

static int __init delta_ag9032v2a_platform_init(void)
{
//    struct i2c_client *client;
    struct i2c_adapter *adapter;
    struct cpld_mux_platform_data *cpld_mux_pdata;
    struct cpld_platform_data     *cpld_pdata;
    struct cpld_mux_platform_data *swpld_mux_pdata;
    struct cpld_platform_data     *swpld_pdata;
    int ret,i = 0;
    
    mutex_init(&dni_lock);
    printk("ag9032v2a_platform module initialization\n");

    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpupld driver\n");
        goto error_cpld_driver;
    }
    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld1_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld1_driver;
    }

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld2_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld2_driver;
    }

    // set the SWPLD prob and remove
    ret = platform_driver_register(&swpld3_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld driver\n");
        goto error_swpld3_driver;
    }

    // register the mux prob which call the SWPLD
    ret = platform_driver_register(&cpld_mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld mux driver\n");
        goto error_cpld_mux_driver;
    }

    // register the mux prob which call the SWPLD
    ret = platform_driver_register(&swpld1_mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld1 mux driver\n");
        goto error_swpld1_mux_driver;
    }

    // register the i2c devices    
    ret = platform_driver_register(&i2c_device_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_driver;
    }

    // register the CPUPLD
    ret = platform_device_register(&cpld_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpupld device\n");
        goto error_cpld_device;
    }

    // link the CPLD and the Mux
    cpld_pdata = ag9032v2a_cpld_platform_data;
    for (i = 0; i < ARRAY_SIZE(cpld_mux_device); i++)
    {
        cpld_mux_pdata = cpld_mux_device[i].dev.platform_data;
        cpld_mux_pdata->cpld = cpld_pdata[system_cpld].client;
        ret = platform_device_register(&cpld_mux_device[i]);
        if (ret) {
            printk(KERN_WARNING "Fail to create swpld mux %d\n", i);
            goto error_cpld_mux;
        }
    }

    adapter = i2c_get_adapter(BUS2);
    i2c_client_9548 = i2c_new_device(adapter, &i2c_info_pca9548[0]);
    i2c_put_adapter(adapter);

    // register the SWPLD
    ret = platform_device_register(&swpld1_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld1 device\n");
        goto error_swpld1_device;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld2_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld2 device\n");
        goto error_swpld2_device;
    }

    // register the SWPLD
    ret = platform_device_register(&swpld3_device);
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld3 device\n");
        goto error_swpld3_device;
    }

    // link the SWPLD1 and the Mux
    swpld_pdata = ag9032v2a_swpld1_platform_data;
    for (i = 0; i < ARRAY_SIZE(swpld1_mux_device); i++)
    {
        swpld_mux_pdata = swpld1_mux_device[i].dev.platform_data;
        swpld_mux_pdata->cpld = swpld_pdata[swpld1].client;
        ret = platform_device_register(&swpld1_mux_device[i]);
        if (ret) {
            printk(KERN_WARNING "Fail to create swpld mux %d\n", i);
            goto error_ag9032v2a_swpld1_mux;
        }
    }    

    for (i = 0; i < ARRAY_SIZE(ag9032v2a_i2c_device); i++)
    {
        ret = platform_device_register(&ag9032v2a_i2c_device[i]);
        if (ret) 
        {
            printk(KERN_WARNING "Fail to create i2c device %d\n", i);
            goto error_ag9032v2a_i2c_device;
        }
    }
    if (ret)
        goto error_cpld_mux;
    return 0;

error_ag9032v2a_i2c_device:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&ag9032v2a_i2c_device[i]);
    }
    i = ARRAY_SIZE(swpld1_mux_device);
error_ag9032v2a_swpld1_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&swpld1_mux_device[i]);
    }
    platform_device_unregister(&swpld3_device);
error_swpld3_device:
    platform_device_unregister(&swpld2_device);
error_swpld2_device:
    platform_device_unregister(&swpld1_device);
error_swpld1_device:
    i2c_unregister_device(i2c_client_9548);
    i = ARRAY_SIZE(cpld_mux_device);
error_cpld_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&cpld_mux_device[i]);
    }
    platform_device_unregister(&cpld_device);
error_cpld_device:
    platform_driver_unregister(&i2c_device_driver);
error_i2c_device_driver:
    platform_driver_unregister(&swpld1_mux_driver);
error_swpld1_mux_driver:
    platform_driver_unregister(&cpld_mux_driver);
error_cpld_mux_driver:
    platform_driver_unregister(&swpld3_driver);
error_swpld3_driver:
    platform_driver_unregister(&swpld2_driver);
error_swpld2_driver:
    platform_driver_unregister(&swpld1_driver);
error_swpld1_driver:
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
}

static void __exit delta_ag9032v2a_platform_exit(void)
{
    int i = 0;

    for (i = 0; i < ARRAY_SIZE(ag9032v2a_i2c_device); i++) {
        platform_device_unregister(&ag9032v2a_i2c_device[i]);
    }   

    for (i = 0; i < ARRAY_SIZE(swpld1_mux_device); i++) {
        platform_device_unregister(&swpld1_mux_device[i]);
    }

    platform_device_unregister(&swpld1_device);
    platform_driver_unregister(&swpld1_driver);

    platform_device_unregister(&swpld2_device);
    platform_driver_unregister(&swpld2_driver);

    platform_device_unregister(&swpld3_device);
    platform_driver_unregister(&swpld3_driver);

    i2c_unregister_device(i2c_client_9548);

    for (i = 0; i < ARRAY_SIZE(cpld_mux_device); i++) {
        platform_device_unregister(&cpld_mux_device[i]);
    }

    platform_driver_unregister(&i2c_device_driver);
    platform_driver_unregister(&swpld1_mux_driver);
    platform_driver_unregister(&cpld_mux_driver);
    platform_device_unregister(&cpld_device);
    platform_driver_unregister(&cpld_driver);    
}

module_init(delta_ag9032v2a_platform_init);
module_exit(delta_ag9032v2a_platform_exit);

MODULE_DESCRIPTION("DELTA ag9032v2a Platform Support");
MODULE_AUTHOR("Johnson Lu <johnson.lu@deltaww.com>");
MODULE_LICENSE("GPL");
