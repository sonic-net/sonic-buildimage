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
#include <linux/platform_data/pca953x.h>

#include <linux/i2c-mux.h>
#include <linux/platform_data/i2c-mux-gpio.h>
#include <linux/i2c/sff-8436.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#define OPTOE_INFO(data) \
    .type = "optoe2", .addr = 0x50

#define et6248brb_i2c_device_num(NUM){                                         \
        .name                   = "delta-et6248brb-i2c-device",                \
        .id                     = NUM,                                         \
        .dev                    = {                                            \
                    .platform_data = &et6248brb_i2c_device_platform_data[NUM], \
                    .release       = device_release,                           \
        },                                                                     \
}

#define et6248brb_gpio_num(NUM){                                                \
        .name                   = "delta-et6248brb-gpio",                       \
        .id                     = NUM,                                          \
        .dev                    = {                                             \
                    .platform_data = &et6248brb_gpio_device_platform_data[NUM], \
                    .release       = device_release,                            \
        },                                                                      \
}

#define PCA9555_A 0x20
#define PCA9555_B 0x21
#define PCA9555_C 0x23

static struct kobject *kobj_gpio;
static struct kobject *kobj_psu;
static struct kobject *kobj_fan;
static struct kobject *kobj_sfp;
static struct kobject *kobj_others;

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
    BUS8,
    BUS9,
    BUS10,
};

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

static struct i2c_device_platform_data et6248brb_i2c_device_platform_data[] = {
    {
        /* FAN controller (0x2e) */
        .parent = 0,
        .info = { I2C_BOARD_INFO("adt7473", 0x2e) },
        .client = NULL,
    },
    {
        /* tmp75 (0x48) */
        .parent = 0,
        .info = { I2C_BOARD_INFO("tmp75", 0x48) },
        .client = NULL,
    },
    { 
        /* EEPROM (0x54) */
        .parent = 1, 
        .info = { I2C_BOARD_INFO("24c08", 0x54) },
        .client = NULL,
    },
    {
        /* sfp 1 (0x50) */
        .parent = 4,
        .info = { OPTOE_INFO() },
        .client = NULL,
    },
    {
        /* sfp 2 (0x50) */
        .parent = 5,
        .info = { OPTOE_INFO() },
        .client = NULL,
    },
    {
        /* tmp75 (0x49) */
        .parent = 7,
        .info = { I2C_BOARD_INFO("tmp75", 0x49) },
        .client = NULL,
    },
    {
        /* tmp75 (0x4a) */
        .parent = 8,
        .info = { I2C_BOARD_INFO("tmp75", 0x4a) },
        .client = NULL,
    },
};

static struct platform_device et6248brb_i2c_device[] = {
    et6248brb_i2c_device_num(0),
    et6248brb_i2c_device_num(1),
    et6248brb_i2c_device_num(2),
    et6248brb_i2c_device_num(3),
    et6248brb_i2c_device_num(4),
    et6248brb_i2c_device_num(5),
    et6248brb_i2c_device_num(6),
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
        .name = "delta-et6248brb-i2c-device",
    }
};
/*----------------   I2C driver   - end   ------------- */

/*----------------   gpio device - start   ----------- */
static struct pca953x_platform_data  pca9555_data_0 = {
    .gpio_base = 200,
};
static struct pca953x_platform_data  pca9555_data_1 = {
    .gpio_base = 216,
};
static struct pca953x_platform_data  pca9555_data_2 = {
    .gpio_base = 232,
};

static struct i2c_board_info __initdata i2c_info_pca9555[] =
{
        {
            I2C_BOARD_INFO("pca9555", 0x20),
            .platform_data = &pca9555_data_0, 
        },
        {
            I2C_BOARD_INFO("pca9555", 0x21),
            .platform_data = &pca9555_data_1, 
        },
        {
            I2C_BOARD_INFO("pca9555", 0x23),
            .platform_data = &pca9555_data_2, 
        },        
};

