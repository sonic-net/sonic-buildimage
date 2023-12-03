/*
 * A hwmon driver for the minipack.
 *
 * Copyright (C) 2018 Accton Technology Corporation.
 * Roy Lee <roy_lee@accton.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
 * This driver retrieves sensors' data through uart interface by opening
 * an user-space tty device node.
 * Designed pariticular for ethernet switch with BMC.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <asm/uaccess.h>


#define DEBUG_BUF_LEN       256
#define DEBUG_INTR(args...) \
    debug_print(__func__, __LINE__, 1, args)

static unsigned int verbose = 0;
module_param(verbose, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(verbose, "Print more information for debugging. Default is disabled.");

static unsigned int poll_interval = 9;
module_param(poll_interval, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(poll_interval, "Time interval for data polling, in unit of second.");


#define DRVNAME "minipack_psensor"     /*Platform Sensor*/

#define MAX_THERMAL_COUNT           8
#define MAX_FAN_COUNT               (8)
#define MAX_PSU_COUNT               (4)
#define CHASSIS_PSU_CHAR_COUNT      (2)    /*2 for input and output.*/
#define CHASSIS_LED_COUNT           (2)
#define CHASSIS_PSU_VOUT_COUNT      (1)    /*V output only.*/
#define CHASSIS_PSU_VOUT_INDEX      (1)    /*V output start index.*/
#define MAX_MODEL_NAME               21
#define MAX_SERIAL_NUMBER            19

#define ATTR_ALLOC_EXTRA	        1   /*For last attribute which is NUll.*/
#define ATTR_NAME_SIZE		        24
#define ATTR_NAME_OUTFIT_SIZE		12
#define ATTR_MAX_LIST   		8

#define TTY_DEVICE                      "/dev/ttyACM0"
#define TTY_PROMPT                      "@bmc-oob"
#define TTY_USER                        "root"
#define TTY_PASSWORD                    "0penBmc"
#define TTY_BAUDRATE                    (57600)
#define TTY_LOGIN_RETRY                 10
#define TTY_LOGIN_RETRY_INTV            (200)
#define TTY_RETRY_INTERVAL              (100)   /*mini-seconds*/
#define TTY_RESPONSE_INTERVAL           (1000)  /*mini-seconds*/
#define TTY_PMBUS_INTERVAL              (1800)  /*mini-seconds*/
#define MAXIMUM_TTY_BUFFER_LENGTH       1024
#define MAXIMUM_TTY_STRING_LENGTH       (MAXIMUM_TTY_BUFFER_LENGTH - 1)
#define TTY_CMD_RETRY                   3
#define TTY_RX_RETRY                    3
#define TTY_CMD_MAX_LEN          (64)
#define TTY_READ_MAX_LEN        (256)

#define TTY_RESP_SEPARATOR      '|'     /*For the ease to debug*/
#define MAX_ATTR_PATTERN        (8)
#define MIN_FAN_RPM             (0)
#define MAX_PSU_VOUT            (12000*1005/1000) /*12 + 0.5%*/
#define MIN_PSU_VOUT            (12000*995/1000)    /*12 - 0.5%*/

#define ATTR_TYPE_INDEX_GAP     (100)

enum sensor_type_e {
    SENSOR_TYPE_THERMAL,
    SENSOR_TYPE_FAN_RPM,
    SENSOR_TYPE_FAN_RPM_DN,
    SENSOR_TYPE_PSU1,
    SENSOR_TYPE_PSU2,
    SENSOR_TYPE_PSU3,
    SENSOR_TYPE_PSU4,
    SENSOR_TYPE_PSU1_MODEL,
    SENSOR_TYPE_PSU2_MODEL,
    SENSOR_TYPE_PSU3_MODEL,
    SENSOR_TYPE_PSU4_MODEL,
    SENSOR_TYPE_PSU1_SERIAL,
    SENSOR_TYPE_PSU2_SERIAL,
    SENSOR_TYPE_PSU3_SERIAL,
    SENSOR_TYPE_PSU4_SERIAL,
    SENSOR_TYPE_MAX,
};

enum sysfs_attributes_index {
    INDEX_THRM_IN_START     = SENSOR_TYPE_THERMAL   *ATTR_TYPE_INDEX_GAP,
    INDEX_FAN_RPM_START     = SENSOR_TYPE_FAN_RPM   *ATTR_TYPE_INDEX_GAP,
    INDEX_FAN_RPM_START_DN  = SENSOR_TYPE_FAN_RPM_DN*ATTR_TYPE_INDEX_GAP,
    INDEX_PSU1_START        = SENSOR_TYPE_PSU1      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU2_START        = SENSOR_TYPE_PSU2      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU3_START        = SENSOR_TYPE_PSU3      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU4_START        = SENSOR_TYPE_PSU4      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU1_MODEL        = SENSOR_TYPE_PSU1_MODEL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU2_MODEL        = SENSOR_TYPE_PSU2_MODEL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU3_MODEL        = SENSOR_TYPE_PSU3_MODEL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU4_MODEL        = SENSOR_TYPE_PSU4_MODEL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU1_SERIAL       = SENSOR_TYPE_PSU1_SERIAL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU2_SERIAL       = SENSOR_TYPE_PSU2_SERIAL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU3_SERIAL       = SENSOR_TYPE_PSU3_SERIAL      *ATTR_TYPE_INDEX_GAP,
    INDEX_PSU4_SERIAL       = SENSOR_TYPE_PSU4_SERIAL      *ATTR_TYPE_INDEX_GAP,
    INDEX_NAME              = SENSOR_TYPE_MAX       *ATTR_TYPE_INDEX_GAP,
};

