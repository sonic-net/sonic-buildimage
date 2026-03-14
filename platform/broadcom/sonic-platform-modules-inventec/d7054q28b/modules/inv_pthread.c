/*****************************
 Cypress platform
******************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/ctype.h>
#include <linux/uaccess.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>

#define INV_PTHREAD_KERNEL_MODULE

#define SHOW_ATTR_WARNING       ("N/A")
#define SHOW_ATTR_NOTPRINT      ("Not Available")
#define SHOW_ATTR_NOTSUPPORT    ("Not Support")

#define INV_HWMID_MAX           (10)
#define INV_HWMID_INIT		(-1)

/*access userspace data to kernel space*/
#define ACC_R           (0)
#define ACC_W           (1)

#define TINY_BUF_SIZE   (8)
#define MAX_PATH_SIZE   (64)
#define MIN_ACC_SIZE    (32)
#define MAX_ACC_SIZE    (256)

/*
 * LED definitions
 */
#define STATUS_LED_MODE_AUTO    0
#define STATUS_LED_MODE_DIAG    1
#define STATUS_LED_MODE_MANU    2

#define STATUS_LED_GRN0         10      // 0 - 000: off
#define STATUS_LED_GRN1         11      // 1 - 001: 0.5hz
#define STATUS_LED_GRN2         12      // 2 - 010: 1 hz
#define STATUS_LED_GRN3         13      // 3 - 011: 2 hz
#define STATUS_LED_GRN7         17      // 7 - 111: on
#define STATUS_LED_RED0         20      // 0 - 000: off
#define STATUS_LED_RED1         21      // 1 - 001: 0.5hz
#define STATUS_LED_RED2         22      // 2 - 010: 1 hz
#define STATUS_LED_RED3         23      // 3 - 011: 2 hz
#define STATUS_LED_RED7         27      // 7 - 111: on
#define STATUS_LED_INVALID      0       // Invalid

ssize_t status_led_change(const char *path1, const char *tmp1, const char *path2, const char *tmp2);
ssize_t status_led_grn(const char *freq);
ssize_t status_led_red(const char *freq);
ssize_t status_led_diag_mode_enable(void);
ssize_t status_led_diag_mode_disable(void);
int status_led_check_color(void);
int status_led_check_diag_mode(void);

#if 1
/* For timestamps in SYSFS_LOG */
#define SYSFS_LOG	printk
#else
//#define SYSFS_LOG(fmt, args...) printk(KERN_WARNING "[SYSFS] %s/%d: " fmt, __func__, __LINE__, ##args)
#define SYSFS_LOG(fmt, args...) printk(KERN_WARNING "[p_thread] " fmt, ##args)
#endif


/* inventec_class *********************************/
static struct kobject *status_kobj;
static struct kset *status_kset;


static struct mutex rw_lock;

static int hwm_psoc = INV_HWMID_INIT;
static int hwm_cpld = INV_HWMID_INIT;

int get_hwm_psoc(void)
{
    return hwm_psoc;
}

int get_hwm_cpld(void)
{
    return hwm_cpld;
}

static ssize_t access_user_space(const char *name, int mode, char *buf, size_t len, loff_t offset)
{
	struct file *fp;
	mm_segment_t fs;
	loff_t pos = offset;
	char *mark = NULL;
	ssize_t vfs_ret = 0;

	if (mode == ACC_R) {
		fp = filp_open(name, O_RDONLY, S_IRUGO);
		if (IS_ERR(fp))
			return -ENODEV;

		fs = get_fs();
		set_fs(KERNEL_DS);

		vfs_ret = vfs_read(fp, buf, len, &pos);

		mark = strpbrk(buf, "\n");
		if (mark)
			*mark = '\0';

		filp_close(fp, NULL);
		set_fs(fs);
	} else if (mode == ACC_W) {
		fp = filp_open(name, O_WRONLY, S_IWUSR | S_IRUGO);
		if (IS_ERR(fp))
			return -ENODEV;

		fs = get_fs();
		set_fs(KERNEL_DS);

		vfs_ret = vfs_write(fp, buf, len, &pos);
		filp_close(fp, NULL);
		set_fs(fs);
	}

	return vfs_ret;
}

int inventec_strtol(const char *sbufp, char **endp, unsigned int base)
{
    char *endptr;
    int value = simple_strtol(sbufp, &endptr, base);
    if (value == 0 && sbufp == endptr) {
	*endp = NULL;
	return value;
    }
    *endp = (char*)1;
    return value;
}

int inventec_singlechar_to_int(const char c)
{
    if ((c >= '0') && (c <= '9')) {
	return (c - '0');
    }
    else
    if ((c >= 'a') && (c <= 'f')) {
	return (c - 'a' + 10);
    }
    else
    if ((c >= 'A') && (c <= 'F')) {
	return (c - 'A' + 10);
    }
    return -1;
}

int inventec_store_input(char *inputp, int count)
{
        int i = 0;
        while(inputp[i] != '\n' && inputp[i] != '\0' && i < count) {
                i++;
        }
        inputp[i] = '\0';
        return strlen(inputp);
}

#if 0
/*
 * Time stamps for kernel log on yocto
 */
#include <linux/rtc.h>

void SYSFS_LOG(char *fmt, ...)
{
        char buf[80], ts[32];
	va_list args;
	int hlen;

	inventec_tmstmp(&ts[0]);
	hlen = sprintf(buf, "[SYSFS] %s ", ts);	// Do not edit this line

        va_start(args, fmt);
        vsprintf(&buf[hlen-1], fmt, args);
        va_end(args);
        printk(KERN_WARNING "[p_thread] %s\n", buf);
}
#endif

