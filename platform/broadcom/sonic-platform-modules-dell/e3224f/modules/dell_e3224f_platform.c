/* Copyright (c) 2020 Dell Inc.
 * dell_e3224f_platform.c - Driver for E3224F switches
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
 */
    #include <linux/module.h>
    #include <linux/kernel.h>
    #include <linux/sysfs.h>
    #include <linux/slab.h>
    #include <linux/stat.h>
    #include <linux/i2c.h>
    #include <linux/i2c-mux.h>
    #include <linux/platform_device.h>
    #include <linux/i2c/sff-8436.h>
    #include <linux/delay.h>
    #include <linux/hwmon-sysfs.h>

    #define PSU_MODULE_BASE_NR      10
    #define FANTRAY_MODULE_BASE_NR  15
    #define SFP_MODULE_BASE_NR      20
    #define SFP_MUX_BASE_NR         8
    #define FANTRAY_MUX_BASE_NR     4
    #define PSU_MUX_BASE_NR         5

    #define PHY_RESET_REG           0x40
    #define RESET_ALL_PHY           0x7F
    #define SYS_CTRL_REG            0x15
    #define POWER_CYCLE_SYS         0x1
    #define CPLD_DEVICE_NUM         3
    #define PF_MUX_DEVICES          3
    #define SYS_MISC_CTRL_REG       0x0B

    #define FAN_0                   0
    #define FAN_1                   1
    #define FAN_2                   2

    static int get_i2c_adapter_name(void);

    static void device_release(struct device *dev)
    {
        return;
    }

    /*
     * E3224F CPLD
     */

    enum cpld_type {
        cpu_cpld,
        sys_cpld,
        port_cpld,
    };

    struct cpld_platform_data {
        int reg_addr;
        struct i2c_client *client;
    };

    static struct cpld_platform_data e3224f_cpld_platform_data[] = {
        [cpu_cpld] = {
            .reg_addr = 0x31,
        },

        [sys_cpld] = {
            .reg_addr = 0x32,
        },

        [port_cpld] = {
            .reg_addr = 0x34,
        },
    };

    static struct platform_device e3224f_cpld = {
        .name               = "dell-e3224f-cpld",
        .id                 = 0,
        .dev                = {
                    .platform_data   = e3224f_cpld_platform_data,
                    .release         = device_release
        },
    };

    /*
     * E3224F MUX
     */

    struct mux_platform_data {
        int parent;
        int base_nr;
        int reg_addr;
        struct i2c_client *cpld;
        int no_of_buses;
        int mux_offset;
    };

    struct pf_mux {
        struct mux_platform_data data;
    };

    static struct mux_platform_data e3224f_mux_platform_data[] = {
        {
            .parent         = SFP_MUX_BASE_NR,
            .base_nr        = SFP_MODULE_BASE_NR,
            .cpld           = NULL,
            .reg_addr       = 0x11,
            .no_of_buses    = 31,
            .mux_offset     = 1,
        },
        {
            .parent         = FANTRAY_MUX_BASE_NR,
            .base_nr        = FANTRAY_MODULE_BASE_NR,
            .cpld           = NULL,
            .reg_addr       = 0x13,
            .no_of_buses    = 3,
            .mux_offset     = 1,
        },
        {
            .parent         = PSU_MUX_BASE_NR,
            .base_nr        = PSU_MODULE_BASE_NR,
            .cpld           = NULL,
            .reg_addr       = 0x12,
            .no_of_buses    = 2,
            .mux_offset     = 1,
        },
    };

    static struct platform_device e3224f_mux[] = {
        {
            .name           = "dell-e3224f-mux",
            .id             = 0,
            .dev            = {
                    .platform_data   = &e3224f_mux_platform_data[0],
                    .release         = device_release,
            },
        },
        {
            .name           = "dell-e3224f-mux",
            .id             = 1,
            .dev            = {
                    .platform_data   = &e3224f_mux_platform_data[1],
                    .release         = device_release,
            },
        },
        {
            .name           = "dell-e3224f-mux",
            .id             = 2,
            .dev            = {
                    .platform_data   = &e3224f_mux_platform_data[2],
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

    static int mux_select(struct i2c_mux_core *muxc, u32 chan)
    {
        struct pf_mux *mux = i2c_mux_priv(muxc);
        u8 chan_data = chan + mux->data.mux_offset;

        return cpld_reg_write_byte(mux->data.cpld, mux->data.reg_addr, chan_data);
    }

    static int __init mux_probe(struct platform_device *pdev)
    {
        struct i2c_mux_core *muxc;
        struct pf_mux *mux;
        struct mux_platform_data *pdata;
        struct i2c_adapter *parent;
        int i, ret;

        pdata = pdev->dev.platform_data;
        if (!pdata) {
            dev_err(&pdev->dev, "Missing platform data\n");
            return -ENODEV;
        }

        mux = devm_kzalloc(&pdev->dev, sizeof(*mux), GFP_KERNEL);
        if (!mux) {
            return -ENOMEM;
        }

        mux->data = *pdata;

        parent = i2c_get_adapter(pdata->parent);
        if (!parent) {
            dev_err(&pdev->dev, "Parent adapter (%d) not found\n",
                pdata->parent);
            return -EPROBE_DEFER;
        }

        muxc = i2c_mux_alloc(parent, &pdev->dev, pdata->no_of_buses, 0, 0,
                             mux_select, NULL);
        if (!muxc) {
            ret = -ENOMEM;
            goto alloc_failed;
        }
        muxc->priv = mux;

        platform_set_drvdata(pdev, muxc);

        for (i = 0; i < pdata->no_of_buses; i++) {
            int nr = pdata->base_nr + i;
            unsigned int class = 0;

            ret = i2c_mux_add_adapter(muxc, nr, i, class);
            if (ret) {
                dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
                goto add_adapter_failed;
            }
        }

        return 0;

    add_adapter_failed:
        i2c_mux_del_adapters(muxc);
    alloc_failed:
        i2c_put_adapter(parent);

        return ret;
    }

    static int mux_remove(struct platform_device *pdev)
    {
        struct i2c_mux_core *muxc = platform_get_drvdata(pdev);

        i2c_mux_del_adapters(muxc);

        i2c_put_adapter(muxc->parent);

        return 0;
    }

    static struct platform_driver mux_driver = {
        .probe  = mux_probe,
        .remove = mux_remove,
        .driver = {
            .owner  = THIS_MODULE,
            .name   = "dell-e3224f-mux",
        },
    };

    static ssize_t sfp_modprs_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        int i;
        s32 ret = 0;
	uint32_t data=0;
        struct cpld_platform_data *pdata = dev->platform_data;

	for (i=0;i<=2;i++) {
            ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x10+i);
            if (ret < 0)
                return sprintf(buf, "read error");

	    data = data + ret << (8*i);
            //printk(KERN_WARNING "sfp_modprs_show %d %x %x\n",i,ret,data);
	}

        return sprintf(buf, "0x%x\n", data);
    }

    static ssize_t sfp_txdis_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        int i;
        s32 ret = 0;
	uint32_t data=0;
        struct cpld_platform_data *pdata = dev->platform_data;

	for (i=0;i<=2;i++) {
            ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x14+i);
            if (ret < 0)
                return sprintf(buf, "read error");

	    data = data + (ret << (8*i));
            //printk(KERN_WARNING "sfp_txdis_show %d %x %x\n",i,ret,data);
	}
        return sprintf(buf, "0x%x\n", (uint32_t)data);
    }

    static ssize_t sfp_txdis_store (struct device *dev, struct device_attribute *devattr, const char *buf, size_t size)
    {
        int i;
        s32 ret = 0;
	long value=0;
	u8 data;
        struct cpld_platform_data *pdata = dev->platform_data;
        ssize_t status;

        status = kstrtol(buf, 0, &value);
        printk(KERN_WARNING "sfp_txdis_store %x\n",value);
        if (status == 0) {

	    for (i=0;i<=2;i++) {
                ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x14+i);
                if (ret < 0)
                    return sprintf(buf, "read error");

		data = ((value >> (8*i)) & 0xFF);
                printk(KERN_WARNING "  txdis [%d] read[%x] data[%x]\n",i,ret,data);

		if (ret != data) {
                    printk(KERN_WARNING "  txdis write [%d] \n",i);
                    status = i2c_smbus_write_byte_data(pdata[port_cpld].client, 0x14+i, data);
                    if (status < 0)
                        printk(KERN_WARNING "  txdis failed to set [%d] data[%x]\n",i,data);
	        }
	    }
            status = size;
	}
        return status;
    }

    static ssize_t sfp_rxlos_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        int i;
        s32 ret = 0;
	uint32_t data=0;
        struct cpld_platform_data *pdata = dev->platform_data;

	for (i=0;i<=2;i++) {
            ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x18+i);
            if (ret < 0)
                return sprintf(buf, "read error");

	    data = data + (ret << (8*i));
            printk(KERN_WARNING "sfp_rxlos_show %d %x %x\n",i,ret,data);
	}
        return sprintf(buf, "0x%x\n", (uint32_t)data);
    }

    static ssize_t sfp_txfault_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        int i;
        s32 ret = 0;
	uint32_t data=0;
        struct cpld_platform_data *pdata = dev->platform_data;

	for (i=0;i<=2;i++) {
            ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x1C+i);
            if (ret < 0)
                return sprintf(buf, "read error");

	    data = data + (ret << (8*i));
            printk(KERN_WARNING "sfp_txfault_show %d %x %x\n",i,ret,data);
	}
        return sprintf(buf, "0x%x\n", (uint32_t)data);
    }

    static ssize_t sfpplus_modprs_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x30);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t sfpplus_txdis_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x31);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t sfpplus_txdis_store (struct device *dev, struct device_attribute *devattr, const char *buf, size_t size)
    {
        long value;
        struct cpld_platform_data *pdata = dev->platform_data;
        s32 ret;
        u8 data;
        ssize_t status;

        status = kstrtol(buf, 0, &value);
        if (status == 0) {
            ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x31);
            if (ret < 0)
                return ret;
            data = (u8)ret & ~(0x0F);
            data = data | (value & 0x0F);
        
            ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x31, data);
            if (ret < 0)
                return ret;

            status = size;
        }

        return status;
    }

    static ssize_t sfpplus_rxlos_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x32);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t sfpplus_txfault_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x33);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t qsfp_modprs_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x20);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t qsfp_rst_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x21);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t qsfp_rst_store (struct device *dev, struct device_attribute *devattr, const char *buf, size_t size)
    {
        long value;
        struct cpld_platform_data *pdata = dev->platform_data;
        s32 ret;
        u8 data;
        ssize_t status;

        status = kstrtol(buf, 0, &value);
        if (status == 0) {
            ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x21);
            if (ret < 0)
                return ret;
            data = (u8)ret & ~(0x0F);
            data = data | (value & 0x0F);
        
            ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x21, data);
            if (ret < 0)
                return ret;

            status = size;
        }

        return status;
    }

    static ssize_t qsfp_lpmode_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x22);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", (u8)ret);
    }

    static ssize_t qsfp_lpmode_store (struct device *dev, struct device_attribute *devattr, const char *buf, size_t size)
    {
        long value;
        struct cpld_platform_data *pdata = dev->platform_data;
        s32 ret;
        u8 data;
        ssize_t status;

        status = kstrtol(buf, 0, &value);
        if (status == 0) {
            ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x22);
            if (ret < 0)
                return ret;
            data = (u8)ret & ~(0x0F);
            data = data | (value & 0x0F);
        
            ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x22, data);
            if (ret < 0)
                return ret;

            status = size;
        }

        return status;
    }

    static ssize_t reboot_cause_show (struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        u8 data;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x10);
        if (ret < 0)
            return sprintf(buf, "read error");

        data = (u8)ret;
        return sprintf(buf, "0x%x\n", data);
    }

    static ssize_t reboot_cause_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
    {
        unsigned long data;
        s32 status, ret;
        struct cpld_platform_data *pdata = dev->platform_data;

        status = kstrtoul(buf, 0, &data);
        ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x10, (u8)(data));
        if (ret < 0)
            return ret;
        return count;
    }

    static ssize_t power_reset_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
    {
        unsigned long data;
        s32 err;
        struct cpld_platform_data *pdata = dev->platform_data;

        err = kstrtoul(buf, 10, &data);
        if (err)
            return err;

        if (data)
        {
            i2c_smbus_write_byte_data(pdata[sys_cpld].client, SYS_CTRL_REG, (u8)(POWER_CYCLE_SYS));
        }

        return count;
    }

    static ssize_t power_reset_show(struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret = 0;
        struct cpld_platform_data *pdata = dev->platform_data;

        ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, SYS_CTRL_REG);
        if (ret < 0)
            return sprintf(buf, "read error");

        return sprintf(buf, "0x%x\n", ret);
    }

    static ssize_t fan_dir_show(struct device *dev, struct device_attribute *devattr, char *buf)
    {
        s32 ret;
        u8 data = 0;
        struct cpld_platform_data *pdata = dev->platform_data;
        struct sensor_device_attribute *sa = to_sensor_dev_attr(devattr);
    int index = sa->index;
    u8 mask = 1 << (index+4);

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xA);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = (u8)((ret & mask) >> (index+4));

    return sprintf(buf, "%s\n", data? "B2F" : "F2B");
}