static struct i2c_device_platform_data et6248brb_gpio_device_platform_data[] = {
    {
        /* GPIO expander A (0x20) */
        .parent = 0,
        .info = { I2C_BOARD_INFO("pca9555", 0x20) },
        .client = NULL,
    },
    {
        /* GPIO expander B (0x21) */
        .parent = 0,
        .info = { I2C_BOARD_INFO("pca9555", 0x21) },
        .client = NULL,
    },
    {
        /* GPIO expander C (0x23) */
        .parent = 0,
        .info = { I2C_BOARD_INFO("pca9555", 0x23) },
        .client = NULL,
    },
};

static struct platform_device et6248brb_gpio_device[] = {
    et6248brb_gpio_num(0),
    et6248brb_gpio_num(1),
    et6248brb_gpio_num(2),
};

static struct gpio_attribute_data {
    uint8_t bus;
    uint8_t addr;
    uint8_t reg;
    uint8_t mask;
    char    note[150];
};

enum gpio_attributes {
    PSU1_SMB_ALERT,
    PSU2_SMB_ALERT,
    EEPROM_WP,
    FAN1_LED_AG,
    FAN2_LED_AG,
    D_FAN_ALERT,
    PSU1_PRES,
    PSU2_PRES,
    FAN_EEPROM_WP,
    D_FAN_M_PRESENT2,
    D_FAN_M_PRESENT1,
    PSU1_PG,
    PSU2_PG,
    BCM54282_INT,
    SFP_MOD_P1,
    SFP_RXLOS_P1,
    SFP_MOD_P2,
    SFP_RXLOS_P2,
    SFP_TX_FAULT_P1,
    SFP_TX_FAULT_P2,
    SFP_TX_DIS_P1,
    SFP_TX_DIS_P2,
    OOB_BCM54616S_INT,
};

static struct gpio_attribute_data attribute_data[] = {
//PCA9555_A  
    [PSU1_SMB_ALERT] = {
        .bus  = BUS0,     .addr = PCA9555_A,
        .reg  = 0x00,     .mask = 1 << 0,
        .note = "0=PSU1 interrupt is occurr\n1=PSU1 interrupt is NOT occurr"
    },
    [PSU2_SMB_ALERT] = {
        .bus  = BUS0,     .addr = PCA9555_A,
        .reg  = 0x00,     .mask = 1 << 3,
        .note = "0=PSU2 interrupt is occurr\n1=PSU2 interrupt is NOT occurr"
    },
     [EEPROM_WP] = {
        .bus  = BUS0,     .addr = PCA9555_A,
        .reg  = 0x03,     .mask = 1 << 3,
        .note = "0=SYS eeprom write protect is disable\n1=SYS eeprom write protect is enable and can not be programmed"

    },
    [FAN1_LED_AG] = {
        .bus  = BUS0,     .addr = PCA9555_A,
        .reg  = 0x03,     .mask =  0x30,
        .note = "00=LED off or FAN tray1 is not present\n01=LED Red,FAN tray1 fail\n10=LED Green,FAN tray1 is present\n11=reserved"
    },
    [FAN2_LED_AG] = {
        .bus  = BUS0,     .addr = PCA9555_A,
        .reg  = 0x03,     .mask =  0xc0,
        .note = "00=LED off or FAN tray1 is not present\n01=LED Red,FAN tray1 fail\n10=LED Green,FAN tray1 is present\n11=reserved"
    },   
//PCA9555_B
    [D_FAN_ALERT] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x00,     .mask = 1 << 0,
        .note ="0=FAN is NOT issue the alram\n1=FAN issue the alarm"

    },
    [PSU1_PRES] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x00,     .mask = 1 << 2,
        .note ="0=PSU1 is present\n1=PSU1 is NOT present"
    },
    [PSU2_PRES] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x00,     .mask = 1 << 3,
        .note ="0=PSU2 is present\n1=PSU2 is NOT present"
    },
    [FAN_EEPROM_WP] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 1,
        .note ="0=FAN eeprom write protect is disable\n1=FAN eeprom write protect is enable and can not be programmed"
    },  
    [D_FAN_M_PRESENT2] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 2,
        .note ="0=FAN2 module is present\n1=FAN2 module is NOT present"
    }, 
    [D_FAN_M_PRESENT1] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 3,
        .note ="0=FAN1 moduel is present\n1=FAN1 module is NOT present"
    }, 
    [PSU1_PG] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 4,
        .note ="0=PSU1 is FAIL or not present\n1=PSU1 is GOOD"
    }, 
    [PSU2_PG] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 5,
        .note ="0=PSU2 is FAIL or not present\n1=PSU2 is GOOD"
    }, 
    [BCM54282_INT] = {
        .bus  = BUS0,     .addr = PCA9555_B,
        .reg  = 0x01,     .mask = 1 << 6,
        .note ="0=BCM54282 PHY interrupt occurrs\n1=BCM54282 PHY interrupt is not occurrs"
    }, 
