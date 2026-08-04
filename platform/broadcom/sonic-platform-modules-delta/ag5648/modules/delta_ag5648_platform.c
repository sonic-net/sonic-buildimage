#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/platform_data/pca954x.h>
#include <linux/i2c-mux.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/i2c/sff-8436.h>

#define BUS4_DEV_NUM    54
#define DEFAULT_NUM      1
#define BUS4_BASE_NUM   10
#define BUS4_MUX_REG  0x18

#define TEMP_FAN_VAL  0x06
#define FANIO_CTL_VAL 0x07
#define FAN_CTRL_VAL  0x05
#define PSU1_VAL      0x00
#define PSU2_VAL      0x20
#define HOT_SWAP1_VAL 0x10
#define HOT_SWAP2_VAL 0x30

#define SYSTEM_CPLD_REG  0x31
#define MASTER_CPLD_REG  0x35
#define SLAVE_CPLD_REG   0x39

#define FAN_LED_REG      0x05
#define LED_REG          0x04

#define SFP_PRESENCE_1   0x08
#define SFP_PRESENCE_2   0x09
#define SFP_PRESENCE_3   0x0a
#define SFP_PRESENCE_4   0x0b
#define SFP_PRESENCE_5   0x0c
#define SFP_PRESENCE_6   0x08
#define SFP_PRESENCE_7   0x09
#define QSFP_PRESENCE    0x12
        
#define QSFP_RESPONSE    0x10
#define QSFP_LP_MODE     0x11
#define QSFP_RESET       0x13        


#define SFF8436_INFO(data) \
    .type = "sff8436", .addr = 0x50, .platform_data = (data)

#define SFF_8436_PORT(eedata) \
    .byte_len = 256, .page_size = 1, .flags = SFF_8436_FLAG_READONLY
	
#define ag5648_i2c_device_num(NUM){                                           \
        .name                   = "delta-ag5648-i2c-device",                  \
        .id                     = NUM,                                        \
        .dev                    = {                                           \
                    .platform_data = &ag5648_i2c_device_platform_data[NUM],   \
                    .release       = device_release,                          \
        },                                                                    \
}

/*Define struct to get client of i2c_new_deivce */
struct i2c_client * i2c_client_9548;

enum{
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
};

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
/*----------------   I2C device   - start   ------------- */
static void device_release(struct device *dev)
{
    return;
}

struct i2c_device_platform_data {
    int parent;
    struct i2c_board_info           info;
    struct i2c_client              *client;
};
/* pca9548 - add 8 bus */
static struct pca954x_platform_mode pca954x_mode[] = {
  { .adap_id = 2,
    .deselect_on_exit = 1,
  },
  { .adap_id = 3,
    .deselect_on_exit = 1,
  },
  { .adap_id = 4,
    .deselect_on_exit = 1,
  },
  { .adap_id = 5,
    .deselect_on_exit = 1,
  },
  { .adap_id = 6,
    .deselect_on_exit = 1,
  },
  { .adap_id = 7,
    .deselect_on_exit = 1,
  },
  { .adap_id = 8,
    .deselect_on_exit = 1,
  },
  { .adap_id = 9,
    .deselect_on_exit = 1,
  },
};

static struct pca954x_platform_data pca954x_data = {
  .modes = pca954x_mode,
  .num_modes = ARRAY_SIZE(pca954x_mode),
};

static struct i2c_board_info __initdata i2c_info_pca9548[] =
{
        {
            I2C_BOARD_INFO("pca9548", 0x70),
            .platform_data = &pca954x_data, 
        },
};


static struct sff_8436_platform_data sff_8436_port[] = {
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
    { SFF_8436_PORT() },
};


