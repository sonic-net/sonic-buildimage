#include <linux/kernel.h> /* Wd're doing kernel work */  
#include <linux/module.h> /* specifically, a module */  
#include <linux/types.h>
#include <linux/init.h>   /* Need for the macros */
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/mfd/core.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "ragile.h"

int lpc_cpld_verbose = 0;
int lpc_cpld_error = 0;
module_param(lpc_cpld_verbose, int, S_IRUGO | S_IWUSR);
module_param(lpc_cpld_error, int, S_IRUGO | S_IWUSR);


#define LPC_CPLD_VERBOSE(fmt, args...) do {                                        \
    if (lpc_cpld_verbose) { \
        printk(KERN_ERR "[LPC_CPLD_I2C_DEVICE][VERBOSE][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LPC_CPLD_ERROR(fmt, args...) do {                                        \
        if (lpc_cpld_error) { \
            printk(KERN_ERR "[LPC_CPLD_I2C_DEVICE][ERROR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
        } \
    } while (0)

#define PCI_VENDOR_ID_D1527_LPC             (0x8c54)
#define PCI_VENDOR_ID_C3000_LPC             (0x19dc)

#define MAX_CPLD_REG_SIZE               (0x100)
#define LPC_GET_CPLD_ID(addr)           ((addr >> 16) & 0xff)
#define LPC_GET_CPLD_OFFSET(addr)       ((addr) & 0xff)
typedef struct rg_lpc_device_s {
    u16 base;
    u16 size;
    u8  type;
    u8  id;
} rg_lpc_device_t;

typedef enum rg_lpc_dev_type_s {
    LPC_DEVICE_CPLD         = 1,
    LPC_DEVICE_FPGA         = 2,
} rg_lpc_dev_type_t;

static rg_lpc_device_t g_rg_lpc_dev[] = {
    {.base = 0x700, .size = MAX_CPLD_REG_SIZE, .type = LPC_DEVICE_CPLD, .id = 0},
    {.base = 0x900, .size = MAX_CPLD_REG_SIZE, .type = LPC_DEVICE_CPLD, .id = 1},
    {.base = 0xb00, .size = MAX_CPLD_REG_SIZE, .type = LPC_DEVICE_CPLD, .id = 2},
    /*{.base = 0x900, .size = MAX_FPGA_REG_SIZE, .type = LPC_DEVICE_FPGA, .id = 0},*/
};

static rg_lpc_device_t* lpc_get_device_info(int type, int id)
{
    int i;

    for (i = 0; i < ARRAY_SIZE(g_rg_lpc_dev); i++) {
        if ((g_rg_lpc_dev[i].type == type) && (g_rg_lpc_dev[i].id == id)) {
            return &g_rg_lpc_dev[i];
        }
    }

    return NULL;
}

static int lpc_cpld_read(int address, u8 *val)
{
    int cpld_id;
    rg_lpc_device_t *info;

    LPC_CPLD_ERROR("Enter\n");
    cpld_id = LPC_GET_CPLD_ID(address);
    LPC_CPLD_ERROR("icpld_id=%d\n", cpld_id);
    info = lpc_get_device_info(LPC_DEVICE_CPLD, cpld_id);
    if (info == NULL) {
        LPC_CPLD_ERROR("lpc_get_device_info addr 0x%x id %d failed.\r\n", address, cpld_id);
        return -1;
    }
    
    *val = inb(info->base + LPC_GET_CPLD_OFFSET(address));
    LPC_CPLD_VERBOSE("Leave info->base 0x%x, addr 0x%x, cpld_id %d, val 0x%x.\r\n", info->base, address, cpld_id, *val);
    return 0;
}

static int lpc_cpld_write(int address, u8 reg_val)
{
    int cpld_id;
    rg_lpc_device_t *info;

    cpld_id = LPC_GET_CPLD_ID(address);
    info = lpc_get_device_info(LPC_DEVICE_CPLD, cpld_id);
    if (info == NULL) {
        LPC_CPLD_ERROR("lpc_get_device_info addr 0x%x id %d failed.\r\n", address, cpld_id);
        return -1;
    }
    
    outb(reg_val, info->base + LPC_GET_CPLD_OFFSET(address));
    LPC_CPLD_VERBOSE("Leave info->base 0x%x, addr 0x%x, cpld_id %d, val 0x%x.\r\n", info->base, address, cpld_id, reg_val);
    return 0;
}

static ssize_t show_cpld_version(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, i;
    u8 data[4];
    u32 index = to_sensor_dev_attr(da)->index;

    memset(data, 0 ,sizeof(data));
    for (i = 0; i < 4; i++) {
        ret = lpc_cpld_read(index + i, &data[i]);
        if (ret != 0) {
            memset(data, 0 ,sizeof(data));
            LPC_CPLD_ERROR("get cpld version failed!\n");
            break;
        }
    } 

    return snprintf(buf, COMMON_STR_LEN, "%02x %02x %02x %02x \n", data[0], data[1], data[2], data[3]);
    
}

static ssize_t show_cpld_sysfs_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 data;
    int ret;

    ret = lpc_cpld_read(attr->index, &data);
    if (ret != 0) {
        LPC_CPLD_ERROR("get cpld[0x%x] value failed!\n", attr->index);
        data = 0;
    }
    return snprintf(buf, COMMON_STR_LEN, "%02x\n", data);
}

