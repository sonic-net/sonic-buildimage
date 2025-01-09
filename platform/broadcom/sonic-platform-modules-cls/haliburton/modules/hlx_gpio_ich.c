/* Copyright (c) 2018 Dell Inc.
 * gpio_ich.c - ICH driver for Rangeley switches
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
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/pci.h>
#include <linux/gpio.h>
#include <linux/mfd/lpc_ich.h>
#include <linux/acpi.h>

#define DRV_NAME "hlx-ich"

#define C2000_PCU_DEVICE_ID 0x1f38

// GPIO registers
// GPIO Core Control/Access Registers in I/O Space
#define GPIO_SC_USE_SEL		0x0
#define GPIO_SC_IO_SEL		0x4
#define GPIO_SC_GP_LVL		0x8
#define GPIO_SC_TPE			0xC
#define GPIO_SC_TNE			0x10
#define GPIO_SC_TS			0x14

// GPIO SUS Control/Access Registers in I/O Space
#define GPIO_SUS_USE_SEL	0x80
#define GPIO_SUS_IO_SEL		0x84
#define GPIO_SUS_GP_LVL		0x88
#define GPIO_SUS_TPE		0x8C
#define GPIO_SUS_TNE		0x90
#define GPIO_SUS_TS			0x94
#define GPIO_SUS_WAKE_EN	0x98

static const unsigned char c2000_gpio_regs[] = {
  GPIO_SC_USE_SEL,
  GPIO_SC_IO_SEL,
  GPIO_SC_GP_LVL,
  GPIO_SC_TPE,
  GPIO_SC_TNE,
  GPIO_SC_TS,
  GPIO_SUS_USE_SEL,
  GPIO_SUS_IO_SEL,
  GPIO_SUS_GP_LVL,
  GPIO_SUS_TPE,
  GPIO_SUS_TNE,
  GPIO_SUS_TS,
  GPIO_SUS_WAKE_EN
};

#define GPIO_REG_LEN 0x4

#define GPIOS0_EN  (1 << 0)
#define GPIOS1_EN  (1 << 1)
#define GPIOS2_EN  (1 << 2)
#define GPIOS3_EN  (1 << 3)
#define GPIOS4_EN  (1 << 4)
#define GPIOS5_EN  (1 << 5)
#define GPIOS6_EN  (1 << 6)
#define GPIOS7_EN  (1 << 7)

#define GPIOSUS0_EN  (1 << 0)
#define GPIOSUS1_EN  (1 << 1)
#define GPIOSUS2_EN  (1 << 2)
#define GPIOSUS3_EN  (1 << 3)
// GPIOSUS4_EN : unused
// GPIOSUS5_EN : unused
#define GPIOSUS6_EN  (1 << 6)
#define GPIOSUS7_EN  (1 << 7)


// GPE0a_EN - General Purpose Event 0 Enables
#define GPIO_GPE0a_EN_SUS0	(1 << 16)
#define GPIO_GPE0a_EN_SUS1	(1 << 17)
#define GPIO_GPE0a_EN_SUS2	(1 << 18)
#define GPIO_GPE0a_EN_SUS3	(1 << 19)
// GPIO_GPE0a_EN_SUS4 : unused
// GPIO_GPE0a_EN_SUS5 : unused
#define GPIO_GPE0a_EN_SUS6	(1 << 22)
#define GPIO_GPE0a_EN_SUS7	(1 << 23)

#define GPIO_GPE0a_EN_CORE0	(1 << 24)
#define GPIO_GPE0a_EN_CORE1	(1 << 25)
#define GPIO_GPE0a_EN_CORE2	(1 << 26)
#define GPIO_GPE0a_EN_CORE3	(1 << 27)
#define GPIO_GPE0a_EN_CORE4	(1 << 28)
#define GPIO_GPE0a_EN_CORE5	(1 << 29)
#define GPIO_GPE0a_EN_CORE6	(1 << 30)
#define GPIO_GPE0a_EN_CORE7	(1 << 31)

// GPE0a_STS - General Purpose Event 0 Status
// We're interested in only SUS6 for now
#define GPIO_GPE0a_STS_SUS6	(1 << 22)
#define GPIO_GPE0a_STS_SUS7	(1 << 23)

// GPIO_ROUT - GPIO_ROUT Register
#define GPIO_ROUT_OFFSET_SUS0	0
#define GPIO_ROUT_OFFSET_SUS1	2
#define GPIO_ROUT_OFFSET_SUS2	4
#define GPIO_ROUT_OFFSET_SUS3	6
// GPIO_ROUT_OFFSET_SUS4 : unused
// GPIO_ROUT_OFFSET_SUS5 : unused
#define GPIO_ROUT_OFFSET_SUS6	12
#define GPIO_ROUT_OFFSET_SUS7	14

#define GPIO_ROUT_OFFSET_CORE0   16
#define GPIO_ROUT_OFFSET_CORE1   18
#define GPIO_ROUT_OFFSET_CORE2   20
#define GPIO_ROUT_OFFSET_CORE3   22
#define GPIO_ROUT_OFFSET_CORE4   24
#define GPIO_ROUT_OFFSET_CORE5   26
#define GPIO_ROUT_OFFSET_CORE6   28
#define GPIO_ROUT_OFFSET_CORE7   30

enum GPIO_ROUT {
	GPIO_NO_EFFECT = 0,
	GPIO_SMI,
	GPIO_SCI,
	GPIO_RESERVED
};

/*
 * GPIO resources
 * defined as in drivers/gpio/gpio-ich.c
 */
