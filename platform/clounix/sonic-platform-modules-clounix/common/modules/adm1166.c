#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/jiffies.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/jiffies.h>

#include <linux/hwmon-sysfs.h>

#define MAX_RAM_ADDR (0xff)
#define MAX_EEPROM_LEN (0x400)
#define EEPROM_START_ADDR (0xF800)
#define EEPROM_END_ADDR (0xFBFF)
#define BLOCK_RD (0xFD)
#define BLOCK_WT (0xFC)

#define updcfg (0x90)
#define bbctrl (0x9C)
#define bbsearch (0xd9)
#define bbaddr (0xf1)

#define prevstext (0xea)
#define fault_msg_len (128)
#define fault_msg_offset (0x180)

enum LOG_LEVEL{
    INFO = 0x1,
    ERR  = 0x2,
    DBG  = 0x4,
    ALL  = 0xf
};

#define MAX_ADM1166_NUMS (2)

static int g_adm1166_loglevel = 0x2;
#define ADM1166_INFO(fmt, args...) do {                                        \
    if (g_adm1166_loglevel & INFO) { \
        printk(KERN_INFO "[ADM1166][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define ADM1166_ERR(fmt, args...) do {                                        \
    if (g_adm1166_loglevel & ERR) { \
        printk(KERN_ERR "[ADM1166][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define ADM1166_DBG(fmt, args...) do {                                        \
    if (g_adm1166_loglevel & DBG) { \
        printk(KERN_DEBUG "[ADM1166][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

module_param(g_adm1166_loglevel, int, 0644);
MODULE_PARM_DESC(g_adm1166_loglevel, "The log level(info=0x1, err=0x2, dbg=0x4).\n");

ssize_t chip_id_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char data;
    unsigned char addr;

    addr = 0xf4;
    i2c_master_send(client, &addr, 1);
    i2c_master_recv(client, &data, 1);

    return sprintf(buf, "%x\r\n", data);
}

ssize_t adm1166_eeprom_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char addr[2];
    unsigned short i;

    addr[0] = updcfg;
    addr[1] = 1;
    i2c_master_send(client, addr, 2);

    addr[0] = bbctrl;
    addr[1] = 1;
    i2c_master_send(client, addr, 2);

    for (i=0; i<(MAX_EEPROM_LEN/I2C_SMBUS_BLOCK_MAX); i++) {
        addr[0] = (i*I2C_SMBUS_BLOCK_MAX + EEPROM_START_ADDR) >> 8;
        addr[1] = (i*I2C_SMBUS_BLOCK_MAX + EEPROM_START_ADDR) & 0xff;
        i2c_master_send(client, addr, 2);
        i2c_smbus_read_block_data(client, 0xfd, buf);
        buf += I2C_SMBUS_BLOCK_MAX;
    }

    return MAX_EEPROM_LEN;
}

ssize_t adm1166_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char data;
    unsigned char addr;

    for (addr=0; addr<MAX_RAM_ADDR; addr++) {
        i2c_master_send(client, &addr, 1);
        i2c_master_recv(client, &data, 1);
        *buf = data;
        buf++;
    }

    return MAX_RAM_ADDR;
}

ssize_t adm1166_fault_log_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char data;
    unsigned char addr;
    unsigned char ret = 0;

    for (addr=prevstext; addr<bbaddr; addr++) {
        i2c_master_send(client, &addr, 1);
        i2c_master_recv(client, &data, 1);
        ret += sprintf(buf, "%02x ", data);
        buf += 3;
    }

    ret += sprintf(buf, "\n");

    return ret;
}

ssize_t adm1166_fault_log_addr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    unsigned char data;
    unsigned char addr;

    addr = bbaddr;
    i2c_master_send(client, &addr, 1);
    i2c_master_recv(client, &data, 1);

    return sprintf(buf, "%02x\n", data);
}

#define ref_voltage_mV (2048)
#define adc_bits (12)
#define rrsel_1 (0x80)
#define rrsel_2 (0x81)
#define rrctl (0x82)

static unsigned char addr_list[] = {0xa0, 0xa2, 0xa4, 0xa6, 0xa8,
                                    0xaa, 0xac, 0xae, 0xb0, 0xb2,
                                    0xb4, 0xb6};
static unsigned int divider_factor_cfg[MAX_ADM1166_NUMS][12] = {0};

static void process_elec_data(unsigned long long *data, unsigned char index, unsigned short addr)
{
    int chip_index = (addr == 0x34) ? 0 : 1;

    /* frist translate the voltage unit to uV */
    *data = ((*data * ref_voltage_mV)/((1 << adc_bits) - 1));
    *data *= divider_factor_cfg[chip_index][index];
    *data /= 1000;
    /* cacl over return value's unit is mV */
    return;
}

ssize_t factor_cfg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int chip_index = (client->addr == 0x34) ? 0 : 1;
    int i;
    char *p = buf;

    for (i = 0; i < 12; i++) {
        p += sprintf(p, "%d ", divider_factor_cfg[chip_index][i]);
    }
    p += sprintf(p, "\n");
    return (p - buf);
}

