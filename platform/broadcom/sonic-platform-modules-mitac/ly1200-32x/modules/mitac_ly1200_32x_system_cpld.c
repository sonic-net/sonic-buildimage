#define CONFIG_DRV_SYSCPLD_WDT 1

#include <linux/module.h>
#include <linux/i2c.h>

#include "system_cpld_reg.h"
#include "system_cpld_sysfs.h"
#ifdef CONFIG_DRV_SYSCPLD_WDT
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/miscdevice.h>
#include <linux/reboot.h>
#include <linux/watchdog.h>
#include <asm/uaccess.h>
#endif

static int debug_flag = 0;

struct system_cpld_data {
    struct mutex lock;

    struct i2c_client *client;
    struct device_attribute bin;
};
struct system_cpld_data *system_cpld;

static const struct i2c_device_id system_cpld_ids[] = {
    { "system_cpld", 0 },
    { /* END OF LIST */ }
};
MODULE_DEVICE_TABLE(i2c, system_cpld_ids);

static int system_cpld_raw_read(struct device *dev, struct device_attribute *attr, char *buf,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    unsigned int reg_val = 0, fld_val;
    static int debug_flag;
    struct system_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    int err;

    if (reg_width != 8){
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }
    mutex_lock(&data->lock);
    if ((err = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0) {
        /* CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n", reg_name, err);
        return err;
    }
    reg_val = err;
    if (debug_flag) {
            printk("%s: reg_offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    mutex_unlock(&data->lock);
    if (fld_width == reg_width) {fld_val = reg_val & fld_mask;}
    else {fld_val = (reg_val >> fld_shift) & fld_mask;}
    return sprintf(buf, "0x%x\n", fld_val);
}

static int system_cpld_raw_write(struct device *dev, struct device_attribute *attr, const char *buf, size_t count,
    int reg_offset, int reg_width, int fld_shift, int fld_width, int fld_mask, char* reg_name){
    int ret_code;
    unsigned int reg_val, fld_val;
    unsigned long val;
    static int debug_flag;
    struct system_cpld_data *data = dev_get_drvdata(dev);
    struct i2c_client *client = data->client;
    if (reg_width != 8){
        printk("%s: Register table width setting failed.\n", reg_name);
        return -EINVAL;
    }
    /* Parse buf and store to fld_val */
    if ((ret_code = kstrtoul(buf, 16, &val))){
        printk("%s: Conversion value = %s failed, errno = %d.\n", reg_name, buf, ret_code);
        return ret_code;
    }
    fld_val = (unsigned int)val;
    mutex_lock(&data->lock);
    if ((ret_code = i2c_smbus_read_byte_data(client, (u8)reg_offset)) < 0) {
        /* Handle CPLD read error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c read failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    reg_val = ret_code;
    if (debug_flag) {
        printk("%s: offset = %d, width = %d, cur value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    if (fld_width == reg_width) {reg_val = fld_val & fld_mask;}
    else {reg_val = (reg_val & ~(fld_mask << fld_shift)) |
                    ((fld_val & (fld_mask)) << fld_shift);}
    if ((ret_code = i2c_smbus_write_byte_data(client, (u8)reg_offset, (u8)reg_val)) != 0) {
        /* Handle CPLD write error condition */;
        mutex_unlock(&data->lock);
        printk("%s: i2c write failed, error code = %d.\n",  reg_name, ret_code);
        return ret_code;
    }
    else if (debug_flag) {
        printk("%s: offset = %d, width = %d, new value = 0x%x.\n", reg_name, reg_offset, reg_width, reg_val);
    }
    mutex_unlock(&data->lock);
    return count;
}

/*--------------------special file for debug---------------------- */
static ssize_t system_cpld_debug_read(struct device *dev, struct device_attribute *attr,
             char *buf){

    return sprintf(buf, "%d\n", debug_flag);
}


static ssize_t system_cpld_debug_write(struct device *dev, struct device_attribute *attr,
            const char *buf, size_t count)
{
    int temp;
    int error;

    error = kstrtoint(buf, 10, &temp);
    if (error){
        printk(KERN_INFO "%s: Conversion value = %s failed.\n", __FUNCTION__, buf);
        return count;
    }
    debug_flag = temp;

    if(debug_flag)
        printk("%s, debug_flag = %d\n", __FUNCTION__, debug_flag);

    return count;
}
SYSFS_MISC_RW_ATTR_DEF(debug, system_cpld_debug_read, system_cpld_debug_write)


/* ----------------define misc group---------------------------- */
static struct attribute *misc_attributes[] = {
    SYSFS_ATTR_PTR(mjr_rev),              /* register: sys_cpld_rev */
    SYSFS_ATTR_PTR(mnr_rev),              /* register: sys_cpld_rev */

    SYSFS_ATTR_PTR(scrtch_reg),           /* register: sys_cpld_gpr */

    SYSFS_ATTR_PTR(brd_rev),              /* register: cpu_brd_rev_type */
    SYSFS_ATTR_PTR(brd_type),             /* register: cpu_brd_rev_type */

    SYSFS_ATTR_PTR(ssd_present),          /* register: sys_srr */
    SYSFS_ATTR_PTR(spi_cs_sel),           /* register: sys_srr */
    SYSFS_ATTR_PTR(rst_bios_switch),      /* register: sys_srr */
    SYSFS_ATTR_PTR(cpld_upgrade_rst),     /* register: sys_srr */

    SYSFS_ATTR_PTR(cpld_spi_wp),          /* register: sys_eeprom_wp */
    SYSFS_ATTR_PTR(system_id_eeprom_wp),  /* register: sys_eeprom_wp */
    SYSFS_ATTR_PTR(spi_me_wp),            /* register: sys_eeprom_wp */
    SYSFS_ATTR_PTR(spi_bios_wp),          /* register: sys_eeprom_wp */
    SYSFS_ATTR_PTR(spi_bak_bios_wp),      /* register: sys_eeprom_wp */

    SYSFS_ATTR_PTR(vrhot_irq_en),         /* register: sys_irq */
    SYSFS_ATTR_PTR(cpu_thermtrip_irq_en), /* register: sys_irq */
    SYSFS_ATTR_PTR(temp_alert_irq_en),    /* register: sys_irq */
    SYSFS_ATTR_PTR(vrhot_irq),            /* register: sys_irq */
    SYSFS_ATTR_PTR(cpu_thermtrip_irq),    /* register: sys_irq */
    SYSFS_ATTR_PTR(temp_alert_irq),       /* register: sys_irq */

    SYSFS_ATTR_PTR(wd_timer),             /* register: sys_wd */
    SYSFS_ATTR_PTR(wd_en),                /* register: sys_wd */
    SYSFS_ATTR_PTR(wd_punch),             /* register: sys_wd */

    SYSFS_ATTR_PTR(mb_rst_en),            /* register: sys_mb_rst_en */

    SYSFS_ATTR_PTR(pwr_v3p3_en),          /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_vcc_vnn_en),       /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_vccsram_en),       /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_vddq_en),          /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_vcc_ref_en),       /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_v1p05_en),         /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_v1p8_en),          /* register: cpu_pwr_en_status */
    SYSFS_ATTR_PTR(pwr_v2p5_en),          /* register: cpu_pwr_en_status */

    SYSFS_ATTR_PTR(pg_v3p3),              /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_vcc_vnn),           /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_vccsram),           /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_vddq),              /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_vcc_ref),           /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_v1p05),             /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_v1p8),              /* register: cpu_pwr_status */
    SYSFS_ATTR_PTR(pg_v2p5),              /* register: cpu_pwr_status */

    SYSFS_ATTR_PTR(debug),          /* debug flag for print more messages */
    NULL
};
static const struct attribute_group system_cpld_group_misc = {
    .attrs = misc_attributes,
};