static ssize_t fan_prs_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;
    struct sensor_device_attribute *sa = to_sensor_dev_attr(devattr);
    int index = sa->index;
    uint8_t mask = 1 << index;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xA);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = (u32)((ret & mask) >> index);

    data = ~data & 0x1;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t psu0_prs_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xC);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x80))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t psu1_prs_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xC);
    if (ret < 0)
        return sprintf(buf, "read error");

    if (!(ret & 0x08))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t psu0_status_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xC);
    if (ret < 0)
        return sprintf(buf, "read error");

    if ((ret & 0x40))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t psu1_status_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0xC);
    if (ret < 0)
        return sprintf(buf, "read error");

    if ((ret & 0x04))
        data = 1;

    return sprintf(buf, "%d\n", data);
}

static ssize_t fani_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;
    struct sensor_device_attribute *sa = to_sensor_dev_attr(devattr);
    int index = sa->index;
    uint8_t mask = 3 << (index*2);

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x9);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & mask) >> (index*2);

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "amber\n");
            break;
        default:
            ret = sprintf(buf, "unknown\n");
    }

    return ret;
}

static ssize_t fani_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 mask, data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;
    struct sensor_device_attribute *sa = to_sensor_dev_attr(devattr);
    int index = sa->index;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "amber", 5))
    {
        data = 2;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else
    {
        return -1;
    }
     
    
    mask = ~((uint8_t)(3 << (index*2)));
    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x9);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x9, (u8)((ret & mask) | (data << (index * 2))));
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t system_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u8)(ret & 0x30) >> 4;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "blink_green\n");
            break;
        case 1:
            ret = sprintf(buf, "green\n");
            break;
        case 2:
            ret = sprintf(buf, "yellow\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t system_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "blink_green", 11))
    {
        data = 0;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 1;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x7, (u8)((ret & 0xCF) | (data << 4)));
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t watchdog_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpu_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "%x\n", data);
}

