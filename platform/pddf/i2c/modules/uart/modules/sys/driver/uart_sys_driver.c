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
#include <linux/types.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/i2c-dev.h>
#include "pddf_client_defs.h"
#include "uart_sys_defs.h"
#include "uart_sys_api.h"

#define UART_TTYS1_OFFSET   (0x2f8)
#define BMC_VER_CMD (0xd)
#define BMC_VER_SUBCMD1 (0xaa)
#define BMC_VER_SUBCMD2 (0xaa)

extern void get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *bmc_read_data);

struct mutex uart_lock;
u8 calmode = 0;
EXPORT_SYMBOL(calmode);

static int uart_probe(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    SYS_PDATA *pdata = (SYS_PDATA *)(client->dev.platform_data); 
    if (pdata->sirq == 3 || pdata->sirq == 11)
    disable_irq(pdata->sirq);

	outb(0, UART_TTYS1_OFFSET + UART_IER);        /* Turn off interrupts - Port1 */ 

 	/*      PORT 1 - Communication Settings         */
	outb(UART_LCR_CONF_MODE_A, UART_TTYS1_OFFSET + UART_LCR);  /* SET DLAB ON */

	outb(0x0c, UART_TTYS1_OFFSET + UART_DLL);

	/* Set Baud rate - Divisor Latch Low Byte */
	/*         0x01 = 115,200 BPS */
	/*         0x02 =  57,600 BPS */
	/*         0x06 =  19,200 BPS */
	/*         0x0C =   9,600 BPS */
	/*         0x18 =   4,800 BPS */
	/*         0x30 =   2,400 BPS */

	outb(0x0, UART_TTYS1_OFFSET + UART_DLM);  /* Set Baud rate - Divisor Latch High Byte */
	
	outb(UART_LCR_CONF_MODE_B, UART_TTYS1_OFFSET + UART_LCR);  
	outb(0x0, UART_TTYS1_OFFSET + UART_EFR);
	outb(0x0, UART_TTYS1_OFFSET + UART_LCR);
	/* SET DLAB OFF */
	outb(UART_FCR_ENABLE_FIFO, UART_TTYS1_OFFSET + UART_FCR);
	
	outb(UART_LCR_WLEN8, UART_TTYS1_OFFSET + UART_LCR);  /* 8 Bits, No Parity, 1 Stop Bit */
	
	outb(UART_FCR_ENABLE_FIFO|UART_FCR_DMA_SELECT|UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT, UART_TTYS1_OFFSET + UART_FCR);  
    //outb(UART_FCR_ENABLE_FIFO|UART_FCR_R_TRIG_11|UART_FCR_T_TRIG_11|UART_FCR_CLEAR_RCVR|UART_FCR_CLEAR_XMIT, UART_TTYS1_OFFSET + UART_FCR);
	/* FIFO Control Register */
	
	outb(UART_MCR_OUT1 | UART_MCR_OUT2 | UART_MCR_RTS | UART_MCR_DTR, UART_TTYS1_OFFSET + UART_MCR);  
    //outb(UART_MCR_OUT2|UART_MCR_RTS|UART_MCR_DTR, UART_TTYS1_OFFSET + UART_MCR);
	/* Turn on DTR, RTS, and OUT2 */
	outb(0xf, UART_TTYS1_OFFSET + UART_IER);  
    //outb(UART_IER_RDI | UART_IER_THRI, UART_TTYS1_OFFSET + UART_IER);
	/* Interrupt when data received */

	return 0;
}

static int calmode_get(struct i2c_client *client, const struct i2c_device_id *dev_id)
{
    int major, minor, patch;
    union i2c_smbus_data bmc_version_data = {.block={0x00}};

    get_bmc_data(BMC_VER_CMD, BMC_VER_SUBCMD1, BMC_VER_SUBCMD2, 3, &bmc_version_data);
    major = bmc_version_data.block[1];
    minor = bmc_version_data.block[2];
    patch = bmc_version_data.block[3];

    if (bmc_version_data.block[0] == 0)
    calmode = 0;
    else if (major > 1 || (major == 1 && minor > 3) || (major == 1 && minor == 3 && patch >= 1)) 
    calmode = 2;
    else 
    calmode = 1;

    // printk("calmode = %d", calmode);

    return 0;
}

