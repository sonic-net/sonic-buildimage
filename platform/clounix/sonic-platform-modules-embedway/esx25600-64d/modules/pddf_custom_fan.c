#include <linux/string.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>

#include "pddf_client_defs.h"
#include "pddf_fan_defs.h"
#include "pddf_fan_driver.h"

static int *log_level = &fan_log_level;
static struct mutex lock;
struct i2c_client cpld3_client;

/*
因硬件fan pwm只有16个档位（0~15），软件百分比和实际的档位映射如下
    percent	pwm_level
    100	    15
    94~99	14
    87~93	13
    80~86	12
    74~79	11
    67~73	10
    60~66	9
    54~59	8
    47~53	7
    40~46	6
    34~39	5
    27~33	4
    20~26	3
    14~19	2
    7~13	1
    0~6	    0
*/
/* 初始化为-1，未set pwm percent前从pwm档位寄存器换算 */
static int fan_pwm_percent = -1;//一个pwm寄存器控制所有fan

static int speed_target[MAX_NUM_FAN];
static int speed_tolerance[MAX_NUM_FAN];
static int is_bmc_armed = 0;

extern void *get_device_table(char *name);

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info);
int sonic_i2c_get_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info);
int sonic_i2c_get_target_l(void *client, FAN_DATA_ATTR *udata, void *info);
int sonic_i2c_get_speed_target(void *client, FAN_DATA_ATTR *udata, void *info);
int sonic_i2c_get_speed_tolerance(void *client, FAN_DATA_ATTR *udata, void *info);

extern FAN_SYSFS_ATTR_DATA data_fan1_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan2_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan3_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan4_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan5_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan6_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan7_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan8_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan9_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan10_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan11_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan12_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan12_pwm;
extern FAN_SYSFS_ATTR_DATA data_fan1_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan2_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan3_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan4_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan5_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan6_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan7_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan8_speed_target;
extern FAN_SYSFS_ATTR_DATA data_fan1_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan2_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan3_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan4_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan5_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan6_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan7_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan8_speed_tolerance;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l0 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l1 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l2 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l3 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l4 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l5 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l6 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l7 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l8 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l9 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l10;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l11;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l12;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l13;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l14;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_f_l15;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l0 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l1 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l2 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l3 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l4 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l5 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l6 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l7 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l8 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l9 ;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l10;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l11;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l12;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l13;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l14;
extern FAN_SYSFS_ATTR_DATA data_fan_speed_target_r_l15;

int sonic_i2c_update_target_tolerance(void *client, int pwm_level)
{
    int status = -1;
    struct i2c_client * p_client = (struct i2c_client *)client;
    struct fan_data *data = i2c_get_clientdata(client);
    FAN_PDATA *pdata = (FAN_PDATA *)(p_client->dev.platform_data);
    FAN_DATA_ATTR *usr_data = NULL;
    int i,j,ret;
    char attr_buf_f[32];
    char attr_buf_r[32];

    ret = snprintf(attr_buf_f, sizeof(attr_buf_f), "fan_speed_target_f_l%d", pwm_level);
    if (ret < 0) {
        pddf_err(FAN, "%s: pwm_level: %d\n", __FUNCTION__, pwm_level);
        return status;
    }

    ret = snprintf(attr_buf_r, sizeof(attr_buf_r), "fan_speed_target_r_l%d", pwm_level);
    if (ret < 0) {
        pddf_err(FAN, "%s: pwm_level: %d\n", __FUNCTION__, pwm_level);
        return status;
    }
    pddf_dbg(FAN, "%s: %s %s\n", __FUNCTION__, attr_buf_f,attr_buf_r);

    mutex_lock(&lock);
    for (i=0; i < data->num_attr; i++) {
        if (strcmp(attr_buf_f, pdata->fan_attrs[i].aname) == 0) {
            for (j = 0; j < MAX_NUM_FAN; j++) {
                if ((j % 2) == 0) {//front
                    usr_data = &pdata->fan_attrs[i];
                    speed_target[j] = usr_data->mult;//stored speed_target
                    speed_tolerance[j] = usr_data->len;//stored speed_tolerance
                }
            }
        }
        if (strcmp(attr_buf_r, pdata->fan_attrs[i].aname) == 0) {
            for (j = 0; j < MAX_NUM_FAN; j++) {
                if ((j % 2) != 0) {//rear
                    usr_data = &pdata->fan_attrs[i];
                    speed_target[j] = usr_data->mult;//stored speed_target
                    speed_tolerance[j] = usr_data->len;//stored speed_tolerance
                }
            }
        }
    }
    mutex_unlock(&lock);
    
    for(i=0;i<MAX_NUM_FAN;i++) {
        pddf_dbg(FAN, "%s: fan%d_speed_target:%d\n", __FUNCTION__, i,speed_target[i]);
        pddf_dbg(FAN, "%s: fan%d_speed_tolerance:%d\n", __FUNCTION__, i,speed_tolerance[i]);
    }

    return 0;
}

