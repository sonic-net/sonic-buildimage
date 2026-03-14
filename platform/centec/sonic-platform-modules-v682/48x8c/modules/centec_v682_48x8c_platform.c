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
extern void v682_48x8c_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value);
extern enum led_brightness v682_48x8c_led_port_get(struct led_classdev *led_cdev);

static struct led_classdev led_dev_port[PORT_NUM] = {
{   .name = "port0",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port1",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port2",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port3",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port4",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port5",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port6",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port7",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port8",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port9",     .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port10",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port11",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port12",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port13",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port14",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port15",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port16",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port17",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port18",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port19",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port20",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port21",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port22",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port23",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port24",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port25",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port26",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port27",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port28",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port29",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port30",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port31",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port32",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port33",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port34",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port35",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port36",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port37",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port38",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port39",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port40",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port41",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port42",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port43",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port44",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port45",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port46",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port47",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port48",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port49",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port50",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port51",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port52",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port53",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port54",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
{   .name = "port55",    .brightness_set = v682_48x8c_led_port_set,    .brightness_get = v682_48x8c_led_port_get,},
};
static unsigned char port_led_mode[PORT_NUM] = {0};

void v682_48x8c_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);
    
    port_led_mode[portNum] = set_value;

    return;
}

enum led_brightness v682_48x8c_led_port_get(struct led_classdev *led_cdev)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);    
    
    return port_led_mode[portNum];
}

static int v682_48x8c_init_led(void)
{
    int ret = 0;
    int i = 0;

    for (i = 0; i < PORT_NUM; i++)
    {
        ret = led_classdev_register(NULL, &(led_dev_port[i]));
        if (ret != 0)
        {
            printk(KERN_CRIT "create v682_48x8c led_dev_port%d device failed\n", i);
            continue;
        }
    }
    
    return ret;
}

static int v682_48x8c_exit_led(void)
{
    int i = 0;

    for (i = 0; i < PORT_NUM; i++)
    {
        led_classdev_unregister(&(led_dev_port[i]));
    }

    return 0;
}
#endif

static int v682_48x8c_init(void)
{
    int ret = 0;
    int failed = 0;
    
    printk(KERN_ALERT "init v682_48x8c board dirver...\n");
    
    ret = v682_48x8c_init_led();
    if (ret != 0)
    {
        failed = 1;
    }

    if (failed)
        printk(KERN_INFO "init v682_48x8c board driver failed\n");
    else
        printk(KERN_ALERT "init v682_48x8c board dirver...ok\n");
    
    return 0;
}

static void v682_48x8c_exit(void)
{
    printk(KERN_INFO "deinit v682_48x8c board dirver...\n");
    
    v682_48x8c_exit_led();

    printk(KERN_INFO "deinit v682_48x8c board dirver...ok\n");
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("shil centecNetworks, Inc");
MODULE_DESCRIPTION("v682-48x8c board driver");
module_init(v682_48x8c_init);
module_exit(v682_48x8c_exit);
