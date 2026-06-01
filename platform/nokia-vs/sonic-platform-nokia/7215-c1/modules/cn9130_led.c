/*
 * LED initialization for Nokia-7215-IXS-C1
 *
 * Copyright (C) 2023 Nokia Corporation.
 * Natarajan Subbiramani <natarajan.subbiramani.ext@nokia.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * see <http://www.gnu.org/licenses/>
 *
 * Based on ad7414.c
 * Copyright 2006 Stefan Roese <sr at denx.de>, DENX Software Engineering
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
#include <linux/io.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define DRV_NAME "cn9130_led"

#define REG_PORT1_AN     0xF2130E0CULL
#define REG_PORT0_AN     0xF2132E0CULL
#define REG_PORT1_TYPE   0xF2130E00ULL
#define REG_PORT0_TYPE   0xF2132E00ULL

#define AN_BYPASS_BIT    3
#define PORT_TYPE_BIT    1
#define AN_VAL_UNSET     (-1)

static const struct {
    u64 phys;
    u32 val;
} writes[] = {
    { 0xF212A004ULL, 0x0010012D },
    { 0xF2130E90ULL, 0x000001FA },
    { 0xF2132E90ULL, 0x000005FA },
    { 0xF212A000ULL, 0x000F0035 },
    { 0xF212A120ULL, 0x00000000 },
    { 0xF2130E0CULL, 0x00009E4C },
    { 0xF2132E0CULL, 0x00009E4C },
};

static struct kobject *cn9130_kobj;
static struct task_struct *poll_thread;

static int bypass_an_val[2] = { AN_VAL_UNSET, AN_VAL_UNSET };

static const struct {
    u64 an_reg;
    u64 type_reg;
} port_regs[2] = {
    { REG_PORT0_AN, REG_PORT0_TYPE },
    { REG_PORT1_AN, REG_PORT1_TYPE },
};

static int mmio_read32(u64 phys, u32 *val)
{
    void __iomem *base, *addr;
    unsigned long page = phys & PAGE_MASK;
    unsigned long off  = phys & ~PAGE_MASK;

    base = ioremap(page, off + sizeof(u32));
    if (!base)
        return -ENOMEM;

    addr = base + off;
    *val = ioread32(addr);
    iounmap(base);
    return 0;
}

static int mmio_write32(u64 phys, u32 val)
{
    void __iomem *base, *addr;
    unsigned long page = phys & PAGE_MASK;
    unsigned long off  = phys & ~PAGE_MASK;

    base = ioremap(page, off + sizeof(u32));
    if (!base)
        return -ENOMEM;

    addr = base + off;
    iowrite32(val, addr);
    iounmap(base);
    return 0;
}

static void set_bit_in_reg(u64 reg, int bit, int val)
{
    u32 regval;

    if (mmio_read32(reg, &regval))
        return;
    if (val)
        regval |= BIT(bit);
    else
        regval &= ~BIT(bit);
    mmio_write32(reg, regval);
}

/* --- poll thread: enforce AN bypass every second --- */

static int cn9130_poll_fn(void *data)
{
    while (!kthread_should_stop()) {
        int i;

        for (i = 0; i < 2; i++) {
            u32 an_regval;

            if (bypass_an_val[i] != AN_VAL_UNSET) {
                /*
                 * User has written a desired value --
                 * enforce it on the AN register every cycle.
                 */
                if (mmio_read32(port_regs[i].an_reg, &an_regval) == 0) {
                    int cur = (an_regval >> AN_BYPASS_BIT) & 1;

                    if (cur != bypass_an_val[i])
                        set_bit_in_reg(port_regs[i].an_reg,
                                       AN_BYPASS_BIT,
                                       bypass_an_val[i]);
                }
            } else {
                /*
                 * No user value yet --
                 * read port_type and copy to AN bypass bit.
                 * The phy link driver sets port_type; we mirror
                 * it into the AN register (type 1 = bypass AN).
                 */
                u32 type_regval;

                if (mmio_read32(port_regs[i].type_reg, &type_regval) == 0) {
                    int type_bit = (type_regval >> PORT_TYPE_BIT) & 1;

                    set_bit_in_reg(port_regs[i].an_reg,
                                   AN_BYPASS_BIT, type_bit);
                }
            }
        }

        ssleep(1);
    }
    return 0;
}

/* --- sysfs helpers --- */

static ssize_t bit_show(u64 reg, int bit, char *buf)
{
    u32 val;

    if (mmio_read32(reg, &val))
        return -EIO;
    return sprintf(buf, "%d\n", (val >> bit) & 1);
}

/* --- sysfs: bypass_an per port --- */

