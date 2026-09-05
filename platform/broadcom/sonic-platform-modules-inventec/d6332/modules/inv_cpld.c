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

#define USE_SMBUS    		1
#define CPLD_POLLING_PERIOD 	1000
#define ENABLE_SIMULATE		0
#define ENABLE_AUTOFAN		1

#define CPLD2_ADDRESS		0x33
static struct i2c_client *client2;
static u8 hasCPLD2 = 1;

#if ENABLE_SIMULATE
	u8 sim_register[0x90];
#endif

/* definition */
#define CPLD_INFO_OFFSET		0x00
#define CPLD_BIOSCS_OFFSET		0x04
#define CPLD_CTL_OFFSET			0x0C
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
#define FAN_NUM				5
#define PWM_MIN				30
#define PWM_DEFAULT			150

/* Each client has this additional data */
struct cpld_data {
	struct device *hwmon_dev;
	struct mutex update_lock;
	struct task_struct *cpld_thread;
	u8 diag;
	u8 model;
	u8 fan_direction;
	u8 operation_command;
	u8 stack_mode;
};

/*-----------------------------------------------------------------------*/
static ssize_t cpld_i2c_read(struct i2c_client *client, u8 *buf, u8 offset, size_t count)
{
#if ENABLE_SIMULATE
	memcpy(buf,&sim_register[offset],count);
	return count;
#else
#if USE_SMBUS
	int i;

	for(i=0; i<count; i++)
	{
		buf[i] = i2c_smbus_read_byte_data(client, offset+i);
	}
	return count;
#else
	struct i2c_msg msg[2];
	char msgbuf[2];
	int status;

	memset(msg, 0, sizeof(msg));

	msgbuf[0] = offset;

	msg[0].addr = client->addr;
	msg[0].buf = msgbuf;
	msg[0].len = 1;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = buf;
	msg[1].len = count;

	status = i2c_transfer(client->adapter, msg, 2);

	if(status == 2)
	    status = count;

	return status;
#endif
#endif
}

static ssize_t cpld_i2c_write(struct i2c_client *client, char *buf, unsigned offset, size_t count)
{
#if ENABLE_SIMULATE
	memcpy(&sim_register[offset],buf,count);
	return count;
#else
#if USE_SMBUS
	int i;

	for(i=0; i<count; i++)
	{
		i2c_smbus_write_byte_data(client, offset+i, buf[i]);
	}
	return count;
#else
	struct i2c_msg msg;
	int status;
	u8 writebuf[64];

	int i = 0;

	msg.addr = client->addr;
	msg.flags = 0;

	/* msg.buf is u8 and casts will mask the values */
	msg.buf = writebuf;

	msg.buf[i++] = offset;
	memcpy(&msg.buf[i], buf, count);
	msg.len = i + count;

	status = i2c_transfer(client->adapter, &msg, 1);
	if (status == 1)
		status = count;

	return status;
#endif
#endif
}

/*-----------------------------------------------------------------------*/
/* sysfs attributes for hwmon */

static char* model_str[] = {
    "10GBaseT",      //0
    "SFP",       	 //1
};

static char* fandirection_str[] = {
    "Rear-to-Front",         //0
    "Front-to-Rear",       	 //1
};

