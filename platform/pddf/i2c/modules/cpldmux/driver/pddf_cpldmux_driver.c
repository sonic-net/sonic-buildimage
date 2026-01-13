/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 * A pddf kernel driver module for CPLDMUX
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include <linux/i2c-mux.h>
#include <linux/platform_device.h>
#include "pddf_client_defs.h"
#include "pddf_cpldmux_defs.h"
#include "pddf_multifpgapci_defs.h"

extern PDDF_CPLDMUX_DATA pddf_cpldmux_data;

/* Users may overwrite these select and delsect functions as per their requirements
 * by overwriting them in custom driver.
 *
 * Example driver skeleton:
 *
 * #include <linux/module.h>
 * #include <linux/i2c.h>
 * ..
 * ..
 * #include "pddf_cpldmux_defs.h"
 *
 * extern PDDF_CPLDMUX_OPS pddf_cpldmux_ops;
 *
 * int cpldmux_byte_write(struct i2c_client *client, u8 regaddr, u8 val)
 * {
 *     // Provide custom implementation here
 * }
 *
 * int pddf_cpldmux_deselect(struct i2c_mux_core *muxc, uint32_t chan)
 * {
 *     // Provide the custom implementation here
 * }
 *
 * static int __init pddf_custom_cpldmux_init(void)
 * {
 *     pddf_cpldmux_ops.deselect = pddf_cpldmux_deselect;
 *     pddf_cpldmux_ops.byte_write = cpldmux_byte_write;
 *     return 0;
 * }
 */

PDDF_CPLDMUX_OPS pddf_cpldmux_ops = {
    .select = pddf_cpldmux_select_default,
    .deselect = pddf_cpldmux_deselect_default,
};
EXPORT_SYMBOL(pddf_cpldmux_ops);


/* NOTE: Never use i2c_smbus_write_byte_data() or i2c_smbus_xfer() since these operations
 * locks the parent bus which might lead to mutex deadlock.
 */
static int cpldmux_byte_write(struct i2c_client *client, u8 regaddr, u8 val)
{
    union i2c_smbus_data data;

    data.byte = val;
    return client->adapter->algo->smbus_xfer(client->adapter, client->addr,
                                             client->flags,
                                             I2C_SMBUS_WRITE,
                                             regaddr, I2C_SMBUS_BYTE_DATA, &data);
}

int pddf_cpldmux_select_default(struct i2c_mux_core *muxc, uint32_t chan)
{
    PDDF_CPLDMUX_PRIV_DATA  *private = i2c_mux_priv(muxc);
    PDDF_CPLDMUX_PDATA *pdata = NULL;
    PDDF_CPLDMUX_CHAN_DATA *sdata = NULL;
    int ret = 0;

    /* Select the chan_data based upon the chan */
    pdata = &private->data;
    if (chan>=pdata->num_chan)
    {
        printk(KERN_ERR "%s: wrong channel number %d, supported channels %d\n",__FUNCTION__, chan, pdata->num_chan);
        return 0;
    }

    if ((pdata->chan_cache != 1) || (private->last_chan != chan)) {
        sdata = &pdata->chan_data[chan];
        switch (pdata->dev_type) {
        case CPLD_MUX:
            // cpld_mux
            pddf_dbg(
                CPLDMUX,
                KERN_INFO
                "%s: Writing 0x%x at 0x%x offset of cpld 0x%x to enable chan %d\n",
                __FUNCTION__, sdata->cpld_sel,
                sdata->cpld_offset, sdata->cpld_devaddr, chan);
            ret = cpldmux_byte_write(
                pdata->cpld, sdata->cpld_offset,
                (uint8_t)(sdata->cpld_sel & 0xff));
            break;
        case MULTIFPGAPCI_MUX:
            ret = ptr_multifpgapci_writepci(
                pdata->fpga_pci_dev,
                sdata->cpld_sel,
                sdata->cpld_offset);
            break;
        default:
            printk(KERN_ERR "%s: Unexpected device type %d\n",
                   __FUNCTION__, pdata->dev_type);
            break;
        }
        private->last_chan = chan;
    }

    if (ret) {
        printk(KERN_ERR "%s: Error status = %d", __FUNCTION__, ret);
    }

    return ret;
}

