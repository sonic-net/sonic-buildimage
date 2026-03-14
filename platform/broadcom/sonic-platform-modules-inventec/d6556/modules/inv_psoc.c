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
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#define SWITCH_TEMPERATURE_SOCK     "/proc/switch/temp"
#define PSOC_POLLING_PERIOD 1000

#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>

#define IPMI_MAX_INTF            (4)
#define NETFN_OEM 0x30
#define CMD_GETDATA 0x31
#define CMD_SETDATA 0x32
#define FAN_NUM  4
#define PSU_NUM  2

#define FAN_CLEI_SUPPORT 1
#define PSU_CLEI_SUPPORT 0

#define PSU1		0x5800
#define PSU2		0x5900
#define BMC_PMBusNumber	3
#define PMBus_Vendor	0x99
#define PMBus_Serial	0x9E
#define PMBus_Temp2 	0x8E
#define PMBus_Version	0x9B
#define MaxLeng_Result	0x40

#define BMC_FanCLEIBusNumber 9
#define DEVICE_CLEI_ADDR	0x52,0x53,0x54,0x55,0x56,0x50,0x51

#define MAX_IPMI_RECV_LENGTH 0xff
static char CLEI_ADDR[]={DEVICE_CLEI_ADDR};
struct task_struct  *kthread_auto_update;
static long pmbus_reg2data_linear(int data, int linear16);
struct ipmi_result{
	char result[MAX_IPMI_RECV_LENGTH];
	int result_length;
};

DEFINE_MUTEX(ipmi_mutex);
DEFINE_MUTEX(ipmi2_mutex);
static struct ipmi_result ipmiresult;
static struct device *hwmon_dev;
static struct kobject *device_kobj;
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

struct __attribute__ ((__packed__)) clei {
    u8 issue_number[3];
    u8 abbreviation_number[9];
    u8 fc_number[10];
    u8 clei_code[10];
    u8 product_year_and_month[5];
    u8 label_location_code[2];
    u8 serial_number[5];
    u8 pcb_revision[5];
    u8 vendor_name[10];
    u8 reserved[5];
};

struct __attribute__ ((__packed__))  psoc_layout {
    u8 ctl;                 //offset: 0
    u16 switch_temp;        //offset: 1

    // BYTE[03:20] - voltage
    u16 voltage[15];       //offset: 0x03-0x20

    // BYTE[21:27] - ExtFan 
    u8  led_ctl2;        //offset: 21
    u8  ext_pwm;         //offset: 22
    u16 ext_rpm[2];      //offset: 23
    u8  gpi_fan2;        //offset: 27

    //gpo
    u8 led_ctl;             //offset: 28

    u8 gpio;                //offset: 29

    //pwm duty
    u8 pwm[4];        //offset: 2a
    u8 pwm_psu[2];    //offset: 2e

    //fan rpm
    u16 fan[4*2];     //offset: 30
    
    u8  reserve1[4];        //offset: 40

    //gpi 
    u8 gpi_fan;             //offset: 44 

    //psu state
    u8 psu_state;           //offset: 45

    //temperature
    u16 temp[5];            //offset: 46
    u16 temp_psu[2];  //offset: 50

    //version
    u8 version[2];          //offset: 54
    
    u8  reserve2[4];        //offset: 56
    struct psoc_psu_layout psu_info;      //offset: 5a
};        

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

#define PWM2_OFFSET		PSOC_OFF(ext_pwm)
#define RPM2_OFFSET		PSOC_OFF(ext_rpm)
#define FAN_LED2_OFFSET         PSOC_OFF(led_ctl2)
#define FAN_GPI2_OFFSET         PSOC_OFF(gpi_fan2)

#define CLEI_OFF(m)    offsetof(struct clei, m)
#define FAN1_CLEI_INDEX         0
#define FAN2_CLEI_INDEX         1
#define FAN3_CLEI_INDEX         2
#define FAN4_CLEI_INDEX         3
#define FAN5_CLEI_INDEX		4
#define PSU1_CLEI_INDEX         5
#define PSU2_CLEI_INDEX         6

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
	struct	ipmi_system_interface_addr addr;
	struct	kernel_ipmi_msg msg;
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

static ssize_t psoc_ipmi_read(u8 *buf, u8 offset, size_t count)
{
    uint8_t data[2];
    int result_len=0;
    int rv;

    data[0] = offset;
    data[1] = count;

    rv=start_ipmi_command(NETFN_OEM, CMD_GETDATA,data,2, buf, &result_len);

    return result_len;
}

