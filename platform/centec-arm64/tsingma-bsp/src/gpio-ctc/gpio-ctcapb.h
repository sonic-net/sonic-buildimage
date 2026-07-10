/*
 * Copyright(c) 2014 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef GPIO_CTC_APB_H
#define GPIO_CTC_APB_H

struct ctcapb_port_property {
	struct fwnode_handle *fwnode;
	unsigned int idx;
	unsigned int ngpio;
	unsigned int gpio_base;
	unsigned int irq;
	bool irq_shared;
};

struct ctcapb_platform_data {
	struct ctcapb_port_property *properties;
	unsigned int nports;
};

struct GpioSoc_regs {
	u32 GpioDataCtl;	/* 0x00000000 */
	u32 GpioOutCtl;		/* 0x00000004 */
	u32 GpioDebCtl;		/* 0x00000008 */
	u32 GpioReadData;	/* 0x0000000c */
	u32 GpioIntrEn;		/* 0x00000010 */
	u32 GpioIntrMask;	/* 0x00000014 */
	u32 GpioIntrLevel;	/* 0x00000018 */
	u32 GpioIntrPolarity;	/* 0x0000001c */
	u32 GpioIntrStatus;	/* 0x00000020 */
	u32 GpioIntrRaw;	/* 0x00000024 */
	u32 GpioEoiCtl;		/* 0x00000028 */
	u32 rsv11;
	u32 GpioDebCnt;		/* 0x00000030 */
	u32 GpioVerId;		/* 0x00000034 */
	u32 rsv14;
	u32 rsv15;
	u32 GpioHsDataCtl;	/* 0x00000040 */
	u32 GpioHsOutCtl;	/* 0x00000044 */
	u32 GpioHsDebCtl;	/* 0x00000048 */
	u32 GpioHsReadData;	/* 0x0000004c */
	u32 GpioHsIntrEn;	/* 0x00000050 */
	u32 GpioHsIntrMask;	/* 0x00000054 */
	u32 GpioHsIntrLevel;	/* 0x00000058 */
	u32 GpioHsIntrPolarity;	/* 0x0000005c */
	u32 GpioHsIntrStatus;	/* 0x00000060 */
	u32 GpioHsIntrRaw;	/* 0x00000064 */
	u32 GpioHsEoiCtl;	/* 0x00000068 */
};

struct GpioSoc_port_regs {
	u32 GpioDataCtl;
	u32 GpioOutCtl;
	u32 GpioDebCtl;
	u32 GpioReadData;
	u32 GpioIntrEn;
	u32 GpioIntrMask;
	u32 GpioIntrLevel;
	u32 GpioIntrPolarity;
	u32 GpioIntrStatus;
	u32 GpioIntrRaw;
	u32 GpioEoiCtl;
};

#endif
