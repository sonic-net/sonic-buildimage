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
#include <linux/version.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,152)
#include <linux/i2c-mux-gpio.h>
#else
#include <linux/platform_data/i2c-mux-gpio.h>
#endif
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/i2c-smbus.h>
#include <linux/string.h>
#include "platform_common.h"
#include "dfd_tlveeprom.h"

#define PLATFORM_I2C_RETRY_TIMES    3

#define DFD_TLVEEPROM_I2C_BUS       (0)
#define DFD_TLVEEPROM_I2C_ADDR      (0x56)
#define DFD_E2PROM_MAX_LEN          (256)
#define DFD_CARDTYPE_EXT_TLVLEN     (4)

#define PLATFORM_CARDTYPE_RETRY_CNT      (10)
#define PLATFORM_CARDTYPE_RETRY_TIMES    (1000)

int debuglevel = 0;
module_param(debuglevel, int, S_IRUGO | S_IWUSR);

static int dfd_my_type = 0;
module_param(dfd_my_type, int, S_IRUGO | S_IWUSR);

int g_common_debug_error = 0;
module_param(g_common_debug_error, int, S_IRUGO | S_IWUSR);

int g_common_debug_verbose = 0;
module_param(g_common_debug_verbose, int, S_IRUGO | S_IWUSR);

uint32_t dfd_my_type_i2c_bus = 0;
module_param(dfd_my_type_i2c_bus, int, S_IRUGO | S_IWUSR);

uint32_t dfd_my_type_i2c_addr = 0;
module_param(dfd_my_type_i2c_addr, int, S_IRUGO | S_IWUSR);

#define RUJIE_COMMON_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_common_debug_verbose) { \
        printk(KERN_ERR "[PLATFORM_COMMON][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RUJIE_COMMON_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_common_debug_error) { \
        printk(KERN_ERR "[PLATFORM_COMMON][ERROR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int32_t dfd_i2c_read(char *dev, uint32_t addr, uint32_t offset_addr, unsigned char *
buf, int32_t size)
{
    struct file *fp;
    struct i2c_client client;
    int i ,j;
    int rv;
    s32 val_t;

    val_t = -1;
    rv = 0;
    fp = filp_open(dev, O_RDWR, S_IRUSR | S_IWUSR);
    if (IS_ERR(fp)) {
        DBG_ERROR("i2c open fail.\n");
        RUJIE_COMMON_DEBUG_ERROR("i2c open fail.\n");
        return -1;
    }
    memcpy(&client, fp->private_data, sizeof(struct i2c_client));
    client.addr = addr;
    for (j = 0 ;j < size ;j++){
        for (i = 0; i < PLATFORM_I2C_RETRY_TIMES; i++) {
          if ((val_t = i2c_smbus_read_byte_data(&client, (offset_addr + j)))< 0) {
              DBG_DEBUG("read try(%d)time  offset_addr:%x \n", i, offset_addr);
              continue;
          }  else {
              * (buf + j) = val_t;
              break;
          }
        }
        if (val_t < 0) {
            rv = -1;
            break;
        }
    }
    filp_close(fp, NULL);
    return rv;
}

static int dfd_tlvinfo_get_cardtype(void)
{
    char i2c_path[16] = {0};
    int ret;
    int cardtype;
    u_int8_t eeprom[DFD_E2PROM_MAX_LEN];
    dfd_i2c_dev_t i2c_dev;
    uint8_t buf[DFD_CARDTYPE_EXT_TLVLEN];
    uint8_t len;
    dfd_tlv_type_t tlv_type;

    if (dfd_my_type_i2c_bus != 0) {
        i2c_dev.bus = dfd_my_type_i2c_bus;
    } else {
        i2c_dev.bus = DFD_TLVEEPROM_I2C_BUS;
    }

    if (dfd_my_type_i2c_addr != 0) {
        i2c_dev.addr = dfd_my_type_i2c_addr;
    } else {
        i2c_dev.addr = DFD_TLVEEPROM_I2C_ADDR;
    }
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_dev.bus);
    RUJIE_COMMON_DEBUG_VERBOSE("Read device eeprom info:(dev:%s, addr:%02x).\n", i2c_path, i2c_dev.addr);

    ret = dfd_i2c_read(i2c_path, i2c_dev.addr, 0, eeprom, DFD_E2PROM_MAX_LEN);
    if (ret != 0) {
        DBG_ERROR("Read eeprom info error(dev: %s, addr: %02x).\n", i2c_path, i2c_dev.addr);
        RUJIE_COMMON_DEBUG_ERROR("Read eeprom info error(dev: %s, addr: %02x).\n", i2c_path, i2c_dev.addr);
        return ret;
    }

    tlv_type.main_type = TLV_CODE_VENDOR_EXT;
    tlv_type.ext_type = DFD_TLVINFO_EXT_TLV_TYPE_DEV_TYPE;
    len = sizeof(buf);
    mem_clear(buf, len);
    ret = dfd_tlvinfo_get_e2prom_info(eeprom, DFD_E2PROM_MAX_LEN, &tlv_type, buf, &len);
    if (ret) {
        DBG_ERROR("dfd_tlvinfo_get_e2prom_info failed ret %d.\n", ret);
        return -1;
    }
    for (ret = 0; ret < 4; ret++) {
        DBG_DEBUG("buf 0x%02x.\n", buf[ret]);
    }

    cardtype = ntohl(*((uint32_t *)buf));
    DBG_DEBUG("cardtype 0x%x.\n", cardtype);
    return cardtype;
}

static int __dfd_get_my_card_type(void)
{
    return dfd_tlvinfo_get_cardtype();
}

int dfd_get_my_card_type(void)
{
    int type;
    int cnt;

    if (dfd_my_type != 0) {
        DBG_DEBUG("my_type = 0x%x\r\n", dfd_my_type);
        return dfd_my_type;
    }

    cnt = PLATFORM_CARDTYPE_RETRY_CNT;
    while (cnt--) {
        type = __dfd_get_my_card_type();
        if (type < 0) {
            RUJIE_COMMON_DEBUG_ERROR("__dfd_get_my_card_type fail cnt %d, ret %d.\n", cnt, type);
            msleep(PLATFORM_CARDTYPE_RETRY_TIMES);
            continue;
        }
        RUJIE_COMMON_DEBUG_VERBOSE("success to get type 0x%x.\n", type);
        break;
    }

    dfd_my_type = type;
    return dfd_my_type;
}
EXPORT_SYMBOL(dfd_get_my_card_type);

static int __init platform_common_init(void)
{
    int ret;

    RUJIE_COMMON_DEBUG_VERBOSE("Enter.\n");
    ret = dfd_get_my_card_type();
    if (ret <= 0) {
        RUJIE_COMMON_DEBUG_ERROR("dfd_get_my_card_type failed, ret %d.\n", ret);
        printk(KERN_ERR "Warning: Device type get failed, please check the TLV-EEPROM!\n");
        return -1;
    }

    RUJIE_COMMON_DEBUG_VERBOSE("Leave success type 0x%x.\n", ret);
    return 0;
}

static void __exit platform_common_exit(void)
{
    RUJIE_COMMON_DEBUG_VERBOSE("Exit.\n");
}

module_init(platform_common_init);
module_exit(platform_common_exit);

MODULE_DESCRIPTION("Platform Support");
MODULE_AUTHOR("support");
MODULE_LICENSE("GPL");
