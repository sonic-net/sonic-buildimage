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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define BUS3_DEV_NUM 9
#define BUS4_DEV_NUM 6
#define BUS5_DEV_NUM 32
#define DEFAULT_NUM  1
#define BUS3_BASE_NUM 30
#define BUS4_BASE_NUM 40
#define BUS5_BASE_NUM 50

#define BUS3_MUX_REG 0x21
#define BUS4_MUX_REG 0x21
#define BUS5_MUX_REG 0x20

#define TEMP_FAN_VAL  0x06
#define FANIO_CTL_VAL 0x07
#define FAN_CTRL_VAL  0x05
#define PSU1_VAL      0x00
#define PSU2_VAL      0x20
#define HOT_SWAP1_VAL 0x10
#define HOT_SWAP2_VAL 0x30
#define FAN_EEPROM1_VAL   0x00
#define FAN_EEPROM2_VAL   0x01
#define FAN_EEPROM3_VAL   0x02
#define FAN_EEPROM4_VAL   0x03
#define FAN_EEPROM5_VAL   0x04

#define SWPLD_REG         0x31
#define SWPLD_SFP_MUX_REG 0x20

#define SYS_LED_REG      0x1C
#define FAN1_LED_REG     0x1D
#define FAN2_LED_REG     0x1E

#define SFP_PRESENCE_1    0x38
#define SFP_PRESENCE_2    0x39
#define SFP_PRESENCE_3    0x3A
#define SFP_PRESENCE_4    0x3B

#define SFP_LP_MODE_1     0x34
#define SFP_LP_MODE_2     0x35
#define SFP_LP_MODE_3     0x36
#define SFP_LP_MODE_4     0x37

#define SFP_RESET_1       0x3C
#define SFP_RESET_2       0x3D
#define SFP_RESET_3       0x3E
#define SFP_RESET_4       0x3F

#define SFP_RESPONSE_1    0x30
#define SFP_RESPONSE_2    0x31
#define SFP_RESPONSE_3    0x32
#define SFP_RESPONSE_4    0x33

#define SFF8436_INFO(data) \
    .type = "sff8436", .addr = 0x50, .platform_data = (data)

#define SFF_8346_PORT(eedata) \
    .byte_len = 256, .page_size = 1, .flags = SFF_8436_FLAG_READONLY
	
#define ag9032v1_i2c_device_num(NUM){                                         \
        .name                   = "delta-ag9032v1-i2c-device",                \
        .id                     = NUM,                                        \
        .dev                    = {                                           \
                    .platform_data = &ag9032v1_i2c_device_platform_data[NUM], \
                    .release       = device_release,                          \
        },                                                                    \
}

/*Define struct to get client of i2c_new_deivce */
struct i2c_client * i2c_client_9547;

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
/* pca9547 - add 8 bus */
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

static struct i2c_board_info __initdata i2c_info_pca9547[] =
{
        {
            I2C_BOARD_INFO("pca9547", 0x71),
            .platform_data = &pca954x_data, 
        },
};


static struct sff_8436_platform_data sff_8436_port[] = {
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
    { SFF_8346_PORT() },
};

static struct i2c_device_platform_data ag9032v1_i2c_device_platform_data[] = {
    {
        /* tmp75 (0x4d) */
        .parent = 2,
        .info = { I2C_BOARD_INFO("tmp75", 0x4d) },
        .client = NULL,
    },
    {
        /* id eeprom (0x53) */
        .parent = 2,
        .info = { I2C_BOARD_INFO("24c02", 0x53) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4c) */
        .parent = 7,
        .info = { I2C_BOARD_INFO("tmp75", 0x4c) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4d) */
        .parent = 7,
        .info = { I2C_BOARD_INFO("tmp75", 0x4d) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4e) */
        .parent = 7,
        .info = { I2C_BOARD_INFO("tmp75", 0x4e) },
        .client = NULL,
    },
    { 
        /* tmp75 (0x4f) */
        .parent = 30, 
        .info = { I2C_BOARD_INFO("tmp75", 0x4f) },
        .client = NULL,
    },
    { 
        /* FAN 1 Controller (0x2c) */
        .parent = 37, 
        .info = { I2C_BOARD_INFO("emc2305", 0x2c) },
        .client = NULL,
    },
    { 
        /* FAN 2 Controller (0x2d) */
        .parent = 38, 
        .info = { I2C_BOARD_INFO("emc2305", 0x2d) },
        .client = NULL,
    },
    {
        /* psu 1 (0x58) */
        .parent = 40,
        .info = { .type = "dni_ag9032v1_psu", .addr = 0x58, .platform_data = (void *) 0 },
        .client = NULL,
    },
    {
        /* psu 2 (0x58) */
        .parent = 41,
        .info = { .type = "dni_ag9032v1_psu", .addr = 0x58, .platform_data = (void *) 1 },
        .client = NULL,
    },
    {
        /* hot-swap 1 (0x40) */
        .parent = 42,
        .info = { .type = "ltc4215", .addr = 0x40, .platform_data = (void *) 0 },
        .client = NULL,
    },
    {
        /* hot-swap 2 (0x40) */
        .parent = 43,
        .info = { .type = "ltc4215", .addr = 0x40, .platform_data = (void *) 1 },
        .client = NULL,
    },
    {
        /* qsfp 1 (0x50) */
        .parent = 50,
        .info = { SFF8436_INFO(&sff_8436_port[0]) },
        .client = NULL,
    },
    {
        /* qsfp 2 (0x50) */
        .parent = 51,
        .info = { SFF8436_INFO(&sff_8436_port[1]) },
        .client = NULL,
    },
    {
        /* qsfp 3 (0x50) */
        .parent = 52,
        .info = { SFF8436_INFO(&sff_8436_port[2]) },
        .client = NULL,
    },
    {
        /* qsfp 4 (0x50) */
        .parent = 53,
        .info = { SFF8436_INFO(&sff_8436_port[3]) },
        .client = NULL,
    },
    {
        /* qsfp 5 (0x50) */
        .parent = 54,
        .info = { SFF8436_INFO(&sff_8436_port[4]) },
        .client = NULL,
    },
    {
        /* qsfp 6 (0x50) */
        .parent = 55,
        .info = { SFF8436_INFO(&sff_8436_port[5]) },
        .client = NULL,
    },
    {
        /* qsfp 7 (0x50) */
        .parent = 56,
        .info = { SFF8436_INFO(&sff_8436_port[6]) },
        .client = NULL,
    },
    {
        /* qsfp 8 (0x50) */
        .parent = 57,
        .info = { SFF8436_INFO(&sff_8436_port[7]) },
        .client = NULL,
    },
    {
        /* qsfp 9 (0x50) */
        .parent = 58,
        .info = { SFF8436_INFO(&sff_8436_port[8]) },
        .client = NULL,
    },
    {
        /* qsfp 10 (0x50) */
        .parent = 59,
        .info = { SFF8436_INFO(&sff_8436_port[9]) },
        .client = NULL,
    },
    {
        /* qsfp 11 (0x50) */
        .parent = 60,
        .info = { SFF8436_INFO(&sff_8436_port[10]) },
        .client = NULL,
    },
    {
        /* qsfp 12 (0x50) */
        .parent = 61,
        .info = { SFF8436_INFO(&sff_8436_port[11]) },
        .client = NULL,
    },
    {
        /* qsfp 13 (0x50) */
        .parent = 62,
        .info = { SFF8436_INFO(&sff_8436_port[12]) },
        .client = NULL,
    },
    {
        /* qsfp 14 (0x50) */
        .parent = 63,
        .info = { SFF8436_INFO(&sff_8436_port[13]) },
        .client = NULL,
    },
    {
        /* qsfp 15 (0x50) */
        .parent = 64,
        .info = { SFF8436_INFO(&sff_8436_port[14]) },
        .client = NULL,
    },
    {
        /* qsfp 16 (0x50) */
        .parent = 65,
        .info = { SFF8436_INFO(&sff_8436_port[15]) },
        .client = NULL,
    },
    {
        /* qsfp 17 (0x50) */
        .parent = 66,
        .info = { SFF8436_INFO(&sff_8436_port[16]) },
        .client = NULL,
    },
    {
        /* qsfp 18 (0x50) */
        .parent = 67,
        .info = { SFF8436_INFO(&sff_8436_port[17]) },
        .client = NULL,
    },
    {
        /* qsfp 19 (0x50) */
        .parent = 68,
        .info = { SFF8436_INFO(&sff_8436_port[18]) },
        .client = NULL,
    },
    {
        /* qsfp 20 (0x50) */
        .parent = 69,
        .info = { SFF8436_INFO(&sff_8436_port[19]) },
        .client = NULL,
    },
    {
        /* qsfp 21 (0x50) */
        .parent = 70,
        .info = { SFF8436_INFO(&sff_8436_port[20]) },
        .client = NULL,
    },
    {
        /* qsfp 22 (0x50) */
        .parent = 71,
        .info = { SFF8436_INFO(&sff_8436_port[21]) },
        .client = NULL,
    },
    {
        /* qsfp 23 (0x50) */
        .parent = 72,
        .info = { SFF8436_INFO(&sff_8436_port[22]) },
        .client = NULL,
    },
    {
        /* qsfp 24 (0x50) */
        .parent = 73,
        .info = { SFF8436_INFO(&sff_8436_port[23]) },
        .client = NULL,
    },
    {
        /* qsfp 25 (0x50) */
        .parent = 74,
        .info = { SFF8436_INFO(&sff_8436_port[24]) },
        .client = NULL,
    },
    {
        /* qsfp 26 (0x50) */
        .parent = 75,
        .info = { SFF8436_INFO(&sff_8436_port[25]) },
        .client = NULL,
    },
    {
        /* qsfp 27 (0x50) */
        .parent = 76,
        .info = { SFF8436_INFO(&sff_8436_port[26]) },
        .client = NULL,
    },
    {
        /* qsfp 28 (0x50) */
        .parent = 77,
        .info = { SFF8436_INFO(&sff_8436_port[27]) },
        .client = NULL,
    },
    {
        /* qsfp 29 (0x50) */
        .parent = 78,
        .info = { SFF8436_INFO(&sff_8436_port[28]) },
        .client = NULL,
    },
    {
        /* qsfp 30 (0x50) */
        .parent = 79,
        .info = { SFF8436_INFO(&sff_8436_port[29]) },
        .client = NULL,
    },
    {
        /* qsfp 31 (0x50) */
        .parent = 80,
        .info = { SFF8436_INFO(&sff_8436_port[30]) },
        .client = NULL,
    },
    {
        /* qsfp 32 (0x50) */
        .parent = 81,
        .info = { SFF8436_INFO(&sff_8436_port[31]) },
        .client = NULL,
    },
};