#define ICH_RES_GPIO    0
#define ICH_RES_GPE0    1
static struct resource gpio_ich_res[] = {
        /* GPIO */
        {
                .flags = IORESOURCE_IO,
        },
        /* ACPI - GPE0 */
        {
                .flags = IORESOURCE_IO,
        },
};

// ACPI registers
#define ACPI_GPE0a_STS 0x20
#define ACPI_GPE0a_EN  0x28

// PMC registers
#define PMC_GPIO_ROUT 0x58
#define PMC_REG_LEN   0x4

// lpc_ich_priv is derived from drivers/mfd/lpc_ich.c
struct lpc_ich_priv {
	int chipset;

	int abase;		/* ACPI base */
	int actrl_pbase;	/* ACPI control or PMC base */
	int gbase;		/* GPIO base */
	int gctrl;		/* GPIO control */

	int abase_save;		/* Cached ACPI base value */
	int actrl_pbase_save;		/* Cached ACPI control or PMC base value */
	int gctrl_save;		/* Cached GPIO control value */
};

#define ICH_RES_MEM_GCS_PMC	0

#define IO_REG_WRITE(val, reg, base_res)	outl(val, (reg) + (base_res)->start)
#define IO_REG_READ(reg, base_res)		inl((reg) + (base_res)->start)

struct resource pmc_res = {.flags = IORESOURCE_MEM,};

static struct kobject *hlx_kobj;
static unsigned short force_id;
module_param(force_id, ushort, 0);

#define SMF_REG_ADDR            0x200
#define SIO_REG_DEVID           0x1
#define SIO_Z9100_ID            0x1
#define SIO_S6100_ID            0x2
#define SIO_S4200_ID            0x3
#define SIO_S5148_ID            0x4
#define SIO_E1031_ID            0x5a

enum kinds {
        z9100smf, s6100smf, e1031
};

struct hlx_ich_data {
    enum kinds kind;
    struct resource *gpio_base, *acpi_base, *pmc_base;
    int gpio_alloc,pmc_alloc;
    unsigned int int_gpio_sus7_count;
};

// GPIO sysfs attributes