/* Pmbus reg defines are copied from drivers/hwmon/pmbus/pmbus.h*/
#define PMBUS_READ_VIN			0x88
#define PMBUS_READ_IIN			0x89
#define PMBUS_READ_VOUT			0x8B
#define PMBUS_READ_IOUT			0x8C
#define PMBUS_READ_POUT			0x96
#define PMBUS_READ_PIN			0x97
#define PMBUS_READ_FAN			0x90
#define PMBUS_READ_TEMP			0x8D


#define PMBUS_REG_START  PMBUS_READ_VIN
#define PMBUS_REG_END    PMBUS_READ_PIN

enum psu_data_e {
    PSU_DATA_VIN = 0,
    PSU_DATA_VOUT,
    PSU_DATA_IIN,
    PSU_DATA_IOUT,
    PSU_DATA_PIN,
    PSU_DATA_POUT,
    PSU_DATA_FAN,
    PSU_DATA_TEMP,
    PSU_DATA_MAX,
};

struct pmbus_reg_t {
    u8   addr;
    bool power;     /*power attribute is in unit of uW instead of mW.*/
} pmbus_regs[] = {
    [PSU_DATA_VIN ] = {PMBUS_READ_VIN, false},
    [PSU_DATA_VOUT] = {PMBUS_READ_VOUT, false},
    [PSU_DATA_IIN ] = {PMBUS_READ_IIN, false},
    [PSU_DATA_IOUT] = {PMBUS_READ_IOUT, false},
    [PSU_DATA_PIN ] = {PMBUS_READ_PIN, true},
    [PSU_DATA_POUT] = {PMBUS_READ_POUT, true},
    [PSU_DATA_FAN] =  {PMBUS_READ_FAN, false},
    [PSU_DATA_TEMP] = {PMBUS_READ_TEMP, false},
};

struct sensor_data {
    int lm75_input[MAX_THERMAL_COUNT];
    int fan_rpm[MAX_FAN_COUNT];
    int fan_rpm_dn[MAX_FAN_COUNT];
    int psu_data [MAX_PSU_COUNT][PSU_DATA_MAX];
    int psu_model [MAX_PSU_COUNT][MAX_MODEL_NAME];
    int psu_serial [MAX_PSU_COUNT][MAX_SERIAL_NUMBER];
    int led_bright[CHASSIS_LED_COUNT];
};

struct psensor {
    struct psensor *next;
    char name[ATTR_NAME_SIZE+1];	/* sysfs sensor name */
    struct sensor_device_attribute sensor_dev_attr;
};

struct minipack_data {
    struct platform_device *pdev;
    struct device	    *dev;
    struct device	    *hwmon_dev;
    struct mutex	    update_lock;
    struct tty_struct   *tty;
    struct ktermios     old_ktermios;
    bool			 logged_in;
    bool			 valid[SENSOR_TYPE_MAX];
    unsigned long	 last_updated[SENSOR_TYPE_MAX];	  /* In jiffies */
    struct sensor_data sdata;
    int num_attributes;
    struct attribute_group group;
};

typedef ssize_t (*show_func)( struct device *dev,
                              struct device_attribute *attr, char *buf);
typedef ssize_t (*store_func)(struct device *dev,
                              struct device_attribute *attr, const char *buf,
                              size_t count);
static ssize_t show_name(struct device *dev, struct device_attribute *da,
                         char *buf);
static ssize_t _attr_show(struct device *dev, struct device_attribute *da,
                          char *buf);
static ssize_t show_fan_min(struct device *dev, struct device_attribute *da,
                            char *buf);
static ssize_t show_psu_vout_min(struct device *dev, struct device_attribute *da,
                                 char *buf);
static ssize_t show_psu_vout_max(struct device *dev, struct device_attribute *da,
                                 char *buf);
static int add_attr2group(struct minipack_data *data, struct attribute *attr);


struct attr_pattern {
    int  count;              /*How many attr of this pattern*/
    int  index_addon;        /* For attribute naming, e.g.
                              * X of "fanX_input",start from a number but 0.
                              */
    char prefix[ATTR_NAME_OUTFIT_SIZE+1];        /*e.g. "temp"4_input*/
    char postfix[ATTR_NAME_OUTFIT_SIZE+1];       /*e.g. temp4"_input"*/
    umode_t     mode;
    show_func   show;
    store_func  store;
};

struct sensor_set {
    int  total;         /*How many attr of this type*/
    int  start_index;
    char *query;
    u32  mdelay;        /*TTY delay to read after sent.*/
    struct attr_pattern *ptn[MAX_ATTR_PATTERN];
};



#define DECLARE_PSU_SENSOR_DEVICE_ATTR(index) \
static struct attr_pattern psu##index##_vin =  {CHASSIS_PSU_CHAR_COUNT, \
CHASSIS_PSU_CHAR_COUNT*index, "in","_input", S_IRUGO, _attr_show, NULL}; \
static struct attr_pattern psu##index##_curr =  {CHASSIS_PSU_CHAR_COUNT, \
CHASSIS_PSU_CHAR_COUNT*index, "curr","_input", S_IRUGO, _attr_show, NULL}; \
static struct attr_pattern psu##index##_pwr =  {CHASSIS_PSU_CHAR_COUNT, \
CHASSIS_PSU_CHAR_COUNT*index, "power","_input", S_IRUGO, _attr_show, NULL}; \
static struct attr_pattern psu##index##_fan_speed =  {1, \
1*index, "psu","_fan_speed", S_IRUGO, _attr_show, NULL}; \
static struct attr_pattern psu##index##_temp_input =  {1, \
1*index, "psu","_temp_input", S_IRUGO, _attr_show, NULL}; \
static struct attr_pattern psu##index##_vmax =  {CHASSIS_PSU_VOUT_COUNT, \
CHASSIS_PSU_CHAR_COUNT*index + 1, "in","_max", S_IRUGO, show_psu_vout_max, NULL};\
static struct attr_pattern psu##index##_vmin =  {CHASSIS_PSU_VOUT_COUNT, \
CHASSIS_PSU_CHAR_COUNT*index + 1, "in","_min", S_IRUGO, show_psu_vout_min, NULL}