static struct platform_device ag9032v1_i2c_device[] = {
    ag9032v1_i2c_device_num(0),
    ag9032v1_i2c_device_num(1),
    ag9032v1_i2c_device_num(2),
    ag9032v1_i2c_device_num(3),
    ag9032v1_i2c_device_num(4),
    ag9032v1_i2c_device_num(5),
    ag9032v1_i2c_device_num(6),
    ag9032v1_i2c_device_num(7),
    ag9032v1_i2c_device_num(8),
    ag9032v1_i2c_device_num(9),
    ag9032v1_i2c_device_num(10),
    ag9032v1_i2c_device_num(11),
    ag9032v1_i2c_device_num(12),
    ag9032v1_i2c_device_num(13),
    ag9032v1_i2c_device_num(14),
    ag9032v1_i2c_device_num(15),
    ag9032v1_i2c_device_num(16),
    ag9032v1_i2c_device_num(17),
    ag9032v1_i2c_device_num(18),
    ag9032v1_i2c_device_num(19),
    ag9032v1_i2c_device_num(20),
    ag9032v1_i2c_device_num(21),
    ag9032v1_i2c_device_num(22),
    ag9032v1_i2c_device_num(23),
    ag9032v1_i2c_device_num(24),
    ag9032v1_i2c_device_num(25),
    ag9032v1_i2c_device_num(26),
    ag9032v1_i2c_device_num(27),
    ag9032v1_i2c_device_num(28),
    ag9032v1_i2c_device_num(29),
    ag9032v1_i2c_device_num(30),
    ag9032v1_i2c_device_num(31),
    ag9032v1_i2c_device_num(32),
    ag9032v1_i2c_device_num(33),
    ag9032v1_i2c_device_num(34),
    ag9032v1_i2c_device_num(35),
    ag9032v1_i2c_device_num(36),
    ag9032v1_i2c_device_num(37),
    ag9032v1_i2c_device_num(38),
    ag9032v1_i2c_device_num(39),
    ag9032v1_i2c_device_num(40),
    ag9032v1_i2c_device_num(41),
    ag9032v1_i2c_device_num(42),
    ag9032v1_i2c_device_num(43),
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
        .name = "delta-ag9032v1-i2c-device",
    }
};

/*----------------   I2C driver   - end   ------------- */

/*----------------    CPLD - start   ------------- */

/*    CPLD  -- device   */