static ssize_t psoc_ipmi_write(char *buf, unsigned offset, size_t count)
{
    uint8_t data[count+1],result[1];
    int result_len;

    data[0] = offset;
    memcpy(&data[1],buf,count);

    start_ipmi_command(NETFN_OEM, CMD_SETDATA,data,count+1, result, &result_len);	
    return count;
}


static u16 psoc_read16(u8 offset)
{
	u16 value = 0;
	u8 buf[]={0,0};
    
    if(psoc_ipmi_read(buf, offset, 2) == 2)
        value = (buf[0]<<8 | buf[1]<<0);
    
	return value;
}

static u8 psoc_read8(u8 offset)
{
	u8 value = 0;
	u8 buf = 0;
    
    if(psoc_ipmi_read(&buf, offset, 1) == 1)
        value = buf;
    
	return value;
}

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
	u32 status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 byte=0;
	int shift = (attr->index == 0)?3:0;
    
    status = psoc_ipmi_read(&byte, PSOC_PSU_OFFSET, 1);
	
    byte = (byte >> shift) & 0x7;
	
	status = sprintf (buf, "%d : %s\n", byte, psu_str[byte]);
	    
	return strlen(buf);
}

static ssize_t show_ipmi_pmbus(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	uint8_t data[4],result[MaxLeng_Result];
	int result_len=0;

	data[0] = BMC_PMBusNumber;
	data[1] = (attr->index & 0xFF00 ) >>7;
	data[3] = attr->index & 0xff;
	if(data[3]==PMBus_Temp2) 
		{data[2]=2;}
	else
		{data[2]=MaxLeng_Result;}

	if(start_ipmi_command(0x06, 0x52,data,4, result, &result_len)==0)
	{
		if(data[3]==PMBus_Temp2)
		{
  			return sprintf(buf, "%ld \n", pmbus_reg2data_linear(result[0] | (result[1]<<8), 0 ));
		}
		result[result[0]+1]='\0';
	    return sprintf(buf, "%s\n",&result[1] );
	}
	else
	{
		return 0;
	}
}

static ssize_t show_clei(struct device *dev, struct device_attribute *da,
                         char *buf)
{
        struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
        u8 device_index = attr->index & 0xFF;

        uint8_t data[5],result[MaxLeng_Result];
        int result_len=0;

        data[0] = (device_index<=FAN5_CLEI_INDEX) ? BMC_FanCLEIBusNumber:BMC_PMBusNumber;
	data[1] = CLEI_ADDR[device_index]<<1;
	data[2] = sizeof(struct clei);
	data[3] = (device_index<=FAN5_CLEI_INDEX) ? 0x00 : 0x01; //PSU CLEI will start from 0x0100
	data[4] = 0;

        if(start_ipmi_command(0x06, 0x52,data,5, result, &result_len)==0)
	{
		if(result_len < sizeof(struct clei)) memset(result, 0, sizeof(struct clei));
                sprintf (buf, "Issue Number: %.3s\n", &result[CLEI_OFF(issue_number)]);
                sprintf (buf, "%sAbbreviation Number: %.9s\n", buf,  &result[CLEI_OFF(abbreviation_number)]);
                sprintf (buf, "%sFC Number: %.10s\n", buf,  &result[CLEI_OFF(fc_number)]);
                sprintf (buf, "%sCLEI Code: %.10s\n", buf,  &result[CLEI_OFF(clei_code)]);
                sprintf (buf, "%sProduct Year and Month: %.5s\n", buf,  &result[CLEI_OFF(product_year_and_month)]);
               	sprintf (buf, "%s2D Label Location Code: %.2s\n", buf,  &result[CLEI_OFF(label_location_code)]);
                sprintf (buf, "%sSerial Number: %.5s\n", buf,  &result[CLEI_OFF(serial_number)]);
                sprintf (buf, "%sPCB Revision: %.5s\n", buf,  &result[CLEI_OFF(pcb_revision)]);
                sprintf (buf, "%sVendor Name: %.10s\n", buf,  &result[CLEI_OFF(vendor_name)]);
                return strlen(buf);
	}
	else
	{
		return sprintf(buf, "NONE\n");
	}
}