static ssize_t watchdog_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret, err;
    unsigned long val;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    err = kstrtoul(buf, 10, &val);
    if (err)
        return err;

    data = (u8) val;
    if (data)
    {
        ret = i2c_smbus_write_byte_data(pdata[cpu_cpld].client, 0x7, data);
        if (ret < 0)
            return ret;
    }

    return count;
}

static ssize_t locator_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x08) >> 3;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "blink_blue\n");
            break;
        default:
            ret = sprintf(buf, "invalid\n");
    }

    return ret;
}

static ssize_t locator_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "blink_blue", 10))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x7, (u8)((ret & 0xF7) | (data << 3)));
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t power_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x06) >> 1;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "yellow\n");
            break;
        case 2:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t power_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 1;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x7, (u8)((ret & 0xF9) | (data << 1)));
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t primary_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u32)(ret & 0x1);

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "off\n");
            break;
    }

    return ret;
}

static ssize_t primary_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "green", 5))
    {
        data = 0;
    }
    else if (!strncmp(buf, "off", 3))
    {
        data = 1;
    }
    else
    {
        return -1;
    }

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x7, (u8)((ret & 0xFE) | data));
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t fan_led_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return sprintf(buf, "read error");

    data = (u8)(ret & 0xC0) >> 6;

    switch (data)
    {
        case 0:
            ret = sprintf(buf, "off\n");
            break;
        case 1:
            ret = sprintf(buf, "yellow\n");
            break;
        case 2:
            ret = sprintf(buf, "green\n");
            break;
        default:
            ret = sprintf(buf, "blink_yellow\n");
    }

    return ret;
}