enum cpld_type {
    system_cpld,
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

enum swpld_attributes {
    SW_BOARD_ID,
    SW_BOARD_VER,
    SWPLD_VER,
    SYS_RST,
    B56960_RST,
    MB_A_PLD_RST,
    MB_B_PLD_RST,
    PSU1_PWR_OK,
    PSU2_PWR_OK,
    HS1_PWR_OK,
    HS2_PWR_OK,
    B54616_RST,
    B54616_INT,
    B54616_MASK_INT,
    PB_HS_INT,
    MB_HS_INT,
    PB_PWR_INT,
    MB_PWR_INT,
    FAN_INT,
    PB_HS_MASK_INT,
    MB_HS_MASK_INT,
    PB_PWR1_MASK_INT,
    PB_PWR2_MASK_INT,
    FAN_MASK_INT,
    QSFP_01TO08_INT,
    QSFP_08TO16_INT,
    QSFP_17TO24_INT,
    QSFP_25TO32_INT,
    QSFP_01TO08_ABS,
    QSFP_08TO16_ABS,
    QSFP_17TO24_ABS,
    QSFP_25TO32_ABS,
    QSFP_01TO08_MASK_INT,
    QSFP_08TO16_MASK_INT,
    QSFP_17TO24_MASK_INT,
    QSFP_25TO32_MASK_INT,
    QSFP_01TO08_MASK_ABS,
    QSFP_08TO16_MASK_ABS,
    QSFP_17TO24_MASK_ABS,
    QSFP_25TO32_MASK_ABS,
    QSFP01_MOD_INT,
    QSFP02_MOD_INT,
    QSFP03_MOD_INT,
    QSFP04_MOD_INT,
    QSFP05_MOD_INT,
    QSFP06_MOD_INT,
    QSFP07_MOD_INT,
    QSFP08_MOD_INT,
    QSFP09_MOD_INT,
    QSFP10_MOD_INT,
    QSFP11_MOD_INT,
    QSFP12_MOD_INT,
    QSFP13_MOD_INT,
    QSFP14_MOD_INT,
    QSFP15_MOD_INT,
    QSFP16_MOD_INT,
    QSFP17_MOD_INT,
    QSFP18_MOD_INT,
    QSFP19_MOD_INT,
    QSFP20_MOD_INT,
    QSFP21_MOD_INT,
    QSFP22_MOD_INT,
    QSFP23_MOD_INT,
    QSFP24_MOD_INT,
    QSFP25_MOD_INT,
    QSFP26_MOD_INT,
    QSFP27_MOD_INT,
    QSFP28_MOD_INT,
    QSFP29_MOD_INT,
    QSFP30_MOD_INT,
    QSFP31_MOD_INT,
    QSFP32_MOD_INT,
};

static struct cpld_platform_data ag9032v1_cpld_platform_data[] = {
    [system_cpld] = {
        .reg_addr = SWPLD_REG,
    },
};

static struct platform_device ag9032v1_cpld = {
    .name               = "delta-ag9032v1-swpld",
    .id                 = 0,
    .dev                = {
                .platform_data   = ag9032v1_cpld_platform_data,
                .release         = device_release,
    },
};

static struct swpld_attribute_data {   
    int reg_addr;
    int reg_mask;
    char reg_note[150];
};

static struct swpld_attribute_data controller_interrupt_data[] = {
//BOARD
    [SYS_RST] = {
        .reg_addr = 0x04,
        .reg_mask = 7,
        .reg_note = "“1” = Normal operation\n“0” = Reset"
    },
    [B56960_RST] = {
        .reg_addr = 0x04,
        .reg_mask = 6,
        .reg_note = "“1” = Normal operation\n“0” = Reset"
    },
    [MB_A_PLD_RST] = {
        .reg_addr = 0x04,
        .reg_mask = 4,
        .reg_note = "“1” = Normal operation\n“0” = Reset"
    },
    [MB_B_PLD_RST] = {
        .reg_addr = 0x04,
        .reg_mask = 3,
        .reg_note = "“1” = Normal operation\n“0” = Reset"
    },
//PSU
    [PSU1_PWR_OK] = {
        .reg_addr = 0x0a,
        .reg_mask = 7,
        .reg_note = "‘0’ = Power rail is failed\n‘1’ = Power rail is good"
    },
    [PSU2_PWR_OK] = {
        .reg_addr = 0x0a,
        .reg_mask = 6,
        .reg_note = "‘0’ = Power rail is failed\n‘1’ = Power rail is good"
    },
//HOT SWAP
    [HS1_PWR_OK] = {
        .reg_addr = 0x08,
        .reg_mask = 5,
        .reg_note = "‘0’ = Hot swap controller disabled\n‘1’ = Hot swap controller enabled"
    },
    [HS2_PWR_OK] = {
        .reg_addr = 0x08,
        .reg_mask = 4,
        .reg_note = "‘0’ = Hot swap controller disabled\n‘1’ = Hot swap controller enabled"
    },
//BCM54616S
    [B54616_RST] = {
        .reg_addr = 0x04,
        .reg_mask = 5,
        .reg_note = "“0” = Reset\n“1” = Normal operation"
    },
    [B54616_INT] = {
        .reg_addr = 0x16,
        .reg_mask = 7,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [B54616_MASK_INT] = {
        .reg_addr = 0x17,
        .reg_mask = 7,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
//QSFP
    [PB_HS_INT] = {
        .reg_addr = 0x10,
        .reg_mask = 7,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [MB_HS_INT] = {
        .reg_addr = 0x10,
        .reg_mask = 6,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [PB_PWR_INT] = {
        .reg_addr = 0x10,
        .reg_mask = 5,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [MB_PWR_INT] = {
        .reg_addr = 0x10,
        .reg_mask = 4,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [FAN_INT] = {
        .reg_addr = 0x10,
        .reg_mask = 3,
        .reg_note = "‘0’ = Interrupt occurs\n‘1’ = Interrupt doesn’t occur"
    },
    [PB_HS_MASK_INT] = {
        .reg_addr = 0x11,
        .reg_mask = 7,
        .reg_note = "‘0’ = Interrupt doesn’t masked\n‘1’ = Interrupt masked"
    },
    [MB_HS_MASK_INT] = {
        .reg_addr = 0x11,
        .reg_mask = 6,
        .reg_note = "‘0’ = Interrupt doesn’t masked\n‘1’ = Interrupt masked"
    },
    [PB_PWR1_MASK_INT] = {
        .reg_addr = 0x11,
        .reg_mask = 5,
        .reg_note = "‘0’ = Interrupt doesn’t masked\n‘1’ = Interrupt masked"
    },
    [PB_PWR2_MASK_INT] = {
        .reg_addr = 0x11,
        .reg_mask = 4,
        .reg_note = "‘0’ = Interrupt doesn’t masked\n‘1’ = Interrupt masked"
    },
    [FAN_MASK_INT] = {
        .reg_addr = 0x11,
        .reg_mask = 3,
        .reg_note = "‘0’ = Interrupt doesn’t masked\n‘1’ = Interrupt masked"
    },
    [QSFP_01TO08_INT] = {
        .reg_addr = 0x12,
        .reg_mask = 7,
        .reg_note = "“0” = Interrupt occurs\n“1” = Interrupt doesn’t occur"
    },
    [QSFP_08TO16_INT] = {
        .reg_addr = 0x12,
        .reg_mask = 6,
        .reg_note = "“0” = Interrupt occurs\n“1” = Interrupt doesn’t occur"
    },
    [QSFP_17TO24_INT] = {
        .reg_addr = 0x12,
        .reg_mask = 5,
        .reg_note = "“0” = Interrupt occurs\n“1” = Interrupt doesn’t occur"
    },
    [QSFP_25TO32_INT] = {
        .reg_addr = 0x12,
        .reg_mask = 4,
        .reg_note = "“0” = Interrupt occurs\n“1” = Interrupt doesn’t occur"
    },
    [QSFP_01TO08_ABS] = {
        .reg_addr = 0x12,
        .reg_mask = 3,
        .reg_note = "“0” = Absence status change\n“1” = Absence status not changes"
    },
    [QSFP_08TO16_ABS] = {
        .reg_addr = 0x12,
        .reg_mask = 2,
        .reg_note = "“0” = Absence status change\n“1” = Absence status not changes"
    },
    [QSFP_17TO24_ABS] = {
        .reg_addr = 0x12,
        .reg_mask = 1,
        .reg_note = "“0” = Absence status change\n“1” = Absence status not changes"
    },
    [QSFP_25TO32_ABS] = {
        .reg_addr = 0x12,
        .reg_mask = 0,
        .reg_note = "“0” = Absence status change\n“1” = Absence status not changes"
    },
    [QSFP_01TO08_MASK_INT] = {
        .reg_addr = 0x13,
        .reg_mask = 7,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_08TO16_MASK_INT] = {
        .reg_addr = 0x13,
        .reg_mask = 6,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_17TO24_MASK_INT] = {
        .reg_addr = 0x13,
        .reg_mask = 5,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_25TO32_MASK_INT] = {
        .reg_addr = 0x13,
        .reg_mask = 4,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_01TO08_MASK_ABS] = {
        .reg_addr = 0x13,
        .reg_mask = 3,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_08TO16_MASK_ABS] = {
        .reg_addr = 0x13,
        .reg_mask = 2,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_17TO24_MASK_ABS] = {
        .reg_addr = 0x13,
        .reg_mask = 1,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP_25TO32_MASK_ABS] = {
        .reg_addr = 0x13,
        .reg_mask = 0,
        .reg_note = "“0” = Interrupt doesn’t masked\n“1” = Interrupt masked"
    },
    [QSFP01_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 7,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP02_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 6,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP03_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 5,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP04_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 4,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP05_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 3,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP06_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 2,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP07_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 1,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP08_MOD_INT] = {
        .reg_addr = 0x40,
        .reg_mask = 0,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP09_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 7,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP10_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 6,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP11_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 5,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP12_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 4,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP13_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 3,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP14_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 2,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP15_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 1,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP16_MOD_INT] = {
        .reg_addr = 0x41,
        .reg_mask = 0,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP17_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 7,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP18_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 6,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP19_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 5,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP20_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 4,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP21_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 3,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP22_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 2,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP23_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 1,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP24_MOD_INT] = {
        .reg_addr = 0x42,
        .reg_mask = 0,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP25_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 7,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP26_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 6,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP27_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 5,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP28_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 4,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP29_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 3,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP30_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 2,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP31_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 1,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
    [QSFP32_MOD_INT] = {
        .reg_addr = 0x43,
        .reg_mask = 0,
        .reg_note = "“0” = The module issue the interrupt\n“1” = The module NOT issue the interrupt"
    },
};

static ssize_t get_present(struct device *dev, struct device_attribute \
                            *dev_attr, char *buf)
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_PRESENCE_1);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u32)reverse_8bits(ret) & 0xff;
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_PRESENCE_2);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 8;
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_PRESENCE_3);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 16;
        
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_PRESENCE_4);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 24;
        
    return sprintf(buf, "0x%08x\n", data); //return 32bits data
}

static ssize_t get_lpmode(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_LP_MODE_1);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u32)(reverse_8bits(ret) & 0xff);
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_LP_MODE_2);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 8;
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_LP_MODE_3);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 16;
        
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_LP_MODE_4);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 24;
        
    return sprintf(buf, "0x%08x\n", data); //return 32bits data
}