//PCA9555_C
    [SFP_MOD_P1] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 0,
        .note ="0=SFP1 transceiver is present\n1=SFP1 transceiver is NOT present"
    },  
    [SFP_RXLOS_P1] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 1,
        .note ="0=SFP1 transceiver is NOT asserted Loss of signal\n1=SFP1 transceiver is asserted Loss of signal"
    }, 
    [SFP_MOD_P2] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 2,
        .note ="0=SFP1 transceiver is present\n1=SFP1 transceiver is NOT present"
    },
    [SFP_RXLOS_P2] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 3,
        .note ="0=SFP1 transceiver is NOT asserted Loss of signal\n1=SFP1 transceiver is asserted Loss of signal"
    },
    [SFP_TX_FAULT_P1] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 4,
        .note ="0=SFP1 transceiver is NOT asserted TXFAULT signal\n1=SFP1 transceiver is asserted TXFAULT signal"
    },
    [SFP_TX_FAULT_P2] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x00,     .mask = 1 << 5,
        .note ="0=SFP1 transceiver is NOT asserted TXFAULT signal\n1=SFP1 transceiver is asserted TXFAULT signal"
    },
    [SFP_TX_DIS_P1] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x02,     .mask = 1 << 6,
        .note ="0=SFP1 transceiver is turn ON\n1=SFP1 transceiver is turn OFF"
    },
    [SFP_TX_DIS_P2] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x02,     .mask = 1 << 7,
        .note ="0=SFP2 transceiver is turn ON\n1=SFP2 transceiver is turn OFF"
    },
    [OOB_BCM54616S_INT] = {
        .bus  = BUS0,     .addr = PCA9555_C,
        .reg  = 0x01,     .mask = 1 << 3,
        .note ="0=BCM54616S PHY interrupt occurrs\n1=BCM54616S PHY interrupt is not occurrs"
    },
};

unsigned char dni_log2 (unsigned char num){
    unsigned char num_log2 = 0;
    while(num > 0){
        num = num >> 1;
        num_log2 += 1;
    }
    return num_log2 -1;
}

void set_direction(struct i2c_device_platform_data *pdata, unsigned char reg, unsigned char mask){

    int value;
    int read_only;
    unsigned char mask_out;   
    unsigned char set_data;

    read_only = 1;  //Configuration:GPI
    if (reg > 0x1){
        reg = reg - 0x02;
        read_only = 0;  //Configuration:GPO
    }

    value = i2c_smbus_read_byte_data(pdata->client, reg + 0x06);
    mask_out = value & ~(mask);

    if(read_only){
        set_data = mask_out | mask;
    }
    else{
        set_data = mask_out;
    }

    i2c_smbus_write_byte_data(pdata->client, reg + 0x06, set_data);
    return;
}