static ssize_t fan_led_store(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    if (!strncmp(buf, "off", 3))
    {
        data = 0;
    }
    else if (!strncmp(buf, "yellow", 6))
    {
        data = 1;
    }
    else if (!strncmp(buf, "green", 5))
    {
        data = 2;
    }
    else if (!strncmp(buf, "blink_yellow", 12))
    {
        data = 3;
    }
    else
    {
        return -1;
    }

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x7);
    if (ret < 0)
        return ret;

    ret = i2c_smbus_write_byte_data(pdata[sys_cpld].client, 0x7, (u8)((ret & 0x3F) | (data << 6)));
    if (ret < 0)
        return ret;

    return count;
}


static ssize_t power_good_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 pwr_good1 = 0;
    u8 pwr_good2 = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpu_cpld].client, 0xc);
    if (ret < 0)
        return sprintf(buf, "read error");
    pwr_good1 = ret;

    ret = i2c_smbus_read_byte_data(pdata[cpu_cpld].client, 0xd);
    if (ret < 0)
        return sprintf(buf, "read error");
    pwr_good2 = ret;

    return sprintf(buf, "0x%x\n", (pwr_good1 == 0xFF &&  (pwr_good2 & 0x1F) == 0x1F));
}

static ssize_t cpu_cpld_mjr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpu_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t cpu_cpld_mnr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[cpu_cpld].client, 0x0);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t sys_cpld_mjr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t sys_cpld_mnr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[sys_cpld].client, 0x0);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t port_cpld_mjr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x1);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;
    printk(KERN_WARNING "port_cpld_mjr_ver_show  %d\n",ret);

    return sprintf(buf, "0x%x\n", data);
}

