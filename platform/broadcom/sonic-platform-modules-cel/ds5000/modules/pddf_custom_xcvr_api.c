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
 *
 * A pddf kernel driver module for Optic component
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
#include "../../../../pddf/i2c/modules/include/pddf_client_defs.h"
#include "../../../../pddf/i2c/modules/include/pddf_xcvr_defs.h"
#include "../../../../pddf/i2c/modules/include/pddf_xcvr_api.h"

/*#define SFP_DEBUG*/
#ifdef SFP_DEBUG
#define sfp_dbg(...) printk(__VA_ARGS__)
#else
#define sfp_dbg(...)
#endif

extern XCVR_SYSFS_ATTR_OPS xcvr_ops[];
extern void *get_device_table(char *name);
extern int (*ptr_fpgapci_read)(uint32_t);
extern int (*ptr_fpgapci_write)(uint32_t, uint32_t);

static struct mutex pddf_osfp_mutex;

int pddf_osfp_mutex_lock(void)
{
    mutex_lock(&pddf_osfp_mutex);
    return 0;
}

int pddf_osfp_mutex_unlock(void)
{
    mutex_unlock(&pddf_osfp_mutex);
    return 0;
}

static int pddf_xcvr_select(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data)
{
    uint8_t index;
    uint32_t devaddr = 0x10;
    char devname[32]; 
    int status = -1;
    int retry = 10;
    
    if (data->index < 32){
        index = data->index;
        devaddr = 0x30;
        strcpy(devname, "CPLD1");
    }else if (data->index < 64){
        index = data->index - 32;
        devaddr = 0x31;
        strcpy(devname, "CPLD2");
    }else{
        status = 0;
    }
    
    if (data->index < 64){
        pddf_osfp_mutex_lock();
        while (retry)
        {
            status = board_i2c_cpld_write_new(devaddr, devname, 0x10, (uint8_t)index);
            if (unlikely(status < 0))
            {
                msleep(60);
                retry--;
                continue;
            }
            break;
        }
    }
    
    if (status < 0)
        printk(KERN_ERR "Error: Failed to write port index to CPLD register, data->index = %d\n", data->index);
    
    return status;
}

static int pddf_xcvr_release(struct i2c_client *client, XCVR_ATTR *adata, struct xcvr_data *data)
{   
    if (data->index < 64){
        pddf_osfp_mutex_unlock();
    }
    
    return 0;
}