static ssize_t get_gpio_reg(struct device *dev, struct device_attribute *dev_attr, char *buf) 
{
    int ret;
    int mask;
    int value;
    char note[150];
    unsigned char pca9555_num;
    unsigned char reg;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_gpio);
    struct i2c_device_platform_data *pdata = i2cdev->platform_data;

    switch(attribute_data[attr->index].addr){
        case PCA9555_A:
            pca9555_num = 0;
            break; 
        case PCA9555_B:
            pca9555_num = 1;
            break; 
        case PCA9555_C:
            pca9555_num = 2;
            break; 
        default:
            return sprintf(buf, "attribute address error");
    }

    set_direction(&pdata[pca9555_num], attribute_data[attr->index].reg, attribute_data[attr->index].mask);

    switch (attr->index) {
        case PSU1_SMB_ALERT ... OOB_BCM54616S_INT:
            reg = attribute_data[attr->index].reg;
            mask  = attribute_data[attr->index].mask;
            value = i2c_smbus_read_byte_data(pdata[pca9555_num].client, reg);
            sprintf(note, "\n%s\n",attribute_data[attr->index].note);
            value = (value & mask);
            break;
        default:
            return sprintf(buf, "%d not found", attr->index);
    } 

    switch (mask) {
        case 0xFF:
            return sprintf(buf, "0x%02x%s", value, note);
        case 0x0F:
            return sprintf(buf, "0x%01x%s", value, note);
        case 0xF0:
            value = value >> 4;
            return sprintf(buf, "0x%01x%s", value, note);
        case 0xC0:
            value = value >> 6;
            return sprintf(buf, "0x%01x%s", value, note);
        case 0x30:
            value = value >> 4;
            return sprintf(buf, "0x%01x%s", value, note);
        default :
            value = value >> dni_log2(mask);
            return sprintf(buf, "%d%s", value, note);
    }
}


static ssize_t set_gpio_reg(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    int err;
    int value;
    unsigned long set_data;
    unsigned char set_reg;
    unsigned char mask;  
    unsigned char mask_out;
    unsigned char pca9555_num;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_gpio);
    struct i2c_device_platform_data *pdata = i2cdev->platform_data;
    
    err = kstrtoul(buf, 0, &set_data);
    if (err){
        return err;
    }

    if (set_data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }    

    switch(attribute_data[attr->index].addr){
        case PCA9555_A:
            pca9555_num = 0;
            break; 
        case PCA9555_B:
            pca9555_num = 1;
            break; 
        case PCA9555_C:
            pca9555_num = 2;
            break;
        default:
            return sprintf(buf, "attribute address error");
    }

    set_direction(&pdata[pca9555_num], attribute_data[attr->index].reg, attribute_data[attr->index].mask);

    switch (attr->index) {
        case PSU1_SMB_ALERT ... OOB_BCM54616S_INT:
            set_reg   = attribute_data[attr->index].reg;
            mask  = attribute_data[attr->index].mask;
            value = i2c_smbus_read_byte_data(pdata[pca9555_num].client, set_reg);
            mask_out = value & ~(mask);
            break;
        default:
            return sprintf(buf, "%d not found", attr->index);
    }

    switch (mask) {
        case 0xFF:
            set_data = mask_out | (set_data & mask);
            break;
        case 0x0F:
            set_data = mask_out | (set_data & mask);
            break;
        case 0xF0:
            set_data = set_data << 4;
            set_data = mask_out | (set_data & mask);
            break;
        case 0xC0:
            set_data = set_data << 6;
            set_data = mask_out | (set_data & mask);
            break;
        case 0x30:
            set_data = set_data << 4;
            set_data = mask_out | (set_data & mask);
            break;
        default :
            set_data = mask_out | (set_data << dni_log2(mask) );
    }

    i2c_smbus_write_byte_data(pdata[pca9555_num].client, set_reg, set_data);
    return count;
}
static SENSOR_DEVICE_ATTR(psu1_smb_alert,    S_IRUGO,           get_gpio_reg, NULL,         PSU1_SMB_ALERT);
static SENSOR_DEVICE_ATTR(psu2_smb_alert,    S_IRUGO,           get_gpio_reg, NULL,         PSU2_SMB_ALERT);
static SENSOR_DEVICE_ATTR(eeprom_wp,         S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, EEPROM_WP);
static SENSOR_DEVICE_ATTR(fan1_led_ag,       S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, FAN1_LED_AG);
static SENSOR_DEVICE_ATTR(fan2_led_ag,       S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, FAN2_LED_AG);
static SENSOR_DEVICE_ATTR(d_fan_alert,       S_IRUGO,           get_gpio_reg, NULL,         D_FAN_ALERT);
static SENSOR_DEVICE_ATTR(psu1_pres,         S_IRUGO,           get_gpio_reg, NULL,         PSU1_PRES);
static SENSOR_DEVICE_ATTR(psu2_pres,         S_IRUGO,           get_gpio_reg, NULL,         PSU2_PRES);
static SENSOR_DEVICE_ATTR(fan_eeprom_wp,     S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, FAN_EEPROM_WP);
static SENSOR_DEVICE_ATTR(d_fan_m_present2,  S_IRUGO,           get_gpio_reg, NULL,         D_FAN_M_PRESENT2);
static SENSOR_DEVICE_ATTR(d_fan_m_present1,  S_IRUGO,           get_gpio_reg, NULL,         D_FAN_M_PRESENT1);
static SENSOR_DEVICE_ATTR(psu1_pg,           S_IRUGO,           get_gpio_reg, NULL,         PSU1_PG);
static SENSOR_DEVICE_ATTR(psu2_pg,           S_IRUGO,           get_gpio_reg, NULL,         PSU2_PG);
static SENSOR_DEVICE_ATTR(bcm54282_int,      S_IRUGO,           get_gpio_reg, NULL,         BCM54282_INT);