static struct i2c_device_platform_data ag5648_i2c_device_platform_data[] = {
    {
        /* id eeprom (0x53) */
        .parent = 2,
        .info = { I2C_BOARD_INFO("24c02", 0x53) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4d) */
        .parent = 2,
        .info = { I2C_BOARD_INFO("tmp75", 0x4d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x49) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("tmp75", 0x49) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4b) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("tmp75", 0x4b) },
    },
    {
        /* tmp75 (0x4c) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("tmp75", 0x4c) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4e) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("tmp75", 0x4e) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4f) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("tmp75", 0x4f) },
    },
    {
        /* tmp75 (0x4d) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("emc2305", 0x4d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4d) */
        .parent = 5,
        .info = { I2C_BOARD_INFO("emc2305", 0x4d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x40) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("ltc4215", 0x40) },
        .client = NULL,
    },
    {
        /* tmp75 (0x42) */
        .parent = 3,
        .info = { I2C_BOARD_INFO("ltc4215", 0x42) },
    },
    {
        /* psu1 (0x59) */
        .parent = 6,
        .info = { I2C_BOARD_INFO("dni_ag5648_psu", 0x59) },
        .client = NULL,
    },
    {
        /* psu2 (0x58) */
        .parent = 6,
        .info = { I2C_BOARD_INFO("dni_ag5648_psu", 0x58) },
    },
    {
        /* qsfp 1 (0x50) */
        .parent = 10,
        .info = { SFF8436_INFO(&sff_8436_port[0]) },
        .client = NULL,
    },
    {
        /* qsfp 2 (0x50) */
        .parent = 11,
        .info = { SFF8436_INFO(&sff_8436_port[1]) },
        .client = NULL,
    },
    {
        /* qsfp 3 (0x50) */
        .parent = 12,
        .info = { SFF8436_INFO(&sff_8436_port[2]) },
        .client = NULL,
    },
    {
        /* qsfp 4 (0x50) */
        .parent = 13,
        .info = { SFF8436_INFO(&sff_8436_port[3]) },
        .client = NULL,
    },
    {
        /* qsfp 5 (0x50) */
        .parent = 14,
        .info = { SFF8436_INFO(&sff_8436_port[4]) },
        .client = NULL,
    },
    {
        /* qsfp 6 (0x50) */
        .parent = 15,
        .info = { SFF8436_INFO(&sff_8436_port[5]) },
        .client = NULL,
    },
    {
        /* qsfp 7 (0x50) */
        .parent = 16,
        .info = { SFF8436_INFO(&sff_8436_port[6]) },
        .client = NULL,
    },
    {
        /* qsfp 8 (0x50) */
        .parent = 17,
        .info = { SFF8436_INFO(&sff_8436_port[7]) },
        .client = NULL,
    },
    {
        /* qsfp 9 (0x50) */
        .parent = 18,
        .info = { SFF8436_INFO(&sff_8436_port[8]) },
        .client = NULL,
    },
    {
        /* qsfp 10 (0x50) */
        .parent = 19,
        .info = { SFF8436_INFO(&sff_8436_port[9]) },
        .client = NULL,
    },
    {
        /* qsfp 11 (0x50) */
        .parent = 20,
        .info = { SFF8436_INFO(&sff_8436_port[10]) },
        .client = NULL,
    },
    {
        /* qsfp 12 (0x50) */
        .parent = 21,
        .info = { SFF8436_INFO(&sff_8436_port[11]) },
        .client = NULL,
    },
    {
        /* qsfp 13 (0x50) */
        .parent = 22,
        .info = { SFF8436_INFO(&sff_8436_port[12]) },
        .client = NULL,
    },
    {
        /* qsfp 14 (0x50) */
        .parent = 23,
        .info = { SFF8436_INFO(&sff_8436_port[13]) },
        .client = NULL,
    },
    {
        /* qsfp 15 (0x50) */
        .parent = 24,
        .info = { SFF8436_INFO(&sff_8436_port[14]) },
        .client = NULL,
    },
    {
        /* qsfp 16 (0x50) */
        .parent = 25,
        .info = { SFF8436_INFO(&sff_8436_port[15]) },
        .client = NULL,
    },
    {
        /* qsfp 17 (0x50) */
        .parent = 26,
        .info = { SFF8436_INFO(&sff_8436_port[16]) },
        .client = NULL,
    },
    {
        /* qsfp 18 (0x50) */
        .parent = 27,
        .info = { SFF8436_INFO(&sff_8436_port[17]) },
        .client = NULL,
    },
    {
        /* qsfp 19 (0x50) */
        .parent = 28,
        .info = { SFF8436_INFO(&sff_8436_port[18]) },
        .client = NULL,
    },
    {
        /* qsfp 20 (0x50) */
        .parent = 29,
        .info = { SFF8436_INFO(&sff_8436_port[19]) },
        .client = NULL,
    },
    {
        /* qsfp 21 (0x50) */
        .parent = 30,
        .info = { SFF8436_INFO(&sff_8436_port[20]) },
        .client = NULL,
    },
    {
        /* qsfp 22 (0x50) */
        .parent = 31,
        .info = { SFF8436_INFO(&sff_8436_port[21]) },
        .client = NULL,
    },
    {
        /* qsfp 23 (0x50) */
        .parent = 32,
        .info = { SFF8436_INFO(&sff_8436_port[22]) },
        .client = NULL,
    },
    {
        /* qsfp 24 (0x50) */
        .parent = 33,
        .info = { SFF8436_INFO(&sff_8436_port[23]) },
        .client = NULL,
    },
    {
        /* qsfp 25 (0x50) */
        .parent = 34,
        .info = { SFF8436_INFO(&sff_8436_port[24]) },
        .client = NULL,
    },
    {
        /* qsfp 26 (0x50) */
        .parent = 35,
        .info = { SFF8436_INFO(&sff_8436_port[25]) },
        .client = NULL,
    },
    {
        /* qsfp 27 (0x50) */
        .parent = 36,
        .info = { SFF8436_INFO(&sff_8436_port[26]) },
        .client = NULL,
    },
    {
        /* qsfp 28 (0x50) */
        .parent = 37,
        .info = { SFF8436_INFO(&sff_8436_port[27]) },
        .client = NULL,
    },
    {
        /* qsfp 29 (0x50) */
        .parent = 38,
        .info = { SFF8436_INFO(&sff_8436_port[28]) },
        .client = NULL,
    },
    {
        /* qsfp 30 (0x50) */
        .parent = 39,
        .info = { SFF8436_INFO(&sff_8436_port[29]) },
        .client = NULL,
    },
    {
        /* qsfp 31 (0x50) */
        .parent = 40,
        .info = { SFF8436_INFO(&sff_8436_port[30]) },
        .client = NULL,
    },
    {
        /* qsfp 32 (0x50) */
        .parent = 41,
        .info = { SFF8436_INFO(&sff_8436_port[31]) },
        .client = NULL,
    },
    {
        /* qsfp 33 (0x50) */
        .parent = 42,
        .info = { SFF8436_INFO(&sff_8436_port[32]) },
        .client = NULL,
    },
    {
        /* qsfp 34 (0x50) */
        .parent = 43,
        .info = { SFF8436_INFO(&sff_8436_port[33]) },
        .client = NULL,
    },
    {
        /* qsfp 35 (0x50) */
        .parent = 44,
        .info = { SFF8436_INFO(&sff_8436_port[34]) },
        .client = NULL,
    },
    {
        /* qsfp 36 (0x50) */
        .parent = 45,
        .info = { SFF8436_INFO(&sff_8436_port[35]) },
        .client = NULL,
    },
    {
        /* qsfp 37 (0x50) */
        .parent = 46,
        .info = { SFF8436_INFO(&sff_8436_port[36]) },
        .client = NULL,
    },
    {
        /* qsfp 38 (0x50) */
        .parent = 47,
        .info = { SFF8436_INFO(&sff_8436_port[37]) },
        .client = NULL,
    },
    {
        /* qsfp 39 (0x50) */
        .parent = 48,
        .info = { SFF8436_INFO(&sff_8436_port[38]) },
        .client = NULL,
    },
    {
        /* qsfp 40 (0x50) */
        .parent = 49,
        .info = { SFF8436_INFO(&sff_8436_port[39]) },
        .client = NULL,
    },
    {
        /* qsfp 41 (0x50) */
        .parent = 50,
        .info = { SFF8436_INFO(&sff_8436_port[40]) },
        .client = NULL,
    },
    {
        /* qsfp 42 (0x50) */
        .parent = 51,
        .info = { SFF8436_INFO(&sff_8436_port[41]) },
        .client = NULL,
    },
    {
        /* qsfp 43 (0x50) */
        .parent = 52,
        .info = { SFF8436_INFO(&sff_8436_port[42]) },
        .client = NULL,
    },
    {
        /* qsfp 44 (0x50) */
        .parent = 53,
        .info = { SFF8436_INFO(&sff_8436_port[43]) },
        .client = NULL,
    },
    {
        /* qsfp 45 (0x50) */
        .parent = 54,
        .info = { SFF8436_INFO(&sff_8436_port[44]) },
        .client = NULL,
    },
    {
        /* qsfp 46 (0x50) */
        .parent = 55,
        .info = { SFF8436_INFO(&sff_8436_port[45]) },
        .client = NULL,
    },
    {
        /* qsfp 47 (0x50) */
        .parent = 56,
        .info = { SFF8436_INFO(&sff_8436_port[46]) },
        .client = NULL,
    },
    {
        /* qsfp 48 (0x50) */
        .parent = 57,
        .info = { SFF8436_INFO(&sff_8436_port[47]) },
        .client = NULL,
    },
    {
        /* qsfp 49 (0x50) */
        .parent = 58,
        .info = { SFF8436_INFO(&sff_8436_port[48]) },
        .client = NULL,
    },
    {
        /* qsfp 50 (0x50) */
        .parent = 59,
        .info = { SFF8436_INFO(&sff_8436_port[49]) },
        .client = NULL,
    },
    {
        /* qsfp 51 (0x50) */
        .parent = 60,
        .info = { SFF8436_INFO(&sff_8436_port[50]) },
        .client = NULL,
    },
    {
        /* qsfp 52 (0x50) */
        .parent = 61,
        .info = { SFF8436_INFO(&sff_8436_port[51]) },
        .client = NULL,
    },
    {
        /* qsfp 53 (0x50) */
        .parent = 62,
        .info = { SFF8436_INFO(&sff_8436_port[52]) },
        .client = NULL,
    },
    {
        /* qsfp 54 (0x50) */
        .parent = 63,
        .info = { SFF8436_INFO(&sff_8436_port[53]) },
        .client = NULL,
    },
};