static ssize_t set_lpmode(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;
    
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_LP_MODE_1, (u8)reverse_8bits(data & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_LP_MODE_2, (u8)(reverse_8bits(data >> 8) & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_LP_MODE_3, (u8)(reverse_8bits(data >> 16) & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_LP_MODE_4, (u8)(reverse_8bits(data >> 24) & 0xff));

    return count;
}

static ssize_t get_reset(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESET_1);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u32)(reverse_8bits(ret) & 0xff);
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESET_2);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 8;
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESET_3);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 16;
        
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESET_4);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 24;
 
    return sprintf(buf, "0x%08x\n", data); //return 32bits data
}

static ssize_t set_reset(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESET_1, (u8)reverse_8bits(data & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESET_2, (u8)reverse_8bits((data >> 8)& 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESET_3, (u8)reverse_8bits((data >> 16) & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESET_4, (u8)reverse_8bits((data >> 24) & 0xff));

    return count;
}

static ssize_t get_response(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    u32 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESPONSE_1);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data = (u32)(reverse_8bits(ret) & 0xff);
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESPONSE_2);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 8;
 
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESPONSE_3);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 16;
        
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, SFP_RESPONSE_4);
    if (ret < 0)
        return sprintf(buf, "error number(%d)",ret);
    data |= (u32)(reverse_8bits(ret) & 0xff) << 24;
 
    return sprintf(buf, "0x%08x\n", data); //return 32bits data
}

static ssize_t set_response(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count) 
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 16, &data);
    if (err)
        return err;

    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESPONSE_1, (u8)reverse_8bits(data & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESPONSE_2, (u8)reverse_8bits((data >> 8)& 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESPONSE_3, (u8)reverse_8bits((data >> 16) & 0xff));
    i2c_smbus_write_byte_data(pdata[system_cpld].client, SFP_RESPONSE_4, (u8)reverse_8bits((data >> 24) & 0xff));

    return count;
}
	
struct platform_led_status{
	int reg_data;
	char *led_status;
	int led_id;
};
	
static struct platform_led_status led_info[] = {
    {
		.reg_data = 0x40,
		.led_status = "pwr1_green",
		.led_id = 0,
	},
	{
		.reg_data = 0x80,
		.led_status = "pwr1_amber",
		.led_id = 0,
	},
	{
		.reg_data = 0x00,
		.led_status = "pwr1_off",
		.led_id = 0,
	},
	{
		.reg_data = 0xc0,
		.led_status = "pwr1_off",
		.led_id = 0,
	},
    {
		.reg_data = 0x10,
		.led_status = "pwr2_green",
		.led_id = 1,
	},
	{
		.reg_data = 0x20,
		.led_status = "pwr2_amber",
		.led_id = 1,
	},
	{
		.reg_data = 0x00,
		.led_status = "pwr2_off",
		.led_id = 1,
	},
	{
		.reg_data = 0x30,
		.led_status = "pwr2_off",
		.led_id = 1,
	},
	{
		.reg_data = 0x04,               
		.led_status = "sys_green",
		.led_id = 2,
	},
	{
		.reg_data = 0x08,
		.led_status = "sys_blinking_green",
		.led_id = 2,
	},
	{
		.reg_data = 0x0c,
		.led_status = "sys_red",
		.led_id = 2,
	},
	{
		.reg_data = 0x00,
		.led_status = "sys_off",
		.led_id = 2,
	},	
	{
		.reg_data = 0x01, 
		.led_status = "fan_green",
		.led_id = 3,
	},
	{
		.reg_data = 0x02,
		.led_status = "fan_amber",
		.led_id = 3,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan_off",
		.led_id = 3,
	},
	{
		.reg_data = 0x03,
		.led_status = "fan_off",
		.led_id = 3,
	},
	{
		.reg_data = 0x40,
		.led_status = "fan1_green",
		.led_id = 4,
	},
	{
		.reg_data = 0x80,
		.led_status = "fan1_red",
		.led_id = 4,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan1_off",
		.led_id = 4,
	},
	{
		.reg_data = 0x10,
		.led_status = "fan2_green",
		.led_id = 5,
	},
	{
		.reg_data = 0x20,
		.led_status = "fan2_red",
		.led_id = 5,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan2_off",
		.led_id = 5,
	},	
	{
		.reg_data = 0x04,
		.led_status = "fan3_green",
	    .led_id = 6,
	},
	{
		.reg_data = 0x08,
		.led_status = "fan3_red",
		.led_id = 6,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan3_off",
		.led_id = 6,
	},
	{
		.reg_data = 0x01,
		.led_status = "fan4_green",
		.led_id = 7,
	},
	{
		.reg_data = 0x02,
		.led_status = "fan4_red",
		.led_id = 7,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan4_off",
		.led_id = 7,
	},
	{
		.reg_data = 0x40,
		.led_status = "fan5_green",
		.led_id = 8,
	},
	{
		.reg_data = 0x80,
		.led_status = "fan5_red",
		.led_id = 8,
	},
	{
		.reg_data = 0x00,
		.led_status = "fan5_off",
		.led_id = 8,
	},
};
	
struct platform_led_data{
	int reg_addr;	
	int mask;		
};
	
