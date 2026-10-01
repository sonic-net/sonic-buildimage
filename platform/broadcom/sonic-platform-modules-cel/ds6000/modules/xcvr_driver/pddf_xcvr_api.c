/*
 * Copyright (C) 2026 Celestica Inc.
 *
 * Based on: PDDF's pddf_xcvr_api.c
 * Description of various APIs related to transciever component
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_xcvr_defs.h"

#ifdef SFP_DEBUG
#define sfp_dbg(...) printk(__VA_ARGS__)
#else
#define sfp_dbg(...)
#endif

extern XCVR_SYSFS_ATTR_OPS xcvr_ops[];
extern void *get_device_table(char *name);
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);

int get_xcvr_module_attr_data(struct i2c_client *client, struct device *dev,
                            struct device_attribute *da);


/**
 * __i2c_atr_smbus_read_byte_data - address translator version of SMBus "read byte" protocol
 *
 * @client: Handle to slave device
 * @dev_addr: The client address use inplace of client->addr
 * @command: Byte interpreted by slave
 *
 * Perform the SMBus "read byte" with the specific client address instead one
 * provided with client.
 */
s32 __i2c_atr_smbus_read_byte_data(const struct i2c_client *client, u16 dev_addr,
                    u8 command)
{
	union i2c_smbus_data data;
	int status;

	status = i2c_smbus_xfer(client->adapter, dev_addr, client->flags,
				I2C_SMBUS_READ, command,
				I2C_SMBUS_BYTE_DATA, &data);
	return (status < 0) ? status : data.byte;
}

/**
 * __i2c_atr_smbus_write_byte_data - address translator version of SMBus "write byte" protocol
 * @client: Handle to slave device
 * @dev_addr: The client address use inplace of client->addr
 * @command: Byte interpreted by slave
 * @value: Byte being written
 *
 * Perform the SMBus "write byte" with the specific client address instead one
 * provided with client.
 */
s32 __i2c_atr_smbus_write_byte_data(const struct i2c_client *client,
                u16 dev_addr,u8 command, u8 value)
{
	union i2c_smbus_data data;
	data.byte = value;
	return i2c_smbus_xfer(client->adapter, dev_addr, client->flags,
			      I2C_SMBUS_WRITE, command,
			      I2C_SMBUS_BYTE_DATA, &data);
}


/**
 * xcvr_i2c_cpld_read - perform the xcvr read from registered i2c client.
 *
 * @client: The registerd CPLD client
 * @info: The xcvr attributes info
 * Return: int 
 */
int xcvr_i2c_cpld_read(struct i2c_client *client, XCVR_ATTR *info)
{
    int status = 0;

    if (info != NULL)
    {
        if (client)
        {
            /* Switchboard CPLD only support single byte */
            if (info->len == 1)
            {
                status = __i2c_atr_smbus_read_byte_data(client, info->devaddr,
                            info->offset);
            }
            else {
                printk(KERN_ERR "PDDF_XCVR: Doesn't support CPLD this read mode yet.");
                status = -ENOTSUPP;
            }
        }
        else {
            printk(KERN_ERR "Unable to get the client handle for %s\n", info->devname);
            status = -ENODEV;
        }
    }

    return status;
}

/**
 * xcvr_i2c_cpld_write - perform the xcvr read from registered i2c client.
 *
 * @client: The registerd CPLD client
 * @info: The xcvr attributes info
 * @val: value to be written
 * Return: int 
 */
int xcvr_i2c_cpld_write(struct i2c_client *client, XCVR_ATTR *info, uint32_t val)
{
    int status = 0;
    unsigned int val_mask = 0, dnd_value = 0;
    uint32_t reg;

    val_mask = BIT_INDEX(info->mask);

    if (client)
    {
        if (info->len == 1)
            status = __i2c_atr_smbus_read_byte_data(client, info->devaddr, info->offset);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block CPLD read yet");
            status = -ENOTSUPP;
        }
    }
    else
    {
        printk(KERN_ERR "Unable to get the client handle for %s\n", info->devname);
        status = -ENODEV;
    }

    if (status < 0)
        return status;
    else
    {
        msleep(60);
        dnd_value = status & ~val_mask;
        if (((val == 1) && (info->cmpval != 0)) || ((val == 0) && (info->cmpval == 0)))
            reg = dnd_value | val_mask;
        else
            reg = dnd_value;
        if (info->len == 1)
            status = __i2c_atr_smbus_write_byte_data(client, info->devaddr, info->offset, (uint8_t)reg);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block CPLD write yet");
            status = -ENOTSUPP;
        }
    }
    return status;
}

