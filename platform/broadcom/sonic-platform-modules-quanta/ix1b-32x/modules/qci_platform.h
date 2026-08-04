/*
 * Driver model definations for Quanta Platform drivers
 *
 * Copyright (C) 2015-2016 	Quanta QCT
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef __QCI_PLATFORM_H_INCLUDED
#define __QCI_PLATFORM_H_INCLUDED

#define MUX_INFO(bus, deselect) \
	{.adap_id = bus, .deselect_on_exit = deselect}

#define GPIO_INFO(id, gpio_nr) \
	{.gpio_id = id, .system_gpio_nr = gpio_nr}

struct platform_gpio {
	int gpio_id;
	int system_gpio_nr;
};

/* FIXME: Please add important GPIO which need to be request */
enum PLATFORM_GPIO_ID {
	GPIO_PSU1_PRSNT,
	GPIO_PSU1_PWRGD,
	GPIO_PSU2_PRSNT,
	GPIO_PSU2_PWRGD,
};

#define LED_ON  LED_FULL

#define LED_INFO(id) \
	{.led_id = id, .cdev = NULL}

struct platform_led {
	int led_id;
	struct led_classdev *cdev;
};

enum PLATFORM_LED_ID {
	SYSLED_AMBER,
	SYSLED_GREEN,
	FRONT_PSU1_GREEN,
	FRONT_PSU1_RED,
	FRONT_PSU2_GREEN,
	FRONT_PSU2_RED,
	FRONT_FAN_GREEN,
	FRONT_FAN_RED,
	REAR_FAN1_RED,
	REAR_FAN2_RED,
	REAR_FAN3_RED,
	REAR_FAN4_RED,
	REAR_FAN5_RED,
	REAR_FAN6_RED,
	TOTAL_LED_NR
};

extern int qci_platform_get_gpio(int platform_gpio);
extern int qci_platform_set_led(int led_id, bool led_on);

#endif		/* __QCI_PLATFORM_H_INCLUDED */
