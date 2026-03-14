// SPDX-License-Identifier: GPL-2.0
/* Copyright (C) 2017-2021 Pensando Systems, Inc
 * Copyright (C) 2008 Magnus Damm
 *
 * Based on uio_pdrv.c by Uwe Kleine-Koenig,
 * Copyright (C) 2008 by Digi International Inc.
 */

#include <linux/module.h>
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/of.h>
#include <linux/ioctl.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/uio_driver.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/stringify.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include "mdev_drv.h"

#define DEVINFO_SIZE            0x1000
#define DRVCFG_SIZE             0x80
#define MSIXCFG_SIZE            0x40
#define DOORBELL_PG_SIZE        0x8
#define TSTAMP_SIZE             0x8
#define MDEV_NODE_NAME_LEN      0x8

typedef enum mdev_type {
	MDEV_TYPE_MNET,
	MDEV_TYPE_MCRYPT,
} mdev_type_t;

struct mdev_dev;

typedef int (*platform_rsrc_func_t)(struct mdev_dev *,
				    struct mdev_create_req *);
typedef int (*attach_func_t)(struct platform_device *);
typedef int (*detach_func_t)(struct platform_device *);

struct mdev_dev {
	struct device_node *of_node;
	struct platform_device *pdev;
	struct list_head node;
	mdev_type_t type;
	platform_rsrc_func_t platform_rsrc;
	attach_func_t attach;
	detach_func_t detach;
};

LIST_HEAD(mdev_list);

static struct class *mdev_class;
static dev_t mdev_dev;
struct device *mdev_device;
struct device *mnet_device;
static unsigned int mdev_major;
static struct cdev mdev_cdev;

/* Yuck */
extern int ionic_probe(struct platform_device *pfdev);
extern int ionic_remove(struct platform_device *pfdev);

struct uio_pdrv_genirq_platdata {
	struct uio_info *uioinfo;
	spinlock_t lock;
	unsigned long flags;
	struct platform_device *pdev;
};

/* Bits in uio_pdrv_genirq_platdata.flags */
enum {
	UIO_IRQ_DISABLED = 0,
};

static int uio_pdrv_genirq_open(struct uio_info *info, struct inode *inode)
{
	return 0;
}

static int uio_pdrv_genirq_release(struct uio_info *info, struct inode *inode)
{
	return 0;
}

static irqreturn_t uio_pdrv_genirq_handler(int irq, struct uio_info *dev_info)
{
	struct uio_pdrv_genirq_platdata *priv = dev_info->priv;

	/* Just disable the interrupt in the interrupt controller, and
	 * remember the state so we can allow user space to enable it later.
	 */

	spin_lock(&priv->lock);
	if (!__test_and_set_bit(UIO_IRQ_DISABLED, &priv->flags))
		disable_irq_nosync(irq);
	spin_unlock(&priv->lock);

	return IRQ_HANDLED;
}

static int uio_pdrv_genirq_irqcontrol(struct uio_info *dev_info, s32 irq_on)
{
	struct uio_pdrv_genirq_platdata *priv = dev_info->priv;
	unsigned long flags;

	/* Allow user space to enable and disable the interrupt
	 * in the interrupt controller, but keep track of the
	 * state to prevent per-irq depth damage.
	 *
	 * Serialize this operation to support multiple tasks and concurrency
	 * with irq handler on SMP systems.
	 */

	spin_lock_irqsave(&priv->lock, flags);
	if (irq_on) {
		if (__test_and_clear_bit(UIO_IRQ_DISABLED, &priv->flags))
			enable_irq(dev_info->irq);
	} else {
		if (!__test_and_set_bit(UIO_IRQ_DISABLED, &priv->flags))
			disable_irq_nosync(dev_info->irq);
	}
	spin_unlock_irqrestore(&priv->lock, flags);

	return 0;
}