int pddf_cpldmux_deselect_default(struct i2c_mux_core *muxc, uint32_t chan)
{
    PDDF_CPLDMUX_PRIV_DATA  *private = i2c_mux_priv(muxc);
    PDDF_CPLDMUX_PDATA *pdata = NULL;
    PDDF_CPLDMUX_CHAN_DATA *sdata = NULL;
    int ret = 0;

    /* Select the chan_data based upon the chan */
    pdata = &private->data;
    if (chan>=pdata->num_chan)
    {
        printk(KERN_ERR "%s: wrong channel number %d, supported channels %d\n",__FUNCTION__, chan, pdata->num_chan);
        return 0;
    }
    sdata = &pdata->chan_data[chan];
    switch (pdata->dev_type) {
    case CPLD_MUX:
        pddf_dbg(
            CPLDMUX,
            KERN_INFO
            "%s: Writing 0x%x at 0x%x offset of cpld 0x%x to disable chan %d",
            __FUNCTION__, sdata->cpld_desel, sdata->cpld_offset,
            sdata->cpld_devaddr, chan);
        ret = cpldmux_byte_write(pdata->cpld, sdata->cpld_offset,
                     (uint8_t)(sdata->cpld_desel));
        break;
    case MULTIFPGAPCI_MUX:
        ret = ptr_multifpgapci_writepci(
            pdata->fpga_pci_dev,
            sdata->cpld_desel,
            sdata->cpld_offset);
        break;
    default:
        printk(KERN_ERR "%s: Unexpected device type %d\n", __FUNCTION__,
               pdata->dev_type);
        break;
    }

    if (ret) {
        printk(KERN_ERR "%s: Error status = %d", __FUNCTION__, ret);
    }

    return ret;
}

static int cpld_mux_probe(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc;
    PDDF_CPLDMUX_PRIV_DATA *private;
    PDDF_CPLDMUX_PDATA *pdata;
    struct i2c_adapter *adap;
    int i, ret, ndev;

    pdata = pdev->dev.platform_data;
    if (!pdata) {
        dev_err(&pdev->dev, "CPLDMUX platform data not found\n");
        return -ENODEV;
    }
    private = (PDDF_CPLDMUX_PRIV_DATA *)kzalloc(sizeof(PDDF_CPLDMUX_PRIV_DATA) , GFP_KERNEL);
    if (!private) {
        printk(KERN_ERR "Failed to allocate memory for priv data\n");
        return -ENOMEM;
    }
    
    private->last_chan = 0xff; /*Giving imaginary high value so that proper channel is selected at first iteration*/
    memcpy(&private->data, pdata, sizeof(PDDF_CPLDMUX_PDATA));
    
    adap = i2c_get_adapter(pdata->parent_bus);
    if (!adap) {
        kfree(private);
        dev_err(&pdev->dev, "Parent adapter (%d) not found\n", pdata->parent_bus);
        return -ENODEV;
    }
    ndev = pdata->num_chan;

    muxc = i2c_mux_alloc(adap, &pdev->dev, ndev, 0, 0, pddf_cpldmux_ops.select, pddf_cpldmux_ops.deselect);
    if (!muxc) {
        ret = -ENOMEM;
        goto alloc_failed;
    }
    muxc->priv = private;
    platform_set_drvdata(pdev, muxc);

    for (i = 0; i < ndev; i++)
    {
        int nr = pdata->base_chan + i;
        ret = i2c_mux_add_adapter(muxc, nr, i);
        if (ret) {
            dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
            goto add_adapter_failed;
        }
    }
    dev_info(&pdev->dev, "%d port mux on %s adapter\n", ndev, adap->name);
    return 0;

add_adapter_failed:
    i2c_mux_del_adapters(muxc);
alloc_failed:
    kfree(private);
    i2c_put_adapter(adap);
    return ret;
}

static void cpld_mux_remove(struct platform_device *pdev)
{
    struct i2c_mux_core *muxc  = platform_get_drvdata(pdev);
    struct i2c_adapter *adap = muxc->parent;
    PDDF_CPLDMUX_PDATA *cpldmux_pdata = pdev->dev.platform_data;

    i2c_mux_del_adapters(muxc);
    if (muxc->priv)
        kfree(muxc->priv);
    i2c_put_adapter(adap);

    if (cpldmux_pdata) {
        pddf_dbg(CPLDMUX, KERN_DEBUG "%s: Freeing cpldmux platform data\n", __FUNCTION__);
        kfree(cpldmux_pdata);
    }
}

static const struct platform_device_id mux_ids[] = {
    { "cpld_mux", 0 },
    { "multifpgapci_mux", 0 },
    {},
};

static struct platform_driver cpld_mux_driver = {
    .probe  = cpld_mux_probe,
    .remove = cpld_mux_remove, /* TODO */
    .driver = {
        .owner  = THIS_MODULE,
        .name   = "cpld_mux",
    },
    .id_table = mux_ids,
};

static int __init board_i2c_cpldmux_init(void)
{
    int ret;
    ret = platform_driver_register(&cpld_mux_driver);
    if (ret) {
        printk(KERN_WARNING "Fail to register swpld mux driver\n");
    }
    return (ret);
}

static void __exit board_i2c_cpldmux_exit(void)
{
	platform_driver_unregister(&cpld_mux_driver);
}
	
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("board_i2c_cpldmux driver");
MODULE_LICENSE("GPL");

module_init(board_i2c_cpldmux_init);
module_exit(board_i2c_cpldmux_exit);