static ssize_t show_info(struct device *dev, struct device_attribute *da,
			char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte[4] = {0,0,0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, byte, CPLD_INFO_OFFSET, 4);
	mutex_unlock(&data->update_lock);

	sprintf (buf, "The CPLD release date is %02d/%02d/%d.\n",
	byte[2] & 0xf, (byte[3] & 0x1f), 2014+(byte[2] >> 4));	/* mm/dd/yyyy*/
	sprintf (buf, "%sThe Model is %s %s\n", buf, model_str[(byte[0]>>5) & 0x01],fandirection_str[(byte[0]>>7) & 0x01]);
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
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, byte, CPLD_POWERSTATUS_OFFSET, 2);
	mutex_unlock(&data->update_lock);

	sprintf (buf, "PGD_P5V: %s\n", powerstatus_str[(byte[0]>>7) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_STBY: %s\n", buf, powerstatus_str[(byte[0]>>6) & 0x01]);
	//if(data->model==0) sprintf (buf, "%sPGD_P1V88: %s\n", buf, powerstatus_str[(byte[0]>>5) & 0x01]);
	sprintf (buf, "%sPGD_P1V8_A: %s\n", buf, powerstatus_str[(byte[0]>>4) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_SYS: %s\n", buf, powerstatus_str[(byte[0]>>3) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_A: %s\n", buf, powerstatus_str[(byte[0]>>2) & 0x01]);
	sprintf (buf, "%sPGD_P3V3_B: %s\n", buf, powerstatus_str[(byte[0]>>1) & 0x01]);
	sprintf (buf, "%sPGD_P1V2: %s\n", buf, powerstatus_str[(byte[0]>>0) & 0x01]);
	sprintf (buf, "%sPGD_P0V8_A: %s\n", buf,powerstatus_str[(byte[1]>>7) & 0x01]);
	sprintf (buf, "%sPGD_P0V89_ROV: %s\n", buf, powerstatus_str[(byte[1]>>6) & 0x01]);
	//if(data->model==0) sprintf (buf, "%sPGD_P1V0_A: %s\n", buf, powerstatus_str[(byte[1]>>5) & 0x01]);
	//if(data->model==0) sprintf (buf, "%sPGD_P1V0_B: %s\n", buf, powerstatus_str[(byte[1]>>4) & 0x01]);
	sprintf (buf, "%sSW_PWR_READY: %s\n", buf, powerstatus_str[(byte[1]>>3) & 0x01]);
	//sprintf (buf, "%sCORE_PWRGD_TO_CPLD: %s\n", buf, powerstatus_str[(byte[1]>>2) & 0x01]);
	//sprintf (buf, "%sCPU_STBY_PWROK: %s\n", buf, powerstatus_str[(byte[1]>>1) & 0x01]);
	sprintf (buf, "%sPGD_P1V8_A_STBY: %s\n", buf, powerstatus_str[(byte[1]>>0) & 0x01]);

	return strlen(buf);
}

static ssize_t show_diag(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	return sprintf (buf, "%d\n", data->diag);
}

static ssize_t set_diag(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 diag = simple_strtol(buf, NULL, 10);
	data->diag = diag?1:0;
	return count;
}

static char* stackmode_str[] = {
    "Non-Stack member",     	//0
    "Stack Master",      	//1
    "Stack Backup/Member",  	//2
    "Stack Error",    		//3
};

static ssize_t show_stackmode(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	return sprintf (buf, "%d: %s\n", data->stack_mode,stackmode_str[data->stack_mode]);
}

static ssize_t set_stackmode(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 stackmode = simple_strtol(buf, NULL, 10);

	if(stackmode<4) data->stack_mode = stackmode;

	return count;
}

static char* resetbutton_str[] = {
    "No press",     		//0
    "Reserved",      		//1
    "Press and hold <5s",  	//2
    "Press and hold >5s",   	//3
};

static ssize_t show_resetbuttonstatus(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_RESETBUTTONSTATUS_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	byte &=0x03;

	return sprintf (buf, "0x%02X:%s\n", byte,resetbutton_str[byte]);
}

static char* resetcause_str[] = {
    "Power-On",     	//0
    "Watchdog",      	//1
    "SoftwareReboot",  	//2
    "ResetButton",    	//3
    "Panic",		//4
    "ThermalTrip"	//5
};

static ssize_t show_resetcause(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_RSTCAUSE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	sprintf (buf, "0x%02X:", byte);
	if(byte==0) sprintf (buf, "%sNone",buf);
	if(byte&0x01)  sprintf (buf, "%s%s ",buf,resetcause_str[0]);
	if(byte&0x02)  sprintf (buf, "%s%s ",buf,resetcause_str[1]);
	if(byte&0x04)  sprintf (buf, "%s%s ",buf,resetcause_str[2]);
	if(byte&0x08)  sprintf (buf, "%s%s ",buf,resetcause_str[3]);
	if(byte&0x10)  sprintf (buf, "%s%s ",buf,resetcause_str[4]);
	if(byte&0x20)  sprintf (buf, "%s%s ",buf,resetcause_str[5]);
	return sprintf (buf, "%s\n", buf);
}

static ssize_t set_resetcause(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	cpld_i2c_write(client, &byte, CPLD_RSTCAUSE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return count;
}

static char* interrupt_str[] = {
    "PCIE_INTR_L", 		//0
    "EXT_USB_OC_N",      	//1
    "PS2_ALERT_N",  		//2
    "PS1_ALERT_N",    		//3
    "PLD_SEN5_ALERT_N",		//4
    "PLD_SEN4_ALERT_N",     	//5
    "PLD_SEN3_ALERT_N",  	//6
    "UCD90160_TEMP_INT_N",    	//7
    "RSTBTN_INT_N",		//8
    "WDT_IRQ_N",  		//9
    "RSTBTN_5s_INT_N",    	//10
    "Reserved"			//11
};

static ssize_t show_interrupt(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte[4] = {0,0,0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, byte, CPLD_INT_OFFSET, 4);
	mutex_unlock(&data->update_lock);

	sprintf (buf, "0x%02X 0x%02X:", byte[0],byte[2]);
	if(byte[0]==0xff && byte[2]==0x07) sprintf (buf, "%sNone",buf);
	if(!(byte[0]&0x01)) sprintf (buf, "%s%s ",buf,interrupt_str[0]);
	if(!(byte[0]&0x02)) sprintf (buf, "%s%s ",buf,interrupt_str[1]);
	//if(!(byte[0]&0x04)) sprintf (buf, "%s%s ",buf,interrupt_str[2]);
	//if(!(byte[0]&0x08)) sprintf (buf, "%s%s ",buf,interrupt_str[3]);
	if(!(byte[0]&0x10)) sprintf (buf, "%s%s ",buf,interrupt_str[4]);
	if(!(byte[0]&0x20)) sprintf (buf, "%s%s ",buf,interrupt_str[5]);
	if(!(byte[0]&0x40)) sprintf (buf, "%s%s ",buf,interrupt_str[6]);
	if(!(byte[0]&0x80)) sprintf (buf, "%s%s ",buf,interrupt_str[7]);
	if(!(byte[2]&0x01)) sprintf (buf, "%s%s%s ",buf,interrupt_str[8] ,(byte[3]&0x01)?"(Blocked)":"");
	if(!(byte[2]&0x02)) sprintf (buf, "%s%s%s ",buf,interrupt_str[9] ,(byte[3]&0x02)?"(Blocked)":"");
	if(!(byte[2]&0x04)) sprintf (buf, "%s%s%s ",buf,interrupt_str[10],(byte[3]&0x04)?"(Blocked)":"");

	return sprintf (buf, "%s\n", buf);
}

static ssize_t show_operationcommand(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	return sprintf (buf, "%d\n", data->operation_command);
}

static ssize_t set_operationcommand(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 operationcommand = simple_strtol(buf, NULL, 10);

	data->operation_command = operationcommand?1:0;

	return count;
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
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_BIOSCS_OFFSET, 1);
	mutex_unlock(&data->update_lock);

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
    "OFF",     //000
    "ON",      //001
    "1 Hz",    //010
    "2 Hz",    //011
};

static ssize_t show_led(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;
	int shift = attr->index;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_LED_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	byte = (byte >> shift) & 0x3;

	return sprintf (buf, "%d: %s\n", byte, led_str[byte]);
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
	u8 byte=0;
	int shift = (attr->index == 0)?0:4;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, &byte, CPLD_PSU_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	byte = (byte >> shift) & 0x3;

	return sprintf (buf, "%d:%s\n", byte, psu_str[byte]);
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte=0;
	u8 offset = attr->index  + CPLD_PWM_OFFSET;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, &byte, offset, 1);
	mutex_unlock(&data->update_lock);

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
	u8 offset = attr->index*2  + CPLD_RPM_OFFSET;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, byte, offset, 2);
	mutex_unlock(&data->update_lock);

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
	u8 offset = CPLD_FANSTATUS_OFFSET;
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, byte, offset, 2);
	mutex_unlock(&data->update_lock);
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
	u8 byte[2] = {0,0};

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client2, byte, CPLD_FANLED_OFFSET, 2);
	mutex_unlock(&data->update_lock);
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
	cpld_i2c_write(client2, byte, CPLD_FANLED_OFFSET, 2);
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
	u8 byte=0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_WATCHDOGENABLE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

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
	u8 byte=0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_WATCHDOGCONFIG_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d seconds\n",byte);
}

