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
#include <linux/delay.h>

//#include "I2CHostCommunication.h"

#define IMPLEMENT_IPMI_CODE 1
int USE_IPMI=0;
//=================================
#if IMPLEMENT_IPMI_CODE
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>

#define IPMI_MAX_INTF            (4)
#define NETFN_OEM 0x30
#define CMD_GETDATA 0x31
#define CMD_SETDATA 0x32

struct mutex ipmi_mutex;

static void msg_handler(struct ipmi_recv_msg *msg,void* handler_data);
static ipmi_user_t ipmi_mh_user = NULL;
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
#endif
//=================================

#define USE_SMBUS    1

#define FAN_NUM  4 
#define PSU_NUM  2 

struct __attribute__ ((__packed__))  psoc_psu_layout {
    u16 psu1_iin;
    u16 psu2_iin;
    u16 psu1_iout;
    u16 psu2_iout;
    
    u16 psu1_pin;
    u16 psu2_pin;
    u16 psu1_pout;
    u16 psu2_pout;
    
    u16 psu1_vin;
    u16 psu2_vin;
    u16 psu1_vout;
    u16 psu2_vout;
};

struct __attribute__ ((__packed__))  psoc_layout {
    u8 ctl;                 //offset: 0
    u16 switch_temp;        //offset: 1
    u8 reserve0;            //offset: 3

    u8 fw_upgrade;          //offset: 4

    //i2c bridge
    u8 i2c_st;              //offset: 5
    u8 i2c_ctl;             //offset: 6
    u8 i2c_addr;            //offset: 7
    u8 i2c_data[0x20];      //offset: 8 

    //gpo
    u8 led_ctl;             //offset: 28

    u8 gpio;                //offset: 29

    //pwm duty
    u8 pwm[FAN_NUM];        //offset: 2a
    u8 pwm_psu[PSU_NUM];    //offset: 2e

    //fan rpm
    u16 fan[FAN_NUM*2];     //offset: 30
    
    u8  reserve1[4];        //offset: 40

    //gpi 
    u8 gpi_fan;             //offset: 44 

    //psu state
    u8 psu_state;           //offset: 45

    //temperature
    u16 temp[5];            //offset: 46
    u16 temp_psu[PSU_NUM];  //offset: 50

    //version
    u8 version[2];          //offset: 54
    
    u8  reserve2[4];        //offset: 56
    struct psoc_psu_layout psu_info;      //offset: 5a
};        

/* definition */
/* definition */
#define PSOC_OFF(m)    offsetof(struct psoc_layout, m)
#define PSOC_PSU_OFF(m)    offsetof(struct psoc_psu_layout, m)

#define SWITCH_TMP_OFFSET       PSOC_OFF(switch_temp)
#define PWM_OFFSET              PSOC_OFF(pwm)
#define THERMAL_OFFSET          PSOC_OFF(temp)
#define RPM_OFFSET              PSOC_OFF(fan)
#define DIAG_FLAG_OFFSET        PSOC_OFF(ctl)
#define FAN_LED_OFFSET          PSOC_OFF(led_ctl)
#define FAN_GPI_OFFSET          PSOC_OFF(gpi_fan)
#define PSOC_PSU_OFFSET         PSOC_OFF(psu_state)
#define VERSION_OFFSET          PSOC_OFF(version)
#define PSU_INFO_OFFSET         PSOC_OFF(psu_info)

/* Each client has this additional data */
struct psoc_data {
	struct device		*hwmon_dev;
	struct mutex		update_lock;
	u32                 diag;
};

/*-----------------------------------------------------------------------*/
#if IMPLEMENT_IPMI_CODE
static void msg_handler(struct ipmi_recv_msg *recv_msg,void* handler_data)
{
    struct completion *comp = recv_msg->user_msg_data;
    if (comp)
         complete(comp);
    else
        ipmi_free_recv_msg(recv_msg);
    return;
}