static struct platform_device ag5648_i2c_device[] = {
    ag5648_i2c_device_num(0),
    ag5648_i2c_device_num(1),
    ag5648_i2c_device_num(2),
    ag5648_i2c_device_num(3),
    ag5648_i2c_device_num(4),
    ag5648_i2c_device_num(5),
    ag5648_i2c_device_num(6),
    ag5648_i2c_device_num(7),
    ag5648_i2c_device_num(8),
    ag5648_i2c_device_num(9),
    ag5648_i2c_device_num(10),
    ag5648_i2c_device_num(11),
    ag5648_i2c_device_num(12),   
    ag5648_i2c_device_num(13),
    ag5648_i2c_device_num(14),
    ag5648_i2c_device_num(15),
    ag5648_i2c_device_num(16),
    ag5648_i2c_device_num(17),
    ag5648_i2c_device_num(18),
    ag5648_i2c_device_num(19),
    ag5648_i2c_device_num(20),
    ag5648_i2c_device_num(21),
    ag5648_i2c_device_num(22),
    ag5648_i2c_device_num(23),
    ag5648_i2c_device_num(24),
    ag5648_i2c_device_num(25),
    ag5648_i2c_device_num(26),
    ag5648_i2c_device_num(27),
    ag5648_i2c_device_num(28),
    ag5648_i2c_device_num(29),
    ag5648_i2c_device_num(30),
    ag5648_i2c_device_num(31),
    ag5648_i2c_device_num(32),
    ag5648_i2c_device_num(33),
    ag5648_i2c_device_num(34),
    ag5648_i2c_device_num(35),
    ag5648_i2c_device_num(36),
    ag5648_i2c_device_num(37),
    ag5648_i2c_device_num(38),
    ag5648_i2c_device_num(39),
    ag5648_i2c_device_num(40),
    ag5648_i2c_device_num(41),
    ag5648_i2c_device_num(42),
    ag5648_i2c_device_num(43),
    ag5648_i2c_device_num(44),
    ag5648_i2c_device_num(45),
    ag5648_i2c_device_num(46),
    ag5648_i2c_device_num(47),
    ag5648_i2c_device_num(48),
    ag5648_i2c_device_num(49),
    ag5648_i2c_device_num(50),
    ag5648_i2c_device_num(51),
    ag5648_i2c_device_num(52),
    ag5648_i2c_device_num(53),
    ag5648_i2c_device_num(54),
    ag5648_i2c_device_num(55),
    ag5648_i2c_device_num(56),
    ag5648_i2c_device_num(57),
    ag5648_i2c_device_num(58),
    ag5648_i2c_device_num(59),
    ag5648_i2c_device_num(60),
    ag5648_i2c_device_num(61),
    ag5648_i2c_device_num(62),
    ag5648_i2c_device_num(63),
    ag5648_i2c_device_num(64),
    ag5648_i2c_device_num(65),
    ag5648_i2c_device_num(66),
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
        .name = "delta-ag5648-i2c-device",
    }
};