#define DECLARE_PSU_ATTR(index) \
    &psu##index##_vin, &psu##index##_curr, &psu##index##_pwr, \
    &psu##index##_fan_speed, &psu##index##_temp_input ,&psu##index##_vmax, &psu##index##_vmin

#define DECLARE_PSU_MODEL_NAME_SENSOR_DEVICE_ATTR(index) \
static struct attr_pattern psu##index##_model_name =  {1, \
1*index, "psu","_model_name", S_IRUGO, _attr_show, NULL};

#define DECLARE_PSU_MODEL_NAME_ATTR(index) \
    &psu##index##_model_name
    
#define DECLARE_PSU_SERIAL_NUMBER_SENSOR_DEVICE_ATTR(index) \
static struct attr_pattern psu##index##_serial_name =  {1, \
1*index, "psu","_serial_num", S_IRUGO, _attr_show, NULL};

#define DECLARE_PSU_SERIAL_NUMBER_ATTR(index) \
    &psu##index##_serial_name


DECLARE_PSU_SENSOR_DEVICE_ATTR(0);
DECLARE_PSU_SENSOR_DEVICE_ATTR(1);
DECLARE_PSU_SENSOR_DEVICE_ATTR(2);
DECLARE_PSU_SENSOR_DEVICE_ATTR(3);

DECLARE_PSU_MODEL_NAME_SENSOR_DEVICE_ATTR(0);
DECLARE_PSU_MODEL_NAME_SENSOR_DEVICE_ATTR(1);
DECLARE_PSU_MODEL_NAME_SENSOR_DEVICE_ATTR(2);
DECLARE_PSU_MODEL_NAME_SENSOR_DEVICE_ATTR(3);

DECLARE_PSU_SERIAL_NUMBER_SENSOR_DEVICE_ATTR(0);
DECLARE_PSU_SERIAL_NUMBER_SENSOR_DEVICE_ATTR(1);
DECLARE_PSU_SERIAL_NUMBER_SENSOR_DEVICE_ATTR(2);
DECLARE_PSU_SERIAL_NUMBER_SENSOR_DEVICE_ATTR(3);


static char tty_cmd[SENSOR_TYPE_MAX][TTY_CMD_MAX_LEN] = {
    "ls -v /sys/class/hwmon/hwmon*/temp1_input|head -n "\
    __stringify(MAX_THERMAL_COUNT)"| xargs cat\r",
    "ls -v /sys/bus/i2c/devices/72-0033/fan*_input | xargs cat\r",
    "ls -v /sys/bus/i2c/devices/64-0033/fan*_input | xargs cat\r",
    "i2cdump -y -f -r "\
    __stringify(PMBUS_REG_START)"-" __stringify(PMBUS_REG_END)\
    " 49 0x59 w\r",
    "i2cdump -y -f -r "\
    __stringify(PMBUS_REG_START)"-" __stringify(PMBUS_REG_END)\
    " 48 0x58 w\r",
    "i2cdump -y -f -r "\
    __stringify(PMBUS_REG_START)"-" __stringify(PMBUS_REG_END)\
    " 57 0x59 w\r",
    "i2cdump -y -f -r "\
    __stringify(PMBUS_REG_START)"-" __stringify(PMBUS_REG_END)\
    " 56 0x58 w\r",
    "i2cdump -y -f 49 0x59 s 0x9a|tail -n +2|cut -c56-\r",    
    "i2cdump -y -f 48 0x58 s 0x9a|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 57 0x59 s 0x9a|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 56 0x58 s 0x9a|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 49 0x59 s 0x9e|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 48 0x58 s 0x9e|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 57 0x59 s 0x9e|tail -n +2|cut -c56-\r",
    "i2cdump -y -f 56 0x58 s 0x9e|tail -n +2|cut -c56-\r",
};

static struct attr_pattern temp_in =
{   MAX_THERMAL_COUNT, 0, "temp","_input",
    S_IRUGO, _attr_show, NULL
};
static struct attr_pattern fan_in_up =
{   MAX_FAN_COUNT, 0, "fan","_input",
    S_IRUGO, _attr_show, NULL
};
static struct attr_pattern fan_min_up =
{   MAX_FAN_COUNT, 0, "fan","_min",
    S_IRUGO, show_fan_min, NULL
};
static struct attr_pattern fan_in_dw =
{   MAX_FAN_COUNT, MAX_FAN_COUNT, "fan","_input",
    S_IRUGO, _attr_show, NULL
};
static struct attr_pattern fan_min_dw =
{   MAX_FAN_COUNT, MAX_FAN_COUNT, "fan","_min",
    S_IRUGO, show_fan_min, NULL
};

