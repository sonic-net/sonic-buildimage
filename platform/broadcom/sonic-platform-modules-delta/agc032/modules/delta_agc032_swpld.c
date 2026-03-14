#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/hwmon-sysfs.h>
#include <linux/hwmon.h>



#define SWPLD1_ADDR 0x32
#define SWPLD2_ADDR 0x34
#define SWPLD3_ADDR 0x35



enum cpld_type {
    swpld1 = 0,
    swpld2,
    swpld3,
};

enum {
    BUS0 = 0,
    BUS1,
    BUS2,
    BUS3,
    BUS4,
    BUS5,
    BUS6,
    BUS7,
    BUS8,
};

enum swpld1_attr{
    BOARD_ID = 0,
    BOARD_VER,
    SWPLD1_VER_TYPE,
    SWPLD1_VER,
    USB_HUB_RST,
    SYNCE_RST,    
    BMC_RST,
    LPC_RST,
    OOB_PCIE_RST,
    MAC_PCIE_RST,
    MAC_RST,
    VR_3V3_RST,
    VR_0V8_RST,
    FAN_MUX_RST,
    QSFP5_MUX_RST,
    QSFP4_MUX_RST,
    QSFP3_MUX_RST,
    QSFP2_MUX_RST,
    QSFP1_MUX_RST,
    PSU1_PRESENT,
    PSU1_STATE,
    PSU1_ALERT,
    PSU2_PRESENT,
    PSU2_STATE,
    PSU2_ALERT,
    PSU1_EN,
    PSU1_EEPROM_WP,
    PSU2_EN,
    PSU2_EEPROM_WP,
    VCC_5V_EN,
    VCC_3V3_EN,
    VCC_VCORE_EN,
    VCC_MAC_1V8_EN,
    VCC_MAC_1V2_EN,
    VCC_MAC_0V8_EN,
    VCC_PLL_0V8_EN,
    VCC_SYS_EN,
    OOB_PWR_STATE,
    OOB_OP_EN,
    USB1_OP_EN,
    PSU_INTR,
    FAN_ALERT,
    SWPLD2_INTR,
    SWPLD3_INTR,
    PSU1_LED,
    PSU2_LED,
    SYS_LED,
    FAN_LED,
    FAN1_LED,
    FAN2_LED,
    FAN3_LED,
    FAN4_LED,
    FAN5_LED,
    FAN6_LED,
    SYNCE_EEPROM_WB,
    CONSOLE_SEL,
    SYS_EEPROM_WB,
};

/* SWPLD2 */
enum swpld2_attr{
    SWPLD2_VER_TYPE = 0,
    SWPLD2_VER,
    QSFP_P01_RST,
    QSFP_P02_RST,    
    QSFP_P03_RST,
    QSFP_P04_RST,
    QSFP_P05_RST,
    QSFP_P06_RST,
    QSFP_P07_RST,
    QSFP_P08_RST,
    QSFP_P09_RST,
    QSFP_P10_RST,
    QSFP_P11_RST,
    QSFP_P12_RST,
    QSFP_P13_RST,
    QSFP_P14_RST,
    QSFP_P15_RST,
    QSFP_P16_RST,
    QSFP_P01_LPMODE,
    QSFP_P02_LPMODE,
    QSFP_P03_LPMODE,
    QSFP_P04_LPMODE,
    QSFP_P05_LPMODE,
    QSFP_P06_LPMODE,
    QSFP_P07_LPMODE,
    QSFP_P08_LPMODE,
    QSFP_P09_LPMODE,
    QSFP_P10_LPMODE,
    QSFP_P11_LPMODE,
    QSFP_P12_LPMODE,
    QSFP_P13_LPMODE,
    QSFP_P14_LPMODE,
    QSFP_P15_LPMODE,
    QSFP_P16_LPMODE,
    QSFP_P01_MODPRS,
    QSFP_P02_MODPRS,
    QSFP_P03_MODPRS,
    QSFP_P04_MODPRS,
    QSFP_P05_MODPRS,
    QSFP_P06_MODPRS,
    QSFP_P07_MODPRS,
    QSFP_P08_MODPRS,
    QSFP_P09_MODPRS,
    QSFP_P10_MODPRS,
    QSFP_P11_MODPRS,
    QSFP_P12_MODPRS,
    QSFP_P13_MODPRS,
    QSFP_P14_MODPRS,
    QSFP_P15_MODPRS,
    QSFP_P16_MODPRS,
    QSFP_P01_INTR,
    QSFP_P02_INTR,
    QSFP_P03_INTR,
    QSFP_P04_INTR,
    QSFP_P05_INTR,
    QSFP_P06_INTR,
    QSFP_P07_INTR,
    QSFP_P08_INTR,
    QSFP_P09_INTR,
    QSFP_P10_INTR,
    QSFP_P11_INTR,
    QSFP_P12_INTR,
    QSFP_P13_INTR,
    QSFP_P14_INTR,
    QSFP_P15_INTR,
    QSFP_P16_INTR,
};

/* SWPLD3 */
enum swpld3_attr{
    SWPLD3_VER_TYPE = 160,
    SWPLD3_VER,
    QSFP_P17_RST,
    QSFP_P18_RST,
    QSFP_P19_RST,
    QSFP_P20_RST,
    QSFP_P21_RST,
    QSFP_P22_RST,
    QSFP_P23_RST,
    QSFP_P24_RST,
    QSFP_P25_RST,
    QSFP_P26_RST,
    QSFP_P27_RST,
    QSFP_P28_RST,
    QSFP_P29_RST,
    QSFP_P30_RST,
    QSFP_P31_RST,
    QSFP_P32_RST,
    QSFP_P17_LPMODE,
    QSFP_P18_LPMODE,
    QSFP_P19_LPMODE,
    QSFP_P20_LPMODE,
    QSFP_P21_LPMODE,
    QSFP_P22_LPMODE,
    QSFP_P23_LPMODE,
    QSFP_P24_LPMODE,
    QSFP_P25_LPMODE,
    QSFP_P26_LPMODE,
    QSFP_P27_LPMODE,
    QSFP_P28_LPMODE,
    QSFP_P29_LPMODE,
    QSFP_P30_LPMODE,
    QSFP_P31_LPMODE,
    QSFP_P32_LPMODE,
    QSFP_P17_MODPRS,
    QSFP_P18_MODPRS,
    QSFP_P19_MODPRS,
    QSFP_P20_MODPRS,
    QSFP_P21_MODPRS,
    QSFP_P22_MODPRS,
    QSFP_P23_MODPRS,
    QSFP_P24_MODPRS,
    QSFP_P25_MODPRS,
    QSFP_P26_MODPRS,
    QSFP_P27_MODPRS,
    QSFP_P28_MODPRS,
    QSFP_P29_MODPRS,
    QSFP_P30_MODPRS,
    QSFP_P31_MODPRS,
    QSFP_P32_MODPRS,
    QSFP_P17_INTR,
    QSFP_P18_INTR,
    QSFP_P19_INTR,
    QSFP_P20_INTR,
    QSFP_P21_INTR,
    QSFP_P22_INTR,
    QSFP_P23_INTR,
    QSFP_P24_INTR,
    QSFP_P25_INTR,
    QSFP_P26_INTR,
    QSFP_P27_INTR,
    QSFP_P28_INTR,
    QSFP_P29_INTR,
    QSFP_P30_INTR,
    QSFP_P31_INTR,
    QSFP_P32_INTR,
    SFP_P0_MODPRS,
    SFP_P0_RXLOS,
    SFP_P0_TXFAULT,
    SFP_P1_MODPRS,
    SFP_P1_RXLOS,
    SFP_P1_TXFAULT,
    SFP_P0_TXDIS,
    SFP_P1_TXDIS,
};


