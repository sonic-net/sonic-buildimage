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
#include "uart_sys_defs.h"


/*#define SYS_DEBUG*/
#ifdef SYS_DEBUG
#define sys_dbg(...) printk(__VA_ARGS__)
#else
#define sys_dbg(...)
#endif

#define UART_TTYS1_OFFSET   (0x2f8)   /* /dev/ttyS1 */
#define MAX_UART_ORDER_SIZE (64)
#define MAX_RW_DELAY (800000)
#define MAX_SINGLE_R_DELAY (20000)
#define UART_TX_WAIT_MIN_US (50)
#define UART_TX_WAIT_MAX_US (100)

extern struct mutex uart_lock;
extern void *get_device_table(char *name); 

static const unsigned short ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

static void uart_read_lock(void)
{
    mutex_lock(&uart_lock);
}

static void uart_read_unlock(void)
{
    mutex_unlock(&uart_lock);
}

static unsigned short CRC16(u8 *q, int len)
{
    unsigned short crc = 0;

    while (len-- > 0)
        crc = (crc << 8) ^ ccitt_table[((crc >> 8) ^ (*q++ & 0xff)) & 0xff];
    return crc;
}

static int cmp_read_crc(u8 *ptr_str, int len)
{
    if (len > 2)
    {
        u8 crc_read[2];
        crc_read[0] = ptr_str[len - 2];
        crc_read[1] = ptr_str[len - 1];
	    unsigned short read_crc_cal = CRC16(ptr_str, len - 2);
	    //printf("crc1 = %x %x, crc2 = %x %x\n", crc_read[0], crc_read[1], (read_crc_cal&0xff00)>>8, (read_crc_cal&0xff));
	    if(crc_read[0] == ((read_crc_cal&0xff00)>>8) && crc_read[1] == (read_crc_cal&0xff))
	    	return 0;
	    else
	    	return -1;
    }
    else
        return -1;
}

void set_bmc_data(const char *command, const char *fst_command, const char *sec_command, const u8 vnum_command)
{
    unsigned int n = 0, size = 0, written = 0;
	u8 status = 0;
	unsigned int loop_times = 0;
	char uart_order[BUFFERSIZE] = {0};
	unsigned short crc_value = 0;
	written = scnprintf(uart_order, BUFFERSIZE, "uart_");
	if(vnum_command == 1)
		written += scnprintf(uart_order + written, BUFFERSIZE - written,
                            "%s", command);
	else if(vnum_command == 2)
		written += scnprintf(uart_order + written, BUFFERSIZE - written,
                            "%s_%s", command, fst_command);
	else if(vnum_command == 3)
		written += scnprintf(uart_order + written, BUFFERSIZE - written,
                            "%s_%s_%s", command, fst_command, sec_command);

    crc_value = CRC16(uart_order, written);
    written += scnprintf(uart_order + written, BUFFERSIZE - written,
                        "_%x%x#", (crc_value & 0xff00)>>8, crc_value & 0xff);

    uart_read_lock();
	size = written;
	do{
		while(!(inb(UART_TTYS1_OFFSET + UART_LSR) & UART_LSR_TEMT))
		{
			usleep_range(UART_TX_WAIT_MIN_US, UART_TX_WAIT_MAX_US);
		}
		outb(uart_order[n] & 0xff,UART_TTYS1_OFFSET);
		n++;
	}while(size != n);
		
	while(loop_times < MAX_RW_DELAY) {
		status = inb(UART_TTYS1_OFFSET + UART_LSR);
		if (status & UART_LSR_DR)
		{
			break;
		}
		loop_times++;
	}
    uart_read_unlock();
	return;
}
EXPORT_SYMBOL(set_bmc_data);