struct sensor_set model_ssets[SENSOR_TYPE_MAX] =
{
    {   MAX_THERMAL_COUNT, INDEX_THRM_IN_START, tty_cmd[SENSOR_TYPE_THERMAL],
        TTY_RESPONSE_INTERVAL, {&temp_in, NULL},
    },
    {   MAX_FAN_COUNT, INDEX_FAN_RPM_START, tty_cmd[SENSOR_TYPE_FAN_RPM],
        TTY_RESPONSE_INTERVAL, {&fan_in_up, &fan_min_up, NULL},
    },
    {   MAX_FAN_COUNT, INDEX_FAN_RPM_START_DN, tty_cmd[SENSOR_TYPE_FAN_RPM_DN],
        TTY_RESPONSE_INTERVAL, {&fan_in_dw, &fan_min_dw, NULL},
    },
    {   PSU_DATA_MAX, INDEX_PSU1_START, tty_cmd[SENSOR_TYPE_PSU1],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_ATTR(0), NULL},
    },
    {   PSU_DATA_MAX, INDEX_PSU2_START, tty_cmd[SENSOR_TYPE_PSU2],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_ATTR(1), NULL},
    },
    {   PSU_DATA_MAX, INDEX_PSU3_START, tty_cmd[SENSOR_TYPE_PSU3],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_ATTR(2), NULL},
    },
    {   PSU_DATA_MAX, INDEX_PSU4_START, tty_cmd[SENSOR_TYPE_PSU4],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_ATTR(3), NULL},
    },
    {   1, INDEX_PSU1_MODEL, tty_cmd[SENSOR_TYPE_PSU1_MODEL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_MODEL_NAME_ATTR(0), NULL},        
    },
    {   1, INDEX_PSU2_MODEL, tty_cmd[SENSOR_TYPE_PSU2_MODEL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_MODEL_NAME_ATTR(1), NULL},        
    },
    {   1, INDEX_PSU3_MODEL, tty_cmd[SENSOR_TYPE_PSU3_MODEL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_MODEL_NAME_ATTR(2), NULL},
    },
    {   1, INDEX_PSU4_MODEL, tty_cmd[SENSOR_TYPE_PSU4_MODEL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_MODEL_NAME_ATTR(3), NULL},        
    },
     {   1, INDEX_PSU1_SERIAL, tty_cmd[SENSOR_TYPE_PSU1_SERIAL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_SERIAL_NUMBER_ATTR(0), NULL},        
    },
    {   1, INDEX_PSU2_SERIAL, tty_cmd[SENSOR_TYPE_PSU2_SERIAL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_SERIAL_NUMBER_ATTR(1), NULL},        
    },
    {   1, INDEX_PSU3_SERIAL, tty_cmd[SENSOR_TYPE_PSU3_SERIAL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_SERIAL_NUMBER_ATTR(2), NULL},
    },
    {   1, INDEX_PSU4_SERIAL, tty_cmd[SENSOR_TYPE_PSU4_SERIAL],
        TTY_PMBUS_INTERVAL, {DECLARE_PSU_SERIAL_NUMBER_ATTR(3), NULL},        
    },
    
};
static struct minipack_data *mp_data = NULL;

/*-----------------------------------------------------------------------*/

static void debug_print(const char *func, int line, int level, 
                        const char *fmt, ...)
{
    va_list args;
    char buf[DEBUG_BUF_LEN];
    if (verbose >= level) {
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        pr_info("[DBG]%s#%d: %s\n", func, line, buf);
    }
}



static int _tty_wait(u32 mdelay) {
    if (mdelay) {
        msleep(mdelay);
    }
    return 0;
}

static int _tty_open(struct file **fd)
{
    struct ktermios kt;
    struct tty_struct *tty;
    speed_t baudrate = 57600;
    struct file *tty_fd = *fd;

    /*TTY must be not opened.*/
    if (tty_fd != NULL) {
        return -EINVAL;
    }

    tty_fd = filp_open(TTY_DEVICE, O_RDWR|O_NOCTTY|O_NDELAY, 0644);
    if (IS_ERR(tty_fd)) {
        DEBUG_INTR("Failed to open file(%s)\r\n", TTY_DEVICE);
        return -ENOENT;
    } else {
        spin_lock(&tty_fd->f_lock);
        tty = ((struct tty_file_private *)tty_fd ->private_data)->tty;
        mp_data->old_ktermios = (tty->termios);

        kt = tty->termios;
        tty_termios_encode_baud_rate(&kt, baudrate, baudrate);
        kt.c_cflag = B57600 | CS8 | CLOCAL | CREAD;
        kt.c_iflag = IGNPAR;
        kt.c_oflag = 0;
        kt.c_lflag = 0;
        kt.c_cc[VMIN] = 0;
        kt.c_cc[VTIME] = 0;
        tty_set_termios(tty, &kt);

        mp_data->tty = tty ;
        *fd = tty_fd;
        return 0;
    }
    return -ENXIO;
}

static int _tty_close(struct file **tty_fd)
{
    struct file *fd = *tty_fd;

    if(*tty_fd == NULL) {
        return -EINVAL;
    }
    spin_unlock(&fd->f_lock);
    filp_close(*tty_fd, 0);
    *tty_fd = NULL;

    return 0;
}

static int _tty_tx(struct file *tty_fd, const char *str)
{
    int rc;

    /*Sanity check*/
    if (tty_fd == NULL)
        return -EINVAL;

    rc = kernel_write(tty_fd, str, strlen(str)+1,0);
    
    if (rc < 0) {
        pr_info( "failed to write(%d)\n", rc);
        return -EBUSY;
    }
    DEBUG_INTR("[TX]%s-%d, %d BYTES, write:\n\"%s\"\n", __func__, __LINE__,rc, str);
    return rc;
}

static int _tty_rx(struct file *tty_fd, char *buf, int max_len)
{
    int rc;
    u32 timeout = 0;

    /*Sanity check*/
    if (tty_fd == NULL)
        return -EINVAL;

    /*Clear for remained data cause ambiguous string*/
    memset(buf, 0, max_len);
    do {
        rc = kernel_read(tty_fd, buf, max_len, 0);
        if (rc == 0) {  /*Buffer Empty, waits. */
            timeout++;
            _tty_wait(TTY_RETRY_INTERVAL);
            continue;
        } else {
            break;
        }
    } while (rc < 0 && timeout < TTY_RX_RETRY);

    if (timeout == TTY_RX_RETRY) {
        rc = -EAGAIN;
    }
    DEBUG_INTR("[RX]%s-%d, %d BYTES, read:\n\"%s\"\n", __func__, __LINE__,rc, buf);
    return rc;
}