static ssize_t show_thermal(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index * 2 + THERMAL_OFFSET;

	status = psoc_read16(offset);

	return sprintf(buf, "%d\n",
		       (s8)(status>>8) * 1000  );
}

static ssize_t show_pwm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index;

	status = psoc_read8(offset);

	return sprintf(buf, "%d\n",
		       status);
}

static ssize_t set_pwm(struct device *dev,
			   struct device_attribute *da,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index;
	u8 pwm = simple_strtol(buf, NULL, 10);
	if(pwm > 255) pwm = 255;  
        psoc_ipmi_write(&pwm, offset, 1);

	return count;
}


static ssize_t show_rpm(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index;
	status = psoc_read16(offset);

	return sprintf(buf, "%d\n",
		       status);
}

static ssize_t show_switch_tmp(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status=0;
	u16 temp = 0;

        status = psoc_ipmi_read((u8*)&temp, SWITCH_TMP_OFFSET, 2);
	
	status = sprintf (buf, "%d\n",  (s8)(temp>>8) * 1000  );
	    
	return strlen(buf);
}

static ssize_t set_switch_tmp(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	long temp = simple_strtol(buf, NULL, 10);
        u16 temp2 =  ( (temp/1000) <<8 ) & 0xFF00 ;
    
	psoc_ipmi_write((u8*)&temp2, SWITCH_TMP_OFFSET, 2);

	
	return count;
}

static ssize_t show_diag(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status=0;
	u8 diag_flag = 0;

        status = psoc_ipmi_read((u8*)&diag_flag, DIAG_FLAG_OFFSET, 1);

	status = sprintf (buf, "%d\n", ((diag_flag & 0x80)?1:0));
	    
	return strlen(buf);
}

static ssize_t set_diag(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	u8 value = 0;
	u8 diag = simple_strtol(buf, NULL, 10);
	
        diag = diag?1:0;

	psoc_ipmi_read((u8*)&value, DIAG_FLAG_OFFSET, 1);
	if(diag) value |= (1<<7);
	else     value &= ~(1<<7);
	psoc_ipmi_write((u8*)&value, DIAG_FLAG_OFFSET, 1);
	
	return count;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	u16 status=0;

	status = psoc_read16(VERSION_OFFSET);
	
	return sprintf(buf, "ver: %x.%x\n", (status & 0xFF00)>>8,  (status & 0xFF) );
}


static ssize_t show_fan_led(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 bit = attr->index;

	if(bit < 8) { status = psoc_read8(FAN_LED_OFFSET); }
#if FAN_NUM>4
        if(bit >= 8) { status = psoc_read8(FAN_LED2_OFFSET); bit-=8; }
#endif

	return sprintf(buf, "%d\n",
		       (status & (1<<bit))?1:0 );
}

static ssize_t set_fan_led(struct device *dev,
			   struct device_attribute *devattr,
			   const char *buf, size_t count)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
	u8 bit = attr->index;
	u8 led_state = 0;

	u8 v = simple_strtol(buf, NULL, 10);

        if(attr->index < 8) { led_state = psoc_read8(FAN_LED_OFFSET ); }
#if FAN_NUM>4
        if(attr->index >= 8) { led_state = psoc_read8(FAN_LED2_OFFSET); bit-=8; }
#endif
        if(v) led_state |=  (1<<bit);
        else  led_state &= ~(1<<bit);    

        if(attr->index < 8) { psoc_ipmi_write(&led_state, FAN_LED_OFFSET, 1);}
#if FAN_NUM>4
        if(attr->index >= 8) { psoc_ipmi_write(&led_state, FAN_LED2_OFFSET,1);}
#endif
	return count;
}

static ssize_t show_value8(struct device *dev, struct device_attribute *da,
			 char *buf)
{
	int status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index;

	status = psoc_read8(offset);
	
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
	u16 status=0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	u8 offset = attr->index + PSU_INFO_OFFSET;

	status = psoc_read16(offset);
        
	if((strstr(attr->dev_attr.attr.name, "vout")!=NULL)|(strstr(attr->dev_attr.attr.name, "in3")!=NULL)|(strstr(attr->dev_attr.attr.name, "in4")!=NULL)) {
		offset=1;
	}
	else {
		offset=0;
        }

	return sprintf(buf, "%ld \n", pmbus_reg2data_linear(status, offset ));
}

