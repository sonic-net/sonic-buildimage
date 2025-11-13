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
 * A pddf kernel module to manage various LEDs of a switch
 */

#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include "pddf_client_defs.h"
#include "pddf_led_defs.h"
#include "pddf_multifpgapci_defs.h"

#define DEBUG 0
#define MAX_PSU_NUM 2
#define MAX_FANTRAY_NUM 6
#define MAX_PORT_NUM (48 * 8)
LED_OPS_DATA sys_led_ops_data[1]={0};
LED_OPS_DATA psu_led_ops_data[MAX_PSU_NUM]={0};
LED_OPS_DATA diag_led_ops_data[1]= {0};
LED_OPS_DATA fan_led_ops_data[1]= {0};
LED_OPS_DATA loc_led_ops_data[1]= {0};
LED_OPS_DATA bmc_led_ops_data[1]= {0};
LED_OPS_DATA fantray_led_ops_data[MAX_FANTRAY_NUM]={0};
LED_OPS_DATA port_led_ops_data[MAX_PORT_NUM] = {0};
LED_OPS_DATA temp_data={0};
LED_OPS_DATA* dev_list[LED_TYPE_MAX] = {
    sys_led_ops_data,
    psu_led_ops_data,
    fan_led_ops_data,
    fantray_led_ops_data,
    diag_led_ops_data,
    loc_led_ops_data,
    bmc_led_ops_data,
    port_led_ops_data,
    NULL
};
int num_psus = 0;
int num_fantrays = 0;

extern int board_i2c_cpld_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_cpld_write(unsigned short cpld_addr, u8 reg, u8 value);
extern int board_i2c_fpga_read(unsigned short cpld_addr, u8 reg);
extern int board_i2c_fpga_write(unsigned short cpld_addr, u8 reg, u8 value);

