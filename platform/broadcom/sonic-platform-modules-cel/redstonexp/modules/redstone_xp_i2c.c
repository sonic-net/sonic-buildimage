/*
 * redstone_xp_i2c.c - driver for Redstone XP's CPLD I2C-MUX
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/acpi.h>
#include <linux/io.h>
#include <linux/dmi.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <uapi/linux/stat.h>

#define DRIVER_NAME "redstone_xp_i2c"

#define CPLD1_VERSION_ADDR 0x100
#define LENGTH_PORT_CPLD        54

#define PORT_BANK1_START        1
#define PORT_BANK1_END          18
#define PORT_BANK2_START        19
#define PORT_BANK2_END          36
#define PORT_BANK3_START        37
#define PORT_BANK3_END          48
#define PORT_BANK4_START        49
#define PORT_BANK4_END          54

#define PORT_ID_BANK1           0x210
#define PORT_ID_BANK2           0x290
#define PORT_ID_BANK3           0x390
#define PORT_ID_BANK4           0x310

#define OPCODE_ID_BANK1         0x211
#define OPCODE_ID_BANK2         0x291
#define OPCODE_ID_BANK3         0x391
#define OPCODE_ID_BANK4         0x311

#define DEVADDR_ID_BANK1        0x212
#define DEVADDR_ID_BANK2        0x292
#define DEVADDR_ID_BANK3        0x392
#define DEVADDR_ID_BANK4        0x312

#define CMDBYT_ID_BANK1         0x213
#define CMDBYT_ID_BANK2         0x293
#define CMDBYT_ID_BANK3         0x393
#define CMDBYT_ID_BANK4         0x313

#define WRITE_ID_BANK1          0x220
#define WRITE_ID_BANK2          0x2A0
#define WRITE_ID_BANK3          0x3A0
#define WRITE_ID_BANK4          0x320

#define READ_ID_BANK1           0x230
#define READ_ID_BANK2           0x2B0
#define READ_ID_BANK3           0x3B0
#define READ_ID_BANK4           0x330

#define SSRR_ID_BANK1           0x216
#define SSRR_ID_BANK2           0x296
#define SSRR_ID_BANK3           0x396
#define SSRR_ID_BANK4           0x316

#define HST_CNTL2_QUICK         0x00
#define HST_CNTL2_BYTE          0x01
#define HST_CNTL2_BYTE_DATA     0x02
#define HST_CNTL2_WORD_DATA     0x03
#define HST_CNTL2_BLOCK         0x05

struct redstone_xp_i2c_data {
        int portid;
};

struct redstone_xp_cpld_data {
        struct i2c_adapter *i2c_adapter[LENGTH_PORT_CPLD];
        struct mutex       cpld_lock;
        uint16_t           read_addr;
};

struct redstone_xp_cpld_data *cpld_data;

static struct resource redstone_xp_i2c_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void redstone_xp_i2c_dev_release(struct device * dev)
{
        return;
}

static struct platform_device redstone_xp_i2c_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(redstone_xp_i2c_resources),
        .resource       = redstone_xp_i2c_resources,
        .dev = {
                        .release = redstone_xp_i2c_dev_release,
        }
};

static int i2c_read_eeprom(struct i2c_adapter *a, u16 addr,
               struct redstone_xp_i2c_data *new_data, u8 cmd, union i2c_smbus_data *data){

        u32 reg;
        int ioBase=0;
        char byte;
        short temp;
        short portid, opcode, devaddr, cmdbyte0, ssrr, writedata, readdata;
        __u16 word_data;
        int error = -EIO;

        mutex_lock(&cpld_data->cpld_lock);

        if ((new_data->portid >= PORT_BANK1_START) && (new_data->portid <= PORT_BANK1_END)) {
                portid = PORT_ID_BANK1;
                opcode = OPCODE_ID_BANK1;
                devaddr = DEVADDR_ID_BANK1;
                cmdbyte0 = CMDBYT_ID_BANK1;
                ssrr = SSRR_ID_BANK1;
                writedata = WRITE_ID_BANK1;
                readdata = READ_ID_BANK1;

        } else if ((new_data->portid >= PORT_BANK2_START) && (new_data->portid <= PORT_BANK2_END)) {
                portid = PORT_ID_BANK2;
                opcode = OPCODE_ID_BANK2;
                devaddr = DEVADDR_ID_BANK2;
                cmdbyte0 = CMDBYT_ID_BANK2;
                ssrr = SSRR_ID_BANK2;
                writedata = WRITE_ID_BANK2;
                readdata = READ_ID_BANK2;

        } else if ((new_data->portid  >= PORT_BANK3_START) && (new_data->portid  <= PORT_BANK3_END)) {
                portid = PORT_ID_BANK3;
                opcode = OPCODE_ID_BANK3;
                devaddr = DEVADDR_ID_BANK3;
                cmdbyte0 = CMDBYT_ID_BANK3;
                ssrr = SSRR_ID_BANK3;
                writedata = WRITE_ID_BANK3;
                readdata = READ_ID_BANK3;

        } else if ((new_data->portid  >= PORT_BANK4_START) && (new_data->portid  <= PORT_BANK4_END)) {
                portid = PORT_ID_BANK4;
                opcode = OPCODE_ID_BANK4;
                devaddr = DEVADDR_ID_BANK4;
                cmdbyte0 = CMDBYT_ID_BANK4;
                ssrr = SSRR_ID_BANK4;
                writedata = WRITE_ID_BANK4;
                readdata = READ_ID_BANK4;

        } else {
            /* Invalid parameter! */
                error = -EINVAL;
                goto exit;
        }

        while ((inb(ioBase + ssrr) & 0x40));
        if ((inb(ioBase + ssrr) & 0x80) == 0x80) {
                error = -EIO;
                /* Read error reset the port */
                outb(0x00, ioBase + ssrr);
                udelay(3000);
                outb(0x01, ioBase + ssrr);
                goto exit;
        }

        byte = 0x40 + new_data->portid;
        reg = cmd;
        outb(byte, ioBase + portid);
        outb(reg, ioBase + cmdbyte0);
        byte = 33; // 33 DX010
        outb(byte, ioBase + opcode);
        addr = addr << 1;
        addr |= 0x01;
        outb(addr, ioBase + devaddr);
        while ((inb(ioBase + ssrr) & 0x40))
        {
                udelay(100);
        }

        if ((inb(ioBase + ssrr) & 0x80) == 0x80) {
            /* Read error reset the port */
                error = -EIO;
                outb(0x00, ioBase + ssrr);
                udelay(3000);
                outb(0x01, ioBase + ssrr);
                goto exit;
        }

        temp = ioBase + readdata;
        word_data = inb(temp);
        word_data |= (inb(++temp) << 8);

        mutex_unlock(&cpld_data->cpld_lock);
        data->word = word_data;
        return 0;