void get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *bmc_read_data)
{
    unsigned int n = 0, size = 0, r_data_num = 0, i = 0, written = 0;
	u8 status = 0;
	unsigned int loop_times = 0;
	u8 temp[BUFFERSIZE]={0};
	char uart_order[MAX_UART_ORDER_SIZE] = {0};
	unsigned short crc_value = 0;
    written = scnprintf(uart_order, MAX_UART_ORDER_SIZE, "uart_");
	if(vnum_command == 1)
        written += scnprintf(uart_order + written, MAX_UART_ORDER_SIZE - written,
                            "0x%02x", command);
	else if(vnum_command == 2)
        written += scnprintf(uart_order + written, MAX_UART_ORDER_SIZE - written,
                            "0x%02x_0x%02x", command, fst_command);
	else if(vnum_command == 3)
        written += scnprintf(uart_order + written, MAX_UART_ORDER_SIZE - written,
                            "0x%02x_0x%02x_0x%02x", command, fst_command, sec_command);

	crc_value = CRC16(uart_order, written);
    written += scnprintf(uart_order + written, MAX_UART_ORDER_SIZE - written,
                        "_%x%x#", (crc_value & 0xff00)>>8, crc_value & 0xff);

    uart_read_lock();
	size = written;
	do{
		while(!(inb(UART_TTYS1_OFFSET + UART_LSR) & UART_LSR_TEMT))
		{
			usleep_range(UART_TX_WAIT_MIN_US, UART_TX_WAIT_MAX_US);
		}
		outb(uart_order[n] & 0xff,UART_TTYS1_OFFSET);
		n++;
	}while(size != n);
		
	while(loop_times < MAX_RW_DELAY) {
		status = inb(UART_TTYS1_OFFSET + UART_LSR);
		if (status & UART_LSR_DR)
		{
			break;
		}
		loop_times++;
	}
	if(loop_times == MAX_RW_DELAY)
	{
        printk(KERN_WARNING "%s: UART read timeout\n", __FUNCTION__);
		goto _exit;
	}

	while(1){
		loop_times = 0;
		while(loop_times < MAX_SINGLE_R_DELAY) 
		{
			status = inb(UART_TTYS1_OFFSET + UART_LSR);
			if (status & UART_LSR_DR)
			{
				break;
			}
			loop_times++;
		}
		if(loop_times == MAX_SINGLE_R_DELAY)
		{
			break;
		}
		temp[r_data_num] = inb(UART_TTYS1_OFFSET);
		r_data_num++;
		
		if (r_data_num == BUFFERSIZE - 1)
		{
			break;
		}
	}
	
	if(r_data_num > 2){
		if(!cmp_read_crc(temp, r_data_num)){
			for(i = 0; i < r_data_num - 2; i++)
				bmc_read_data->block[i] = temp[i];
		}
	}

_exit:
    uart_read_unlock();
	return;
}
EXPORT_SYMBOL(get_bmc_data);