struct platform_data {
    int reg_addr;
    struct i2c_client *client;
};

struct cpld_platform_data {
    int reg_addr;
    struct i2c_client *client;
};

static struct kobject *kobj_swpld1;
static struct kobject *kobj_swpld2;
static struct kobject *kobj_swpld3;

static struct kobject *kobj_test1;
static struct kobject *kobj_test2;

unsigned char swpld1_reg_addr;
unsigned char swpld2_reg_addr;
unsigned char swpld3_reg_addr;



static ssize_t swpld1_data_show(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld1->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    bool hex_fmt = 0;
    char desc[256] = {0};

    select = attr->index;
    switch(select) {
        case BOARD_ID:
            offset = 0x1;
            hex_fmt = 1;
            scnprintf(desc, PAGE_SIZE, "\nBorad ID.\n");
            break;
        case BOARD_VER:
            offset = 0x2;
            hex_fmt = 1;
            scnprintf(desc, PAGE_SIZE, "\nBorad Version.\n");
            break;
        case SWPLD1_VER_TYPE:
            offset = 0x3;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nSWPLD1 Version Type.\n");
            break;
        case SWPLD1_VER:
            offset = 0x3;
            mask = 0x7f;
            scnprintf(desc, PAGE_SIZE, "\nSWPLD1 Version.\n");
            break;
        case USB_HUB_RST:
            offset = 0x6;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nUSB Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case SYNCE_RST:
            offset = 0x6;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nSynce Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case BMC_RST:
            offset = 0x6;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nBMC Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case LPC_RST:
            offset = 0x6;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nBMC LPC Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case OOB_PCIE_RST:
            offset = 0x6;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nOOB PCIE Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case MAC_PCIE_RST:
            offset = 0x6;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nMAC PCIE Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case MAC_RST:
            offset = 0x6;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nMAC Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case VR_3V3_RST:
            offset = 0x7;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nVR 3V3 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case VR_0V8_RST:
            offset = 0x7;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nVR 0V8 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case FAN_MUX_RST:
            offset = 0x8;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nFAN I2C Mux Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case QSFP5_MUX_RST:
            offset = 0x8;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nQSFP5 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case QSFP4_MUX_RST:
            offset = 0x8;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nQSFP4 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case QSFP3_MUX_RST:
            offset = 0x8;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nQSFP3 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case QSFP2_MUX_RST:
            offset = 0x8;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nQSFP2 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case QSFP1_MUX_RST:
            offset = 0x8;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nQSFP1 Reset.\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case PSU1_PRESENT:
            offset = 0x11;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU1 Not Present.\n“0” = PSU1 Present.\n");
            break;
        case PSU1_STATE:
            offset = 0x11;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU1 Power Not Good.\n“0” = PSU1 Power Good.\n");
            break;
        case PSU1_ALERT:
            offset = 0x11;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU1 Alert Active.\n“0” = PSU1 Alert Not Active.\n");
            break;
        case PSU2_PRESENT:
            offset = 0x11;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU2 Not Present.\n“0” = PSU2 Present.\n");
            break;
        case PSU2_STATE:
            offset = 0x11;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case PSU2_ALERT:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU2 Alert Active.\n“0” = PSU2 Alert Not Active.\n");
            break;
        case PSU1_EN:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable PSU1.\n“0” = Enable PSU1.\n");
            break;
        case PSU1_EEPROM_WP:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable PSU1 EEPROM Write Protect.\n“0” = Disable PSU1 EEPROM Write Protect.\n");
            break;
        case PSU2_EN:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable PSU2.\n“0” = Enable PSU2.\n");
            break;
        case PSU2_EEPROM_WP:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable PSU2 EEPROM Write Protect.\n“0” = Disable PSU2 EEPROM Write Protect.\n");
            break;
        case VCC_5V_EN:
            offset = 0x23;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable VCC 5V.\n“0” = Disable VCC 5V.\n");
            break;
        case VCC_3V3_EN:
            offset = 0x23;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable VCC 3V3.\n“0” = Disable VCC 3V3.\n");
            break;
        case VCC_VCORE_EN:
            offset = 0x23;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable AVS 0V91.\n“0” = Disable AVS 0V91.\n");
            break;
        case VCC_MAC_1V8_EN:
            offset = 0x23;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable MAC 1V8.\n“0” = Disable MAC 1V8.\n");
            break;
        case VCC_MAC_1V2_EN:
            offset = 0x23;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable MAC 1V2.\n“0” = Disable MAC 1V2.\n");
            break;
        case VCC_MAC_0V8_EN:
            offset = 0x23;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable MAC 0V8.\n“0” = Disable MAC 0V8.\n");
            break;
        case VCC_PLL_0V8_EN:
            offset = 0x23;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable PLL 0V8.\n“0” = Disable PLL 0V8.\n");
            break;
        case VCC_SYS_EN:
            offset = 0x23;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable System Power.\n“0” = Disable System Power.\n");
            break;
        case OOB_PWR_STATE:
            offset = 0x24;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = OOB Power Rail Good.\n“0” = OOB Power Rail Not Good.\n");
            break;
        case OOB_OP_EN:
            offset = 0x24;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Enable OOB Operation.\n“0” = Disable OOB Operation.\n");
            break;
        case USB1_OP_EN:
            offset = 0x24;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable USB Operation.\n“0” = Enable USB Operation.\n");
            break;
        case PSU_INTR:
            offset = 0x26;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = PSU Interrupt Not Occured.\n“0” = PSU Interrupt Occured.\n");
            break;
        case FAN_ALERT:
            offset = 0x26;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Fan Interrupt Not Occured.\n“0” = Fan Interrupt Occured.\n");
            break;
        case SWPLD2_INTR:
            offset = 0x27;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = SWPLD2 Interrupt Not Occured.\n“0” = SWPLD2 Interrupt Occured.\n");
            break;
        case SWPLD3_INTR:
            offset = 0x27;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = SWPLD3 Interrupt Not Occured.\n“0” = SWPLD3 Interrupt Occured.\n)");
            break;
        case PSU1_LED:
            offset = 0x41;
            shift = 2;
            mask = 0x0C;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = Auto.\n");
            break;
        case PSU2_LED:
            offset = 0x41;
            shift = 0;
            mask = 0x03;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = Auto.\n");
            break;
        case SYS_LED:
            offset = 0x42;
            shift = 2;
            mask = 0x0C;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Red\n“2” = LED Blink Green.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN_LED:
            offset = 0x42;
            shift = 0;
            mask = 0x03;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Amber.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN1_LED:
            offset = 0x46;
            shift = 6;
            mask = 0xc0;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN2_LED:
            offset = 0x46;
            shift = 4;
            mask = 0x30;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN3_LED:
            offset = 0x46;
            shift = 2;
            mask = 0x0c;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN4_LED:
            offset = 0x46;
            shift = 0;
            mask = 0x03;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN5_LED:
            offset = 0x47;
            shift = 6;
            mask = 0xc0;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case FAN6_LED:
            offset = 0x47;
            shift = 4;
            mask = 0x30;
            scnprintf(desc, PAGE_SIZE, "\n“3” = LED Off\n“2” = LED Red.\n“1” = LED Green\n“0” = LED Off.\n");
            break;
        case SYNCE_EEPROM_WB:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case CONSOLE_SEL:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
        case SYS_EEPROM_WB:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation\n“0” = Reset.\n");
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[swpld1].client, offset);
    value = (value & mask) >> shift;
    if(hex_fmt) {
        return scnprintf(buf, PAGE_SIZE, "0x%02x%s", value, desc);
    } else {
        return scnprintf(buf, PAGE_SIZE, "%d%s", value, desc);
    }
}