static ssize_t port_cpld_mnr_ver_show(struct device *dev,
                                   struct device_attribute *devattr, char *buf)
{
    s32 ret;
    u8 data = 0;
    struct cpld_platform_data *pdata = dev->platform_data;

    ret = i2c_smbus_read_byte_data(pdata[port_cpld].client, 0x0);
    if (ret < 0)
        return sprintf(buf, "read error");
    data = ret;

    return sprintf(buf, "0x%x\n", data);
}

static DEVICE_ATTR_RW(sfp_txdis);
static DEVICE_ATTR_RO(sfp_modprs);
static DEVICE_ATTR_RO(sfp_rxlos);
static DEVICE_ATTR_RO(sfp_txfault);
static DEVICE_ATTR_RW(sfpplus_txdis);
static DEVICE_ATTR_RO(sfpplus_modprs);
static DEVICE_ATTR_RO(sfpplus_rxlos);
static DEVICE_ATTR_RO(sfpplus_txfault);
static DEVICE_ATTR_RO(qsfp_modprs);
static DEVICE_ATTR_RW(qsfp_rst);
static DEVICE_ATTR_RW(qsfp_lpmode);
static DEVICE_ATTR_RW(reboot_cause);
static DEVICE_ATTR_RW(power_reset);
static DEVICE_ATTR_RO(psu0_prs);
static DEVICE_ATTR_RO(psu1_prs);
static DEVICE_ATTR_RO(psu0_status);
static DEVICE_ATTR_RO(psu1_status);
static DEVICE_ATTR_RW(system_led);
static DEVICE_ATTR_RW(watchdog);
static DEVICE_ATTR_RW(locator_led);
static DEVICE_ATTR_RW(power_led);
static DEVICE_ATTR_RW(primary_led);
static DEVICE_ATTR_RW(fan_led);
static DEVICE_ATTR_RO(power_good);
static DEVICE_ATTR_RO(cpu_cpld_mjr_ver);
static DEVICE_ATTR_RO(cpu_cpld_mnr_ver);
static DEVICE_ATTR_RO(sys_cpld_mjr_ver);
static DEVICE_ATTR_RO(sys_cpld_mnr_ver);
static DEVICE_ATTR_RO(port_cpld_mjr_ver);
static DEVICE_ATTR_RO(port_cpld_mnr_ver);