static int mdev_uio_pdrv_genirq_probe(struct platform_device *pdev)
{
	struct uio_info *uioinfo = dev_get_platdata(&pdev->dev);
	struct uio_pdrv_genirq_platdata *priv;
	struct uio_mem *uiomem;
	int ret = -EINVAL;
	int i;

	if (pdev->dev.of_node) {
		/* alloc uioinfo for one device */
		uioinfo = devm_kzalloc(&pdev->dev, sizeof(*uioinfo),
				       GFP_KERNEL);
		if (!uioinfo) {
			dev_err(&pdev->dev, "unable to kmalloc\n");
			return -ENOMEM;
		}
		uioinfo->name = pdev->name;
		uioinfo->version = "devicetree";
		/* Multiple IRQs are not supported */
	}

	if (!uioinfo || !uioinfo->name || !uioinfo->version) {
		dev_err(&pdev->dev, "missing platform_data\n");
		return ret;
	}

	if (uioinfo->handler || uioinfo->irqcontrol ||
	    uioinfo->irq_flags & IRQF_SHARED) {
		dev_err(&pdev->dev, "interrupt configuration error\n");
		return ret;
	}

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "unable to kmalloc\n");
		return -ENOMEM;
	}

	priv->uioinfo = uioinfo;
	spin_lock_init(&priv->lock);
	priv->flags = 0; /* interrupt is enabled to begin with */
	priv->pdev = pdev;

	if (!uioinfo->irq) {
#if (KERNEL_VERSION(5, 10, 0) > LINUX_VERSION_CODE)
		ret = platform_get_irq(pdev, 0);
#else
		ret = platform_get_irq_optional(pdev, 0);
#endif
		uioinfo->irq = ret;
		if (ret == -ENXIO && pdev->dev.of_node)
			uioinfo->irq = UIO_IRQ_NONE;
		else if (ret < 0) {
			dev_err(&pdev->dev, "failed to get IRQ: %d\n", ret);
			return ret;
		}
	}

	uiomem = &uioinfo->mem[0];

	for (i = 0; i < pdev->num_resources; ++i) {
		struct resource *r = &pdev->resource[i];

		if (r->flags != IORESOURCE_MEM)
			continue;

		if (uiomem >= &uioinfo->mem[MAX_UIO_MAPS]) {
			dev_warn(&pdev->dev, "device has more than "
					__stringify(MAX_UIO_MAPS)
					" I/O memory resources.\n");
			break;
		}

		uiomem->memtype = UIO_MEM_PHYS;
		uiomem->addr = r->start & PAGE_MASK;
		uiomem->offs = (r->start & (PAGE_SIZE - 1));
		uiomem->size = PAGE_ALIGN(resource_size(r));
		dev_info(&pdev->dev, "resource %d size %llu", i, uiomem->size);
		uiomem->name = r->name;
		++uiomem;
	}

	while (uiomem < &uioinfo->mem[MAX_UIO_MAPS]) {
		uiomem->size = 0;
		++uiomem;
	}

	/* This driver requires no hardware specific kernel code to handle
	 * interrupts. Instead, the interrupt handler simply disables the
	 * interrupt in the interrupt controller. User space is responsible
	 * for performing hardware specific acknowledge and re-enabling of
	 * the interrupt in the interrupt controller.
	 *
	 * Interrupt sharing is not supported.
	 */

	uioinfo->handler = uio_pdrv_genirq_handler;
	uioinfo->irqcontrol = uio_pdrv_genirq_irqcontrol;
	uioinfo->open = uio_pdrv_genirq_open;
	uioinfo->release = uio_pdrv_genirq_release;
	uioinfo->priv = priv;

	ret = uio_register_device(&pdev->dev, priv->uioinfo);
	if (ret) {
		dev_err(&pdev->dev, "unable to register uio device\n");
		return ret;
	}

	platform_set_drvdata(pdev, priv);
	return 0;
}

static int mdev_uio_pdrv_genirq_remove(struct platform_device *pdev)
{
	struct uio_pdrv_genirq_platdata *priv = platform_get_drvdata(pdev);

	uio_unregister_device(priv->uioinfo);

	priv->uioinfo->handler = NULL;
	priv->uioinfo->irqcontrol = NULL;

	return 0;
}

static int mdev_open(struct inode *inode, struct file *filep)
{
	return 0;
}

static int mdev_close(struct inode *i, struct file *f)
{
	return 0;
}