ssize_t
inventec_show_attr(char *buf_p, const char *invdevp)
{
    int  inv_len = MAX_ACC_SIZE;	/* INV driver return max length      */
    char tmp_buf[MAX_ACC_SIZE];
    char *str_negative = "-", *mark = NULL;

    /* [Step2] Get data by uaccess */
    memset(tmp_buf, 0, sizeof(tmp_buf));
    mutex_lock(&rw_lock);
    if (access_user_space(invdevp, ACC_R, tmp_buf, inv_len, 0) < 0) {
        /* u_access fail */
        mutex_unlock(&rw_lock);
        return sprintf(buf_p, "%s\n", SHOW_ATTR_WARNING);
    }
    mutex_unlock(&rw_lock);

    /* [Step3] Check return value
     * - Ex: When transceiver not plugged
     *   => SWPS return error code "-202"
     *   => Pic8 need return "NA" (assume)
     */
    if (strcspn(tmp_buf, str_negative) == 0) {
        /* error case: <ex> "-202" */
        return sprintf(buf_p, "%s\n", SHOW_ATTR_WARNING);
    }

    /* OK case:*/
    mark = strpbrk(tmp_buf, "\n");
    if (mark) { *mark = '\0'; }

    return sprintf(buf_p, "%s\n", tmp_buf);
}

ssize_t
inventec_store_attr(const char *buf_p, size_t count, const char *invdevp)
{
    ssize_t ret = 0;

    /* [Step2] Get data by uaccess */
    mutex_lock(&rw_lock);
    if ((ret = access_user_space(invdevp, ACC_W, (char*)buf_p, count, 0)) < 0) {
        /* u_access fail */
        mutex_unlock(&rw_lock);
        return -EINVAL;
    }
    mutex_unlock(&rw_lock);

    /* OK case:*/
    return ret;
}

int sysfs_detect_hwmon_index(void)
{
    char hwmon_buf[MAX_ACC_SIZE];
    char hwmon_path[MAX_PATH_SIZE];
    int hwid = 0;

    for (hwid = 0;
	 hwid < INV_HWMID_MAX && (hwm_psoc == INV_HWMID_INIT || hwm_cpld == INV_HWMID_INIT);
	 hwid++) {
	memset(hwmon_buf, 0, sizeof(hwmon_buf));
	sprintf(hwmon_path, "/sys/class/hwmon/hwmon%d/device/name", hwid);

	inventec_show_attr(hwmon_buf, hwmon_path);
	if (strncmp(hwmon_buf, "inv_psoc", 8) == 0) {
	    hwm_psoc = hwid;
	}
	else
	if (strncmp(hwmon_buf, "inv_bmc", 7) == 0) {
	    hwm_psoc = hwid;
	}
	else
	if (strncmp(hwmon_buf, "inv_cpld", 8) == 0) {
	    hwm_cpld = hwid;
	}
    }
    if (hwid >= INV_HWMID_MAX) {
	printk(KERN_ERR "[p_thread] detect hwmon index failed, psoc = %d, cpld = %d\n", hwm_psoc, hwm_cpld);
	return -1;
    }
    printk(KERN_INFO "[p_thread] detect hwmon index success, psoc = %d, cpld = %d\n", hwm_psoc, hwm_cpld);
    return 0;
}

static int __init
inventec_class_init(void)
{
    mutex_init(&rw_lock);

#ifdef	INV_PTHREAD_KERNEL_MODULE
    if (sysfs_detect_hwmon_index() < 0) {
        return -1;
    }
#endif

    printk(KERN_INFO "[p_thread] [%s/%d] Module initial success.\n",__func__,__LINE__);

    return 0;
}

static void __exit
inventec_class_exit(void)
{
    printk(KERN_INFO "[p_thread] [%s/%d] Remove module.\n",__func__,__LINE__);
}

/* fan device *************************************/
#define FAN_DEV_PATH_STATE	"/sys/class/hwmon/hwmon%d/device/fan_gpi"
#define FAN_DEV_PATH_FAN1_INPUT "/sys/class/hwmon/hwmon%d/device/fan1_input"
#define FAN_DEV_PATH_FAN2_INPUT "/sys/class/hwmon/hwmon%d/device/fan2_input"
#define FAN_DEV_PATH_FAN3_INPUT "/sys/class/hwmon/hwmon%d/device/fan3_input"
#define FAN_DEV_PATH_FAN4_INPUT "/sys/class/hwmon/hwmon%d/device/fan4_input"
#define FAN_DEV_PATH_FAN5_INPUT "/sys/class/hwmon/hwmon%d/device/fan5_input"
#define FAN_DEV_PATH_FAN6_INPUT "/sys/class/hwmon/hwmon%d/device/fan6_input"
#define FAN_DEV_PATH_FAN7_INPUT "/sys/class/hwmon/hwmon%d/device/fan7_input"
#define FAN_DEV_PATH_FAN8_INPUT "/sys/class/hwmon/hwmon%d/device/fan8_input"

static char fan_dev_path_state[MAX_PATH_SIZE];
static char fan_dev_path_fan1_input[MAX_PATH_SIZE];
static char fan_dev_path_fan2_input[MAX_PATH_SIZE];
static char fan_dev_path_fan3_input[MAX_PATH_SIZE];
static char fan_dev_path_fan4_input[MAX_PATH_SIZE];
static char fan_dev_path_fan5_input[MAX_PATH_SIZE];
static char fan_dev_path_fan6_input[MAX_PATH_SIZE];
static char fan_dev_path_fan7_input[MAX_PATH_SIZE];
static char fan_dev_path_fan8_input[MAX_PATH_SIZE];

void sysfs_fan_path_init(void)
{
    sprintf(&fan_dev_path_state[0],	FAN_DEV_PATH_STATE,	 get_hwm_psoc());
    sprintf(&fan_dev_path_fan1_input[0],FAN_DEV_PATH_FAN1_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan2_input[0],FAN_DEV_PATH_FAN2_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan3_input[0],FAN_DEV_PATH_FAN3_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan4_input[0],FAN_DEV_PATH_FAN4_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan5_input[0],FAN_DEV_PATH_FAN5_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan6_input[0],FAN_DEV_PATH_FAN6_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan7_input[0],FAN_DEV_PATH_FAN7_INPUT, get_hwm_psoc());
    sprintf(&fan_dev_path_fan8_input[0],FAN_DEV_PATH_FAN8_INPUT, get_hwm_psoc());
}