static struct platform_led_data led_data[] = {	
	{
		.reg_addr = SYS_LED_REG,
		.mask = 0xc0,		
	},
	{
		.reg_addr = SYS_LED_REG,
        .mask = 0x30,		
	},
	{
		.reg_addr = SYS_LED_REG,
        .mask = 0x0c,		
	},
	{
		.reg_addr = SYS_LED_REG,
        .mask = 0x03,
	},
	{
		.reg_addr = FAN1_LED_REG,
        .mask = 0xc0,		
	},
	{
		.reg_addr = FAN1_LED_REG,
        .mask = 0x30,		
	},
	{
		.reg_addr = FAN1_LED_REG,
        .mask = 0x0c,		
	},	
	{
		.reg_addr = FAN1_LED_REG,
        .mask = 0x03,		
	},
	{
    	.reg_addr = FAN2_LED_REG,
        .mask = 0xc0,		
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
        board_data = i2c_smbus_read_byte_data(pdata[system_cpld].client, led_data[led_data_number].reg_addr);	
		if(board_data >= 0){
		    board_data &= led_data[led_data_number].mask;		
		    for(led_info_number = 0; led_info_number < ARRAY_SIZE(led_info); led_info_number++){
	            if (led_data_number == led_info[led_info_number].led_id){
                    if(board_data == led_info[led_info_number].reg_data){					
	                sprintf(str[led_data_number], "%s", led_info[led_info_number].led_status);
				    }
			    }		
		    }
		}
		else
			printk( KERN_ERR "Missing LED board data\n");
	}		    		
	return sprintf(buf,"%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",str[0],str[1],str[2],str[3],str[4],str[5],str[6],str[7],str[8]);
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
					led_reg_value = i2c_smbus_read_byte_data(pdata[system_cpld].client, led_data[led_data_number].reg_addr);
                    if(led_reg_value >= 0){					
			            led_reg_value &= (~led_data[led_data_number].mask);					
				        led_reg_value |= led_info[led_info_number].reg_data;					
					    i2c_smbus_write_byte_data(pdata[system_cpld].client, (u8)(led_data[led_data_number].reg_addr & 0xff), (u8)(led_reg_value & 0xff));
					}
					else
						printk( KERN_ERR "Missing LED reg. data\n");
				}
		    }
		}
	}
    return count;
}

static unsigned char swpld_reg_addr;
static ssize_t get_swpld_reg_value(struct device *dev, struct device_attribute *devattr, char *buf) 
{
    int ret;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, swpld_reg_addr);
    return sprintf(buf, "0x%02x\n", ret);
}

static ssize_t set_swpld_reg_value(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{
    unsigned long data;
    int err;
    struct cpld_platform_data *pdata = dev->platform_data;
    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    i2c_smbus_write_byte_data(pdata[system_cpld].client, swpld_reg_addr, data);

    return count;
}

static ssize_t get_swpld_reg_addr(struct device *dev, struct device_attribute *devattr, char *buf) 
{

    return sprintf(buf, "0x%02x\n", swpld_reg_addr);
}

static ssize_t set_swpld_reg_addr(struct device *dev, struct device_attribute *attr,
             const char *buf, size_t count)
{
    unsigned long data;
    int err;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }
    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }
    swpld_reg_addr = data;

    return count;
}

static DEVICE_ATTR(swpld_reg_value, S_IRUGO | S_IWUSR, get_swpld_reg_value, set_swpld_reg_value);
static DEVICE_ATTR(swpld_reg_addr,  S_IRUGO | S_IWUSR, get_swpld_reg_addr,  set_swpld_reg_addr);
static DEVICE_ATTR(sfp_present,  S_IRUGO,           get_present,   NULL         );
static DEVICE_ATTR(sfp_lpmode,   S_IRUGO | S_IWUSR, get_lpmode,    set_lpmode   );
static DEVICE_ATTR(sfp_reset,    S_IRUGO | S_IWUSR, get_reset,     set_reset    );
static DEVICE_ATTR(sfp_response, S_IRUGO | S_IWUSR, get_response,  set_response );
static DEVICE_ATTR(led_control,  S_IRUGO | S_IWUSR, get_led_color, set_led_color);

static struct attribute *ag9032v1_cpld_attrs[] = {
    &dev_attr_swpld_reg_value.attr,
    &dev_attr_swpld_reg_addr.attr,
    &dev_attr_sfp_response.attr,
    &dev_attr_sfp_present.attr,
    &dev_attr_sfp_lpmode.attr,
    &dev_attr_sfp_reset.attr,
    &dev_attr_led_control.attr,
    NULL,
};

static struct attribute_group ag9032v1_cpld_attr_grp = {
    .attrs = ag9032v1_cpld_attrs,
};

static struct kobject *kobj_swpld;
static struct kobject *kobj_board;
static struct kobject *kobj_psu;
static struct kobject *kobj_hot_swap;
static struct kobject *kobj_controller_interrupt;
static struct kobject *kobj_BCM54616S;

static ssize_t get_swpld_data(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld);
    struct cpld_platform_data *pdata = i2cdev->platform_data;


    unsigned char reg;
    int mask;
    int value;
    char note[150];
        
    switch (attr->index) {
    //attributes on BOARD
        case SW_BOARD_ID:
            reg  = 0x00;
            ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            value = ret >> 4;
            sprintf(note, "\n“0x00”: L9032NB-AL-R\n“0x01”: AK9032-R\n“0x02”: AG9032-R\n“0x03”: AG9032R-R\n“0x04”: AG9032 V1-R\n");
            return sprintf(buf, "0x%02x%s", value, note);
        case SW_BOARD_VER:
            reg  = 0x00;
            ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            value = ret & 0x0F;
            sprintf(note, "\n“0x00”: proto-A\n“0x01”: proto-B\n");
            return sprintf(buf, "0x%02x%s", value, note);
        case SWPLD_VER:
            reg  = 0x01;
            ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            value = ret & 0xFF;
            sprintf(note, " ");
            return sprintf(buf, "0x%02x%s", value, note);
    //other attributes
        case SYS_RST ... QSFP32_MOD_INT:
            reg  = controller_interrupt_data[attr->index].reg_addr;
            mask = controller_interrupt_data[attr->index].reg_mask;
            sprintf(note, "\n%s\n",controller_interrupt_data[attr->index].reg_note);
            ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
            value = (ret & (1 << mask)) >> mask;
            return sprintf(buf, "%d%s", value, note);            
        default:
            return sprintf(buf, "%d not found", attr->index);
    }  
}

static ssize_t set_swpld_data(struct device *dev, struct device_attribute *dev_attr, const char *buf, size_t count)
{
    int ret;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_board->parent);
    struct cpld_platform_data *pdata = i2cdev->platform_data;
    unsigned char reg;
    int mask;
    int value;
    char note[180];
    int data;
    int val;
    u8 mask_out;

    ret = kstrtoint(buf, 0, &val);

    if (ret)
    {
    return ret;
    }
    if (val > 1)
    {
    return -EINVAL;
    }
    
    switch (attr->index) {
        case SYS_RST ... MB_B_PLD_RST:
        case HS1_PWR_OK ... HS2_PWR_OK:
        case B54616_RST:
        case QSFP_01TO08_MASK_INT... QSFP_25TO32_MASK_ABS:
            reg  = controller_interrupt_data[attr->index].reg_addr;
            mask = controller_interrupt_data[attr->index].reg_mask;
            break;          
        default:
            return sprintf(buf, "%d not found", attr->index);
    }  
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
    mask_out = ret & ~((u8)(1 << mask));
    data = mask_out | (val << mask);
    i2c_smbus_write_byte_data(pdata[system_cpld].client, reg, (u8)(data & 0xff));
    ret = i2c_smbus_read_byte_data(pdata[system_cpld].client, reg);
    return count;
}

static SENSOR_DEVICE_ATTR(sw_board_id,     S_IRUGO,           get_swpld_data, NULL,           SW_BOARD_ID);
static SENSOR_DEVICE_ATTR(sw_board_ver,    S_IRUGO,           get_swpld_data, NULL,           SW_BOARD_VER);
static SENSOR_DEVICE_ATTR(swpld_ver,       S_IRUGO,           get_swpld_data, NULL,           SWPLD_VER);
static SENSOR_DEVICE_ATTR(sys_rst,         S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, SYS_RST);
static SENSOR_DEVICE_ATTR(B56960_rst,      S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, B56960_RST);
static SENSOR_DEVICE_ATTR(mb_a_pld_rst,    S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, MB_A_PLD_RST);
static SENSOR_DEVICE_ATTR(mb_b_pld_rst,    S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, MB_B_PLD_RST);