extern ssize_t show_pddf_data(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t store_pddf_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern ssize_t show_pddf_s3ip_data(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t store_pddf_s3ip_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

extern void* get_device_table(char *name);

static LED_STATUS find_state_index(const char* state_str) {
    int index;
    char *ptr = (char *)state_str;
    while (*ptr && *ptr!= '\n' && *ptr !='\0') ptr++;
    *ptr='\0';
    for ( index = 0; index < MAX_LED_STATUS; index++) {
        if (strcmp(state_str, LED_STATUS_STR[index]) == 0 ) {
            return index;
        }
    }
    return MAX_LED_STATUS;
}

static LED_TYPE get_dev_type(char* name)
{
    LED_TYPE ret = LED_TYPE_MAX;
    if(strcasecmp(name, "SYS_LED") == 0) {
        ret = LED_SYS;
    } else if(strcasecmp(name, "FAN_LED") == 0) {
        ret = LED_FAN;
    } else if(strstr(name, "PSU_LED")) {
        ret = LED_PSU;
    } else if(strcasecmp(name, "DIAG_LED") == 0) {
        ret = LED_DIAG;
    } else if(strcasecmp(name, "LOC_LED") == 0) {
        ret = LED_LOC;
    } else if(strstr(name, "FANTRAY_LED")) {
        ret = LED_FANTRAY;
    } else if(strstr(name, "PORT_LED")) {
        ret = LED_PORT;
    }
#if DEBUG > 1
    pddf_dbg(LED, KERN_INFO "LED get_dev_type: %s; %d\n", name, ret);
#endif
    return (ret);
}
static int dev_index_check(LED_TYPE type, int index)
{
#if DEBUG
    pddf_dbg(LED, "dev_index_check: type:%s[%d] index:%d num_psus:%d num_fantrays:%d\n",
        LED_TYPE_STR[type], type, index, num_psus, num_fantrays);
#endif
    switch(type)
    {
        case LED_PSU:
            if(index >= MAX_PSU_NUM) return (-1);
            break;
        case LED_FANTRAY:
            if(index >= MAX_FANTRAY_NUM) return (-1);
            break;
        case LED_PORT:
            if(index >= MAX_PORT_NUM) return (-1);
            break;
        default:
            if(index >= 1) return (-1);
            break;
    }
    return (0);
}

static LED_OPS_DATA* find_led_ops_data(struct device_attribute *da)
{
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    LED_OPS_DATA* ptr = (LED_OPS_DATA*)_ptr->addr;
    LED_TYPE led_type;
    if(!ptr || strlen(ptr->device_name) == 0 ) return (NULL);

    if((led_type=get_dev_type(ptr->device_name)) == LED_TYPE_MAX) {
        printk(KERN_ERR "PDDF_LED ERROR *%s Unsupported Led Type\n", __func__);
        return (NULL);
    }
    if(dev_index_check(led_type, ptr->index) == -1) {
        printk(KERN_ERR "PDDF_LED ERROR %s invalid index: %d for type:%s;%d\n", __func__, ptr->index, ptr->device_name, led_type);
        return (NULL);
    }
#if DEBUG > 1
    pddf_dbg(LED, "find_led_ops_data: name:%s; index=%d tempAddr:%p actualAddr:%p\n",
        ptr->device_name, ptr->index, ptr, dev_list[led_type]+ptr->index);
#endif
    return (dev_list[led_type]+ptr->index);
}

static void print_led_data(LED_OPS_DATA *ptr, LED_STATUS state)
{
    int i = 0;
    if(!ptr) return ;
    pddf_dbg(LED, KERN_INFO "Print %s index:%d num_psus:%d num_fantrays:%d ADDR=%p\n",
        ptr->device_name, ptr->index, num_psus, num_fantrays, ptr);
    pddf_dbg(LED, KERN_INFO "\tindex: %d\n", ptr->index);
    pddf_dbg(LED, KERN_INFO "\tdevtype/devname: %s:%s\n", ptr->attr_devtype, ptr->attr_devname);
    pddf_dbg(LED, KERN_INFO  "\tcur_state: %d; %s \n", ptr->cur_state.state, ptr->cur_state.color);
    for (i = 0; i< MAX_LED_STATUS; i++) {
        if(ptr->data[i].swpld_addr && (i == state || state == -1)) {
            pddf_dbg(LED, KERN_INFO "\t\t[%s]: addr/offset:0x%x;0x%x color:%s; value:[%s][0x%x][0x%x] mask_bits: 0x%x;"
                    "pos:%d attr_devtype:%s attr_devname:%s\n",LED_STATUS_STR[i], ptr->data[i].swpld_addr,
                    ptr->data[i].swpld_addr_offset, LED_STATUS_STR[i], ptr->data[i].value,
                    ptr->data[i].reg_values[0], ptr->data[i].reg_values[1], ptr->data[i].bits.mask_bits,
                    ptr->data[i].bits.pos, ptr->data[i].attr_devtype, ptr->data[i].attr_devname);
        }
    }
}

int led_multifpgapci_read(LED_OPS_DATA *ops_ptr, uint32_t *output) {
    struct pci_dev *pci_dev = NULL;

    if (ptr_multifpgapci_readpci == NULL) {
        printk(KERN_ERR "PDDF_LED: pddf_multifpgapci_module is not loaded");
        return -1;
    }

    pci_dev = (struct pci_dev *)get_device_table(ops_ptr->attr_devname);
    if (pci_dev == NULL) {
      printk(KERN_ERR "PDDF_LED: Unable to get pci_dev of %s for %s\n",
             ops_ptr->attr_devname, ops_ptr->device_name);
      return -1;
    }
    return ptr_multifpgapci_readpci(
        pci_dev, ops_ptr->swpld_addr + ops_ptr->swpld_addr_offset, output);
}

int led_multifpgapci_write(LED_OPS_DATA *ops_ptr, uint32_t val) {
    struct pci_dev *pci_dev = NULL;

    if (ptr_multifpgapci_readpci == NULL || ptr_multifpgapci_writepci == NULL) {
        printk(KERN_ERR "PDDF_LED: pddf_multifpgapci_module is not loaded");
        return -1;
    }

    pci_dev = (struct pci_dev *)get_device_table(ops_ptr->attr_devname);
    if (pci_dev == NULL) {
      printk(KERN_ERR "PDDF_LED: Unable to get pci_dev of %s for %s\n",
             ops_ptr->attr_devname, ops_ptr->device_name);
      return -1;
    }
    return ptr_multifpgapci_writepci(
        pci_dev, val, ops_ptr->swpld_addr + ops_ptr->swpld_addr_offset);
}

ssize_t get_status_led(struct device_attribute *da)
{
    int ret=0;
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    LED_OPS_DATA* temp_data_ptr=(LED_OPS_DATA*)_ptr->addr;
    LED_OPS_DATA* ops_ptr=find_led_ops_data(da);
    uint32_t color_val=0, sys_val=0;
    int state=0;
    int cpld_type=0;
    int j;

    if (!ops_ptr) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: Cannot find LED Ptr", __func__);
        return (-1);
    }

    if (ops_ptr->swpld_addr == 0x0) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: device: %s %d not configured\n", __func__,
            temp_data_ptr->device_name, temp_data_ptr->index);
        return (-1);
    }

    if (strcmp(ops_ptr->attr_devtype, "cpld") == 0) {
        cpld_type = 1;
        sys_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        if (sys_val < 0) {
            pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x read failed\n",__func__,
                ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
            return sys_val;
        }
    } else if (strcmp(ops_ptr->attr_devtype, "fpgai2c") == 0) {
        sys_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        if (sys_val < 0) {
            pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x read failed\n",__func__,
                ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
            return sys_val;
        }
    } else if (strcmp(ops_ptr->attr_devtype, "multifpgapci") == 0) {
        ret = led_multifpgapci_read(ops_ptr, &sys_val);
        if (ret)
            goto ret;
    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        return (-1);
    }

    strcpy(temp_data.cur_state.color, "None");
    for (state=0; state<MAX_LED_STATUS; state++) {
        color_val = (sys_val & ~ops_ptr->data[state].bits.mask_bits);
        for (j = 0; j < VALUE_SIZE && ops_ptr->data[state].reg_values[j] != 0xff; j++) {
           if ((color_val ^ (ops_ptr->data[state].reg_values[j] << ops_ptr->data[state].bits.pos)) == 0) {
                strcpy(temp_data.cur_state.color, LED_STATUS_STR[state]);
                goto found_match;
           }
        }
    }

