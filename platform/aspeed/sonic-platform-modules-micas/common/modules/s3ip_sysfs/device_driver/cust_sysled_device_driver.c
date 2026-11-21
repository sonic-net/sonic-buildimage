#include <linux/miscdevice.h>
#include <asm/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "./include/device_driver_common.h"
#include "../sysfs_driver/include/sysled_sysfs.h"
#include "../switch_driver/include/dfd_sysfs_common.h"

#define CUST_SYSLED_INFO(fmt, args...) LOG_INFO("cust_sysled: ", fmt, ##args)
#define CUST_SYSLED_ERR(fmt, args...)  LOG_ERR("cust_sysled: ", fmt, ##args)
#define CUST_SYSLED_DBG(fmt, args...)  LOG_DBG("cust_sysled: ", fmt, ##args)

#define LED_SIZE 32

#define SYS_LED_FILE "/tmp/s3ip_sys_led"

static struct switch_drivers_s *g_drv = NULL;

static int g_loglevel = 0;
/* ioctl COMMAND */
#define SYS_LED_TYPE 'S'
#define SYS_LED_RED             _IO(SYS_LED_TYPE, 0)
#define SYS_LED_GREEN           _IO(SYS_LED_TYPE, 1)
#define SYS_LED_YELLOW          _IO(SYS_LED_TYPE, 2)

/* the colors are classified as the following kinds. */
typedef enum {
    CLS_SYS_LED_STATUS_DARK = 0,
    CLS_SYS_LED_STATUS_GREEN = 8,
    CLS_SYS_LED_STATUS_RED = 2,
    CLS_SYS_LED_STATUS_YELLOW = 4,
    CLS_SYS_LED_STATUS_GREEN_4HZ_FLASH = 1,
    CLS_SYS_LED_STATUS_OTHER= 0xFF,
} cls_led_staus_t;

static unsigned char s3ip_sys_led_status = CLS_SYS_LED_STATUS_GREEN_4HZ_FLASH; /* class of s3ip color */
static unsigned char wb_sys_led_status = CLS_SYS_LED_STATUS_GREEN;  /* class of hal ctrl */
static unsigned char s3ip_sys_led_value = S3IP_LED_STATUS_GREEN; /* s3ip value */

static bool check_led_status(int sys_bmc_led, unsigned char status)
{
    if ((s3ip_sys_led_status == status)
        || (wb_sys_led_status == status)
        || (sys_bmc_led == status)) {
        return true;
    }

    return false;
}

static void file_write(char* fpath, char* data)
{
    struct file *filp;

    filp = filp_open(fpath, O_RDWR|O_CREAT, 0644);
    if (IS_ERR(filp)) {
        CUST_SYSLED_DBG("open file error...\n");
        return;
    }
    kernel_write(filp, data, strlen(data), &filp->f_pos);
    filp_close(filp,NULL);
}

static void save_s3ip_sys_led(unsigned char status)
{
    char buf[LED_SIZE];

    memset(buf, 0, LED_SIZE);
    snprintf(buf, LED_SIZE, "0x%x\n", status);
    file_write(SYS_LED_FILE, buf);
}

static int file_read(char *fpath, int32_t addr, uint8_t *val, int32_t read_bytes)
{
    int32_t ret;
    struct file *filp;
    loff_t pos;

    if ((fpath == NULL) || (val == NULL) || (addr < 0) || (read_bytes < 0)) {
        CUST_SYSLED_DBG("input arguments error, addr=%d read_bytes=%d \n", addr, read_bytes);
        return -1;
    }

    filp = filp_open(fpath, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        CUST_SYSLED_DBG("open file[%s] fail \n", fpath);
        return -1;
    }
    pos = addr;
    ret = kernel_read(filp, val, read_bytes, &pos);
    if (ret < 0) {
        CUST_SYSLED_DBG("read file[%s] fail \n", fpath);
        ret = -1;
    }
    filp_close(filp, NULL);
    return ret;
}

static int read_s3ip_sys_led(unsigned char* sys)
{
    int tmp_value;
    char buf[LED_SIZE];
    int ret;

    memset(buf, 0, LED_SIZE);
    ret = file_read(SYS_LED_FILE, 0, buf, LED_SIZE);
    if (ret < 0) {
        CUST_SYSLED_DBG("file_read fail \n");
        return -1;
    }

    tmp_value = simple_strtoul(buf, NULL, 16);
    *sys = tmp_value & 0xFF;
    return 0;
}