int ipmi_command(char NetFn, char cmd,char *data,int data_length, char* result, int* result_length)
{
    int rv=0,i;
    struct	ipmi_system_interface_addr addr;
    uint8_t	_data[data_length];
    struct	kernel_ipmi_msg msg;
    struct	completion comp;

    if(!mutex_trylock(&ipmi_mutex)) return 0;

//    for (i=0,rv=1; i<IPMI_MAX_INTF && rv; i++) {
//	 rv = ipmi_create_user(i, &ipmi_hndlrs, NULL, &ipmi_mh_user);
//    }

    if (rv < 0) {
        mutex_unlock(&ipmi_mutex);
        return rv;
    }

    addr.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE; 
    addr.channel = IPMI_BMC_CHANNEL;  
    addr.lun = 0;

    memcpy(_data,data,data_length);
    msg.netfn = NetFn;
    msg.cmd = cmd;
    msg.data = _data;
    msg.data_len = data_length;

    init_completion(&comp);
    rv = ipmi_request_supply_msgs(ipmi_mh_user, (struct ipmi_addr*)&addr, 0,&msg, &comp, &halt_smi_msg, &halt_recv_msg, 0);

    if (rv) {
//        ipmi_destroy_user(ipmi_mh_user);
        mutex_unlock(&ipmi_mutex);
        return -6;
    }

   wait_for_completion(&comp);
   
   rv=halt_recv_msg.msg.data[0];
   if(rv==0) {
        *result_length=halt_recv_msg.msg.data_len-1;
	memcpy(result,&halt_recv_msg.msg.data[1],halt_recv_msg.msg.data_len-1);
   }
   ipmi_free_recv_msg(&halt_recv_msg);
//   ipmi_destroy_user(ipmi_mh_user);
   mutex_unlock(&ipmi_mutex);
   return rv;
}
#endif

static ssize_t psoc_i2c_read(struct i2c_client *client, u8 *buf, u8 offset, size_t count)
{
#if USE_SMBUS    
if (USE_IPMI==0)
{
    int i;
	
    for(i=0; i<count; i++) {
        buf[i] = i2c_smbus_read_byte_data(client, offset+i);
    }	
    return count;
}
#if IMPLEMENT_IPMI_CODE
else
{
    uint8_t data[2];
    int result_len=0;
    int rv;

    data[0] = offset;
    data[1] = count;

    rv=ipmi_command(NETFN_OEM, CMD_GETDATA,data,2, buf, &result_len);

    return result_len;
}
#endif
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
}

static ssize_t psoc_i2c_write(struct i2c_client *client, char *buf, unsigned offset, size_t count)
{
#if USE_SMBUS    
if(USE_IPMI==0)
{
    int i;
	
    for(i=0; i<count; i++) {
        i2c_smbus_write_byte_data(client, offset+i, buf[i]);
    }	
    return count;
}
#if IMPLEMENT_IPMI_CODE
else
{
    uint8_t data[count+1],result[1];
    int result_len;
    data[0] = offset;
    memcpy(&data[1],buf,count);

    ipmi_command(NETFN_OEM, CMD_SETDATA,data,count+1, result, &result_len);	
    return count;
}
#endif
#else
	struct i2c_msg msg;
	int status;
    u8 writebuf[256];
	
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
}

#if 0
static u32 psoc_read32(struct i2c_client *client, u8 offset)
{
	u32 value = 0;
	u8 buf[4];
    
    if( psoc_i2c_read(client, buf, offset, 4) == 4)
        value = (buf[0]<<24 | buf[1]<<16 | buf[2]<<8 | buf[3]);
    
	return value;
}
#endif

static u16 psoc_read16(struct i2c_client *client, u8 offset)
{
	u16 value = 0;
	u8 buf[2];
    
    if(psoc_i2c_read(client, buf, offset, 2) == 2)
        value = (buf[0]<<8 | buf[1]<<0);
    
	return value;
}

static u8 psoc_read8(struct i2c_client *client, u8 offset)
{
	u8 value = 0;
	u8 buf = 0;
    
    if(psoc_i2c_read(client, &buf, offset, 1) == 1)
        value = buf;
    
	return value;
}

//PSOC i2c bridge regsters
#define PSOC_I2C_STATUS         0x05
#define PSOC_I2C_CNTRL          0x06
#define PSOC_I2C_ADDR           0x07
#define PSOC_I2C_DATA           0x08