static ssize_t show_name(struct device *dev,
			struct device_attribute *devattr, char *buf)
{
	return sprintf(buf, "inv_psoc\n");
}

static DEVICE_ATTR(name, S_IRUGO, show_name, NULL);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO,			show_thermal, 0, 0);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO,			show_thermal, 0, 1);
static SENSOR_DEVICE_ATTR(temp3_input, S_IRUGO,			show_thermal, 0, 2);
static SENSOR_DEVICE_ATTR(temp4_input, S_IRUGO,			show_thermal, 0, 3);
static SENSOR_DEVICE_ATTR(temp5_input, S_IRUGO,			show_thermal, 0, 4);
static SENSOR_DEVICE_ATTR(thermal_psu1, S_IRUGO,               show_thermal, 0, 5);
static SENSOR_DEVICE_ATTR(thermal_psu2, S_IRUGO,               show_thermal, 0, 6);
static SENSOR_DEVICE_ATTR(temp7_input, S_IRUGO,                 show_thermal, 0, 5);
static SENSOR_DEVICE_ATTR(temp8_input, S_IRUGO,                 show_thermal, 0, 6);

static SENSOR_DEVICE_ATTR(pwm1, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 0 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm2, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 1 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm3, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 2 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm4, S_IWUSR|S_IRUGO,			show_pwm, set_pwm, 3 + PWM_OFFSET);
#if FAN_NUM > 4
static SENSOR_DEVICE_ATTR(pwm5, S_IWUSR|S_IRUGO,                        show_pwm, set_pwm, 0 + PWM2_OFFSET);
#endif
static SENSOR_DEVICE_ATTR(pwm_psu1, S_IWUSR|S_IRUGO,		show_pwm, set_pwm, 4 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm_psu2, S_IWUSR|S_IRUGO,		show_pwm, set_pwm, 5 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm6, S_IWUSR|S_IRUGO,            show_pwm, set_pwm, 4 + PWM_OFFSET);
static SENSOR_DEVICE_ATTR(pwm7, S_IWUSR|S_IRUGO,            show_pwm, set_pwm, 5 + PWM_OFFSET);

static SENSOR_DEVICE_ATTR(psu1,  S_IRUGO,			        show_psu_st, 0, 0);
static SENSOR_DEVICE_ATTR(psu2,  S_IRUGO,			        show_psu_st, 0, 1);

static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO,			show_rpm, 0, 0*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO,			show_rpm, 0, 1*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan3_input, S_IRUGO,			show_rpm, 0, 2*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan4_input, S_IRUGO,			show_rpm, 0, 3*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan5_input, S_IRUGO,			show_rpm, 0, 4*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan6_input, S_IRUGO,			show_rpm, 0, 5*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan7_input, S_IRUGO,			show_rpm, 0, 6*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan8_input, S_IRUGO,			show_rpm, 0, 7*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(rpm_psu1, S_IRUGO,		show_rpm, 0, 8*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(rpm_psu2, S_IRUGO,		show_rpm, 0, 9*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan11_input, S_IRUGO,            show_rpm, 0, 8*2 + RPM_OFFSET);
static SENSOR_DEVICE_ATTR(fan12_input, S_IRUGO,            show_rpm, 0, 9*2 + RPM_OFFSET);

#if FAN_NUM > 4
static SENSOR_DEVICE_ATTR(fan9_input , S_IRUGO,                  show_rpm, 0,0*2 + RPM2_OFFSET);
static SENSOR_DEVICE_ATTR(fan10_input, S_IRUGO,                  show_rpm, 0,1*2 + RPM2_OFFSET);
#endif

static SENSOR_DEVICE_ATTR(switch_tmp, S_IWUSR|S_IRUGO,			show_switch_tmp, set_switch_tmp, 0);
static SENSOR_DEVICE_ATTR(temp6_input, S_IWUSR|S_IRUGO,                  show_switch_tmp, set_switch_tmp, 0);

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

#if FAN_NUM>4
static SENSOR_DEVICE_ATTR(fan_led_grn5, S_IWUSR|S_IRUGO,                        show_fan_led, set_fan_led, 8);
static SENSOR_DEVICE_ATTR(fan_led_red5, S_IWUSR|S_IRUGO,                        show_fan_led, set_fan_led, 12);
static SENSOR_DEVICE_ATTR(fan_gpi2,     S_IRUGO,                                show_value8,  0,  FAN_GPI2_OFFSET);
#endif 

static SENSOR_DEVICE_ATTR(fan_gpi,      S_IRUGO,			        show_value8,  0,           FAN_GPI_OFFSET);
static SENSOR_DEVICE_ATTR(psoc_psu1_vin,      S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vin));
static SENSOR_DEVICE_ATTR(psoc_psu1_vout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vout));
static SENSOR_DEVICE_ATTR(psoc_psu1_iin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iin));
static SENSOR_DEVICE_ATTR(psoc_psu1_iout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iout));
static SENSOR_DEVICE_ATTR(psoc_psu1_pin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pin));
static SENSOR_DEVICE_ATTR(psoc_psu1_pout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pout));