static int get_sys_led_status_form_value(unsigned char value, unsigned char* status)
{
    unsigned char tmp_status;

    /*
     *  Classify the value
     */
    tmp_status = CLS_SYS_LED_STATUS_DARK;
    switch(value) {
    case S3IP_LED_STATUS_DARK:
        tmp_status = CLS_SYS_LED_STATUS_OTHER;
        break;
    case S3IP_LED_STATUS_GREEN:
        tmp_status = CLS_SYS_LED_STATUS_GREEN;
        break;
    case S3IP_LED_STATUS_RED:
        tmp_status = CLS_SYS_LED_STATUS_RED;
        break;
    case S3IP_LED_STATUS_YELLOW:
        tmp_status = CLS_SYS_LED_STATUS_YELLOW;
        break;
    case S3IP_LED_STATUS_GREEN_FLASH:
        tmp_status = CLS_SYS_LED_STATUS_GREEN_4HZ_FLASH;
        break;
    case S3IP_LED_STATUS_YELLOW_FLASH:
        tmp_status = CLS_SYS_LED_STATUS_OTHER;
        break;
    case S3IP_LED_STATUS_RED_FLASH:
        tmp_status = CLS_SYS_LED_STATUS_OTHER;
        break;
    default:
        CUST_SYSLED_INFO("Invalid value 0x%x.\n", value);
        return -1;
    }

    *status = tmp_status;
    return 0;
}


static int change_colorFrBmcToS3ip(unsigned char value, unsigned char* status)
{
    unsigned char tmp_status;

    /*
     *  Classify the value
     *  In th5, the color from bmc is s3ip color.
     *  1: green, 3: red
     *  In td4/th4 and so on, the color from bmc:
     *  8: green, 2: red
     */
    tmp_status = CLS_SYS_LED_STATUS_DARK;
    switch(value) {
    case S3IP_LED_STATUS_GREEN:
        tmp_status = CLS_SYS_LED_STATUS_GREEN;
        break;
    case S3IP_LED_STATUS_RED:
        tmp_status = CLS_SYS_LED_STATUS_RED;
        break;
    case CLS_SYS_LED_STATUS_RED:
        tmp_status = CLS_SYS_LED_STATUS_RED;
        break;
    case CLS_SYS_LED_STATUS_GREEN:
        tmp_status = CLS_SYS_LED_STATUS_GREEN;
        break;
    default:
        CUST_SYSLED_INFO("Invalid value 0x%x.\n", value);
        return -1;
    }

    *status = tmp_status;
    return 0;
}


static int set_sys_led(void)
{
    int rv = 0;
    char buf[LED_SIZE];
    int tmp_sys_bmc_led;
    unsigned char sys_bmc_led;
    int led;

    check_p(g_drv);
    check_p(g_drv->get_sys_led_by_bmc);
    check_p(g_drv->set_sys_led_status);

    rv = g_drv->get_sys_led_by_bmc(buf, LED_SIZE - 1);
    if (rv < 0) {
        CUST_SYSLED_ERR("read get_sys_led_by_bmc failed, rv = %d \n", rv);
        sys_bmc_led = CLS_SYS_LED_STATUS_GREEN;
    } else {
        tmp_sys_bmc_led = (int)simple_strtol(buf, NULL, 10);
        rv = change_colorFrBmcToS3ip(tmp_sys_bmc_led, &sys_bmc_led);
        if (rv < 0) {
            CUST_SYSLED_DBG("change_colorFrBmcToS3ip failed, rv = %d \n", rv);
            sys_bmc_led = CLS_SYS_LED_STATUS_GREEN;
        }
    }

    CUST_SYSLED_DBG("s3ip_sys_led_status = 0x%x\n", s3ip_sys_led_status);
    CUST_SYSLED_DBG("s3ip_sys_led_value = 0x%x\n", s3ip_sys_led_value);
    CUST_SYSLED_DBG("wb_sys_led_status = 0x%x\n", wb_sys_led_status);
    CUST_SYSLED_DBG("sys_bmc_led = 0x%x\n", sys_bmc_led);

    /* here, compare color from BMC, color from s3ip, color from hal-fan */
    led = S3IP_LED_STATUS_GREEN;
    if (check_led_status(sys_bmc_led, CLS_SYS_LED_STATUS_GREEN_4HZ_FLASH)) {
        led = S3IP_LED_STATUS_GREEN_FLASH;
    } else if (check_led_status(sys_bmc_led, CLS_SYS_LED_STATUS_RED)) {
        led = S3IP_LED_STATUS_RED;
    } else if (check_led_status(sys_bmc_led, CLS_SYS_LED_STATUS_YELLOW)) {
        led = S3IP_LED_STATUS_YELLOW;
    } else if (check_led_status(sys_bmc_led, CLS_SYS_LED_STATUS_OTHER)) {
        led = s3ip_sys_led_value;
    } else {
        led = S3IP_LED_STATUS_GREEN;
    }

    CUST_SYSLED_DBG("set_sys_led_status led = 0x%x\n", led);
    rv = g_drv->set_sys_led_status(led);
    if (rv < 0) {
       CUST_SYSLED_ERR("set_sys_led_status failed, rv = %d \n", rv);
        return rv;
    }

    return rv;
}