//status bit definition
#define PSOC_I2C_START          (1 << 0)
#define PSOC_PMB_SEL            (1 << 7)

//addr bits definition
#define PSOC_I2C_READ           (1 << 0)

//PMBUS registers definition
#define PMBUS_READ_VIN                  (0x88)
#define PMBUS_READ_IIN                  (0x89)
#define PMBUS_READ_VOUT                 (0x8B)
#define PMBUS_READ_IOUT                 (0x8C)
#define PMBUS_READ_POUT                 (0x96)
#define PMBUS_READ_PIN                  (0x97)

/*
CPLD report the PSU0 status
000 = PSU normal operation
100 = PSU fault
010 = PSU unpowered
111 = PSU not installed

7 6 | 5 4 3 |  2 1 0
----------------------
    | psu1  |  psu0
*/
static char* psu_str[] = {
    "normal",           //000
    "NA",               //001
    "unpowered",        //010
    "NA",               //011
    "fault",            //100
    "NA",               //101
    "NA",               //110
    "not installed",    //111
};

static ssize_t show_psu_st(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u32 status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 byte;
	int shift = (attr->index == 0)?3:0;
    
	mutex_lock(&data->update_lock);
    status = psoc_i2c_read(client, &byte, PSOC_PSU_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	
    byte = (byte >> shift) & 0x7;
	
	status = sprintf (buf, "%d : %s\n", byte, psu_str[byte]);
	    
	return strlen(buf);
}

/*-----------------------------------------------------------------------*/

/* sysfs attributes for hwmon */
#define PSU1		0x5800
#define PSU2		0x5900
#define BMC_I2cBus	3   //BMC's I2C-1
#define PMBus_Vender	0x99
#define PMBus_Serial	0x9E
#define PMBus_Temp2 	0x8E
#define PMBus_Version	0x9B
#define MaxLeng_Result	0x20
#define MaxLog
static long pmbus_reg2data_linear(int data, int linear16);


static ssize_t show_ipmi_i2c(struct device *dev, struct device_attribute *da,
                         char *buf)
{
        struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	uint8_t data[4],result[MaxLeng_Result];
	int result_len;

	data[0] = BMC_I2cBus;
	data[1] = (attr->index & 0xFF00 ) >>7;
	data[3] = attr->index & 0xff;
	if(data[3]==PMBus_Temp2) 
		data[2]=2;
	else
		data[2]=MaxLeng_Result;

	if(ipmi_command(0x06, 0x52,data,4, result, &result_len)==0)
	{
		if(data[3]==PMBus_Temp2)
		{
  			return sprintf(buf, "%ld \n", pmbus_reg2data_linear(result[0] | (result[1]<<8), 0 ));
		}
		result[result[0]+1]='\0';
	
	        return sprintf(buf, "%s\n",&result[1] );
	}
	else
		return 0;
}

static ssize_t show_ipmi_sollog(struct device *dev, struct device_attribute *da,
                         char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
        uint8_t data[5],result[256];
	int result_len;
	uint32_t i;

	for(i=0;i<0xffffff;i+=255)
	{
       		data[0] = attr->index;
       	 	data[1] = (i & 0x0000ff);
      	  	data[2] = (i & 0x00ff00)>>8;
        	data[3] = (i & 0xff0000)>>16;
		data[4] = 0;

		result_len=0;

       	 	if(ipmi_command(0x32, 0xFE, data, 5, result, &result_len)==0)
		{
			if(result_len==0) break;
			result[result_len+1]='\0';
	 		printk("%s",result);
		}
		else break;

		if(result_len==0) break;
	}

	return 0;
}

static ssize_t show_thermal(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index * 2 + THERMAL_OFFSET;
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read16(client, offset);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%d\n",
		       (s8)(status>>8) * 1000  );
}



static ssize_t show_pwm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index  + PWM_OFFSET;
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read8(client, offset);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%d\n",
		       status);
}

static ssize_t set_pwm(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index  + PWM_OFFSET;

	u8 pwm = simple_strtol(buf, NULL, 10);
	if(pwm > 255) pwm = 255;
	
	if(data->diag) {    
    	mutex_lock(&data->update_lock);
    	psoc_i2c_write(client, &pwm, offset, 1);
    	mutex_unlock(&data->update_lock);
    }
	
	return count;
}