static ssize_t swpld1_data_store(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld1->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    int err = 0;
    unsigned long data;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    switch (attr->index) {
        case USB_HUB_RST:
            offset = 0x6;
            shift = 6;
            mask = (1 << shift);
            break;
        case SYNCE_RST:
            offset = 0x6;
            shift = 5;
            mask = (1 << shift);
            break;
        case BMC_RST:
            offset = 0x6;
            shift = 4;
            mask = (1 << shift);
            break;
        case LPC_RST:
            offset = 0x6;
            shift = 3;
            mask = (1 << shift);
            break;
        case OOB_PCIE_RST:
            offset = 0x6;
            shift = 2;
            mask = (1 << shift);
            break;
        case MAC_PCIE_RST:
            offset = 0x6;
            shift = 1;
            mask = (1 << shift);
            break;
        case MAC_RST:
            offset = 0x6;
            shift = 0;
            mask = (1 << shift);
            break;
        case VR_3V3_RST:
            offset = 0x7;
            shift = 3;
            mask = (1 << shift);
            break;
        case VR_0V8_RST:
            offset = 0x7;
            shift = 2;
            mask = (1 << shift);
            break;
        case FAN_MUX_RST:
            offset = 0x8;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP5_MUX_RST:
            offset = 0x8;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP4_MUX_RST:
            offset = 0x8;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP3_MUX_RST:
            offset = 0x8;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP2_MUX_RST:
            offset = 0x8;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP1_MUX_RST:
            offset = 0x8;
            shift = 1;
            mask = (1 << shift);
            break;
         case PSU1_EN:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            break;
        case PSU1_EEPROM_WP:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            break;
        case PSU2_EN:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            break;
        case PSU2_EEPROM_WP:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            break;
        case VCC_5V_EN:
            offset = 0x23;
            shift = 7;
            mask = (1 << shift);
            break;
        case VCC_3V3_EN:
            offset = 0x23;
            shift = 6;
            mask = (1 << shift);
            break;
        case VCC_VCORE_EN:
            offset = 0x23;
            shift = 5;
            mask = (1 << shift);
            break;
        case VCC_MAC_1V8_EN:
            offset = 0x23;
            shift = 4;
            mask = (1 << shift);
            break; 
        case VCC_MAC_1V2_EN:
            offset = 0x23;
            shift = 3;
            mask = (1 << shift);
            break;
        case VCC_MAC_0V8_EN:
            offset = 0x23;
            shift = 2;
            mask = (1 << shift);
            break;
        case VCC_PLL_0V8_EN:
            offset = 0x23;
            shift = 1;
            mask = (1 << shift);
            break;
        case VCC_SYS_EN:
            offset = 0x23;
            shift = 0;
            mask = (1 << shift);
            break;
        case OOB_OP_EN:
            offset = 0x24;
            shift = 6;
            mask = (1 << shift);
            break;
        case USB1_OP_EN:
            offset = 0x24;
            shift = 1;
            mask = (1 << shift);
            break;
        case PSU1_LED:
            offset = 0x41;
            shift = 2;
            mask = 0x0C;
            break;
        case PSU2_LED:
            offset = 0x41;
            shift = 0;
            mask = 0x03;
            break;
        case SYS_LED:
            offset = 0x42;
            shift = 2;
            mask = 0x0C;
            break;
        case FAN_LED:
            offset = 0x42;
            shift = 0;
            mask = 0x03;
            break;
        case FAN1_LED:
            offset = 0x46;
            shift = 6;
            mask = 0xc0;
            break;
        case FAN2_LED:
            offset = 0x46;
            shift = 4;
            mask = 0x30;
            break;
        case FAN3_LED:
            offset = 0x46;
            shift = 2;
            mask = 0x0c;
            break;
        case FAN4_LED:
            offset = 0x46;
            shift = 0;
            mask = 0x03;
            break;
        case FAN5_LED:
            offset = 0x47;
            shift = 6;
            mask = 0xc0;
            break;
        case FAN6_LED:
            offset = 0x47;
            shift = 4;
            mask = 0x30;
            break;
        case SYNCE_EEPROM_WB:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            break;
        case CONSOLE_SEL:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            break;
        case SYS_EEPROM_WB:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[swpld1].client, offset);
    data = (value & ~mask) | (data << shift);
    i2c_smbus_write_byte_data(pdata[swpld1].client, offset, data);

    return count;
}


/* offset 0x01 */
static SENSOR_DEVICE_ATTR(board_id,         S_IRUGO,           swpld1_data_show, NULL,              BOARD_ID);
/* offser 0x02 */
static SENSOR_DEVICE_ATTR(board_ver,        S_IRUGO,           swpld1_data_show, NULL,              BOARD_VER);
/* offset 0x03 */
static SENSOR_DEVICE_ATTR(swpld1_ver_type,  S_IRUGO,           swpld1_data_show, NULL,              SWPLD1_VER_TYPE);
static SENSOR_DEVICE_ATTR(swpld1_ver,       S_IRUGO,           swpld1_data_show, NULL,              SWPLD1_VER);
/* offset 0x06 */
static SENSOR_DEVICE_ATTR(usb_hub_rst,      S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, USB_HUB_RST);
static SENSOR_DEVICE_ATTR(synce_rst,        S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, SYNCE_RST);
static SENSOR_DEVICE_ATTR(bmc_rst,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, BMC_RST);
static SENSOR_DEVICE_ATTR(lpc_rst,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, LPC_RST);
static SENSOR_DEVICE_ATTR(oob_pcie_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, OOB_PCIE_RST);
static SENSOR_DEVICE_ATTR(mac_pcie_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, MAC_PCIE_RST);
static SENSOR_DEVICE_ATTR(mac_rst,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, MAC_RST);
/* offset 0x07 */
static SENSOR_DEVICE_ATTR(vr_3v3_rst,       S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VR_3V3_RST);
static SENSOR_DEVICE_ATTR(vr_0v8_rst,       S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VR_0V8_RST);
/* offset 0x08 */
static SENSOR_DEVICE_ATTR(fan_mux_rst,       S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN_MUX_RST);
static SENSOR_DEVICE_ATTR(qsfp5_mux_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, QSFP5_MUX_RST);
static SENSOR_DEVICE_ATTR(qsfp4_mux_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, QSFP4_MUX_RST);
static SENSOR_DEVICE_ATTR(qsfp3_mux_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, QSFP3_MUX_RST);
static SENSOR_DEVICE_ATTR(qsfp2_mux_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, QSFP2_MUX_RST);
static SENSOR_DEVICE_ATTR(qsfp1_mux_rst,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, QSFP1_MUX_RST);
/* offset 0x11 */
static SENSOR_DEVICE_ATTR(psu1_present,      S_IRUGO,           swpld1_data_show, NULL,              PSU1_PRESENT);
static SENSOR_DEVICE_ATTR(psu1_state,        S_IRUGO,           swpld1_data_show, NULL,              PSU1_STATE);
static SENSOR_DEVICE_ATTR(psu1_alert,        S_IRUGO,           swpld1_data_show, NULL,              PSU1_ALERT);
static SENSOR_DEVICE_ATTR(psu2_present,      S_IRUGO,           swpld1_data_show, NULL,              PSU2_PRESENT);
static SENSOR_DEVICE_ATTR(psu2_state,        S_IRUGO,           swpld1_data_show, NULL,              PSU2_STATE);
static SENSOR_DEVICE_ATTR(psu2_alert,        S_IRUGO,           swpld1_data_show, NULL,              PSU2_ALERT);
/* offset 0x12 */
static SENSOR_DEVICE_ATTR(psu1_en,           S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU1_EN);
static SENSOR_DEVICE_ATTR(psu1_eeprom_wp,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU1_EEPROM_WP);
static SENSOR_DEVICE_ATTR(psu2_en,           S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU2_EN);
static SENSOR_DEVICE_ATTR(psu2_eeprom_wp,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU2_EEPROM_WP);
/* offset 0x23 */
static SENSOR_DEVICE_ATTR(vcc_5v_en,         S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_5V_EN);
static SENSOR_DEVICE_ATTR(vcc_3v3_en,        S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_3V3_EN);
static SENSOR_DEVICE_ATTR(vcc_vcore_en,      S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_VCORE_EN);
static SENSOR_DEVICE_ATTR(vcc_mac_1v8_en,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_MAC_1V8_EN);
static SENSOR_DEVICE_ATTR(vcc_mac_1v2_en,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_MAC_1V2_EN);
static SENSOR_DEVICE_ATTR(vcc_mac_0v8_en,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_MAC_0V8_EN);
static SENSOR_DEVICE_ATTR(vcc_pll_0v8_en,    S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_PLL_0V8_EN);
static SENSOR_DEVICE_ATTR(vcc_sys_en,        S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, VCC_SYS_EN);
/* offset 0x24 */
static SENSOR_DEVICE_ATTR(oob_pwr_state,     S_IRUGO,           swpld1_data_show, NULL,              OOB_PWR_STATE);
static SENSOR_DEVICE_ATTR(oob_op_en,         S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, OOB_OP_EN);
static SENSOR_DEVICE_ATTR(usb1_op_en,        S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, USB1_OP_EN);
/* offset 0x26 */
static SENSOR_DEVICE_ATTR(psu_intr,          S_IRUGO,           swpld1_data_show, NULL,              PSU_INTR);
static SENSOR_DEVICE_ATTR(fan_alert,         S_IRUGO,           swpld1_data_show, NULL,              FAN_ALERT);
/* offset 0x27 */
static SENSOR_DEVICE_ATTR(swpld2_intr,       S_IRUGO,           swpld1_data_show, NULL,              SWPLD2_INTR);
static SENSOR_DEVICE_ATTR(swpld3_intr,       S_IRUGO,           swpld1_data_show, NULL,              SWPLD3_INTR);
/* offset 0x41 */
static SENSOR_DEVICE_ATTR(psu1_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU1_LED);
static SENSOR_DEVICE_ATTR(psu2_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, PSU2_LED);
/* offset 0x42 */
static SENSOR_DEVICE_ATTR(sys_led,           S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, SYS_LED);
static SENSOR_DEVICE_ATTR(fan_led,           S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN_LED);
/* offset 0x46 */
static SENSOR_DEVICE_ATTR(fan1_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN1_LED);
static SENSOR_DEVICE_ATTR(fan2_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN2_LED);
static SENSOR_DEVICE_ATTR(fan3_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN3_LED);
static SENSOR_DEVICE_ATTR(fan4_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN4_LED);
/* offset 0x47 */
static SENSOR_DEVICE_ATTR(fan5_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN5_LED);
static SENSOR_DEVICE_ATTR(fan6_led,          S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, FAN6_LED);
/* offset 0x51*/
static SENSOR_DEVICE_ATTR(synce_eeprom_wb,   S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, SYNCE_EEPROM_WB);
static SENSOR_DEVICE_ATTR(console_sel,       S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, CONSOLE_SEL);
static SENSOR_DEVICE_ATTR(sys_eeprom_wb,     S_IRUGO | S_IWUSR, swpld1_data_show, swpld1_data_store, SYS_EEPROM_WB);