static SENSOR_DEVICE_ATTR(fan0_dir, S_IRUGO, fan_dir_show, NULL, FAN_0);
static SENSOR_DEVICE_ATTR(fan1_dir, S_IRUGO, fan_dir_show, NULL, FAN_1);
static SENSOR_DEVICE_ATTR(fan2_dir, S_IRUGO, fan_dir_show, NULL, FAN_2);
static SENSOR_DEVICE_ATTR(fan0_prs, S_IRUGO, fan_prs_show, NULL, FAN_0);
static SENSOR_DEVICE_ATTR(fan1_prs, S_IRUGO, fan_prs_show, NULL, FAN_1);
static SENSOR_DEVICE_ATTR(fan2_prs, S_IRUGO, fan_prs_show, NULL, FAN_2);
static SENSOR_DEVICE_ATTR(fan0_led, S_IRUGO | S_IWUSR, fani_led_show, fani_led_store, FAN_0);
static SENSOR_DEVICE_ATTR(fan1_led, S_IRUGO | S_IWUSR, fani_led_show, fani_led_store, FAN_1);
static SENSOR_DEVICE_ATTR(fan2_led, S_IRUGO | S_IWUSR, fani_led_show, fani_led_store, FAN_2);

static struct attribute *e3224f_cpld_attrs[] = {
    &dev_attr_sfp_txdis.attr,
    &dev_attr_sfp_modprs.attr,
    &dev_attr_sfp_rxlos.attr,
    &dev_attr_sfp_txfault.attr,
    &dev_attr_sfpplus_txdis.attr,
    &dev_attr_sfpplus_modprs.attr,
    &dev_attr_sfpplus_rxlos.attr,
    &dev_attr_sfpplus_txfault.attr,
    &dev_attr_qsfp_modprs.attr,
    &dev_attr_qsfp_rst.attr,
    &dev_attr_qsfp_lpmode.attr,
    &dev_attr_reboot_cause.attr,
    &dev_attr_power_reset.attr,
    &sensor_dev_attr_fan0_dir.dev_attr.attr,
    &sensor_dev_attr_fan1_dir.dev_attr.attr,
    &sensor_dev_attr_fan2_dir.dev_attr.attr,
    &sensor_dev_attr_fan0_prs.dev_attr.attr,
    &sensor_dev_attr_fan1_prs.dev_attr.attr,
    &sensor_dev_attr_fan2_prs.dev_attr.attr,
    &sensor_dev_attr_fan0_led.dev_attr.attr,
    &sensor_dev_attr_fan1_led.dev_attr.attr,
    &sensor_dev_attr_fan2_led.dev_attr.attr,
    &dev_attr_psu0_prs.attr,
    &dev_attr_psu1_prs.attr,
    &dev_attr_psu0_status.attr,
    &dev_attr_psu1_status.attr,
    &dev_attr_system_led.attr,
    &dev_attr_watchdog.attr,
    &dev_attr_locator_led.attr,
    &dev_attr_power_led.attr,
    &dev_attr_primary_led.attr,
    &dev_attr_fan_led.attr,
    &dev_attr_power_good.attr,
    &dev_attr_cpu_cpld_mjr_ver.attr,
    &dev_attr_cpu_cpld_mnr_ver.attr,
    &dev_attr_sys_cpld_mjr_ver.attr,
    &dev_attr_sys_cpld_mnr_ver.attr,
    &dev_attr_port_cpld_mjr_ver.attr,
    &dev_attr_port_cpld_mnr_ver.attr,
    NULL,
};