found_match:
#if DEBUG
    pddf_dbg(LED, KERN_ERR "Get : %s:%d addr/offset:0x%x; 0x%x devtype:%s;%s value=0x%x [%s]\n",
        ops_ptr->device_name, ops_ptr->index, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset,
        ops_ptr->attr_devtype, cpld_type? "cpld": "fpgai2c", sys_val, temp_data.cur_state.color);
#endif

ret:
    if (ret) {
        printk(KERN_ERR "%s: Error status = %d", __FUNCTION__, ret);
    }

    return(ret);
}

ssize_t set_status_led(struct device_attribute *da)
{
    int ret=0;
    uint32_t sys_val=0, new_val=0, read_val=0;
    LED_STATUS cur_state = MAX_LED_STATUS;
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    LED_OPS_DATA* temp_data_ptr=(LED_OPS_DATA*)_ptr->addr;
    LED_OPS_DATA* ops_ptr=find_led_ops_data(da);
    char* _buf=temp_data_ptr->cur_state.color;
    int cpld_type = 0;

    if (!ops_ptr) {
        pddf_dbg(LED, KERN_ERR "PDDF_LED ERROR %s: Cannot find LED Ptr", __func__);
        return (-1);
    }

    if (ops_ptr->swpld_addr == 0x0) {
        pddf_dbg(LED, KERN_ERR "PDDF_LED ERROR %s: device: %s %d not configured\n",
            __func__, ops_ptr->device_name, ops_ptr->index);
        return (-1);
    }

#if DEBUG
    pddf_dbg(LED, KERN_INFO "%s: Set [%s;%d] color[%s]\n", __func__,
        temp_data_ptr->device_name, temp_data_ptr->index,
        temp_data_ptr->cur_state.color);
#endif

    cur_state = find_state_index(_buf);
    if (cur_state == MAX_LED_STATUS) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: not supported: %s\n", _buf, __func__);
        return (-1);
    }

    if (ops_ptr->data[cur_state].swpld_addr != 0x0) {
        if (strcmp(ops_ptr->data[cur_state].attr_devtype, "cpld") == 0) {
            cpld_type = 1;
            sys_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
            if (sys_val < 0)
                return sys_val;
        } else if (strcmp(ops_ptr->data[cur_state].attr_devtype, "fpgai2c") == 0) {
            sys_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
            if (sys_val < 0)
                return sys_val;
        } else if (strcmp(ops_ptr->attr_devtype, "multifpgapci") == 0) {
            ret = led_multifpgapci_read(ops_ptr, &sys_val);
            if (ret)
                goto ret;
        } else {
            pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s not configured\n",__func__,
                ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype);
            return (-1);
        }

        new_val = (sys_val & ops_ptr->data[cur_state].bits.mask_bits) |
                    (ops_ptr->data[cur_state].reg_values[0] << ops_ptr->data[cur_state].bits.pos);

    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s state %d; %s not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, cur_state, _buf);
        return (-1);
    }

    if (strcmp(ops_ptr->data[cur_state].attr_devtype, "cpld") == 0) {
        ret = board_i2c_cpld_write(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset, new_val);
        read_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else if (strcmp(ops_ptr->data[cur_state].attr_devtype, "fpgai2c") == 0) {
        ret = board_i2c_fpga_write(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset, (uint8_t)new_val);
        read_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else if (strcmp(ops_ptr->attr_devtype, "multifpgapci") == 0) {
        ret = led_multifpgapci_write(ops_ptr, new_val);
        if (ret)
          goto ret;
        ret = led_multifpgapci_read(ops_ptr, &read_val);
        if (ret)
          goto ret;
    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype);
        return (-1);
    }

#if DEBUG
    pddf_dbg(LED, KERN_INFO "Set color:%s; 0x%x:0x%x sys_val:0x%x new_val:0x%x devtype:%s w_ret:0x%x read:0x%x devtype:%s\n",
        LED_STATUS_STR[cur_state], ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset, sys_val, new_val,
        cpld_type? "cpld":"fpgai2c", ret, read_val, ops_ptr->data[cur_state].attr_devtype);
#endif

ret:
    if (ret) {
        printk(KERN_ERR "%s: Error status = %d", __FUNCTION__, ret);
    }

    return(ret);
}