/*Clear Rx buffer by reading it out.*/
static int _tty_clear_rxbuf(struct file *tty_fd, char* buf, size_t max_size) {
    int rc, i;
    int retry = TTY_CMD_RETRY;

    if (tty_fd == NULL) {
        return -EINVAL;
    }

    i = 0;
    do {
        rc = kernel_read(tty_fd, buf, max_size, 0);
        memset(buf, 0, max_size);
        i++;
    } while (rc > 0 && i < retry);

    return rc;
}

static int _tty_writeNread(struct file *tty_fd,
                           char *wr_p, char *rd_p, int rx_max_len, u32 mdelay)
{
    int     rc;

    /*Presumed file is opened!*/
    if (tty_fd == NULL)
        return -EINVAL;

    memset(rd_p, 0, rx_max_len);

    rc = _tty_tx(tty_fd, wr_p);
    if (rc < 0) {
        DEBUG_INTR( "failed to write(%d)\n", rc);
        goto exit;
    }
    _tty_wait(mdelay);
    rc = _tty_rx(tty_fd, rd_p, rx_max_len);
    if (rc < 0) {
        DEBUG_INTR("failed to read(%d)\n", rc);
        goto exit;
    }

exit:
    return rc;
}

static bool _is_logged_in(struct file *tty_fd, char* buf, size_t max_size)
{
    int i, ret;

    if (mp_data->logged_in) {
        return true;
    }

    _tty_clear_rxbuf(tty_fd, buf, max_size);
    for (i = 0; i < TTY_LOGIN_RETRY; i++) {
        ret = _tty_writeNread(tty_fd, "\r\r", buf, max_size, TTY_RETRY_INTERVAL);
        if (ret < 0) {
            DEBUG_INTR("%s-%d, failed ret:%d\n", __func__, __LINE__, ret);
            continue;
        }

        DEBUG_INTR("%s-%d, tty_buf:%s\n", __func__, __LINE__, buf);
        /*Check if logined by comparing BMC's cmd prompt.*/
        if (strstr(buf, TTY_PROMPT) != NULL) {
            mp_data->logged_in = true;
            return true;
        } else {
            return false;
        }
    }
    return false;

}

static int _tty_login(struct file *tty_fd, char* buf, size_t max_size)
{
    int i;
    int ret = -EINVAL;

    if(!tty_fd)
        return -EINVAL;

    for (i = 1; i <= TTY_LOGIN_RETRY; i++) {
        if (_is_logged_in(tty_fd, buf, max_size))
            return 0;

        DEBUG_INTR("%s-%d, tty_buf:%s\n", __func__, __LINE__, buf);
        if ((strstr(buf, "bmc") != NULL) &&
                (strstr(buf, "login:") != NULL))
        {
            ret = _tty_writeNread(tty_fd, TTY_USER"\r",
                                  buf, max_size, TTY_LOGIN_RETRY_INTV);
            if (ret < 0) {
                DEBUG_INTR("%s-%d, failed ret:%d\n", __func__, __LINE__, ret);
                continue;
            }
            DEBUG_INTR("%s-%d, tty_buf:%s\n", __func__, __LINE__, buf);
            if (strstr(buf, "Password:") != NULL) {
                DEBUG_INTR("%s-%d, tty_buf:%s\n", __func__, __LINE__, buf);
                ret = _tty_writeNread(tty_fd, TTY_PASSWORD"\r", buf, max_size, 0);
                if (ret < 0) {
                    DEBUG_INTR("%s-%d, failed ret:%d\n", __func__, __LINE__, ret);
                    continue;
                }
                if (strstr(buf, TTY_PROMPT) != NULL) {
                    DEBUG_INTR("%s-%d, tty_buf:%s\n", __func__, __LINE__, buf);
                    return 0;
                }
            }
        }
        _tty_wait(TTY_RETRY_INTERVAL);
    }

    dev_err(mp_data->dev, "Failed on %s ret:%d\n", __func__, ret);
    return -EAGAIN;
}

static int
bmc_transaction(char *cmd, char* resp, int max, u32 mdelay)
{
    u32  i;
    struct file *tty_fd = NULL;
    char *buf;
    int buf_size = MAXIMUM_TTY_BUFFER_LENGTH;
    int ret = 0;

    if(!cmd || !resp)
        return -EINVAL;

    if (_tty_open(&tty_fd) != 0) {
        DEBUG_INTR("ERROR: Cannot open TTY device\n");
        return -EAGAIN;
    }
    buf = (char *)kmalloc(buf_size, GFP_KERNEL);
    if (!buf) {
        ret = -ENOMEM;
        goto exit;
    }
    if (_tty_login(tty_fd, buf, buf_size) != 0) {
        dev_err(mp_data->dev, "Failed to login TTY device\n");
        _tty_close(&tty_fd);
        ret = -ENOENT;
        goto exit;
    }

    i = 0;
    do {
        ret = _tty_writeNread(tty_fd, cmd, buf, buf_size,
                              mdelay+((i)*500));
        if (ret < 0) {
            goto exit;
        }
        i++;
    } while(strstr(buf, cmd) == NULL && i <= TTY_CMD_RETRY);
    if (i > TTY_CMD_RETRY) {
        ret = -ENOENT;
        goto exit;
    }
    strncpy(resp, buf, max);
exit:
    kfree(buf);
    _tty_close(&tty_fd);
    return ret;
}