static ssize_t show_watchdog_counter(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte=0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_WATCHDOGCOUNTER_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d seconds\n",byte);
}

static ssize_t show_paniccode(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 byte = 0;

	mutex_lock(&data->update_lock);
	cpld_i2c_read(client, &byte, CPLD_PANICCODE_OFFSET, 1);
	mutex_unlock(&data->update_lock);

	return sprintf (buf, "0x%02X\n", byte);
}

#if ENABLE_SIMULATE
static ssize_t show_simdump(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u8 i, j;
	sprintf(buf,"usage: echo 0xAABB > sim_buffer (AA is address, BB is value)\n\n");
	sprintf(buf,"%s       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n",buf);
	sprintf(buf,"%s======================================================\n",buf);
	sprintf(buf,"%s0000:  ",buf);
	for (j = 0, i = 0; j < sizeof(sim_register); j++, i++)
	{
		sprintf(buf,"%s%02x ", buf, (int)sim_register[i]);
		if ((i & 0x0F) == 0x0F) sprintf(buf,"%s\n%04x:  ", buf, (i+1));
	}
	return sprintf(buf,"%s\n",buf);
}

static ssize_t set_simbuffer(struct device *dev, struct device_attribute *da,
			   const char *buf, size_t count)
{
	u16 byte = simple_strtol(buf, NULL, 16);
	u8 address = (byte >> 8);
	u8 value = (byte & 0xff);

	sim_register[address]=value;

	return count;
}
#endif
static SENSOR_DEVICE_ATTR(info,         S_IRUGO,		show_info, 0, 0);
static SENSOR_DEVICE_ATTR(diag, 	S_IWUSR|S_IRUGO,	show_diag, set_diag, 0);
static SENSOR_DEVICE_ATTR(interrupt, 	S_IRUGO,   		show_interrupt, 0, 0);

static SENSOR_DEVICE_ATTR(stacking_led, S_IWUSR|S_IRUGO,   show_led, set_led, 0);
static SENSOR_DEVICE_ATTR(fan_led,      S_IWUSR|S_IRUGO,   show_led, set_led, 2);
static SENSOR_DEVICE_ATTR(power_led,    S_IWUSR|S_IRUGO,   show_led, set_led, 4);
static SENSOR_DEVICE_ATTR(service_led,  S_IWUSR|S_IRUGO,   show_led, set_led, 6);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 3);
static SENSOR_DEVICE_ATTR(pwm5, S_IWUSR|S_IRUGO,	show_pwm, set_pwm, 4);

static SENSOR_DEVICE_ATTR(fanmodule1_type, S_IRUGO,	show_fantype, 0, 0);
static SENSOR_DEVICE_ATTR(fanmodule2_type, S_IRUGO,	show_fantype, 0, 1);
static SENSOR_DEVICE_ATTR(fanmodule3_type, S_IRUGO,	show_fantype, 0, 2);
static SENSOR_DEVICE_ATTR(fanmodule4_type, S_IRUGO,	show_fantype, 0, 3);
static SENSOR_DEVICE_ATTR(fanmodule5_type, S_IRUGO,	show_fantype, 0, 4);

