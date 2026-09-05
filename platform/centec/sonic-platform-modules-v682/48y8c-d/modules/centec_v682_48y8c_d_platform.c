#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/leds.h>
#include <linux/spinlock.h>

#define SEP(XXX) 1
#define IS_INVALID_PTR(_PTR_) ((_PTR_ == NULL) || IS_ERR(_PTR_))
#define IS_VALID_PTR(_PTR_) (!IS_INVALID_PTR(_PTR_))

#if SEP("defines")
#define SFP_NUM                 48
#define QSFP_NUM                8
#define PORT_NUM                (SFP_NUM + QSFP_NUM)
#endif

#if SEP("drivers:leds")
extern void v682_48y8c_d_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value);
extern enum led_brightness v682_48y8c_d_led_port_get(struct led_classdev *led_cdev);

static struct led_classdev led_dev_port[PORT_NUM] = {
{   .name = "port0",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port1",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port2",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port3",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port4",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port5",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port6",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port7",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port8",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port9",     .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port10",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port11",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port12",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port13",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port14",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port15",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port16",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port17",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port18",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port19",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port20",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port21",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port22",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port23",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port24",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port25",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port26",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port27",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port28",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port29",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port30",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port31",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port32",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port33",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port34",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port35",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port36",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port37",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port38",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port39",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port40",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port41",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port42",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port43",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port44",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port45",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port46",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port47",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port48",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port49",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port50",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port51",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port52",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port53",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port54",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
{   .name = "port55",    .brightness_set = v682_48y8c_d_led_port_set,    .brightness_get = v682_48y8c_d_led_port_get,},
};
static unsigned char port_led_mode[PORT_NUM] = {0};

void v682_48y8c_d_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);
    
    port_led_mode[portNum] = set_value;

    return;
}

enum led_brightness v682_48y8c_d_led_port_get(struct led_classdev *led_cdev)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);    
    
    return port_led_mode[portNum];
}

static int v682_48y8c_d_init_led(void)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < PORT_NUM; i++)
    {
        ret = led_classdev_register(NULL, &(led_dev_port[i]));
        if (ret != 0)
        {
            printk(KERN_CRIT "create v682_48y8c_d led_dev_port%d device failed\n", i);
            continue;
        }
    }
    
    return ret;
}

static int v682_48y8c_d_exit_led(void)
{
    int i = 0;

    for (i = 0; i < PORT_NUM; i++)
    {
        led_classdev_unregister(&(led_dev_port[i]));
    }

    return 0;
}
#endif

#if SEP("drivers:sfp")
#define MAX_SFP_EEPROM_DATA_LEN 256
#define MAX_SFP_EEPROM_NUM 3
struct sfp_info_t {
    char eeprom[MAX_SFP_EEPROM_NUM][MAX_SFP_EEPROM_DATA_LEN + 1];
    unsigned short data_len[MAX_SFP_EEPROM_NUM];
    int presence;
    int enable;
    spinlock_t lock;
};
static struct class *sfp_class = NULL;
static struct device *sfp_dev[SFP_NUM+QSFP_NUM + 1] = {NULL};
static struct sfp_info_t sfp_info[SFP_NUM+QSFP_NUM + 1];

static ssize_t v682_48y8c_d_sfp_read_presence(struct device *dev, struct device_attribute *attr, char *buf)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;
    int presence = 0;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp read presence, invalid port number!\n");
        buf[0] = '\0';
        return 0;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    presence = sfp_info[portNum].presence;
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);
    return sprintf(buf, "%d\n", presence);
}

static ssize_t v682_48y8c_d_sfp_write_presence(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;
    int presence = simple_strtol(buf, NULL, 10);

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp read presence, invalid port number!\n");
        return size;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    sfp_info[portNum].presence = presence;
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);
    
    return size;
}

static ssize_t v682_48y8c_d_sfp_read_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;
    int enable = 0;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp read enable, invalid port number!\n");
        buf[0] = '\0';
        return 0;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    enable = sfp_info[portNum].enable;
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);
    return sprintf(buf, "%d\n", enable);
}

static ssize_t v682_48y8c_d_sfp_write_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;
    int enable = simple_strtol(buf, NULL, 10);

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp read enable, invalid port number!\n");
        return size;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    sfp_info[portNum].enable = enable;
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);
    
    return size;
}

static ssize_t v682_48y8c_d_sfp_read_eeprom(struct device *dev, struct device_attribute *attr, char *buf)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;
    size_t size = 0;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp read eeprom, invalid port number!\n");
        buf[0] = '\0';
        return 0;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    memcpy(buf, sfp_info[portNum].eeprom[0], sfp_info[portNum].data_len[0]);
    size = sfp_info[portNum].data_len[0];
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);

    return size;
}