static struct attribute *ag9032v1_swpld_attrs_board[] = {
    &sensor_dev_attr_sw_board_id.dev_attr.attr,
    &sensor_dev_attr_sw_board_ver.dev_attr.attr,
    &sensor_dev_attr_swpld_ver.dev_attr.attr,
    &sensor_dev_attr_sys_rst.dev_attr.attr,
    &sensor_dev_attr_B56960_rst.dev_attr.attr,
    &sensor_dev_attr_mb_a_pld_rst.dev_attr.attr,
    &sensor_dev_attr_mb_b_pld_rst.dev_attr.attr,
    NULL,
};

static SENSOR_DEVICE_ATTR(psu1_pwr_ok,      S_IRUGO, get_swpld_data, NULL, PSU1_PWR_OK);
static SENSOR_DEVICE_ATTR(psu2_pwr_ok,      S_IRUGO, get_swpld_data, NULL, PSU2_PWR_OK);

static struct attribute *ag9032v1_swpld_attrs_psu[] = {
    &sensor_dev_attr_psu1_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_psu2_pwr_ok.dev_attr.attr,
    NULL,
};

static SENSOR_DEVICE_ATTR(hs1_pwr_ok,      S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, HS1_PWR_OK);
static SENSOR_DEVICE_ATTR(hs2_pwr_ok,      S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, HS2_PWR_OK);

static struct attribute *ag9032v1_swpld_attrs_hot_swap[] = {
    &sensor_dev_attr_hs1_pwr_ok.dev_attr.attr,
    &sensor_dev_attr_hs2_pwr_ok.dev_attr.attr,
    NULL,
};

static SENSOR_DEVICE_ATTR(B54616_rst,      S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data,  B54616_RST     );
static SENSOR_DEVICE_ATTR(B54616_int,      S_IRUGO,           get_swpld_data, NULL,            B54616_INT     );
static SENSOR_DEVICE_ATTR(B54616_mask_int, S_IRUGO,           get_swpld_data, NULL,            B54616_MASK_INT);

static struct attribute *ag9032v1_swpld_attrs_BCM54616S[] = {
    &sensor_dev_attr_B54616_rst.dev_attr.attr,
    &sensor_dev_attr_B54616_int.dev_attr.attr,
    &sensor_dev_attr_B54616_mask_int.dev_attr.attr,
    NULL,
};

static SENSOR_DEVICE_ATTR(pb_hs_int,            S_IRUGO, get_swpld_data, NULL, PB_HS_INT);
static SENSOR_DEVICE_ATTR(mb_hs_int,            S_IRUGO, get_swpld_data, NULL, MB_HS_INT);
static SENSOR_DEVICE_ATTR(pb_pwr_int,           S_IRUGO, get_swpld_data, NULL, PB_PWR_INT);
static SENSOR_DEVICE_ATTR(mb_pwr_int,           S_IRUGO, get_swpld_data, NULL, MB_PWR_INT);
static SENSOR_DEVICE_ATTR(fan_int,              S_IRUGO, get_swpld_data, NULL, FAN_INT);
static SENSOR_DEVICE_ATTR(pb_hs_mask_int,       S_IRUGO, get_swpld_data, NULL, PB_HS_MASK_INT);
static SENSOR_DEVICE_ATTR(mb_hs_mask_int,       S_IRUGO, get_swpld_data, NULL, MB_HS_MASK_INT);
static SENSOR_DEVICE_ATTR(pb_pwr1_mask_int,     S_IRUGO, get_swpld_data, NULL, PB_PWR1_MASK_INT);
static SENSOR_DEVICE_ATTR(pb_pwr2_mask_int,     S_IRUGO, get_swpld_data, NULL, PB_PWR2_MASK_INT);
static SENSOR_DEVICE_ATTR(fan_mask_int,         S_IRUGO, get_swpld_data, NULL, FAN_MASK_INT);
static SENSOR_DEVICE_ATTR(qsfp_01to08_int,      S_IRUGO, get_swpld_data, NULL, QSFP_01TO08_INT);
static SENSOR_DEVICE_ATTR(qsfp_08to16_int,      S_IRUGO, get_swpld_data, NULL, QSFP_08TO16_INT);
static SENSOR_DEVICE_ATTR(qsfp_17to24_int,      S_IRUGO, get_swpld_data, NULL, QSFP_17TO24_INT);
static SENSOR_DEVICE_ATTR(qsfp_25to32_int,      S_IRUGO, get_swpld_data, NULL, QSFP_25TO32_INT);
static SENSOR_DEVICE_ATTR(qsfp_01to08_abs,      S_IRUGO, get_swpld_data, NULL, QSFP_01TO08_ABS);
static SENSOR_DEVICE_ATTR(qsfp_08to16_abs,      S_IRUGO, get_swpld_data, NULL, QSFP_08TO16_ABS);
static SENSOR_DEVICE_ATTR(qsfp_17to24_abs,      S_IRUGO, get_swpld_data, NULL, QSFP_17TO24_ABS);
static SENSOR_DEVICE_ATTR(qsfp_25to32_abs,      S_IRUGO, get_swpld_data, NULL, QSFP_25TO32_ABS);
static SENSOR_DEVICE_ATTR(qsfp_01to08_mask_int, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_01TO08_MASK_INT);
static SENSOR_DEVICE_ATTR(qsfp_08to16_mask_int, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_08TO16_MASK_INT);
static SENSOR_DEVICE_ATTR(qsfp_17to24_mask_int, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_17TO24_MASK_INT);
static SENSOR_DEVICE_ATTR(qsfp_25to32_mask_int, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_25TO32_MASK_INT);
static SENSOR_DEVICE_ATTR(qsfp_01to08_mask_abs, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_01TO08_MASK_ABS);
static SENSOR_DEVICE_ATTR(qsfp_08to16_mask_abs, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_08TO16_MASK_ABS);
static SENSOR_DEVICE_ATTR(qsfp_17to24_mask_abs, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_17TO24_MASK_ABS);
static SENSOR_DEVICE_ATTR(qsfp_25to32_mask_abs, S_IRUGO | S_IWUSR, get_swpld_data, set_swpld_data, QSFP_25TO32_MASK_ABS);
static SENSOR_DEVICE_ATTR(qsfp01_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP01_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp02_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP02_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp03_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP03_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp04_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP04_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp05_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP05_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp06_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP06_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp07_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP07_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp08_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP08_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp09_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP09_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp10_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP10_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp11_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP11_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp12_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP12_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp13_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP13_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp14_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP14_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp15_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP15_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp16_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP16_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp17_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP17_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp18_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP18_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp19_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP19_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp20_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP20_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp21_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP21_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp22_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP22_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp23_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP23_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp24_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP24_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp25_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP25_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp26_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP26_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp27_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP27_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp28_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP28_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp29_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP29_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp30_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP30_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp31_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP31_MOD_INT);
static SENSOR_DEVICE_ATTR(qsfp32_mod_int, S_IRUGO, get_swpld_data, NULL, QSFP32_MOD_INT);