static int fan_cpld_client_write(FAN_DATA_ATTR *udata, uint32_t val)
{
    int status = 0;

    if (udata->len==1)
    {
        status = board_i2c_cpld_write(udata->devaddr, udata->offset, val);
    }
    else
    {
        /* Get the I2C client for the CPLD */
        struct i2c_client *client_ptr=NULL;
        client_ptr = (struct i2c_client *)get_device_table(udata->devname);
        if (client_ptr)
        {
            if (udata->len==2)
            {
                uint8_t val_lsb = val & 0xFF;
                uint8_t val_hsb = (val >> 8) & 0xFF;
                /* TODO: Check this logic for LE and BE */
                status = i2c_smbus_write_byte_data(client_ptr, udata->offset, val_lsb);
                if (status==0) status = i2c_smbus_write_byte_data(client_ptr, udata->offset+1, val_hsb);
            }
            else
                pddf_err(FAN, "Doesn't support block CPLD write yet");
        }
        else
            pddf_err(FAN, "Unable to get the client handle for %s\n", udata->devname);
    }

    return status;
}

static int fan_cpld_client_read(FAN_DATA_ATTR *udata)
{
    int status = -1;

    if (udata!=NULL)
    {
        if (udata->len==1)
        {
            status = board_i2c_cpld_read(udata->devaddr , udata->offset);
        }
        else
        {
            /* Get the I2C client for the CPLD */
            struct i2c_client *client_ptr=NULL;
            client_ptr = (struct i2c_client *)get_device_table(udata->devname);
            if (client_ptr)
            {
                if (udata->len==2)
                {
                    status = i2c_smbus_read_word_swapped(client_ptr, udata->offset);
                }
                else
                    pddf_err(FAN, "Doesn't support block CPLD read yet");
            }
            else
                pddf_err(FAN, "Unable to get the client handle for %s\n", udata->devname);
        }

    }

    return status;
}

int sonic_i2c_set_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    int val = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    if (is_bmc_armed) {
        //搭载BMC时，不允许从COMe修改风扇pwm
        pddf_info(FAN, "FAN PWM cannot be configured when BMC is armed.\n");
        return status;
    }

    val = painfo->val.intval;

    if (val > 255)
    {
        return -EINVAL;
    }
    if(val > 100)
    {
        val = 100;
    }

    //驱动层面不做差分check无条件设置寄存器
    //if (fan_pwm_percent == val) return status;

    mutex_lock(&lock);
    fan_pwm_percent = val;
    mutex_unlock(&lock);
    
    // convert 0~100 to level 0~15(mult is 15)
    val = ((val * udata->mult) / 100);

    if (val < 4) val = 4;//fixed a min level from SW

    val = (val & udata->mask);

    if (strcmp(udata->devtype, "cpld") == 0)
    {
        status = fan_cpld_client_write(udata, val);
    }

    pddf_dbg(FAN, "set fan pwm reg val(level): %d\n",val);

    return status;
}

int sonic_i2c_get_fan_pwm_custom(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;
    int val;

    if (is_bmc_armed || fan_pwm_percent == -1) {
        //搭载BMC时，因BMC未提供fan ratio值，由pwm档位寄存器换算
        if (strcmp(udata->devtype, "cpld") == 0)
        {
            val = fan_cpld_client_read(udata);
        }
        if (val < 0) {
            status = val;
        } else {
            val = val & udata->mask;
            // level 0~15(mult is 15) for 0%~100%
            val = ((val * 100) / udata->mult);
            painfo->val.intval = val;
        }
    } else {
        painfo->val.intval = fan_pwm_percent;
    }

    return status;
}