static int uart_remove(struct i2c_client *client)
{
    SYS_PDATA *pdata = (SYS_PDATA *)(client->dev.platform_data); 
    if (pdata->sirq == 3 || pdata->sirq == 11)
    enable_irq(pdata->sirq);

    return 0;
}

struct pddf_ops_t pddf_sys_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = uart_probe,
	.post_probe = calmode_get,

	.pre_remove = uart_remove,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};

SYS_SYSFS_ATTR_DATA access_uart_enable_set = {SYS_ENABLE_SET, S_IRUGO | S_IWUSR, NULL, NULL, NULL, NULL, sys_store_default, NULL, sys_set_enable, NULL};
EXPORT_SYMBOL(access_uart_enable_set);

SYS_SYSFS_ATTR_DATA access_uart_bmc_version = {SYS_BMC_VERSION, S_IRUGO, sys_show_default, NULL, sys_get_bmc_version, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_bmc_version);

SYS_SYSFS_ATTR_DATA access_uart_cpld_version = {SYS_CPLD_VERSION, S_IRUGO, sys_show_default, NULL, sys_get_cpld_version, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_cpld_version);

SYS_SYSFS_ATTR_DATA access_uart_bom_version = {SYS_BOM_VERSION, S_IRUGO, sys_show_default, NULL, sys_get_bom_version, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_bom_version);

SYS_SYSFS_ATTR_DATA access_uart_pcb_version = {SYS_PCB_VERSION, S_IRUGO, sys_show_default, NULL, sys_get_pcb_version, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_pcb_version);

SYS_SYSFS_ATTR_DATA access_uart_heartbeat_default = {SYS_HEARTBEAT_DEFAULT, S_IRUGO, sys_show_default, NULL, sys_heartbeat_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_heartbeat_default);

SYS_SYSFS_ATTR_DATA access_uart_rtc_time_set = {SYS_RTC_TIME_SET, S_IRUGO | S_IWUSR, NULL, NULL, NULL, NULL, sys_store_default, NULL, sys_set_rtc_time, NULL};
EXPORT_SYMBOL(access_uart_rtc_time_set);

SYS_SYSFS_ATTR_DATA_ENTRY sys_sysfs_attr_data_tbl[]=
{
	{ "shutdown_set", &access_uart_enable_set},
	{ "bmc_version", &access_uart_bmc_version},
	{ "cpld1_version", &access_uart_cpld_version},
	{ "cpld2_version", &access_uart_cpld_version},
	{ "cpld3_version", &access_uart_cpld_version},
	{ "cpld4_version", &access_uart_cpld_version},
	{ "bom1_version", &access_uart_bom_version},
	{ "bom2_version", &access_uart_bom_version},
	{ "pcb1_version", &access_uart_pcb_version},
	{ "pcb2_version", &access_uart_pcb_version},
	{ "heartbeat_close", &access_uart_heartbeat_default},
	{ "heartbeat_open", &access_uart_heartbeat_default},
	{ "heartbeat_set", &access_uart_heartbeat_default},
	{ "rtc_time_set", &access_uart_rtc_time_set},
    { "cp2112_1_reset", &access_uart_enable_set},
};

void *get_uart_sys_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(sys_sysfs_attr_data_tbl)/sizeof(sys_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, sys_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &sys_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_sys_access_data);