static struct attribute *swpld1_device_attrs[] = {
    &sensor_dev_attr_board_id.dev_attr.attr,
    &sensor_dev_attr_board_ver.dev_attr.attr,
    &sensor_dev_attr_swpld1_ver_type.dev_attr.attr,
    &sensor_dev_attr_swpld1_ver.dev_attr.attr,
    &sensor_dev_attr_usb_hub_rst.dev_attr.attr,
    &sensor_dev_attr_synce_rst.dev_attr.attr,
    &sensor_dev_attr_bmc_rst.dev_attr.attr,
    &sensor_dev_attr_lpc_rst.dev_attr.attr,
    &sensor_dev_attr_oob_pcie_rst.dev_attr.attr,
    &sensor_dev_attr_mac_pcie_rst.dev_attr.attr,
    &sensor_dev_attr_mac_rst.dev_attr.attr,
    &sensor_dev_attr_vr_3v3_rst.dev_attr.attr,
    &sensor_dev_attr_vr_0v8_rst.dev_attr.attr,
    &sensor_dev_attr_fan_mux_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp5_mux_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp4_mux_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp3_mux_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp2_mux_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp1_mux_rst.dev_attr.attr,
    &sensor_dev_attr_psu1_present.dev_attr.attr,
    &sensor_dev_attr_psu1_state.dev_attr.attr,
    &sensor_dev_attr_psu1_alert.dev_attr.attr,
    &sensor_dev_attr_psu2_present.dev_attr.attr,
    &sensor_dev_attr_psu2_state.dev_attr.attr,
    &sensor_dev_attr_psu2_alert.dev_attr.attr,
    &sensor_dev_attr_psu1_en.dev_attr.attr,
    &sensor_dev_attr_psu1_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_psu2_en.dev_attr.attr,
    &sensor_dev_attr_psu2_eeprom_wp.dev_attr.attr,
    &sensor_dev_attr_vcc_5v_en.dev_attr.attr,
    &sensor_dev_attr_vcc_3v3_en.dev_attr.attr,
    &sensor_dev_attr_vcc_vcore_en.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_1v8_en.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_1v2_en.dev_attr.attr,
    &sensor_dev_attr_vcc_mac_0v8_en.dev_attr.attr,
    &sensor_dev_attr_vcc_pll_0v8_en.dev_attr.attr,
    &sensor_dev_attr_vcc_sys_en.dev_attr.attr,
    &sensor_dev_attr_oob_pwr_state.dev_attr.attr,
    &sensor_dev_attr_oob_op_en.dev_attr.attr,
    &sensor_dev_attr_usb1_op_en.dev_attr.attr,
    &sensor_dev_attr_psu_intr.dev_attr.attr,
    &sensor_dev_attr_fan_alert.dev_attr.attr,
    &sensor_dev_attr_swpld2_intr.dev_attr.attr,
    &sensor_dev_attr_swpld3_intr.dev_attr.attr,
    &sensor_dev_attr_psu1_led.dev_attr.attr,
    &sensor_dev_attr_psu2_led.dev_attr.attr,
    &sensor_dev_attr_sys_led.dev_attr.attr,
    &sensor_dev_attr_fan_led.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &sensor_dev_attr_fan3_led.dev_attr.attr,
    &sensor_dev_attr_fan4_led.dev_attr.attr,
    &sensor_dev_attr_fan5_led.dev_attr.attr,
    &sensor_dev_attr_fan6_led.dev_attr.attr,
    &sensor_dev_attr_synce_eeprom_wb.dev_attr.attr,
    &sensor_dev_attr_console_sel.dev_attr.attr,
    &sensor_dev_attr_sys_eeprom_wb.dev_attr.attr,
};


