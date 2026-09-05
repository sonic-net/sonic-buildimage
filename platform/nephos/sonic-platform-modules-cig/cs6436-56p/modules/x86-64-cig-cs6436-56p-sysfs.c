/*
 * A hwmon driver for the CIG cs6436-56P sysfs Module
 *
 * Copyright (C) 2018 Cambridge, Inc.
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
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/uaccess.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h> 
#include <linux/fs.h> 
#include <asm/uaccess.h> 
#include <asm/device.h>
#include <linux/cdev.h>

#include "i2c-algo-lpc.h"


static LIST_HEAD(sysfs_client_list);
static struct mutex      list_lock;

struct sysfs_client_node {
        struct i2c_client *client;
        struct list_head   list;
};

#define DEVICE_NAME  "cigfs"
static int dev_major;
static struct class *dev_class;
static struct cdev *dev_cdev;
static struct device *dev_device;
static struct class *psu_class;
static struct class *sfp_class;


void cs6436_56p_sysfs_add_client(struct i2c_client *client)
{
        struct sysfs_client_node *node = kzalloc(sizeof(struct sysfs_client_node), GFP_KERNEL);

        if (!node) {
                dev_dbg(&client->dev, "Can't allocate sysfs_client_node (0x%x)\n", client->addr);
                return;
        }
        node->client = client;

        mutex_lock(&list_lock);
        list_add(&node->list, &sysfs_client_list);
        mutex_unlock(&list_lock);
}

EXPORT_SYMBOL(cs6436_56p_sysfs_add_client);

void cs6436_56p_sysfs_remove_client(struct i2c_client *client)
{
        struct list_head *list_node = NULL;
        struct sysfs_client_node *sysfs_node = NULL;
        int found = 0;

        mutex_lock(&list_lock);

        list_for_each(list_node, &sysfs_client_list)
        {
            sysfs_node = list_entry(list_node, struct sysfs_client_node, list);
			if (IS_ERR(sysfs_node))
			{
				break;
			}
            if (sysfs_node->client == client) {
                found = 1;
                break;
            }
        }
        if (found) {
                list_del(list_node);
                kfree(sysfs_node);
        }

        mutex_unlock(&list_lock);
}

EXPORT_SYMBOL(cs6436_56p_sysfs_remove_client);

struct class * cs6436_56p_sysfs_create_symclass(char *cls_name)
{
	int rc = 0;
	struct class *my_class;
/**************************************************************************************/
	my_class = class_create(THIS_MODULE,cls_name);
	if (IS_ERR(my_class)) {
		pr_err("failed to create my class\n");
	}
	return my_class;

/**************************************************************************************/
}

void cs6436_56p_sysfs_delete_symclass(struct class *my_class)
{
/**************************************************************************************/

	if (IS_ERR(my_class)) {
		pr_err("Pointer is invaild\n");
	}
	class_destroy(my_class);

/**************************************************************************************/
}





int cs6436_56p_sysfs_create_symlink(struct class *my_class,char * driver_name,char *device_name)
{
    struct list_head   *list_node = NULL;
    struct sysfs_client_node *sysfs_node = NULL;
    int ret = -EPERM;
	int rc = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &sysfs_client_list)
    {
        sysfs_node = list_entry(list_node, struct sysfs_client_node, list);
        if (!strcmp(sysfs_node->client->name,driver_name)) {	
			rc = sysfs_create_link(&my_class->p->subsys.kobj, &sysfs_node->client->dev.kobj,device_name);
			if(rc)
			{
				pr_err("failed to create symlink %d\n",rc); 
			}
            break;
        }
    }
    mutex_unlock(&list_lock);
    return ret;
}


int cs6436_56p_sysfs_delete_symlink(struct class *my_class,char * driver_name,char *device_name)
{
    struct list_head   *list_node = NULL;
    struct sysfs_client_node *sysfs_node = NULL;
    int ret = -EPERM;
	int rc = 0;

    mutex_lock(&list_lock);
    list_for_each(list_node, &sysfs_client_list)
    {
        sysfs_node = list_entry(list_node, struct sysfs_client_node, list);
        if (!strcmp(sysfs_node->client->name,driver_name)) {	
			sysfs_remove_link(&my_class->p->subsys.kobj,device_name);
            break;
        }
    }
    mutex_unlock(&list_lock);
    return ret;
}


static int cs6436_56p_sysfs_open(struct inode *inode, struct file *file)
{
    return 0;
}



