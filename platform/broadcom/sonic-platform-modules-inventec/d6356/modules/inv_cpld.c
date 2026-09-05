/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/kthread.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>

/* definition */
#define CPLD_INFO_OFFSET		0x00
#define CPLD_BIOSCS_OFFSET		0x04
#define CPLD_CTL_OFFSET			0x0C
#define CPLD_SYSLED_OFFSET		0x0E
#define CPLD_LED_OFFSET			0x2E
#define CPLD_INT_OFFSET			0x30
#define CPLD_INTMASK_OFFSET		0x31
#define CPLD_INT2_OFFSET		0x32
#define CPLD_INTMASK2_OFFSET		0x33
#define CPLD_PSU_OFFSET			0x40
#define CPLD_POWERSTATUS_OFFSET		0x41
#define CPLD_PWM_OFFSET			0x50
#define CPLD_RPM_OFFSET			0x55
#define CPLD_FANSTATUS_OFFSET		0x69
#define CPLD_FANLED_OFFSET		0x6B
#define CPLD_RESETBUTTONSTATUS_OFFSET	0x75
#define CPLD_RSTCAUSE_OFFSET		0x76
#define CPLD_WATCHDOGCOUNTER_OFFSET	0x77
#define CPLD_WATCHDOGCONFIG_OFFSET	0x78
#define CPLD_WATCHDOGENABLE_OFFSET	0x79
#define CPLD_PANICCODE_OFFSET		0x7E
#define CPLD2_ADDRESS			0x33

#define FAN_NUM				4
static u8 hasCPLD2 = 1;
static struct i2c_client *client2;

/* Each client has this additional data */
struct cpld_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	u8 diag;
	struct task_struct *tsk;
};

/*-----------------------------------------------------------------------*/
static ssize_t cpld_i2c_read(struct i2c_client *client, u8 *buf, u8 offset, size_t count)
{
	int i;
	s32 temp = 0;
	
	for(i=0; i<count; i++)
	{
		temp = i2c_smbus_read_byte_data(client, offset+i);
		if(temp<0) 
		{
//			printk("CPLD Read fail! Error Code: %d\n",temp);
			return 0;
		}
		buf[i] = temp & 0xff;
	}
	return count;
}

static ssize_t cpld_i2c_write(struct i2c_client *client, char *buf, unsigned offset, size_t count)
{   
	int i;
	
	for(i=0; i<count; i++)
	{
		i2c_smbus_write_byte_data(client, offset+i, buf[i]);
	}
	return count;
}

/*-----------------------IPMI API--------------------------------------*/
#define MAX_IPMI_RECV_LENGTH	0xFF
#define IPMI_MAX_INTF		4
#define NETFN_OEM 0x30
#define CMD_GETDATA 0x31
#define CMD_SETDATA 0x32
#define IPMI_DIAGFLAG_OFFSET 0x00
#define IPMI_SWITCHTEMP_OFFSET 0x02

struct ipmi_result{
        char result[MAX_IPMI_RECV_LENGTH];
        int result_length;
};

DEFINE_MUTEX(ipmi_mutex);
DEFINE_MUTEX(ipmi2_mutex);
static struct ipmi_result ipmiresult;
static ipmi_user_t ipmi_mh_user = NULL;
static void msg_handler(struct ipmi_recv_msg *msg,void* handler_data);
static struct ipmi_user_hndl ipmi_hndlrs = {   .ipmi_recv_hndl = msg_handler,};

static atomic_t dummy_count = ATOMIC_INIT(0);
static void dummy_smi_free(struct ipmi_smi_msg *msg)
{
        atomic_dec(&dummy_count);
}
static void dummy_recv_free(struct ipmi_recv_msg *msg)
{
        atomic_dec(&dummy_count);
}
static struct ipmi_smi_msg halt_smi_msg = {
        .done = dummy_smi_free
};
static struct ipmi_recv_msg halt_recv_msg = {
        .done = dummy_recv_free
};