ssize_t show_pddf_data(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int ret = 0;
    struct pddf_data_attribute *ptr = (struct pddf_data_attribute *)da;
    switch (ptr->type)
    {
        case PDDF_CHAR:
            ret = sprintf(buf, "%s\n", ptr->addr);
            break;
        case PDDF_INT_DEC:
            ret = sprintf(buf, "%d\n", *(int*)(ptr->addr));
            break;
        case PDDF_INT_HEX:
            ret = sprintf(buf, "0x%x\n", *(int*)(ptr->addr));
            break;
        case PDDF_USHORT:
            ret = sprintf(buf, "0x%x\n", *(unsigned short *)(ptr->addr));
            break;
        case PDDF_UINT32:
            ret = sprintf(buf, "0x%x\n", *(uint32_t *)(ptr->addr));
            break;
        default:
            break;
    }
#if DEBUG > 1
    pddf_dbg(LED, "[ READ ] DATA ATTR PTR [%s] TYPE:%d, Value:[%s]\n",
        ptr->dev_attr.attr.name, ptr->type, buf);
#endif
    return ret;
}

ssize_t store_pddf_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret = 0, num = 0;
    struct pddf_data_attribute *ptr = (struct pddf_data_attribute *)da;
    switch (ptr->type)
    {
        case PDDF_CHAR:
            strncpy(ptr->addr, buf, strlen(buf)-1); // to discard newline char form buf
            ptr->addr[strlen(buf)-1] = '\0';
#if DEBUG
            pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR [%s] PDDF_CHAR  VALUE:%s\n",
                ptr->dev_attr.attr.name, ptr->addr);
#endif
            break;
        case PDDF_INT_DEC:
            ret = kstrtoint(buf,10,&num);
            if (ret==0)
                *(int *)(ptr->addr) = num;
#if DEBUG
            pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR [%s] PDDF_DEC  VALUE:%d\n",
                ptr->dev_attr.attr.name, *(int *)(ptr->addr));
#endif
            break;
        case PDDF_INT_HEX:
            ret = kstrtoint(buf,16,&num);
            if (ret==0)
                *(int *)(ptr->addr) = num;
#if DEBUG
            pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR [%s] PDDF_HEX  VALUE:0x%x\n",
                ptr->dev_attr.attr.name, *(int *)(ptr->addr));
#endif
            break;
        case PDDF_USHORT:
            ret = kstrtoint(buf,16,&num);
            if (ret==0)
                *(unsigned short *)(ptr->addr) = (unsigned short)num;
#if DEBUG
            pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR [%s] PDDF_USHORT  VALUE:%x\n",
                ptr->dev_attr.attr.name, *(unsigned short *)(ptr->addr));
#endif
            break;
        case PDDF_UINT32:
            ret = kstrtoint(buf,16,&num);
            if (ret==0)
                *(uint32_t *)(ptr->addr) = (uint32_t)num;
#if DEBUG
            pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR [%s] PDDF_UINT32 VALUE:%d\n",
                ptr->dev_attr.attr.name, *(uint32_t *)(ptr->addr));
#endif
            break;
        default:
            break;
    }
    return count;
}

ssize_t show_pddf_s3ip_data(struct device *dev, struct device_attribute *da,
             char *buf)
{
    int ret = 0;
    pddf_dbg(LED, KERN_ERR " %s", __FUNCTION__);
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    if (_ptr == NULL) {
       pddf_dbg(LED, KERN_ERR "%s return", __FUNCTION__);
       return -1;
    }
    LED_OPS_DATA* ops_ptr=(LED_OPS_DATA*)_ptr->addr;
    uint32_t color_val=0, sys_val=0;
    int state=0, j;
    int cpld_type=0;
    if (!ops_ptr) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: Cannot find LED Ptr", __func__);
        return (-1);
    }
    if (ops_ptr->swpld_addr == 0x0) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: device: %s %d not configured\n", __func__,
            ops_ptr->device_name, ops_ptr->index);
        return (-1);
    }
    if ( strcmp(ops_ptr->attr_devtype, "cpld") == 0) {
        cpld_type=1;
        sys_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else if ( strcmp(ops_ptr->attr_devtype, "fpgai2c") == 0) {
        sys_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        return (-1);
    }

    if (sys_val < 0) {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x read failed\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        return sys_val;
    }
    for (state=0; state<MAX_LED_STATUS; state++) {
        color_val = (sys_val & ~ops_ptr->data[state].bits.mask_bits);
        for (j = 0; j < VALUE_SIZE && ops_ptr->data[state].reg_values[j] != 0xff; j++) {
           if ((color_val ^ (ops_ptr->data[state].reg_values[j] << ops_ptr->data[state].bits.pos))==0) {
                ret = sprintf(buf, "%d\n", state);
                break;
           }
        }
    }
#if DEBUG
    pddf_dbg(LED, KERN_ERR "Get : %s:%d addr/offset:0x%x; 0x%x devtype:%s;%s value=0x%x [%d]\n",
        ops_ptr->device_name, ops_ptr->index, ops_ptr->swpld_addr,
        ops_ptr->swpld_addr_offset, ops_ptr->attr_devtype, cpld_type? "cpld": "fpgai2c", sys_val, state);
#endif
    return ret;
}

