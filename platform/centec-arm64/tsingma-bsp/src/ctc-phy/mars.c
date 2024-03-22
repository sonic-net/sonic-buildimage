/*
 * drivers/net/phy/mars.c
 *
 * Driver for Centec PHYs
 *
 * Author: liuht
 *
 * Copyright 2002-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mii.h>
#include <linux/ethtool.h>
#include <linux/phy.h>
#include <linux/of.h>

#include <linux/io.h>
#include <asm/irq.h>
#include <linux/uaccess.h>

 /* Mask used for ID comparisons */
#define CTC_PHY_ID_MASK		0xffffffff

/* Known PHY IDs */
#define CTC_PHY_ID_MARS1S_V1        0x00782013
#define CTC_PHY_ID_MARS1S           0x01E04013
#define CTC_PHY_ID_MARS1P_V1	    0x00782011
#define CTC_PHY_ID_MARS1P 	    0x01E04011
#define CTC_PHY_IMASK               0x12
#define CTC_PHY_IEVENT              0x13

#define CTC_PHY_IMASK_INIT		0x6c00
#define CTC_PHY_IMASK_CLEAR	0x0000

#define CTC_PHY_REG_SPACE  0
#define CTC_SDS_REG_SPACE 1

static int mars_ext_read(struct phy_device *phydev, u32 regnum)
{
	int ret;

	ret = phy_write(phydev, 0x1e, regnum);
	if (ret < 0)
		return ret;

	return phy_read(phydev, 0x1f);
}

static int mars_ext_write(struct phy_device *phydev, u32 regnum, u16 val)
{
	int ret;

	ret = phy_write(phydev, 0x1e, regnum);
	if (ret < 0)
		return ret;

	return phy_write(phydev, 0x1f, val);
}

static int mars_select_reg_space(struct phy_device *phydev, int space)
{
	int ret;

	if (space == CTC_PHY_REG_SPACE) {
		ret = mars_ext_write(phydev, 0xa000, 0x0);
	} else {
		ret = mars_ext_write(phydev, 0xa000, 0x2);
	}

	return ret;
}

static int mars_config_advert(struct phy_device *phydev)
{
	int err, bmsr, changed = 0;
	u32 adv;

	/* Only allow advertising what this PHY supports */
	linkmode_and(phydev->advertising, phydev->advertising,
		     phydev->supported);

	adv = linkmode_adv_to_mii_adv_t(phydev->advertising);

	/* Setup standard advertisement */
	err = phy_modify_changed(phydev, MII_ADVERTISE,
				 ADVERTISE_ALL | ADVERTISE_100BASE4 |
				 ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM,
				 adv);
	if (err < 0)
		return err;
	if (err > 0)
		changed = 1;

	bmsr = phy_read(phydev, MII_BMSR);
	if (bmsr < 0)
		return bmsr;

	/* Per 802.3-2008, Section 22.2.4.2.16 Extended status all
	 * 1000Mbits/sec capable PHYs shall have the BMSR_ESTATEN bit set to a
	 * logical 1.
	 */
	if (!(bmsr & BMSR_ESTATEN))
		return changed;

	adv = linkmode_adv_to_mii_ctrl1000_t(phydev->advertising);

	err = phy_modify_changed(phydev, MII_CTRL1000,
				 ADVERTISE_1000FULL | ADVERTISE_1000HALF, adv);
	if (err < 0)
		return err;
	if (err > 0)
		changed = 1;

	return changed;
}

int mars1s_config_aneg(struct phy_device *phydev)
{
	int err, changed = 0;

	if (AUTONEG_ENABLE != phydev->autoneg)
		return genphy_setup_forced(phydev);

	err = mars_config_advert(phydev);
	if (err < 0)		/* error */
		return err;

	changed |= err;

	if (changed == 0) {
		/* Advertisement hasn't changed, but maybe aneg was never on to
		 * begin with?  Or maybe phy was isolated?
		 */
		int ctl = phy_read(phydev, MII_BMCR);

		if (ctl < 0)
			return ctl;

		if (!(ctl & BMCR_ANENABLE) || (ctl & BMCR_ISOLATE))
			changed = 1;	/* do restart aneg */
	}

	/* Only restart aneg if we are advertising something different
	 * than we were before.
	 */
	if (changed > 0)
		return genphy_restart_aneg(phydev);

	return 0;
}

static int mars_ack_interrupt(struct phy_device *phydev)
{
	int err;

#if 1
	/* Clear the interrupts by reading the reg */
	err = phy_read(phydev, CTC_PHY_IEVENT);
#else
	err = mars_ext_read(phydev, 0xa011);
#endif
	if (err < 0)
		return err;

	return 0;
}