int sys_update_hw(struct device *dev, struct sys_attr_info *info, SYS_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    SYS_SYSFS_ATTR_DATA *sysfs_attr_data = NULL;


    mutex_lock(&info->update_lock);

    sysfs_attr_data = udata->access_data;
    if (sysfs_attr_data->pre_set != NULL)
    {
        status = (sysfs_attr_data->pre_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }
    if (sysfs_attr_data->do_set != NULL)
    {
        status = (sysfs_attr_data->do_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

    }
    if (sysfs_attr_data->post_set != NULL)
    {
        status = (sysfs_attr_data->post_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }

    mutex_unlock(&info->update_lock);

    return 0;
}


int sys_update_attr(struct device *dev, struct sys_attr_info *data, SYS_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    SYS_SYSFS_ATTR_DATA *sysfs_attr_data=NULL;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid) 
    {
        dev_dbg(&client->dev, "Starting update for %s\n", data->name);

        sysfs_attr_data = udata->access_data;
        if (sysfs_attr_data->pre_get != NULL)
        {
            status = (sysfs_attr_data->pre_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }
        if (sysfs_attr_data->do_get != NULL)
        {
            status = (sysfs_attr_data->do_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

        }
        if (sysfs_attr_data->post_get != NULL)
        {
            status = (sysfs_attr_data->post_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);
    return 0;
}


ssize_t sys_show_default(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct sys_data *data = i2c_get_clientdata(client);
    SYS_PDATA *pdata = (SYS_PDATA *)(client->dev.platform_data);
    SYS_DATA_ATTR *usr_data = NULL;
    struct sys_attr_info *sysfs_attr_info = NULL;
    int i, status=0;

    for (i=0;i<data->num_attr;i++)
    {
        if ( strncmp(attr->dev_attr.attr.name, pdata->sys_attrs[i].aname, ATTR_NAME_LEN) == 0 ) 
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->sys_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL)
    {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    sys_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index)
    {
        case SYS_CPLD_VERSION:
        case SYS_BOM_VERSION:
        case SYS_PCB_VERSION:
        case SYS_HEARTBEAT_DEFAULT:
            return scnprintf(buf, PAGE_SIZE, "%d\n", sysfs_attr_info->val.charval);
            break;
        case SYS_BMC_VERSION:
            return scnprintf(buf, PAGE_SIZE, "%s\n", sysfs_attr_info->val.strval);
            break;
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

exit:
    return scnprintf(buf, PAGE_SIZE, "%d\n", status);
}


ssize_t sys_store_default(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct sys_data *data = i2c_get_clientdata(client);
    SYS_PDATA *pdata = (SYS_PDATA *)(client->dev.platform_data);
    SYS_DATA_ATTR *usr_data = NULL;
    struct sys_attr_info *sysfs_attr_info = NULL;
    int i;
    u32 set_value = 0;

    for (i=0;i<data->num_attr;i++)
    {
        if (strncmp(data->attr_info[i].name, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0 && strncmp(pdata->sys_attrs[i].aname, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0)
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->sys_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL) {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    switch(attr->index)
    {
        case SYS_ENABLE_SET:
            if (kstrtoint(buf, 10, &set_value))
            {
                printk(KERN_ERR "%s: Unable to convert string into value for %s\n", __FUNCTION__, usr_data->aname);
                return -EINVAL;
            }
            if (set_value != 1)
            {
                printk(KERN_ERR "%s: User set an illegal parameter:%d\n", __FUNCTION__, set_value);
                return -EINVAL;
            }

            sysfs_attr_info->val.intval = set_value;
            break;
        case SYS_RTC_TIME_SET:
            memset(sysfs_attr_info->val.strval, 0, sizeof(sysfs_attr_info->val.strval)); 
            strlcpy(sysfs_attr_info->val.strval, buf, TIME_SIZE);
            break;
        default:
            printk(KERN_ERR "%s: Unable to find the attr index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

    sys_update_hw(dev, sysfs_attr_info, usr_data);

exit:
    return count;
}


int sys_get_bmc_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u8 byte = info->byte;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};

    get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);

    scnprintf(padata->val.strval, sizeof(padata->val.strval), "V%d.%dR%02d", bmc_read_data.block[byte], bmc_read_data.block[byte + 1], bmc_read_data.block[byte + 2]);
    return 0;
}

int sys_get_cpld_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;

    if (strncmp(info->devname, "BMC", ATTR_NAME_LEN) == 0)
    {
        u8 command = info->cmd;
        u8 sub_command_1 = info->subcmd1;
        u8 sub_command_2 = info->subcmd2;
        u8 byte = info->byte;
        union i2c_smbus_data bmc_read_data = {.block={0x00}};

        get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
        padata->val.charval = bmc_read_data.block[byte];
    }
    else
    {
        u8 version = info->version;
        struct i2c_client *client_ptr=NULL;
        client_ptr = (struct i2c_client *)get_device_table(info->devname);
    
        version = i2c_smbus_read_byte_data(client_ptr, version);
        padata->val.charval = version & 0xff;
    }
    return 0;
}

int sys_get_bom_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u8 byte = info->byte;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};

    get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
    padata->val.charval = bmc_read_data.block[byte];
    return 0;
}

int sys_get_pcb_version(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u8 byte = info->byte;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};

    get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
    padata->val.charval = bmc_read_data.block[byte];
    return 0;
}

int sys_heartbeat_default(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u8 byte = info->byte;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};

    get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
    padata->val.charval = bmc_read_data.block[byte];
    return 0;
}

int sys_set_enable(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u32 i = padata->val.intval;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};
    
    get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);

    return 0;
}

int sys_set_rtc_time(struct i2c_client *client, SYS_DATA_ATTR *info, void *data)
{
    struct sys_attr_info *padata = (struct sys_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 command_str[TIME_SIZE] = {0};
    u8 sub_command_1_str[TIME_SIZE] = {0};
    u8 sub_command_2_str[TIME_SIZE] = {0};

    scnprintf(command_str, sizeof(command_str), "0x%02x", command);
    scnprintf(sub_command_1_str, sizeof(sub_command_1_str), "0x%02x",sub_command_1);
    strlcpy(sub_command_2_str, padata->val.strval, TIME_SIZE);
    set_bmc_data(command_str, sub_command_1_str, sub_command_2_str, 3);
    return 0;
}