#define FAN_STATE_NORMAL	"normal"
#define FAN_STATE_FAULTY	"faulty"
#define FAN_STATE_UNINSTALLED	"uninstalled"
#define FAN_STATE_UNKNOW	"unknown state"
#define FAN_STATE_INVALID	"Invalid state value"
#define FAN_STATE_READ_ERROR	"state read error"

#define FAN_LOG_UNINSTALLED	"removed"
#define FAN_LOG_NORMAL		"inserted"

//#define FAN_STATE_BIT_NORMAL		0
#define FAN_STATE_BIT_FAULTY		0
#define FAN_STATE_BIT_UNINSTALLED	1
#define FAN_STATE_BIT_UNKNOW		2
#define FAN_STATE_BIT_INVALID		3
#define FAN_STATE_BIT_READ_ERROR	4

static struct fans_tbl_s {
        char *fan_name;
        char *fan_front;
        char *fan_rear;
        unsigned int fan_state;
} fans_tbl[] = {
        {"fan1",	fan_dev_path_fan1_input,
			fan_dev_path_fan2_input,	0},
        {"fan2",	fan_dev_path_fan3_input,
			fan_dev_path_fan4_input,	0},
        {"fan3",	fan_dev_path_fan5_input,
			fan_dev_path_fan6_input,	0},
        {"fan4",	fan_dev_path_fan7_input,
			fan_dev_path_fan8_input,	0},
};
#define FAN_TBL_TOTAL	( sizeof(fans_tbl)/ sizeof(const struct fans_tbl_s) )

#define FAN_STATE_CHECK(i,b)	(fans_tbl[i].fan_state & (1<<b))
#define FAN_STATE_SET(i,b)	(fans_tbl[i].fan_state |= (1<<b))
#define FAN_STATE_CLEAR(i,b)	(fans_tbl[i].fan_state &= ~(1<<b))
#define FAN_STATE_INIT(i)	(fans_tbl[i].fan_state = 0)

static int
fans_faulty_log(int fan_id)
{
    char buf[MAX_ACC_SIZE];
    int pwm;

    memset(&buf[0], 0, MAX_ACC_SIZE);
    if (inventec_show_attr(buf, fans_tbl[fan_id].fan_front) < 0) {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_READ_ERROR)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_READ_ERROR);
	    SYSFS_LOG("[p_thread] %s: front %s\n",fans_tbl[fan_id].fan_name,FAN_STATE_READ_ERROR);
	}
	return 1;
    }
    pwm = simple_strtol(buf, NULL, 10);
    if (pwm <= 0) {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_FAULTY)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_FAULTY);
	    //SYSFS_LOG("[p_thread] %s: %s\n",fans_tbl[fan_id].fan_name,FAN_STATE_FAULTY);
	}
	return 1;
    }

    memset(&buf[0], 0, MAX_ACC_SIZE);
    if (inventec_show_attr(buf, fans_tbl[fan_id].fan_rear) < 0) {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_READ_ERROR)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_READ_ERROR);
	    SYSFS_LOG("[p_thread] %s: rear %s\n",fans_tbl[fan_id].fan_name,FAN_STATE_READ_ERROR);
	}
	return 1;
    }
    pwm = simple_strtol(buf, NULL, 10);
    if (pwm <= 0) {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_FAULTY)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_FAULTY);
	    //SYSFS_LOG("[p_thread] %s: %s\n",fans_tbl[fan_id].fan_name,FAN_STATE_FAULTY);
	}
	return 1;
    }

    if(fans_tbl[fan_id].fan_state != 0) {
	fans_tbl[fan_id].fan_state = 0;
	SYSFS_LOG("[p_thread] %s: %s\n",fans_tbl[fan_id].fan_name,FAN_LOG_NORMAL);
    }
    return 0;
}

/* INV drivers mapping */
static int
fans_control_log(int fan_id)
{
    char buf[MAX_ACC_SIZE];
    unsigned int statebit2, statebit3, bitshift;

    if (inventec_show_attr(buf, fan_dev_path_state) < 0) {
	SYSFS_LOG("[p_thread] read fan_gpi failed\n");
	return 1;
    }

    if (buf[0] != '0' || (buf[1] != 'x' && buf[1] != 'X')) {
	SYSFS_LOG("[p_thread] %s/%d: %s %s\n",__func__,__LINE__,FAN_STATE_INVALID, buf);
	return 1;
    }

    if ((statebit2 = inventec_singlechar_to_int(buf[2])) == -1) {
	SYSFS_LOG("[p_thread] Error value read from %s\n", fan_dev_path_state);
	return 1;
    }
    if (buf[2] == 'f' || buf[2] == 'F') statebit2 = 0;

    if ((statebit3 = inventec_singlechar_to_int(buf[3])) == -1) {
	SYSFS_LOG("[p_thread] Error value read from %s\n", fan_dev_path_state);
	return 1;
    }

    bitshift = fan_id;
    //SYSFS_LOG("[p_thread] 1: statebit2 = 0x%x statebit3 = 0x%x bitshift = 0x%x\n",statebit2,statebit3,bitshift);
    if ((statebit2 & 1<<bitshift) && (statebit3 & 1<<bitshift)) {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_UNINSTALLED)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_UNINSTALLED);
	    SYSFS_LOG("[p_thread] %s: %s\n",fans_tbl[fan_id].fan_name,FAN_LOG_UNINSTALLED);
	}
	return 1;
    }
    else
    if (!(statebit2 & 1<<bitshift) && !(statebit3 & 1<<bitshift)) {
	return fans_faulty_log(fan_id);
    }
    else {
	if(!FAN_STATE_CHECK(fan_id, FAN_STATE_BIT_FAULTY)) {
	    FAN_STATE_SET(fan_id, FAN_STATE_BIT_FAULTY);
	    SYSFS_LOG("[p_thread] %s: %s\n",fans_tbl[fan_id].fan_name,FAN_STATE_FAULTY);
	}
	return 1;
    }
    return 0;
}