static SENSOR_DEVICE_ATTR(in1_input,          S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vin));
static SENSOR_DEVICE_ATTR(curr1_input,        S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iin));
static SENSOR_DEVICE_ATTR(power1_input,       S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pin));
static SENSOR_DEVICE_ATTR(in3_input,          S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_vout));
static SENSOR_DEVICE_ATTR(curr3_input,        S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_iout));
static SENSOR_DEVICE_ATTR(power3_input,       S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu1_pout));

static SENSOR_DEVICE_ATTR(psoc_psu2_vin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_vout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vout));
static SENSOR_DEVICE_ATTR(psoc_psu2_iin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_iout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iout));
static SENSOR_DEVICE_ATTR(psoc_psu2_pin,      S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pin)); 
static SENSOR_DEVICE_ATTR(psoc_psu2_pout,     S_IRUGO,			        show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pout));

static SENSOR_DEVICE_ATTR(in2_input,          S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vin));
static SENSOR_DEVICE_ATTR(curr2_input,        S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iin));
static SENSOR_DEVICE_ATTR(power2_input,       S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pin));
static SENSOR_DEVICE_ATTR(in4_input,          S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_vout));
static SENSOR_DEVICE_ATTR(curr4_input,        S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_iout));
static SENSOR_DEVICE_ATTR(power4_input,       S_IRUGO,                          show_psu_psoc,  0,           PSOC_PSU_OFF(psu2_pout));

//IPMI
static SENSOR_DEVICE_ATTR(thermal2_psu1,	S_IRUGO,                show_ipmi_pmbus, 0, PSU1 | PMBus_Temp2);
static SENSOR_DEVICE_ATTR(temp9_input,          S_IRUGO,                show_ipmi_pmbus, 0, PSU1 | PMBus_Temp2);

static SENSOR_DEVICE_ATTR(psoc_psu1_vendor,	S_IRUGO,                show_ipmi_pmbus, 0, PSU1 | PMBus_Vendor);
static SENSOR_DEVICE_ATTR(psoc_psu1_serial,	S_IRUGO,                show_ipmi_pmbus, 0, PSU1 | PMBus_Serial);
static SENSOR_DEVICE_ATTR(psoc_psu1_version,	S_IRUGO,                show_ipmi_pmbus, 0, PSU1 | PMBus_Version);

static SENSOR_DEVICE_ATTR(thermal2_psu2,         S_IRUGO,                show_ipmi_pmbus, 0, PSU2 | PMBus_Temp2);
static SENSOR_DEVICE_ATTR(temp10_input,          S_IRUGO,                show_ipmi_pmbus, 0, PSU2 | PMBus_Temp2);
static SENSOR_DEVICE_ATTR(psoc_psu2_vendor,     S_IRUGO,                show_ipmi_pmbus, 0, PSU2 | PMBus_Vendor);
static SENSOR_DEVICE_ATTR(psoc_psu2_serial,     S_IRUGO,                show_ipmi_pmbus, 0, PSU2 | PMBus_Serial);
static SENSOR_DEVICE_ATTR(psoc_psu2_version,    S_IRUGO,                show_ipmi_pmbus, 0, PSU2 | PMBus_Version);

//CLEI
#if FAN_CLEI_SUPPORT
static SENSOR_DEVICE_ATTR(fan1_clei, S_IRUGO, show_clei, 0, FAN1_CLEI_INDEX );
static SENSOR_DEVICE_ATTR(fan2_clei, S_IRUGO, show_clei, 0, FAN2_CLEI_INDEX );
static SENSOR_DEVICE_ATTR(fan3_clei, S_IRUGO, show_clei, 0, FAN3_CLEI_INDEX );
static SENSOR_DEVICE_ATTR(fan4_clei, S_IRUGO, show_clei, 0, FAN4_CLEI_INDEX );
#if FAN_NUM > 4
static SENSOR_DEVICE_ATTR(fan5_clei, S_IRUGO, show_clei, 0, FAN5_CLEI_INDEX );
#endif
#endif