static SENSOR_DEVICE_ATTR(sfp_mod_p1,        S_IRUGO,           get_gpio_reg, NULL,         SFP_MOD_P1);
static SENSOR_DEVICE_ATTR(sfp_rxlos_p1,      S_IRUGO,           get_gpio_reg, NULL,         SFP_RXLOS_P1);
static SENSOR_DEVICE_ATTR(sfp_mod_p2,        S_IRUGO,           get_gpio_reg, NULL,         SFP_MOD_P2);
static SENSOR_DEVICE_ATTR(sfp_rxlos_p2,      S_IRUGO,           get_gpio_reg, NULL,         SFP_RXLOS_P2);
static SENSOR_DEVICE_ATTR(sfp_tx_fault_p1,   S_IRUGO,           get_gpio_reg, NULL,         SFP_TX_FAULT_P1);
static SENSOR_DEVICE_ATTR(sfp_tx_fault_p2,   S_IRUGO ,          get_gpio_reg, NULL,         SFP_TX_FAULT_P2);
static SENSOR_DEVICE_ATTR(sfp_tx_dis_p1,     S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, SFP_TX_DIS_P1);
static SENSOR_DEVICE_ATTR(sfp_tx_dis_p2,     S_IRUGO | S_IWUSR, get_gpio_reg, set_gpio_reg, SFP_TX_DIS_P2);
static SENSOR_DEVICE_ATTR(oob_bcm54616s_int, S_IRUGO,           get_gpio_reg, NULL,         OOB_BCM54616S_INT);

static struct attribute *et6248brb_psu_attrs[] = {
    &sensor_dev_attr_psu1_smb_alert.dev_attr.attr,
    &sensor_dev_attr_psu2_smb_alert.dev_attr.attr,
    &sensor_dev_attr_psu1_pres.dev_attr.attr,
    &sensor_dev_attr_psu2_pres.dev_attr.attr,
    &sensor_dev_attr_psu1_pg.dev_attr.attr,
    &sensor_dev_attr_psu2_pg.dev_attr.attr,
    NULL,
};