static ssize_t swpld2_data_show(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld2->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    bool hex_fmt = 0;
    char desc[256] = {0};

    select = attr->index;
    switch(select) {
        case SWPLD2_VER_TYPE:
            offset = 0x1;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nSWPLD2 Version Type.\n");
            break;
        case SWPLD2_VER:
            offset = 0x1;
            mask = 0x7f;
            scnprintf(desc, PAGE_SIZE, "\nSWPLD2 Version.\n");
            break;
        case QSFP_P01_RST:
            offset = 0x11;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P02_RST:
            offset = 0x11;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P03_RST:
            offset = 0x11;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P04_RST:
            offset = 0x11;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P05_RST:
            offset = 0x11;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P06_RST:
            offset = 0x11;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P07_RST:
            offset = 0x11;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P08_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P09_RST:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P10_RST:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P11_RST:
            offset = 0x12;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P12_RST:
            offset = 0x12;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P13_RST:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P14_RST:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P15_RST:
            offset = 0x12;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P16_RST:
            offset = 0x12;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P01_LPMODE:
            offset = 0x21;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P02_LPMODE:
            offset = 0x21;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P03_LPMODE:
            offset = 0x21;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P04_LPMODE:
            offset = 0x21;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P05_LPMODE:
            offset = 0x21;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P06_LPMODE:
            offset = 0x21;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P07_LPMODE:
            offset = 0x21;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P08_LPMODE:
            offset = 0x21;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P09_LPMODE:
            offset = 0x22;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P10_LPMODE:
            offset = 0x22;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P11_LPMODE:
            offset = 0x22;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P12_LPMODE:
            offset = 0x22;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P13_LPMODE:
            offset = 0x22;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P14_LPMODE:
            offset = 0x22;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P15_LPMODE:
            offset = 0x22;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P16_LPMODE:
            offset = 0x22;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P01_MODPRS:
            offset = 0x51;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P02_MODPRS:
            offset = 0x51;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P03_MODPRS:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P04_MODPRS:
            offset = 0x51;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P05_MODPRS:
            offset = 0x51;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P06_MODPRS:
            offset = 0x51;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P07_MODPRS:
            offset = 0x51;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P08_MODPRS:
            offset = 0x51;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P09_MODPRS:
            offset = 0x52;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P10_MODPRS:
            offset = 0x52;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P11_MODPRS:
            offset = 0x52;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P12_MODPRS:
            offset = 0x52;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P13_MODPRS:
            offset = 0x52;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P14_MODPRS:
            offset = 0x52;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P15_MODPRS:
            offset = 0x52;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P16_MODPRS:
            offset = 0x52;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P01_INTR:
            offset = 0x61;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P02_INTR:
            offset = 0x61;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P03_INTR:
            offset = 0x61;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P04_INTR:
            offset = 0x61;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P05_INTR:
            offset = 0x61;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P06_INTR:
            offset = 0x61;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P07_INTR:
            offset = 0x61;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P08_INTR:
            offset = 0x61;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P09_INTR:
            offset = 0x62;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P10_INTR:
            offset = 0x62;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P11_INTR:
            offset = 0x62;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P12_INTR:
            offset = 0x62;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P13_INTR:
            offset = 0x62;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P14_INTR:
            offset = 0x62;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P15_INTR:
            offset = 0x62;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P16_INTR:
            offset = 0x62;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[swpld2].client, offset);
    value = (value & mask) >> shift;
    if(hex_fmt) {
        return scnprintf(buf, PAGE_SIZE, "0x%02x%s", value, desc);
    } else {
        return scnprintf(buf, PAGE_SIZE, "%d%s", value, desc);
    }
}

static ssize_t swpld2_data_store(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld2->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    int err = 0;
    unsigned long data;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    switch (attr->index) {
        case QSFP_P01_RST:
            offset = 0x11;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P02_RST:
            offset = 0x11;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P03_RST:
            offset = 0x11;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P04_RST:
            offset = 0x11;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P05_RST:
            offset = 0x11;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P06_RST:
            offset = 0x11;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P07_RST:
            offset = 0x11;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P08_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            break;
        case QSFP_P09_RST:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P10_RST:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P11_RST:
            offset = 0x12;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P12_RST:
            offset = 0x12;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P13_RST:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P14_RST:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P15_RST:
            offset = 0x12;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P16_RST:
            offset = 0x12;
            shift = 0;
            mask = (1 << shift);
            break;
        case QSFP_P01_LPMODE:
            offset = 0x21;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P02_LPMODE:
            offset = 0x21;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P03_LPMODE:
            offset = 0x21;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P04_LPMODE:
            offset = 0x21;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P05_LPMODE:
            offset = 0x21;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P06_LPMODE:
            offset = 0x21;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P07_LPMODE:
            offset = 0x21;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P08_LPMODE:
            offset = 0x21;
            shift = 0;
            mask = (1 << shift);
            break;
         case QSFP_P09_LPMODE:
            offset = 0x22;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P10_LPMODE:
            offset = 0x22;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P11_LPMODE:
            offset = 0x22;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P12_LPMODE:
            offset = 0x22;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P13_LPMODE:
            offset = 0x22;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P14_LPMODE:
            offset = 0x22;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P15_LPMODE:
            offset = 0x22;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P16_LPMODE:
            offset = 0x22;
            shift = 0;
            mask = (1 << shift);
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[swpld2].client, offset);
    data = (value & ~mask) | (data << shift);
    i2c_smbus_write_byte_data(pdata[swpld2].client, offset, data);

    return count;
}


/* offset 0x01 */
static SENSOR_DEVICE_ATTR(swpld2_ver_type,  S_IRUGO,           swpld2_data_show, NULL,              SWPLD2_VER_TYPE);
static SENSOR_DEVICE_ATTR(swpld2_ver,       S_IRUGO,           swpld2_data_show, NULL,              SWPLD2_VER);
/* offset 0x11 */
static SENSOR_DEVICE_ATTR(qsfp_p01_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P01_RST);
static SENSOR_DEVICE_ATTR(qsfp_p02_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P02_RST);
static SENSOR_DEVICE_ATTR(qsfp_p03_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P03_RST);
static SENSOR_DEVICE_ATTR(qsfp_p04_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P04_RST);
static SENSOR_DEVICE_ATTR(qsfp_p05_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P05_RST);
static SENSOR_DEVICE_ATTR(qsfp_p06_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P06_RST);
static SENSOR_DEVICE_ATTR(qsfp_p07_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P07_RST);
static SENSOR_DEVICE_ATTR(qsfp_p08_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P08_RST);
/* offset 0x12 */
static SENSOR_DEVICE_ATTR(qsfp_p09_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P09_RST);
static SENSOR_DEVICE_ATTR(qsfp_p10_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P10_RST);
static SENSOR_DEVICE_ATTR(qsfp_p11_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P11_RST);
static SENSOR_DEVICE_ATTR(qsfp_p12_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P12_RST);
static SENSOR_DEVICE_ATTR(qsfp_p13_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P13_RST);
static SENSOR_DEVICE_ATTR(qsfp_p14_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P14_RST);
static SENSOR_DEVICE_ATTR(qsfp_p15_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P15_RST);
static SENSOR_DEVICE_ATTR(qsfp_p16_rst,     S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P16_RST);
/* offset 0x21 */
static SENSOR_DEVICE_ATTR(qsfp_p01_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P01_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p02_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P02_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p03_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P03_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p04_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P04_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p05_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P05_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p06_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P06_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p07_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P07_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p08_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P08_LPMODE);
/* offset 0x22 */
static SENSOR_DEVICE_ATTR(qsfp_p09_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P09_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p10_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P10_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p11_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P11_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p12_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P12_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p13_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P13_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p14_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P14_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p15_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P15_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p16_lpmode,  S_IRUGO | S_IWUSR, swpld2_data_show, swpld2_data_store, QSFP_P16_LPMODE);
/* offset 0x51 */
static SENSOR_DEVICE_ATTR(qsfp_p01_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P01_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p02_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P02_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p03_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P03_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p04_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P04_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p05_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P05_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p06_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P06_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p07_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P07_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p08_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P08_MODPRS);
/* offset 0x52 */
static SENSOR_DEVICE_ATTR(qsfp_p09_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P09_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p10_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P10_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p11_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P11_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p12_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P12_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p13_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P13_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p14_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P14_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p15_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P15_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p16_modprs,  S_IRUGO,           swpld2_data_show, NULL,              QSFP_P16_MODPRS);
/* offset 0x61 */
static SENSOR_DEVICE_ATTR(qsfp_p01_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P01_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p02_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P02_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p03_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P03_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p04_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P04_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p05_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P05_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p06_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P06_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p07_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P07_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p08_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P08_INTR);
/* offset 0x62 */
static SENSOR_DEVICE_ATTR(qsfp_p09_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P09_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p10_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P10_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p11_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P11_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p12_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P12_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p13_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P13_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p14_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P14_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p15_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P15_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p16_intr,    S_IRUGO,           swpld2_data_show, NULL,              QSFP_P16_INTR);


