#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_cpldmux_defs.h"

extern PDDF_CPLDMUX_OPS pddf_cpldmux_ops;

int pddf_cpldmux_select(struct i2c_mux_core *muxc, uint32_t chan);
int pddf_cpldmux_deselect(struct i2c_mux_core *muxc, uint32_t chan);

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

int pddf_cpldmux_select(struct i2c_mux_core *muxc, uint32_t chan)
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

    if ( (pdata->chan_cache!=1) || (private->last_chan!=chan) )
    {
        sdata = &pdata->chan_data[chan];
        // pddf_dbg(CPLDMUX, KERN_INFO "%s: Writing 0x%x at 0x%x offset of cpld 0x%x to enable chan %d\n", __FUNCTION__, sdata->cpld_sel, sdata->cpld_offset, sdata->cpld_devaddr, chan);
        ret =  cpldmux_byte_write(pdata->cpld, sdata->cpld_offset,  (uint8_t)(sdata->cpld_sel & 0xff));
        private->last_chan = chan;
    }

    return ret;
}

int pddf_cpldmux_deselect(struct i2c_mux_core *muxc, uint32_t chan)
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

    // pddf_dbg(CPLDMUX, KERN_INFO "%s: Writing 0x%x at 0x%x offset of cpld 0x%x to disable chan %d", __FUNCTION__, sdata->cpld_desel, sdata->cpld_offset, sdata->cpld_devaddr, chan);
    ret = cpldmux_byte_write(pdata->cpld, sdata->cpld_offset,  (uint8_t)(sdata->cpld_desel));
    return ret;
}

static int __init pddf_custom_cpldmux_init(void)
{
    pddf_cpldmux_ops.select = pddf_cpldmux_select;
    pddf_cpldmux_ops.deselect = pddf_cpldmux_deselect;
    return 0;
}

static void __exit pddf_custom_cpldmux_exit(void)
{
    return;
}

MODULE_AUTHOR("Nonodark Huang");
MODULE_DESCRIPTION("custom board_i2c_cpldmux driver");
MODULE_LICENSE("GPL");

module_init(pddf_custom_cpldmux_init);
module_exit(pddf_custom_cpldmux_exit);