static struct attribute *et6248brb_fan_attrs[] = {
    &sensor_dev_attr_fan1_led_ag.dev_attr.attr,
    &sensor_dev_attr_fan2_led_ag.dev_attr.attr,
    &sensor_dev_attr_d_fan_alert.dev_attr.attr,
    &sensor_dev_attr_fan_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_d_fan_m_present1.dev_attr.attr,
    &sensor_dev_attr_d_fan_m_present2.dev_attr.attr,
    NULL,
};

static struct attribute *et6248brb_sfp_attrs[] = {
    &sensor_dev_attr_sfp_mod_p1.dev_attr.attr,
    &sensor_dev_attr_sfp_rxlos_p1.dev_attr.attr,
    &sensor_dev_attr_sfp_mod_p2.dev_attr.attr,
    &sensor_dev_attr_sfp_rxlos_p2.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_fault_p1.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_fault_p2.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_dis_p1.dev_attr.attr,
    &sensor_dev_attr_sfp_tx_dis_p2.dev_attr.attr,
    NULL,
};

static struct attribute *et6248brb_others_attrs[] = {
    &sensor_dev_attr_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_bcm54282_int.dev_attr.attr,
    &sensor_dev_attr_oob_bcm54616s_int.dev_attr.attr,
    NULL,
};

static struct attribute_group et6248brb_psu_attr_grp = {
    .attrs = et6248brb_psu_attrs,
};

static struct attribute_group et6248brb_fan_attr_grp = {
    .attrs = et6248brb_fan_attrs,

};
static struct attribute_group et6248brb_sfp_attr_grp = {
    .attrs = et6248brb_sfp_attrs,
};

static struct attribute_group et6248brb_others_attr_grp = {
    .attrs = et6248brb_others_attrs,
};
/*----------------   gpio device - end   ------------- */

/*----------------   gpio driver - start   ----------- */
static int __init gpio_device_probe(struct platform_device *pdev)
{
    struct i2c_device_platform_data *pdata;
    struct i2c_adapter *parent;
    int ret;

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

    if(pdata->info.addr == PCA9555_A){
        kobj_gpio = &pdev->dev.kobj;
        kobj_psu = kobject_create_and_add("PSU", &pdev->dev.kobj);
        if (!kobj_psu){
            printk(KERN_WARNING "Fail to create 'PSU' directory");
            goto error;
        }

        kobj_fan = kobject_create_and_add("FAN", &pdev->dev.kobj);
        if (!kobj_fan){
            printk(KERN_WARNING "Fail to create 'FAN' directory");
            goto error;
        }

        kobj_sfp = kobject_create_and_add("SFP", &pdev->dev.kobj);
        if (!kobj_sfp){
            printk(KERN_WARNING "Fail to create 'SFP' directory");
            goto error;
        }
        kobj_others = kobject_create_and_add("Others", &pdev->dev.kobj);
        if (!kobj_others){
            printk(KERN_WARNING "Fail to create 'Others' directory");
            goto error;
        }
        ret = sysfs_create_group(kobj_psu, &et6248brb_psu_attr_grp);
        if (ret) {
            printk(KERN_WARNING "Fail to create 'psu' attribute group");
            goto error;
        }
        ret = sysfs_create_group(kobj_fan, &et6248brb_fan_attr_grp);
        if (ret) {
            printk(KERN_WARNING "Fail to create 'fan' attribute group");
            goto error;
        }
        ret = sysfs_create_group(kobj_sfp, &et6248brb_sfp_attr_grp);
        if (ret) {
            printk(KERN_WARNING "Fail to create 'sfp' attribute group");
            goto error;
        }
        ret = sysfs_create_group(kobj_others, &et6248brb_others_attr_grp);
        if (ret) {
            printk(KERN_WARNING "Fail to create 'others' attribute group");
            goto error;
        }
    }
    return 0;

error:
    kobject_put(kobj_gpio);
    kobject_put(kobj_psu);
    kobject_put(kobj_fan);
    kobject_put(kobj_sfp);
    kobject_put(kobj_others);
    i2c_unregister_device(pdata->client);
    i2c_put_adapter(parent);

    return -ENODEV;
}