int xcvr_i2c_fpga_read(XCVR_ATTR *info)
{
    int status = -1;
    int retry = 10;

    if (info!=NULL)
    {
        /* Get the I2C client for the CPLD */
        struct i2c_client *client_ptr=NULL;
        client_ptr = (struct i2c_client *)get_device_table(info->devname);
        if (client_ptr)
        {
            if (info->len==1)
            {
                while(retry)
                {
                    status = i2c_smbus_read_byte_data(client_ptr , info->offset);
                    if (unlikely(status < 0))
                    {
                        msleep(60);
                        retry--;
                        continue;
                    }
                    break;
                }
            }
            else if (info->len==2)
            {
                retry = 10;
                while(retry)
                {
                    status = i2c_smbus_read_word_swapped(client_ptr, info->offset);
                    if (unlikely(status < 0))
                    {
                        msleep(60);
                        retry--;
                        continue;
                    }
                    break;
                }
            }
            else
                printk(KERN_ERR "PDDF_XCVR: Doesn't support block FPGAI2C read yet");
        }
        else
            printk(KERN_ERR "Unable to get the client handle for %s\n", info->devname);
    }

    return status;
}

int xcvr_i2c_fpga_write(XCVR_ATTR *info, uint32_t val)
{
    int status = 0;
    unsigned int val_mask = 0, dnd_value = 0;
    uint32_t reg;
    struct i2c_client *client_ptr = NULL;

    val_mask = BIT_INDEX(info->mask);
    /* Get the I2C client for the CPLD */
    client_ptr = (struct i2c_client *)get_device_table(info->devname);

    if (client_ptr)
    {
        if (info->len == 1)
            status = i2c_smbus_read_byte_data(client_ptr, info->offset);
        else if (info->len == 2)
            status = i2c_smbus_read_word_swapped(client_ptr, info->offset);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block FPGAI2C read yet");
            status = -1;
        }
    }
    else
    {
        printk(KERN_ERR "Unable to get the client handle for %s\n", info->devname);
        status = -1;
    }

    if (status < 0)
        return status;
    else
    {
        msleep(60);
        dnd_value = status & ~val_mask;
        if (((val == 1) && (info->cmpval != 0)) || ((val == 0) && (info->cmpval == 0)))
            reg = dnd_value | val_mask;
        else
            reg = dnd_value;
        if (info->len == 1)
            status = i2c_smbus_write_byte_data(client_ptr, info->offset, (uint8_t)reg);
        else if (info->len == 2)
            status = i2c_smbus_write_word_swapped(client_ptr, info->offset, (uint16_t)reg);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block FPGAI2C write yet");
            status = -1;
        }
    }
    return status;
}

int xcvr_fpgapci_read(XCVR_ATTR *info)
{
    int reg_val= 0;
    uint32_t offset = 0;

    if (ptr_fpgapci_read == NULL) {
        printk(KERN_ERR "PDDF_XCVR: Doesn't support FPGAPCI read yet");
        return (-1);
    }

    offset = info->devaddr + info->offset;
    reg_val = ptr_fpgapci_read(offset);
    return reg_val;
}

int xcvr_fpgapci_write(XCVR_ATTR *info, uint32_t val)
{
    int status= 0;
    uint32_t reg, val_mask = 0, dnd_value = 0, reg_val;
    uint32_t offset = 0;

    if (ptr_fpgapci_read == NULL || ptr_fpgapci_write == NULL) {
        printk(KERN_ERR "PDDF_XCVR: Doesn't support FPGAPCI read or write yet");
        return (-1);
    }

    offset = info->devaddr + info->offset;
    val_mask = BIT_INDEX(info->mask);
    reg_val = ptr_fpgapci_read(offset);
    dnd_value =  reg_val & ~val_mask;

    if (((val == 1) && (info->cmpval != 0)) || ((val == 0) && (info->cmpval == 0)))
         reg = dnd_value | val_mask;
    else
         reg = dnd_value;

    status = ptr_fpgapci_write(offset, reg);
    return status;
}