static struct attribute *swpld2_device_attrs[] = {
    &sensor_dev_attr_swpld2_ver_type.dev_attr.attr,
    &sensor_dev_attr_swpld2_ver.dev_attr.attr,
    &sensor_dev_attr_qsfp_p01_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p02_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p03_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p04_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p05_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p06_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p07_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p08_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p09_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p10_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p11_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p12_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p13_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p14_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p15_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p16_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p01_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p02_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p03_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p04_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p05_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p06_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p07_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p08_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p09_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p10_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p11_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p12_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p13_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p14_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p15_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p16_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p01_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p02_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p03_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p04_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p05_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p06_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p07_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p08_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p09_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p10_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p11_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p12_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p13_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p14_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p15_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p16_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p01_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p02_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p03_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p04_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p05_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p06_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p07_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p08_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p09_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p10_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p11_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p12_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p13_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p14_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p15_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p16_intr.dev_attr.attr,
};  


static ssize_t swpld3_data_show(struct device *dev, struct device_attribute *dev_attr, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld3->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    bool hex_fmt = 0;
    char desc[256] = {0};

    select = attr->index;
    switch(select) {
        case SWPLD3_VER_TYPE:
            offset = 0x1;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\nSWPLD3 Version Type.\n");
            break;
        case SWPLD3_VER:
            offset = 0x1;
            mask = 0x7f;
            scnprintf(desc, PAGE_SIZE, "\nSWPLD3 Version.\n");
            break;
        case QSFP_P17_RST:
            offset = 0x11;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P18_RST:
            offset = 0x11;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P19_RST:
            offset = 0x11;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P20_RST:
            offset = 0x11;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P21_RST:
            offset = 0x11;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P22_RST:
            offset = 0x11;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P23_RST:
            offset = 0x11;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P24_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P25_RST:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P26_RST:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P27_RST:
            offset = 0x12;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P28_RST:
            offset = 0x12;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P29_RST:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P30_RST:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P31_RST:
            offset = 0x12;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
        case QSFP_P32_RST:
            offset = 0x12;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Normal Operation.\n“0” = Reset.\n");
            break;
         case QSFP_P17_LPMODE:
            offset = 0x21;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P18_LPMODE:
            offset = 0x21;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P19_LPMODE:
            offset = 0x21;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P20_LPMODE:
            offset = 0x21;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P21_LPMODE:
            offset = 0x21;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P22_LPMODE:
            offset = 0x21;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P23_LPMODE:
            offset = 0x21;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P24_LPMODE:
            offset = 0x21;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P25_LPMODE:
            offset = 0x22;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P26_LPMODE:
            offset = 0x22;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P27_LPMODE:
            offset = 0x22;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P28_LPMODE:
            offset = 0x22;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
         case QSFP_P29_LPMODE:
            offset = 0x22;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P30_LPMODE:
            offset = 0x22;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P31_LPMODE:
            offset = 0x22;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P32_LPMODE:
            offset = 0x22;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = LP Mode.\n“0” = Not LP Mode.\n");
            break;
        case QSFP_P17_MODPRS:
            offset = 0x51;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P18_MODPRS:
            offset = 0x51;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P19_MODPRS:
            offset = 0x51;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P20_MODPRS:
            offset = 0x51;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
         case QSFP_P21_MODPRS:
            offset = 0x51;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P22_MODPRS:
            offset = 0x51;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P23_MODPRS:
            offset = 0x51;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P24_MODPRS:
            offset = 0x51;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P25_MODPRS:
            offset = 0x52;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P26_MODPRS:
            offset = 0x52;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P27_MODPRS:
            offset = 0x52;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P28_MODPRS:
            offset = 0x52;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P29_MODPRS:
            offset = 0x52;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P30_MODPRS:
            offset = 0x52;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P31_MODPRS:
            offset = 0x52;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P32_MODPRS:
            offset = 0x52;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Present.\n“0” = Present.\n");
            break;
        case QSFP_P17_INTR:
            offset = 0x61;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P18_INTR:
            offset = 0x61;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P19_INTR:
            offset = 0x61;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P20_INTR:
            offset = 0x61;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P21_INTR:
            offset = 0x61;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P22_INTR:
            offset = 0x61;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P23_INTR:
            offset = 0x61;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P24_INTR:
            offset = 0x61;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P25_INTR:
            offset = 0x62;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P26_INTR:
            offset = 0x62;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P27_INTR:
            offset = 0x62;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P28_INTR:
            offset = 0x62;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P29_INTR:
            offset = 0x62;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P30_INTR:
            offset = 0x62;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P31_INTR:
            offset = 0x62;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case QSFP_P32_INTR:
            offset = 0x62;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Not Assert Intrrupt.\n“0” = Assert Intrrupt.\n");
            break;
        case SFP_P0_MODPRS:
            offset = 0x71;
            shift = 6;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Absent.\n“0” = Present.\n");
            break;
        case SFP_P0_RXLOS:
            offset = 0x71;
            shift = 5;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Receive Loss.\n“0” = Normal Operation.\n");
            break;
        case SFP_P0_TXFAULT:
            offset = 0x71;
            shift = 4;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Transmit Fault.\n“0” = Normal Operation.\n");
            break;
        case SFP_P1_MODPRS:
            offset = 0x71;
            shift = 2;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Absent.\n“0” = Present.\n");
            break;
        case SFP_P1_RXLOS:
            offset = 0x71;
            shift = 1;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Receive Loss.\n“0” = Normal Operation.\n");
            break;
        case SFP_P1_TXFAULT:
            offset = 0x71;
            shift = 0;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Transmit Fault.\n“0” = Normal Operation.\n");
            break;
        case SFP_P0_TXDIS:
            offset = 0x72;
            shift = 7;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable.\n“0” = Normal Operation.\n");
            break;
        case SFP_P1_TXDIS:
            offset = 0x72;
            shift = 3;
            mask = (1 << shift);
            scnprintf(desc, PAGE_SIZE, "\n“1” = Disable.\n“0” = Normal Operation.\n");
            break;
    }

    value = i2c_smbus_read_byte_data(pdata[swpld3].client, offset);
    value = (value & mask) >> shift;
    if(hex_fmt) {
        return scnprintf(buf, PAGE_SIZE, "0x%02x%s", value, desc);
    } else {
        return scnprintf(buf, PAGE_SIZE, "%d%s", value, desc);
    }
}