static struct attribute *ag9032v1_swpld_attrs_controller_interrupt[] = {
    &sensor_dev_attr_pb_hs_int.dev_attr.attr,
    &sensor_dev_attr_mb_hs_int.dev_attr.attr,
    &sensor_dev_attr_pb_pwr_int.dev_attr.attr,
    &sensor_dev_attr_mb_pwr_int.dev_attr.attr,
    &sensor_dev_attr_fan_int.dev_attr.attr,
    &sensor_dev_attr_pb_hs_mask_int.dev_attr.attr,
    &sensor_dev_attr_mb_hs_mask_int.dev_attr.attr,
    &sensor_dev_attr_pb_pwr1_mask_int.dev_attr.attr,
    &sensor_dev_attr_pb_pwr2_mask_int.dev_attr.attr,
    &sensor_dev_attr_fan_mask_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_01to08_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_08to16_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_17to24_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_25to32_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_01to08_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_08to16_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_17to24_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_25to32_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_01to08_mask_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_08to16_mask_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_17to24_mask_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_25to32_mask_int.dev_attr.attr,
    &sensor_dev_attr_qsfp_01to08_mask_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_08to16_mask_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_17to24_mask_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp_25to32_mask_abs.dev_attr.attr,
    &sensor_dev_attr_qsfp01_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp02_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp03_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp04_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp05_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp06_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp07_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp08_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp09_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp10_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp11_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp12_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp13_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp14_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp15_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp16_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp17_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp18_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp19_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp20_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp21_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp22_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp23_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp24_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp25_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp26_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp27_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp28_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp29_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp30_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp31_mod_int.dev_attr.attr,
    &sensor_dev_attr_qsfp32_mod_int.dev_attr.attr,
    NULL,
};

static struct attribute_group ag9032v1_swpld_attr_grp_board = {
    .attrs = ag9032v1_swpld_attrs_board,
};
static struct attribute_group ag9032v1_swpld_attr_grp_psu = {
    .attrs = ag9032v1_swpld_attrs_psu,
};
static struct attribute_group ag9032v1_swpld_attr_grp_hot_swap = {
    .attrs = ag9032v1_swpld_attrs_hot_swap,
};
static struct attribute_group ag9032v1_swpld_attr_grp_BCM54616S = {
    .attrs = ag9032v1_swpld_attrs_BCM54616S,
};
static struct attribute_group ag9032v1_swpld_attr_grp_controller_interrupt = {
    .attrs = ag9032v1_swpld_attrs_controller_interrupt,
};

/*    CPLD  -- driver   */
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

    parent = i2c_get_adapter(BUS6);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", BUS6);
        return -ENODEV;
    }

    pdata[system_cpld].client = i2c_new_dummy(parent, pdata[system_cpld].reg_addr);
    if (!pdata[system_cpld].client) {
        printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[system_cpld].reg_addr);
        goto error;
    }

    kobj_swpld = &pdev->dev.kobj;
    kobj_board = kobject_create_and_add("Board", &pdev->dev.kobj);
    if (!kobj_board){
        printk(KERN_WARNING "Fail to create directory");
        goto error;
    }

    kobj_psu = kobject_create_and_add("PSU", &pdev->dev.kobj);
    if (!kobj_psu){
        printk(KERN_WARNING "Fail to create directory");
        goto error;
    }

    kobj_hot_swap = kobject_create_and_add("HOT_SWAP", &pdev->dev.kobj);
    if (!kobj_hot_swap){
        printk(KERN_WARNING "Fail to create directory");
        goto error;
    }

    kobj_controller_interrupt = kobject_create_and_add("Controller_interrupt", &pdev->dev.kobj);
    if (!kobj_controller_interrupt){
        printk(KERN_WARNING "Fail to create directory");
        goto error;
    }

    kobj_BCM54616S = kobject_create_and_add("BCM54616S", &pdev->dev.kobj);
    if (!kobj_BCM54616S){
        printk(KERN_WARNING "Fail to create directory");
        goto error;
    }

    ret = sysfs_create_group(kobj_board, &ag9032v1_swpld_attr_grp_board);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    ret = sysfs_create_group(kobj_psu, &ag9032v1_swpld_attr_grp_psu);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    ret = sysfs_create_group(kobj_hot_swap, &ag9032v1_swpld_attr_grp_hot_swap);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    ret = sysfs_create_group(kobj_BCM54616S, &ag9032v1_swpld_attr_grp_BCM54616S);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    ret = sysfs_create_group(kobj_controller_interrupt, &ag9032v1_swpld_attr_grp_controller_interrupt);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &ag9032v1_cpld_attr_grp);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld attribute group");
        goto error;
   }

    return 0;

error:
    kobject_put(kobj_swpld);
    kobject_put(kobj_board);
    kobject_put(kobj_psu);
    kobject_put(kobj_hot_swap);
    kobject_put(kobj_controller_interrupt);
    kobject_put(kobj_BCM54616S);
    i2c_unregister_device(pdata[system_cpld].client);
    i2c_put_adapter(parent);

    return -ENODEV; 
}