ssize_t store_pddf_s3ip_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret = 0;
    int cur_state = 0;
    uint32_t sys_val=0, new_val=0, read_val=0;
    int cpld_type=0;

    pddf_dbg(LED, KERN_ERR "%s: %s;%d", __FUNCTION__, buf, cur_state);
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    ret = kstrtoint(buf,10,&cur_state);
    if (_ptr == NULL || cur_state >= MAX_LED_STATUS || ret !=0) {
       pddf_dbg(LED, KERN_ERR "%s return", __FUNCTION__);
       return -1;
    }
    LED_OPS_DATA* ops_ptr=(LED_OPS_DATA*)_ptr->addr;

    if ( strcmp(ops_ptr->attr_devtype, "cpld") == 0) {
        cpld_type=1;
        sys_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else if ( strcmp(ops_ptr->attr_devtype, "fpgai2c") == 0) {
        sys_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s 0x%x:0x%x not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype, ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
        return (-1);
    }

    new_val = (sys_val & ops_ptr->data[cur_state].bits.mask_bits) |
            (ops_ptr->data[cur_state].reg_values[0] << ops_ptr->data[cur_state].bits.pos);

    if ( strcmp(ops_ptr->data[cur_state].attr_devtype, "cpld") == 0) {
        ret = board_i2c_cpld_write(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset, new_val);
        read_val = board_i2c_cpld_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else if ( strcmp(ops_ptr->data[cur_state].attr_devtype, "fpgai2c") == 0) {
        ret = board_i2c_fpga_write(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset, (uint8_t)new_val);
        read_val = board_i2c_fpga_read(ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset);
    } else {
        pddf_dbg(LED, KERN_ERR "ERROR %s: %s %d devtype:%s not configured\n",__func__,
            ops_ptr->device_name, ops_ptr->index, ops_ptr->attr_devtype);
        return (-1);
    }

#if DEBUG
    pddf_dbg(LED, KERN_INFO "Set color:%s; 0x%x:0x%x sys_val:0x%x new_val:0x%x devtype:%s w_ret:0x%x read:0x%x devtype:%s\n",
        LED_STATUS_STR[cur_state], ops_ptr->swpld_addr, ops_ptr->swpld_addr_offset,
        sys_val, new_val, cpld_type? "cpld":"fpgai2c", ret, read_val, ops_ptr->data[cur_state].attr_devtype);
#endif
    return count;
}


static int load_led_ops_data(struct device_attribute *da, LED_STATUS state)
{
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    LED_OPS_DATA* ptr = (LED_OPS_DATA*)_ptr->addr;
    LED_TYPE led_type;
    LED_OPS_DATA* ops_ptr = NULL;
    int i = 0;
    char *token = NULL, *value_ptr = NULL;

    if(!ptr || strlen(ptr->device_name)==0 ) {
        pddf_dbg(LED, KERN_INFO "SYSTEM_LED: load_led_ops_data return -1 device_name:%s\n", ptr? ptr->device_name:"NULL");
        return(-1);
    }

    if(ptr->device_name)
    {
        pddf_dbg(LED, KERN_INFO "[%s]: load_led_ops_data: index=%d addr=0x%x;0x%x devtype:%s devname=%s valu=%s\n",
                ptr->device_name, ptr->index, ptr->swpld_addr, ptr->swpld_addr_offset, ptr->attr_devtype, ptr->attr_devname, ptr->data[0].value);
    }
    if((led_type=get_dev_type(ptr->device_name))==LED_TYPE_MAX) {
        pddf_dbg(LED, KERN_ERR "PDDF_LED ERROR *%s Unsupported Led Type\n", __func__);
        return(-1);
    }
    if(dev_index_check(led_type, ptr->index)==-1) {
        pddf_dbg(LED, KERN_ERR "PDDF_LED ERROR %s invalid index: %d for type:%d\n", __func__, ptr->index, led_type);
        return(-1);
    }
    ops_ptr = dev_list[led_type]+ptr->index;

    memcpy(ops_ptr->device_name, ptr->device_name, sizeof(ops_ptr->device_name));
    ops_ptr->index = ptr->index;
    memcpy(&ops_ptr->data[state], &ptr->data[0], sizeof(LED_DATA));
    ops_ptr->data[state].swpld_addr = ptr->swpld_addr;
    ops_ptr->data[state].swpld_addr_offset = ptr->swpld_addr_offset;
    ops_ptr->swpld_addr = ptr->swpld_addr;
    ops_ptr->swpld_addr_offset = ptr->swpld_addr_offset;
    memcpy(ops_ptr->data[state].attr_devtype, ptr->attr_devtype, sizeof(ops_ptr->data[state].attr_devtype));
    memcpy(ops_ptr->data[state].attr_devname, ptr->attr_devname, sizeof(ops_ptr->data[state].attr_devname));
    memcpy(ops_ptr->attr_devtype, ptr->attr_devtype, sizeof(ops_ptr->attr_devtype));
    memcpy(ops_ptr->attr_devname, ptr->attr_devname, sizeof(ops_ptr->attr_devname));
#ifdef __STDC_LIB_EXT1__
    memset_s(ops_ptr->data[state].reg_values, sizeof(ops_ptr->data[state].reg_values), 0xff, sizeof(ops_ptr->data[state].reg_values));
#else
    memset(ops_ptr->data[state].reg_values, 0xff, sizeof(ops_ptr->data[state].reg_values));
#endif
    value_ptr = kzalloc(sizeof(ops_ptr->data[state].value), GFP_KERNEL);
    if (value_ptr) {
       memcpy(value_ptr, ops_ptr->data[state].value, sizeof(ops_ptr->data[state].value));
       while((token = strsep((char**)&value_ptr,";")) != NULL  && i < VALUE_SIZE) {
          if (kstrtou8(token, 16, &ops_ptr->data[state].reg_values[i])) {
                pddf_dbg(LED, KERN_ERR "load_led_ops_data: [%s] conversion error\n", token);
           }
           i++;
       }
       kfree(value_ptr);
    }

    print_led_data(dev_list[led_type]+ptr->index, state);

    memset(ptr, 0, sizeof(LED_OPS_DATA));
    return (0);
}

static int show_led_ops_data(struct device_attribute *da)
{
    LED_OPS_DATA* ops_ptr = find_led_ops_data(da);
    print_led_data(ops_ptr, -1);
    return(0);
}

static int verify_led_ops_data(struct device_attribute *da)
{
    struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
    LED_OPS_DATA* ptr=(LED_OPS_DATA*)_ptr->addr;
    LED_OPS_DATA* ops_ptr=find_led_ops_data(da);

    if(ops_ptr)
        memcpy(ptr, ops_ptr, sizeof(LED_OPS_DATA));
    else
    {
        pddf_dbg(LED, "SYSTEM_LED: verify_led_ops_data: Failed to find ops_ptr name:%s; index=%d\n", ptr->device_name, ptr->index);
    }
    return (0);
}


ssize_t dev_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
#if DEBUG
    pddf_dbg(LED, KERN_INFO "dev_operation [%s]\n", buf);
#endif
    if(strncmp(buf, "show", strlen("show")) == 0) {
        show_led_ops_data(da);
    }
    else if(strncmp(buf, "verify", strlen("verify")) == 0) {
        verify_led_ops_data(da);
    }
    else if(strncmp(buf, "get_status", strlen("get_status")) == 0) {
        get_status_led(da);
    }
    else if(strncmp(buf, "set_status", strlen("set_status")) == 0) {
        set_status_led(da);
    }
    else {
        LED_STATUS index = find_state_index(buf);
        if (index < MAX_LED_STATUS) {
            load_led_ops_data(da, index);
        } else {
            printk(KERN_ERR "PDDF_ERROR: %s: Invalid value for dev_ops %s\n", __FUNCTION__, buf);
        }
    }
    return (count);
}