static SENSOR_DEVICE_ATTR(fanmodule1_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 0);
static SENSOR_DEVICE_ATTR(fanmodule2_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 1);
static SENSOR_DEVICE_ATTR(fanmodule3_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 2);
static SENSOR_DEVICE_ATTR(fanmodule4_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 3);
static SENSOR_DEVICE_ATTR(fanmodule5_led, S_IWUSR|S_IRUGO,	show_fanled, set_fanled, 4);

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO,	show_rpm, 0, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO,	show_rpm, 0, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO,	show_rpm, 0, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO,	show_rpm, 0, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO,	show_rpm, 0, 4);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO,	show_rpm, 0, 5);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO,	show_rpm, 0, 6);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO,	show_rpm, 0, 7);
static SENSOR_DEVICE_ATTR(fan9_input, S_IRUGO,	show_rpm, 0, 8);
static SENSOR_DEVICE_ATTR(fan10_input,S_IRUGO,	show_rpm, 0, 9);

static SENSOR_DEVICE_ATTR(psu1,         S_IRUGO,           	show_psu, 0, 0);
static SENSOR_DEVICE_ATTR(psu2,         S_IRUGO,           	show_psu, 0, 1);
static SENSOR_DEVICE_ATTR(power_status, S_IRUGO,           	show_powerstatus, 0, 0);
static SENSOR_DEVICE_ATTR(reset_cause,  S_IWUSR|S_IRUGO,   	show_resetcause, set_resetcause, 0);
static SENSOR_DEVICE_ATTR(resetbutton_status,  S_IRUGO,		show_resetbuttonstatus, 0, 0);
static SENSOR_DEVICE_ATTR(panic_code,	S_IRUGO,           	show_paniccode,  0, 0);

static SENSOR_DEVICE_ATTR(watchdog_feed,	S_IWUSR,		0, set_watchdog_feed, 0);
static SENSOR_DEVICE_ATTR(watchdog_enable,	S_IWUSR|S_IRUGO,	show_watchdog_enable, set_watchdog_enable, 0);
static SENSOR_DEVICE_ATTR(watchdog_config,	S_IWUSR|S_IRUGO,	show_watchdog_config, set_watchdog_config, 0);
static SENSOR_DEVICE_ATTR(watchdog_counter,	S_IRUGO,           	show_watchdog_counter,  0, 0);

static SENSOR_DEVICE_ATTR(stack_mode,		S_IWUSR|S_IRUGO,   	show_stackmode, set_stackmode, 0);
static SENSOR_DEVICE_ATTR(operation_command,	S_IWUSR|S_IRUGO,   	show_operationcommand, set_operationcommand, 0);
#if ENABLE_SIMULATE
	static SENSOR_DEVICE_ATTR(sim_buffer,	S_IWUSR|S_IRUGO,	show_simdump, set_simbuffer, 0);
#endif
static SENSOR_DEVICE_ATTR(bios_cs,      	S_IWUSR|S_IRUGO,   	show_bios_cs, set_bios_cs, 0);

static struct attribute *cpld_attributes[] = {
	&sensor_dev_attr_info.dev_attr.attr,
	&sensor_dev_attr_diag.dev_attr.attr,

	&sensor_dev_attr_stacking_led.dev_attr.attr,
	&sensor_dev_attr_fan_led.dev_attr.attr,
	&sensor_dev_attr_power_led.dev_attr.attr,
	&sensor_dev_attr_service_led.dev_attr.attr,

	&sensor_dev_attr_interrupt.dev_attr.attr,

	&sensor_dev_attr_psu1.dev_attr.attr,
	&sensor_dev_attr_psu2.dev_attr.attr,
	&sensor_dev_attr_power_status.dev_attr.attr,
	&sensor_dev_attr_reset_cause.dev_attr.attr,
	&sensor_dev_attr_resetbutton_status.dev_attr.attr,
	&sensor_dev_attr_panic_code.dev_attr.attr,

	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
	&sensor_dev_attr_pwm5.dev_attr.attr,

	&sensor_dev_attr_fanmodule1_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule2_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule3_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule4_type.dev_attr.attr,
	&sensor_dev_attr_fanmodule5_type.dev_attr.attr,

	&sensor_dev_attr_fanmodule1_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule2_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule3_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule4_led.dev_attr.attr,
	&sensor_dev_attr_fanmodule5_led.dev_attr.attr,

	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan6_input.dev_attr.attr,
	&sensor_dev_attr_fan7_input.dev_attr.attr,
	&sensor_dev_attr_fan8_input.dev_attr.attr,
	&sensor_dev_attr_fan9_input.dev_attr.attr,
	&sensor_dev_attr_fan10_input.dev_attr.attr,

	&sensor_dev_attr_watchdog_feed.dev_attr.attr,
	&sensor_dev_attr_watchdog_enable.dev_attr.attr,
	&sensor_dev_attr_watchdog_config.dev_attr.attr,
	&sensor_dev_attr_watchdog_counter.dev_attr.attr,

	&sensor_dev_attr_stack_mode.dev_attr.attr,
	&sensor_dev_attr_operation_command.dev_attr.attr,

	&sensor_dev_attr_bios_cs.dev_attr.attr,
#if ENABLE_SIMULATE
	&sensor_dev_attr_sim_buffer.dev_attr.attr,
#endif
	NULL
};

