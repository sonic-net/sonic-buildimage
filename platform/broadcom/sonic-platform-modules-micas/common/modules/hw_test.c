/*
 * An hw_test driver for hw test read/write function
 *
 * Copyright (C) 2024 Micas Networks Inc.
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

#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <asm/atomic.h>
#include <asm/unistd.h>
#include <linux/major.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <asm/irq.h>
#include <linux/phy.h>
#include <linux/list.h>
#include <linux/slab.h>

#include "hw_test.h"

extern struct bus_type mdio_bus_type;

struct board_mdio_dev {
    struct list_head list;
    struct mii_bus *mdio_bus;
    int mdio_index;
};

struct board_phy_dev {
    struct list_head list;
    struct phy_device *phydev;
    int phy_index;
};

static LIST_HEAD(mdio_dev_list);
static LIST_HEAD(phydev_list);
static struct class *class_mdio_bus = NULL;

#define PRINT_BUF_SIZE         (256)
#define INVALID_PHY_ADDR       (0xFF)
#define MAX_MDIO_DEVICE_NUMS   (1000)
#define MAX_PHY_DEVICE_NUMS    (1000)

#define dram_debug(fmt, ...) \
        printk(KERN_NOTICE pr_fmt(fmt), ##__VA_ARGS__)

static ssize_t dram_dev_read (struct file *file, char __user *buf, size_t count,
                              loff_t *offset)
{
    u8 value8;
    u16 value16;
    u32 value32;

    if (file->private_data != NULL) {
        return -EINVAL;
    }

    file->private_data = ioremap(file->f_pos, count);

    if (!file->private_data) {
        pr_notice("%s, %d\n", __FUNCTION__, __LINE__);
        return -ENODEV;
    }

    rmb();
    switch (count) {
    case 1:
        value8 = readb(file->private_data);
        if (copy_to_user(buf, &value8, sizeof(u8))) {
            return -EFAULT;
        }
        break;
    case 2:
        value16 = readw(file->private_data);
        if (copy_to_user(buf, &value16, sizeof(u16))) {
            return -EFAULT;
        }
        break;
    case 4:
        value32 = readl(file->private_data);
        if (copy_to_user(buf, &value32, sizeof(u32))) {
            return -EFAULT;
        }
        break;
    default:
        return -EINVAL;
    }

    iounmap(file->private_data);
    file->private_data = NULL;
    return count;

}

static ssize_t dram_dev_write (struct file *file, const char __user *buf, size_t count,
                               loff_t *offset)
{
    u8 value8;
    u16 value16;
    u32 value32;

    if (file->private_data != NULL) {
        return -EINVAL;
    }

    file->private_data = ioremap(file->f_pos, count);

    if (!file->private_data) {
        pr_err("%s, %d\n", __FUNCTION__, __LINE__);
        return -ENODEV;
    }

    switch (count) {
    case 1:
        if (copy_from_user(&value8, buf, sizeof(u8))) {
            return -EFAULT;
        }
        writeb(value8, file->private_data);
        break;
    case 2:
        if (copy_from_user(&value16, buf, sizeof(u16))) {
            return -EFAULT;
        }
        writew(value16, file->private_data);
        break;
    case 4:
        if (copy_from_user(&value32, buf, sizeof(u32))) {
            return -EFAULT;
        }
        writel(value32, file->private_data);
        break;
    default:
        return -EINVAL;
    }

    wmb();
    iounmap(file->private_data);
    file->private_data = NULL;
    return count;
}

static loff_t dram_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret;

    switch (origin) {
        case 0:
            file->f_pos = offset;
            ret = file->f_pos;
            break;
        case 1:
            file->f_pos += offset;
            ret = file->f_pos;
            break;
        default:
            ret = -EINVAL;
    }
    return ret;
}

static int temp_mdiobus_read(struct mii_bus *bus, int phy_addr, int regnum)
{
    return 0;
}

static int temp_mdiobus_write(struct mii_bus *bus, int phy_addr, int regnum, u16 value)
{
    return 0;
}

static int init_class_mdio_bus(void)
{
    struct mii_bus *bus;
    int err = 0;

    bus = mdiobus_alloc();

	bus->name  = "temp_mdio_bus";
    snprintf(bus->id, MII_BUS_ID_SIZE, "temp_mdio_bus0");
    bus->read  = temp_mdiobus_read;
	bus->write = temp_mdiobus_write;

	err = mdiobus_register(bus);
	if (err) {
        printk(KERN_ERR "temp mdio bus register fail\n");
        return -1;
	}

    class_mdio_bus = bus->dev.class;
    mdiobus_unregister(bus);

    return 0;
}

static int mdio_match_success(struct device *dev, const void * data)
{

    return 1;
}

static int add_all_mdio_devices_to_list(void)
{
    struct device *dev, *dev_start = NULL;
    struct board_mdio_dev *mdio_dev = NULL;
    int i = 0;
    struct class *bus_class = class_mdio_bus;

    for (i = 0; i < MAX_MDIO_DEVICE_NUMS; i++) {
        dev = class_find_device(bus_class, dev_start, NULL, mdio_match_success);
        if (dev != NULL) {
            mdio_dev = kzalloc(sizeof(struct board_mdio_dev), GFP_KERNEL);
            if (mdio_dev == NULL) {
                printk(KERN_ERR "%s: alloc fail\n", __func__);
                return -EFAULT;
            }

            mdio_dev->mdio_index = i;
            mdio_dev->mdio_bus = to_mii_bus(dev);
            list_add_tail(&mdio_dev->list, &mdio_dev_list);

            dev_start = dev;
        } else {
            break;
        }
    }

    printk(KERN_INFO "mdio dev numbers = %d\n", i);

    return 0;
}

static void delete_all_mdio_devices_from_list(void)
{
    struct list_head *n, *pos;
    struct board_mdio_dev *mdio_dev;

    list_for_each_safe(pos, n, &mdio_dev_list) {
        list_del(pos);
        mdio_dev = list_entry(pos, struct board_mdio_dev, list);
        kfree(mdio_dev);
    }

    return;
}

void list_all_mdio_devices_info(void)
{
    struct board_mdio_dev *mdio_dev;
    unsigned char phyaddr[PHY_MAX_ADDR];
    int i = 0, j = 0;
    int phydev_num = 0;
    char buf[PRINT_BUF_SIZE];
    int len = 0;

    printk(KERN_INFO "all the mdio devs info:\n");
    printk(KERN_INFO "index        busid                 name                  phy_num     phyaddr \n");
    list_for_each_entry(mdio_dev, &mdio_dev_list, list) {
        i = 0;
        j = 0;
        phydev_num = 0;
        mem_clear(phyaddr, INVALID_PHY_ADDR, sizeof(phyaddr));
        mem_clear(buf, 0, sizeof(buf));

        for (i = 0; i < PHY_MAX_ADDR; i++) {
            if (mdio_dev->mdio_bus->mdio_map[i]) {
                phydev_num++;
                phyaddr[j] = (unsigned char)i;
                j++;
            }
        }

        len = snprintf(buf, sizeof(buf), "  %-10d  %-20s  %-20s  %-10d ", mdio_dev->mdio_index,
                  mdio_dev->mdio_bus->id, mdio_dev->mdio_bus->name, phydev_num);

        for (i = 0; i < PHY_MAX_ADDR; i++) {
            if (phyaddr[i] == INVALID_PHY_ADDR) {
                break;
            }

            len += snprintf(&buf[len], sizeof(buf) - len,  " %#x", phyaddr[i]);
        }

        printk(KERN_INFO "%s\n", buf);
    }

    return;
}

static struct mii_bus *get_mdio_dev_according_to_index(int mdio_index)
{
    struct board_mdio_dev *mdio_dev;
    list_for_each_entry(mdio_dev, &mdio_dev_list, list) {
        if (mdio_dev->mdio_index == mdio_index) {
            return mdio_dev->mdio_bus;
        }
    }

    printk(KERN_ERR "no exist the mdio dev it's mdio_index = %d, please exec cmd [hw_test.bin mdiodev_list] to view mdiodev info\n",
        mdio_index);

    return NULL;
}

int board_mdio_read(int mdio_index, int phyaddr, u32 regnum)
{
    struct mii_bus *bus;
    int reg_val;

    bus = get_mdio_dev_according_to_index(mdio_index);
    if (bus == NULL) {
        return -1;
    }

    reg_val = mdiobus_read(bus, phyaddr, regnum);

    return reg_val;
}

int board_mdio_write(int mdio_index, int phyaddr, u32 regnum, u16 val)
{
    struct mii_bus *bus;
    int ret;

    bus = get_mdio_dev_according_to_index(mdio_index);
    if (bus == NULL) {
        return -1;
    }

    ret = mdiobus_write(bus, phyaddr, regnum, val);

    return ret;
}

static int phy_match_success(struct device *dev, const void * data)
{

    return 1;
}

static int add_all_phydevs_to_list(void)
{
    struct device *dev, *dev_start = NULL;
    struct board_phy_dev *board_phydev = NULL;
    int i = 0;

    for (i = 0; i < MAX_PHY_DEVICE_NUMS; i++) {
        dev = bus_find_device(&mdio_bus_type, dev_start, NULL, phy_match_success);
        if (dev != NULL) {
            board_phydev = kzalloc(sizeof(struct board_phy_dev), GFP_KERNEL);
            if (board_phydev == NULL) {
                printk(KERN_ERR "%s: alloc fail\n", __func__);
                return -EFAULT;
            }

            board_phydev->phy_index = i;
            board_phydev->phydev = to_phy_device(dev);
            list_add_tail(&board_phydev->list, &phydev_list);

            dev_start = dev;
        } else {
            break;
        }
    }

    printk(KERN_INFO "phydev num = %d\n", i);

    return 0;
}

static void delete_all_phydevs_from_list(void)
{
    struct list_head *n, *pos;
    struct board_phy_dev *board_phydev;

    list_for_each_safe(pos, n, &phydev_list) {
        list_del(pos);
        board_phydev = list_entry(pos, struct board_phy_dev, list);
        kfree(board_phydev);
    }

    return;
}

void list_all_phydevs_info(void)
{
    struct board_phy_dev *board_phydev;

    printk(KERN_INFO "all the phydevs info:\n");
    printk(KERN_INFO "index        phyaddr     phyId       phydev_name\n");
    list_for_each_entry(board_phydev, &phydev_list, list) {
        printk(KERN_INFO "  %-10d  %#-10x  %#-10x  %-20s\n", board_phydev->phy_index, board_phydev->phydev->mdio.addr,\
              board_phydev->phydev->phy_id, dev_name(&board_phydev->phydev->mdio.dev));
    }

    return;
}

static struct phy_device *get_phy_dev_according_to_index(int phy_index)
{
    struct board_phy_dev *board_phydev;
    list_for_each_entry(board_phydev, &phydev_list, list) {
        if (board_phydev->phy_index == phy_index) {
            return board_phydev->phydev;
        }
    }

    printk(KERN_ERR "no exist the phydev it's phy_index = %d, please exec cmd [hw_test.bin phydev_list] to view phydev info\n", phy_index);

    return NULL;
}

int board_phy_read(int phy_index, u32 regnum)
{
    struct phy_device *phydev;
    int reg_val;

    phydev = get_phy_dev_according_to_index(phy_index);
    if (phydev == NULL) {
        return -1;
    }

    reg_val = phy_read(phydev, regnum);

    return reg_val;
}

int board_phy_write(int phy_index, u32 regnum, u16 val)
{
    struct phy_device *phydev;
    int ret;

    phydev = get_phy_dev_according_to_index(phy_index);
    if (phydev == NULL) {
        return -1;
    }

    ret = phy_write(phydev, regnum, val);

    return ret;
}

static long dram_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    void __user *argp = (void __user *)arg;
    int ret = 0;
    struct phydev_user_info phy_user_info;
    struct mdio_dev_user_info mdio_user_info;

    switch (cmd) {
    case CMD_PHY_LIST:
        list_all_phydevs_info();
        break;

    case CMD_PHY_READ:
        if (copy_from_user(&phy_user_info, argp, sizeof(struct phydev_user_info)))
            return -EFAULT;

        ret = board_phy_read(phy_user_info.phy_index, phy_user_info.regnum);
        if (ret < 0) {
            return -EFAULT;
        }

        phy_user_info.regval = (u32)ret;

        if (copy_to_user(argp, &phy_user_info, sizeof(struct phydev_user_info)))
            return -EFAULT;

        break;

    case CMD_PHY_WRITE:
        if (copy_from_user(&phy_user_info, argp, sizeof(struct phydev_user_info)))
            return -EFAULT;

        ret = board_phy_write(phy_user_info.phy_index, phy_user_info.regnum, (u16)phy_user_info.regval);
        if (ret < 0) {
            return -EFAULT;
        }

        break;

    case CMD_MDIO_LIST:
        list_all_mdio_devices_info();
        break;

    case CMD_MDIO_READ:
        if (copy_from_user(&mdio_user_info, argp, sizeof(struct mdio_dev_user_info)))
            return -EFAULT;

        ret = board_mdio_read(mdio_user_info.mdio_index, mdio_user_info.phyaddr, mdio_user_info.regnum);
        if (ret < 0) {
            return -EFAULT;
        }

        mdio_user_info.regval = (u32)ret;

        if (copy_to_user(argp, &mdio_user_info, sizeof(struct mdio_dev_user_info)))
            return -EFAULT;

        break;

    case CMD_MDIO_WRITE:
        if (copy_from_user(&mdio_user_info, argp, sizeof(struct mdio_dev_user_info)))
            return -EFAULT;

        ret = board_mdio_write(mdio_user_info.mdio_index, mdio_user_info.phyaddr, mdio_user_info.regnum, (u16)mdio_user_info.regval);
        if (ret < 0) {
            return -EFAULT;
        }

        break;

    default:
        printk("unknown ioctl cmd\n");
        break;
    }

    return 0;
}

static int dram_dev_open(struct inode *inode, struct file *file)
{
    file->private_data = NULL;
    file->f_pos = 0;
    return 0;

}

static int dram_dev_release(struct inode *inode, struct file *file)
{
    if (file->private_data) {
        iounmap(file->private_data);
    }
    return 0;
}

static const struct file_operations dram_dev_fops = {
    .owner      = THIS_MODULE,
    .llseek     = dram_dev_llseek,
    .read       = dram_dev_read,
    .write      = dram_dev_write,
    .unlocked_ioctl = dram_dev_ioctl,
    .open       = dram_dev_open,
    .release    = dram_dev_release,
};

static struct miscdevice dram_dev = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "dram_test",
    .fops   = &dram_dev_fops,
};

static int __init dram_init(void)
{
    if (add_all_phydevs_to_list() != 0) {
        printk(KERN_ERR "add all phydev to list fail\n");
        delete_all_phydevs_from_list();
        return -ENXIO;
    }

    if (init_class_mdio_bus() == 0) {
        if (add_all_mdio_devices_to_list() == -EFAULT) {
            printk(KERN_ERR "add all mdiodev to list fail\n");
            delete_all_mdio_devices_from_list();
            delete_all_phydevs_from_list();
            return -ENXIO;
        }
    }

    if (misc_register(&dram_dev) != 0) {
        pr_notice("Register %s failed.\n", dram_dev.name);
        delete_all_mdio_devices_from_list();
        delete_all_phydevs_from_list();
        return -ENXIO;
    }

    return 0;
}

static void __exit dram_exit(void)
{
    misc_deregister(&dram_dev);

    delete_all_mdio_devices_from_list();
    delete_all_phydevs_from_list();
}

module_init(dram_init);
module_exit(dram_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