static int mdev_get_mnet_platform_rsrc(struct mdev_dev *mdev,
				       struct mdev_create_req *req)
{
	struct resource mnet_resource[] = {
		{ /*devinfo*/
			.flags    = IORESOURCE_MEM,
			.start    = req->regs_pa,
			.end      = req->regs_pa + DEVINFO_SIZE - 1
		}, {/*drvcfg/intr_ctrl*/
			.flags    = IORESOURCE_MEM,
			.start    = req->drvcfg_pa,
			.end      = req->drvcfg_pa + DRVCFG_SIZE - 1
		}, {/*msixcfg*/
			.flags    = IORESOURCE_MEM,
			.start    = req->msixcfg_pa,
			.end      = req->msixcfg_pa + MSIXCFG_SIZE - 1
		}, {/*doorbell*/
			.flags    = IORESOURCE_MEM,
			.start    = req->doorbell_pa,
			.end      = req->doorbell_pa + DOORBELL_PG_SIZE - 1
		}, {/*tstamp*/
			.flags    = IORESOURCE_MEM,
			.start    = req->tstamp_pa,
			.end      = req->tstamp_pa + TSTAMP_SIZE - 1
		}
	};

	/* add resource info */
	return platform_device_add_resources(mdev->pdev, mnet_resource,
					     ARRAY_SIZE(mnet_resource));
}

static int mdev_get_mcrypt_platform_rsrc(struct mdev_dev *mdev,
					 struct mdev_create_req *req)
{
	struct resource mcrypt_resource[] = {
		{ /*devinfo*/
			.flags    = IORESOURCE_MEM,
			.start    = req->regs_pa,
			.end      = req->regs_pa + DEVINFO_SIZE - 1
		}, {/*drvcfg/intr_ctrl*/
			.flags    = IORESOURCE_MEM,
			.start    = req->drvcfg_pa,
			.end      = req->drvcfg_pa + DRVCFG_SIZE - 1
		}, {/*msixcfg*/
			.flags    = IORESOURCE_MEM,
			.start    = req->msixcfg_pa,
			.end      = req->msixcfg_pa + MSIXCFG_SIZE - 1
		}, {/*doorbell*/
			.flags    = IORESOURCE_MEM,
			.start    = req->doorbell_pa,
			.end      = req->doorbell_pa + DOORBELL_PG_SIZE - 1
		}
	};

	/* add resource info */
	return platform_device_add_resources(mdev->pdev, mcrypt_resource,
					     ARRAY_SIZE(mcrypt_resource));
}

static int mdev_attach_one(struct mdev_dev *mdev,
			   struct mdev_create_req *req)
{
	char *mdev_name = NULL;
	int err = 0;

	mdev->pdev = of_find_device_by_node(mdev->of_node);
	if (!mdev->pdev) {
		dev_err(mdev_device, "Can't find device for of_node %s\n",
			mdev->of_node->name);
		err = -ENXIO;
		goto err;
	}

	err = (*mdev->platform_rsrc)(mdev, req);
	if (err) {
		dev_err(mdev_device, "Can't get platform resources\n");
		err = -ENOSPC;
		goto err_unset_pdev;
	}

	mdev_name = devm_kzalloc(mdev_device, MDEV_NAME_LEN + 1, GFP_KERNEL);
	if (!mdev_name) {
		dev_err(mdev_device, "Can't allocate memory for name\n");
		err = -ENOMEM;
		goto err_unset_pdev;
	}

	strncpy(mdev_name, req->name, MDEV_NAME_LEN);
	mdev->pdev->name = mdev_name;

	/* call probe with this platform_device */
	err = (*mdev->attach)(mdev->pdev);
	if (err) {
		dev_err(mdev_device, "probe for %s failed: %d\n",
			mdev->pdev->name, err);
		goto err_free_name;
	}

	dev_info(mdev_device, "%s created successfully\n", mdev->pdev->name);
	return 0;

err_free_name:
	//devm_kfree(mdev_device, mdev->pdev->name);
	//mdev->pdev->name = NULL;
err_unset_pdev:
	mdev->pdev = NULL;
err:
	return err;
}

static int mdev_detach_one(struct mdev_dev *mdev)
{
	int err;

	if (!mdev->pdev)
		return 0;

	dev_info(mdev_device, "Removing interface %s\n", mdev->pdev->name);
	err = (*mdev->detach)(mdev->pdev);
	if (err) {
		dev_err(mdev_device, "Failed to remove %s\n",
			mdev->pdev->name);
		return err;
	}

	dev_info(mdev_device, "Successfully removed %s\n", mdev->pdev->name);

	//devm_kfree(mdev_device, mdev->pdev->name);
	mdev->pdev = NULL;

	return 0;
}