static ssize_t swpld3_data_store(struct device *dev, struct device_attribute *dev_attr,
             const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(dev_attr);
    struct device *i2cdev = kobj_to_dev(kobj_swpld3->parent);
    struct platform_data *pdata = i2cdev->platform_data;
    unsigned int select = 0;
    unsigned char offset = 0;
    int mask = 0xFF;
    int shift = 0;
    int value = 0;
    int err = 0;
    unsigned long data;

    err = kstrtoul(buf, 0, &data);
    if (err){
        return err;
    }

    if (data > 0xff){
        printk(KERN_ALERT "address out of range (0x00-0xFF)\n");
        return count;
    }

    switch (attr->index) {
        case QSFP_P17_RST:
            offset = 0x11;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P18_RST:
            offset = 0x11;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P19_RST:
            offset = 0x11;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P20_RST:
            offset = 0x11;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P21_RST:
            offset = 0x11;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P22_RST:
            offset = 0x11;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P23_RST:
            offset = 0x11;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P24_RST:
            offset = 0x11;
            shift = 0;
            mask = (1 << shift);
            break;
        case QSFP_P25_RST:
            offset = 0x12;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P26_RST:
            offset = 0x12;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P27_RST:
            offset = 0x12;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P28_RST:
            offset = 0x12;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P29_RST:
            offset = 0x12;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P30_RST:
            offset = 0x12;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P31_RST:
            offset = 0x12;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P32_RST:
            offset = 0x12;
            shift = 0;
            mask = (1 << shift);
            break;
        case QSFP_P17_LPMODE:
            offset = 0x21;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P18_LPMODE:
            offset = 0x21;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P19_LPMODE:
            offset = 0x21;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P20_LPMODE:
            offset = 0x21;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P21_LPMODE:
            offset = 0x21;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P22_LPMODE:
            offset = 0x21;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P23_LPMODE:
            offset = 0x21;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P24_LPMODE:
            offset = 0x21;
            shift = 0;
            mask = (1 << shift);
            break;
        case QSFP_P25_LPMODE:
            offset = 0x22;
            shift = 7;
            mask = (1 << shift);
            break;
        case QSFP_P26_LPMODE:
            offset = 0x22;
            shift = 6;
            mask = (1 << shift);
            break;
        case QSFP_P27_LPMODE:
            offset = 0x22;
            shift = 5;
            mask = (1 << shift);
            break;
        case QSFP_P28_LPMODE:
            offset = 0x22;
            shift = 4;
            mask = (1 << shift);
            break;
        case QSFP_P29_LPMODE:
            offset = 0x22;
            shift = 3;
            mask = (1 << shift);
            break;
        case QSFP_P30_LPMODE:
            offset = 0x22;
            shift = 2;
            mask = (1 << shift);
            break;
        case QSFP_P31_LPMODE:
            offset = 0x22;
            shift = 1;
            mask = (1 << shift);
            break;
        case QSFP_P32_LPMODE:
            offset = 0x22;
            shift = 0;
            mask = (1 << shift);
            break;
        case SFP_P0_TXDIS:
            offset = 0x72;
            shift = 7;
            break;
        case SFP_P1_TXDIS:
            offset = 0x72;
            shift = 3;
            mask = (1 << shift);
            break;

    }

    value = i2c_smbus_read_byte_data(pdata[swpld3].client, offset);
    data = (value & ~mask) | (data << shift);
    i2c_smbus_write_byte_data(pdata[swpld3].client, offset, data);

    return count;
}

/* offset 0x01 */
static SENSOR_DEVICE_ATTR(swpld3_ver_type,  S_IRUGO,           swpld3_data_show, NULL,              SWPLD3_VER_TYPE);
static SENSOR_DEVICE_ATTR(swpld3_ver,       S_IRUGO,           swpld3_data_show, NULL,              SWPLD3_VER);
/* offset 0x11 */
static SENSOR_DEVICE_ATTR(qsfp_p17_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P17_RST);
static SENSOR_DEVICE_ATTR(qsfp_p18_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P18_RST);
static SENSOR_DEVICE_ATTR(qsfp_p19_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P19_RST);
static SENSOR_DEVICE_ATTR(qsfp_p20_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P20_RST);
static SENSOR_DEVICE_ATTR(qsfp_p21_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P21_RST);
static SENSOR_DEVICE_ATTR(qsfp_p22_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P22_RST);
static SENSOR_DEVICE_ATTR(qsfp_p23_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P23_RST);
static SENSOR_DEVICE_ATTR(qsfp_p24_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P24_RST);
/* offset 0x12 */
static SENSOR_DEVICE_ATTR(qsfp_p25_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P25_RST);
static SENSOR_DEVICE_ATTR(qsfp_p26_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P26_RST);
static SENSOR_DEVICE_ATTR(qsfp_p27_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P27_RST);
static SENSOR_DEVICE_ATTR(qsfp_p28_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P28_RST);
static SENSOR_DEVICE_ATTR(qsfp_p29_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P29_RST);
static SENSOR_DEVICE_ATTR(qsfp_p30_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P30_RST);
static SENSOR_DEVICE_ATTR(qsfp_p31_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P31_RST);
static SENSOR_DEVICE_ATTR(qsfp_p32_rst,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P32_RST);
/* offset 0x21 */
static SENSOR_DEVICE_ATTR(qsfp_p17_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P17_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p18_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P18_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p19_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P19_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p20_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P20_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p21_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P21_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p22_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P22_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p23_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P23_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p24_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P24_LPMODE);
/* offset 0x22 */
static SENSOR_DEVICE_ATTR(qsfp_p25_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P25_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p26_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P26_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p27_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P27_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p28_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P28_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p29_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P29_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p30_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P30_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p31_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P31_LPMODE);
static SENSOR_DEVICE_ATTR(qsfp_p32_lpmode,  S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, QSFP_P32_LPMODE);
/* offset 0x51 */
static SENSOR_DEVICE_ATTR(qsfp_p17_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P17_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p18_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P18_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p19_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P19_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p20_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P20_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p21_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P21_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p22_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P22_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p23_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P23_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p24_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P24_MODPRS);
/* offset 0x52 */
static SENSOR_DEVICE_ATTR(qsfp_p25_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P25_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p26_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P26_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p27_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P27_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p28_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P28_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p29_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P29_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p30_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P30_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p31_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P31_MODPRS);
static SENSOR_DEVICE_ATTR(qsfp_p32_modprs,  S_IRUGO,           swpld3_data_show, NULL,              QSFP_P32_MODPRS);
/* offset 0x61 */
static SENSOR_DEVICE_ATTR(qsfp_p17_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P17_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p18_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P18_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p19_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P19_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p20_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P20_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p21_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P21_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p22_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P22_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p23_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P23_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p24_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P24_INTR);
/* offset 0x62 */
static SENSOR_DEVICE_ATTR(qsfp_p25_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P25_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p26_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P26_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p27_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P27_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p28_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P28_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p29_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P29_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p30_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P30_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p31_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P31_INTR);
static SENSOR_DEVICE_ATTR(qsfp_p32_intr,    S_IRUGO,           swpld3_data_show, NULL,              QSFP_P32_INTR);
/* offset 0x71 */
static SENSOR_DEVICE_ATTR(sfp_p0_modprs,    S_IRUGO,           swpld3_data_show, NULL,              SFP_P0_MODPRS);
static SENSOR_DEVICE_ATTR(sfp_p0_rxlos,     S_IRUGO,           swpld3_data_show, NULL,              SFP_P0_RXLOS);
static SENSOR_DEVICE_ATTR(sfp_p0_txfault,   S_IRUGO,           swpld3_data_show, NULL,              SFP_P0_TXFAULT);
static SENSOR_DEVICE_ATTR(sfp_p1_modprs,    S_IRUGO,           swpld3_data_show, NULL,              SFP_P1_MODPRS);
static SENSOR_DEVICE_ATTR(sfp_p1_rxlos,     S_IRUGO,           swpld3_data_show, NULL,              SFP_P1_RXLOS);
static SENSOR_DEVICE_ATTR(sfp_p1_txfault,   S_IRUGO,           swpld3_data_show, NULL,              SFP_P1_TXFAULT);
/* offset 0x72 */
static SENSOR_DEVICE_ATTR(sfp_p0_txdis,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, SFP_P0_TXDIS);
static SENSOR_DEVICE_ATTR(sfp_p1_txdis,     S_IRUGO | S_IWUSR, swpld3_data_show, swpld3_data_store, SFP_P1_TXDIS);