static void init_s3ip_sys_led(void)
{
    int rv;
    unsigned char s3ip_value;
    unsigned char tmp_status;

    rv = read_s3ip_sys_led(&s3ip_value);
    if (rv < 0) {
        CUST_SYSLED_DBG("read_s3ip_sys_led fail \n");
        return;
    }

    s3ip_sys_led_value = s3ip_value;
    tmp_status = CLS_SYS_LED_STATUS_DARK;
    rv = get_sys_led_status_form_value(s3ip_value, &tmp_status);
    if (rv < 0) {
        CUST_SYSLED_DBG("get_sys_led_status_form_value failed, rv = %d \n", rv);
        return;
    }

    s3ip_sys_led_status = tmp_status;
    CUST_SYSLED_DBG("init_s3ip_sys_led ok, s3ip_sys_led_status = %d \n", s3ip_sys_led_status);
}

/*
 * s3ip sysfs set function call cust_set_sys_led
 */
int cust_set_sys_led(unsigned char sys)
{
    int rv;
    unsigned char tmp_status;
    /*
     * use cpld control, set the cpld reg.
     * */
    tmp_status = CLS_SYS_LED_STATUS_DARK;
    rv = get_sys_led_status_form_value(sys, &tmp_status);
    if (rv < 0) {
        CUST_SYSLED_DBG("get_sys_led_status_form_value failed, rv = %d \n", rv);
        CUST_SYSLED_ERR("Invalid command, can not set sys 0x%x.\n", sys);
        return rv;
    }

    s3ip_sys_led_status = tmp_status;
    s3ip_sys_led_value = sys;
    CUST_SYSLED_DBG("set sys 0x%x.\n", sys);
    rv = set_sys_led();
    if (rv < 0) {
        CUST_SYSLED_ERR("set_sys_led failed, rv = %d \n", rv);
        return rv;
    }

    /* save s3ip color to file */
    save_s3ip_sys_led(s3ip_sys_led_value);

    return 0;
}

EXPORT_SYMBOL(cust_set_sys_led);

static int sys_led_open(struct inode *inode, struct file *file)
{
    return 0;
}

static long sys_led_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int rv;

    /* Verify user arguments. */
    if (_IOC_TYPE(cmd) != SYS_LED_TYPE)
        return -ENOTTY;

    switch (cmd) {
    case SYS_LED_RED:
        wb_sys_led_status = CLS_SYS_LED_STATUS_RED;
        break;
    case SYS_LED_GREEN:
        wb_sys_led_status = CLS_SYS_LED_STATUS_GREEN;
        break;
    case SYS_LED_YELLOW:
        wb_sys_led_status = CLS_SYS_LED_STATUS_YELLOW;
        break;
    default:
        wb_sys_led_status = CLS_SYS_LED_STATUS_RED;
        CUST_SYSLED_INFO("Invalid ioctl command.\n");
        return -ENOTTY;
    }

    CUST_SYSLED_DBG("wb_sys_led_status = 0x%x\n", wb_sys_led_status);
    rv = set_sys_led();
    if (rv < 0) {
        CUST_SYSLED_INFO("sys_led_ioctl failed, rv = %d \n", rv);
        return rv;
    }

    return 0;
}

static int sys_led_release(struct inode *inode, struct file *file)
{
    return 0;
}

static const struct file_operations sys_led_dev_fops = {
    .owner      = THIS_MODULE,
    .unlocked_ioctl = sys_led_ioctl,
    .open       = sys_led_open,
    .release    = sys_led_release,
};


static struct miscdevice sys_led_dev = {
    .minor = SYS_LED_TYPE,
    .name = "sys_led",
    .fops = &sys_led_dev_fops,
};

static int cust_sysled_init(void)
{
    int ret = 0;

    CUST_SYSLED_INFO("...\n");
    g_drv = s3ip_switch_driver_get();
    check_p(g_drv);
    init_s3ip_sys_led();

    ret = misc_register(&sys_led_dev);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

static void cust_sysled_exit(void){
    misc_deregister(&sys_led_dev);
    CUST_SYSLED_INFO(".\n");
    return;
}

module_init(cust_sysled_init);
module_exit(cust_sysled_exit);
module_param(g_loglevel, int, 0644);
MODULE_PARM_DESC(g_loglevel, "the log level(info=0x1, err=0x2, dbg=0x4, all=0xf).\n");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("cust sysled driver");