static void dev_attr_init(struct device_attribute *dev_attr,
                          const char *name, umode_t mode,
                          show_func show, store_func store)
{
    sysfs_attr_init(&dev_attr->attr);
    dev_attr->attr.name = name;
    dev_attr->attr.mode = mode;
    dev_attr->show = show;
    dev_attr->store = store;
}

/*Allowcat sensor_device_attributes and adds them to a group.*/
static int attributs_init(struct minipack_data *data)
{
    struct sensor_set *model = model_ssets;
    char name[64] = {0};
    int start, pi, si, ai, num, ret, acc;
    struct psensor *sensor;
    struct sensor_device_attribute *sensor_dattr;
    struct device_attribute *dev_attr;

    /*name*/
    sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
    if (!sensor)
        return -ENOENT;
    sensor_dattr = &sensor->sensor_dev_attr;
    dev_attr = &sensor_dattr->dev_attr;
    snprintf(sensor->name, sizeof(sensor->name), "name");
    dev_attr_init(dev_attr, sensor->name, S_IRUGO, show_name, NULL);
    sensor_dattr->index = INDEX_NAME;
    ret = add_attr2group(data, &dev_attr->attr);
    if (ret)
        return ret;

    /*types*/
    for (si = 0; si < SENSOR_TYPE_MAX; si++) {
        struct sensor_set *ss = model+si;
        num = ss->total;
        start = ss->start_index;
        acc = start;
        for (pi = 0; ss->ptn[pi]; pi++)
        {
            char *prefix, *postfix;
            struct attr_pattern *ptn = ss->ptn[pi];
            prefix = ptn->prefix;
            postfix = ptn->postfix;
            for (ai = 0; ai < ptn->count ; ai++) {
                u32 ptn_index =  ai + ptn->index_addon + 1 ;
                snprintf(name, sizeof(name), "%s%d%s",prefix, ptn_index, postfix);
                sensor = devm_kzalloc(data->dev, sizeof(*sensor), GFP_KERNEL);
                if (!sensor)
                    return -ENOENT;

                sensor_dattr = &sensor->sensor_dev_attr;
                dev_attr = &sensor_dattr->dev_attr;
                snprintf(sensor->name, sizeof(sensor->name), name);
                dev_attr_init(dev_attr, sensor->name, ptn->mode,
                              ptn->show, ptn->store);
                sensor_dattr->index = acc + ai;

                DEBUG_INTR("%s-%d, name:%s index:%d\n", __func__, __LINE__,
                           name, sensor_dattr->index);
                ret = add_attr2group(data, &dev_attr->attr);
                if (ret)
                    return ret;
            }
            acc += ptn->count;
        }
    }

    return 0 ;
}

static void mp_data_init(struct minipack_data *data)
{

}

static int extract_numbers(char *buf, int *out, int out_cnt)
{
    char *ptr;
    int  cnt, x;

    ptr = buf;
    /*replace non-digits into '|', for sscanf(%s)'s ease to handle it.*/
    for (x = 0; x < strlen(ptr); x++) {
        if( (ptr[x]<'0' || ptr[x] >'9') && ptr[x] != '-')
            ptr[x] = TTY_RESP_SEPARATOR;
    }

    cnt = 0;
    while (strchr(ptr, TTY_RESP_SEPARATOR))
    {
        if (sscanf(ptr,"%d%s",out,ptr)) {
            cnt++;
            out++;
            if (cnt == out_cnt) {
                return 0;
            }
        } else {
            ptr++;
        }
    }

    return  -EINVAL;
}

static int extract_2byteHex(char *buf, int *out, int out_cnt)
{
    char tmp[TTY_READ_MAX_LEN];
    char *ptr;
    int  cnt, x;
    long y;


    if (strstr(buf, "XXXX") != NULL) {
        return -ENODEV;
    }

    ptr = buf;
    /*replace non-hex into '|', for sscanf(%s)'s ease to handle it.*/
    for (x = 0; x < strlen(ptr); x++) {
        if((ptr[x]>='0' && ptr[x] <= '9') || (ptr[x] >= 'a' && ptr[x] <= 'f'))
            ;
        else
            ptr[x] = TTY_RESP_SEPARATOR;
    }

    cnt = 0;
    while (strchr(ptr, TTY_RESP_SEPARATOR))
    {
        ptr++;
        if (strlen(ptr) && *ptr == TTY_RESP_SEPARATOR) {
            continue;
        }
        if (sscanf(ptr,"%4s",tmp)) {
            if(!strchr(tmp, TTY_RESP_SEPARATOR)) {
                if (kstrtol(tmp, 16, &y) < 0)
                    return -EINVAL;
                *out = (int)y;
                cnt++;
                out++;
                if (cnt == out_cnt) {
                    return 0;
                }
                ptr += 4;
            }
        } else {
            break;
        }
    }

    return  -EINVAL;
}

int pmbus_linear11(int in, bool power) {
    int exponent;
    int mantissa, val;
    int data = in;

    exponent = ((short)data) >> 11;
    mantissa = ((short)((data & 0x7ff) << 5)) >> 5;

    val = mantissa*1000;
    if (power)
        val *= 1000;
    if (exponent >= 0)
        val <<= exponent;
    else
        val >>= -exponent;

    return val;
}

static int get_pmbus_regs_partial(int *in, int in_cnt, int *out, int *out_cnt)
{
    int i, j, k;
    k = 0;
    for (j = 0; j < ARRAY_SIZE(pmbus_regs); j++) {
        for (i = 0; i < in_cnt; i++) {
            if (i == (pmbus_regs[j].addr - PMBUS_REG_START))  {
                *out = pmbus_linear11(in[i], pmbus_regs[j].power);
                out++;
                k++;
                break;
            }
        }
    }
    if (k == 0)
        return -EINVAL;

    *out_cnt = k;
    return 0;
}