static inline bool mdev_ioctl_matches(struct mdev_dev *mdev, uint32_t cmd)
{
	if (cmd == MDEV_CREATE_MNET && mdev->type == MDEV_TYPE_MNET)
		return true;

	if (cmd == MDEV_CREATE_MCRYPT && mdev->type == MDEV_TYPE_MCRYPT)
		return true;

	return false;
}

static long mdev_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	char name[MDEV_NAME_LEN+1] = {0};
	struct mdev_create_req req;
	struct mdev_dev *mdev;
	int ret = -EDQUOT;

	switch (cmd) {
	case MDEV_CREATE_MNET:
	case MDEV_CREATE_MCRYPT:
		if (copy_from_user(&req, argp, sizeof(req))) {
			dev_err(mdev_device, "copy_from_user failed\n");
			ret = -EFAULT;
			break;
		}
		dev_info(mdev_device, "Creating %s %s\n",
			 req.name, req.is_uio_dev ? "(UIO)" : "");

		/* scan the list to see if it already exists,
		 * and if so, quietly ignore this request
		 */
		list_for_each_entry(mdev, &mdev_list, node) {
			if (mdev->pdev &&
			    !strncmp(mdev->pdev->name, req.name, MDEV_NAME_LEN))
				return 0;
		}

		/* find the first useful empty slot */
		list_for_each_entry(mdev, &mdev_list, node) {
			if (mdev->pdev || !mdev_ioctl_matches(mdev, cmd))
				continue;

			if (req.is_uio_dev) {
				mdev->attach = mdev_uio_pdrv_genirq_probe;
				mdev->detach = mdev_uio_pdrv_genirq_remove;
			} else if (mdev->type == MDEV_TYPE_MNET) {
				mdev->attach = ionic_probe;
				mdev->detach = ionic_remove;
			} else {
				ret = -EINVAL;
				break;
			}

			ret = mdev_attach_one(mdev, &req);
			break;
		}
		break;

	case MDEV_DESTROY:
		if (copy_from_user(name, argp, MDEV_NAME_LEN)) {
			dev_err(mdev_device, "copy_from_user failed\n");
			ret = -EFAULT;
			break;
		}
		dev_info(mdev_device, "Removing %s\n", name);
		list_for_each_entry(mdev, &mdev_list, node) {
			if (!mdev->pdev ||
			    strncmp(mdev->pdev->name, name, MDEV_NAME_LEN))
				continue;

			ret = mdev_detach_one(mdev);
			break;
		}
		break;

	default:
		dev_dbg(mdev_device, "Invalid ioctl %d\n", cmd);
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int mdev_probe(struct platform_device *pfdev)
{
	return 0;
}

static void mdev_remove(struct platform_device *pfdev)
{
	struct mdev_dev *mdev, *tmp;

	list_for_each_entry_safe(mdev, tmp, &mdev_list, node) {
		(void)mdev_detach_one(mdev);
		list_del(&mdev->node);
		devm_kfree(mdev_device, mdev);
	}

	return;
}

static const struct of_device_id mdev_of_match[] = {
	{.compatible = "pensando,mnet"},
	{.compatible = "pensando,mcrypt"},
	{/* end of table */}
};

static struct platform_driver mdev_driver = {
	.probe = mdev_probe,
	.remove = mdev_remove,
	.driver = {
		.name = "pensando-mdev",
		.owner = THIS_MODULE,
		.of_match_table = mdev_of_match,
	},
};

static const struct file_operations mdev_fops = {
	.owner = THIS_MODULE,
	.open = mdev_open,
	.release = mdev_close,
	.unlocked_ioctl = mdev_ioctl,
};

static int mdev_init_dev_list(uint32_t max_dev, const char *pfx,
			      platform_rsrc_func_t platform_rsrc)
{
	char of_node_name[MDEV_NODE_NAME_LEN + 1] = {0};
	struct mdev_dev *mdev;
	uint32_t i;

	for (i = 0; i < max_dev; i++) {
		mdev = devm_kzalloc(mdev_device, sizeof(*mdev), GFP_KERNEL);
		if (!mdev)
			return -ENOMEM;

		snprintf(of_node_name, sizeof(of_node_name), "%s%u",
			 pfx, i);
		mdev->of_node = of_find_node_by_name(NULL, of_node_name);

		/* skip any node not found in device tree */
		if (mdev->of_node == NULL) {
			devm_kfree(mdev_device, mdev);
			continue;
		}

		dev_info(mdev_device, "Found node %s\n", mdev->of_node->name);
		mdev->platform_rsrc = platform_rsrc;
		list_add_tail(&mdev->node, &mdev_list);

		// TODO: Should this put() happen when driver unloads?
		of_node_put(mdev->of_node);
	}

	return 0;
}

static int __init mdev_init(void)
{
	struct mdev_dev *mdev, *tmp;
	int ret;

	mdev_class = class_create(DRV_NAME);
	if (IS_ERR(mdev_class)) {
		ret = PTR_ERR(mdev_class);
		goto error_out;
	}

	ret = alloc_chrdev_region(&mdev_dev, 0, NUM_MDEV_DEVICES,
				  MDEV_CHAR_DEV_NAME);
	if (ret < 0)
		goto error_destroy_class;

	mdev_major = MAJOR(mdev_dev);

	pr_info("Pensando mdev driver: mdev_major = %d\n", mdev_major);

	mdev_device = device_create(mdev_class, NULL,
				    MKDEV(mdev_major, 0), NULL, DRV_NAME);
	if (IS_ERR(mdev_device)) {
		pr_err("Failed to create device %s", DRV_NAME);
		ret = PTR_ERR(mdev_class);
		goto error_unregister_chrdev;
	}

	dev_info(mdev_device, "device %s created\n", DRV_NAME);

#ifndef MDEV_HACK
	mnet_device = device_create(mdev_class, NULL,
				    MKDEV(mdev_major, 1), NULL, DRV_NAME_ALT);
	if (IS_ERR(mnet_device)) {
		pr_err("Failed to create device %s", DRV_NAME_ALT);
		ret = PTR_ERR(mdev_class);
		goto error_destroy_mdev;
	}

	dev_info(mdev_device, "device %s created\n", DRV_NAME_ALT);
#endif

	cdev_init(&mdev_cdev, &mdev_fops);

	mdev_cdev.owner = THIS_MODULE;

	ret = cdev_add(&mdev_cdev, mdev_dev, NUM_MDEV_DEVICES);
	if (ret) {
		dev_err(mdev_device, "Error adding character device %s\n",
			MDEV_CHAR_DEV_NAME);
		goto error_destroy_mnet;
	}

	ret = mdev_init_dev_list(MAX_MNET_DEVICES, "mnet",
				 mdev_get_mnet_platform_rsrc);
	if (ret)
		goto error_destroy_cdev;

	ret = mdev_init_dev_list(MAX_MCRYPT_DEVICES, "mcrypt",
				 mdev_get_mcrypt_platform_rsrc);
	if (ret)
		goto error_destroy_list;

	ret = platform_driver_register(&mdev_driver);
	if (ret)
		goto error_destroy_list;

	return 0;

error_destroy_list:
	list_for_each_entry_safe(mdev, tmp, &mdev_list, node) {
		list_del(&mdev->node);
		devm_kfree(mdev_device, mdev);
	}
error_destroy_cdev:
	cdev_del(&mdev_cdev);
error_destroy_mnet:
#ifndef MDEV_HACK
	device_destroy(mdev_class, MKDEV(mdev_major, 1));
error_destroy_mdev:
#endif
	device_destroy(mdev_class, MKDEV(mdev_major, 0));
error_unregister_chrdev:
	unregister_chrdev_region(mdev_dev, NUM_MDEV_DEVICES);
error_destroy_class:
	class_destroy(mdev_class);
error_out:
	return ret;
}

static void __exit mdev_cleanup(void)
{
	platform_driver_unregister(&mdev_driver);
	cdev_del(&mdev_cdev);
#ifndef MDEV_HACK
	device_destroy(mdev_class, MKDEV(mdev_major, 1));
#endif
	device_destroy(mdev_class, MKDEV(mdev_major, 0));
	unregister_chrdev_region(mdev_dev, NUM_MDEV_DEVICES);
	class_destroy(mdev_class);
}

module_init(mdev_init);
module_exit(mdev_cleanup);

MODULE_AUTHOR("Pensando Systems");
MODULE_DESCRIPTION(DRV_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_VERSION(DRV_VERSION);