exit:
        mutex_unlock(&cpld_data->cpld_lock);
        return error;
}

static int redstone_xp_i2c_access(struct i2c_adapter *a, u16 addr,
              unsigned short flags, char rw, u8 cmd,
              int size, union i2c_smbus_data *data)
{

        int error = 0;

        struct redstone_xp_i2c_data *new_data;

        /* Write the command register */
        new_data = i2c_get_adapdata(a);

        /* Map the size to what the chip understands */
        switch (size) {
        case I2C_SMBUS_QUICK:
                size = HST_CNTL2_QUICK;
        break;
        case I2C_SMBUS_BYTE:
                size = HST_CNTL2_BYTE;
        break;
        case I2C_SMBUS_BYTE_DATA:
                size = HST_CNTL2_BYTE_DATA;
        break;
        case I2C_SMBUS_WORD_DATA:
            	size = HST_CNTL2_WORD_DATA;
        break;
        case I2C_SMBUS_BLOCK_DATA:
                size = HST_CNTL2_BLOCK;
        break;
        default:
                dev_warn(&a->dev, "Unsupported transaction %d\n", size);
                error = -EOPNOTSUPP;
                goto Done;
        }

        switch (size) {
        case HST_CNTL2_BYTE:    /* Result put in SMBHSTDAT0 */
        break;
        case HST_CNTL2_BYTE_DATA:
        break;
        case HST_CNTL2_WORD_DATA:
                if( 0 == i2c_read_eeprom(a, addr, new_data, cmd, data)){
                    error = 0;
                }else{
                    error = -EIO;
                }
        break;
        }

Done:
        return error;
}