#ifdef CONFIG_DRV_SYSCPLD_WDT
/*
 *****************************************************************************
 *
 * Watchdog Driver
 *
 *****************************************************************************
 */
/* wdt_timeout[] are defined by CPLD spec , -1 means researved
   300 sec is not supported */
int wdt_timeout[]={15,30,60,90,120,180,240,300,-1,-1,-1,-1,-1,-1,-1,-1};
#define WD_TIMO_MAX_NUM 16
/* Default margin */
#define WD_TIMO 30

static int wdt_margin = WD_TIMO;
module_param(wdt_margin, int, 0);
MODULE_PARM_DESC(wdt_margin, "Watchdog timeout in seconds (default "
    __MODULE_STRING(WD_TIMO) "s)");

static unsigned long wdt_is_open;
static int boot_flag;

/**
 * wdt_ping:
 *
 * Reload counter one with the watchdog timeout. We don't bother reloading
 * the cascade counter.
 */
static void wdt_ping(void)
{
    struct device *dev = &system_cpld->client->dev;
    struct device_attribute *fake_attr=NULL;
    char *buf="0";/* 0: punch is defined by CPLD spec */
    int err;
    err = system_cpld_wd_punch_raw_write(dev, fake_attr, buf, (size_t)0);
    if(err < 0){
        system_cpld_wd_punch_raw_write(dev, fake_attr, buf, (size_t)0);
    }
}