static ssize_t show_rpm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index*2  + RPM_OFFSET;
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read16(client, offset);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%d\n",
		       status);
}

static ssize_t show_switch_tmp(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status;
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u16 temp = 0;
    
	mutex_lock(&data->update_lock);
    status = psoc_i2c_read(client, (u8*)&temp, SWITCH_TMP_OFFSET, 2);
	mutex_unlock(&data->update_lock);
	
	status = sprintf (buf, "%d\n",  (s8)(temp>>8) * 1000  );
	    
	return strlen(buf);
}

static ssize_t set_switch_tmp(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	//struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);

	long temp = simple_strtol(buf, NULL, 10);
    u16 temp2 =  ( (temp/1000) <<8 ) & 0xFF00 ;
    
    //printk("set_switch_tmp temp=%d, temp2=0x%x (%x,%x)\n", temp, temp2, ( ( (temp/1000) <<8 ) & 0xFF00 ),  (( (temp%1000) / 10 ) & 0xFF));
    
	mutex_lock(&data->update_lock);
	psoc_i2c_write(client, (u8*)&temp2, SWITCH_TMP_OFFSET, 2);
	mutex_unlock(&data->update_lock);
	
	return count;
}

static ssize_t show_diag(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status;
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 diag_flag = 0;
    
	mutex_lock(&data->update_lock);
    status = psoc_i2c_read(client, (u8*)&diag_flag, DIAG_FLAG_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	
	data->diag = (diag_flag & 0x80)?1:0;
	status = sprintf (buf, "%d\n", data->diag);
	    
	return strlen(buf);
}

static ssize_t set_diag(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	//struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 value = 0;
	u8 diag = simple_strtol(buf, NULL, 10);
	
    diag = diag?1:0;
	data->diag = diag;
	    
	mutex_lock(&data->update_lock);
	psoc_i2c_read(client, (u8*)&value, DIAG_FLAG_OFFSET, 1);
	if(diag) value |= (1<<7);
	else     value &= ~(1<<7);
	psoc_i2c_write(client, (u8*)&value, DIAG_FLAG_OFFSET, 1);
	mutex_unlock(&data->update_lock);
	
	return count;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status;
	//struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read16(client, VERSION_OFFSET);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "ver: %x.%x\n", (status & 0xFF00)>>8,  (status & 0xFF) );
}


static ssize_t show_fan_led(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 bit = attr->index;
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read8(client, FAN_LED_OFFSET);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%d\n",
		       (status & (1<<bit))?1:0 );
}

static ssize_t set_fan_led(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 bit = attr->index;
	u8 led_state = 0;

	u8 v = simple_strtol(buf, NULL, 10);
	
	if(data->diag) {    
    	mutex_lock(&data->update_lock);
    	led_state = psoc_read8(client, FAN_LED_OFFSET);
    	if(v) led_state |=  (1<<bit);
    	else  led_state &= ~(1<<bit);    
    	psoc_i2c_write(client, &led_state, FAN_LED_OFFSET, 1);
    	mutex_unlock(&data->update_lock);
    }
	
	return count;
}

static ssize_t show_value8(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index;
    
	mutex_lock(&data->update_lock);
	
	status = psoc_read8(client, offset);
	
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "0x%02X\n", status );
}

static long pmbus_reg2data_linear(int data, int linear16)
{
    s16 exponent;
    s32 mantissa;
    long val;

    if (linear16) { /* LINEAR16 */
        exponent = -9;
        mantissa = (u16) data;
    } else {  /* LINEAR11 */
        exponent = ((s16)data) >> 11;
        exponent = ((s16)( data & 0xF800) ) >> 11;
        mantissa = ((s32)((data & 0x7ff) << 5)) >> 5;
    }
    
    //printk("data=%d,  m=%d, e=%d\n", data, exponent, mantissa);
    val = mantissa;

    /* scale result to micro-units for power sensors */
    val = val * 1000L;

    if (exponent >= 0)
        val <<= exponent;
    else
        val >>= -exponent;

    return val;
}