static struct attribute_group e3224f_cpld_attr_grp = {
    .attrs = e3224f_cpld_attrs,
};

static int get_ismt_base_nr(void)
{
    struct i2c_adapter *ismt_adap;
    static int ismt_base_nr = -1;

    if (ismt_base_nr != -1) {
        return ismt_base_nr;
    }
    for (ismt_base_nr = 0; ismt_base_nr < 2; ismt_base_nr++) {
        ismt_adap = i2c_get_adapter(ismt_base_nr);
        if (!ismt_adap) {
            printk(KERN_WARNING "iSMT adapter (%d) not found\n", ismt_base_nr);
            return -ENODEV;
        }
        if (!strstr(ismt_adap->name, "iSMT adapter")) {
            i2c_put_adapter(ismt_adap);
            printk("I2C %d adapter is %s\n", ismt_base_nr, ismt_adap->name);
        } else {
            i2c_put_adapter(ismt_adap);
            return ismt_base_nr;
        }
    }
    return -ENODEV;
}

static int get_port_mux_base_nr(void)
{
    struct i2c_adapter *mux_adap;
    static int mux_base_nr = -1;

    if (mux_base_nr != -1) {
        return mux_base_nr;
    }
    for (mux_base_nr = 0; mux_base_nr < 10; mux_base_nr++) {
        mux_adap = i2c_get_adapter(mux_base_nr);
        if (!mux_adap) {
            printk(KERN_WARNING "I2C adapter (%d) not found\n", mux_base_nr);
            continue;
        }
        if (!strstr(mux_adap->name, "mux (chan_id 0)")) {
            i2c_put_adapter(mux_adap);
            printk("I2C %d adapter is %s\n", mux_base_nr, mux_adap->name);
        } else {
            i2c_put_adapter(mux_adap);
            return mux_base_nr;
        }
    }
    return -ENODEV;
}

static int __init cpld_probe(struct platform_device *pdev)
{
    struct cpld_platform_data *pdata;
    struct i2c_adapter *parent;
    int i, cpld_bus, port_cpld_bus;
    int ret;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
        return -ENODEV;
    }

    cpld_bus = get_ismt_base_nr();
    if (cpld_bus < 0) {
        return -ENODEV;
    }
    parent = i2c_get_adapter(cpld_bus);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (%d) not found\n", cpld_bus);
        return -ENODEV;
    }

    for (i = 0; i < (CPLD_DEVICE_NUM - 1); i++) {  // cpu and sys cpld
        pdata[i].client = i2c_new_dummy_device(parent, pdata[i].reg_addr);
        if (!pdata[i].client) {
            printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[i].reg_addr);
            goto error;
        }
    }

    //  PORT CPLD
    port_cpld_bus = get_port_mux_base_nr();
    if (port_cpld_bus < 0) {
        return -ENODEV;
    }
    parent = i2c_get_adapter(port_cpld_bus);
    if (!parent) {
        printk(KERN_WARNING "Parent adapter (port_cpld) not found\n");
        return -ENODEV;
    }
    else
    {
	printk (KERN_WARNING "Parent adapter (port_cpld) FOUND \n");
    }
    pdata[port_cpld].client = i2c_new_dummy_device(parent, pdata[port_cpld].reg_addr);
    if (!pdata[port_cpld].client) {
         printk(KERN_WARNING "Fail to create dummy i2c client for addr %d\n", pdata[port_cpld].reg_addr);
         goto error;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &e3224f_cpld_attr_grp);
    if (ret)
        goto error;

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
    int i;
    struct i2c_adapter *parent = NULL;
    struct cpld_platform_data *pdata = pdev->dev.platform_data;

    sysfs_remove_group(&pdev->dev.kobj, &e3224f_cpld_attr_grp);

    if (!pdata) {
        dev_err(&pdev->dev, "Missing platform data\n");
    } else {
        for (i = 0; i < CPLD_DEVICE_NUM; i++) {
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
        .name   = "dell-e3224f-cpld",
    },
};