static ssize_t set_cpld_sysfs_value(struct device *dev, struct device_attribute *da, const char *buf, size_t 
count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    u8 data;
    unsigned long val;
    int err;
    
    err = kstrtoul(buf, 16, &val);
    if (err)
        return err;
    if ((val < 0) || (val > 0xff)) {
        LPC_CPLD_ERROR("please enter 0x00 ~ 0xff\n");
        return -1;
    }

    data = (u8)val;
    LPC_CPLD_VERBOSE("pos: 0x%02x count = %ld, data = 0x%02x\n", attr->index, count, data);
    err = lpc_cpld_write(attr->index, data);
    if (err != 0) {
        LPC_CPLD_ERROR("set cpld[0x%x] value[0x%x] failed!\n", attr->index, data);
        count = 0;
    }

    return count;
}

/* connect board cpld 0x900 id=1 */
static SENSOR_DEVICE_ATTR(connect_cpld_version, S_IRUGO, show_cpld_version, NULL, 0x10000);
static SENSOR_DEVICE_ATTR(broad_front_sys, S_IRUGO | S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x10072);
static SENSOR_DEVICE_ATTR(psu_status, S_IRUGO, show_cpld_sysfs_value, NULL, 0x10051);
static SENSOR_DEVICE_ATTR(broad_front_pwr, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x10073);
static SENSOR_DEVICE_ATTR(broad_front_fan, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x10074);
static SENSOR_DEVICE_ATTR(sfp_enable, S_IRUGO| S_IWUSR, show_cpld_sysfs_value, set_cpld_sysfs_value, 0x10094);

static struct attribute *lpc_cpld_base_sysfs_attrs[] = {
    &sensor_dev_attr_connect_cpld_version.dev_attr.attr,
    &sensor_dev_attr_broad_front_sys.dev_attr.attr,
    &sensor_dev_attr_psu_status.dev_attr.attr,
    &sensor_dev_attr_broad_front_pwr.dev_attr.attr,
    &sensor_dev_attr_broad_front_fan.dev_attr.attr,
    &sensor_dev_attr_sfp_enable.dev_attr.attr,
    NULL
};

static const struct attribute_group lpc_cpld_base_sysfs_group = {
    .attrs = lpc_cpld_base_sysfs_attrs,
};

static int __init rg_lpc_cpld_init(void)
{
    struct pci_dev *pdev = NULL;
    int status;

    pdev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_VENDOR_ID_D1527_LPC, pdev);
    if (!pdev) {
        LPC_CPLD_ERROR("pci_get_device(0x8086, 0x8c54) failed!\n");
        return -1;
    }

    status = -1;  
    status = sysfs_create_group(&pdev->dev.kobj, &lpc_cpld_base_sysfs_group);
    if (status) {
        LPC_CPLD_ERROR("sysfs_create_group failed!\n");
        return -1;
    }

    LPC_CPLD_VERBOSE("Leave success\n");
    return 0;
}

static void __exit rg_lpc_cpld_exit(void)
{
    struct pci_dev *pdev = NULL;

    pdev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_VENDOR_ID_D1527_LPC, pdev);
    if (!pdev) {
        LPC_CPLD_ERROR("pci_get_device(0x8086, 0x8c54) failed!\n");
        return ;
    }

    sysfs_remove_group(&pdev->dev.kobj, &lpc_cpld_base_sysfs_group);

    LPC_CPLD_VERBOSE("Leave.\n");
}

module_init(rg_lpc_cpld_init);
module_exit(rg_lpc_cpld_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support <support@ragile.com>");