/*----------------   I2C driver   - end   ------------- */

/*----------------    CPLD - start   ------------- */

/*    CPLD  -- device   */

enum cpld_type {
    system_cpld,
    master_cpld,
    slave_cpld,
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

static struct cpld_platform_data ag5648_cpld_platform_data[] = {
    [system_cpld] = {
        .reg_addr = SYSTEM_CPLD_REG,
    },
    [master_cpld] = {
        .reg_addr = MASTER_CPLD_REG,
    },
    [slave_cpld] = {
        .reg_addr = SLAVE_CPLD_REG,
    },
};

static struct platform_device ag5648_cpld = {
    .name               = "delta-ag5648-cpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag5648_cpld_platform_data,
                .release         = device_release
    },
};

static ssize_t get_present(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u32 data = 0;
    u32 data2 = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[slave_cpld].client, SFP_PRESENCE_1);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u32)(ret & 0xff);

    ret = i2c_smbus_read_byte_data(pdata[slave_cpld].client, SFP_PRESENCE_2);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(ret & 0xff) << 8;

    ret = i2c_smbus_read_byte_data(pdata[slave_cpld].client, SFP_PRESENCE_3);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(ret & 0xff) << 16;

    ret = i2c_smbus_read_byte_data(pdata[slave_cpld].client, SFP_PRESENCE_4);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(ret & 0xff) << 24;

    ret = i2c_smbus_read_byte_data(pdata[slave_cpld].client, SFP_PRESENCE_5);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data2 = ((u32)(ret & 0xf)) ;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, SFP_PRESENCE_6);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data2 |= (u32)(ret & 0xff) << 4;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, SFP_PRESENCE_7);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data2 |= (u32)((ret >> 4) & 0xf) << 12;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, QSFP_PRESENCE);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data2 |= (u32)(ret & 0x3f) << 16;

    return sprintf(buf, "0x%06x%x\n", data2, data);
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, QSFP_LP_MODE);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = ((u8)ret & 0x3f) ;

    return sprintf(buf, "0x%02x%012x\n", data, 0);
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;

    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;
    
    data = data >> 48;
    i2c_smbus_write_byte_data(pdata[master_cpld].client, QSFP_LP_MODE, (u8)(data & 0xff));

    return count;
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, QSFP_RESET);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);

    data = ((u8)ret & 0x3f);

    return sprintf(buf, "0x%02x%012x\n", data, 0);
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;
    
    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;
    data = data >> 48;
    i2c_smbus_write_byte_data(pdata[master_cpld].client, QSFP_RESET, (u8)(data & 0xff));

    return count;
}