/**
 * wdt_disable:
 *
 * disables watchdog.
 */
static void wdt_disable(void)
{
    struct device *dev = &system_cpld->client->dev;
    struct device_attribute *fake_attr=NULL;
    char *buf="0";/* 0: disable is defined by CPLD spec */
    int err;
    err = system_cpld_wd_en_raw_write(dev, fake_attr, buf, (size_t)0);
    if(err < 0){
        system_cpld_wd_en_raw_write(dev, fake_attr, buf, (size_t)0);
    }
}

/**
 * wdt_enable:
 *
 * enables watchdog.
 */
static void wdt_enable(void)
{
    struct device *dev = &system_cpld->client->dev;
    struct device_attribute *fake_attr=NULL;
    char *buf="1";/* 1: enable is defined by CPLD spec */
    int err;
    err = system_cpld_wd_en_raw_write(dev, fake_attr, buf, (size_t)0);
    if(err < 0){
        system_cpld_wd_en_raw_write(dev, fake_attr, buf, (size_t)0);
    }
}

/**
 * wdt_set_timeout:
 *
 * set watchdog timeout.
 */
static void wdt_set_timeout(int index)
{
    struct device *dev = &system_cpld->client->dev;
    struct device_attribute *fake_attr=NULL;
    char buf[1];
    if ( WD_TIMO_MAX_NUM == 16 ) {
        sprintf(buf,"%x",index);
        system_cpld_wd_timer_raw_write(dev, fake_attr, buf, (size_t)0);
    }
    else
        printk(KERN_INFO "%s: It is out of spec.\n", __FUNCTION__);
}

/**
 * wdt_write:
 * @file: file handle to the watchdog
 * @buf: buffer to write (unused as data does not matter here
 * @count: count of bytes
 * @ppos: pointer to the position to write. No seeks allowed
 *
 * A write to a watchdog device is defined as a keepalive signal. Any
 * write of data will do, as we we don't define content meaning.
 */
static ssize_t wdt_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
    if (count) {
        wdt_ping();
        return 1;
    }
    return 0;
}

/**
 * wdt_ioctl:
 * @inode: inode of the device
 * @file: file handle to the device
 * @cmd: watchdog command
 * @arg: argument pointer
 *
 * The watchdog API defines a common set of functions for all watchdogs
 * according to their available features. We only actually usefully support
 * querying capabilities and current status.
 */
static int wdt_ioctl(struct file *file, unsigned int cmd,
                     unsigned long arg)
{
    int new_margin, rv, i;
    static struct watchdog_info ident = {
        .options = WDIOF_KEEPALIVEPING | WDIOF_SETTIMEOUT,
        .firmware_version = 1,
        .identity = "SYS_CPLD WTD"
    };

    switch (cmd) {
    case WDIOC_GETSUPPORT:
        return copy_to_user((struct watchdog_info __user *)arg, &ident,
                            sizeof(ident)) ? -EFAULT : 0;

    case WDIOC_GETSTATUS:
    case WDIOC_GETBOOTSTATUS:
        return put_user(boot_flag, (int __user *)arg);
    case WDIOC_KEEPALIVE:
        wdt_ping();
        return 0;
    case WDIOC_SETTIMEOUT:
        if (get_user(new_margin, (int __user *)arg))
            return -EFAULT;
        /* Arbitrary, can't find the card's limits */
        if (new_margin <= 1)
            return -EINVAL;
        for( i=0; i<WD_TIMO_MAX_NUM; i++)
        {
            if (new_margin == wdt_timeout[i])
                break;
        }
        if ( i >= WD_TIMO_MAX_NUM || i < 0 )
            return -EINVAL;
        wdt_set_timeout(i);
    case WDIOC_GETTIMEOUT:
        return put_user(wdt_margin, (int __user *)arg);

    case WDIOC_SETOPTIONS:
        if (copy_from_user(&rv, (int __user *)arg, sizeof(int)))
            return -EFAULT;

        if (rv & WDIOS_DISABLECARD) {
            pr_info("System CPLD: disable watchdog\n");
            wdt_disable();
        }

        if (rv & WDIOS_ENABLECARD) {
            pr_info("System CPLD: enable watchdog\n");
            wdt_enable();
        }
        return -EINVAL;
    }
    return -ENOTTY;
}

static long wdt_unlocked_ioctl(struct file *file, unsigned int cmd,
                               unsigned long arg)
{
    int ret;

    ret = wdt_ioctl(file, cmd, arg);

    return ret;
}