static int extract_psu_str(int type, char *buf, int *out, int len)
{
    char *ptr;
    int  x, i;

    ptr = buf;
    
    i=0;
    for (x = 0; x < strlen(ptr); x++) {
        if(ptr[x] > 0x7A || ptr[x] <0x1E)             
            continue;

        out[i]=ptr[x];
        i++;
        if(i==(len-2))
            break;
    }
    out[len-1]=0x0;

    return  0;
}

static int comm2BMC(enum sensor_type_e type, int *out, int out_cnt)
{
    char cmd[TTY_CMD_MAX_LEN], resp[TTY_READ_MAX_LEN];
    char *ptr;
    int ret;

    if (out == NULL)
        return -EINVAL;
    if (out_cnt == 0)
        return 0;

    snprintf(cmd, sizeof(cmd), model_ssets[type].query);
    DEBUG_INTR("%s-%d, cmd:%s\n", __func__, __LINE__, cmd);
    ret = bmc_transaction(cmd, resp, sizeof(resp)-1, model_ssets[type].mdelay);
    if (ret < 0) {
        return ret;
    }
    /*Strip off string of command just sent, if any.*/
    if (strstr(resp, cmd) != NULL) {
        ptr = resp + strlen(cmd);
    } else {
        ptr = resp;
    }

    switch (type) {
    case SENSOR_TYPE_THERMAL:
    case SENSOR_TYPE_FAN_RPM:
    case SENSOR_TYPE_FAN_RPM_DN:
        ret = extract_numbers(ptr, out, out_cnt);
        break;
    case SENSOR_TYPE_PSU1:
    case SENSOR_TYPE_PSU2:
    case SENSOR_TYPE_PSU3:
    case SENSOR_TYPE_PSU4:
    {
        int reg[(PMBUS_REG_END - PMBUS_REG_START) +1];
        int total = (PMBUS_REG_END - PMBUS_REG_START) +1;
        ret = extract_2byteHex(ptr, reg, total);
        if(ret == 0)
            get_pmbus_regs_partial(reg, total, out, &out_cnt);
        break;
    }
    case SENSOR_TYPE_PSU1_MODEL:
    case SENSOR_TYPE_PSU2_MODEL:
    case SENSOR_TYPE_PSU3_MODEL:
    case SENSOR_TYPE_PSU4_MODEL:
        ret=extract_psu_str(type, ptr, out, MAX_MODEL_NAME);
        break;
    case SENSOR_TYPE_PSU1_SERIAL:
    case SENSOR_TYPE_PSU2_SERIAL:
    case SENSOR_TYPE_PSU3_SERIAL:
    case SENSOR_TYPE_PSU4_SERIAL:
        ret=extract_psu_str(type ,ptr, out, MAX_SERIAL_NUMBER);
        break;
    default:
        return -EINVAL;
    }

    return ret;
}

static int get_type_data (
    struct sensor_data *data, enum sensor_type_e type, int index,
    int **out, int *count)
{
    struct sensor_set *model = model_ssets;

    switch (type) {
    case SENSOR_TYPE_THERMAL:
        *out = &data->lm75_input[index];
        break;
    case SENSOR_TYPE_FAN_RPM:
        *out = &data->fan_rpm[index];
        break;
    case SENSOR_TYPE_FAN_RPM_DN:
        *out = &data->fan_rpm_dn[index];
        break;
    case SENSOR_TYPE_PSU1:
    case SENSOR_TYPE_PSU2:
    case SENSOR_TYPE_PSU3:
    case SENSOR_TYPE_PSU4:
        *out = &data->psu_data[type-SENSOR_TYPE_PSU1][index];
        break;
    case SENSOR_TYPE_PSU1_MODEL:
    case SENSOR_TYPE_PSU2_MODEL:
    case SENSOR_TYPE_PSU3_MODEL:
    case SENSOR_TYPE_PSU4_MODEL:
        *out = &data->psu_model[type-SENSOR_TYPE_PSU1_MODEL][index];
        break;
    case SENSOR_TYPE_PSU1_SERIAL:
    case SENSOR_TYPE_PSU2_SERIAL:
    case SENSOR_TYPE_PSU3_SERIAL:
    case SENSOR_TYPE_PSU4_SERIAL:
        *out = &data->psu_serial[type-SENSOR_TYPE_PSU1_SERIAL][index];
        break;
    default:
        return -EINVAL;
    }
    *count = model[type].total;
    return 0;
}