ssize_t store_config_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret, num;
    struct pddf_data_attribute *ptr = (struct pddf_data_attribute *)da;
    if(strncmp(ptr->dev_attr.attr.name, "num_psus", strlen("num_psus")) == 0) {
        ret = kstrtoint(buf,10,&num);
        if (ret==0)
            *(int *)(ptr->addr) = num;
#if DEBUG
        pddf_dbg(LED, "[ WRITE ] ATTR CONFIG [%s] VALUE:%d; %d\n",
                ptr->dev_attr.attr.name, num, num_psus);
#endif
        return(count);
    }
    if (strncmp(ptr->dev_attr.attr.name, "num_fantrays", strlen("num_fantrays")) ==0) {
       ret = kstrtoint(buf, 10, &num);
       if (ret == 0)
          *(int *)(ptr->addr) = num;
#if DEBUG
        pddf_dbg(LED, "[ WRITE ] ATTR CONFIG [%s] VALUE:%d; %d\n",
            ptr->dev_attr.attr.name, num, num_fantrays);
#endif
        return (count);
    }
    return (count);
}

ssize_t store_bits_data(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int len = 0, num1 = 0, num2 = 0, i=0, rc1=0, rc2=0;
    unsigned int mask = 0xFFFFFFFF;
    char *pptr=NULL;
    char bits[NAME_SIZE];
    struct pddf_data_attribute *ptr = (struct pddf_data_attribute *)da;
    MASK_BITS* bits_ptr=(MASK_BITS*)(ptr->addr);
    strncpy(bits_ptr->bits, buf, strlen(buf)-1); // to discard newline char form buf
    bits_ptr->bits[strlen(buf)-1] = '\0';
    if((pptr=strstr(buf,":")) != NULL) {
        len = pptr-buf;
        sprintf(bits, buf);
        bits[len] = '\0';
        rc1 = kstrtoint(bits, 16, &num1);
        if (rc1 == 0)
        {
            sprintf(bits, ++pptr);
            rc2 = kstrtoint(bits, 16, &num2);
            if (rc2 == 0)
            {
                for (i=num2; i<=num1; i++) {
                   mask &= ~(1 << i);
                }
                bits_ptr->mask_bits = mask;
                bits_ptr->pos = num2;
            }
        }
    } else {
        rc1 = kstrtoint(buf, 16, &num1);
        if (rc1 == 0)
        {
            bits_ptr->mask_bits = mask & ~(1 << num1);
            bits_ptr->pos = num1;
        }
    }
#if DEBUG
        pddf_dbg(LED, KERN_ERR "[ WRITE ] ATTR PTR Bits [%s] VALUE:%s mask:0x%x; pos:0x%x\n",
            ptr->dev_attr.attr.name, bits_ptr->bits, bits_ptr->mask_bits, bits_ptr->pos);
#endif
    return (count);
}