#if PSU_CLEI_SUPPORT
static SENSOR_DEVICE_ATTR(psu1_clei, S_IRUGO, show_clei, 0, PSU1_CLEI_INDEX );
static SENSOR_DEVICE_ATTR(psu2_clei, S_IRUGO, show_clei, 0, PSU2_CLEI_INDEX );
#endif

static struct attribute *psoc_attributes[] = {
    //name
	&dev_attr_name.attr,
    //thermal
	&sensor_dev_attr_temp1_input.dev_attr.attr,
	&sensor_dev_attr_temp2_input.dev_attr.attr,
	&sensor_dev_attr_temp3_input.dev_attr.attr,
	&sensor_dev_attr_temp4_input.dev_attr.attr,
	&sensor_dev_attr_temp5_input.dev_attr.attr,
        &sensor_dev_attr_thermal_psu1.dev_attr.attr,
        &sensor_dev_attr_thermal_psu2.dev_attr.attr,

        &sensor_dev_attr_temp7_input.dev_attr.attr,
        &sensor_dev_attr_temp8_input.dev_attr.attr,

    //pwm
	&sensor_dev_attr_pwm1.dev_attr.attr,
	&sensor_dev_attr_pwm2.dev_attr.attr,
	&sensor_dev_attr_pwm3.dev_attr.attr,
	&sensor_dev_attr_pwm4.dev_attr.attr,
#if FAN_NUM > 4
        &sensor_dev_attr_pwm5.dev_attr.attr,
#endif
	&sensor_dev_attr_pwm_psu1.dev_attr.attr,
	&sensor_dev_attr_pwm_psu2.dev_attr.attr,
        &sensor_dev_attr_pwm6.dev_attr.attr,
        &sensor_dev_attr_pwm7.dev_attr.attr,
	
	//rpm
	&sensor_dev_attr_fan1_input.dev_attr.attr,
	&sensor_dev_attr_fan2_input.dev_attr.attr,
	&sensor_dev_attr_fan3_input.dev_attr.attr,
	&sensor_dev_attr_fan4_input.dev_attr.attr,
	&sensor_dev_attr_fan5_input.dev_attr.attr,
	&sensor_dev_attr_fan6_input.dev_attr.attr,
	&sensor_dev_attr_fan7_input.dev_attr.attr,
	&sensor_dev_attr_fan8_input.dev_attr.attr,
#if FAN_NUM > 4
        &sensor_dev_attr_fan9_input.dev_attr.attr,
        &sensor_dev_attr_fan10_input.dev_attr.attr,
#endif	
	&sensor_dev_attr_rpm_psu1.dev_attr.attr,
	&sensor_dev_attr_rpm_psu2.dev_attr.attr,
        &sensor_dev_attr_fan11_input.dev_attr.attr,
        &sensor_dev_attr_fan12_input.dev_attr.attr,
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
#if FAN_NUM >4
        &sensor_dev_attr_fan_led_grn5.dev_attr.attr,
        &sensor_dev_attr_fan_led_red5.dev_attr.attr,
        &sensor_dev_attr_fan_gpi2.dev_attr.attr,
#endif
	//fan GPI 
	&sensor_dev_attr_fan_gpi.dev_attr.attr,
	&sensor_dev_attr_psu1.dev_attr.attr,
	&sensor_dev_attr_psu2.dev_attr.attr,


	//psu_psoc
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

        &sensor_dev_attr_in1_input.dev_attr.attr,
        &sensor_dev_attr_curr1_input.dev_attr.attr,
        &sensor_dev_attr_power1_input.dev_attr.attr,
        &sensor_dev_attr_in2_input.dev_attr.attr,
        &sensor_dev_attr_curr2_input.dev_attr.attr,
        &sensor_dev_attr_power2_input.dev_attr.attr,
        &sensor_dev_attr_in3_input.dev_attr.attr,
        &sensor_dev_attr_curr3_input.dev_attr.attr,
        &sensor_dev_attr_power3_input.dev_attr.attr,
        &sensor_dev_attr_in4_input.dev_attr.attr,
        &sensor_dev_attr_curr4_input.dev_attr.attr,
        &sensor_dev_attr_power4_input.dev_attr.attr,