static ssize_t get_response(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[master_cpld].client, QSFP_RESPONSE);

    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u8)ret & 0x3f;

    return sprintf(buf, "0x%02x%012x\n", data, 0);
}

static ssize_t set_response(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;
    data = data >> 48;
    i2c_smbus_write_byte_data(pdata[master_cpld].client, QSFP_RESPONSE, (u8)(data & 0xff));

    return count;
}

struct platform_led_status{
	int reg_data;
	char *led_status;
	int led_id;
};
	
static struct platform_led_status led_info[] = {
    {
        .reg_data = 0x00,//0000 0000
        .led_status = "fan_off",
        .led_id = 0,
    },
    {
        .reg_data = 1 << 6,//0100 0000
        .led_status = "fan_Amber",
        .led_id = 0,
    },
    {
        .reg_data = 2 << 6,//1000 0000
        .led_status = "fan_green",
        .led_id = 0,
    },
    {
        .reg_data = 3 << 6,//1100 0000
        .led_status = "fan_Blinking_yellow",
        .led_id = 0,
    },
    {
        .reg_data = 0x00,
        .led_status = "sys_Blinking_green",
        .led_id = 1,
    },
    {
        .reg_data = 1 << 4,
        .led_status = "sys_green",
        .led_id = 1,
    },
    {
        .reg_data = 2 << 4,
        .led_status = "sys_Amber",
        .led_id = 1,
    },
    {
        .reg_data = 3 << 4 ,
        .led_status = "sys_Amber",
        .led_id = 1,
    },
    {
    	.reg_data = 0x00,
    	.led_status = "pwr_off",
    	.led_id = 2,
    },
    {
    	.reg_data = 1 << 1,
    	.led_status = "pwr_Amber",
    	.led_id = 2,
    },
    {
    	.reg_data = 2 << 1,
    	.led_status = "pwr_green",
    	.led_id = 2,
    },
    {
    	.reg_data = 3 << 1,
    	.led_status = "pwr_Blinking_Amber",
    	.led_id = 2,
    },
    {
        .reg_data = 0x00,
        .led_status = "fan4_off",
        .led_id = 3,
    },
    {
        .reg_data = 1 << 6,
        .led_status = "fan4_green",
        .led_id = 3,
    },
    {
        .reg_data = 2 << 6,
        .led_status = "fan4_Amber",
        .led_id = 3,
    },
    {
        .reg_data = 0x00,
        .led_status = "fan3_off",
        .led_id = 4,
    },
    {
        .reg_data = 1 << 4,
        .led_status = "fan3_green",
        .led_id = 4,
    },
    {
        .reg_data = 2 << 4,
        .led_status = "fan3_Amber",
        .led_id = 4,
    },
    {
        .reg_data = 0x00,
        .led_status = "fan2_off",
        .led_id = 5,
    },
    {
        .reg_data = 1 << 2,
        .led_status = "fan2_green",
        .led_id = 5,
    },
    {
        .reg_data = 1 << 3,
        .led_status = "fan2_Amber",
        .led_id = 5,
    },
    {
        .reg_data = 0x00,
        .led_status = "fan1_off",
        .led_id = 6,
    },
    {
        .reg_data = 1,
        .led_status = "fan1_green",
        .led_id = 6,
    },
    {
        .reg_data = 2,
        .led_status = "fan1_Amber",
        .led_id = 6,
    },
};
	
struct platform_led_data{
	int reg_addr;	
	int mask;		
};
	