/**************************************************************************
 * platform/ attributes
 **************************************************************************/
PDDF_LED_DATA_ATTR(platform, num_psus, S_IWUSR|S_IRUGO, show_pddf_data,
        store_config_data, PDDF_INT_DEC, sizeof(int), (void*)&num_psus);
PDDF_LED_DATA_ATTR(platform, num_fantrays, S_IWUSR|S_IRUGO, show_pddf_data,
        store_config_data, PDDF_INT_DEC, sizeof(int), (void*)&num_fantrays);

struct attribute* attrs_platform[]={
    &pddf_dev_platform_attr_num_psus.dev_attr.attr,
    &pddf_dev_platform_attr_num_fantrays.dev_attr.attr,
    NULL,
};
struct attribute_group attr_group_platform={
    .attrs = attrs_platform,
};

/**************************************************************************
 * led/ attributes
 **************************************************************************/
PDDF_LED_DATA_ATTR(dev, device_name, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_CHAR, NAME_SIZE, (void*)&temp_data.device_name);
PDDF_LED_DATA_ATTR(dev, attr_devtype, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_CHAR, NAME_SIZE, (void*)&temp_data.attr_devtype);
PDDF_LED_DATA_ATTR(dev, attr_devname, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_CHAR, NAME_SIZE, (void*)&temp_data.attr_devname);
PDDF_LED_DATA_ATTR(dev, index, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&temp_data.index);
PDDF_LED_DATA_ATTR(dev, swpld_addr, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&temp_data.swpld_addr);
PDDF_LED_DATA_ATTR(dev, swpld_addr_offset, S_IWUSR|S_IRUGO, show_pddf_data,
            store_pddf_data, PDDF_INT_HEX, sizeof(int), (void*)&temp_data.swpld_addr_offset);
PDDF_LED_DATA_ATTR(dev, dev_ops , S_IWUSR, NULL,
            dev_operation, PDDF_CHAR, NAME_SIZE, (void*)&temp_data);

struct attribute* attrs_dev[] = {
    &pddf_dev_dev_attr_device_name.dev_attr.attr,
    &pddf_dev_dev_attr_attr_devtype.dev_attr.attr,
    &pddf_dev_dev_attr_attr_devname.dev_attr.attr,
    &pddf_dev_dev_attr_index.dev_attr.attr,
    &pddf_dev_dev_attr_swpld_addr.dev_attr.attr,
    &pddf_dev_dev_attr_swpld_addr_offset.dev_attr.attr,
    &pddf_dev_dev_attr_dev_ops.dev_attr.attr,
    NULL,
};

struct attribute_group attr_group_dev = {
    .attrs = attrs_dev,
};

/**************************************************************************
 * state_attr/ attributes
 **************************************************************************/
#define LED_DEV_STATE_ATTR_GROUP(name, func) \
    PDDF_LED_DATA_ATTR(name, bits, S_IWUSR|S_IRUGO, show_pddf_data, \
        store_bits_data, PDDF_CHAR, NAME_SIZE, func.bits.bits); \
    PDDF_LED_DATA_ATTR(name, value, S_IWUSR|S_IRUGO, show_pddf_data, \
        store_pddf_data, PDDF_CHAR, NAME_SIZE, func.value); \
    struct attribute* attrs_##name[]={ \
        &pddf_dev_##name##_attr_bits.dev_attr.attr, \
            &pddf_dev_##name##_attr_value.dev_attr.attr, \
            NULL, \
    }; \
    struct attribute_group attr_group_##name={ \
            .attrs = attrs_##name, \
    }; \


LED_DEV_STATE_ATTR_GROUP(state_attr, (void*)&temp_data.data[0])