ssize_t electrical_info_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sensor_attr = container_of(attr, struct sensor_device_attribute, dev_attr);
    int chip_index = (client->addr == 0x34) ? 0 : 1;
    unsigned long data;

    if (kstrtoul(buf, 0, &data))
        return -EINVAL; 

    divider_factor_cfg[chip_index][sensor_attr->index] = data;
    return count;
}

static ssize_t electrical_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct sensor_device_attribute *sensor_attr = container_of(attr, struct sensor_device_attribute, dev_attr);
    unsigned long long data;
    unsigned char reg_set[2];
    unsigned char i2c_data;
    unsigned char i2c_addr;
    unsigned long t_out;

    reg_set[1] = 0;
    reg_set[0] = rrsel_1;
    i2c_master_send(client, reg_set, 2);
    reg_set[0] = rrsel_2;
    i2c_master_send(client, reg_set, 2);

    reg_set[0] = rrctl;
    reg_set[1] = 0x1;
    i2c_master_send(client, reg_set, 2);
   
    t_out = jiffies + msecs_to_jiffies(500);
    while (1) {
        i2c_addr = rrctl;
        i2c_master_send(client, &i2c_addr, 1);
        i2c_master_recv(client, &i2c_data, 1);
        if (i2c_data == 0) {
            break;
        } else {
            if (time_after(jiffies, t_out))
                return 0;
        }
    }
    reg_set[0] = rrctl;
    reg_set[1] = 0x8;
    i2c_master_send(client, reg_set, 2);

    i2c_addr = addr_list[sensor_attr->index];

    i2c_master_send(client, &i2c_addr, 1);
    i2c_master_recv(client, &i2c_data, 1);
    data = (i2c_data & 0xf) * 256;
    
    i2c_addr++;
    i2c_master_send(client, &i2c_addr, 1);
    i2c_master_recv(client, &i2c_data, 1);
    data += i2c_data;

    reg_set[0] = rrctl;
    reg_set[1] = 0x0;
    i2c_master_send(client, reg_set, 2);
    
    process_elec_data(&data, sensor_attr->index, client->addr);

    return sprintf(buf, "%llu\n", data);
}


SENSOR_DEVICE_ATTR_RW(vp1, electrical_info, 0);
SENSOR_DEVICE_ATTR_RW(vp2, electrical_info, 1);
SENSOR_DEVICE_ATTR_RW(vp3, electrical_info, 2);
SENSOR_DEVICE_ATTR_RW(vp4, electrical_info, 3);
SENSOR_DEVICE_ATTR_RW(vh, electrical_info, 4);
SENSOR_DEVICE_ATTR_RW(vx1, electrical_info, 5);
SENSOR_DEVICE_ATTR_RW(vx2, electrical_info, 6);
SENSOR_DEVICE_ATTR_RW(vx3, electrical_info, 7);
SENSOR_DEVICE_ATTR_RW(vx4, electrical_info, 8);
SENSOR_DEVICE_ATTR_RW(vx5, electrical_info, 9);
SENSOR_DEVICE_ATTR_RW(aux1, electrical_info, 10);
SENSOR_DEVICE_ATTR_RW(aux2, electrical_info, 11);

DEVICE_ATTR_RO(chip_id);
DEVICE_ATTR_RO(factor_cfg);
DEVICE_ATTR_RO(adm1166_dump);
DEVICE_ATTR_RO(adm1166_eeprom_dump);
DEVICE_ATTR_RO(adm1166_fault_log_addr);
DEVICE_ATTR_RO(adm1166_fault_log_info);