static int mars_config_intr(struct phy_device *phydev)
{
	int err;

#if 1
	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = phy_write(phydev, CTC_PHY_IMASK, CTC_PHY_IMASK_INIT);
	else
		err = phy_write(phydev, CTC_PHY_IMASK, CTC_PHY_IMASK_CLEAR);
#else
	if (phydev->interrupts == PHY_INTERRUPT_ENABLED)
		err = mars_ext_write(phydev, 0xa010, 0xffff);
	else
		err = mars_ext_write(phydev, 0xa010, 0x0000);
#endif
	return err;
}

#if 0
static int mars_set_link_timer_6_3ms(struct phy_device *phydev)
{
	int ret = 0;

	ret = mars_select_reg_space(phydev, CTC_SDS_REG_SPACE);
	if (!ret)
		mars_ext_write(phydev, 0xa5, 0xc);
	mars_select_reg_space(phydev, CTC_PHY_REG_SPACE);

	return 0;
}
#endif

static int mars_set_link_timer_2_6ms(struct phy_device *phydev)
{
	int ret = 0;

	ret = mars_select_reg_space(phydev, CTC_SDS_REG_SPACE);
	if (!ret)
		mars_ext_write(phydev, 0xa5, 0x5);
	mars_select_reg_space(phydev, CTC_PHY_REG_SPACE);

	return 0;
}

int mars_config_init(struct phy_device *phydev)
{
	return mars_set_link_timer_2_6ms(phydev);
}

int mars1p_config_init(struct phy_device *phydev)
{
	/*RGMII clock 2.5M when link down, bit12:1->0 */
	mars_ext_write(phydev, 0xc, 0x8051);
	/*Disable sleep mode, bit15:1->0 */
	mars_ext_write(phydev, 0x27, 0x2029);
	/* disable PHY to respond to MDIO access with PHYAD0 */
	/* MMD7 8001h: bit6: 0, change value: 0x7f --> 0x3f */
	phy_write(phydev, 0xd, 0x7);
	phy_write(phydev, 0xe, 0x8001);
	phy_write(phydev, 0xd, 0x4007);
	phy_write(phydev, 0xe, 0x3f);

	return mars_set_link_timer_2_6ms(phydev);
}

static struct phy_driver ctc_drivers[] = {
	{
	 .phy_id = CTC_PHY_ID_MARS1S,
	 .phy_id_mask = CTC_PHY_ID_MASK,
	 .name = "CTC MARS1S",
	 .config_init = mars_config_init,
	 .features = PHY_GBIT_FEATURES,
	 .config_aneg = mars1s_config_aneg,
	 .ack_interrupt = &mars_ack_interrupt,
	 .config_intr = &mars_config_intr,
	 .read_status = genphy_read_status,
	 .suspend = genphy_suspend,
	 .resume = genphy_resume,
	 },
	{
	 .phy_id = CTC_PHY_ID_MARS1S_V1,
	 .phy_id_mask = CTC_PHY_ID_MASK,
	 .name = "CTC MARS1S_V1",
	 .config_init = mars_config_init,
	 .features = PHY_GBIT_FEATURES,
	 .config_aneg = mars1s_config_aneg,
	 .ack_interrupt = &mars_ack_interrupt,
	 .config_intr = &mars_config_intr,
	 .read_status = genphy_read_status,
	 .suspend = genphy_suspend,
	 .resume = genphy_resume,
	 },
	{
	 .phy_id = CTC_PHY_ID_MARS1P,
	 .phy_id_mask = CTC_PHY_ID_MASK,
	 .name = "CTC MARS1P",
	 .config_init = mars1p_config_init,
	 .features = PHY_GBIT_FEATURES,
	 .config_aneg = mars1s_config_aneg,
	 .ack_interrupt = &mars_ack_interrupt,
	 .config_intr = &mars_config_intr,
	 .read_status = genphy_read_status,
	 .suspend = genphy_suspend,
	 .resume = genphy_resume,
	 },
	{
	 .phy_id = CTC_PHY_ID_MARS1P_V1,
	 .phy_id_mask = CTC_PHY_ID_MASK,
	 .name = "CTC MARS1P_V1",
	 .config_init = mars1p_config_init,
	 .features = PHY_GBIT_FEATURES,
	 .config_aneg = mars1s_config_aneg,
	 .ack_interrupt = &mars_ack_interrupt,
	 .config_intr = &mars_config_intr,
	 .read_status = genphy_read_status,
	 .suspend = genphy_suspend,
	 .resume = genphy_resume,
	 },
};

module_phy_driver(ctc_drivers);

static struct mdio_device_id __maybe_unused mars_tbl[] = {
	{CTC_PHY_ID_MARS1S, CTC_PHY_ID_MASK},
	{CTC_PHY_ID_MARS1S_V1, CTC_PHY_ID_MASK},
	{CTC_PHY_ID_MARS1P, CTC_PHY_ID_MASK},
	{CTC_PHY_ID_MARS1P_V1, CTC_PHY_ID_MASK},
	{}
};

MODULE_DEVICE_TABLE(mdio, mars_tbl);

MODULE_AUTHOR("Centec, Inc.");
MODULE_LICENSE("GPL");