static void msg_handler(struct ipmi_recv_msg *recv_msg,void* handler_data)
{
    struct ipmi_result *msg_result = recv_msg->user_msg_data;

    if(recv_msg->msg.data[0]==0 && recv_msg->msg.data_len>0) {
        msg_result->result_length=recv_msg->msg.data_len-1;
        memcpy(msg_result->result, &recv_msg->msg.data[1], recv_msg->msg.data_len-1);
    }
    ipmi_free_recv_msg(recv_msg);
    mutex_unlock(&ipmi_mutex);

    return;
}

int start_ipmi_command(char NetFn, char cmd,char *data,int data_length, char* result, int* result_length)
{
    int rv=0,i;
    int timeout;

    //wait previous command finish at least 50msec
    timeout=50;
    while((mutex_is_locked(&ipmi_mutex) == 1 || (mutex_is_locked(&ipmi2_mutex) == 1)) && (--timeout)>0) { usleep_range(1000,1010); }
    if(timeout==0) { return -1; }
    mutex_lock(&ipmi_mutex);
    mutex_lock(&ipmi2_mutex);

    if(ipmi_mh_user == NULL) {
        for (i=0,rv=1; i<IPMI_MAX_INTF && rv; i++) {
            rv = ipmi_create_user(i, &ipmi_hndlrs, NULL, &ipmi_mh_user);
            }
    }

    if (rv < 0) {
        mutex_unlock(&ipmi_mutex);
        mutex_unlock(&ipmi2_mutex);
        return rv;
    }
    else {
        struct  ipmi_system_interface_addr addr;
        struct  kernel_ipmi_msg msg;
        uint8_t msg_data[data_length];

        memcpy(msg_data,data,data_length);
        addr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
        addr.channel = IPMI_BMC_CHANNEL;
        addr.lun = 0;

        msg.netfn = NetFn;
        msg.cmd = cmd;
        msg.data = msg_data;
        msg.data_len = data_length;

        rv = ipmi_request_supply_msgs(ipmi_mh_user, (struct ipmi_addr*)&addr, 0,&msg, &ipmiresult, &halt_smi_msg, &halt_recv_msg, 0);
        if (rv) {
                mutex_unlock(&ipmi_mutex);
                mutex_unlock(&ipmi2_mutex);
                return -6;
        }

        //skip command if 1sec no response from remote
        timeout=1000;
        while(mutex_is_locked(&ipmi_mutex) == 1 && (--timeout)>0) { usleep_range(1000,1100);}
        if(timeout==0) {
                mutex_unlock(&ipmi2_mutex);
                return -1;
        }
        else {
                *result_length=ipmiresult.result_length;
                memcpy(result,ipmiresult.result,*result_length);
                mutex_unlock(&ipmi2_mutex);
                return 0;
        }
    }
    return 0;
}
EXPORT_SYMBOL(start_ipmi_command);

static int cpld_thread(void *p)
{
#ifndef XORP
	struct i2c_client *client = p;

	u8 byte[9];
	uint8_t result[MAX_IPMI_RECV_LENGTH];
	int result_len=0;
	
	//Handle LED control by the driver
	byte[0]=0x01;
	cpld_i2c_write(client, byte, CPLD_CTL_OFFSET, 1);

	//Disable BMC Watchdog
	byte[0]=0x04;
	byte[1]=0x00;
	byte[2]=0x00;
	byte[3]=0x00;
	byte[4]=0x2C;
	byte[5]=0x01;
	start_ipmi_command(0x06, 0x24, byte, 6, result, &result_len);
#endif
	return 0;
}
/*-----------------------------------------------------------------------*/
/* sysfs attributes for hwmon */
static ssize_t show_info(struct device *dev, struct device_attribute *da,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;

	u8 byte[4] = {0,0,0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, byte, CPLD_INFO_OFFSET, 4);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	sprintf (buf, "The CPLD release date is %02d/%02d/%d.\n", 
	byte[2] & 0xf, (byte[3] & 0x1f), 2014+(byte[2] >> 4));	/* mm/dd/yyyy*/
	sprintf (buf, "%sThe PCB  version is %X\n", buf,  byte[0]&0xf);
	sprintf (buf, "%sThe CPLD version is %d.%d\n", buf, byte[1]>>4, byte[1]&0xf);

	if(hasCPLD2) {
		mutex_lock(&data->update_lock);
		cpld_i2c_read(client2, byte, CPLD_INFO_OFFSET, 4);
		mutex_unlock(&data->update_lock);

		sprintf (buf, "%s\nThe CPLD2 release date is %02d/%02d/%d.\n", buf, 
		byte[2] & 0xf, (byte[3] & 0x1f), 2014+(byte[2] >> 4));	/* mm/dd/yyyy*/
		sprintf (buf, "%sThe CPLD2 version is %d.%d\n", buf, byte[1]>>4, byte[1]&0xf);
	}
	
	return strlen(buf);
}