/**************************************************************************
 * cur_state/ attributes
 **************************************************************************/
PDDF_LED_DATA_ATTR(cur_state, color, S_IWUSR|S_IRUGO, show_pddf_data,
        store_pddf_data, PDDF_CHAR, NAME_SIZE, (void*)&temp_data.cur_state.color);
PDDF_LED_DATA_ATTR(cur_state, sys_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void*)&sys_led_ops_data);
PDDF_LED_DATA_ATTR(cur_state, loc_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, NAME_SIZE, (void*)&loc_led_ops_data);
PDDF_LED_DATA_ATTR(cur_state, bmc_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void*)&bmc_led_ops_data);
PDDF_LED_DATA_ATTR(cur_state, fan_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void*)&fan_led_ops_data);
PDDF_LED_DATA_ATTR(cur_state, psu_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), NULL);
PDDF_LED_DATA_ATTR(cur_state, psu1_led, S_IRUGO, show_pddf_s3ip_data,
            NULL, PDDF_INT_DEC, sizeof(int), (void *)&psu_led_ops_data[0]);
PDDF_LED_DATA_ATTR(cur_state, psu2_led, S_IRUGO, show_pddf_s3ip_data,
            NULL, PDDF_INT_DEC, sizeof(int), (void *)&psu_led_ops_data[1]);
PDDF_LED_DATA_ATTR(cur_state, fantray1_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[0]);
PDDF_LED_DATA_ATTR(cur_state, fantray2_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[1]);
PDDF_LED_DATA_ATTR(cur_state, fantray3_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[2]);
PDDF_LED_DATA_ATTR(cur_state, fantray4_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[3]);
PDDF_LED_DATA_ATTR(cur_state, fantray5_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[4]);
PDDF_LED_DATA_ATTR(cur_state, fantray6_led, S_IWUSR|S_IRUGO, show_pddf_s3ip_data,
            store_pddf_s3ip_data, PDDF_INT_DEC, sizeof(int), (void *)&fantray_led_ops_data[5]);


struct attribute* attrs_cur_state[] = {
    &pddf_dev_cur_state_attr_color.dev_attr.attr,
    &pddf_dev_cur_state_attr_sys_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_loc_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_bmc_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fan_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_psu_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_psu1_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_psu2_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray1_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray2_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray3_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray4_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray5_led.dev_attr.attr,
    &pddf_dev_cur_state_attr_fantray6_led.dev_attr.attr,
    NULL,
};

struct attribute_group attr_group_cur_state={
    .attrs = attrs_cur_state,
};

/*************************************************************************/
#define KOBJ_FREE(obj) \
    if(obj) kobject_put(obj); \

void free_kobjs(void)
{
    KOBJ_FREE(cur_state_kobj)
    KOBJ_FREE(state_attr_kobj)
    KOBJ_FREE(led_kobj)
    KOBJ_FREE(platform_kobj)
}

int KBOJ_CREATE(char* name, struct kobject* parent, struct kobject** child)
{
    if (parent) {
        *child = kobject_create_and_add(name, parent);
    } else {
        printk(KERN_ERR "PDDF_LED ERROR to create %s kobj; null parent\n", name);
        free_kobjs();
        return (-ENOMEM);
    }
    return (0);
}

int LED_DEV_ATTR_CREATE(struct kobject *kobj, const struct attribute_group *attr, const char* name)
{
    int status = sysfs_create_group(kobj, attr);
    if(status) {
        pddf_dbg(LED, KERN_ERR "Driver ERROR: sysfs_create %s failed rc=%d\n", name, status);
    }
    return (status);
}


static int __init led_init(void) {
    struct kobject *device_kobj;
    pddf_dbg(LED, KERN_INFO "PDDF GENERIC LED MODULE init..\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj)
        return -ENOMEM;

    KBOJ_CREATE("platform", device_kobj, &platform_kobj);
    KBOJ_CREATE("led", device_kobj, &led_kobj);
    KBOJ_CREATE("state_attr", led_kobj, &state_attr_kobj);
    KBOJ_CREATE("cur_state", led_kobj, &cur_state_kobj);

    LED_DEV_ATTR_CREATE(platform_kobj, &attr_group_platform, "attr_group_platform");
    LED_DEV_ATTR_CREATE(led_kobj, &attr_group_dev, "attr_group_dev");
    LED_DEV_ATTR_CREATE(state_attr_kobj, &attr_group_state_attr, "attr_group_state_attr");
    LED_DEV_ATTR_CREATE(cur_state_kobj, &attr_group_cur_state, "attr_group_cur_state");
    return (0);
}

static void __exit led_exit(void) {
    pddf_dbg(LED, "PDDF GENERIC LED MODULE exit..\n");
    free_kobjs();
}

module_init(led_init);
module_exit(led_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("led driver");
MODULE_LICENSE("GPL");
