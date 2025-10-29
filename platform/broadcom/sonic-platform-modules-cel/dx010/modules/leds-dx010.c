/*
 * leds-dx010-status.c - Driver for Seastone DX010 front panel LEDs
 *
 * Copyright (C) 2017 Celestica Corp.
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/io.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define DRIVER_NAME     "leds_dx010"
#define FRONT_LED_STAT  0x303

static int dx010_led_blink_stat(struct led_classdev *led_cdev,
                                unsigned long *delay_on,
                                unsigned long *delay_off)
{
        unsigned char led;

        if (!(*delay_on == 0 && *delay_off == 0) &&
            !(*delay_on == 250 && *delay_off == 250) &&
            !(*delay_on == 500 && *delay_off == 500))
                return -EINVAL;

        led = inb(FRONT_LED_STAT);
        led &= 0xfc;

        if ((*delay_on == 250) && (*delay_off == 250))
                led |= 0x02;
        else if ((*delay_on == 500) && (*delay_off == 500))
                led |= 0x01;

        outb(led, FRONT_LED_STAT);

        return 0;
}

static ssize_t dx010_led_blink_show_stat(struct device *dev,
                                  struct device_attribute *attr, char *buf)
{
        struct led_classdev *leddev = dev_get_drvdata(dev);
        unsigned char led;
        const char *msg;

        led = inb(FRONT_LED_STAT);
        led &= 0x03;

        switch (led)
        {
        case 0:
                msg = "No blinking, turn on";
                break;
        case 1:
                msg = "1 Hz is blinking";
                break;
        case 2:
                msg = "4 Hz is blinking";
                break;
        case 3:
                msg = "No blinking, turn off";
                break;
        default:
                msg = "Unknown error";
                break;
        }

        return sprintf(buf, "%s\n", msg);
}

static ssize_t dx010_led_blink_store_stat(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
        int ret;
        struct led_classdev *leddev = dev_get_drvdata(dev);
        unsigned long blink_state;
        unsigned char led;

        ret = kstrtoul(buf, 10, &blink_state);
        if (ret)
                return ret;

        led = inb(FRONT_LED_STAT);
        led &= 0xfc;

        switch (blink_state)
        {
        case 0:
                led |= 0x03;
                break;
        case 1:
                break;
        case 250:
                led |= 0x02;
                break;
        case 500:
                led |= 0x01;
                break;
        default:
                return -EINVAL;
                break;
        }

        outb(led, FRONT_LED_STAT);

        return size;
}
static DEVICE_ATTR(blink, 0644, dx010_led_blink_show_stat, dx010_led_blink_store_stat);

static void dx010_led_brightness_set_stat(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
        unsigned char led;

        led = inb(FRONT_LED_STAT);
        led &= 0xfc;

        if (!brightness)
                led |= 0x03;

        outb( led, FRONT_LED_STAT);
}

enum led_brightness dx010_led_brightness_get_p2(struct led_classdev *led_cdev)
{
        unsigned char led;

        led = inb(FRONT_LED_STAT);

        return (led & 0x08) ? LED_OFF : LED_FULL;
}

static void dx010_led_brightness_set_p2(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
        unsigned char led;

        led = inb(FRONT_LED_STAT);
        led &= 0xf7;

        if (!brightness)
                led |= 0x08;

        outb( led, FRONT_LED_STAT);
}

enum led_brightness dx010_led_brightness_get_p1(struct led_classdev *led_cdev)
{
        unsigned char led;

        led = inb(FRONT_LED_STAT);

        return (led & 0x04) ? LED_OFF : LED_FULL;
}

static void dx010_led_brightness_set_p1(struct led_classdev *led_cdev,
                                        enum led_brightness brightness)
{
        unsigned char led;

        led = inb(FRONT_LED_STAT);
        led &= 0xfb;

        if (!brightness)
                led |= 0x04;

        outb( led, FRONT_LED_STAT);
}

static struct led_classdev dx010_leds[] = {
        {
                .name           = "dx010:green:p-1",
                .brightness     = LED_OFF,
                .max_brightness = 1,
                .brightness_get = dx010_led_brightness_get_p1,
                .brightness_set = dx010_led_brightness_set_p1,
        },
        {
                .name           = "dx010:green:p-2",
                .brightness     = LED_OFF,
                .max_brightness = 1,
                .brightness_get = dx010_led_brightness_get_p2,
                .brightness_set = dx010_led_brightness_set_p2,
        },
        {
                .name           = "dx010:green:stat",
                .brightness     = LED_OFF,
                .max_brightness = 1,
                .brightness_set = dx010_led_brightness_set_stat,
                .blink_set      = dx010_led_blink_stat,
                .flags          = LED_CORE_SUSPENDRESUME,
        },
};

static struct resource dx010_led_resources[] = {
        {
                .flags  = IORESOURCE_IO,
        },
};

static void dx010_led_dev_release( struct device * dev)
{
        return;
}

static struct platform_device dx010_lpc_dev = {
        .name           = DRIVER_NAME,
        .id             = -1,
        .num_resources  = ARRAY_SIZE(dx010_led_resources),
        .resource       = dx010_led_resources,
        .dev = {
                        .release = dx010_led_dev_release,
        }
};

static int dx010_led_drv_probe(struct platform_device *pdev)
{
        int i, ret;

        for (i = 0; i < ARRAY_SIZE(dx010_leds); i++) {
                ret = led_classdev_register(&pdev->dev, &dx010_leds[i]);
                if (ret < 0)
                        goto exit;
        }

        ret = device_create_file(&pdev->dev, &dev_attr_blink);
        if (ret)
        {
                for (i = 0; i < ARRAY_SIZE(dx010_leds); i++)
                        led_classdev_unregister(&dx010_leds[i]);
        }
exit:
        return ret;
}

static void dx010_led_drv_remove(struct platform_device *pdev)
{
        int i;

        for (i = 0; i < ARRAY_SIZE(dx010_leds); i++)
                led_classdev_unregister(&dx010_leds[i]);

        device_remove_file(&pdev->dev, &dev_attr_blink);
}

static struct platform_driver dx010_led_drv = {
        .probe  = dx010_led_drv_probe,
        .remove = __exit_p(dx010_led_drv_remove),
        .driver = {
                .name   = DRIVER_NAME,
        },
};

int dx010_led_init(void)
{
        platform_device_register(&dx010_lpc_dev);
        platform_driver_register(&dx010_led_drv);

        return 0;
}

void dx010_led_exit(void)
{
        platform_driver_unregister(&dx010_led_drv);
        platform_device_unregister(&dx010_lpc_dev);
}

module_init(dx010_led_init);
module_exit(dx010_led_exit);

MODULE_AUTHOR("Abhisit Sangjan  <asang@celestica.com>");
MODULE_DESCRIPTION("Celestica SeaStone DX010 LEDs Front Panel Status Driver");
MODULE_LICENSE("GPL");