static u32 redstone_xp_i2c_func(struct i2c_adapter *a)
{
        return I2C_FUNC_SMBUS_QUICK | I2C_FUNC_SMBUS_BYTE |
               I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA |
               I2C_FUNC_SMBUS_BLOCK_DATA;
}

static const struct i2c_algorithm redstone_xp_i2c_algorithm = {
        .smbus_xfer = redstone_xp_i2c_access,
        .functionality  = redstone_xp_i2c_func,
};

static struct i2c_adapter * redstone_xp_i2c_init(struct platform_device *pdev, int portid)
{
        int error;

        struct i2c_adapter *new_adapter;
        struct redstone_xp_i2c_data *new_data;

        new_adapter = kzalloc(sizeof(*new_adapter), GFP_KERNEL);
                if (!new_adapter)
                        return NULL;

        new_adapter->dev.parent = &pdev->dev;
        new_adapter->owner = THIS_MODULE;
        new_adapter->class = I2C_CLASS_HWMON | I2C_CLASS_SPD;
        new_adapter->algo  = &redstone_xp_i2c_algorithm;

        snprintf(new_adapter->name, sizeof(new_adapter->name),
                        "SMBus redstone_xp i2c Adapter portid@%04x", portid);

        new_data = kzalloc(sizeof(*new_data), GFP_KERNEL);
        if (!new_data)
                return NULL;

        new_data->portid = portid;

        i2c_set_adapdata(new_adapter,new_data);

        error = i2c_add_adapter(new_adapter);
        if(error)
                return NULL;

        return new_adapter;
};

static int redstone_xp_i2c_drv_probe(struct platform_device *pdev)
{
        struct resource *res;
        int ret = 0;
        int portid_count;

        cpld_data = devm_kzalloc(&pdev->dev, sizeof(struct redstone_xp_cpld_data),
                        GFP_KERNEL);
        if (!cpld_data)
                return -ENOMEM;

        mutex_init(&cpld_data->cpld_lock);
        cpld_data->read_addr = CPLD1_VERSION_ADDR;

        res = platform_get_resource(pdev, IORESOURCE_IO, 0);
        if (unlikely(!res)) {
                printk(KERN_ERR " Specified Resource Not Available...\n");
                return -1;
        }

        for(portid_count=1 ; portid_count<=LENGTH_PORT_CPLD ; portid_count++)
                cpld_data->i2c_adapter[portid_count-1] = redstone_xp_i2c_init(pdev, portid_count);

        return 0;
}

static int redstone_xp_i2c_drv_remove(struct platform_device *pdev)
{
        int portid_count;

        for (portid_count=1 ; portid_count<=LENGTH_PORT_CPLD ; portid_count++)
                i2c_del_adapter(cpld_data->i2c_adapter[portid_count-1]);

        return 0;
}

static struct platform_driver redstone_xp_i2c_drv = {
        .probe  = redstone_xp_i2c_drv_probe,
        .remove = __exit_p(redstone_xp_i2c_drv_remove),
        .driver = {
        .name   = DRIVER_NAME,
        },
};

int redstone_xp_i2c_driver_init(void)
{
        platform_device_register(&redstone_xp_i2c_dev);
        platform_driver_register(&redstone_xp_i2c_drv);

        return 0;
}

void redstone_xp_i2c_driver_exit(void)
{
        platform_driver_unregister(&redstone_xp_i2c_drv);
        platform_device_unregister(&redstone_xp_i2c_dev);
}

module_init(redstone_xp_i2c_driver_init);
module_exit(redstone_xp_i2c_driver_exit);

MODULE_AUTHOR("Derek Vance <dacrepair@gmail.com>");
MODULE_VERSION("1.0.0");
MODULE_DESCRIPTION("Celestica Redstone XP D2060 CPLD I2C-MUX Driver");
MODULE_LICENSE("GPL");