static ssize_t v682_48y8c_d_sfp_write_eeprom(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int portNum = 0;
    const char *name = dev_name(dev);
    unsigned long flags = 0;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 0) || (portNum >= SFP_NUM + QSFP_NUM))
    {
        printk(KERN_CRIT "sfp write eeprom, invalid port number!\n");
        return size;
    }

    spin_lock_irqsave(&(sfp_info[portNum].lock), flags);
    memcpy(sfp_info[portNum].eeprom[0], buf, size);
    sfp_info[portNum].data_len[0] = size;
    spin_unlock_irqrestore(&(sfp_info[portNum].lock), flags);
    
    return size;
}

static DEVICE_ATTR(sfp_presence, S_IRUGO|S_IWUSR, v682_48y8c_d_sfp_read_presence, v682_48y8c_d_sfp_write_presence);
static DEVICE_ATTR(sfp_enable, S_IRUGO|S_IWUSR, v682_48y8c_d_sfp_read_enable, v682_48y8c_d_sfp_write_enable);
static DEVICE_ATTR(sfp_eeprom, S_IRUGO|S_IWUSR, v682_48y8c_d_sfp_read_eeprom, v682_48y8c_d_sfp_write_eeprom);

static int v682_48y8c_d_init_sfp(void)
{
    int ret = 0;
    int i = 0;
    
    sfp_class = class_create(THIS_MODULE, "sfp");
    if (IS_INVALID_PTR(sfp_class))
    {
        sfp_class = NULL;
        printk(KERN_CRIT "create v682_48y8c_d class sfp failed\n");
        return -1;
    }

    for (i = 0; i < SFP_NUM + QSFP_NUM; i++)
    {
        memset(&(sfp_info[i].eeprom), 0, sizeof(sfp_info[i].eeprom));
        memset(&(sfp_info[i].data_len), 0, sizeof(sfp_info[i].data_len));
        spin_lock_init(&(sfp_info[i].lock));

        sfp_dev[i] = device_create(sfp_class, NULL, MKDEV(223, i), NULL, "sfp%d", i);
        if (IS_INVALID_PTR(sfp_dev[i]))
        {
            sfp_dev[i] = NULL;
            printk(KERN_CRIT "create v682_48y8c_d sfp[%d] device failed\n", i);
            continue;
        }

        ret = device_create_file(sfp_dev[i], &dev_attr_sfp_presence);
        if (ret != 0)
        {
            printk(KERN_CRIT "create v682_48y8c_d sfp[%d] device attr:presence failed\n", i);
            continue;
        }

        ret = device_create_file(sfp_dev[i], &dev_attr_sfp_enable);
        if (ret != 0)
        {
            printk(KERN_CRIT "create v682_48y8c_d sfp[%d] device attr:enable failed\n", i);
            continue;
        }

        ret = device_create_file(sfp_dev[i], &dev_attr_sfp_eeprom);
        if (ret != 0)
        {
            printk(KERN_CRIT "create v682_48y8c_d sfp[%d] device attr:eeprom failed\n", i);
            continue;
        }
    }
    
    return ret;
}

static int v682_48y8c_d_exit_sfp(void)
{
    int i = 0;

    for (i = 0; i < SFP_NUM + QSFP_NUM; i++)
    {
        if (IS_VALID_PTR(sfp_dev[i]))
        {
            device_remove_file(sfp_dev[i], &dev_attr_sfp_presence);
            device_remove_file(sfp_dev[i], &dev_attr_sfp_enable);
            device_remove_file(sfp_dev[i], &dev_attr_sfp_eeprom);
            device_destroy(sfp_class, MKDEV(223, i));
            sfp_dev[i] = NULL;
        }
    }

    if (IS_VALID_PTR(sfp_class))
    {
        class_destroy(sfp_class);
        sfp_class = NULL;
    }

    return 0;
}
#endif

static int v682_48y8c_d_init(void)
{
    int ret = 0;
    int failed = 0;
    
    printk(KERN_ALERT "init v682_48y8c_d board dirver...\n");
    
    ret = v682_48y8c_d_init_led();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = v682_48y8c_d_init_sfp();
    if (ret != 0)
    {
        failed = 1;
    }

    if (failed)
        printk(KERN_INFO "init v682_48y8c_d board driver failed\n");
    else
        printk(KERN_ALERT "init v682_48y8c_d board dirver...ok\n");
    
    return 0;
}

static void v682_48y8c_d_exit(void)
{
    printk(KERN_INFO "deinit v682_48y8c_d board dirver...\n");
    
    v682_48y8c_d_exit_sfp();
    v682_48y8c_d_exit_led();

    printk(KERN_INFO "deinit v682_48y8c_d board dirver...ok\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("shil centecNetworks, Inc");
MODULE_DESCRIPTION("v682-48y8c-d board driver");
module_init(v682_48y8c_d_init);
module_exit(v682_48y8c_d_exit);