static struct platform_led_data led_data[] = {	
    {
	.reg_addr = LED_REG,    //0x04
	.mask = 0xc0,//1100 0000		
    },
    {
	.reg_addr = LED_REG,
        .mask = 0x30,//0011 0000	
    },
    {
	.reg_addr = LED_REG,
        .mask = 0x06,//0000 0110	
    },
    {
        .reg_addr = FAN_LED_REG,//0x05
        .mask = 0xc0,//1100 0000        
    },
    {
        .reg_addr = FAN_LED_REG,
        .mask = 0x30,//0011 0000    
    },
    {
        .reg_addr = FAN_LED_REG,
        .mask = 0x0c,//0000 1100    
    },
    {
        .reg_addr = FAN_LED_REG,
        .mask = 0x03,//0000 0011
    },  
};
	

static ssize_t get_led_color(struct device *dev, struct device_attribute *devattr, char *buf)
{
    char str[9][20] = {0};
    int board_data;
    int led_data_number;
    int led_info_number;
    struct cpld_platform_data *pdata = dev->platform_data;

    for(led_data_number = 0; led_data_number < ARRAY_SIZE(led_data); led_data_number++){	
        board_data = i2c_smbus_read_byte_data(pdata[slave_cpld].client, led_data[led_data_number].reg_addr);	
        if(board_data >= 0){
                board_data &= led_data[led_data_number].mask;		
                for(led_info_number = 0; led_info_number < ARRAY_SIZE(led_info); led_info_number++){
                    if (led_data_number == led_info[led_info_number].led_id \
                          && board_data == led_info[led_info_number].reg_data){
                        sprintf(str[led_data_number], "%s", led_info[led_info_number].led_status);
                    }		
                }
        }
        else
            printk( KERN_ERR "Missing LED board data\n");
        }		    		
	return sprintf(buf,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n",str[0],str[1],str[2],str[3],str[4],str[5],str[6]);
}

static ssize_t set_led_color(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
	int led_info_number;
	int led_data_number;
	int str_compar;
	int led_reg_value;
    struct cpld_platform_data *pdata = dev->platform_data;
	
	for(led_info_number = 0; led_info_number < ARRAY_SIZE(led_info); led_info_number++){
		str_compar = strncmp(buf,led_info[led_info_number].led_status,strlen(led_info[led_info_number].led_status));
		if(str_compar == 0){									
			for(led_data_number = 0; led_data_number < ARRAY_SIZE(led_data); led_data_number++){				
			    if(led_info[led_info_number].led_id == led_data_number){
					led_reg_value = i2c_smbus_read_byte_data(pdata[slave_cpld].client, led_data[led_data_number].reg_addr);
                    if(led_reg_value >= 0){					
			            led_reg_value &= (~led_data[led_data_number].mask);					
				        led_reg_value |= led_info[led_info_number].reg_data;					
					    i2c_smbus_write_byte_data(pdata[slave_cpld].client, (u8)(led_data[led_data_number].reg_addr & 0xff), (u8)(led_reg_value & 0xff));
					}
					else
						printk( KERN_ERR "Missing LED reg. data\n");
				}
		    }
		}
	}
    return count;
}

static DEVICE_ATTR(sfp_response, S_IWUSR | S_IRUGO, get_response,  set_response );
static DEVICE_ATTR(sfp_present,  S_IRUGO,           get_present,   NULL         );
static DEVICE_ATTR(sfp_lpmode,   S_IWUSR | S_IRUGO, get_lpmode,    set_lpmode   );
static DEVICE_ATTR(sfp_reset,    S_IWUSR | S_IRUGO, get_reset,     set_reset    );

static DEVICE_ATTR(led_control,  S_IRUGO | S_IWUSR, get_led_color, set_led_color);

static struct attribute *ag5648_cpld_attrs[] = {
    &dev_attr_sfp_response.attr,    
    &dev_attr_sfp_present.attr,
    &dev_attr_sfp_lpmode.attr,
    &dev_attr_sfp_reset.attr,
    &dev_attr_led_control.attr,
    NULL,
};

static struct attribute_group ag5648_cpld_attr_grp = {
    .attrs = ag5648_cpld_attrs,
};

/*    CPLD  -- driver   */
static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret,i;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS2);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n",BUS2);
        return -ENODEV;
    }

    for (i = 0; i < ARRAY_SIZE(ag5648_cpld_platform_data); i++) {
        pdata[i].client = i2c_new_dummy(parent, pdata[i].reg_addr);
        if (!pdata[i].client) {
            printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[i].reg_addr);
            goto error;
        }
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &ag5648_cpld_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    return 0;