static ssize_t show_psu_psoc(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	struct i2c_client *client = to_i2c_client(dev);
	struct psoc_data *data = i2c_get_clientdata(client);
	u8 offset = attr->index + PSU_INFO_OFFSET;
    
	mutex_lock(&data->update_lock);
	status = psoc_read16(client, offset);
	mutex_unlock(&data->update_lock);
	
	return sprintf(buf, "%ld \n", pmbus_reg2data_linear(status, strstr(attr->dev_attr.attr.name, "vout")? 1:0 ));
}



static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO,			show_thermal, 0, 0);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO,			show_thermal, 0, 1);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO,			show_thermal, 0, 2);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO,			show_thermal, 0, 3);
static SENSOR_DEVICE_ATTR(temp5_input, S_IRUGO,			show_thermal, 0, 4);
static SENSOR_DEVICE_ATTR(thermal_psu1, S_IRUGO,		show_thermal, 0, 5);
static SENSOR_DEVICE_ATTR(thermal_psu2, S_IRUGO,		show_thermal, 0, 6);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 0);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 1);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 2);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 3);
static SENSOR_DEVICE_ATTR(pwm_psu1, S_IWUSR|S_IRUGO,		show_pwm, set_pwm, 4);
static SENSOR_DEVICE_ATTR(pwm_psu2, S_IWUSR|S_IRUGO,		show_pwm, set_pwm, 5);

static SENSOR_DEVICE_ATTR(psu0,  S_IRUGO,			        show_psu_st, 0, 0);
static SENSOR_DEVICE_ATTR(psu1,  S_IRUGO,			        show_psu_st, 0, 1);

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO,			show_rpm, 0, 0);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO,			show_rpm, 0, 1);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO,			show_rpm, 0, 2);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO,			show_rpm, 0, 3);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO,			show_rpm, 0, 4);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO,			show_rpm, 0, 5);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO,			show_rpm, 0, 6);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO,			show_rpm, 0, 7);
static SENSOR_DEVICE_ATTR(rpm_psu1, S_IRUGO,		show_rpm, 0, 8);
static SENSOR_DEVICE_ATTR(rpm_psu2, S_IRUGO,		show_rpm, 0, 9);

static SENSOR_DEVICE_ATTR(switch_tmp, S_IWUSR|S_IRUGO,			show_switch_tmp, set_switch_tmp, 0);
static SENSOR_DEVICE_ATTR(temp6_input, S_IWUSR|S_IRUGO,                 show_switch_tmp, set_switch_tmp, 0);

static SENSOR_DEVICE_ATTR(diag, S_IWUSR|S_IRUGO,			show_diag, set_diag, 0);
static SENSOR_DEVICE_ATTR(version, S_IRUGO,			show_version, 0, 0);

static SENSOR_DEVICE_ATTR(fan_led_grn1, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 0);
static SENSOR_DEVICE_ATTR(fan_led_grn2, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 1);
static SENSOR_DEVICE_ATTR(fan_led_grn3, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 2);
static SENSOR_DEVICE_ATTR(fan_led_grn4, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 3);
static SENSOR_DEVICE_ATTR(fan_led_red1, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 4);
static SENSOR_DEVICE_ATTR(fan_led_red2, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 5);
static SENSOR_DEVICE_ATTR(fan_led_red3, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 6);
static SENSOR_DEVICE_ATTR(fan_led_red4, S_IWUSR|S_IRUGO,			show_fan_led, set_fan_led, 7);

static SENSOR_DEVICE_ATTR(fan_gpi,      S_IRUGO,			        show_value8,  0,           FAN_GPI_OFFSET);
static SENSOR_DEVICE_ATTR(psoc_psu1_vin,      S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vin));
static SENSOR_DEVICE_ATTR(psoc_psu1_vout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vout));
static SENSOR_DEVICE_ATTR(psoc_psu1_iin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iin));
static SENSOR_DEVICE_ATTR(psoc_psu1_iout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iout));
static SENSOR_DEVICE_ATTR(psoc_psu1_pin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pin));
static SENSOR_DEVICE_ATTR(psoc_psu1_pout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pout));


