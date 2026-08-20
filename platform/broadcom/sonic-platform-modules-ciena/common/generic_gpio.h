#ifndef _GENERIC_GPIO_H
#define _GENERIC_GPIO_H

#include <linux/device.h>
#include <linux/module.h>          /* module_init/exit, MODULE* */
#include <linux/export.h>
#include <linux/spinlock.h>
#include <linux/bitops.h>
#include <linux/interrupt.h>
#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/platform_device.h> /* platform_*                */
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/slab.h>            /* kzalloc/kfree             */

#define DRIVER_NAME "ciena-" MOD_NAME "-gpio-drv"
#define DEVICE_NAME GPIO_NAME

int CIENA_GPIO_EXPORT_FUNC(struct platform_device *pdev);
int CIENA_GPIO_UNEXPORT_FUNC(struct platform_device *pdev);

/* -------------------------------------------------------------------------- */
static int generic_gpio_platform_probe(struct platform_device *pdev)
{
	int                     rc;
	struct platform_device *pdev_parent = container_of(pdev->dev.parent, struct platform_device, dev);

	dev_info(&pdev->dev, "%s\n", __func__);

	rc = CIENA_GPIO_EXPORT_FUNC(pdev_parent);

	return 0;
}

/* -------------------------------------------------------------------------- */
static void generic_gpio_platform_remove(struct platform_device *pdev)
{
	int                     rc;
	struct platform_device *pdev_parent = container_of(pdev->dev.parent, struct platform_device, dev);

	dev_info(&pdev->dev, "%s\n", __func__);

	rc = CIENA_GPIO_UNEXPORT_FUNC(pdev_parent);
}

/* -------------------------------------------------------------------------- */
static const struct platform_device_id generic_gpio_platform_ids[] = {
	{.name = DEVICE_NAME},
	{},
};
#ifndef CIENA_GPIO_DRIVER
#define CIENA_GPIO_MODULE_EXPORT
MODULE_DEVICE_TABLE(platform, generic_gpio_platform_ids);
#endif

/* -------------------------------------------------------------------------- */
#ifndef CIENA_GPIO_DRIVER
#define CIENA_GPIO_DRIVER generic_gpio_driver
static
#endif
struct platform_driver CIENA_GPIO_DRIVER = {
	.driver = {
		.name           = DRIVER_NAME,
		.owner          = THIS_MODULE,
	},
	.probe    = generic_gpio_platform_probe,
	.remove   = generic_gpio_platform_remove,
	.id_table = generic_gpio_platform_ids,
};

#ifdef CIENA_GPIO_MODULE_EXPORT

module_platform_driver(generic_gpio_driver);
MODULE_DESCRIPTION(MOD_NAME " GPIO driver");
MODULE_LICENSE("GPL v2");

#endif /* CIENA_GPIO_MODULE_EXPORT */

#endif /* _GENERIC_GPIO_H */