static struct attribute *adm1166_attrs[] = {
    &sensor_dev_attr_vp1.dev_attr.attr,
    &sensor_dev_attr_vp2.dev_attr.attr,
    &sensor_dev_attr_vp3.dev_attr.attr,
    &sensor_dev_attr_vp4.dev_attr.attr,
    &sensor_dev_attr_vh.dev_attr.attr,
    &sensor_dev_attr_vx1.dev_attr.attr,
    &sensor_dev_attr_vx2.dev_attr.attr,
    &sensor_dev_attr_vx3.dev_attr.attr,
    &sensor_dev_attr_vx4.dev_attr.attr,
    &sensor_dev_attr_vx5.dev_attr.attr,
    &sensor_dev_attr_aux1.dev_attr.attr,
    &sensor_dev_attr_aux2.dev_attr.attr,

    &dev_attr_adm1166_fault_log_info.attr,
    &dev_attr_adm1166_fault_log_addr.attr,
    &dev_attr_adm1166_eeprom_dump.attr,
    &dev_attr_adm1166_dump.attr,
    &dev_attr_factor_cfg.attr,
    &dev_attr_chip_id.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = adm1166_attrs,
};

static void fault_log_space_check(struct i2c_client *client)
{
    unsigned char addr[2];
    unsigned char data;
    unsigned int i;
    unsigned long time;

    addr[0] = bbaddr;
    i2c_master_send(client, addr, 1);
    i2c_master_recv(client, &data, 1);

    if (data == 0) {
        ADM1166_INFO("%s %x fault log full!\n", client->name, client->addr);
    } else {
        ADM1166_INFO("%s %x fault log at 0xf980 + %x\n", client->name, client->addr, data);
        return;
    }

    /* disbale eeprom black box*/
    addr[0] = bbctrl;
    addr[1] = 1;
    i2c_master_send(client, addr, 2);

    /* erase enable */
    addr[0] = updcfg;
    addr[1] = 5;
    i2c_master_send(client, addr, 2);

    for (i = 0; i < fault_msg_len/I2C_SMBUS_BLOCK_MAX; i++) {
        addr[0] = (EEPROM_START_ADDR + fault_msg_offset + i*I2C_SMBUS_BLOCK_MAX) >> 8;
        addr[1] = (EEPROM_START_ADDR + fault_msg_offset + i*I2C_SMBUS_BLOCK_MAX) & 0xff;
        i2c_master_send(client, addr, 2);
        data = 0xfe;
        i2c_master_send(client, &data, 1);
        mdelay(200);
    }

    /* erase disable */
    addr[0] = updcfg;
    addr[1] = 1;
    i2c_master_send(client, addr, 2);

    addr[0] = bbsearch;
    addr[1] = 1;
    i2c_master_send(client, addr, 2);

    time = jiffies + msecs_to_jiffies(2000);
    while (1) {
        addr[0] = bbaddr;
        i2c_master_send(client, addr, 1);
        i2c_master_recv(client, &data, 1);

        if (data == 0x80) {
            ADM1166_INFO("%s %x fault log clear over\n", client->name, client->addr);
            break;
        }

        if (time_after(jiffies, time)) {
            ADM1166_INFO("%s %x fault log clear fail\n", client->name, client->addr);
            break;
        }
    }

    /* enable black box */
    addr[0] = bbctrl;
    addr[1] = 0;
    i2c_master_send(client, addr, 2);

    return;
}

static int adm1166_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    ADM1166_INFO("i2c slave:%s addr:%x start probe.", client->name, client->addr);

    fault_log_space_check(client);

    return sysfs_create_group(&(client->dev.kobj), &attr_group);
}

static int adm1166_remove(struct i2c_client *client)
{
    sysfs_remove_group(&(client->dev.kobj), &attr_group);
    return 0;
}

static struct i2c_device_id adm1166_id[] = {
    {"adm1166", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, adm1166_id);

static struct i2c_driver adm1166_driver = {
    .driver = {
        .name = "adm1166",
    },
    .probe = adm1166_probe,
    .remove = adm1166_remove,
    .id_table = adm1166_id,
};

module_i2c_driver(adm1166_driver);

MODULE_AUTHOR("Bao Hengxi baohx@clounix.com");
MODULE_DESCRIPTION("driver for adm1166");
MODULE_LICENSE("GPL v2");
