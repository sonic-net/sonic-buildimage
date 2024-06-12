/*
 * wb_mac_th3.c - A driver for control wb_mac_th3 base on wb_mac.c
 *
 * Copyright (c) 1998, 1999  Frodo Looijaard <frodol@dds.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define MAC_TEMP_INVALID    (99999999)

#define MAC_SIZE		    (256)
#define MAC_TEMP_NUM        (16)

#define MAC_ID_REG          (0x02000000)

typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

typedef enum{
    MAC_TYPE_START,
    TD4_X9 = 0xb780,
    TD4_X9_8 = 0xb788,
    TH3 = 0xb980,
    TD3 = 0xb870,
    TD4 = 0xb880,
    TH4 = 0xb990,
    MAC_TYPE_END,
} mac_type;

typedef struct sensor_regs_s {
    int id;
    u32 reg;
} sensor_reg_t;

typedef struct mac_temp_regs_s {
    int mac_type;
    sensor_reg_t sensor_reg[MAC_TEMP_NUM];
} mac_temp_reg_t;

typedef enum {
    MAC_TEMP_START,
    MAC_TEMP_INDEX1,
    MAC_TEMP_INDEX2,
    MAC_TEMP_INDEX3,
    MAC_TEMP_INDEX4,
    MAC_TEMP_INDEX5,
    MAC_TEMP_INDEX6,
    MAC_TEMP_INDEX7,
    MAC_TEMP_INDEX8,
    MAC_TEMP_INDEX9,
    MAC_TEMP_INDEX10,
    MAC_TEMP_INDEX11,
    MAC_TEMP_INDEX12,
    MAC_TEMP_INDEX13,
    MAC_TEMP_INDEX14,
    MAC_TEMP_INDEX15,
    MAC_TEMP_END,
} mac_hwmon_index;

static mac_temp_reg_t mac_temp_reg[] = {
    {
        /* TD3 */
        .mac_type = TD3,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x02004700},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x02004800},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x02004900},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x02004a00},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x02004b00},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02004c00},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02004d00},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02004e00},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02005200},
            {.id = MAC_TEMP_INDEX10, .reg = 0x02005100},
            {.id = MAC_TEMP_INDEX11, .reg = 0x02005000},
            {.id = MAC_TEMP_INDEX12, .reg = 0x02004f00},
        },
    },
    {
        /* TD4 */
        .mac_type = TD4,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x02004900},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x02004b00},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x02004d00},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x02004f00},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x02005100},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02005300},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02005500},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02005700},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02005900},
            {.id = MAC_TEMP_INDEX10, .reg = 0x02005b00},
            {.id = MAC_TEMP_INDEX11, .reg = 0x02005d00},
            {.id = MAC_TEMP_INDEX12, .reg = 0x02005f00},
            {.id = MAC_TEMP_INDEX13, .reg = 0x02006100},
            {.id = MAC_TEMP_INDEX14, .reg = 0x02006300},
            {.id = MAC_TEMP_INDEX15, .reg = 0x02006500},
        },
    },
    {
        /* TD4_X9 */
        .mac_type = TD4_X9,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x02005a00},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x02005c00},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x02005e00},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x02006000},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x02006200},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02006400},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02006600},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02006800},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02006a00},
        },
    },
    {
        /* TD4_X9_8 */
        .mac_type = TD4_X9_8,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x02005a00},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x02005c00},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x02005e00},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x02006000},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x02006200},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02006400},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02006600},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02006800},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02006a00},
        },
    },
    {
        /* TH3 */
        .mac_type = TH3,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x02004a00},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x02004b00},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x02004c00},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x02004d00},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x02004e00},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02004f00},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02005000},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02005100},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02005200},
            {.id = MAC_TEMP_INDEX10, .reg = 0x02005300},
            {.id = MAC_TEMP_INDEX11, .reg = 0x02005400},
            {.id = MAC_TEMP_INDEX12, .reg = 0x02005500},
            {.id = MAC_TEMP_INDEX13, .reg = 0x02005600},
            {.id = MAC_TEMP_INDEX14, .reg = 0x02005700},
            {.id = MAC_TEMP_INDEX15, .reg = 0x02005800},
        },
    },
    {
        /* TH4 */
        .mac_type = TH4,
        .sensor_reg = {
            {.id = MAC_TEMP_INDEX1,  .reg = 0x0201d800},
            {.id = MAC_TEMP_INDEX2,  .reg = 0x0201e000},
            {.id = MAC_TEMP_INDEX3,  .reg = 0x0201e800},
            {.id = MAC_TEMP_INDEX4,  .reg = 0x0201f000},
            {.id = MAC_TEMP_INDEX5,  .reg = 0x0201f800},
            {.id = MAC_TEMP_INDEX6,  .reg = 0x02020000},
            {.id = MAC_TEMP_INDEX7,  .reg = 0x02020800},
            {.id = MAC_TEMP_INDEX8,  .reg = 0x02021000},
            {.id = MAC_TEMP_INDEX9,  .reg = 0x02021800},
            {.id = MAC_TEMP_INDEX10, .reg = 0x02022000},
            {.id = MAC_TEMP_INDEX11, .reg = 0x02022800},
            {.id = MAC_TEMP_INDEX12, .reg = 0x02023000},
            {.id = MAC_TEMP_INDEX13, .reg = 0x02023800},
            {.id = MAC_TEMP_INDEX14, .reg = 0x02024000},
            {.id = MAC_TEMP_INDEX15, .reg = 0x02024800},
        },
    },
};