static SENSOR_DEVICE_ATTR(psoc_psu2_vin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_vout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vout));
static SENSOR_DEVICE_ATTR(psoc_psu2_iin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_iout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iout));
static SENSOR_DEVICE_ATTR(psoc_psu2_pin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_pout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pout));

static SENSOR_DEVICE_ATTR(in1_input,        S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vin));
static SENSOR_DEVICE_ATTR(in2_input,        S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vin));
static SENSOR_DEVICE_ATTR(curr1_input,      S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iin));
static SENSOR_DEVICE_ATTR(curr2_input,      S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iin));
static SENSOR_DEVICE_ATTR(power1_input,     S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pin));
static SENSOR_DEVICE_ATTR(power2_input,     S_IRUGO,        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pin));


//IPMI
static SENSOR_DEVICE_ATTR(thermal2_psu1,	S_IRUGO,                show_ipmi_i2c, 0, PSU1 | PMBus_Temp2);
static SENSOR_DEVICE_ATTR(psoc_psu1_vender,	S_IRUGO,                show_ipmi_i2c, 0, PSU1 | PMBus_Vender);
static SENSOR_DEVICE_ATTR(psoc_psu1_serial,	S_IRUGO,                show_ipmi_i2c, 0, PSU1 | PMBus_Serial);
static SENSOR_DEVICE_ATTR(psoc_psu1_version,	S_IRUGO,                show_ipmi_i2c, 0, PSU1 | PMBus_Version);

static SENSOR_DEVICE_ATTR(thermal2_psu2,        S_IRUGO,                show_ipmi_i2c, 0, PSU2 | PMBus_Temp2);
static SENSOR_DEVICE_ATTR(psoc_psu2_vender,     S_IRUGO,                show_ipmi_i2c, 0, PSU2 | PMBus_Vender);
static SENSOR_DEVICE_ATTR(psoc_psu2_serial,     S_IRUGO,                show_ipmi_i2c, 0, PSU2 | PMBus_Serial);
static SENSOR_DEVICE_ATTR(psoc_psu2_version,    S_IRUGO,                show_ipmi_i2c, 0, PSU2 | PMBus_Version);

static SENSOR_DEVICE_ATTR(sollog1,     S_IRUGO,                show_ipmi_sollog, 0, 1);
static SENSOR_DEVICE_ATTR(sollog2,     S_IRUGO,                show_ipmi_sollog, 0, 2);
 
static struct attribute *psoc_attributes[] = {
    //thermal
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp4_input.dev_attr.attr,
	&sensor_dev_attr_temp5_input.dev_attr.attr,
	
	&sensor_dev_attr_thermal_psu1.dev_attr.attr,
	&sensor_dev_attr_thermal_psu2.dev_attr.attr,


    //pwm
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
	&sensor_dev_attr_pwm_psu1.dev_attr.attr,
	&sensor_dev_attr_pwm_psu2.dev_attr.attr,
	
	//rpm
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan6_input.dev_attr.attr,
	&sensor_dev_attr_fan7_input.dev_attr.attr,
	&sensor_dev_attr_fan8_input.dev_attr.attr,
	
	&sensor_dev_attr_rpm_psu1.dev_attr.attr,
	&sensor_dev_attr_rpm_psu2.dev_attr.attr,
    
    //switch temperature
	&sensor_dev_attr_switch_tmp.dev_attr.attr,
        &sensor_dev_attr_temp6_input.dev_attr.attr,

    //diag flag
	&sensor_dev_attr_diag.dev_attr.attr,
	
	//version
	&sensor_dev_attr_version.dev_attr.attr,
	
	//fan led 
	&sensor_dev_attr_fan_led_grn1.dev_attr.attr,
	&sensor_dev_attr_fan_led_grn2.dev_attr.attr,
	&sensor_dev_attr_fan_led_grn3.dev_attr.attr,
	&sensor_dev_attr_fan_led_grn4.dev_attr.attr,
	&sensor_dev_attr_fan_led_red1.dev_attr.attr,
	&sensor_dev_attr_fan_led_red2.dev_attr.attr,
	&sensor_dev_attr_fan_led_red3.dev_attr.attr,
	&sensor_dev_attr_fan_led_red4.dev_attr.attr,