static struct sensor_data*
update_data(struct device *dev, enum sensor_type_e type) {
    struct minipack_data *data = mp_data;
    bool			*valid = &data->valid[type];
    unsigned long   *last_updated = &data->last_updated[type];
    struct sensor_data* ret = NULL;
    int *data_ptr = NULL;
    int data_cnt, rc;

    mutex_lock(&data->update_lock);
    if (time_after(jiffies, (*last_updated) + (poll_interval*HZ))
            || !(*valid))
    {
        rc = get_type_data(&data->sdata, type, 0, &data_ptr, &data_cnt);
        if (rc < 0)
            goto exit_err;

        DEBUG_INTR("%s-%d, type:%d cnt:%d\n", __func__, __LINE__, type, data_cnt);
        rc = comm2BMC(type, data_ptr, data_cnt);
        if (rc < 0) {
            /*Clear data if failed.*/
            if (data_ptr != NULL)
                memset(data_ptr, 0, sizeof(*data_ptr)*data_cnt);
            /*Make it valid if no such device, to avoid frequenctly access.*/
            if (rc == -ENODEV) {
                *valid = 1;
            } else {
                *valid = 0;
            }
        } else {
            *valid = 1;
        }
        *last_updated = jiffies;
    }
    ret =  &data->sdata;
exit_err:
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t show_name(struct device *dev, struct device_attribute *da,
                         char *buf)
{
    return sprintf(buf, "%s\n", DRVNAME);
}

static ssize_t _attr_show(struct device *dev, struct device_attribute *da,
                          char *buf)
{
    int index, count, rc;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct sensor_data* data;
    int *out = NULL;
    int i;
    char model_name[MAX_MODEL_NAME];
    char serial_number[MAX_SERIAL_NUMBER];
    enum sensor_type_e type;

    type = attr->index / ATTR_TYPE_INDEX_GAP;

    DEBUG_INTR("%s-%d, type:%d index:%d\n", __func__, __LINE__, type, attr->index);
    data = update_data(dev, type);
    if (data == NULL)
        return -EINVAL;

    index = attr->index % ATTR_TYPE_INDEX_GAP;
    rc = get_type_data(data, type, index, &out, &count);
    if (rc < 0 || out == NULL)
        return -EINVAL;

    if( index > count)
        return -EINVAL;
        
    if(type >=SENSOR_TYPE_PSU1_MODEL && type <=SENSOR_TYPE_PSU4_MODEL)
    {
        for(i=0; i<MAX_MODEL_NAME; i++)
        {
            model_name[i]=out[i];
        }
        if(strstr(model_name, "Error")!=NULL)
        {
            model_name[0]='\0';
        }
   
        return sprintf(buf, "%s\n",  model_name);
    }
    if(type >=SENSOR_TYPE_PSU1_SERIAL && type <=SENSOR_TYPE_PSU4_SERIAL)
    {
        
        for(i=0; i<MAX_SERIAL_NUMBER; i++)
        {
            serial_number[i]=out[i];
        }        
        if(strstr(serial_number, "Error")!=NULL)
        {
            serial_number[0]='\0';
        }
        return sprintf(buf, "%s\n",  serial_number);

    }


    return sprintf(buf, "%d\n",  *out);
}

static ssize_t show_fan_min(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    return sprintf(buf, "%d\n",  MIN_FAN_RPM);
}

static ssize_t show_psu_vout_max(struct device *dev, struct device_attribute *da,
                                 char *buf)
{
    return sprintf(buf, "%d\n",  MAX_PSU_VOUT);
}

static ssize_t show_psu_vout_min(struct device *dev, struct device_attribute *da,
                                 char *buf)
{
    return sprintf(buf, "%d\n",  MIN_PSU_VOUT);
}

static int add_attr2group(struct minipack_data *data, struct attribute *attr)
{
    int new_max_attrs = ++data->num_attributes + ATTR_ALLOC_EXTRA;
    void *new_attrs = krealloc(data->group.attrs,
                               new_max_attrs * sizeof(void *),
                               GFP_KERNEL);
    DEBUG_INTR("%s-%d, num_attr:%d\n", __func__, __LINE__, new_max_attrs);
    if (!new_attrs)
        return -ENOMEM;
    data->group.attrs = new_attrs;


    data->group.attrs[data->num_attributes-1] = attr;
    data->group.attrs[data->num_attributes] = NULL;

    return 0;
}

static int minipack_probe(struct platform_device *pdev)
{
    int status = -1;

    mp_data->dev = &pdev->dev;
    status = attributs_init(mp_data);
    DEBUG_INTR("%s-%d, status:%d\n", __func__, __LINE__, status);
    if (status) {
        goto exit;
    }

    /* Register sysfs hooks */
    status = sysfs_create_group(&pdev->dev.kobj, &mp_data->group);
    DEBUG_INTR("%s-%d, status:%d\n", __func__, __LINE__, status);

    if (status) {
        goto exit_kfree;
    }

    mp_data->hwmon_dev = hwmon_device_register(&pdev->dev);
    if (IS_ERR(mp_data->hwmon_dev)) {
        status = PTR_ERR(mp_data->hwmon_dev);
        goto exit_remove;
    }
    dev_info(&pdev->dev, "minipack sensors found\n");
    return 0;

exit_remove:
    sysfs_remove_group(&pdev->dev.kobj, &mp_data->group);
exit_kfree:
    kfree(mp_data->group.attrs);
exit:
    return status;
}

static int minipack_remove(struct platform_device *pdev)
{
    hwmon_device_unregister(mp_data->hwmon_dev);
    sysfs_remove_group(&pdev->dev.kobj, &mp_data->group);
    kfree(mp_data->group.attrs);
    return 0;
}

static struct platform_driver minipack_driver = {
    .probe		= minipack_probe,
    .remove		= minipack_remove,
    .driver		= {
        .name	= DRVNAME,
        .owner	= THIS_MODULE,
    },
};


static int __init minipack_init(void)
{
    int ret;

    ret = platform_driver_register(&minipack_driver);
    if (ret < 0) {
        goto exit;
    }
    mp_data = kzalloc(sizeof(struct minipack_data), GFP_KERNEL);
    if (!mp_data) {
        ret = -ENOMEM;
        platform_driver_unregister(&minipack_driver);
        goto exit;
    }
    mutex_init(&mp_data->update_lock);
    mp_data_init(mp_data);

    mp_data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(mp_data->pdev)) {
        ret = PTR_ERR(mp_data->pdev);
        platform_driver_unregister(&minipack_driver);
        kfree(mp_data);
        goto exit;
    }
exit:
    return ret;
}

static void __exit minipack_exit(void)
{
    if (!mp_data) {
        return;
    }
    platform_device_unregister(mp_data->pdev);
    platform_driver_unregister(&minipack_driver);
    kfree(mp_data);
}

module_init(minipack_init);
module_exit(minipack_exit);

MODULE_AUTHOR("Roy Lee <roy_lee@accton.com>");
MODULE_DESCRIPTION("Minipack platform sensors driver");
MODULE_LICENSE("GPL");