static ssize_t bypass_an_port0_show(struct kobject *kobj,
                                    struct kobj_attribute *attr, char *buf)
{
    return bit_show(REG_PORT0_AN, AN_BYPASS_BIT, buf);
}

static ssize_t bypass_an_port0_store(struct kobject *kobj,
                                     struct kobj_attribute *attr,
                                     const char *buf, size_t count)
{
    unsigned long input;
    int ret;

    ret = kstrtoul(buf, 0, &input);
    if (ret)
        return ret;

    bypass_an_val[0] = input ? 1 : 0;
    set_bit_in_reg(REG_PORT0_AN, AN_BYPASS_BIT, bypass_an_val[0]);
    return count;
}

static ssize_t bypass_an_port1_show(struct kobject *kobj,
                                    struct kobj_attribute *attr, char *buf)
{
    return bit_show(REG_PORT1_AN, AN_BYPASS_BIT, buf);
}

static ssize_t bypass_an_port1_store(struct kobject *kobj,
                                     struct kobj_attribute *attr,
                                     const char *buf, size_t count)
{
    unsigned long input;
    int ret;

    ret = kstrtoul(buf, 0, &input);
    if (ret)
        return ret;

    bypass_an_val[1] = input ? 1 : 0;
    set_bit_in_reg(REG_PORT1_AN, AN_BYPASS_BIT, bypass_an_val[1]);
    return count;
}

/* --- sysfs: port_type per port (read-only, set by phy link driver) --- */

static ssize_t port_type_port0_show(struct kobject *kobj,
                                    struct kobj_attribute *attr, char *buf)
{
    return bit_show(REG_PORT0_TYPE, PORT_TYPE_BIT, buf);
}

static ssize_t port_type_port1_show(struct kobject *kobj,
                                    struct kobj_attribute *attr, char *buf)
{
    return bit_show(REG_PORT1_TYPE, PORT_TYPE_BIT, buf);
}

static struct kobj_attribute bypass_an_port0_attr = __ATTR_RW(bypass_an_port0);
static struct kobj_attribute bypass_an_port1_attr = __ATTR_RW(bypass_an_port1);
static struct kobj_attribute port_type_port0_attr = __ATTR_RO(port_type_port0);
static struct kobj_attribute port_type_port1_attr = __ATTR_RO(port_type_port1);

static struct attribute *cn9130_attrs[] = {
    &bypass_an_port0_attr.attr,
    &bypass_an_port1_attr.attr,
    &port_type_port0_attr.attr,
    &port_type_port1_attr.attr,
    NULL,
};

static const struct attribute_group cn9130_attr_group = {
    .attrs = cn9130_attrs,
};

static int __init cn9130_led_init(void)
{
    int i, ret;
    const int count = ARRAY_SIZE(writes);

    pr_info(DRV_NAME ": starting hardcoded MMIO writes (%d entries)\n", count);

    for (i = 0; i < count; i++) {
        u64 phys = writes[i].phys;
        u32 val  = writes[i].val;
        u32 rb;

        if (mmio_write32(phys, val)) {
            pr_err(DRV_NAME ": write failed for phys=0x%llx\n",
                   (unsigned long long)phys);
            continue;
        }
        if (mmio_read32(phys, &rb) == 0)
            pr_info(DRV_NAME ": wrote 0x%08X to 0x%llx, readback=0x%08X\n",
                    val, (unsigned long long)phys, rb);
    }

    pr_info(DRV_NAME ": all writes complete\n");

    cn9130_kobj = kobject_create_and_add(DRV_NAME, kernel_kobj);
    if (!cn9130_kobj) {
        pr_err(DRV_NAME ": failed to create kobject\n");
        return -ENOMEM;
    }

    ret = sysfs_create_group(cn9130_kobj, &cn9130_attr_group);
    if (ret) {
        pr_err(DRV_NAME ": failed to create sysfs group\n");
        kobject_put(cn9130_kobj);
        return ret;
    }

    poll_thread = kthread_run(cn9130_poll_fn, NULL, DRV_NAME "_poll");
    if (IS_ERR(poll_thread)) {
        pr_err(DRV_NAME ": failed to create poll thread\n");
        sysfs_remove_group(cn9130_kobj, &cn9130_attr_group);
        kobject_put(cn9130_kobj);
        return PTR_ERR(poll_thread);
    }

    pr_info(DRV_NAME ": poll thread started\n");
    return 0;
}

static void __exit cn9130_led_exit(void)
{
    kthread_stop(poll_thread);
    sysfs_remove_group(cn9130_kobj, &cn9130_attr_group);
    kobject_put(cn9130_kobj);
    pr_info(DRV_NAME ": unloaded\n");
}

module_init(cn9130_led_init);
module_exit(cn9130_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("NOKIA");
MODULE_DESCRIPTION("LED driver for CN9130");