static struct attribute *swpld3_device_attrs[] = {
    &sensor_dev_attr_swpld3_ver_type.dev_attr.attr,
    &sensor_dev_attr_swpld3_ver.dev_attr.attr,
    &sensor_dev_attr_qsfp_p17_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p18_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p19_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p20_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p21_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p22_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p23_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p24_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p25_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p26_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p27_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p28_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p29_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p30_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p31_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p32_rst.dev_attr.attr,
    &sensor_dev_attr_qsfp_p17_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p18_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p19_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p20_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p21_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p22_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p23_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p24_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p25_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p26_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p27_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p28_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p29_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p30_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p31_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p32_lpmode.dev_attr.attr,
    &sensor_dev_attr_qsfp_p17_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p18_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p19_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p20_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p21_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p22_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p23_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p24_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p25_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p26_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p27_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p28_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p29_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p30_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p31_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p32_modprs.dev_attr.attr,
    &sensor_dev_attr_qsfp_p17_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p18_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p19_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p20_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p21_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p22_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p23_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p24_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p25_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p26_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p27_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p28_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p29_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p30_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p31_intr.dev_attr.attr,
    &sensor_dev_attr_qsfp_p32_intr.dev_attr.attr,
    &sensor_dev_attr_sfp_p0_modprs.dev_attr.attr,
    &sensor_dev_attr_sfp_p0_rxlos.dev_attr.attr,
    &sensor_dev_attr_sfp_p0_txfault.dev_attr.attr,
    &sensor_dev_attr_sfp_p1_modprs.dev_attr.attr,
    &sensor_dev_attr_sfp_p1_rxlos.dev_attr.attr,
    &sensor_dev_attr_sfp_p1_txfault.dev_attr.attr,
    &sensor_dev_attr_sfp_p0_txdis.dev_attr.attr,
    &sensor_dev_attr_sfp_p1_txdis.dev_attr.attr,
};

static struct attribute_group swpld1_device_attr_group = {
    .attrs = swpld1_device_attrs,
};

static struct attribute_group swpld2_device_attr_group = {
    .attrs = swpld2_device_attrs,
};

static struct attribute_group swpld3_device_attr_group = {
    .attrs = swpld3_device_attrs,
};


static struct cpld_platform_data agc032_swpld_platform_data[] = {
    [swpld1] = {
        .reg_addr = SWPLD1_ADDR,
    },
    [swpld2] = {
        .reg_addr = SWPLD2_ADDR,
    },
    [swpld3] = {
        .reg_addr = SWPLD3_ADDR,
    },
};

static void device_release(struct device *dev)
{
    return;
}


static struct platform_device swpld_device = {
        .name               = "delta-agc032-swpld",
        .id                 = 0,
        .dev                = {
                    .platform_data   = agc032_swpld_platform_data,
                    .release         = device_release
        },
};


static int __init swpld_probe(struct platform_device *pdev)
{
    int ret;
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLD platform data not found\n");
        return -ENODEV;
    }

    parent = i2c_get_adapter(BUS8);
    if (!parent) {
        printk(KERN_ERR "Parent adapter (%d) not found\n", BUS8);
        return -ENODEV;
    }

    pdata[swpld1].client = i2c_new_dummy(parent, pdata[swpld1].reg_addr);
    if (!pdata[swpld1].client) {
        printk(KERN_ERR "Fail to create dummy i2c client for addr %d\n", pdata[swpld1].reg_addr);
        goto error_swpld1;
    }

    pdata[swpld2].client = i2c_new_dummy(parent, pdata[swpld2].reg_addr);
    if (!pdata[swpld2].client) {
        printk(KERN_ERR "Fail to create dummy i2c client for addr %d\n", pdata[swpld2].reg_addr);
        goto error_swpld2;
    }

    pdata[swpld3].client = i2c_new_dummy(parent, pdata[swpld3].reg_addr);
    if (!pdata[swpld3].client) {
        printk(KERN_ERR "Fail to create dummy i2c client for addr %d\n", pdata[swpld3].reg_addr);
        goto error_swpld3;
    }
#if 1
    kobj_swpld1 = kobject_create_and_add("SWPLD1", &pdev->dev.kobj);
    if (!kobj_swpld1){
        printk(KERN_ERR "Fail to create directory");
        goto error_swpld3;
    }
 
    kobj_swpld2 = kobject_create_and_add("SWPLD2", &pdev->dev.kobj);
    if (!kobj_swpld2){
        printk(KERN_ERR "Fail to create directory");
        goto error_swpld3;
    }

    kobj_swpld3 = kobject_create_and_add("SWPLD3", &pdev->dev.kobj);
    if (!kobj_swpld3){
        printk(KERN_ERR "Fail to create directory");
        goto error_swpld3;
    }
#endif    
#if 1 
    ret = sysfs_create_group(kobj_swpld1, &swpld1_device_attr_group);
    if (ret) {
        printk(KERN_ERR "Fail to create SWPLD1 attribute group");
        goto error_add_swpld1;
    }
#endif
#if 1
    ret = sysfs_create_group(kobj_swpld2, &swpld2_device_attr_group);
    if (ret) {
        printk(KERN_ERR "Fail to create SWPLD2 attribute group");
        goto error_add_swpld2;
    }
#endif
#if 1
    ret = sysfs_create_group(kobj_swpld3, &swpld3_device_attr_group);
    if (ret) {
        printk(KERN_ERR "Fail to create SWPLD3 attribute group");
        goto error_add_swpld3;
    }
#endif
    return 0;

error_add_swpld3:
    kobject_put(kobj_swpld2);
error_add_swpld2:
    kobject_put(kobj_swpld1);
error_add_swpld1:
    i2c_unregister_device(pdata[swpld3].client);
error_swpld3:
    i2c_unregister_device(pdata[swpld2].client);
error_swpld2:
    i2c_unregister_device(pdata[swpld1].client);
error_swpld1:
    i2c_put_adapter(parent);
}

static int __exit swpld_remove(struct platform_device *pdev)
{
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;

    if (!kobj_swpld1){
        sysfs_remove_group(kobj_swpld1, &swpld1_device_attr_group);
    }
    if (!kobj_swpld2){
        sysfs_remove_group(kobj_swpld2, &swpld2_device_attr_group);
    }
    if (!kobj_swpld3){
        sysfs_remove_group(kobj_swpld3, &swpld3_device_attr_group);
    }
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
        if (pdata[swpld2].client) {
            if (!parent) {
                parent = (pdata[swpld2].client)->adapter;
            }
            i2c_unregister_device(pdata[swpld2].client);
        }
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

static struct platform_driver swpld_driver = {
    .probe  = swpld_probe,
    .remove = __exit_p(swpld_remove),
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "delta-agc032-swpld",
    },
};

static int __init delta_agc032_swpld_init(void)
{
    int ret;
    int i = 0;

    printk(KERN_INFO "agc032_platform_swpld module initialization\n");

    /* Register SWPLD driver */
    ret = platform_driver_register(&swpld_driver);
    if (ret) {
        printk(KERN_ERR "Fail to register swpld driver\n");
        goto error_swpld1_driver;
    }

    /* Register SWPLD Device */
    ret = platform_device_register(&swpld_device);
    if (ret) {
        printk(KERN_ERR "Fail to create swpld device\n");
        goto error_swpld1_device;
    }

    return 0;

error_swpld1_device:
    platform_driver_unregister(&swpld_driver);
error_swpld1_driver:
    return ret;
}

static void __exit delta_agc032_swpld_exit(void)
{

    platform_device_unregister(&swpld_device);
    platform_driver_unregister(&swpld_driver);
}

module_init(delta_agc032_swpld_init);
module_exit(delta_agc032_swpld_exit);

MODULE_DESCRIPTION("DNI agc032 SWPLD Platform Support");
MODULE_AUTHOR("James Ke <james.ke@deltaww.com>");
MODULE_LICENSE("GPL");
