#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c-smbus.h>
#include <linux/string.h>

static int dfd_my_type = 0;
module_param(dfd_my_type, int, S_IRUGO | S_IWUSR);

int g_common_debug_error = 0;
module_param(g_common_debug_error, int, S_IRUGO | S_IWUSR);

int g_common_debug_verbose = 0;
module_param(g_common_debug_verbose, int, S_IRUGO | S_IWUSR);

#define RAGILE_COMMON_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_common_debug_verbose) { \
        printk(KERN_ERR "[RAGILE_COMMON][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RAGILE_COMMON_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_common_debug_error) { \
        printk(KERN_ERR "[RAGILE_COMMON][ERROR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)


int dfd_get_my_card_type(void)
{
    if (dfd_my_type != 0) {
        RAGILE_COMMON_DEBUG_VERBOSE("my_type = 0x%x\r\n", dfd_my_type);
        return dfd_my_type;
    }

    return -1;
}
EXPORT_SYMBOL(dfd_get_my_card_type);

static int __init ragile_common_init(void)
{
    int ret;

    RAGILE_COMMON_DEBUG_VERBOSE("Enter.\n");
    ret = dfd_get_my_card_type();
    if (ret <= 0) {
        RAGILE_COMMON_DEBUG_ERROR("dfd_get_my_card_type failed, ret %d.\n", ret);
        printk(KERN_ERR "Warning: Device type get failed, please check the TLV-EEPROM!\n");
        return -1;
    }

    RAGILE_COMMON_DEBUG_VERBOSE("Leave success type 0x%x.\n", ret);
    return 0;
}

static void __exit ragile_common_exit(void)
{
    RAGILE_COMMON_DEBUG_VERBOSE("Exit.\n");
}

module_init(ragile_common_init);
module_exit(ragile_common_exit);

MODULE_DESCRIPTION("ragile Platform Support");
MODULE_AUTHOR("support <support@ragile.com>");
MODULE_LICENSE("GPL");

