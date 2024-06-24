#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/leds.h>
#include <linux/slab.h>
#include <linux/dmi.h>

extern int wistron_fpga_sysled_get(int led_type);
extern int wistron_fpga_sysled_set(int led_type, int light_mode);
#define SYSLED_NUM	4

#define DRVNAME "wistron_led"

struct wistron_led_data {
	struct platform_device *pdev;
	struct mutex           lock;
	int                    sysled_status[SYSLED_NUM];
};

static struct wistron_led_data  *ledctl = NULL;

enum led_type {
	LED_TYPE_LOC,
	LED_TYPE_DIAG,
	LED_TYPE_FAN,
	LED_TYPE_PSU,
	LED_TYPE_END
};

enum led_light_mode {
	LED_MODE_OFF = 0,
	LED_MODE_GREEN,
	LED_MODE_AMBER,
	LED_MODE_BLK_GREEN,
	LED_MODE_UNKNOWN
};

static void wistron_led_update(void)
{
	int i;
	mutex_lock(&ledctl->lock);
	for (i = 0; i < LED_TYPE_END; i++) {
		ledctl->sysled_status[i] = wistron_fpga_sysled_get(i);
	}
	mutex_unlock(&ledctl->lock);
}

static void wistron_led_set(struct led_classdev *led_cdev, enum led_brightness led_light_mode, enum led_type type)
{
	mutex_lock(&ledctl->lock);
	wistron_fpga_sysled_set(type, led_light_mode);
	mutex_unlock(&ledctl->lock);
}

static void wistron_led_diag_set(struct led_classdev *led_cdev, enum led_brightness led_light_mode)
{
	wistron_led_set(led_cdev, led_light_mode, LED_TYPE_DIAG);
}

static enum led_brightness wistron_led_diag_get(struct led_classdev *cdev)
{
	wistron_led_update();
	return ledctl->sysled_status[LED_TYPE_DIAG];
}

static void wistron_led_loc_set(struct led_classdev *led_cdev, enum led_brightness led_light_mode)
{
	wistron_led_set(led_cdev, led_light_mode, LED_TYPE_LOC);
}

static enum led_brightness wistron_led_loc_get(struct led_classdev *cdev)
{
	wistron_led_update();
	return ledctl->sysled_status[LED_TYPE_LOC];
}

static void wistron_led_fan_set(struct led_classdev *led_cdev, enum led_brightness led_light_mode)
{
	wistron_led_set(led_cdev, led_light_mode, LED_TYPE_FAN);
}

static enum led_brightness wistron_led_fan_get(struct led_classdev *cdev)
{
	wistron_led_update();
	return ledctl->sysled_status[LED_TYPE_FAN];
}

static void wistron_led_psu_set(struct led_classdev *led_cdev, enum led_brightness led_light_mode)
{
	wistron_led_set(led_cdev, led_light_mode, LED_TYPE_PSU);
}

static enum led_brightness wistron_led_psu_get(struct led_classdev *cdev)
{
	wistron_led_update();
	return ledctl->sysled_status[LED_TYPE_PSU];
}

static struct led_classdev wistron_leds[] = {
	[LED_TYPE_DIAG] = {
		.name            = "wistron_led::sys_led",
		.default_trigger = "unused",
		.brightness_set  = wistron_led_diag_set,
		.brightness_get  = wistron_led_diag_get,
		.flags           = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_BLK_GREEN,
	},
	[LED_TYPE_LOC] = {
		.name            = "wistron_led::loc_led",
		.default_trigger = "unused",
		.brightness_set  = wistron_led_loc_set,
		.brightness_get  = wistron_led_loc_get,
		.flags           = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_BLK_GREEN,
	},
	[LED_TYPE_FAN] = {
		.name            = "wistron_led::fan_led",
		.default_trigger = "unused",
		.brightness_set  = wistron_led_fan_set,
		.brightness_get  = wistron_led_fan_get,
		.flags           = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AMBER,
	},
	[LED_TYPE_PSU] = {
		.name            = "wistron_led::psu_led",
		.default_trigger = "unused",
		.brightness_set  = wistron_led_psu_set,
		.brightness_get  = wistron_led_psu_get,
		.flags           = LED_CORE_SUSPENDRESUME,
		.max_brightness  = LED_MODE_AMBER,
	},
};

static int wistron_led_suspend(struct platform_device *dev, pm_message_t state)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(wistron_leds); i++) {
		led_classdev_suspend(&wistron_leds[i]);
	}

	return 0;
}

static int wistron_led_resume(struct platform_device *dev)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(wistron_leds); i++) {
		led_classdev_resume(&wistron_leds[i]);
	}

	return 0;
}

static int wistron_led_probe(struct platform_device *pdev)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(wistron_leds); i++) {
		ret = led_classdev_register(&pdev->dev, &wistron_leds[i]);

		if (ret < 0)
			break;
	}

	/* Check if all LEDs were successfully registered */
	if (i != ARRAY_SIZE(wistron_leds)) {
		int j;

		/* only unregister the LEDs that were successfully registered */
		for (j = 0; j < i; j++) {
			led_classdev_unregister(&wistron_leds[i]);
		}
	}

	return ret;
}

static int wistron_led_remove(struct platform_device *pdev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(wistron_leds); i++) {
		led_classdev_unregister(&wistron_leds[i]);
	}

	return 0;
}

static struct platform_driver wistron_led_driver = {
	.probe    = wistron_led_probe,
	.remove   = wistron_led_remove,
	.suspend  = wistron_led_suspend,
	.resume   = wistron_led_resume,
	.driver   = {
		.name   = DRVNAME,
		.owner  = THIS_MODULE,
	},
};

static int __init wistron_led_init(void)
{
	int ret;

	ret = platform_driver_register(&wistron_led_driver);
	if (ret < 0)
		goto exit;

	ledctl = kzalloc(sizeof(struct wistron_led_data), GFP_KERNEL);
	if (!ledctl) {
		ret = -ENOMEM;
		platform_driver_unregister(&wistron_led_driver);
		goto exit;
	}

	mutex_init(&ledctl->lock);

	ledctl->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(ledctl->pdev)) {
		ret = PTR_ERR(ledctl->pdev);
		platform_driver_unregister(&wistron_led_driver);
		kfree(ledctl);
		goto exit;
	}

exit:
	return ret;
}

static void __exit wistron_led_exit(void)
{
	platform_device_unregister(ledctl->pdev);
	platform_driver_unregister(&wistron_led_driver);
	kfree(ledctl);
}

module_init(wistron_led_init);
module_exit(wistron_led_exit);

MODULE_AUTHOR("Wistron");
MODULE_DESCRIPTION("wistron_led driver");
MODULE_LICENSE("GPL");