struct i2c_client *client_notifier;

static const struct attribute_group cpld_group = {
	.attrs = cpld_attributes,
};

static int log_reboot_to_cpld (struct notifier_block *self, unsigned long event, void *ptr)
{
	u8 byte=0;
	if (event == SYS_DOWN || event == SYS_HALT)
	{
		cpld_i2c_read(client_notifier, &byte, CPLD_RSTCAUSE_OFFSET, 1);
		byte |= 0x04;
		cpld_i2c_write(client_notifier, &byte, CPLD_RSTCAUSE_OFFSET, 1);
	}
	return NOTIFY_DONE;
}

static int log_panic_to_cpld(struct notifier_block *self, unsigned long event, void *ptr)
{
#if 1
// Can't use SMBus if smp is stopped (need patch the panic.c)
	if (client_notifier != NULL)
	{
		u8 byte=0;
		cpld_i2c_read(client_notifier, &byte, CPLD_RSTCAUSE_OFFSET, 1);
		byte |= 0x10;
		cpld_i2c_write(client_notifier, &byte, CPLD_RSTCAUSE_OFFSET, 1);
	}
#endif
	return NOTIFY_DONE;
}

static struct notifier_block reboot_notifier = {
	.notifier_call = log_reboot_to_cpld,
};

static struct notifier_block panic_notifier = {
	.notifier_call = log_panic_to_cpld,
};

#if ENABLE_AUTOFAN
#define SWITCH_ADDRESS 3-004e
#define ENV_ADDRESS    3-004a
#define CPU_ADDRESS    3-0048
#define PSU1_ADDRESS   2-0058
#define PSU2_ADDRESS   2-0059
#define PSU1_ADDRESS_DVT 2-005a
#define PSU2_ADDRESS_DVT 2-005b

#define _STR(s) #s
#define __STR(s) _STR(s)
#define __File_input(__file) __STR(/sys/bus/i2c/devices/__file/hwmon/hwmon%d/temp1_input)
#define __File_max(__file) __STR(/sys/bus/i2c/devices/__file/hwmon/hwmon%d/temp1_max)
#define __File_max_hyst(__file) __STR(/sys/bus/i2c/devices/__file/hwmon/hwmon%d/temp1_max_hyst)
#define __File_pwm(__file) __STR(/sys/bus/i2c/devices/__file/hwmon/hwmon%d/pwm1)

#define SWITCH_TEMPERATURE __File_input(SWITCH_ADDRESS)
#define ENV_TEMPERATURE    __File_input(ENV_ADDRESS)
#define CPU_TEMPERATURE    __File_input(CPU_ADDRESS)
#define SWITCH_MAX         __File_max(SWITCH_ADDRESS)
#define ENV_MAX            __File_max(ENV_ADDRESS)
#define CPU_MAX            __File_max(CPU_ADDRESS)
#define SWITCH_MAX_HYST    __File_max_hyst(SWITCH_ADDRESS)
#define ENV_MAX_HYST       __File_max_hyst(ENV_ADDRESS)
#define CPU_MAX_HYST       __File_max_hyst(CPU_ADDRESS)
#define PSU1_PWM           __File_pwm(PSU1_ADDRESS)
#define PSU2_PWM           __File_pwm(PSU2_ADDRESS)
#define PSU1_PWM_DVT       __File_pwm(PSU1_ADDRESS_DVT)
#define PSU2_PWM_DVT       __File_pwm(PSU2_ADDRESS_DVT)

#define n_entries	9
#define temp_hysteresis 2
#define MAX_HWMON 9