	//fan GPI 
	&sensor_dev_attr_fan_gpi.dev_attr.attr,
	&sensor_dev_attr_psu0.dev_attr.attr,
	&sensor_dev_attr_psu1.dev_attr.attr,


	//psu_psoc, new added on psoc 1.9
	&sensor_dev_attr_psoc_psu1_vin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_vout.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_iin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_iout.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_pin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_pout.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_vin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_vout.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_iin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_iout.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_pin.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_pout.dev_attr.attr,

        // Add new fields which matching standard
        &sensor_dev_attr_in1_input.dev_attr.attr,
        &sensor_dev_attr_in2_input.dev_attr.attr,
        &sensor_dev_attr_curr1_input.dev_attr.attr,
        &sensor_dev_attr_curr2_input.dev_attr.attr,
        &sensor_dev_attr_power1_input.dev_attr.attr,
        &sensor_dev_attr_power2_input.dev_attr.attr,

	//ipmi_command
	&sensor_dev_attr_thermal2_psu1.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_vender.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_serial.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_version.dev_attr.attr,

	&sensor_dev_attr_thermal2_psu2.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_vender.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_serial.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_version.dev_attr.attr,

        &sensor_dev_attr_sollog1.dev_attr.attr,
        &sensor_dev_attr_sollog2.dev_attr.attr,

	NULL
};

static const struct attribute_group psoc_group = {
	.attrs = psoc_attributes,
};

/*-----------------------------------------------------------------------*/

/* device probe and removal */

static int
psoc_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct psoc_data *data;
	int status,i,rv;

    printk("+%s\n", __func__);
    
	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA | I2C_FUNC_SMBUS_WORD_DATA))
		return -EIO;

	data = kzalloc(sizeof(struct psoc_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	i2c_set_clientdata(client, data);
	mutex_init(&data->update_lock);
	data->diag    = 0;
	
#if IMPLEMENT_IPMI_CODE
        for (i=0,rv=1; i<IPMI_MAX_INTF && rv; i++) {
             rv = ipmi_create_user(i, &ipmi_hndlrs, NULL, &ipmi_mh_user);
        }
        if(rv==0) {
                USE_IPMI=1;
               // ipmi_destroy_user(ipmi_mh_user);
                printk(" Enable IPMI PSoC protocol.\n");
		mutex_init(&ipmi_mutex);
        }
#endif
	/* Register sysfs hooks */
	status = sysfs_create_group(&client->dev.kobj, &psoc_group);
	if (status)
		goto exit_free;

	data->hwmon_dev = hwmon_device_register(&client->dev);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

	dev_info(&client->dev, "%s: sensor '%s'\n",
		 dev_name(data->hwmon_dev), client->name);

	return 0;

exit_remove:
	sysfs_remove_group(&client->dev.kobj, &psoc_group);
exit_free:
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return status;
}

static int psoc_remove(struct i2c_client *client)
{
	struct psoc_data *data = i2c_get_clientdata(client);

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &psoc_group);
	i2c_set_clientdata(client, NULL);
	kfree(data);
	return 0;
}

static const struct i2c_device_id psoc_ids[] = {
	{ "inv_psoc", 0, },
	{ /* LIST END */ }
};
MODULE_DEVICE_TABLE(i2c, psoc_ids);

static struct i2c_driver psoc_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name	= "inv_psoc",
	},
	.probe		= psoc_probe,
	.remove		= psoc_remove,
	.id_table	= psoc_ids,
};

/*-----------------------------------------------------------------------*/

/* module glue */

static int __init inv_psoc_init(void)
{
	return i2c_add_driver(&psoc_driver);
}

static void __exit inv_psoc_exit(void)
{
	i2c_del_driver(&psoc_driver);
}

MODULE_AUTHOR("eddie.lan <eddie.lan@inventec>");
MODULE_DESCRIPTION("inv psoc driver");
MODULE_LICENSE("GPL");

module_init(inv_psoc_init);
module_exit(inv_psoc_exit);