static int xcvr_i2c_cpld_read(XCVR_ATTR *info)
{
    int status = -1;
    int retry = 10;
    struct i2c_client *client_ptr=NULL;

    if (info!=NULL)
    {
        /* Get the I2C client for the CPLD */
        client_ptr = (struct i2c_client *)get_device_table(info->devname);
        if (client_ptr)
        {
            if (info->len==1)
            {
                while (retry)
                {
                    status = board_i2c_cpld_read_new(info->devaddr, info->devname, info->offset);
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
                printk(KERN_ERR "PDDF_XCVR: Doesn't support block CPLD read yet");
        }
        else
            printk(KERN_ERR "Unable to get the client handle for %s\n", info->devname);
    }

    return status;
}

static int xcvr_i2c_cpld_write(XCVR_ATTR *info, uint32_t val)
{
    int status = 0;
    unsigned int val_mask = 0, dnd_value = 0;
    uint32_t reg;
    struct i2c_client *client_ptr=NULL;

    val_mask = BIT_INDEX(info->mask);
    /* Get the I2C client for the CPLD */
    client_ptr = (struct i2c_client *)get_device_table(info->devname);

    if (client_ptr)
    {
        if (info->len == 1)
            status = board_i2c_cpld_read_new(info->devaddr, info->devname, info->offset);
        else if (info->len == 2)
            status = i2c_smbus_read_word_swapped(client_ptr, info->offset);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block CPLD read yet");
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
            status = board_i2c_cpld_write_new(info->devaddr, info->devname, info->offset, (uint8_t)reg);
        else if (info->len == 2)
            status = i2c_smbus_write_word_swapped(client_ptr, info->offset, (uint16_t)reg);
        else
        {
            printk(KERN_ERR "PDDF_XCVR: Doesn't support block CPLD write yet");
            status = -1;
        }
    }
    return status;
}

static int xcvr_i2c_fpga_read(XCVR_ATTR *info)
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

static int xcvr_i2c_fpga_write(XCVR_ATTR *info, uint32_t val)
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

static int xcvr_fpgapci_read(XCVR_ATTR *info)
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

static int xcvr_fpgapci_write(XCVR_ATTR *info, uint32_t val)
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

static int pddf_get_mod_rxlos(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t rxlos = 0;


    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(info);
        if (status < 0)
            return status;
        else
        {
            rxlos = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule RxLOS :0x%x, reg_value = 0x%x\n", rxlos, status);
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

static int pddf_get_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t txdis = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(info);
        if (status < 0)
            return status;
        else
        {
            txdis = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule TxDisable :0x%x, reg_value = 0x%x\n", txdis, status);
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

static int pddf_get_mod_txfault(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;
    uint32_t txflt = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_read(info);
        if (status < 0)
            return status;
        else
        {
            txflt = ((status & BIT_INDEX(info->mask)) == info->cmpval) ? 1 : 0;
            sfp_dbg(KERN_INFO "\nModule TxFault :0x%x, reg_value = 0x%x\n", txflt, status);
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

static int pddf_set_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data)
{
    int status = 0;

    if (strcmp(info->devtype, "cpld") == 0)
    {
        status = xcvr_i2c_cpld_write(info, data->txdisable);
    }
    else if (strcmp(info->devtype, "fpgai2c") == 0)
    {
        status = xcvr_i2c_fpga_write(info, data->txdisable);
    }
    else if (strcmp(info->devtype, "fpgapci") == 0)
    {
        status = xcvr_fpgapci_write(info, data->txdisable);
    }
    else
    {
        printk(KERN_ERR "Error: Invalid device type (%s) to set xcvr txdisable\n", info->devtype);
        status = -1;
    }

    return status;
}

static int __init pddf_xcvr_patch_init(void)
{
    mutex_init(&pddf_osfp_mutex);
    
    xcvr_ops[XCVR_PRESENT].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_PRESENT].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_PRESENT].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_PRESENT].post_set = pddf_xcvr_release;    
    xcvr_ops[XCVR_RESET].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_RESET].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_RESET].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_RESET].post_set = pddf_xcvr_release;  
    xcvr_ops[XCVR_INTR_STATUS].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_INTR_STATUS].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_INTR_STATUS].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_INTR_STATUS].post_set = pddf_xcvr_release;    
    xcvr_ops[XCVR_LPMODE].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_LPMODE].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_LPMODE].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_LPMODE].post_set = pddf_xcvr_release; 
    xcvr_ops[XCVR_RXLOS].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_RXLOS].do_get = pddf_get_mod_rxlos;
    xcvr_ops[XCVR_RXLOS].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_RXLOS].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_RXLOS].post_set = pddf_xcvr_release;  
    xcvr_ops[XCVR_TXDISABLE].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_TXDISABLE].do_get = pddf_get_mod_txdisable;
    xcvr_ops[XCVR_TXDISABLE].do_set = pddf_set_mod_txdisable;
    xcvr_ops[XCVR_TXDISABLE].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_TXDISABLE].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_TXDISABLE].post_set = pddf_xcvr_release;  
    xcvr_ops[XCVR_TXFAULT].pre_get = pddf_xcvr_select;
    xcvr_ops[XCVR_TXFAULT].do_get = pddf_get_mod_txfault;
    xcvr_ops[XCVR_TXFAULT].pre_set = pddf_xcvr_select;
    xcvr_ops[XCVR_TXFAULT].post_get = pddf_xcvr_release;
    xcvr_ops[XCVR_TXFAULT].post_set = pddf_xcvr_release;    
    return 0;
}

static void __exit pddf_xcvr_patch_exit(void)
{
    return;
}

MODULE_AUTHOR("Fan Xinghua");
MODULE_DESCRIPTION("pddf custom xcvr api");
MODULE_LICENSE("GPL");

module_init(pddf_xcvr_patch_init);
module_exit(pddf_xcvr_patch_exit);