//[Model:10G/SFP][FanDirection:R2F/F2R][Type:CPU/ASIC/ENV]
u8 Thermaltrip[2][2][3] ={{{64,68,64},{64,68,64}},{{64,68,64},{64,68,64}}};
u8    FanTable[2][2][3][n_entries][2]={
{//10GBastT
	{//Rear-to-Front
		{//CPU
			{ 45, 255 },  \
			{ 40, 201 },  \
			{ 38, 170 },  \
			{ 35, 130 },  \
			{ 31, 105 },  \
			{ 27,  85 },  \
			{ 24,  80 },  \
			{ 20,  70 },  \
			{ 18,  65 }
		}
		,{//ASIC
			{ 52, 255 },  \
			{ 47, 201 },  \
			{ 44, 170 },  \
			{ 42, 130 },  \
			{ 39, 105 },  \
			{ 36,  85 },  \
			{ 33,  80 },  \
			{ 30,  70 },  \
			{ 28,  65 }
		}
		,{//ENV
			{ 47, 255 },  \
			{ 43, 201 },  \
			{ 41, 170 },  \
			{ 39, 130 },  \
			{ 36, 105 },  \
			{ 32,  85 },  \
			{ 30,  80 },  \
			{ 28,  70 },  \
			{ 19,  65 }
		}
	},
	{//Front-to-Rear
		{//CPU
			{ 45, 255 },  \
			{ 40, 201 },  \
			{ 38, 170 },  \
			{ 35, 130 },  \
			{ 31, 105 },  \
			{ 27,  85 },  \
			{ 24,  80 },  \
			{ 20,  70 },  \
			{ 18,  65 }
		}
		,{//ASIC
			{ 52, 255 },  \
			{ 47, 201 },  \
			{ 44, 170 },  \
			{ 42, 130 },  \
			{ 39, 105 },  \
			{ 36,  85 },  \
			{ 33,  80 },  \
			{ 30,  70 },  \
			{ 28,  65 }
		}
		,{//ENV
			{ 47, 255 },  \
			{ 43, 201 },  \
			{ 41, 170 },  \
			{ 39, 130 },  \
			{ 36, 105 },  \
			{ 32,  85 },  \
			{ 30,  80 },  \
			{ 28,  70 },  \
			{ 19,  65 }
		}
	},
},
{//SFP28
	{//Rear-to-Front
		{//CPU
			{ 45, 255 },  \
			{ 40, 201 },  \
			{ 38, 170 },  \
			{ 35, 130 },  \
			{ 31, 105 },  \
			{ 27,  85 },  \
			{ 24,  80 },  \
			{ 20,  70 },  \
			{ 18,  65 }
		}
		,{//ASIC
			{ 52, 255 },  \
			{ 47, 201 },  \
			{ 44, 170 },  \
			{ 42, 130 },  \
			{ 39, 105 },  \
			{ 36,  85 },  \
			{ 33,  80 },  \
			{ 30,  70 },  \
			{ 28,  65 }
		}
		,{//ENV
			{ 47, 255 },  \
			{ 43, 201 },  \
			{ 41, 170 },  \
			{ 39, 130 },  \
			{ 36, 105 },  \
			{ 32,  85 },  \
			{ 30,  80 },  \
			{ 28,  70 },  \
			{ 19,  65 }
		}
	},
	{//Front-to-Rear
		{//CPU
			{ 45, 255 },  \
			{ 40, 201 },  \
			{ 38, 170 },  \
			{ 35, 130 },  \
			{ 31, 105 },  \
			{ 27,  85 },  \
			{ 24,  80 },  \
			{ 20,  70 },  \
			{ 18,  65 }
		}
		,{//ASIC
			{ 52, 255 },  \
			{ 47, 201 },  \
			{ 44, 170 },  \
			{ 42, 130 },  \
			{ 39, 105 },  \
			{ 36,  85 },  \
			{ 33,  80 },  \
			{ 30,  70 },  \
			{ 28,  65 }
		}
		,{//ENV
			{ 47, 255 },  \
			{ 43, 201 },  \
			{ 41, 170 },  \
			{ 39, 130 },  \
			{ 36, 105 },  \
			{ 32,  85 },  \
			{ 30,  80 },  \
			{ 28,  70 },  \
			{ 19,  65 }
		}
	}
}
};

//type 0:CPU,1:ASIC,2:ENV
static u8 find_duty(u8 model, u8 fan_direction, u8 type, u8 temp)
{
	static u8 fantable_index[3]={n_entries-1, n_entries-1, n_entries-1};
	while(1)
	{
		if(fantable_index[type] >= 1 && temp > FanTable[model][fan_direction][type][fantable_index[type]-1][0])
			fantable_index[type]--;
		else if(fantable_index[type] < (n_entries-1) && temp < (FanTable[model][fan_direction][type][fantable_index[type]][0]-temp_hysteresis))
			fantable_index[type]++;
		else
			break;
	}
	return FanTable[model][fan_direction][type][fantable_index[type]][1];
}

static u32 getvalue(char *path)
{
	static struct file *f;
	mm_segment_t old_fs;
	u16 temp = 0;
	char temp_str[]={0,0,0,0,0,0,0,0,0};
	loff_t pos = 0;

	f = filp_open(path,O_RDONLY,0644);
	if(IS_ERR(f)) return -1;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_read(f, temp_str,8,&pos);
	temp = simple_strtoul(temp_str,NULL,10);
	filp_close(f,NULL);
	set_fs(old_fs);

	if(temp<0) temp=0;
	return temp;
}

static u8 setvalue(char *path, u32 value)
{
	static struct file *f;
	mm_segment_t old_fs;
	char temp_str[]={0,0,0,0,0,0,0,0,0};
	u8 len=0;
	loff_t pos = 0;

	f = filp_open(path,O_WRONLY,0644);
	if(IS_ERR(f)) return -1;

	len = sprintf(temp_str,"%d",value);

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	vfs_write(f, temp_str, len, &pos);
	filp_close(f,NULL);
	set_fs(old_fs);

	return 0;
}

static u8 probe_gettemp(char *path)
{
	u8 temp_path[64],i;
	u32 value;

	for(i=0;i<MAX_HWMON;i++)
	{
		sprintf(temp_path,path,i);
		value=getvalue(temp_path);
		if(value!=-1) return (u8)(value/1000);
	}
	return 0;
}

static u8 probe_setvalue(char *path, u32 value)
{
	u8 temp_path[64],i,status;

	for(i=0;i<MAX_HWMON;i++)
	{
		sprintf(temp_path,path,i);
		status=setvalue(temp_path, value);
		if(status==0) break;
	}
	return status;
}