int fans_control(void)
{
    int i, ret = 0;
    static int cd_shutdown = 5;

    for (i = 0; i < FAN_TBL_TOTAL; i++) {
        if(fans_control_log(i) == 1) {
            ret++;
        }
    }

    if (0 < ret && ret < FAN_TBL_TOTAL) {
        status_led_red("2");  //1Hz
        cd_shutdown = 5; //reset count down
    }

    if (ret == FAN_TBL_TOTAL) {
        status_led_red("3"); //4Hz
        if (cd_shutdown == 0) {
            kobject_uevent(status_kobj, KOBJ_REMOVE);
        }
        else if (cd_shutdown > 0)
        {
            printk(KERN_ERR "[p_thread] All fans failed.\n");
            printk(KERN_ERR "[p_thread] System shutdown immediately in %d seconds.\n", cd_shutdown);
        }
        cd_shutdown -= 1;
    }
    return ret;
}

/* End of faninfo_device */

static int __init
fan_device_init(void)
{
#ifdef	INV_PTHREAD_KERNEL_MODULE
    sysfs_fan_path_init();
#endif
    return 0;
}


static void __exit
fan_device_exit(void)
{
    printk(KERN_INFO "[p_thread] Remove fan module.\n");
}

/* psu device *************************************/
static unsigned int psu_voltin = 0;
#define PSU_VOLTIN_ACDC	(70000)

/*
 * normal/unpower/uninstall/fault are PSU states output from driver level
 * checkpsu/error are defined by sysfs
 */
#define PSU_STATE_VAL_NORMAL	(0)
#define PSU_STATE_VAL_UNPOWER	(2)
#define PSU_STATE_VAL_FAULT	(4)
#define PSU_STATE_VAL_UNINSTALL	(7)
#define PSU_STATE_VAL_CHECKPSU	(8)
#define PSU_STATE_VAL_ERROR	(9)

#define PSU_STATE_NORMAL	("0 : normal")
#define PSU_STATE_UNPOWER	("2 : unpowered")
#define PSU_STATE_FAULT		("4 : fault")
#define PSU_STATE_UNINSTALL	("7 : not installed")
#define PSU_STATE_CHECKPSU	("8 : check psu")
#define PSU_STATE_ERROR		("9 : state error")

#define PSU_STATE_LEN_NORMAL	(strlen(PSU_STATE_NORMAL))
#define PSU_STATE_LEN_UNPOWER	(strlen(PSU_STATE_UNPOWER))
#define PSU_STATE_LEN_FAULT	(strlen(PSU_STATE_FAULT))
#define PSU_STATE_LEN_UNINSTALL	(strlen(PSU_STATE_UNINSTALL))
#define PSU_STATE_LEN_CHECKPSU	(strlen(PSU_STATE_CHECKPSU))

typedef struct {
	char *inv_dev_attrp;
	char *inv_dev_pathp;
} psu_dev_t;

typedef struct {
	const char		*psu_name;
	int			psu_major;
        dev_t			psu_devt;
	struct device		*psu_dev_p;
	psu_dev_t		*psu_dev_namep;
	int			psu_dev_total;
	char			*psu_inv_pathp;
	void			*psu_tracking;
	char			*psu_currentin;
	char			*psu_currentout;
	char			*psu_powerin;
	char			*psu_powerout;
	char			*psu_voltin;
	char			*psu_voltout;
} psu_dev_group_t;

#define PSU_DEV_PATH_TEMPLATE	"/sys/class/hwmon/hwmon%d/device/%s"

static char psu_dev_path_state[MAX_PATH_SIZE];
static char psu_dev_path_psu_voltin[MAX_PATH_SIZE];
#if 0
static char psu_dev_path_vendor[MAX_PATH_SIZE];
static char psu_dev_path_version[MAX_PATH_SIZE];
static char psu_dev_path_sn[MAX_PATH_SIZE];
static char psu_dev_path_temperature[MAX_PATH_SIZE];
static char psu_dev_path_fan_speed[MAX_PATH_SIZE];
static char psu_dev_path_fan_pwm[MAX_PATH_SIZE];
static char psu_dev_path_fan_faulty[MAX_PATH_SIZE];
static char psu_dev_path_psu_currentin[MAX_PATH_SIZE];
static char psu_dev_path_psu_currentout[MAX_PATH_SIZE];
static char psu_dev_path_psu_powerin[MAX_PATH_SIZE];
static char psu_dev_path_psu_powerout[MAX_PATH_SIZE];
static char psu_dev_path_psu_voltout[MAX_PATH_SIZE];
static char psu_dev_path_psu_pwm[MAX_PATH_SIZE];
static char psu_dev_path_psu_rpm[MAX_PATH_SIZE];
#endif