	//ipmi_i2c_command
	&sensor_dev_attr_thermal2_psu1.dev_attr.attr,
        &sensor_dev_attr_temp9_input.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_vendor.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_serial.dev_attr.attr,
	&sensor_dev_attr_psoc_psu1_version.dev_attr.attr,

	&sensor_dev_attr_thermal2_psu2.dev_attr.attr,
        &sensor_dev_attr_temp10_input.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_vendor.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_serial.dev_attr.attr,
	&sensor_dev_attr_psoc_psu2_version.dev_attr.attr,

        //clei
#if FAN_CLEI_SUPPORT
        &sensor_dev_attr_fan1_clei.dev_attr.attr,
        &sensor_dev_attr_fan2_clei.dev_attr.attr,
        &sensor_dev_attr_fan3_clei.dev_attr.attr,
        &sensor_dev_attr_fan4_clei.dev_attr.attr,
#if FAN_NUM > 4
        &sensor_dev_attr_fan5_clei.dev_attr.attr,
#endif
#endif 

#if PSU_CLEI_SUPPORT
        &sensor_dev_attr_psu1_clei.dev_attr.attr,
        &sensor_dev_attr_psu2_clei.dev_attr.attr,
#endif
	NULL
};

static const struct attribute_group psoc_group = {
	.attrs = psoc_attributes,
};

//=================================
static void check_switch_temp(void)
{
        static struct file *f;
        mm_segment_t old_fs;

        set_fs(get_ds());
        f = filp_open(SWITCH_TEMPERATURE_SOCK,O_RDONLY,0644);
        if(IS_ERR(f)) {
                return;
        }
        else {
                char temp_str[]={0,0,0,0,0,0,0};
                loff_t pos = 0;
                u16 temp2 = 0;
                old_fs = get_fs();
                set_fs(KERNEL_DS);
                vfs_read(f, temp_str,6,&pos);
                temp2 = ((simple_strtoul(temp_str,NULL,10)/1000) <<8 ) & 0xFF00 ;
                psoc_ipmi_write((u8*)&temp2, SWITCH_TMP_OFFSET, 2);
        }
        filp_close(f,NULL);
        set_fs(old_fs);
}

static int psoc_polling_thread(void *p)
{
    while (!kthread_should_stop())
    {
        check_switch_temp();
        set_current_state(TASK_INTERRUPTIBLE);
        if(kthread_should_stop())
            break;

        schedule_timeout(msecs_to_jiffies(PSOC_POLLING_PERIOD));
    }
    return 0;
}

static int __init inv_psoc_init(void)
{
	int ret;
	
	hwmon_dev = hwmon_device_register(NULL);
	if (IS_ERR(hwmon_dev)) {
		goto fail_hwmon_device_register;
	}	

	device_kobj = kobject_create_and_add("device", &hwmon_dev->kobj);
	if(!device_kobj) {
		goto fail_hwmon_device_register;	
	}
	
	ret = sysfs_create_group(device_kobj, &psoc_group);
	if (ret) {
		goto fail_create_group_hwmon;
	}

        ret = sysfs_create_group(&hwmon_dev->kobj, &psoc_group);
        if (ret) {
                goto fail_create_group_hwmon;
        }

	kthread_auto_update = kthread_run(psoc_polling_thread,NULL,"BMC_DRIVER");
	if (IS_ERR(kthread_auto_update)) {
		goto fail_create_group_hwmon;
	}
	return ret;

fail_create_group_hwmon:
	hwmon_device_unregister(hwmon_dev);
fail_hwmon_device_register:
	return  -ENOMEM;
}

static void __exit inv_psoc_exit(void)
{
        kthread_stop(kthread_auto_update);
	if(ipmi_mh_user!=NULL) {ipmi_destroy_user(ipmi_mh_user);}
	if(hwmon_dev != NULL) hwmon_device_unregister(hwmon_dev);
	sysfs_remove_group(device_kobj, &psoc_group);
}

MODULE_AUTHOR("Ting.Jack <ting.jack@inventec>");
MODULE_DESCRIPTION("inv psoc driver");
MODULE_LICENSE("GPL");

module_init(inv_psoc_init);
module_exit(inv_psoc_exit);