static ssize_t get_sc_use_sel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_USE_SEL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_use_sel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_USE_SEL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sc_io_sel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_IO_SEL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_io_sel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SC_IO_SEL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sc_gp_lvl(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_GP_LVL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_gp_lvl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SC_GP_LVL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sc_gp_tpe(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_TPE,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_gp_tpe(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SC_TPE,ich_data->gpio_base);

    return count;
}

static ssize_t get_sc_gp_tne(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_TNE,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_gp_tne(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SC_TNE,ich_data->gpio_base);

    return count;
}

static ssize_t get_sc_gp_ts(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SC_TS,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sc_gp_ts(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SC_TS,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_use_sel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_USE_SEL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_use_sel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_USE_SEL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_io_sel(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_IO_SEL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_io_sel(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_IO_SEL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_gp_lvl(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_GP_LVL,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_gp_lvl(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_GP_LVL,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_gp_tpe(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_TPE,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_gp_tpe(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_TPE,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_gp_tne(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_TNE,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_gp_tne(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_TNE,ich_data->gpio_base);

    return count;
}

static ssize_t get_sus_gp_ts(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(GPIO_SUS_TS,ich_data->gpio_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sus_gp_ts(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,GPIO_SUS_TS,ich_data->gpio_base);

    return count;
}

static DEVICE_ATTR(sc_use_sel,	S_IRUGO | S_IWUSR, get_sc_use_sel, set_sc_use_sel);
static DEVICE_ATTR(sc_io_sel,	S_IRUGO | S_IWUSR, get_sc_io_sel,  set_sc_io_sel);
static DEVICE_ATTR(sc_gp_lvl,	S_IRUGO | S_IWUSR, get_sc_gp_lvl,  set_sc_gp_lvl);
static DEVICE_ATTR(sc_gp_tpe,   S_IRUGO | S_IWUSR, get_sc_gp_tpe,  set_sc_gp_tpe);
static DEVICE_ATTR(sc_gp_tne,   S_IRUGO | S_IWUSR, get_sc_gp_tne,  set_sc_gp_tne);
static DEVICE_ATTR(sc_gp_ts,    S_IRUGO | S_IWUSR, get_sc_gp_ts,   set_sc_gp_ts);
static DEVICE_ATTR(sus_use_sel,	S_IRUGO | S_IWUSR, get_sus_use_sel,set_sus_use_sel);
static DEVICE_ATTR(sus_io_sel,	S_IRUGO | S_IWUSR, get_sus_io_sel, set_sus_io_sel);
static DEVICE_ATTR(sus_gp_lvl,	S_IRUGO | S_IWUSR, get_sus_gp_lvl, set_sus_gp_lvl);
static DEVICE_ATTR(sus_gp_tpe,	S_IRUGO | S_IWUSR, get_sus_gp_tpe, set_sus_gp_tpe);
static DEVICE_ATTR(sus_gp_tne,	S_IRUGO | S_IWUSR, get_sus_gp_tne, set_sus_gp_tne);
static DEVICE_ATTR(sus_gp_ts,	S_IRUGO | S_IWUSR, get_sus_gp_ts,  set_sus_gp_ts);

static struct attribute *gpio_attrs[] = {
    &dev_attr_sc_use_sel.attr,
    &dev_attr_sc_io_sel.attr,
    &dev_attr_sc_gp_lvl.attr,
    &dev_attr_sc_gp_tpe.attr,
    &dev_attr_sc_gp_tne.attr,
    &dev_attr_sc_gp_ts.attr,
    &dev_attr_sus_use_sel.attr,
    &dev_attr_sus_io_sel.attr,
    &dev_attr_sus_gp_lvl.attr,
    &dev_attr_sus_gp_tpe.attr,
    &dev_attr_sus_gp_tne.attr,
    &dev_attr_sus_gp_ts.attr,
    NULL,
};

static struct attribute_group gpio_attrs_group= {
    .attrs = gpio_attrs,
};

// ACPI sysfs attributes

static ssize_t get_gpe0a_sts(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(ACPI_GPE0a_STS,ich_data->acpi_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_gpe0a_sts(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,ACPI_GPE0a_STS,ich_data->acpi_base);

    return count;
}

static ssize_t get_gpe0a_en(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = IO_REG_READ(ACPI_GPE0a_EN,ich_data->acpi_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_gpe0a_en(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    IO_REG_WRITE(devdata,ACPI_GPE0a_EN,ich_data->acpi_base);

    return count;
}

static DEVICE_ATTR(gpe0a_sts,  S_IRUGO | S_IWUSR, get_gpe0a_sts, set_gpe0a_sts);
static DEVICE_ATTR(gpe0a_en,   S_IRUGO | S_IWUSR, get_gpe0a_en,  set_gpe0a_en);

static struct attribute *acpi_attrs[] = {
    &dev_attr_gpe0a_sts.attr,
    &dev_attr_gpe0a_en.attr,
    NULL,
};

static struct attribute_group acpi_attrs_group= {
    .attrs = acpi_attrs,
};

static ssize_t get_gpio_rout(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = readl(ich_data->pmc_base);
    return sprintf(buf,"0x%08x\n",devdata);
}

// PMC sysfs attributes

static ssize_t set_gpio_rout(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    writel(devdata,ich_data->pmc_base);

    return count;
}

static DEVICE_ATTR(gpio_rout,  S_IRUGO | S_IWUSR, get_gpio_rout, set_gpio_rout);

static struct attribute *pmc_attrs[] = {
    &dev_attr_gpio_rout.attr,
    NULL,
};

static struct attribute_group pmc_attrs_group= {
    .attrs = pmc_attrs,
};

// SCI interrupt sysfs attributes

static ssize_t get_sci_int_gpio_sus7(struct device *dev, struct device_attribute *devattr, char *buf)
{
    u32 devdata=0;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!ich_data) return sprintf(buf, "read error");

    devdata = ich_data->int_gpio_sus7_count;
    return sprintf(buf,"0x%08x\n",devdata);
}

static ssize_t set_sci_int_gpio_sus7(struct device *dev, struct device_attribute *devattr, const char *buf, size_t count)
{
    unsigned long devdata;
    int err;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    err = kstrtoul(buf, 16, &devdata);
    if (err)
        return err;

    ich_data->int_gpio_sus7_count = devdata;

    return count;
}

static DEVICE_ATTR(sci_int_gpio_sus7,  S_IRUGO | S_IWUSR, get_sci_int_gpio_sus7, set_sci_int_gpio_sus7);

static struct attribute *sci_attrs[] = {
    &dev_attr_sci_int_gpio_sus7.attr,
    NULL,
};

static struct attribute_group sci_attrs_group= {
    .attrs = sci_attrs,
};

static u32 hlx_ich_sci_handler(void *context)
{
    unsigned int data;
    struct device *dev = (struct device *)context;
    struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    if(!dev) return ACPI_INTERRUPT_NOT_HANDLED;

    ich_data = dev_get_platdata(dev);
    if(!ich_data) return ACPI_INTERRUPT_NOT_HANDLED;

    data=IO_REG_READ(ACPI_GPE0a_STS,ich_data->acpi_base);
    if(data & GPIO_GPE0a_STS_SUS7) {
	// Clear the SUS7 status
        IO_REG_WRITE(data,ACPI_GPE0a_STS,ich_data->acpi_base);
        ich_data->int_gpio_sus7_count++;
	// and notify the user space clients
	sysfs_notify(&dev->kobj, NULL, "sci_int_gpio_sus7");
	return ACPI_INTERRUPT_HANDLED;
    }

    return ACPI_INTERRUPT_NOT_HANDLED;
}

/*
 * Setup GPIO SUS7 to generate an SCI interrupt for optics detection
 * This can be alternatively be setup using sysfs
 */
int setup_gpio_sus7_sci_interrupt(struct device *dev)
{
  int ret=0;
  unsigned int data;
  struct resource *acpi_base, *pmc_base, *gpio_base;
  struct hlx_ich_data *ich_data = dev_get_platdata(dev);

    gpio_base = ich_data->gpio_base;
    acpi_base = ich_data->acpi_base;
    pmc_base  = ich_data->pmc_base;

    // Enable GPIOSUS7_EN
    data = IO_REG_READ(GPIO_SUS_USE_SEL,gpio_base);
    data |= GPIOSUS7_EN;
    IO_REG_WRITE(data,GPIO_SUS_USE_SEL,gpio_base);

    // GPIOSUS7_EN is input
    data = IO_REG_READ(GPIO_SUS_IO_SEL,gpio_base);
    data |= GPIOSUS7_EN;
    IO_REG_WRITE(data,GPIO_SUS_IO_SEL,gpio_base);

    // Trigger on positive edge for GPIOSUS7_EN
    data = IO_REG_READ(GPIO_SUS_TPE,gpio_base);
    data |= GPIOSUS7_EN;
    IO_REG_WRITE(data,GPIO_SUS_TPE,gpio_base);

    // Enable GPE for SUS7 to generate an SCI
    data=IO_REG_READ(ACPI_GPE0a_EN,acpi_base);
    data|=GPIO_GPE0a_EN_SUS7;
    IO_REG_WRITE(data,ACPI_GPE0a_EN,acpi_base);

    data=readl(pmc_base);
    data=(data & ~(0x3 << GPIO_ROUT_OFFSET_SUS7)) | (GPIO_SCI << GPIO_ROUT_OFFSET_SUS7);
    writel(data,pmc_base);

    ret = acpi_install_sci_handler(hlx_ich_sci_handler,(void*)dev);
    if(ret) {
        pr_info("hlx_ich acpi_install_sci_handler failed %d\n",ret);
        return ret;
    }

    return ret;
}

static int hlx_ich_probe(struct platform_device *pdev)
{
  struct device *dev = &pdev->dev;
  struct hlx_ich_data *ich_data = dev_get_platdata(dev);
  struct pci_dev *lpc_ich_dev;
  struct lpc_ich_priv *priv;
  struct resource *res;
  unsigned int base_addr_cfg, base_addr;
  int ret,i;

	// Get the PCU device
        lpc_ich_dev=pci_get_device(PCI_VENDOR_ID_INTEL,C2000_PCU_DEVICE_ID,NULL);
        priv=(struct lpc_ich_priv*) pci_get_drvdata(lpc_ich_dev);
        if(!priv) {
		pr_info("hlx_ich: Unable to retrieve private data\n");
		return -ENODEV;
        }

	// Retrieve the GPIO Base (that was initialized by lpc-ich)
        pci_read_config_dword(lpc_ich_dev, priv->gbase, &base_addr_cfg);
        base_addr = base_addr_cfg & 0x0000ff80;
        if (!base_addr) {
                pr_info("hlx_ich I/O space for GPIO uninitialized\n");
                ret = -ENODEV;
                goto probe_err;
        }

	res = &gpio_ich_res[ICH_RES_GPIO];
        res->start = base_addr;
        res->end = res->start + 0x9c - 1;
        ret = acpi_check_resource_conflict(res);
        if (ret) {
            pr_info("hlx_ich gpio resource conflict ret %d\n",ret);
        }

        ich_data->gpio_base=res;
	// Request regions for GPIO registers
        for(i=0; i<ARRAY_SIZE(c2000_gpio_regs); i++) {
		if (!request_region(res->start+c2000_gpio_regs[i],GPIO_REG_LEN, "hlx_ich_gpio")) {
			pr_info("hlx_ich: request_region failed for GPIO : %x\n",(unsigned int) res->start+c2000_gpio_regs[i]);
			ret = -EBUSY;
			goto probe_err;
		}
                ich_data->gpio_alloc |= (1<<i);
	}

	/* Register sysfs hooks for gpio */
	ret = sysfs_create_group(&dev->kobj, &gpio_attrs_group);
	if (ret) {
		pr_info("hlx_ich cannot create sysfs for GPIO %d\n",ret);
                ret = -ENOMEM;
                goto probe_err;
	}

	// Retrieve the ACPI Base (that was initialized by lpc-ich)
	pci_read_config_dword(lpc_ich_dev, priv->abase, &base_addr_cfg);
	base_addr = base_addr_cfg & 0x0000ff80;
	if (!base_addr) {
		pr_info("hlx_ich I/O space for ACPI uninitialized\n");
		ret = -ENODEV;
		goto probe_err;
	}

	res = &gpio_ich_res[ICH_RES_GPE0];
	res->start = base_addr;
	res->end = base_addr + 0x40;
	ret = acpi_check_resource_conflict(res);
	if (ret) {
            pr_info("hlx_ich acpi resource conflict ret %d\n",ret);
        }

        // ACPI region is requested by pnp 00:01/ACPI GPE0_BLK
        ich_data->acpi_base=res;

        /* Register sysfs hooks for ACPI */
        ret = sysfs_create_group(&dev->kobj, &acpi_attrs_group);
        if (ret) {
                pr_info("hlx_ich cannot create sysfs for ACPI %d\n",ret);
                ret = -ENOMEM;
                goto probe_err;
        }

	// Retrieve the PMC Base (that was initialized by lpc-ich)
	pci_read_config_dword(lpc_ich_dev, priv->actrl_pbase, &base_addr_cfg);
	base_addr = base_addr_cfg & 0xfffffe00;
        pr_info("hlx_ich base_addr_cfg %x base_addr %x\n",(int)base_addr_cfg,(int)base_addr);
        if (!base_addr) {
                pr_info("hlx_ich PMC space for GPIO uninitialized\n");
                ret = -ENODEV;
                goto probe_err;
        }

        res =  &pmc_res;
        res->start = base_addr + PMC_GPIO_ROUT;
        res->end = base_addr + PMC_GPIO_ROUT + PMC_REG_LEN - 1;
        pr_info("hlx_ich pmc res_start:end %x:%x\n",(int)res->start,(int)res->end);

        ret = acpi_check_resource_conflict(res);
        if (ret) {
            pr_info("hlx_ich acpi resource conflict ret %d\n",ret);
        }

        if (!request_mem_region(res->start,resource_size(res),"hlx_ich_pmc")) {
		pr_info("hlx_ich pmc request_region failed\n");
		ret = -EBUSY;
		goto probe_err;
	} else {
		ich_data->pmc_alloc=1;
	}

	ich_data->pmc_base = ioremap(res->start, resource_size(res));
        if(!ich_data->pmc_base) {
		pr_info("hlx_ich pmc ioremap failed\n");
		ret = -ENOMEM;
		goto probe_err;
        }

        /* Register sysfs hooks for pmc */
        ret = sysfs_create_group(&dev->kobj, &pmc_attrs_group);
        if (ret) {
                pr_info("hlx_ich cannot create sysfs for PMC %d\n",ret);
                ret = -ENOMEM;
                goto probe_err;
        }

        /* Register sysfs hooks for SCI interrupts*/
        ret = sysfs_create_group(&dev->kobj, &sci_attrs_group);
        if (ret) {
                pr_info("hlx_ich cannot create sysfs for SCI %d\n",ret);
                ret = -ENOMEM;
                goto probe_err;
        }

        if((ich_data->kind == z9100smf) || (ich_data->kind == s6100smf) || (ich_data->kind == e1031)) {
	    ret = setup_gpio_sus7_sci_interrupt(dev);
            if (ret) {
                pr_info("hlx_ich unable to setup SCI interrupt %d\n",ret);
                goto probe_err;
            }
        }

	return 0;

probe_err:
        pr_info("hlx_ich hlx_ich_probe failed with : %d\n",ret);
	return ret;
}

static int hlx_ich_remove(struct platform_device *pdev)
{
  struct device *dev = &pdev->dev;
  struct hlx_ich_data *ich_data = dev_get_platdata(dev);
  int i,ret;

    // Release GPIO regions
    for(i=0; i<ARRAY_SIZE(c2000_gpio_regs); i++) {
        if(ich_data->gpio_alloc & (1<<i)) {
            release_region(ich_data->gpio_base->start+c2000_gpio_regs[i], GPIO_REG_LEN);
        }
    }

    // Unmap and release PMC regions
    if(ich_data->pmc_base) iounmap(ich_data->pmc_base);
    if(ich_data->pmc_alloc) release_region(pmc_res.start, PMC_REG_LEN);

    ret = acpi_remove_sci_handler(hlx_ich_sci_handler);
    if(ret) {
        pr_info("hlx_ich acpi_remove_sci_handler failed %d\n",ret);
        return ret;
    }

    pr_info("hlx_ich : hlx_ich_remove done.\n");

    return 0;
}

static struct platform_driver hlx_ich_driver= {
	.driver		= {
		.name	= DRV_NAME,
	},
	.probe		= hlx_ich_probe,
	.remove		= hlx_ich_remove,
};

int __init
init_ich_data(int smf_address, struct hlx_ich_data *ich_data)
{
        int val;

	memset(ich_data, 0, sizeof(struct hlx_ich_data));

        if (force_id)
                val = force_id;
        else
                val = inb(smf_address + SIO_REG_DEVID);

        switch (val) {
                case SIO_Z9100_ID:
                        ich_data->kind = z9100smf;
                        break;
                case SIO_S6100_ID:
                        ich_data->kind = s6100smf;
                        break;
                case SIO_E1031_ID:
			ich_data->kind = e1031;
                        break;
                default:
                        if (val != 0xffff)
                                pr_debug("unsupported chip ID: 0x%04x\n", val);
                        return -ENODEV;
        }

        pr_info("hlx_ich: found SMF for ID %#x\n", ich_data->kind);

        return (0);
}

static struct platform_device *pdev;

static int __init hlx_ich_init(void)
{
        int err;
        struct hlx_ich_data ich_data;

        if (init_ich_data(SMF_REG_ADDR, &ich_data))
                return -ENODEV;

        err = platform_driver_register(&hlx_ich_driver);
        if (err)
                goto exit;

        pdev = platform_device_alloc(DRV_NAME, 0);
        if (!pdev) {
                err = -ENOMEM;
                pr_err("hlx_ich: Device allocation failed\n");
                goto exit_unregister;
        }

        err = platform_device_add_data(pdev, &ich_data,
                        sizeof(struct hlx_ich_data));
        if (err) {
                pr_err("hlx_ich: Platform data allocation failed\n");
                goto exit_device_put;
        }

        /* platform_device_add calls probe() */
        err = platform_device_add(pdev);
        if (err) {
                pr_err("hlx_ich: Device addition failed (%d)\n", err);
                goto exit_device_put;
        }

        return 0;

exit_device_put:
        platform_device_put(pdev);
exit_unregister:
        platform_driver_unregister(&hlx_ich_driver);
exit:
	pr_err("hlx_ich: hlx_ich_init failed (%d)\n", err);
        return err;
}

static void __exit hlx_ich_exit(void)
{
        platform_device_unregister(pdev);
        platform_driver_unregister(&hlx_ich_driver);

        /*Remove sysfs hlx_kobj*/
        kobject_put(hlx_kobj);
}

MODULE_AUTHOR("Padmanabhan Narayanan");
MODULE_DESCRIPTION("ICH driver for Rangeley switches");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:"DRV_NAME);
MODULE_PARM_DESC(force_id, "Override the detected device ID");

module_init(hlx_ich_init);
module_exit(hlx_ich_exit);