static ssize_t cs6436_56p_sysfs_write(struct file *file, const char __user *buf, size_t count, loff_t * ppos)
{
	char str[10],name[18],port[8];
	int ret;
	int i;
	memset(str, 0, sizeof(str));
	ret = copy_from_user(str, buf, count);
	if (ret)
	{
	   printk(KERN_ERR "copy_from_user fail\n");
	   return -EINVAL;
	}

	if(!strncmp(str,"start",5))
	{
		psu_class = cs6436_56p_sysfs_create_symclass("psu");
		cs6436_56p_sysfs_create_symlink(psu_class,"cs6436_56p_psu1","psu1");
		cs6436_56p_sysfs_create_symlink(psu_class,"cs6436_56p_psu2","psu2");
		sfp_class = cs6436_56p_sysfs_create_symclass("swps");
		for(i = 1; i <= 48;i++)
		{
			memset(name,0xff,sizeof(name));
			memset(port,0xff,sizeof(port));
			snprintf(name,sizeof(name),"cs6436_56p_sfp%d",i);
			snprintf(port,sizeof(port),"port%d",i);
			cs6436_56p_sysfs_create_symlink(sfp_class,name,port);
		}
	}
	else if(!strncmp(str,"stop",4))
	{
		cs6436_56p_sysfs_delete_symlink(psu_class,"cs6436_56p_psu1","psu1");
		cs6436_56p_sysfs_delete_symlink(psu_class,"cs6436_56p_psu2","psu2");
		cs6436_56p_sysfs_delete_symclass(psu_class);

		for(i = 1; i <= 48;i++)
		{
			memset(name,0xff,sizeof(name));
			memset(port,0xff,sizeof(port));
			snprintf(name,sizeof(name),"cs6436_56p_sfp%d",i);
			snprintf(port,sizeof(port),"port%d",i);
			cs6436_56p_sysfs_delete_symlink(sfp_class,name,port);
		}
		cs6436_56p_sysfs_delete_symclass(sfp_class);
	}
    return count;
}


static struct file_operations cs6436_56p_sysfs_fops = {
    .owner  = THIS_MODULE,
    .open   = cs6436_56p_sysfs_open,            
    .write  = cs6436_56p_sysfs_write,   
};


static int __init cs6436_56p_sysfs_init(void)
{
	int result = 0;
	int err = 0;
	dev_t dev = MKDEV(dev_major, 0);

	if (dev_major)
		result = register_chrdev_region(dev, 1, DEVICE_NAME);
	else {
		result = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
		dev_major = MAJOR(dev);
	}
	if (result < 0)
	{
		printk("unable to get major %d\n", dev_major);
		err= -EINVAL;
	}
	printk("get major is %d\n", dev_major);
	if (dev_major == 0)
		dev_major = result;

	dev_cdev= kmalloc(sizeof(struct cdev), GFP_KERNEL);  
	if(IS_ERR(dev_cdev)) {  
	    err= -ENOMEM;   
	} 

	cdev_init(dev_cdev, &cs6436_56p_sysfs_fops); 
	dev_cdev->owner = THIS_MODULE;
	dev_cdev->ops = &cs6436_56p_sysfs_fops;
	err = cdev_add(dev_cdev, dev, 1);
	if (err)
	{
		printk("error %d add fpga ", err);
		goto err_malloc; 
	}

	dev_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(dev_class))
	{
		printk("Err:failed in creating class.\n");
		goto err_cdev_add;
	}

	dev_device = device_create(dev_class, NULL, MKDEV(dev_major, 0), NULL, DEVICE_NAME);
	if (IS_ERR(dev_device))
	{
		printk("Err:failed in creating device.\n");
		goto err_class_crt;
	}
	
	mutex_init(&list_lock);

	return err;

	err_class_crt:	
		cdev_del(dev_cdev);	
	err_cdev_add:  
		kfree(dev_cdev);  
	err_malloc:  
		unregister_chrdev_region(MKDEV(dev_major,0), 1);  
		
	return err;

}

static void __exit cs6436_56p_sysfs_exit(void)
{
	cdev_del(dev_cdev);
	printk("cdev_del ok\n");
    device_destroy(dev_class, MKDEV(dev_major, 0));
	
    class_destroy(dev_class);

	 if(dev_cdev != NULL)
        kfree(dev_cdev);

    unregister_chrdev_region(MKDEV(dev_major, 0), 1);
	printk("cs6436_56p_sysfs_exit...\r\n");
}


MODULE_AUTHOR("Zhang Peng <zhangpeng@cigtech.com>");
MODULE_DESCRIPTION("cs6436-56p-sysfs driver");
MODULE_LICENSE("GPL");

module_init(cs6436_56p_sysfs_init);
module_exit(cs6436_56p_sysfs_exit);