static int debuglevel = 0;
module_param(debuglevel, int, S_IRUGO | S_IWUSR);

static int mac_pcie_id = MAC_TYPE_START;
module_param(mac_pcie_id, int, S_IRUGO | S_IWUSR);

#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

#define DBG_ERROR(fmt, arg...)  do { \
     if ( debuglevel > DBG_START) {  \
        printk(KERN_ERR "[ERROR]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)

struct mac_data {
    struct i2c_client   *client;
    struct device		*hwmon_dev;
    struct mutex        update_lock;
    u8                  data[MAC_SIZE]; /* Register value */
};

static int wb_i2c_read_one_time(struct i2c_client *client, u8 *recv_buf, int size)
{
    struct i2c_msg msgs[2];
    int ret = 0;

    if ((client == NULL) || (recv_buf == NULL)) {
        DBG_DEBUG("i2c_client || recv_buf = NULL\r\n");
        return -1;
    }

    mem_clear(msgs, sizeof(msgs));

    msgs[0].buf = recv_buf;
    msgs[0].len = size;
    msgs[0].addr = client->addr;
    msgs[0].flags |= I2C_M_RD;

    ret = i2c_transfer(client->adapter, msgs, 1);
    if (ret < 0) {
        return ret;
    }
    DBG_DEBUG("i2c_transfer, dev_addr 0x%x, size %d.\n", client->addr, size);

    return 0;
}

static int wb_i2c_write_one_time(struct i2c_client *client, u8 *write_buf, int size)
{
    struct i2c_msg msgs[2];
    int ret = 0;

    if ((client == NULL) || (write_buf == NULL)) {
        DBG_DEBUG("i2c_client || write_buf = NULL\r\n");
        return -1;
    }

    if ((size <= 0)) {
        DBG_DEBUG("size invalid, size %d\n", size);
        return -1;
    }

    mem_clear(msgs, sizeof(msgs));

    msgs[0].len = size;
    msgs[0].buf = write_buf;
    msgs[0].addr = client->addr;

    ret = i2c_transfer(client->adapter, msgs, 1);
    if (ret < 0) {
        return ret;
    }
    DBG_DEBUG("i2c_transfer, dev_addr 0x%x, size %d\n", client->addr, size);

    return 0;
}

static u8 step2_buf1[8] = {0x03, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00};
static u8 step2_buf2[8] = {0x03, 0x21, 0x04, 0x0c, 0x2c, 0x38, 0x02, 0x00};
static u8 step2_buf3[8] = {0x03, 0x21, 0x04, 0x10, 0x02, 0x00, 0x4a, 0x00};
static u8 step2_buf4[8] = {0x03, 0x21, 0x04, 0x00, 0x00, 0x00, 0x00, 0x01};
static u8 step2_buf5[4] = {0x03, 0x21, 0x04, 0x08};
static u8 step2_buf6[4] = {0x03, 0x21, 0x04, 0x10};

static int getmac_register(struct i2c_client *client, u32 index, int *reg_value)
{
    int i;
    int ret = 0;
    int value = 0;
    unsigned char read_buf[8];

    if (index == 0) {
        DBG_ERROR("invalid index\n");
        return -1;
    }

    step2_buf3[7]  = index & 0xff;
    step2_buf3[6]  = (index >> 8) & 0xff;
    step2_buf3[5]  = (index >> 16) & 0xff;
    step2_buf3[4]  = (index >> 24) & 0xff;

     ret = wb_i2c_write_one_time(client, step2_buf1, 8);
     if (ret < 0) {
         DBG_ERROR("write step2_buf1 failed, ret = %d\n", ret);
     }
     ret = wb_i2c_write_one_time(client, step2_buf2, 8);
     if (ret < 0) {
         DBG_ERROR("write step2_buf2 failed, ret = %d\n", ret);
     }
     ret = wb_i2c_write_one_time(client, step2_buf3, 8);
     if (ret < 0) {
         DBG_ERROR("write step2_buf3 failed, ret = %d\n", ret);
     }
     ret = wb_i2c_write_one_time(client, step2_buf4, 8);
     if (ret < 0) {
         DBG_ERROR("write step2_buf4 failed, ret = %d\n", ret);
     }

     ret = wb_i2c_write_one_time(client, step2_buf5, 4);
     if (ret < 0) {
         DBG_ERROR("write step2_buf5 failed, ret = %d\n", ret);
     }
     ret = wb_i2c_read_one_time(client, read_buf, 4);
     if (ret < 0) {
         DBG_ERROR("read failed, ret = %d\n", ret);
     }
     for (i = 0; i < 4; i++) {
         DBG_DEBUG("read_buf[%d] = 0x%x \n", i, read_buf[i]);
     }

     ret = wb_i2c_write_one_time(client, step2_buf6, 4);
     if (ret < 0) {
         DBG_ERROR("write step2_buf6 failed, ret = %d\n", ret);
     }

    ret = wb_i2c_read_one_time(client, read_buf, 4);
    if (ret < 0) {
        DBG_ERROR("read failed, ret = %d\n", ret);
        return ret;
    }

    value = (read_buf[0] << 24)| (read_buf[1] << 16) | (read_buf[2] << 8) | read_buf[3];
    *reg_value = value;

    return ret;
}

static int mac_calcute(u32 reg, int *temp)
{
    int ret = 0;
    u32 tmp = 0;

    switch(mac_pcie_id) {
    case TD3:
    case TH3:
        tmp = reg & 0x3ff;
        *temp =  434100 - (tmp * 535);
        break;
    case TD4:
    case TH4:
    case TD4_X9:
    case TD4_X9_8:
        tmp = reg & 0x7ff;
        *temp = (356070 - (tmp * 237));
        break;
    default:
        ret = -1;
        DBG_ERROR("read failed, ret = %d\n", ret);
        break;
    }

    if ((*temp / 1000 < -70) || (*temp / 1000 > 200)) {
        ret = -1;
        DBG_ERROR("mac temp invalid, temp = %d\n", *temp );
    }

    return ret;
}

static int find_reg_type(int type, int *type_index)
{
    int i;
    int size;

    size = ARRAY_SIZE(mac_temp_reg);
    for (i = 0; i < size; i++) {
        if (mac_temp_reg[i].mac_type == type) {
            *type_index = i;
            return 0;
        }
    }

    return -1;
}

static sensor_reg_t * find_reg_offset(int type, int index)
{
    int i;
    int type_index;
    int ret;

    ret = find_reg_type(type, &type_index);
    if (ret < 0) {
        DBG_ERROR("find_reg_type failed, ret = %d\n", ret);
        return NULL;
    }

    for (i = 0; i < MAC_TEMP_NUM; i++) {
        if (mac_temp_reg[type_index].sensor_reg[i].id == index) {
            return &(mac_temp_reg[type_index].sensor_reg[i]);
        }
    }

    return NULL;
}

static int get_mactemp(struct i2c_client *client, int index, int *temp)
{
    int ret;
    int reg_value;

    if (index == 0) {
        DBG_ERROR("invalid index\n");
        return -1;
    }

    ret = getmac_register(client, index, &reg_value);
    if (ret < 0) {
        DBG_ERROR("getmac_register failed, ret = %d\n", ret);
        return ret;
    }
    DBG_DEBUG("reg_value = 0x%x \n", reg_value);

    ret = mac_calcute(reg_value, temp);
    if (ret < 0) {
        DBG_ERROR("mac_calcute failed, ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static ssize_t show_mac_temp(struct device *dev, struct device_attribute *da, char *buf)
{
    struct mac_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    u32 index_value = to_sensor_dev_attr_2(da)->index;
    sensor_reg_t *t;
    int result = 0;
    int temp = -MAC_TEMP_INVALID;

    mutex_lock(&data->update_lock);
    t = find_reg_offset(mac_pcie_id, index_value);
    if (t == NULL) {
        temp = -MAC_TEMP_INVALID;
        DBG_ERROR("find_reg_offset failed, mac_pcie_id = %d, index_value = %d\n", mac_pcie_id, index_value);
    } else {
        result = get_mactemp(client, t->reg, &temp);
        if (result < 0) {
            temp = -MAC_TEMP_INVALID;
            DBG_ERROR("get_mactemp failed, ret = %d\n", result);
        }
    }

    mutex_unlock(&data->update_lock);
    return snprintf(buf, MAC_SIZE, "%d\n", temp);
}

static ssize_t show_mac_max_temp(struct device *dev, struct device_attribute *da, char *buf)
{
    struct mac_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int i;
    int result;
    int temp = -MAC_TEMP_INVALID;
    int type_index;
    int tmp;

    mutex_lock(&data->update_lock);

    result = find_reg_type(mac_pcie_id, &type_index);
    if (result < 0) {
        DBG_ERROR("find_reg_type failed, ret = %d\n", result);
        goto exit;
    }

    for (i = 0; i < MAC_TEMP_NUM; i++) {
        result = get_mactemp(client, mac_temp_reg[type_index].sensor_reg[i].reg, &tmp);
        if (result < 0) {
            DBG_ERROR("get_mactemp failed, ret = %d\n", result);
            tmp = -MAC_TEMP_INVALID;
        }

        temp = (temp > tmp) ? temp : tmp;
    }

exit:
    mutex_unlock(&data->update_lock);
    return snprintf(buf, MAC_SIZE, "%d\n", temp);
}

static int mac_bsc_init(struct i2c_client *client)
{
    int ret;
    int reg_value;
    int mac_id = 0;

    ret = getmac_register(client, MAC_ID_REG, &reg_value);
    if (ret < 0) {
        DBG_ERROR("getmac_register failed, ret = %d\n", ret);
        return ret;
    }

    DBG_DEBUG("reg_value = 0x%x \n", reg_value);
    mac_id = reg_value & 0xffff;
    return mac_id;
}

static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX1);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX2);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX3);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX4);
static SENSOR_DEVICE_ATTR(temp5_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX5);
static SENSOR_DEVICE_ATTR(temp6_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX6);
static SENSOR_DEVICE_ATTR(temp7_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX7);
static SENSOR_DEVICE_ATTR(temp8_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX8);
static SENSOR_DEVICE_ATTR(temp9_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX9);
static SENSOR_DEVICE_ATTR(temp10_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX10);
static SENSOR_DEVICE_ATTR(temp11_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX11);
static SENSOR_DEVICE_ATTR(temp12_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX12);
static SENSOR_DEVICE_ATTR(temp13_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX13);
static SENSOR_DEVICE_ATTR(temp14_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX14);
static SENSOR_DEVICE_ATTR(temp15_input, S_IRUGO, show_mac_temp, NULL, MAC_TEMP_INDEX15);
static SENSOR_DEVICE_ATTR(temp99_input, S_IRUGO, show_mac_max_temp, NULL, 0);

static struct attribute *mac_hwmon_attrs[] = {
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_temp3_input.dev_attr.attr,
    &sensor_dev_attr_temp4_input.dev_attr.attr,
    &sensor_dev_attr_temp5_input.dev_attr.attr,
    &sensor_dev_attr_temp6_input.dev_attr.attr,
    &sensor_dev_attr_temp7_input.dev_attr.attr,
    &sensor_dev_attr_temp8_input.dev_attr.attr,
    &sensor_dev_attr_temp9_input.dev_attr.attr,
    &sensor_dev_attr_temp10_input.dev_attr.attr,
    &sensor_dev_attr_temp11_input.dev_attr.attr,
    &sensor_dev_attr_temp12_input.dev_attr.attr,
    &sensor_dev_attr_temp13_input.dev_attr.attr,
    &sensor_dev_attr_temp14_input.dev_attr.attr,
    &sensor_dev_attr_temp15_input.dev_attr.attr,
    &sensor_dev_attr_temp99_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(mac_hwmon);

static int init_bcs_command(int mac_type) {
    int ret;

    ret = 0;
    switch (mac_type) {
        case TD3:
            step2_buf2[5] = 0x38;
            break;
        case TH3:
        case TH4:
        case TD4:
        case TD4_X9:
        case TD4_X9_8:
            step2_buf2[5] = 0x40;
            break;
        default:
            ret = -1;
            break;
    }
    return ret;
}

static int mac_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct mac_data *data;
    int mac_type;
    int ret;

    mac_type = id->driver_data;
    mac_pcie_id = mac_type;
    if (init_bcs_command(mac_type) < 0) {
        DBG_ERROR("mactype[%x] not support \n", mac_type);
        return -1;
    };

    if (mac_type == TD4) {
        ret = mac_bsc_init(client);
        if (ret < 0) {
            DBG_ERROR("mac_bsc_init failed, ret = %d\n", ret);
            return -1;
        }
        mac_type = ret;
        mac_pcie_id = mac_type;
    }

    DBG_DEBUG("=========mac_probe(%x)===========\n",client->addr);
    DBG_DEBUG("mac_type: %x\n", mac_type);
    data = devm_kzalloc(&client->dev, sizeof(struct mac_data), GFP_KERNEL);
    if (!data) {
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, mac_hwmon_groups);
	if (IS_ERR(data->hwmon_dev)) {
        return PTR_ERR(data->hwmon_dev);
    }

    return 0;
}

static int mac_remove(struct i2c_client *client)
{
    struct mac_data *data = i2c_get_clientdata(client);
    hwmon_device_unregister(data->hwmon_dev);
    return 0;
}

static const struct i2c_device_id mac_id[] = {
    { "wb_mac_bsc_td3", TD3 },
    { "wb_mac_bsc_td4", TD4 },
    { "wb_mac_bsc_th3", TH3 },
    { "wb_mac_bsc_th4", TH4 },
    {}
};
MODULE_DEVICE_TABLE(i2c, mac_id);

static struct i2c_driver wb_mac_bsc_driver = {
    .driver = {
        .name   = "wb_mac_bsc",
    },
    .probe      = mac_probe,
    .remove     = mac_remove,
    .id_table   = mac_id,
};

module_i2c_driver(wb_mac_bsc_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("mac bsc driver");
MODULE_LICENSE("GPL");