void sysfs_psu_path_init(void)
{
    sprintf(&psu_dev_path_state[0],		PSU_DEV_PATH_TEMPLATE,	get_hwm_cpld(), "\%s" );
    sprintf(&psu_dev_path_psu_voltin[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
#if 0
    sprintf(&psu_dev_path_vendor[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_version[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_sn[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_temperature[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_fan_speed[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_fan_pwm[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_fan_faulty[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_currentin[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_currentout[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_powerin[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_powerout[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_voltout[0], 	PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_pwm[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
    sprintf(&psu_dev_path_psu_rpm[0], 		PSU_DEV_PATH_TEMPLATE,	get_hwm_psoc(), "\%s" );
#endif
}

static psu_dev_t psu_dev_name[] = {
	{ "state",		psu_dev_path_state },	// Using cpld
	{ "psu_voltin",		psu_dev_path_psu_voltin },
#if 0
	{ "vendor",		psu_dev_path_vendor },
	{ "version",		psu_dev_path_version },
	{ "sn",			psu_dev_path_sn },
	{ "temperature",	psu_dev_path_temperature },
	{ "fan_speed",		psu_dev_path_fan_speed },
	{ "fan_pwm",		psu_dev_path_fan_pwm },
	{ "fan_faulty",		psu_dev_path_fan_faulty },
	{ "psu_currentin",	psu_dev_path_psu_currentin },
	{ "psu_currentout",	psu_dev_path_psu_currentout },
	{ "psu_powerin",	psu_dev_path_psu_powerin },
	{ "psu_powerout",	psu_dev_path_psu_powerout },
	{ "psu_voltout",	psu_dev_path_psu_voltout },
	{ "psu_pwm",		psu_dev_path_psu_pwm },
	{ "psu_rpm",		psu_dev_path_psu_rpm },
#endif
};
#define PSU_DEV_NAME_TOTAL	( sizeof(psu_dev_name) / sizeof(const psu_dev_t) )

static psu_dev_group_t psu_dev_group[] = {
	{
	  .psu_name = "psu1",
	  .psu_dev_namep = &psu_dev_name[0],
	  .psu_dev_total = sizeof(psu_dev_name) / sizeof(const psu_dev_t),
	},
	{
	  .psu_name = "psu2",
	  .psu_dev_namep = &psu_dev_name[0],
	  .psu_dev_total = sizeof(psu_dev_name) / sizeof(const psu_dev_t),
	},
};
#define PSU_DEV_GROUP_TOTAL	( sizeof(psu_dev_group)/ sizeof(const psu_dev_group_t) )

static char psu_state[4][MIN_ACC_SIZE];

static struct psu_wire_tbl_s {
        char *psu_attr;
        char *psu_name;
	char *psu_wire;
	char *psu_state;
} psu_wire_tbl[] = {
        { "state",		"psu1",	"psu0",			psu_state[0] },	// Using cpld
        { "state",		"psu2",	"psu1",			psu_state[1] },
	{ "psu_voltin",		"psu1",	"psoc_psu1_vin",	psu_state[2] },
	{ "psu_voltin",		"psu2",	"psoc_psu2_vin",	psu_state[3] },
};
#define PSU_WIRE_TBL_TOTAL   ( sizeof(psu_wire_tbl)/ sizeof(const struct psu_wire_tbl_s) )

static char *
psu_attr_get_wirep(const char *psu_attrp, const char *psu_namep, char **psu_statepp)
{
    int i;

    for (i = 0; i < PSU_WIRE_TBL_TOTAL; i++) {
	if (strncmp(psu_wire_tbl[i].psu_attr, psu_attrp, strlen(psu_attrp)) == 0 &&
	    strncmp(psu_wire_tbl[i].psu_name, psu_namep, strlen(psu_namep)) == 0) {
	    if (psu_statepp) {
		*psu_statepp = psu_wire_tbl[i].psu_state;
	    }
	    return psu_wire_tbl[i].psu_wire;
	}
    }
    return NULL;
}

int psu_check_state_normal(char *statep)
{
    if (strstr(statep, "normal")) {
	return 1;
    }
    return 0;
}

#define PSU_ATTR_VOLTIN		("psu_voltin")
#define PSU_ATTR_VOLTIN_LEN	(10)

/* Get PSU voltin for determon AC(110v) or DC(48v) */
void psu_get_voltin(void)
{
    char acc_path[MAX_PATH_SIZE], volt[MIN_ACC_SIZE];
    psu_dev_t *devnamep;
    unsigned int voltin;
    char *invwirep;
    int i, j;

    for (i = 0; i < PSU_DEV_GROUP_TOTAL; i++) {
	//psu_dev_group[i].psu_name;
	devnamep = psu_dev_group[i].psu_dev_namep;
	for (j = 0; j < psu_dev_group[i].psu_dev_total; j++, devnamep++) {
	    if (strncmp(devnamep->inv_dev_attrp, PSU_ATTR_VOLTIN, PSU_ATTR_VOLTIN_LEN) == 0) {
		invwirep = psu_attr_get_wirep(PSU_ATTR_VOLTIN, psu_dev_group[i].psu_name, NULL);
		if (invwirep == NULL) {
		    printk(KERN_DEBUG "[p_thread] Invalid psuname: %s\n", psu_dev_group[i].psu_name);
		    continue;
		}
		sprintf(acc_path, devnamep->inv_dev_pathp, invwirep);
		//printk(KERN_DEBUG "[p_thread] RYU: %s/%d: acc_path = %s\n",__func__,__LINE__,acc_path);
		if (inventec_show_attr(volt, acc_path) <= 0) {
		    printk(KERN_DEBUG "[p_thread] Read %s failed\n", acc_path);
		    continue;
		}
		else {
		    voltin = simple_strtol(&volt[0], NULL, 10);
		    printk(KERN_DEBUG "[p_thread] Read %s %s = %u\n",acc_path,volt,voltin);
		    if (voltin > psu_voltin) {
			psu_voltin = voltin;
		    }
		}
	    }
	}
    }

    SYSFS_LOG("[p_thread] PSU voltin = %u\n", psu_voltin);
}

#define PSU_ATTR_STATE		("state")
#define PSU_ATTR_STATE_LEN	(5)

/* psus_control() by inv_thread */
int psus_control(int log_only)
{
    char acc_path[MAX_PATH_SIZE], state[MIN_ACC_SIZE];
    psu_dev_t *devnamep = NULL;
    char *invwirep = NULL;
    char *psu_statep = NULL;
    int i, j, flag = 0;

    for (i = 0; i < PSU_DEV_GROUP_TOTAL; i++) {
	devnamep = psu_dev_group[i].psu_dev_namep;
	for (j = 0; j < psu_dev_group[i].psu_dev_total; j++, devnamep++) {
	    if (strncmp(devnamep->inv_dev_attrp, PSU_ATTR_STATE, PSU_ATTR_STATE_LEN) == 0) {
		invwirep = psu_attr_get_wirep(PSU_ATTR_STATE, psu_dev_group[i].psu_name, &psu_statep);
		if (invwirep == NULL) {
		    printk(KERN_DEBUG "[p_thread] Invalid psuname: %s\n", psu_dev_group[i].psu_name);
		    continue;
		}
		sprintf(acc_path, devnamep->inv_dev_pathp, invwirep);
		//printk(KERN_INFO "[p_thread] RYU: %s/%d: acc_path = %s\n",__func__,__LINE__,acc_path);
		if (inventec_show_attr(state, acc_path) <= 0) {
		    printk(KERN_DEBUG "[p_thread] Read %s failed\n", acc_path);
		    if (strncmp(psu_statep, PSU_STATE_ERROR, strlen(PSU_STATE_ERROR)) != 0) {
			strcpy(psu_statep, PSU_STATE_ERROR);
			SYSFS_LOG("[p_thread] %s: %s\n",psu_dev_group[i].psu_name,PSU_STATE_ERROR);
		    }
		    flag = 1;
		}
		else
		if (strstr(state, "normal")) {
		    //printk(KERN_INFO "[p_thread] %s: %s\n", psu_dev_group[i].psu_name, state);
		    if (strncmp(psu_statep, state, strlen(state)) != 0) {
			strcpy(psu_statep, state);
			SYSFS_LOG("[p_thread] %s: %s\n",psu_dev_group[i].psu_name,state);
		    }
		}
		else
		if (psu_voltin > PSU_VOLTIN_ACDC) {	/* AC PSUS */
		    //printk(KERN_INFO "[p_thread] RYU: %s: %s\n", psu_dev_group[i].psu_name, state);
		    if (strncmp(psu_statep, state, strlen(state)) != 0) {
			strcpy(psu_statep, state);
			SYSFS_LOG("[p_thread] %s: %s\n",psu_dev_group[i].psu_name,state);
		    }
		    flag = 1;
		}
		else {					/* DC PSUS */
		    if (strncmp(psu_statep, PSU_STATE_CHECKPSU, PSU_STATE_LEN_CHECKPSU) != 0) {
			strcpy(psu_statep, PSU_STATE_CHECKPSU);
			SYSFS_LOG("[p_thread] %s: %s\n",psu_dev_group[i].psu_name,PSU_STATE_CHECKPSU);
		    }
		    flag = 1;
		}
	    }
	}
    }

    if (log_only) {
	return 0;
    }

    //SYSFS_LOG("[p_thread] RYU: %s: flag = %d\n",psu_wire_tbl[i].psu_name,flag);
    if (flag == 1) {
	status_led_grn("3");
	return 1;
    }
    return 0;
}

/* End of psuinfo_device */

static int __init
psu_device_init(void)
{
#ifdef	INV_PTHREAD_KERNEL_MODULE
    sysfs_psu_path_init();
#endif
    return 0;
}


static void __exit
psu_device_exit(void)
{
    printk(KERN_INFO "[p_thread] Remove psu module.\n");
}

/* led device *************************************/
#define STATUS_LED_GRN_PATH	"/sys/class/hwmon/hwmon%d/device/grn_led"
#define STATUS_LED_RED_PATH	"/sys/class/hwmon/hwmon%d/device/red_led"

#define FAN_LED_GRN1_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_grn1"
#define FAN_LED_GRN2_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_grn2"
#define FAN_LED_GRN3_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_grn3"
#define FAN_LED_GRN4_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_grn4"
#define FAN_LED_RED1_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_red1"
#define FAN_LED_RED2_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_red2"
#define FAN_LED_RED3_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_red3"
#define FAN_LED_RED4_PATH	"/sys/class/hwmon/hwmon%d/device/fan_led_red4"

#define HWMON_DEVICE_DIAG_PATH	"/sys/class/hwmon/hwmon%d/device/diag"
#define HWMON_DEVICE_CTRL_PATH	"/sys/class/hwmon/hwmon%d/device/ctl"

static char status_led_grn_path[MAX_PATH_SIZE];
static char status_led_red_path[MAX_PATH_SIZE];
static char fan_led_grn1_path[MAX_PATH_SIZE];
static char fan_led_grn2_path[MAX_PATH_SIZE];
static char fan_led_grn3_path[MAX_PATH_SIZE];
static char fan_led_grn4_path[MAX_PATH_SIZE];
static char fan_led_red1_path[MAX_PATH_SIZE];
static char fan_led_red2_path[MAX_PATH_SIZE];
static char fan_led_red3_path[MAX_PATH_SIZE];
static char fan_led_red4_path[MAX_PATH_SIZE];
static char hwmon_device_diag_path[MAX_PATH_SIZE];
static char hwmon_device_ctrl_path[MAX_PATH_SIZE];

void sysfs_led_path_init(void)
{
    sprintf(&status_led_grn_path[0], STATUS_LED_GRN_PATH, get_hwm_cpld());
    sprintf(&status_led_red_path[0], STATUS_LED_RED_PATH, get_hwm_cpld());
    sprintf(&fan_led_grn1_path[0], FAN_LED_GRN1_PATH, get_hwm_psoc());
    sprintf(&fan_led_grn2_path[0], FAN_LED_GRN2_PATH, get_hwm_psoc());
    sprintf(&fan_led_grn3_path[0], FAN_LED_GRN3_PATH, get_hwm_psoc());
    sprintf(&fan_led_grn4_path[0], FAN_LED_GRN4_PATH, get_hwm_psoc());
    sprintf(&fan_led_red1_path[0], FAN_LED_RED1_PATH, get_hwm_psoc());
    sprintf(&fan_led_red2_path[0], FAN_LED_RED2_PATH, get_hwm_psoc());
    sprintf(&fan_led_red3_path[0], FAN_LED_RED3_PATH, get_hwm_psoc());
    sprintf(&fan_led_red4_path[0], FAN_LED_RED4_PATH, get_hwm_psoc());
    sprintf(&hwmon_device_diag_path[0], HWMON_DEVICE_DIAG_PATH, get_hwm_psoc());
    sprintf(&hwmon_device_ctrl_path[0], HWMON_DEVICE_CTRL_PATH, get_hwm_cpld());
}

/* return 0/off 1/green 2/red */
int
status_led_check_color(void)
{
    char tmpbuf[MIN_ACC_SIZE];
    int ret = STATUS_LED_INVALID;

    if (inventec_show_attr(&tmpbuf[0], status_led_grn_path) > 0) {
	if (tmpbuf[0] == '0') {
	    ret = STATUS_LED_GRN0;
	}
	if (tmpbuf[0] == '1') {
	    ret = STATUS_LED_GRN1;
	}
	if (tmpbuf[0] == '2') {
	    ret = STATUS_LED_GRN2;
	}
	if (tmpbuf[0] == '3') {
	    ret = STATUS_LED_GRN3;
	}
	if (tmpbuf[0] == '7') {
	    ret = STATUS_LED_GRN7;
	}
        return ret;
    }

    if (inventec_show_attr(&tmpbuf[0], status_led_red_path) > 0) {
	if (tmpbuf[0] == '0') {
	    ret = STATUS_LED_RED0;
	}
	if (tmpbuf[0] == '1') {
	    ret = STATUS_LED_RED1;
	}
	if (tmpbuf[0] == '2') {
	    ret = STATUS_LED_RED2;
	}
	if (tmpbuf[0] == '3') {
	    ret = STATUS_LED_RED3;
	}
	if (tmpbuf[0] == '7') {
	    ret = STATUS_LED_RED7;
	}
	return ret;
    }
    return ret;
}

/*
 * Store attr Section
 */
static DEFINE_MUTEX(diag_mutex);

ssize_t status_led_diag_mode_enable(void)
{
    char tmp[MIN_ACC_SIZE];
    ssize_t ret;

    ret = inventec_show_attr(&tmp[0], hwmon_device_diag_path);
    if (ret <= 0) {
        return ret;
    }

    if (tmp[0] == '0') {
	mutex_lock(&diag_mutex);
        ret = inventec_store_attr("1", 1, hwmon_device_diag_path);
        if (ret < 0) {
	    mutex_unlock(&diag_mutex);
            return ret;
        }

        ret = inventec_store_attr("1", 1, hwmon_device_ctrl_path);
        if (ret < 0) {
	    mutex_unlock(&diag_mutex);
            return ret;
        }
	mutex_unlock(&diag_mutex);
    }

    return ret;
}

ssize_t status_led_diag_mode_disable(void)
{
    char tmp[MIN_ACC_SIZE];
    ssize_t ret;

    ret = inventec_show_attr(&tmp[0], hwmon_device_diag_path);
    if (ret <= 0) {
        return ret;
    }

    if (tmp[0] == '1') {
	mutex_lock(&diag_mutex);
        ret = inventec_store_attr("0", 1, hwmon_device_diag_path);
        if (ret < 0) {
	    mutex_unlock(&diag_mutex);
            return 1;
        }

        ret = inventec_store_attr("1", 1, hwmon_device_ctrl_path);
        if (ret < 0) {
	    mutex_unlock(&diag_mutex);
            return 1;
        }
	mutex_unlock(&diag_mutex);
    }
    return 1;
}

ssize_t
status_led_change(const char *path1, const char *tmp1, const char *path2, const char *tmp2)
{
    ssize_t ret;

    ret = inventec_store_attr(tmp1, strlen(tmp1), path1);
    if (ret < 0) {
        return ret;
    }
    ret = inventec_store_attr(tmp2, strlen(tmp2), path2);
    if (ret < 0) {
        return ret;
    }
    if ((ret = status_led_diag_mode_enable()) <= 0) {
        return ret;
    }
    ssleep(1);
    if ((ret = status_led_diag_mode_disable()) <= 0) {
        return ret;
    }
    return ret;
}

ssize_t
status_led_red(const char *freq)
{
    ssize_t ret;

    ret = inventec_store_attr("0", 1, &status_led_grn_path[0]);
    if (ret < 0) {
        return ret;
    }
    ret = inventec_store_attr(freq, strlen(freq), &status_led_red_path[0]);
    if (ret < 0) {
        return ret;
    }
    if ((ret = status_led_diag_mode_enable()) <= 0) {
        return ret;
    }
    ssleep(1);
    if ((ret = status_led_diag_mode_disable()) <= 0) {
        return ret;
    }
    return ret;
}

ssize_t
status_led_grn(const char *freq)
{
    ssize_t ret;

    ret = inventec_store_attr("0", 1, &status_led_red_path[0]);
    if (ret < 0) {
        return ret;
    }
    ret = inventec_store_attr(freq, strlen(freq), &status_led_grn_path[0]);
    if (ret < 0) {
        return ret;
    }
    if ((ret = status_led_diag_mode_enable()) <= 0) {
        return ret;
    }
    ssleep(1);
    if ((ret = status_led_diag_mode_disable()) <= 0) {
        return ret;
    }
    return ret;
}

static int status_led_diag_mode = STATUS_LED_MODE_AUTO;

int status_led_check_diag_mode(void)
{
    return status_led_diag_mode;
}

/* End of ledinfo_device */

static int __init
led_device_init(void)
{
#ifdef	INV_PTHREAD_KERNEL_MODULE
    sysfs_led_path_init();
#endif
    return 0;
}


static void __exit
led_device_exit(void)
{
    printk(KERN_INFO "[p_thread] Remove led module.\n");
}

/* sensor device **********************************/
#define SENSOR_DEV_PATH_SWITCH_TEMP	"/sys/class/hwmon/hwmon%d/device/switch_tmp"

static char sensor_dev_path_switch_temp[MAX_PATH_SIZE];

void sysfs_sensor_path_init(void)
{
    sprintf(&sensor_dev_path_switch_temp[0], SENSOR_DEV_PATH_SWITCH_TEMP, get_hwm_psoc());
}

void switch_temp_update(void)
{
    char buf[MIN_ACC_SIZE];
    ssize_t count = inventec_show_attr(&buf[0], "proc/switch/temp");
    if (count > 0) {
        //printk(KERN_ERR "[p_thread] [STEMP] Switch temperature is out of range: %d\n", stemp);
        inventec_store_attr(&buf[0], count, sensor_dev_path_switch_temp);
    }
}

/**************************************************/
/* From system_device */
static int inv_pthread_control = 1;

int thread_control(void)
{
    return inv_pthread_control;
}

void thread_control_set(int val)
{
    inv_pthread_control = val;
}
/* End system_device */

#define THREAD_SLEEP_MINS	(3)
#define THREAD_DELAY_MINS	(THREAD_SLEEP_MINS + THREAD_SLEEP_MINS + 1)

extern void psu_get_voltin(void);

static struct task_struct *thread_st;
static int thread_data;

#ifdef SWITCH_HEALTH_LED_CHANGE_VIA_GPIO
void led_set_gpio_to_change_status_led(void)
{
    ssize_t ret = inventec_store_attr("57", 2, "/sys/class/gpio/export");
    if (ret < 0) {
	SYSFS_LOG("[p_thread] Write 57 to /sys/class/gpio/export failed\n");
	return;
    }
    ret = inventec_store_attr("low", 3, "/sys/class/gpio/gpio57/direction");
    if (ret < 0) {
	SYSFS_LOG("[p_thread] Write low to /sys/class/gpio/gpio57/direction failed\n");
	return;
    }
    SYSFS_LOG("[p_thread] Set gpio to support status led change successfully\n");
}
#endif

// Function executed by kernel thread
static int thread_fn(void *unused)
{
    /* Delay for guarantee HW ready */
    ssleep(THREAD_DELAY_MINS);

#ifndef	INV_PTHREAD_KERNEL_MODULE
    sysfs_led_path_init();
    sysfs_fan_path_init();
    sysfs_psu_path_init();
#endif
    //sysfs_sensor_path_init();

    /* Default status init */
    status_led_grn("7");

    psu_get_voltin();

#ifdef SWITCH_HEALTH_LED_CHANGE_VIA_GPIO
    led_set_gpio_to_change_status_led();
#endif

    while (1)
    {
	ssleep(THREAD_SLEEP_MINS);

	if (thread_control() == 0) {
	    printk(KERN_INFO "[p_thread] %s/%d: Thread Stop by inv_pthread control\n",__func__,__LINE__);
	    break;
	}

	if (status_led_check_diag_mode() == STATUS_LED_MODE_MANU) {
            /* status led in change color/freq mode, higher priority. Ignore fans sttaus */
            continue;
	}

	//switch_temp_update();

	if (fans_control() > 0) {
	    psus_control(1);
	    continue;
	}
	else
	if (psus_control(0) > 0) {
	    continue;
	}

	if (status_led_check_color() != STATUS_LED_GRN7) {      /* status led red, change it to green */
	    status_led_grn("7");
	}
    }

#ifndef	INV_PTHREAD_KERNEL_MODULE
err_inv_pthread_fn_1:
#endif
    do_exit(0);
    printk(KERN_INFO "[p_thread] %s/%d: Thread Stopped\n",__func__,__LINE__);
    return 0;
}


static ssize_t s_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    int fan_absence;
    size_t count;

    fan_absence = fans_control();
    count += sprintf(&buf[count], "%d\n", fan_absence);
    return count;
}

static ssize_t s_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t count)
{
    return count;
}

static struct attribute status_att = {
    .name = "fan_absence",
    .mode = 0777,
};

static const struct sysfs_ops status_ops = {
    .show = s_show,
    .store = s_store,
};

static struct kobj_type status_ktype = {
    .sysfs_ops = &status_ops,
};


static int __init inv_pthread_init(void)
{
    int retval;

    status_kobj = kzalloc(sizeof(*status_kobj), GFP_KERNEL);
    if(!status_kobj)
        return PTR_ERR(status_kobj);

    status_kset = kset_create_and_add("platform_status", NULL, kernel_kobj);
    if(!status_kset)
        return -1;

    status_kobj->kset = status_kset;

    retval = kobject_init_and_add(status_kobj, &status_ktype, NULL, "fan");
    if(retval)
        return retval;

    retval = sysfs_create_file(status_kobj, &status_att);


    inventec_class_init();
    fan_device_init();
    psu_device_init();
    led_device_init();

    thread_control_set(1);

    printk(KERN_INFO "[p_thread] %s/%d: Creating Thread\n",__func__,__LINE__);
    //Create the kernel thread with name 'inv_pthread'
    thread_st = kthread_run(thread_fn, (void*)&thread_data, "inv_pthread");
    if (thread_st)
        printk(KERN_INFO "[p_thread] inv_pthread Created successfully\n");
    else
        printk(KERN_ERR "[p_thread] inv_pthread creation failed\n");

    return retval;
}

static void __exit inv_pthread_exit(void)
{
    thread_control_set(0);
    /* Delay for guarantee thread exit */
    ssleep(THREAD_DELAY_MINS);

    fan_device_exit();
    psu_device_exit();
    led_device_exit();
    inventec_class_exit();

    sysfs_remove_file(status_kobj, &status_att);
    kset_unregister(status_kset);
    kobject_del(status_kobj);

    printk(KERN_INFO "[p_thread] inv_pthread cleaning Up\n");
}

module_init(inv_pthread_init);
module_exit(inv_pthread_exit);

MODULE_AUTHOR("Robert <yu.robertxk@inventec.com>");
MODULE_DESCRIPTION("Inventec Platform Management Thread");
MODULE_VERSION("version 1.0");
MODULE_LICENSE("GPL");