static void autofanspeed(struct i2c_client *client, u8 fanfail, u8 fanturnoff)
{
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 fanpwm[5]={PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT};
	u8 i;
	u8 duty = PWM_MIN;
	u8 psu_duty = 0;

	if(fanfail==0)
	{
		u8 temp_switch = probe_gettemp(SWITCH_TEMPERATURE);
		u8 temp_env    = probe_gettemp(ENV_TEMPERATURE);
		u8 temp_cpu    = probe_gettemp(CPU_TEMPERATURE);

		u8 duty_cpu    = 0;
		u8 duty_switch = 0;
		u8 duty_env    = 0;

		if(temp_cpu!=0xff) duty_cpu=find_duty(data->model,data->fan_direction,0,temp_cpu);
		if(temp_switch!=0xff) duty_switch=find_duty(data->model,data->fan_direction,1,temp_switch);
		if(temp_env!=0xff) duty_env=find_duty(data->model,data->fan_direction,2,temp_env);

		if(temp_cpu==0xff && temp_switch==0xff && temp_env==0xff)
		{
			duty=PWM_DEFAULT;
		}
		else
		{
			duty=(duty>duty_cpu)?duty:duty_cpu;
			duty=(duty>duty_switch)?duty:duty_switch;
			duty=(duty>duty_env)?duty:duty_env;
		}

		memset(fanpwm,duty,FAN_NUM);
		for(i=0;i<FAN_NUM;i++)
			if(fanturnoff & (1<<i)) fanpwm[i]=PWM_MIN;
#if ENABLE_SIMULATE
		sim_register[0x80]=temp_cpu;
		sim_register[0x81]=temp_switch;
		sim_register[0x82]=temp_env;
		sim_register[0x83]=duty_cpu;
		sim_register[0x84]=duty_switch;
		sim_register[0x85]=duty_env;
		sim_register[0x86]=duty;
		sim_register[0x87]=psu_duty;
#endif
	}
	else
	{
		duty=0xff;
		memset(fanpwm,duty,FAN_NUM);
	}
	if (duty == 0xff)
		psu_duty = 100;
	else if (duty > 200)
		psu_duty = 80;
	else if (duty <= 200 && duty > 150)
		if(data->fan_direction==0) psu_duty = 70; else psu_duty = 65;
	else if (duty <= 150 && duty > 100)
		if(data->fan_direction==0) psu_duty = 60; else psu_duty = 55;
	else
		if(data->fan_direction==0) psu_duty = 50; else psu_duty = 35;
	probe_setvalue(PSU1_PWM, psu_duty);
	probe_setvalue(PSU2_PWM, psu_duty);
	probe_setvalue(PSU1_PWM_DVT, psu_duty);
	probe_setvalue(PSU2_PWM_DVT, psu_duty);

	mutex_lock(&data->update_lock);
	cpld_i2c_write(client2,fanpwm,CPLD_PWM_OFFSET,FAN_NUM);
	mutex_unlock(&data->update_lock);
}
#endif