int sonic_i2c_get_mod_pres(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t modpres = 0;

    if ( strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);

        if (status < 0)
            return status;
        else
        {
            modpres = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod presence :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modpres, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            modpres = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod presence :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modpres, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);

        if (status < 0)
            return status;
        else
        {
            modpres = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod presence :0x%x, reg_value= 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modpres, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if(strcmp(info->devtype, "eeprom") == 0)
    {
        /* get client client for eeprom -  Not Applicable */
    }
    data->modpres = modpres;

    return 0;
}

int sonic_i2c_get_mod_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t modreset=0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            modreset = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod reset :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modreset, status, info->devaddr, info->mask, info->offset);
        }
    } 
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            modreset = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod reset :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modreset, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            modreset = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nMod reset :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", modreset, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if(strcmp(info->devtype, "eeprom") == 0)
    {
        /* get client client for eeprom -  Not Applicable */
    }

    data->reset = modreset;
    return 0;
}

int sonic_i2c_get_mod_intr_status(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t mod_intr = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            mod_intr = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Interrupt :0x%x, reg_value = 0x%x\n", mod_intr, status);
        }
    }
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            mod_intr = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Interrupt :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", mod_intr, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            mod_intr = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Interrupt :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", mod_intr, status, info->devaddr, info->mask, info->offset);
        }
    }

    else if(strcmp(info->devtype, "eeprom") == 0)
    {
        /* get client client for eeprom -  Not Applicable */
    }

    data->intr_status = mod_intr;
    return 0;
}

int sonic_i2c_get_mod_lpmode(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t lpmode = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            lpmode = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule LPmode :0x%x, reg_value = 0x%x\n", lpmode, status);
        }
    }
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            lpmode = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule LPmode :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", lpmode, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            lpmode = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule LPmode :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", lpmode, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if (strcmp(info->devtype, "eeprom") == 0)
    {
        /* get client client for eeprom -  Not Applicable */
    }
    
    data->lpmode = lpmode;
    return 0;
}

int sonic_i2c_get_mod_rxlos(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t rxlos = 0;


    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            rxlos = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule RxLOS :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", rxlos, status, info->devaddr, info->mask, info->offset);
        }
    } 
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            rxlos = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule RxLOS :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", rxlos, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            rxlos = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule RxLOS :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", rxlos, status, info->devaddr, info->mask, info->offset);
        }
    }
    data->rxlos = rxlos;

    return 0;
}

int sonic_i2c_get_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t txdis = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            txdis = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule TxDisable :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txdis, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            txdis = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule TxDisable :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txdis, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            txdis = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule TxDisable :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txdis, status, info->devaddr, info->mask, info->offset);
        }
    }
    data->txdisable = txdis;

    return 0;
}

int sonic_i2c_get_mod_txfault(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t txflt = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(client, info);
        if (status < 0)
            return status;
        else
        {
            txflt = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Txfault :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txflt, status, info->devaddr, info->mask, info->offset);
        }

    } 
    else if ( strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_read(info);

        if (status < 0)
            return status;
        else
        {
            txflt = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Txfault :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txflt, status, info->devaddr, info->mask, info->offset);
        }
    }
    else if ( strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_read(info);
        sfp_dbg(KERN_INFO "\n[%s] status=%x\n", __FUNCTION__, status);
        if (status < 0)
            return status;
        else
        {
            txflt = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule Txfault :0x%x, reg_value = 0x%x, devaddr=0x%x, mask=0x%x, offset=0x%x\n", txflt, status, info->devaddr, info->mask, info->offset);
        }
    }
    data->txfault = txflt;

    return 0;
}

int sonic_i2c_set_mod_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_write(client, info, data->reset);
    }
    else if (strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_write(info, data->reset);
    }
    else if (strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_write(info, data->reset);
    }
    else
    {
        printk(KERN_ERR "Error: Invalid device type (%s) to set xcvr reset\n", info->devtype);
        status = -EINVAL;
    }

    return status;
}

int sonic_i2c_set_mod_lpmode(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_write(client, info, data->lpmode);
    }
    else if (strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_write(info, data->lpmode);
    }
    else if (strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_write(info, data->lpmode);
    }
    else
    {
        printk(KERN_ERR "Error: Invalid device type (%s) to set xcvr lpmode\n", info->devtype);
        status = -1;
    }

    return status;
}