static int __exit gpio_deivce_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent;
    struct i2c_device_platform_data *pdata;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    }
    else if(pdata->info.addr == PCA9555_A){
        sysfs_remove_group(kobj_gpio, &et6248brb_psu_attr_grp);
        sysfs_remove_group(kobj_fan, &et6248brb_fan_attr_grp);
        sysfs_remove_group(kobj_sfp, &et6248brb_sfp_attr_grp);
        sysfs_remove_group(kobj_others, &et6248brb_others_attr_grp);
        kobject_put(kobj_gpio);
        kobject_put(kobj_fan);
        kobject_put(kobj_psu);
        kobject_put(kobj_sfp);
        kobject_put(kobj_others);
    }

    if (pdata->client) {
        if (!parent) {
            parent = (pdata->client)->adapter;
        }
        i2c_unregister_device(pdata->client);
    }

    i2c_put_adapter(parent);
    return 0;
}

static struct platform_driver gpio_device_driver = {
    .probe = gpio_device_probe,
    .remove = __exit_p(gpio_deivce_remove),
    .driver = {
        .owner = THIS_MODULE,
        .name = "delta-et6248brb-gpio",
    }
};
/*----------------   gpio driver - end   ------------- */

/*----------------   module initialization     ------------- */
static int __init delta_et6248brb_platform_init(void)
{
    struct i2c_adapter *adapter;

    int ret, i = 0, j = 0;
    printk("et6248brb_platform module initialization\n");
    
    //Use pca9547 in i2c_mux_pca954x.c
    adapter = i2c_get_adapter(BUS1); 
    i2c_client_9547 = i2c_new_device(adapter, &i2c_info_pca9547[0]);
    i2c_put_adapter(adapter);

    // register the i2c devices
    ret = platform_driver_register(&i2c_device_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_i2c_device_driver;
    }

    // register the i2c devices
    ret = platform_driver_register(&gpio_device_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register i2c device driver\n");
        goto error_gpio_device_driver;
    }

    for (i = 0; i < ARRAY_SIZE(et6248brb_i2c_device); i++)
    {
        ret = platform_device_register(&et6248brb_i2c_device[i]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", i);
            goto error_et6248brb_i2c_device;
        }
    }

    for (j = 0; j < ARRAY_SIZE(et6248brb_gpio_device); j++)
    {
        ret = platform_device_register(&et6248brb_gpio_device[j]);
        if (ret) {
            printk(KERN_WARNING "Fail to create i2c device %d\n", j);
            goto error_et6248brb_gpio_device;
        }
    }

    return 0;
error_et6248brb_gpio_device:
    j--;
    for (; j >= 0; j--) {
        platform_device_unregister(&et6248brb_gpio_device[j]);
    }
error_et6248brb_i2c_device:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&et6248brb_i2c_device[i]);
    }
    platform_driver_unregister(&gpio_device_driver);
error_gpio_device_driver:
    platform_driver_unregister(&i2c_device_driver);
error_i2c_device_driver:
    return ret;
}

static void __exit delta_et6248brb_platform_exit(void)
{
    int i = 0;

    for ( i = 0; i < ARRAY_SIZE(et6248brb_i2c_device); i++ ) {
        platform_device_unregister(&et6248brb_i2c_device[i]);
    }

    for ( i = 0; i < ARRAY_SIZE(et6248brb_gpio_device); i++ ) {
        platform_device_unregister(&et6248brb_gpio_device[i]);
    }
    platform_driver_unregister(&i2c_device_driver);
    platform_driver_unregister(&gpio_device_driver);
    i2c_unregister_device(i2c_client_9547);
}

module_init(delta_et6248brb_platform_init);
module_exit(delta_et6248brb_platform_exit);

MODULE_DESCRIPTION("DNI et6248brb Platform Support");
MODULE_AUTHOR("Jacky Liu <jacky.js.liu@deltaww.com>");
MODULE_LICENSE("GPL");