static int __exit cpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;
    sysfs_remove_group(&pdev->dev.kobj, &ag9032v1_cpld_attr_grp);
    sysfs_remove_group(kobj_board, &ag9032v1_swpld_attr_grp_board);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } 
    else {
        kobject_put(kobj_swpld);
        kobject_put(kobj_board);
        kobject_put(kobj_psu);
        kobject_put(kobj_hot_swap);
        kobject_put(kobj_controller_interrupt);
        kobject_put(kobj_BCM54616S);
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

static struct platform_driver cpld_driver = {
    .probe  = cpld_probe,
    .remove = __exit_p(cpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v1-swpld",
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
static struct swpld_mux_platform_data ag9032v1_swpld_mux_platform_data[] = {
    {
        .parent         = BUS3, 
        .base_nr        = BUS3_BASE_NUM, 
        .cpld           = NULL,
        .reg_addr       = BUS3_MUX_REG ,// the i2c register address which for select mux TEMP(FAN)
    },
    {
        .parent         = BUS4,
        .base_nr        = BUS4_BASE_NUM ,
        .cpld           = NULL,
        .reg_addr       = BUS4_MUX_REG ,
    },
    {
        .parent         = BUS5,
        .base_nr        = BUS5_BASE_NUM ,
        .cpld           = NULL,
        .reg_addr       = BUS5_MUX_REG ,
    },
};

static struct platform_device ag9032v1_swpld_mux[] = {
    {
        .name           = "delta-ag9032v1-swpld-mux",
        .id             = 0,
        .dev            = {
                .platform_data   = &ag9032v1_swpld_mux_platform_data[0],
                .release         = device_release,
        },
    },
    {
        .name           = "delta-ag9032v1-swpld-mux",
        .id             = 1,
        .dev            = {
                .platform_data   = &ag9032v1_swpld_mux_platform_data[1],
                .release         = device_release,
        },
    },
    {
        .name           = "delta-ag9032v1-swpld-mux",
        .id             = 2,
        .dev            = {
                .platform_data   = &ag9032v1_swpld_mux_platform_data[2],
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

    if ( mux->data.base_nr == BUS3_BASE_NUM )
    {
        switch (chan) {  
            case 0:
                swpld_mux_val = TEMP_FAN_VAL;
                break;
            case 1:
                swpld_mux_val = FAN_EEPROM1_VAL;
                break;
            case 2:
                swpld_mux_val = FAN_EEPROM2_VAL;
                break;
            case 3:
                swpld_mux_val = FAN_EEPROM3_VAL;
                break;
            case 4:
                swpld_mux_val = FAN_EEPROM4_VAL;
                break;
            case 5:
                swpld_mux_val = FAN_EEPROM5_VAL;
                break;
            case 6:
                swpld_mux_val = FANIO_CTL_VAL;
                break;
            case 7:
            case 8:
                swpld_mux_val = FAN_CTRL_VAL; 
                break;
        }
    }
    else if ( mux->data.base_nr == BUS4_BASE_NUM )
    {
        switch (chan) {
            case 0:
                swpld_mux_val = PSU1_VAL;
                break;
            case 1:
                swpld_mux_val = PSU2_VAL;
                break;
            case 2:
                swpld_mux_val = HOT_SWAP1_VAL; 
                break;
            case 3:
                swpld_mux_val = HOT_SWAP2_VAL;  
                break;
        }
    }
    else if ( mux->data.base_nr == BUS5_BASE_NUM ){
        if (chan < 9){
            swpld_mux_val = (u8)(chan) + 0x01;
        }
        else if (8 < chan && chan < 19){
            swpld_mux_val = (u8)(chan - 9) + 0x10;
        }
        else if (18 < chan && chan < 29){
            swpld_mux_val = (u8)(chan - 19) + 0x20;
        }
        else if (28 < chan && chan < 39){
            swpld_mux_val = (u8)(chan - 29) + 0x30;
        }
        else{
            swpld_mux_val = 0x00;
        }
    }
    else
    {
        swpld_mux_val = 0x00;
    }
    return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(swpld_mux_val & 0xff));
}
#else // #if LINUX_VERSION_CODE >= KERNEL_VERSION(4,7,0)
static int swpld_mux_select(struct i2c_mux_core *muxc, u32 chan)
{
    struct swpld_mux  *mux = i2c_mux_priv(muxc);
    u8 swpld_mux_val=0; 

    if ( mux->data.base_nr == BUS3_BASE_NUM )
    {
        switch (chan) {  
            case 0:
                swpld_mux_val = TEMP_FAN_VAL;
                break;
            case 1:
                swpld_mux_val = FAN_EEPROM1_VAL;
                break;
            case 2:
                swpld_mux_val = FAN_EEPROM2_VAL;
                break;
            case 3:
                swpld_mux_val = FAN_EEPROM3_VAL;
                break;
            case 4:
                swpld_mux_val = FAN_EEPROM4_VAL;
                break;
            case 5:
                swpld_mux_val = FAN_EEPROM5_VAL;
                break;
            case 6:
                swpld_mux_val = FANIO_CTL_VAL;
                break;
            case 7:
            case 8:
                swpld_mux_val = FAN_CTRL_VAL; 
                break;
        }
    }
    else if ( mux->data.base_nr == BUS4_BASE_NUM )
    {
        switch (chan) {
            case 0:
                swpld_mux_val = PSU1_VAL;
                break;
            case 1:
                swpld_mux_val = PSU2_VAL;
                break;
            case 2:
                swpld_mux_val = HOT_SWAP1_VAL; 
                break;
            case 3:
                swpld_mux_val = HOT_SWAP2_VAL;  
                break;
        }
    }
    else if ( mux->data.base_nr == BUS5_BASE_NUM ){
        if (chan < 9){
            swpld_mux_val = (u8)(chan) + 0x01;
        }
        else if (8 < chan && chan < 19){
            swpld_mux_val = (u8)(chan - 9) + 0x10;
        }
        else if (18 < chan && chan < 29){
            swpld_mux_val = (u8)(chan - 19) + 0x20;
        }
        else if (28 < chan && chan < 39){
            swpld_mux_val = (u8)(chan - 29) + 0x30;
        }
        else{
            swpld_mux_val = 0x00;
        }
    }
    else
    {
        swpld_mux_val = 0x00;
    }
    
    return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, (u8)(swpld_mux_val & 0xff));
}
#endif // #if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)

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
        case BUS3:
            dev_num = BUS3_DEV_NUM;
            break;
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        case BUS5:
            dev_num = BUS5_DEV_NUM; 
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
        case BUS3:
            dev_num = BUS3_DEV_NUM;
            break;
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        case BUS5:
            dev_num = BUS5_DEV_NUM; 
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
#endif // #if LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0)

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
        case BUS3:
            dev_num = BUS3_DEV_NUM;
            break;
        case BUS4:
            dev_num = BUS4_DEV_NUM;
            break;
        case BUS5:
            dev_num = BUS5_DEV_NUM; 
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
    struct i2c_adapter *parent=muxc->parent;

    i2c_mux_del_adapters(muxc);
    i2c_put_adapter(parent);

    return 0;
}
#endif

static struct platform_driver swpld_mux_driver = {
    .probe  = swpld_mux_probe,
    .remove = __exit_p(swpld_mux_remove), /* TODO */
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-ag9032v1-swpld-mux",
    },
};
/*----------------    MUX   - end   ------------- */

/*----------------   module initialization     ------------- */
static int __init delta_ag9032v1_platform_init(void)
{
//    struct i2c_client *client;
    struct i2c_adapter *adapter;
    struct cpld_platform_data *cpld_pdata;
    struct swpld_mux_platform_data *swpld_mux_pdata;
    int ret,i = 0;
    printk("ag9032v1_platform module initialization\n");
    
    //Use pca9547 in i2c_mux_pca954x.c
    adapter = i2c_get_adapter(BUS1); 
    //client = i2c_new_device(adapter, &i2c_info_pca9547[0]);
    i2c_client_9547 = i2c_new_device(adapter, &i2c_info_pca9547[0]);
	
    i2c_put_adapter(adapter);

    // set the CPLD prob and  remove
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
    ret = platform_device_register(&ag9032v1_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_ag9032v1_cpld;
    }
    // link the CPLD and the Mux
    cpld_pdata = ag9032v1_cpld.dev.platform_data;

    for (i = 0; i < ARRAY_SIZE(ag9032v1_swpld_mux); i++)
    {
        swpld_mux_pdata = ag9032v1_swpld_mux[i].dev.platform_data;
        swpld_mux_pdata->cpld = cpld_pdata[system_cpld].client;  
        ret = platform_device_register(&ag9032v1_swpld_mux[i]);          
        if (ret) {
            printk(KERN_WARNING "Fail to create swpld mux %d\n", i);
            goto error_ag9032v1_swpld_mux;
        }
    }

    for (i = 0; i < ARRAY_SIZE(ag9032v1_i2c_device); i++)
    {
        ret = platform_device_register(&ag9032v1_i2c_device[i]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", i);
            goto error_ag9032v1_i2c_device;
        }
    }

    if (ret)
        goto error_ag9032v1_swpld_mux;

    return 0;

error_ag9032v1_i2c_device:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&ag9032v1_i2c_device[i]);
    }
    i = ARRAY_SIZE(ag9032v1_swpld_mux);    
error_ag9032v1_swpld_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&ag9032v1_swpld_mux[i]);
    }
    platform_driver_unregister((struct platform_driver *) &ag9032v1_cpld);
error_ag9032v1_cpld:
    platform_driver_unregister(&i2c_device_driver);
error_i2c_device_driver:
    platform_driver_unregister(&swpld_mux_driver);
error_swpld_mux_driver:
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
}

static void __exit delta_ag9032v1_platform_exit(void)
{
    int i = 0;

    for ( i = 0; i < ARRAY_SIZE(ag9032v1_i2c_device); i++ ) {
        platform_device_unregister(&ag9032v1_i2c_device[i]);
    }

    for (i = 0; i < ARRAY_SIZE(ag9032v1_swpld_mux); i++) {
        platform_device_unregister(&ag9032v1_swpld_mux[i]);
    }

    platform_device_unregister(&ag9032v1_cpld);
    platform_driver_unregister(&i2c_device_driver);
    platform_driver_unregister(&cpld_driver);
    platform_driver_unregister(&swpld_mux_driver);

    i2c_unregister_device(i2c_client_9547);
}

module_init(delta_ag9032v1_platform_init);
module_exit(delta_ag9032v1_platform_exit);

MODULE_DESCRIPTION("DNI ag9032v1 Platform Support");
MODULE_AUTHOR("Neal Tai <neal.tai@deltaww.com>");
MODULE_LICENSE("GPL"); 