error:    
    i--;
    for (; i >= 0; i--) {
        if (pdata[i].client) {
            i2c_unregister_device(pdata[i].client);
        }
    }
    i2c_put_adapter(parent);
    
    return -ENODEV; 
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    int i;
    sysfs_remove_group(&pdev->dev.kobj, &ag5648_cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } 
    else {
        for (i = 0; i < ARRAY_SIZE(ag5648_cpld_platform_data); i++) {
            if (pdata[i].client) {
                if (!parent) {
                    parent = (pdata[i].client)->adapter;
                }
                i2c_unregister_device(pdata[i].client);
            }
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
        .name   = "delta-ag5648-cpld",
    },
};

/*----------------    CPLD  - end   ------------- */

/*----------------    MUX   - start   ------------- */

struct swpld_mux_platform_data {
    int parent;
    int base_nr;
    int reg_addr;
    struct i2c_client *cpld;
};

struct swpld_mux {
    struct i2c_adapter *parent;
    struct i2c_adapter **child;
    struct swpld_mux_platform_data data;
};

static struct swpld_mux_platform_data ag5648_swpld_mux_platform_data[] = {
    {
        .parent         = BUS4, 
        .base_nr        = BUS4_BASE_NUM, 
        .cpld           = NULL,
        .reg_addr       = BUS4_MUX_REG ,
    },
};


static struct platform_device ag5648_swpld_mux[] = {
    {
        .name           = "delta-ag5648-swpld-mux",
        .id             = 0,
        .dev            = {
                .platform_data   = &ag5648_swpld_mux_platform_data[0],
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

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)
static int swpld_mux_select(struct i2c_adapter *adap, void *data, u8 chan)
{
    struct swpld_mux *mux = data;
    u8 swpld_mux_val=0; 
    if ( mux->data.base_nr == BUS4_BASE_NUM )
    {
        swpld_mux_val = (chan + 0x01);
    }
    else
    {
        swpld_mux_val = 0x00;
    }
    swpld_mux_val=swpld_mux_val & (u8)0x3F;

    return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(swpld_mux_val & 0xff));
}
#else // #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static int swpld_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct swpld_mux *mux = i2c_mux_priv(muxc);
    u8 swpld_mux_val=0; 
    if ( mux->data.base_nr == BUS4_BASE_NUM )
    {
        swpld_mux_val = (chan + 0x01);
    }
    else
    {
        swpld_mux_val = 0x00;
    }
    swpld_mux_val=swpld_mux_val & (u8)0x3F;

    return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(swpld_mux_val & 0xff));
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)
static int __init swpld_mux_probe(struct platform_device *pdev)
{
    struct swpld_mux *mux;
    struct swpld_mux_platform_data *pdata;
    struct i2c_adapter *parent;
    int i, ret, dev_num;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n", pdata->parent);
        return -ENODEV;
    }

    /* Judge bus number to decide how many devices*/
    switch (pdata->parent) {
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        default :
            dev_num = DEFAULT_NUM;  
            break;
    }

    mux = kzalloc(sizeof(*mux), GFP_KERNEL);
    if (!mux) {
        ret = -ENOMEM;
        printk(KERN_ERR "Failed to allocate memory for mux\n");
        goto alloc_failed;
    }

    mux->parent = parent;
    mux->data = *pdata;
    mux->child = kzalloc(sizeof(struct i2c_adapter *) * dev_num, GFP_KERNEL);
    if (!mux->child) {
        ret = -ENOMEM;
        printk(KERN_ERR "Failed to allocate memory for device on mux\n");
        goto alloc_failed2;
    }

    for (i = 0; i < dev_num; i++) {
        int nr = pdata->base_nr + i;
        unsigned int class = 0;

        mux->child[i] = i2c_add_mux_adapter(parent, &pdev->dev, mux,
                           nr, i, class,
                           swpld_mux_select, NULL);
        if (!mux->child[i]) {
            ret = -ENODEV;
            dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
            goto add_adapter_failed;
        }
    }

    platform_set_drvdata(pdev, mux);
    return 0;

add_adapter_failed:
    for (; i > 0; i--)
        i2c_del_mux_adapter(mux->child[i - 1]);
    kfree(mux->child);
alloc_failed2:
    kfree(mux);