static int sys_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct sys_data *data;
    int status = 0;
    int i,num, j = 0;
    SYS_PDATA *sys_platform_data;
    SYS_DATA_ATTR *data_attr;
    SYS_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_SYS", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_sys_ops.pre_probe) 
    {
        status = (pddf_sys_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    data = kzalloc(sizeof(struct sys_data), GFP_KERNEL); 
    if (!data) {
        status = -ENOMEM; 
        goto exit;
    }

    i2c_set_clientdata(client, data); 
    
    mutex_init(&uart_lock); 

    dev_info(&client->dev, "chip found\n"); 
    
    /* Take control of the platform data */
    sys_platform_data = (SYS_PDATA *)(client->dev.platform_data);
    num = sys_platform_data->len; 
    data->index = sys_platform_data->idx; 
    data->sirq = sys_platform_data->sirq; 
    data->num_attr = num;

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = sys_platform_data->sys_attrs + i; 
        sysfs_data_entry = get_uart_sys_access_data(data_attr->aname);
        if (sysfs_data_entry == NULL)
		{
			printk(KERN_ERR "%s: Wrong attribute name provided by user '%s'\n", __FUNCTION__, data_attr->aname);
			continue;
		}

        dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
		dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
		strlcpy((char *)dy_ptr->dev_attr.attr.name, data_attr->aname, ATTR_NAME_LEN);
		dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
		dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
		dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
		dy_ptr->index = sysfs_data_entry->a_ptr->index;
		
		data->sys_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

    }
    data->sys_attribute_list[i+j] = NULL;
	data->sys_attribute_group.attrs = data->sys_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->sys_attribute_group); 
    if (status) {
        goto exit_free; 
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL);  
    if (IS_ERR(data->hwmon_dev)) { 
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove; 
    }

    dev_info(&client->dev, "%s: sys '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_sys_ops.post_probe)
    {
        status = (pddf_sys_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove: 
    sysfs_remove_group(&client->dev.kobj, &data->sys_attribute_group);
exit_free:
    /* Free all the allocated attributes */
    for (i=0;data->sys_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->sys_attribute_list[i];
		kfree(ptr);
		data->sys_attribute_list[i] = NULL;
		pddf_dbg("UART_SYS", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}
EXPORT_SYMBOL(uart_lock);

static void sys_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct sys_data *data = i2c_get_clientdata(client); 
    SYS_PDATA *platdata = (SYS_PDATA *)client->dev.platform_data; 
    SYS_DATA_ATTR *platdata_sub = platdata->sys_attrs;
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_sys_ops.pre_remove)
    {
        ret = (pddf_sys_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "SYS pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);   
    sysfs_remove_group(&client->dev.kobj, &data->sys_attribute_group);
    for (i=0; data->sys_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->sys_attribute_list[i];
		kfree(ptr);
		data->sys_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_SYS", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data); 

    if (platdata_sub) { 
        pddf_dbg("UART_SYS", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) { 
        pddf_dbg("UART_SYS", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_sys_ops.post_remove)
    {
        ret = (pddf_sys_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "SYS post_remove function failed\n");
    }
}

enum sys_intf 
{
    SYS_CTRL_INTF,
};

static const struct i2c_device_id sys_ids[] = { 
    { "sys_uart", SYS_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, sys_ids); 

static struct i2c_driver sys_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "sys_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = sys_probe,
    .remove       = sys_remove,
    .id_table     = sys_ids, 
};


int sys_uart_init(void)
{
    int ret = 0;

    if (pddf_sys_ops.pre_init) 
    {
        ret = (pddf_sys_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_SYS", KERN_ERR "UART SYS DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&sys_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_sys_ops.post_init) 
    {
        ret = (pddf_sys_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(sys_uart_init);

void __exit sys_uart_exit(void)
{
    pddf_dbg("UART_SYS", "UART SYS DRIVER.. exit\n");
    if (pddf_sys_ops.pre_exit) (pddf_sys_ops.pre_exit)();
    i2c_del_driver(&sys_driver);
    if (pddf_sys_ops.post_exit) (pddf_sys_ops.post_exit)();

}
EXPORT_SYMBOL(sys_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for sys information");
MODULE_LICENSE("GPL");

module_init(sys_uart_init); 
module_exit(sys_uart_exit);