static char* powerstatus_str[] = {
    "Failed",     //0
    "Good",       //1
};

static ssize_t show_powerstatus(struct device *dev, struct device_attribute *da,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, byte, CPLD_POWERSTATUS_OFFSET, 2);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	sprintf (buf, "PGD_P5V_STBY: %s\n", powerstatus_str[(byte[0]>>7) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_STBY: %s\n", buf, powerstatus_str[(byte[0]>>6) & 0x01]);;
	sprintf (buf, "%sPGD_P1V8_A: %s\n", buf, powerstatus_str[(byte[0]>>4) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_SYS: %s\n", buf, powerstatus_str[(byte[0]>>3) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_A: %s\n", buf, powerstatus_str[(byte[0]>>2) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_B: %s\n", buf, powerstatus_str[(byte[0]>>1) & 0x01]);
	sprintf (buf, "%sPGD_P1V2: %s\n", buf, powerstatus_str[(byte[0]>>0) & 0x01]);
	sprintf (buf, "%sPGD_P0V8_A: %s\n", buf,powerstatus_str[(byte[1]>>7) & 0x01]);
	sprintf (buf, "%sPGD_P0V89_ROV: %s\n", buf, powerstatus_str[(byte[1]>>6) & 0x01]);
	sprintf (buf, "%sSW_PWR_READY: %s\n", buf, powerstatus_str[(byte[1]>>3) & 0x01]);
	sprintf (buf, "%sCPU_STBY_PWROK: %s\n", buf, powerstatus_str[(byte[1]>>0) & 0x01]);

	return strlen(buf);
}

static ssize_t show_diag(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	uint8_t ipmisend[]= { IPMI_DIAGFLAG_OFFSET, 1};
	uint8_t result[MAX_IPMI_RECV_LENGTH];
	int result_len=0;
	start_ipmi_command(NETFN_OEM, CMD_GETDATA,ipmisend, 2, result, &result_len);
	data->diag = (result[0] & 0x80) !=0;
	return sprintf (buf, "%d\n", data->diag);
}

static ssize_t set_diag(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	uint8_t ipmisend[]= { IPMI_DIAGFLAG_OFFSET, 0x80};
	uint8_t result[MAX_IPMI_RECV_LENGTH];
	int result_len=0;
	u8 diag = simple_strtol(buf, NULL, 10);
	data->diag = diag?1:0;
	if (data->diag==0) ipmisend[1] = 0x00;

	start_ipmi_command(NETFN_OEM, CMD_SETDATA,ipmisend, 2, result, &result_len);

	return count;
}

static ssize_t show_thermal(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	uint8_t ipmisend[]= { IPMI_SWITCHTEMP_OFFSET, 1};
	uint8_t result[MAX_IPMI_RECV_LENGTH];
	int result_len=0;
	start_ipmi_command(NETFN_OEM, CMD_GETDATA,ipmisend, 2, result, &result_len);
	return sprintf(buf, "%d\n", result[0] * 1000 );
}

static char* interrupt_str[] = {
    "CPU_SEN_ALERT_N", 		//0
    "EXT_USB_OC_N",      	//1
    "",  			//2
    "",		    		//3
    "PLD_SEN5_ALERT_N",		//4
    "PLD_SEN4_ALERT_N",     	//5
    "PLD_SEN3_ALERT_N",  	//6
    "UCD90160_TEMP_INT_N",    	//7
    "RSTBTN_INT_N",		//8
    "WDT_IRQ_N",		//9
};

static ssize_t show_interrupt(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte[4] = {0,0,0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, byte, CPLD_INT_OFFSET, 4);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	sprintf (buf, "0x%02X 0x%02X:", byte[0],byte[2]);
	if(byte[0]==0xff && byte[2]==0x07) sprintf (buf, "%sNone",buf);
	if(!(byte[0]&0x01)) sprintf (buf, "%s%s ",buf,interrupt_str[0]);
	if(!(byte[0]&0x02)) sprintf (buf, "%s%s ",buf,interrupt_str[1]);
	if(!(byte[0]&0x10)) sprintf (buf, "%s%s ",buf,interrupt_str[4]);
	if(!(byte[0]&0x20)) sprintf (buf, "%s%s ",buf,interrupt_str[5]);
	if(!(byte[0]&0x40)) sprintf (buf, "%s%s ",buf,interrupt_str[6]);
	if(!(byte[0]&0x80)) sprintf (buf, "%s%s ",buf,interrupt_str[7]);
	if(!(byte[2]&0x01)) sprintf (buf, "%s%s%s ",buf,interrupt_str[8] ,(byte[3]&0x01)?"(Blocked)":"");
	if(!(byte[2]&0x02)) sprintf (buf, "%s%s%s ",buf,interrupt_str[9] ,(byte[3]&0x02)?"(Blocked)":"");

	return sprintf (buf, "%s\n", buf);
}

static char* bios_str[] = {
    "BIOS1",     	//0
    "BIOS2",      	//1
};

static ssize_t show_bios_cs(struct device *dev, struct device_attribute *da,
                         char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, &byte, CPLD_BIOSCS_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	byte &= 0x01;

	return sprintf (buf, "%d:%s\n", byte,bios_str[byte]);
}

static ssize_t set_bios_cs(struct device *dev,
                           struct device_attribute *da,
                           const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;
	u8 temp = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_BIOSCS_OFFSET, 1);
	if(temp) byte |= 0x01; else byte &= ~(0x01);
	cpld_i2c_write(client, &byte, CPLD_BIOSCS_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static char* led_str[] = {
    "OFF",		//00
    "Green/Blue",	//01
    "Yellow/Orange",    //10
    "Red",    		//11
};

static ssize_t show_led(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte = 0;
	int shift = attr->index;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, &byte, CPLD_LED_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	byte = (byte >> shift) & 0x3;
	
	return sprintf (buf, "%d:%s\n", byte, led_str[byte]);
}

static ssize_t set_led(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);

	u8 temp = simple_strtol(buf, NULL, 16);
	u8 byte = 0;
	int shift = attr->index;  
	temp &= 0x3;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_LED_OFFSET, 1);
	byte &= ~(0x3<<shift);
	byte |= (temp<<shift);
	cpld_i2c_write(client, &byte, CPLD_LED_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static char* sysled_str[] = {
    "OFF",     //000
    "0.5 Hz",  //001
    "1 Hz",    //010
    "2 Hz",    //011
    "4 Hz",    //100
    "NA",      //101
    "NA",      //110
    "ON",      //111
};

static ssize_t show_sysled(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u32 status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte;
	int shift = (attr->index == 0)?3:0;
    
	mutex_lock(&data->update_lock);
	status = cpld_i2c_read(client, &byte, CPLD_SYSLED_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	
	byte = (byte >> shift) & 0x7;
	status = sprintf (buf, "%d:%s\n", byte, sysled_str[byte]);
	    
	return strlen(buf);
}

static ssize_t set_sysled(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);

	u8 temp = simple_strtol(buf, NULL, 16);
	u8 byte;
	int shift = (attr->index == 0)?3:0;
    
	temp &= 0x7;    

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_SYSLED_OFFSET, 1);
	byte &= ~(0x7<<shift);
	byte |= (temp<<shift);
	cpld_i2c_write(client, &byte, CPLD_SYSLED_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static char* psu_str[] = {
    "unpowered",        //00
    "normal",           //01
    "not installed",    //10
    "not installed",    //11
};

static ssize_t show_psu(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte=0;
	int shift = (attr->index == 0)?0:4;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, &byte, CPLD_PSU_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	byte = (byte >> shift) & 0x3;

	return sprintf (buf, "%d:%s\n", byte, psu_str[byte]);
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte=0;
	u8 offset = attr->index  + CPLD_PWM_OFFSET;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, &byte, offset, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	return sprintf(buf, "%d\n", byte);
}

static ssize_t set_pwm(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index  + CPLD_PWM_OFFSET;
	u8 byte = simple_strtol(buf, NULL, 10);

	mutex_lock(&data->update_lock);
	cpld_i2c_write(client2, &byte, offset, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t show_rpm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 offset = attr->index*2  + CPLD_RPM_OFFSET;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, byte, offset, 2);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	return sprintf(buf, "%d\n", (byte[0]<<8 | byte[1]));
}

static char* fantype_str[] = {
    "Normal Type",   //00
    "REVERSAL Type", //01
    "UNPLUGGED",     //10
    "UNPLUGGED",     //11
};

static ssize_t show_fantype(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 offset = CPLD_FANSTATUS_OFFSET;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, byte, offset, 2);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;
	status = (((byte[0] >> attr->index) & 0x01)) | (((byte[1] >> attr->index) & 0x01)<<1);

	return sprintf(buf, "%d:%s\n",status,fantype_str[status]);
}

static char* fanled_str[] = {
    "None",   //00
    "Green",  //01
    "Red",    //10
    "Both",   //11
};

static ssize_t show_fanled(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client2, byte, CPLD_FANLED_OFFSET, 2);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;
	status = (((byte[0] >> attr->index) & 0x01)) | (((byte[1] >> attr->index) & 0x01)<<1);

	return sprintf(buf, "%d:%s\n",status,fanled_str[status]);
}

static ssize_t set_fanled(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte[2] = {0,0};	
	u8 temp = simple_strtol(buf, NULL, 16);	
	int shift = attr->index;

	temp &= 0x3;
	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, byte, CPLD_FANLED_OFFSET, 2);
	byte[0] &= ~(1<<shift);
	byte[1] &= ~(1<<shift);
	byte[0] |= (temp & 0x01)<<shift;
	byte[1] |= ((temp >> 1) & 0x01)<<shift;
	cpld_i2c_write(client, byte, CPLD_FANLED_OFFSET, 2);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t set_watchdog_feed(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte=0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_WATCHDOGENABLE_OFFSET, 1);
	byte |= 0x02;
	cpld_i2c_write(client, &byte, CPLD_WATCHDOGENABLE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t set_watchdog_enable(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte=0x03;

	mutex_lock(&data->update_lock);
	cpld_i2c_write(client, &byte, CPLD_WATCHDOGENABLE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t show_watchdog_enable(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte=0;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, &byte, CPLD_WATCHDOGENABLE_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	return sprintf(buf, "%d\n",(byte&0x01));
}

static ssize_t set_watchdog_config(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = simple_strtol(buf, NULL, 10);

	if (byte<6) byte=6; 
	mutex_lock(&data->update_lock);
	cpld_i2c_write(client, &byte, CPLD_WATCHDOGCONFIG_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static ssize_t show_watchdog_config(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte=0;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, &byte, CPLD_WATCHDOGCONFIG_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	return sprintf(buf, "%d seconds\n",byte);
}

static ssize_t show_watchdog_counter(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	ssize_t len = 0;
	u8 byte=0;

	mutex_lock(&data->update_lock);
	len = cpld_i2c_read(client, &byte, CPLD_WATCHDOGCOUNTER_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	if (len==0) return 0;

	return sprintf(buf, "%d seconds\n",byte);
}

static SENSOR_DEVICE_ATTR(info,         S_IRUGO,		show_info, 0, 0);
static SENSOR_DEVICE_ATTR(diag, 	S_IWUSR|S_IRUGO,	show_diag, set_diag, 0);
static SENSOR_DEVICE_ATTR(temp1_input,	S_IRUGO,		show_thermal, 0, 0);
static SENSOR_DEVICE_ATTR(interrupt, 	S_IRUGO,   		show_interrupt, 0, 0);

static SENSOR_DEVICE_ATTR(fan_led,       S_IWUSR|S_IRUGO,   show_led, set_led, 2);
static SENSOR_DEVICE_ATTR(power_led,     S_IWUSR|S_IRUGO,   show_led, set_led, 4);
static SENSOR_DEVICE_ATTR(location_led,  S_IWUSR|S_IRUGO,   show_led, set_led, 6);

static SENSOR_DEVICE_ATTR(grn_led, S_IWUSR|S_IRUGO,	show_sysled, set_sysled, 0);
static SENSOR_DEVICE_ATTR(red_led, S_IWUSR|S_IRUGO,	show_sysled, set_sysled, 1);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 3);
#if FAN_NUM>4
static SENSOR_DEVICE_ATTR(pwm5, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 4);
#endif

static SENSOR_DEVICE_ATTR(fanmodule1_type, S_IRUGO,	show_fantype, 0, 0);
static SENSOR_DEVICE_ATTR(fanmodule2_type, S_IRUGO,	show_fantype, 0, 1);
static SENSOR_DEVICE_ATTR(fanmodule3_type, S_IRUGO,	show_fantype, 0, 2);
static SENSOR_DEVICE_ATTR(fanmodule4_type, S_IRUGO,	show_fantype, 0, 3);
#if FAN_NUM>4
static SENSOR_DEVICE_ATTR(fanmodule5_type, S_IRUGO,	show_fantype, 0, 4);
#endif

static SENSOR_DEVICE_ATTR(fanmodule1_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 0);
static SENSOR_DEVICE_ATTR(fanmodule2_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 1);
static SENSOR_DEVICE_ATTR(fanmodule3_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 2);
static SENSOR_DEVICE_ATTR(fanmodule4_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 3);
#if FAN_NUM>4
static SENSOR_DEVICE_ATTR(fanmodule5_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 4);
#endif

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO,	show_rpm, 0, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO,	show_rpm, 0, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO,	show_rpm, 0, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO,	show_rpm, 0, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO,	show_rpm, 0, 4);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO,	show_rpm, 0, 5);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO,	show_rpm, 0, 6);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO,	show_rpm, 0, 7);
#if FAN_NUM>4
static SENSOR_DEVICE_ATTR(fan9_input, S_IRUGO,	show_rpm, 0, 8);
static SENSOR_DEVICE_ATTR(fan10_input,S_IRUGO,	show_rpm, 0, 9);
#endif

static SENSOR_DEVICE_ATTR(psu1,         S_IRUGO,           	show_psu, 0, 0);
static SENSOR_DEVICE_ATTR(psu2,         S_IRUGO,           	show_psu, 0, 1);
static SENSOR_DEVICE_ATTR(power_status, S_IRUGO,           	show_powerstatus, 0, 0);

static SENSOR_DEVICE_ATTR(watchdog_feed,	S_IWUSR,		0, set_watchdog_feed, 0);
static SENSOR_DEVICE_ATTR(watchdog_enable,	S_IWUSR|S_IRUGO,	show_watchdog_enable, set_watchdog_enable, 0);
static SENSOR_DEVICE_ATTR(watchdog_config,	S_IWUSR|S_IRUGO,	show_watchdog_config, set_watchdog_config, 0);
static SENSOR_DEVICE_ATTR(watchdog_counter,	S_IRUGO,           	show_watchdog_counter,  0, 0);

static SENSOR_DEVICE_ATTR(bios_cs,      	S_IWUSR|S_IRUGO,   	show_bios_cs, set_bios_cs, 0);
			
static struct attribute *cpld_attributes[] = {
	&sensor_dev_attr_info.dev_attr.attr,
	&sensor_dev_attr_diag.dev_attr.attr,
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	
	&sensor_dev_attr_fan_led.dev_attr.attr,
	&sensor_dev_attr_power_led.dev_attr.attr,
	&sensor_dev_attr_location_led.dev_attr.attr,

	&sensor_dev_attr_grn_led.dev_attr.attr,
	&sensor_dev_attr_red_led.dev_attr.attr,

	&sensor_dev_attr_interrupt.dev_attr.attr,
	
	&sensor_dev_attr_psu1.dev_attr.attr,
	&sensor_dev_attr_psu2.dev_attr.attr,
	&sensor_dev_attr_power_status.dev_attr.attr,
	
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
#if FAN_NUM>4
	&sensor_dev_attr_pwm5.dev_attr.attr,
#endif	
	&sensor_dev_attr_fanmodule1_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule2_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule3_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule4_type.dev_attr.attr,
#if FAN_NUM>4
	&sensor_dev_attr_fanmodule5_type.dev_attr.attr,
#endif	
	&sensor_dev_attr_fanmodule1_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule2_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule3_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule4_led.dev_attr.attr,
#if FAN_NUM>4
	&sensor_dev_attr_fanmodule5_led.dev_attr.attr,
#endif	
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan6_input.dev_attr.attr,
	&sensor_dev_attr_fan7_input.dev_attr.attr,
	&sensor_dev_attr_fan8_input.dev_attr.attr,
#if FAN_NUM>4
	&sensor_dev_attr_fan9_input.dev_attr.attr,
	&sensor_dev_attr_fan10_input.dev_attr.attr,
#endif
	&sensor_dev_attr_watchdog_feed.dev_attr.attr,
	&sensor_dev_attr_watchdog_enable.dev_attr.attr,
	&sensor_dev_attr_watchdog_config.dev_attr.attr,
	&sensor_dev_attr_watchdog_counter.dev_attr.attr,
	
	&sensor_dev_attr_bios_cs.dev_attr.attr,
	NULL
};

static const struct attribute_group cpld_group = {
	.attrs = cpld_attributes,
};

/*-----------------------------------------------------------------------*/
/* device probe and removal */
static int
cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cpld_data *data;
	int status;
   
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct cpld_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);

	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &cpld_group);

	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	//Check CPLD2 exist or not 
	client2 = i2c_new_dummy(client->adapter, CPLD2_ADDRESS);
	if(!client2) {
		hasCPLD2 = 0;
		client2 = client;
	} else {
		status = i2c_smbus_read_byte_data(client2, CPLD_INFO_OFFSET);
		if(status<0) {
			i2c_unregister_device(client2);
			i2c_set_clientdata(client2, NULL);
			hasCPLD2 = 0;
			client2 = client;		
		}
	}

	data->tsk = kthread_run(cpld_thread,client,"%s",dev_name(data->hwmon_dev));
	dev_info(&client->dev, "%s: sensor '%s'\n",
		 dev_name(data->hwmon_dev), client->name);
	
	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &cpld_group);
exit_free:
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return status;
}

static int cpld_remove(struct i2c_client *client)
{	
	struct cpld_data *data = i2c_get_clientdata(client);

	sysfs_remove_group(&client->dev.kobj, &cpld_group);
	hwmon_device_unregister(data->hwmon_dev);
	i2c_set_clientdata(client, NULL);
	if(hasCPLD2) {
		i2c_unregister_device(client2);
		i2c_set_clientdata(client2, NULL);
	}
	
	kfree(data);
	return 0;
}

static const struct i2c_device_id cpld_ids[] = {
	{ "inv_cpld" , 0, },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, cpld_ids);

static struct i2c_driver cpld_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "inv_cpld",
	},
	.probe		= cpld_probe,
	.remove		= cpld_remove,
	.id_table	= cpld_ids,
};

/*-----------------------------------------------------------------------*/

/* module glue */

static int __init inv_cpld_init(void)
{
	return i2c_add_driver(&cpld_driver);
}

static void __exit inv_cpld_exit(void)
{
	i2c_del_driver(&cpld_driver);
}

MODULE_AUTHOR("jack.ting <ting.jack@inventec>");
MODULE_DESCRIPTION("cpld driver");
MODULE_LICENSE("GPL");

module_init(inv_cpld_init);
module_exit(inv_cpld_exit);