alloc_failed:
    i2c_put_adapter(parent);

    return ret;
}
#else // #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static int __init swpld_mux_probe(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc;
    struct swpld_mux *mux;
    struct swpld_mux_platform_data *pdata;
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
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        default :
            dev_num = DEFAULT_NUM;  
            break;
    }

    muxc = i2c_mux_alloc(parent, &pdev->dev, dev_num, 0, 0,
                                            swpld_mux_select, NULL);
    if (!muxc) {
        ret = -ENOMEM;
        goto alloc_failed;
    }
    muxc->priv = mux;
    platform_set_drvdata(pdev, muxc);


    for (i = 0; i < dev_num; i++) {
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
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)
static int __exit swpld_mux_remove(struct platform_device *pdev)
{
    int i;
    struct swpld_mux *mux = platform_get_drvdata(pdev);
    struct swpld_mux_platform_data *pdata;
    struct i2c_adapter *parent;
    int dev_num;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "SWPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(pdata->parent);
    if (!parent) {
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n",
            pdata->parent);
        return -ENODEV;
    }
    switch (pdata->parent) {
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        default :
            dev_num = DEFAULT_NUM;  
            break;
    }

    for (i = 0; i < dev_num; i++)
        i2c_del_mux_adapter(mux->child[i]);

    platform_set_drvdata(pdev, NULL);
    i2c_put_adapter(mux->parent);
    kfree(mux->child);
    kfree(mux);

    return 0;
}
#else // #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static int __exit swpld_mux_remove(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc = platform_get_drvdata(pdev);

    i2c_mux_del_adapters(muxc);
    i2c_put_adapter(muxc->parent);

    return 0;
}
#endif

static struct platform_driver swpld_mux_driver = {
    .probe  = swpld_mux_probe,
    .remove = __exit_p(swpld_mux_remove), /* TODO */
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag5648-swpld-mux",
    },
};
/*----------------    MUX   - end   ------------- */


/*----------------   module initialization     ------------- */
static int __init delta_ag5648_platform_init(void)
{
    //struct i2c_client *client;
    struct i2c_adapter *adapter;
    struct cpld_platform_data *cpld_pdata;
    struct swpld_mux_platform_data *swpld_pdata;
    int ret,i = 0;

    //Use pca9548 in i2c_mux_pca954x.c
    adapter = i2c_get_adapter(BUS1); 
    
    i2c_client_9548 = i2c_new_device(adapter, &i2c_info_pca9548[0]);
    i2c_put_adapter(adapter);

    // set the CPLD prob and remove
    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpld driver\n");
        goto error_cpld_driver;
    }

    // register the mux prob which call the CPLD
    ret = platform_driver_register(&swpld_mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld mux driver\n");
        goto error_swpld_mux_driver;
    }

    // register the i2c devices    
    ret = platform_driver_register(&i2c_device_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_driver;
    }

    // register the CPLD
    ret = platform_device_register(&ag5648_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_ag5648_cpld;
    }
    // link the CPLD and the Mux
    cpld_pdata = ag5648_cpld.dev.platform_data;


    swpld_pdata = ag5648_swpld_mux[0].dev.platform_data;
    swpld_pdata->cpld = cpld_pdata[master_cpld].client;  
    ret = platform_device_register(&ag5648_swpld_mux[0]);          
    if (ret) {
        printk(KERN_WARNING "Fail to create swpld mux %d\n", i);
        goto error_ag5648_swpld_mux;
    }
    
    for (i = 0; i < ARRAY_SIZE(ag5648_i2c_device); i++)
    {
        ret = platform_device_register(&ag5648_i2c_device[i]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", i);
            goto error_ag5648_i2c_device;
        }
    }

    if (ret)
        goto error_ag5648_swpld_mux;
    return 0;

error_ag5648_i2c_device:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&ag5648_i2c_device[i]);
    }
    i = ARRAY_SIZE(ag5648_swpld_mux);    
error_ag5648_swpld_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&ag5648_swpld_mux[0]);
    }
    platform_device_unregister(&ag5648_cpld);
error_ag5648_cpld:
    platform_driver_unregister(&i2c_device_driver);
error_i2c_device_driver:
    platform_driver_unregister(&swpld_mux_driver);
error_swpld_mux_driver:
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
}

static void __exit delta_ag5648_platform_exit(void)
{
    int i = 0;

    for ( i = 0; i < ARRAY_SIZE(ag5648_i2c_device); i++ ) {
        platform_device_unregister(&ag5648_i2c_device[i]);
    }

    for (i = 0; i < ARRAY_SIZE(ag5648_swpld_mux); i++) {
        platform_device_unregister(&ag5648_swpld_mux[i]);
    }

    platform_device_unregister(&ag5648_cpld);
    platform_driver_unregister(&i2c_device_driver);
    platform_driver_unregister(&cpld_driver);
    platform_driver_unregister(&swpld_mux_driver);
    i2c_unregister_device(i2c_client_9548);
}

module_init(delta_ag5648_platform_init);
module_exit(delta_ag5648_platform_exit);

MODULE_DESCRIPTION("DNI ag5648 Platform Support");
MODULE_AUTHOR("Neal Tai <neal.tai@deltaww.com>");
MODULE_LICENSE("GPL");    