static struct i2c_board_info sys_board_mux[] = {
    {
        I2C_BOARD_INFO("pca9548", 0x71)
    }
};

static int __init dell_e3224f_platform_init(void)
{
    int ret = 0;
    struct i2c_adapter *sys_i2c_adap;
    struct i2c_client  *mux_i2c_cli;
    struct cpld_platform_data *cpld_pdata;
    struct mux_platform_data *pdata;
    int i, sys_i2c_bus;

    printk("dell_e3224f_platform module initialization\n");
    sys_i2c_bus = get_ismt_base_nr();
    if (sys_i2c_bus < 0) {
        return -ENODEV;
    }

    sys_i2c_adap = i2c_get_adapter(sys_i2c_bus);
    mux_i2c_cli = i2c_new_client_device(sys_i2c_adap, sys_board_mux);
    if (!mux_i2c_cli)
        return PTR_ERR_OR_ZERO(mux_i2c_cli);

    ret = platform_driver_register(&cpld_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register cpld driver\n");
        goto error_cpld_driver;
    }

    ret = platform_driver_register(&mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register mux driver\n");
        goto error_mux_driver;
    }

    ret = platform_device_register(&e3224f_cpld);
    if (ret) {
        printk(KERN_WARNING "Fail to create cpld device\n");
        goto error_cpld;
    }

    cpld_pdata = e3224f_cpld.dev.platform_data;

    for (i = 0; i < PF_MUX_DEVICES; i++) {
        pdata = e3224f_mux[i].dev.platform_data;
        pdata->cpld = cpld_pdata[sys_cpld].client;
        ret = platform_device_register(&e3224f_mux[i]);
        if (ret) {
            printk(KERN_WARNING "fail to create mux %d\n", i);
            goto error_mux;
        }
    }
    ret = i2c_smbus_write_byte_data(cpld_pdata[sys_cpld].client, PHY_RESET_REG, RESET_ALL_PHY);
    if (ret)
        goto error_mux;

    /* To enable FAN set FAN_EN bit (Set bit 0 to 1) in SYS_MISC_CTRL: 0x0B by read modify write. */
    ret = i2c_smbus_read_byte_data(cpld_pdata[sys_cpld].client, SYS_MISC_CTRL_REG);
    if (ret < 0)
        goto error_mux;
    ret = i2c_smbus_write_byte_data(cpld_pdata[sys_cpld].client, SYS_MISC_CTRL_REG, (u8)(ret | 0x01));
    if (ret)
        goto error_mux;

    return 0;

error_mux:
    i--;
    for (; i >= 0; i--) {
        platform_device_unregister(&e3224f_mux[i]);
    }
    platform_device_unregister(&e3224f_cpld);
error_cpld:
    platform_driver_unregister(&mux_driver);
error_mux_driver:
    platform_driver_unregister(&cpld_driver);
error_cpld_driver:
    return ret;
}

static void __exit dell_e3224f_platform_exit(void)
{
    int i;

    for (i = 0; i < PF_MUX_DEVICES; i++)
        platform_device_unregister(&e3224f_mux[i]);
    platform_device_unregister(&e3224f_cpld);
    platform_driver_unregister(&cpld_driver);
    platform_driver_unregister(&mux_driver);
}

module_init(dell_e3224f_platform_init);
module_exit(dell_e3224f_platform_exit);

MODULE_DESCRIPTION("DELL E3224F Platform Support");
MODULE_AUTHOR("Dhanakumar Subramanian <dhanakumar_subramani@dell.com>");
MODULE_LICENSE("GPL");