int sonic_i2c_get_target_l(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
     
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;
    painfo->val.intval = udata->mult;

    return status;
}

int sonic_i2c_update_all(void *client)
{
    int status = 0;
    int val = 0;
    struct i2c_client * p_client = (struct i2c_client *)client;
    struct fan_data *data = i2c_get_clientdata(client);
    FAN_PDATA *pdata = (FAN_PDATA *)(p_client->dev.platform_data);
    FAN_DATA_ATTR *usr_data = NULL;
    int i;

    for (i=0; i < data->num_attr; i++) {
        if (strcmp("fan1_pwm", pdata->fan_attrs[i].aname) == 0) {
            usr_data = &pdata->fan_attrs[i];
            val = fan_cpld_client_read(usr_data);

            if (val < 0) {
                status = val;
                val = 15;//any we must get a default pwm_level to update target_tolerance;
            }
            val = val & usr_data->mask;
            sonic_i2c_update_target_tolerance(client, val);
            break;
        }
    }

    return status;
}

int sonic_i2c_get_speed_target(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    sonic_i2c_update_all(client);

    if (udata->offset < MAX_NUM_FAN)
        painfo->val.intval = speed_target[udata->offset];

    return status;
}

int sonic_i2c_get_speed_tolerance(void *client, FAN_DATA_ATTR *udata, void *info)
{
    int status = 0;
    struct fan_attr_info *painfo = (struct fan_attr_info *)info;

    sonic_i2c_update_all(client);

    if (udata->offset < MAX_NUM_FAN)
        painfo->val.intval = speed_tolerance[udata->offset];

    return status;
}

#define CPLD3_ADDR 0x62
#define BMC_LED_OFFSET 0x50
#define BMC_ARMED_BIT_VAL 0x30
#define CP2112_I2C_BUS 0
static int __init pddf_custom_fan_init(void)
{
    int bmc_led;
    /*此时CPLD3还未创建不能使用board_i2c_cpld_read，改用原生i2c api*/
    cpld3_client.adapter = i2c_get_adapter(CP2112_I2C_BUS);
    cpld3_client.addr = CPLD3_ADDR;
    bmc_led = i2c_smbus_read_byte_data(&cpld3_client, BMC_LED_OFFSET);
    if (bmc_led > 0) {
        if ((BMC_ARMED_BIT_VAL & bmc_led) != 0) {
            is_bmc_armed = 1;
        }
    }
    pddf_info(FAN, "is_bmc_armed: %d bmc_led:%x\n", is_bmc_armed, bmc_led);

    data_fan1_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan1_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan2_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan2_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan3_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan3_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan4_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan4_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan5_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan5_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan6_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan6_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan7_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan7_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan8_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan8_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan9_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan9_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan10_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan10_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan11_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan11_pwm.do_set = sonic_i2c_set_fan_pwm_custom;
    data_fan12_pwm.do_get = sonic_i2c_get_fan_pwm_custom;
    data_fan12_pwm.do_set = sonic_i2c_set_fan_pwm_custom;

    data_fan_speed_target_f_l0.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l1.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l2.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l3.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l4.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l5.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l6.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l7.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l8.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l9.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l10.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l11.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l12.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l13.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l14.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_f_l15.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l0.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l1.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l2.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l3.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l4.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l5.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l6.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l7.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l8.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l9.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l10.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l11.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l12.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l13.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l14.do_get = sonic_i2c_get_target_l;
    data_fan_speed_target_r_l15.do_get = sonic_i2c_get_target_l;

    data_fan1_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan2_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan3_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan4_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan5_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan6_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan7_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan8_speed_target.do_get = sonic_i2c_get_speed_target;
    data_fan1_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan2_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan3_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan4_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan5_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan6_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan7_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;
    data_fan8_speed_tolerance.do_get = sonic_i2c_get_speed_tolerance;

    mutex_init(&lock);

    return 0;
}

static void __exit pddf_custom_fan_exit(void)
{
    mutex_destroy(&lock);
    i2c_put_adapter(cpld3_client.adapter);
    return;
}

MODULE_AUTHOR("Embedway");
MODULE_DESCRIPTION("pddf custom fan api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_fan_init);
module_exit(pddf_custom_fan_exit);