int sonic_i2c_set_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_write(client, info, data->txdisable);
    }
    else if (strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_write(info, data->txdisable);
    }
    else
    {
        printk(KERN_ERR "Error: Invalid device type (%s) to set xcvr txdisable\n", info->devtype);
        status = -1;
    }

    return status;
}

ssize_t get_module_presence(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int status = 0, i;

    for (i=0; i<pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            attr_ops = &xcvr_ops[attr->index];

            mutex_lock(&data->update_lock);
            if (attr_ops->pre_get != NULL)
            {
                status = (attr_ops->pre_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            } 
            if (attr_ops->do_get != NULL)
            {
                status = (attr_ops->do_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

            }
            if (attr_ops->post_get != NULL)
            {
                status = (attr_ops->post_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            }
            mutex_unlock(&data->update_lock);
            return sprintf(buf, "%d\n", data->modpres);
        }
    }
    return sprintf(buf, "%s","");
}

ssize_t get_module_reset(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int status = 0, i;

    for (i=0; i<pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            attr_ops = &xcvr_ops[attr->index];

            mutex_lock(&data->update_lock);
            if (attr_ops->pre_get != NULL)
            {
                status = (attr_ops->pre_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: pre_get function fails for %s attribute\n", __FUNCTION__, attr_data->aname);
            } 
            if (attr_ops->do_get != NULL)
            {
                status = (attr_ops->do_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: do_get function fails for %s attribute\n", __FUNCTION__, attr_data->aname);

            }
            if (attr_ops->post_get != NULL)
            {
                status = (attr_ops->post_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: post_get function fails for %s attribute\n", __FUNCTION__, attr_data->aname);
            }

            mutex_unlock(&data->update_lock);

            return sprintf(buf, "%d\n", data->reset);
        }
    }
    return sprintf(buf, "%s","");
}

ssize_t set_module_reset(struct device *dev, struct device_attribute *da, const char *buf, 
        size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int status = 0, i;
    unsigned int set_value;

    for (i=0; i<pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            attr_ops = &xcvr_ops[attr->index];
            if(kstrtoint(buf, 10, &set_value))
                return -EINVAL;
            if ((set_value != 1) && (set_value != 0))
                return -EINVAL;

            data->reset = set_value;

            mutex_lock(&data->update_lock);
            
            if (attr_ops->pre_set != NULL)
            {
                status = (attr_ops->pre_set)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
                }
            if (attr_ops->do_set != NULL)
            {
                status = (attr_ops->do_set)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

            }
            if (attr_ops->post_set != NULL)
            {
                status = (attr_ops->post_set)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            } 
            mutex_unlock(&data->update_lock);

            return count;
        }
    }
    return -EINVAL;
}

ssize_t get_module_intr_status(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int status = 0, i;

    for (i=0; i<pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            attr_ops = &xcvr_ops[attr->index];

            mutex_lock(&data->update_lock);
            if (attr_ops->pre_get != NULL)
            {
                status = (attr_ops->pre_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            } 
            if (attr_ops->do_get != NULL)
            {
                status = (attr_ops->do_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

            }
            if (attr_ops->post_get != NULL)
            {
                status = (attr_ops->post_get)(client, attr_data, data);
                if (status!=0)
                    dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            }

            mutex_unlock(&data->update_lock);
            return sprintf(buf, "%d\n", data->intr_status);
        }
    }
    return sprintf(buf, "%s","");
}

/**
 * get_xcvr_module_attr_data - Get the module attribute data index matched by sensors attribute name.
 * 
 * @client: i2c client hold xcvr platform data.
 * @dev: unused TODO: remove this.
 * @da: sensor device attribute from sysfs.
 * Return: int The index of corespond sensor attribute. -ENODEV if not found.
 */
int get_xcvr_module_attr_data(struct i2c_client *client, struct device *dev, 
                            struct device_attribute *da)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    XCVR_ATTR *attr_data = NULL;
    int i;

    for (i=0; i < pdata->len; i++)
    {
        attr_data = &pdata->xcvr_attrs[i];
        if (strcmp(attr_data->aname, attr->dev_attr.attr.name) == 0)
        {
            return i;
        }
    }
    return -ENODEV;
}

ssize_t get_module_lpmode(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    int idx, status = 0;

    idx = get_xcvr_module_attr_data(client, dev, da);

    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {

        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL)
        {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        } 
        if (attr_ops->do_get != NULL)
        {
            status = (attr_ops->do_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_get != NULL)
        {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);
        return sprintf(buf, "%d\n", data->lpmode);
    }
    else
        return sprintf(buf,"%s","");
}

ssize_t set_module_lpmode(struct device *dev, struct device_attribute *da, const char *buf, 
        size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status = 0;
    uint32_t set_value;
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;

    idx = get_xcvr_module_attr_data(client, dev, da);
    
    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {
        attr_ops = &xcvr_ops[attr->index];
        if(kstrtoint(buf, 10, &set_value))
            return -EINVAL;
        if ((set_value != 1) && (set_value != 0))
            return -EINVAL;

        data->lpmode = set_value;

        mutex_lock(&data->update_lock);
        
        if (attr_ops->pre_set != NULL)
        {
            status = (attr_ops->pre_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            }
        if (attr_ops->do_set != NULL)
        {
            status = (attr_ops->do_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_set != NULL)
        {
            status = (attr_ops->post_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        } 
        mutex_unlock(&data->update_lock);
    }
    return count;
}

ssize_t get_module_rxlos(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status = 0;
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;

    idx = get_xcvr_module_attr_data(client, dev, da);
    
    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {
        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL)
        {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        } 
        if (attr_ops->do_get != NULL)
        {
            status = (attr_ops->do_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_get != NULL)
        {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);
        return sprintf(buf, "%d\n", data->rxlos);
    }
    else
        return sprintf(buf,"%s","");
}

ssize_t get_module_txdisable(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status = 0;
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;
    
    idx = get_xcvr_module_attr_data(client, dev, da);
    
    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {
        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL)
        {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        }
        if (attr_ops->do_get != NULL)
        {
            status = (attr_ops->do_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_get != NULL)
        {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);
        return sprintf(buf, "%d\n", data->txdisable);
    }
    else
        return sprintf(buf,"%s","");
}

ssize_t set_module_txdisable(struct device *dev, struct device_attribute *da, const char *buf, 
        size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct xcvr_data *data = i2c_get_clientdata(client);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    int idx, status = 0;
    uint32_t set_value;
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;

    idx = get_xcvr_module_attr_data(client, dev, da);
    
    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {
        attr_ops = &xcvr_ops[attr->index];
        if(kstrtoint(buf, 10, &set_value))
            return -EINVAL;
        if ((set_value != 1) && (set_value != 0))
            return -EINVAL;

        data->txdisable = set_value;

        mutex_lock(&data->update_lock);
        
        if (attr_ops->pre_set != NULL)
        {
            status = (attr_ops->pre_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
            }
        if (attr_ops->do_set != NULL)
        {
            status = (attr_ops->do_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_set != NULL)
        {
            status = (attr_ops->post_set)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        } 
        mutex_unlock(&data->update_lock);
    }
    return count;
}

ssize_t get_module_txfault(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    XCVR_PDATA *pdata = (XCVR_PDATA *)(client->dev.platform_data);
    struct xcvr_data *data = i2c_get_clientdata(client);
    int idx, status = 0;
    XCVR_ATTR *attr_data = NULL;
    XCVR_SYSFS_ATTR_OPS *attr_ops = NULL;

    idx = get_xcvr_module_attr_data(client, dev, da);
    
    if (idx>=0) attr_data = &pdata->xcvr_attrs[idx];
    
    if (attr_data!=NULL)
    {
        attr_ops = &xcvr_ops[attr->index];

        mutex_lock(&data->update_lock);
        if (attr_ops->pre_get != NULL)
        {
            status = (attr_ops->pre_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        } 
        if (attr_ops->do_get != NULL)
        {
            status = (attr_ops->do_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);

        }
        if (attr_ops->post_get != NULL)
        {
            status = (attr_ops->post_get)(client, attr_data, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, attr_data->aname, status);
        }
        mutex_unlock(&data->update_lock);
        return sprintf(buf, "%d\n", data->txfault);
    }
    return sprintf(buf,"%s","");
}