/*-----------------------------------------------------------------------*/
static int cpld_polling(void *p)
{
	struct i2c_client *client = p;
	struct cpld_data *data = i2c_get_clientdata(client);
	u8 fanrpm[22],fanled[2],frontled,i;
	u8 fandir=0,fanpresent=0,fanfront,fanrear,fanerror;
	u8 fanfail,fanturnoff,psufail,lastStack,retry;
	u8 psustatus=0;
	u8 initial_thermaltrip[3] = {0,0,0};

    	while (!kthread_should_stop())
    	{
		//initial tmp75's thermaltrip value
		if(initial_thermaltrip[0]==0)
		{
			if((probe_setvalue(CPU_MAX, Thermaltrip[data->model][data->fan_direction][0]*1000)!=0xff)
				&& (probe_setvalue(CPU_MAX_HYST, (Thermaltrip[data->model][data->fan_direction][0]-temp_hysteresis)*1000)!=0xff))
					initial_thermaltrip[0]=1;
		}
		if(initial_thermaltrip[1]==0)
		{
			if((probe_setvalue(SWITCH_MAX, Thermaltrip[data->model][data->fan_direction][1]*1000)!=0xff)
				&& (probe_setvalue(SWITCH_MAX_HYST, (Thermaltrip[data->model][data->fan_direction][1]-temp_hysteresis)*1000)!=0xff))
					initial_thermaltrip[1]=1;
		}
		if(initial_thermaltrip[2]==0)
		{
			if((probe_setvalue(ENV_MAX, Thermaltrip[data->model][data->fan_direction][2]*1000)!=0xff)
				&& (probe_setvalue(ENV_MAX_HYST, (Thermaltrip[data->model][data->fan_direction][2]-temp_hysteresis)*1000)!=0xff))
					initial_thermaltrip[2]=1;
		}

		//LED control
		if (data->diag==0 && i2c_smbus_read_byte_data(client, 0)>=0)
		{
			for(retry=0;retry<2;retry++)
			{
				fanfail=0;
				fanled[0]=0; //clean green led
				fanled[1]=0; //clean red led
				fanfront=0;
				fanrear=0;
				fanerror=0;
				fanturnoff=0;

				//------ Fan Check -------
				cpld_i2c_read(client2, fanrpm, CPLD_RPM_OFFSET, 22);
				fandir=fanrpm[20];
				fanpresent=fanrpm[21];

				//Count the fan's direction
				for (i=0;i<FAN_NUM;i++)
				{
					int rpm1 = fanrpm[i*4] << 8 | fanrpm[i*4 + 1];
					int rpm2 = fanrpm[i*4 + 2] << 8 | fanrpm[i*4 + 3];

					if(fanpresent&(1<<i) || (rpm1 < 3000 || rpm1 > 23500) || (rpm2 < 3000 || rpm2 > 23500))
						fanerror++;
					else if(fandir&(1<<i))
						fanrear++;
					else
						fanfront++;
				}
				if(fanerror==0) break;
			}

			//Fan LED control
			for (i=0;i<FAN_NUM;i++)
			{
				//The Front or Rear fan < 256RPM : Red LED
				if(fanpresent&(1<<i)||fanrpm[i*4]==0||fanrpm[i*4+2]==0)
				{
					fanfail++;
					fanled[1] |= (1<<i); //turn-on red led
				}
				else
				{
					if(((fandir>>i)&0x01)!=data->fan_direction)
						fanled[0] |= (1<<i); //turn-on green led
					else
					{
						fanfail++;
						fanturnoff|= (1<<i); //turn off wrong-direction fan
						fanled[1] |= (1<<i); //turn-on red led
					}
				}
			}
			cpld_i2c_write(client2, fanled, CPLD_FANLED_OFFSET, 2);
#if ENABLE_AUTOFAN
			//Fan PWM control
			autofanspeed(client, fanfail, fanturnoff);
#endif
			//------- PSU Check --------
			cpld_i2c_read(client2, &psustatus, CPLD_PSU_OFFSET, 1);
			psustatus&=0x33;
			if(psustatus==0x11) psufail=0; else psufail=1;

			//----- FrontPanel LED -----
			frontled=0;

			//Stack LED in bit0 bit1
			if(data->stack_mode==0) frontled&=~(0x3);	//0: Non-Stack member => off
			if(data->stack_mode==1) frontled|=0x2;		//1: Stack Master => Flash green
			if(data->stack_mode==2) frontled|=0x1;		//2: Stack Backup/Member => Steady green
			if(data->stack_mode==3) frontled|=lastStack;//3: Stack Error => last status
			lastStack=frontled;

			if(fanpresent!=0x00)						//Fan LED in bit2 bit3
			{
				frontled|=(0x02<<2);					//Some fan unpresent => 0x02
			}
			else
			{
				if(fanturnoff||fanfail)					//some fan no speed      => 0x03
					frontled|=(0x03<<2);				//some fan dir different => 0x03
				else
					frontled|=(0x01<<2);				//Normal => 0x01
			}

			if(psustatus&0x22)	         				//POW LED in bit4 bit5
				frontled|=(0x02<<4); 					//not all psu plug-in  => 0x02
			else if (psustatus!=0x11)
				frontled|=(0x03<<4);					//some power is not ok => 0x03
			else
				frontled|=(0x01<<4);					//Normal => 0x01

			if(data->operation_command==1)									//Service LED in bit6 bit7
				frontled|=(0x01<<6);					//Steady if operation
			else if(fanfail>0||psufail>0||data->stack_mode==3)
				frontled|=(0x02<<6);					//Flash if any error
			else if(data->stack_mode==2)
				frontled|=(0x01<<6);					//Steady if mode in Stack Backup/Member
			else
				frontled&=~(0x3<<6);					//Off if mode in Non-Stack member or Stack Master with no operation and error
			cpld_i2c_write(client, &frontled, CPLD_LED_OFFSET, 1);
		}
        	set_current_state(TASK_INTERRUPTIBLE);
        	if(kthread_should_stop()) break;
        	schedule_timeout(msecs_to_jiffies(CPLD_POLLING_PERIOD));
	}
	return 0;
}

/*-----------------------------------------------------------------------*/
/* device probe and removal */

static int
cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cpld_data *data;
	int status;
	u8 byte[5]={PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT,PWM_DEFAULT};

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

	dev_info(&client->dev, "%s: sensor '%s'\n",
		 dev_name(data->hwmon_dev), client->name);

	//initial PWM to 60%
	cpld_i2c_write(client2, byte, CPLD_PWM_OFFSET, 5);

	//Handle LED control by the driver
	byte[0]=0x01;
	cpld_i2c_write(client, byte, CPLD_CTL_OFFSET, 1);
	cpld_i2c_write(client2, byte, CPLD_CTL_OFFSET, 1);

	//Get Model type
	cpld_i2c_read(client, byte, CPLD_INFO_OFFSET, 1);
	data->model = (byte[0]>>5) & 0x01;
	data->fan_direction = (byte[0]>>7) & 0x01;

	//kernel panic notifier
	atomic_notifier_chain_register(&panic_notifier_list, &panic_notifier);

	//soft reboot notifier
	register_reboot_notifier(&reboot_notifier);

	client_notifier=client;

	data->cpld_thread = kthread_run(cpld_polling,client,"%s",dev_name(data->hwmon_dev));

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
	//Return LED control to the CPLD
	u8 byte=0x00;
	cpld_i2c_write(client, &byte, CPLD_CTL_OFFSET, 1);
	cpld_i2c_write(client2, &byte, CPLD_CTL_OFFSET, 1);

	//unregister soft reboot notifier
	unregister_reboot_notifier(&reboot_notifier);

	//unregister kernel panic notifier
	atomic_notifier_chain_unregister(&panic_notifier_list, &panic_notifier);

	//stop cpld thread
	kthread_stop(data->cpld_thread);

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