/**
 * wdt_open:
 * @inode: inode of device
 * @file: file handle to device
 *
 */
static int wdt_open(struct inode *inode, struct file *file)
{
    if (MINOR(inode->i_rdev) == WATCHDOG_MINOR) {
        if (test_and_set_bit(0, &wdt_is_open)) {
            return -EBUSY;
        }
        /*
         *      Activate
         */

        wdt_enable();
        return nonseekable_open(inode, file);
    }
    return -ENODEV;
}

/**
 * wdt_close:
 * @inode: inode to board
 * @file: file handle to board
 *
 */
static int wdt_release(struct inode *inode, struct file *file)
{
    if (MINOR(inode->i_rdev) == WATCHDOG_MINOR)
        clear_bit(0, &wdt_is_open);
    return 0;
}

/**
 * notify_sys:
 * @this: our notifier block
 * @code: the event being reported
 * @unused: unused
 *
 * Our notifier is called on system shutdowns. We want to turn the card
 * off at reboot otherwise the machine will reboot again during memory
 * test or worse yet during the following fsck. This would suck, in fact
 * trust me - if it happens it does suck.
 */
static int wdt_notify_sys(struct notifier_block *this, unsigned long code,
                          void *unused)
{
   if (code == SYS_DOWN || code == SYS_HALT)
       /* Disable Watchdog */
       wdt_disable();
   return NOTIFY_DONE;
}

static const struct file_operations wdt_fops = {
    .owner  = THIS_MODULE,
    .llseek = no_llseek,
    .write  = wdt_write,
    .unlocked_ioctl = wdt_unlocked_ioctl,
    .open   = wdt_open,
    .release = wdt_release,
};

static struct miscdevice wdt_dev = {
    .minor = WATCHDOG_MINOR,
    .name = "watchdog",
    .fops = &wdt_fops,
};

/*
 * The WDT card needs to learn about soft shutdowns in order to
 * turn the timebomb registers off.
 */
static struct notifier_block wdt_notifier = {
    .notifier_call = wdt_notify_sys,
};
#endif /* CONFIG_DRV_SYSCPLD_WDT */

static int system_cpld_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int err;

    /* allocate memory to system_cpld */
    system_cpld = devm_kzalloc(&client->dev, sizeof(struct system_cpld_data) , GFP_KERNEL);

    if (!system_cpld)
        return -ENOMEM;

    mutex_init(&system_cpld->lock);

    err = sysfs_create_group(&client->dev.kobj, &system_cpld_group_misc);
    if (err){
        printk("%s: Error creeat misc group.\n", __FUNCTION__);
    }

    system_cpld->client = client;
    i2c_set_clientdata(client, system_cpld);

    printk(KERN_INFO "%s: System CPLD LCMXO3LF created.\n", __FUNCTION__);

#ifdef CONFIG_DRV_SYSCPLD_WDT
    err = misc_register(&wdt_dev);
    if (err)
        return err;
    err = register_reboot_notifier(&wdt_notifier);
    if (err) {
        misc_deregister(&wdt_dev);
        return err;
    }
    printk(KERN_INFO "%s: System CPLD watchdog created.\n", __FUNCTION__);
#endif

    return 0;

}

static int system_cpld_remove(struct i2c_client *client)
{
#ifdef CONFIG_DRV_SYSCPLD_WDT
    misc_deregister(&wdt_dev);
    unregister_reboot_notifier(&wdt_notifier);
#endif
    sysfs_remove_group(&client->dev.kobj, &system_cpld_group_misc);

    printk(KERN_INFO "%s: System CPLD removed.\n", __FUNCTION__);
    return 0;
}

static struct i2c_driver system_cpld_driver = {
    .driver = {
        .name = "system_cpld",
        .owner = THIS_MODULE,
    },
    .probe = system_cpld_probe,
    .remove = system_cpld_remove,
    .id_table = system_cpld_ids,
};

static int __init system_cpld_init(void)
{
    printk(KERN_INFO "%s: init.\n", __FUNCTION__);
    return i2c_add_driver(&system_cpld_driver);
}
module_init(system_cpld_init);

static void __exit system_cpld_exit(void)
{
    printk(KERN_INFO "%s: exit.\n", __FUNCTION__);
    i2c_del_driver(&system_cpld_driver);
}
module_exit(system_cpld_exit);

MODULE_DESCRIPTION("mitac_ly1200_32x_system_cpld driver");
MODULE_AUTHOR("Eddy Weng <eddy.weng@mic.com.tw>");
MODULE_LICENSE("GPL");

