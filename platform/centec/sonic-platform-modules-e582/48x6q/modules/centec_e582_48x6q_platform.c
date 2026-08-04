#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include "../../pca954x/ctc-pca954x.h"
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/leds.h>

#define SEP(XXX) 1
#define IS_INVALID_PTR(_PTR_) ((_PTR_ == NULL) || IS_ERR(_PTR_))
#define IS_VALID_PTR(_PTR_) (!IS_INVALID_PTR(_PTR_))

#if SEP("defines")
#define PCA9548_CHANNEL_NUM     8
#define PCA9548_ADAPT_ID_START  10
#define SFP_NUM                 48
#define QSFP_NUM                6
#define PORT_NUM                (SFP_NUM+QSFP_NUM)
#endif

#if SEP("i2c:master")
static struct i2c_adapter *i2c_adp_master          = NULL; /* i2c-0-cpu */

static int e582_48x6q_init_i2c_master(void)
{
    /* find i2c-core master */
    i2c_adp_master = i2c_get_adapter(0);
    if(IS_INVALID_PTR(i2c_adp_master))
    {
        i2c_adp_master = NULL;
        printk(KERN_CRIT "e582_48x6q_init_i2c_master can't find i2c-core bus\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_i2c_master(void)
{
    /* uninstall i2c-core master */
    if(IS_VALID_PTR(i2c_adp_master)) {
        i2c_put_adapter(i2c_adp_master);
        i2c_adp_master = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:pca9548")
static struct pca954x_platform_mode i2c_dev_pca9548_platform_mode[PCA9548_CHANNEL_NUM] = {
    [0] = {
        .adap_id = PCA9548_ADAPT_ID_START,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [1] = {
        .adap_id = PCA9548_ADAPT_ID_START + 1,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [2] = {
        .adap_id = PCA9548_ADAPT_ID_START + 2,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [3] = {
        .adap_id = PCA9548_ADAPT_ID_START + 3,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [4] = {
        .adap_id = PCA9548_ADAPT_ID_START + 4,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [5] = {
        .adap_id = PCA9548_ADAPT_ID_START + 5,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [6] = {
        .adap_id = PCA9548_ADAPT_ID_START + 6,
        .deselect_on_exit = 1,
        .class   = 0,
    },
    [7] = {
        .adap_id = PCA9548_ADAPT_ID_START + 7,
        .deselect_on_exit = 1,
        .class   = 0,
    }
};
static struct pca954x_platform_data i2c_dev_pca9548_platform_data = {
    .modes = i2c_dev_pca9548_platform_mode,
    .num_modes = PCA9548_CHANNEL_NUM,
};
static struct i2c_board_info i2c_dev_pca9548 = {
    I2C_BOARD_INFO("ctc_pca9548", 0x70),
    .platform_data = &i2c_dev_pca9548_platform_data,
};
static struct i2c_client  *i2c_client_pca9548x     = NULL;

static int e582_48x6q_init_i2c_pca9548(void)
{
    if(IS_INVALID_PTR(i2c_adp_master))
    {
        i2c_adp_master = NULL;
        printk(KERN_CRIT "e582_48x6q_init_i2c_pca9548 can't find i2c-core bus\n");
        return -1;
    }

    /* install i2c-mux */
    i2c_client_pca9548x = i2c_new_client_device(i2c_adp_master, &i2c_dev_pca9548);
    if(IS_INVALID_PTR(i2c_client_pca9548x))
    {
        i2c_client_pca9548x = NULL;
        printk(KERN_CRIT "install e582_48x6q board pca9548 failed\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_i2c_pca9548(void)
{
    /* uninstall i2c-core master */
    if(IS_VALID_PTR(i2c_client_pca9548x)) {
        i2c_unregister_device(i2c_client_pca9548x);
        i2c_client_pca9548x = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:adt7470")
static struct i2c_board_info i2c_dev_adt7470 = {
    I2C_BOARD_INFO("adt7470", 0x2F),
};
static struct i2c_adapter *i2c_adp_adt7470         = NULL; /* pca9548x-channel 5 */
static struct i2c_client  *i2c_client_adt7470      = NULL;

static int e582_48x6q_init_i2c_adt7470(void)
{
    i2c_adp_adt7470 = i2c_get_adapter(PCA9548_ADAPT_ID_START + 5);
    if(IS_INVALID_PTR(i2c_adp_adt7470))
    {
        i2c_adp_adt7470 = NULL;
        printk(KERN_CRIT "install e582_48x6q board adt7470 failed\n");
        return -1;
    }
    
    i2c_client_adt7470 = i2c_new_client_device(i2c_adp_adt7470, &i2c_dev_adt7470);
    if(IS_INVALID_PTR(i2c_client_adt7470)){
        i2c_client_adt7470 = NULL;
        printk(KERN_CRIT "install e582_48x6q board adt7470 failed\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_i2c_adt7470(void)
{
    if(IS_VALID_PTR(i2c_client_adt7470)) {
        i2c_unregister_device(i2c_client_adt7470);
        i2c_client_adt7470 = NULL;
    }
    
    if(IS_VALID_PTR(i2c_adp_adt7470)) {
        i2c_put_adapter(i2c_adp_adt7470);
        i2c_adp_adt7470 = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:psu")
static struct i2c_adapter *i2c_adp_psu1           = NULL; /* psu1 channel 3 */
static struct i2c_adapter *i2c_adp_psu2           = NULL; /* psu2 channel 1 */
static struct i2c_board_info i2c_dev_psu1 = {
    I2C_BOARD_INFO("i2c-psu1", 0x38),
};
static struct i2c_board_info i2c_dev_psu2 = {
    I2C_BOARD_INFO("i2c-psu2", 0x38),
};
static struct i2c_client  *i2c_client_psu1        = NULL;
static struct i2c_client  *i2c_client_psu2        = NULL;

static int e582_48x6q_init_i2c_psu(void)
{
    i2c_adp_psu1 = i2c_get_adapter(PCA9548_ADAPT_ID_START + 3);
    if(IS_INVALID_PTR(i2c_adp_psu1))
    {
        i2c_adp_psu1 = NULL;
        printk(KERN_CRIT "get e582_48x6q psu1 i2c-adp failed\n");
        return -1;
    }

    i2c_adp_psu2 = i2c_get_adapter(PCA9548_ADAPT_ID_START + 1);
    if(IS_INVALID_PTR(i2c_adp_psu2))
    {
        i2c_adp_psu2 = NULL;
        printk(KERN_CRIT "get e582_48x6q psu2 i2c-adp failed\n");
        return -1;
    }
    
    i2c_client_psu1 = i2c_new_client_device(i2c_adp_psu1, &i2c_dev_psu1);
    if(IS_INVALID_PTR(i2c_client_psu1)){
        i2c_client_psu1 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client psu1 failed\n");
        return -1;
    }

    i2c_client_psu2 = i2c_new_client_device(i2c_adp_psu2, &i2c_dev_psu2);
    if(IS_INVALID_PTR(i2c_client_psu2)){
        i2c_client_psu2 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client psu2 failed\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_i2c_psu(void)
{
    if(IS_VALID_PTR(i2c_client_psu1)) {
        i2c_unregister_device(i2c_client_psu1);
        i2c_client_psu1 = NULL;
    }

    if(IS_VALID_PTR(i2c_client_psu2)) {
        i2c_unregister_device(i2c_client_psu2);
        i2c_client_psu2 = NULL;
    }

    if(IS_VALID_PTR(i2c_adp_psu1)) 
    {
        i2c_put_adapter(i2c_adp_psu1);
        i2c_adp_psu1 = NULL;
    }

    if(IS_VALID_PTR(i2c_adp_psu2)) 
    {
        i2c_put_adapter(i2c_adp_psu2);
        i2c_adp_psu2 = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:epld")
static struct i2c_board_info i2c_dev_epld = {
    I2C_BOARD_INFO("i2c-epld", 0x58),
};
static struct i2c_client  *i2c_client_epld      = NULL;

static int e582_48x6q_init_i2c_epld(void)
{
    if (IS_INVALID_PTR(i2c_adp_master))
    {
         printk(KERN_CRIT "e582_48x6q_init_i2c_epld can't find i2c-core bus\n");
         return -1;
    }
    
    i2c_client_epld = i2c_new_client_device(i2c_adp_master, &i2c_dev_epld);
    if(IS_INVALID_PTR(i2c_client_epld))
    {
        i2c_client_epld = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client epld failed\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_i2c_epld(void)
{
    if(IS_VALID_PTR(i2c_client_epld)) {
        i2c_unregister_device(i2c_client_epld);
        i2c_client_epld = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:gpio")
static struct i2c_board_info i2c_dev_gpio0 = {
    I2C_BOARD_INFO("i2c-gpio0", 0x20),
};
static struct i2c_board_info i2c_dev_gpio1 = {
    I2C_BOARD_INFO("i2c-gpio1", 0x21),
};
static struct i2c_board_info i2c_dev_gpio2 = {
    I2C_BOARD_INFO("i2c-gpio2", 0x22),
};
static struct i2c_board_info i2c_dev_gpio3 = {
    I2C_BOARD_INFO("i2c-gpio3", 0x23),
};
static struct i2c_board_info i2c_dev_gpio4 = {
    I2C_BOARD_INFO("i2c-gpio4", 0x24),
};
static struct i2c_client  *i2c_client_gpio0      = NULL;
static struct i2c_client  *i2c_client_gpio1      = NULL;
static struct i2c_client  *i2c_client_gpio2      = NULL;
static struct i2c_client  *i2c_client_gpio3      = NULL;
static struct i2c_client  *i2c_client_gpio4      = NULL;

static int e582_48x6q_init_i2c_gpio(void)
{
    if (IS_INVALID_PTR(i2c_adp_master))
    {
         printk(KERN_CRIT "e582_48x6q_init_i2c_gpio can't find i2c-core bus\n");
         return -1;
    }
    
    i2c_client_gpio0 = i2c_new_client_device(i2c_adp_master, &i2c_dev_gpio0);
    if(IS_INVALID_PTR(i2c_client_gpio0))
    {
        i2c_client_gpio0 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client gpio0 failed\n");
        return -1;
    }

    i2c_client_gpio1 = i2c_new_client_device(i2c_adp_master, &i2c_dev_gpio1);
    if(IS_INVALID_PTR(i2c_client_gpio1))
    {
        i2c_client_gpio1 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client gpio1 failed\n");
        return -1;
    }

    i2c_client_gpio2 = i2c_new_client_device(i2c_adp_master, &i2c_dev_gpio2);
    if(IS_INVALID_PTR(i2c_client_gpio2))
    {
        i2c_client_gpio2 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client gpio2 failed\n");
        return -1;
    }

    i2c_client_gpio3 = i2c_new_client_device(i2c_adp_master, &i2c_dev_gpio3);
    if(IS_INVALID_PTR(i2c_client_gpio3))
    {
        i2c_client_gpio3 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client gpio3 failed\n");
        return -1;
    }

    i2c_client_gpio4 = i2c_new_client_device(i2c_adp_master, &i2c_dev_gpio4);
    if(IS_INVALID_PTR(i2c_client_gpio4))
    {
        i2c_client_gpio4 = NULL;
        printk(KERN_CRIT "create e582_48x6q board i2c client gpio4 failed\n");
        return -1;
    }    

    return 0;
}

static int e582_48x6q_exit_i2c_gpio(void)
{
    if(IS_VALID_PTR(i2c_client_gpio0)) {
        i2c_unregister_device(i2c_client_gpio0);
        i2c_client_gpio0 = NULL;
    }

    if((i2c_client_gpio1)) {
        i2c_unregister_device(i2c_client_gpio1);
        i2c_client_gpio1 = NULL;
    }

    if(IS_VALID_PTR(i2c_client_gpio2)) {
        i2c_unregister_device(i2c_client_gpio2);
        i2c_client_gpio2 = NULL;
    }

    if(IS_VALID_PTR(i2c_client_gpio3)) {
        i2c_unregister_device(i2c_client_gpio3);
        i2c_client_gpio3 = NULL;
    }

    if(IS_VALID_PTR(i2c_client_gpio4)) {
        i2c_unregister_device(i2c_client_gpio4);
        i2c_client_gpio4 = NULL;
    }
    
    return 0;
}
#endif

#if SEP("i2c:smbus")
static int e582_48x6q_smbus_read_reg(struct i2c_client *client, unsigned char reg, unsigned char* value)
{
    int ret = 0;

    if (IS_INVALID_PTR(client))
    {
        printk(KERN_CRIT "invalid i2c client");
        return -1;
    }
    
    ret = i2c_smbus_read_byte_data(client, reg);
    if (ret >= 0) {
        *value = (unsigned char)ret;
    }
    else
    {
        *value = 0;
        printk(KERN_CRIT "i2c_smbus op failed: ret=%d reg=%d\n",ret ,reg);
        return ret;
    }

    return 0;
}

static int e582_48x6q_smbus_write_reg(struct i2c_client *client, unsigned char reg, unsigned char value)
{
    int ret = 0;
    
    if (IS_INVALID_PTR(client))
    {
        printk(KERN_CRIT "invalid i2c client");
        return -1;
    }
    
    ret = i2c_smbus_write_byte_data(client, reg, value);
    if (ret != 0)
    {
        printk(KERN_CRIT "i2c_smbus op failed: ret=%d reg=%d\n",ret ,reg);
        return ret;
    }

    return 0;
}
#endif

#if SEP("drivers:psu")
static struct class* psu_class = NULL;
static struct device* psu_dev_psu1 = NULL;
static struct device* psu_dev_psu2 = NULL;

static ssize_t e582_48x6q_psu_read_presence(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;
    unsigned char present_no = 0;
    unsigned char present = 0;
    unsigned char value = 0;
    struct i2c_client *i2c_psu_client = NULL;

    if (psu_dev_psu1 == dev)
    {
        i2c_psu_client = i2c_client_gpio0;
        present_no = 17;
    }
    else if (psu_dev_psu2 == dev)
    {
        i2c_psu_client = i2c_client_gpio0;
        present_no = 9;
    }
    else
    {
        return sprintf(buf, "Error: unknown psu device\n");
    }

    if (IS_INVALID_PTR(i2c_psu_client))
    {
        return sprintf(buf, "Error: psu i2c-adapter invalid\n");
    }

    ret = e582_48x6q_smbus_read_reg(i2c_psu_client, present_no/8, &present);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read psu data:%s failed\n", attr->attr.name);
    }

    value = ((present & (1<<(present_no%8))) ? 1 : 0 );
    
    return sprintf(buf, "%d\n", value);
}

static ssize_t e582_48x6q_psu_read_status(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;
    unsigned char workstate_no = 0;
    unsigned char workstate = 0;
    unsigned char value = 0;
    struct i2c_client *i2c_psu_client = NULL;

    if (psu_dev_psu1 == dev)
    {
        i2c_psu_client = i2c_client_gpio0;
        workstate_no = 21;
    }
    else if (psu_dev_psu2 == dev)
    {
        i2c_psu_client = i2c_client_gpio0;
        workstate_no = 13;
    }
    else
    {
        return sprintf(buf, "Error: unknown psu device\n");
    }

    if (IS_INVALID_PTR(i2c_psu_client))
    {
        return sprintf(buf, "Error: psu i2c-adapter invalid\n");
    }

    ret = e582_48x6q_smbus_read_reg(i2c_psu_client, workstate_no/8, &workstate);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read psu data:%s failed\n", attr->attr.name);
    }

    if (ret != 0)
    {
        return sprintf(buf, "Error: read psu data:%s failed\n", attr->attr.name);
    }

    value = ((workstate & (1<<(workstate_no%8))) ? 1 : 0 );
    
    return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(psu_presence, S_IRUGO, e582_48x6q_psu_read_presence, NULL);
static DEVICE_ATTR(psu_status, S_IRUGO, e582_48x6q_psu_read_status, NULL);

static int e582_48x6q_init_psu(void)
{
    int ret = 0;
    
    psu_class = class_create(THIS_MODULE, "psu");
    if (IS_INVALID_PTR(psu_class))
    {
        psu_class = NULL;
        printk(KERN_CRIT "create e582_48x6q class psu failed\n");
        return -1;
    }

    psu_dev_psu1 = device_create(psu_class, NULL, MKDEV(222,0), NULL, "psu1");
    if (IS_INVALID_PTR(psu_dev_psu1))
    {
        psu_dev_psu1 = NULL;
        printk(KERN_CRIT "create e582_48x6q psu1 device failed\n");
        return -1;
    }

    psu_dev_psu2 = device_create(psu_class, NULL, MKDEV(222,1), NULL, "psu2");
    if (IS_INVALID_PTR(psu_dev_psu2))
    {
        psu_dev_psu2 = NULL;
        printk(KERN_CRIT "create e582_48x6q psu2 device failed\n");
        return -1;
    }

    ret = device_create_file(psu_dev_psu1, &dev_attr_psu_presence);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q psu1 device attr:presence failed\n");
        return -1;
    }

    ret = device_create_file(psu_dev_psu1, &dev_attr_psu_status);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q psu1 device attr:status failed\n");
        return -1;
    }

    ret = device_create_file(psu_dev_psu2, &dev_attr_psu_presence);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q psu2 device attr:presence failed\n");
        return -1;
    }

    ret = device_create_file(psu_dev_psu2, &dev_attr_psu_status);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q psu2 device attr:status failed\n");
        return -1;
    }
    
    return 0;
}

static int e582_48x6q_exit_psu(void)
{
    if (IS_VALID_PTR(psu_dev_psu1))
    {
        device_remove_file(psu_dev_psu1, &dev_attr_psu_presence);
        device_remove_file(psu_dev_psu1, &dev_attr_psu_status);
        device_destroy(psu_class, MKDEV(222,0));
    }

    if (IS_VALID_PTR(psu_dev_psu2))
    {
        device_remove_file(psu_dev_psu2, &dev_attr_psu_presence);
        device_remove_file(psu_dev_psu2, &dev_attr_psu_status);
        device_destroy(psu_class, MKDEV(222,1));
    }

    if (IS_VALID_PTR(psu_class))
    {
        class_destroy(psu_class);
        psu_class = NULL;
    }

    return 0;
}
#endif

#if SEP("drivers:leds")
extern void e582_48x6q_led_set(struct led_classdev *led_cdev, enum led_brightness set_value);
extern enum led_brightness e582_48x6q_led_get(struct led_classdev *led_cdev);
extern void e582_48x6q_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value);
extern enum led_brightness e582_48x6q_led_port_get(struct led_classdev *led_cdev);

static struct led_classdev led_dev_system = {
    .name = "system",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_idn = {
    .name = "idn",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_fan1 = {
    .name = "fan1",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_fan2 = {
    .name = "fan2",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_fan3 = {
    .name = "fan3",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_fan4 = {
    .name = "fan4",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_psu1 = {
    .name = "psu1",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_psu2 = {
    .name = "psu2",
    .brightness_set = e582_48x6q_led_set,
    .brightness_get = e582_48x6q_led_get,
};
static struct led_classdev led_dev_port[PORT_NUM] = {
{   .name = "port1",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port2",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port3",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port4",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port5",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port6",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port7",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port8",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port9",     .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port10",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port11",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port12",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port13",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port14",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port15",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port16",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port17",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port18",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port19",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port20",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port21",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port22",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port23",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port24",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port25",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port26",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port27",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port28",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port29",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port30",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port31",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port32",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port33",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port34",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port35",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port36",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port37",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port38",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port39",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port40",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port41",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port42",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port43",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port44",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port45",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port46",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port47",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port48",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port49",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port50",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port51",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port52",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port53",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,},
{   .name = "port54",    .brightness_set = e582_48x6q_led_port_set,    .brightness_get = e582_48x6q_led_port_get,}
};
static unsigned char port_led_mode[PORT_NUM] = {0};

void e582_48x6q_led_set(struct led_classdev *led_cdev, enum led_brightness set_value)
{
    int ret = 0;
    unsigned char reg = 0;
    unsigned char mask = 0;
    unsigned char shift = 0;
    unsigned char led_value = 0;
    struct i2c_client *i2c_led_client = i2c_client_epld;

    if (0 == strcmp(led_dev_system.name, led_cdev->name))
    {
        reg = 0x2;
        mask = 0x0F;
        shift = 4;
    }
    else if (0 == strcmp(led_dev_idn.name, led_cdev->name))
    {
        reg = 0x3;
        mask = 0xFE;
        shift = 0;
    }
    else if (0 == strcmp(led_dev_fan1.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan2.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan3.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan4.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_psu1.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_psu2.name, led_cdev->name))
    {
        goto not_support;
    }
    else
    {
        goto not_support;
    }

    ret = e582_48x6q_smbus_read_reg(i2c_led_client, reg, &led_value);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: read %s led attr failed\n", led_cdev->name);
        return;
    }

    led_value = ((led_value & mask) | ((set_value << shift) & (~mask)));
    
    ret = e582_48x6q_smbus_write_reg(i2c_led_client, reg, led_value);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: write %s led attr failed\n", led_cdev->name);
        return;
    }

    return;
    
not_support:

    printk(KERN_INFO "Error: not support device:%s\n", led_cdev->name);
    return;
}

enum led_brightness e582_48x6q_led_get(struct led_classdev *led_cdev)
{
    int ret = 0;
    unsigned char reg = 0;
    unsigned char mask = 0;
    unsigned char shift = 0;
    unsigned char led_value = 0;
    struct i2c_client *i2c_led_client = i2c_client_epld;

    if (0 == strcmp(led_dev_system.name, led_cdev->name))
    {
        reg = 0x2;
        mask = 0xF0;
        shift = 4;
    }
    else if (0 == strcmp(led_dev_idn.name, led_cdev->name))
    {
        reg = 0x3;
        mask = 0x01;
        shift = 0;
    }
    else if (0 == strcmp(led_dev_fan1.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan2.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan3.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_fan4.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_psu1.name, led_cdev->name))
    {
        goto not_support;
    }
    else if (0 == strcmp(led_dev_psu2.name, led_cdev->name))
    {
        goto not_support;
    }
    else
    {
        goto not_support;
    }

    ret = e582_48x6q_smbus_read_reg(i2c_led_client, reg, &led_value);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: read %s led attr failed\n", led_cdev->name);
        return 0;
    }

    led_value = ((led_value & mask) >> shift);

    return led_value;
    
not_support:

    printk(KERN_INFO "Error: not support device:%s\n", led_cdev->name);
    return 0;
}

void e582_48x6q_led_port_set(struct led_classdev *led_cdev, enum led_brightness set_value)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);
    
    port_led_mode[portNum-1] = set_value;

    return;
}

enum led_brightness e582_48x6q_led_port_get(struct led_classdev *led_cdev)
{
    int portNum = 0;
    
    sscanf(led_cdev->name, "port%d", &portNum);    
    
    return port_led_mode[portNum-1];
}

static int e582_48x6q_init_led(void)
{
    int ret = 0;
    int i = 0;

    ret = led_classdev_register(NULL, &led_dev_system);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_system device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_idn);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_idn device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_fan1);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_fan1 device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_fan2);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_fan2 device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_fan3);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_fan3 device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_fan4);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_fan4 device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_psu1);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_psu1 device failed\n");
        return -1;
    }

    ret = led_classdev_register(NULL, &led_dev_psu2);
    if (ret != 0)
    {
        printk(KERN_CRIT "create e582_48x6q led_dev_psu2 device failed\n");
        return -1;
    }

    for (i=0; i<PORT_NUM; i++)
    {
        ret = led_classdev_register(NULL, &(led_dev_port[i]));
        if (ret != 0)
        {
            printk(KERN_CRIT "create e582_48x6q led_dev_port%d device failed\n", i);
            continue;
        }
    }
    
    return ret;
}

static int e582_48x6q_exit_led(void)
{
    int i = 0;
	
    led_classdev_unregister(&led_dev_system);
    led_classdev_unregister(&led_dev_idn);
    led_classdev_unregister(&led_dev_fan1);
    led_classdev_unregister(&led_dev_fan2);
    led_classdev_unregister(&led_dev_fan3);
    led_classdev_unregister(&led_dev_fan4);
    led_classdev_unregister(&led_dev_psu1);
    led_classdev_unregister(&led_dev_psu2);

    for (i=0; i<PORT_NUM; i++)
    {
        led_classdev_unregister(&(led_dev_port[i]));
    }

    return 0;
}
#endif

#if SEP("drivers:sfp")
static struct class* sfp_class = NULL;
static struct device* sfp_dev[PORT_NUM+1] = {NULL};

static ssize_t e582_48x6q_sfp_read_presence(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;
    unsigned char value = 0;
    unsigned char reg_no = 0;
    unsigned char dir_bank = 0;
    unsigned char input_bank = 0;
    int portNum = 0;
    const char *name = dev_name(dev);
    struct i2c_client *i2c_sfp_client = NULL;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 1) || (portNum > PORT_NUM))
    {
        printk(KERN_CRIT "sfp read presence, invalid port number!\n");
        value = 0;
    }

    if ((portNum >= 1) && (portNum <= 8))
    {
        reg_no = portNum + 31;/*32-39*/
        i2c_sfp_client = i2c_client_gpio0;
    }
    else if ((portNum >= 9) && (portNum <= 16))
    {
        reg_no = portNum + 7;/*16-23*/
        i2c_sfp_client = i2c_client_gpio1;
    }
    else if ((portNum >= 17) && (portNum <= 24))
    {
        reg_no = portNum - 17;/*0-7*/
        i2c_sfp_client = i2c_client_gpio2;
    }
    else if ((portNum >= 25) && (portNum <= 32))
    {
        reg_no = portNum - 1;/*24-31*/
        i2c_sfp_client = i2c_client_gpio2;
    }
    else if ((portNum >= 33) && (portNum <= 40))
    {
        reg_no = portNum - 25;/*8-15*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 41) && (portNum <= 48))
    {
        reg_no = portNum - 9;/*32-39*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 49) && (portNum <= 54))
    {
        reg_no = portNum - 25;/*24-29*/
        i2c_sfp_client = i2c_client_gpio4;
    }

    dir_bank = (reg_no/8) + 0x18;
    ret = e582_48x6q_smbus_write_reg(i2c_sfp_client, dir_bank, 0xff);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read sfp data:%s set dir-ctl failed\n", attr->attr.name);
    }
    
    input_bank = (reg_no/8) + 0x0;
    ret = e582_48x6q_smbus_read_reg(i2c_sfp_client, input_bank, &value);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read sfp data:%s failed\n", attr->attr.name);
    }

    value = ((value & (1<<(reg_no%8))) ? 0 : 1 );/*1:PRESENT 0:ABSENT*/
    
    return sprintf(buf, "%d\n", value);
}

static ssize_t e582_48x6q_sfp_read_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;
    unsigned char value = 0;
    unsigned char reg_no = 0;
    unsigned char dir_bank = 0;
    unsigned char input_bank = 0;
    int portNum = 0;
    const char *name = dev_name(dev);
    struct i2c_client *i2c_sfp_client = NULL;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 1) || (portNum > PORT_NUM))
    {
        printk(KERN_CRIT "sfp read presence, invalid port number!\n");
        value = 0;
    }

    if ((portNum >= 1) && (portNum <= 8))
    {
        reg_no = portNum + 23;/*24-31*/
        i2c_sfp_client = i2c_client_gpio0;
    }
    else if ((portNum >= 9) && (portNum <= 16))
    {
        reg_no = portNum - 1;/*8-15*/
        i2c_sfp_client = i2c_client_gpio1;
    }
    else if ((portNum >= 17) && (portNum <= 24))
    {
        reg_no = portNum + 15;/*32-39*/
        i2c_sfp_client = i2c_client_gpio1;
    }
    else if ((portNum >= 25) && (portNum <= 32))
    {
        reg_no = portNum - 9;/*16-23*/
        i2c_sfp_client = i2c_client_gpio2;
    }
    else if ((portNum >= 33) && (portNum <= 40))
    {
        reg_no = portNum - 33;/*0-7*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 41) && (portNum <= 48))
    {
        reg_no = portNum - 17;/*24-31*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 49) && (portNum <= 54))
    {
        printk(KERN_INFO "%s not supported!\n", name);
        return sprintf(buf, "%d\n", 0);
    }

    dir_bank = (reg_no/8) + 0x18;
    ret = e582_48x6q_smbus_write_reg(i2c_sfp_client, dir_bank, 0xff);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read sfp data:%s set dir-ctl failed\n", attr->attr.name);
    }
    
    input_bank = (reg_no/8) + 0x8;
    ret = e582_48x6q_smbus_read_reg(i2c_sfp_client, input_bank, &value);
    if (ret != 0)
    {
        return sprintf(buf, "Error: read sfp data:%s failed\n", attr->attr.name);
    }

    value = ((value & (1<<(reg_no%8))) ? 0 : 1 );
    
    return sprintf(buf, "%d\n", value);
}

static ssize_t e582_48x6q_sfp_write_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
    int ret = 0;
    unsigned char value = 0;
    unsigned char set_value = simple_strtol(buf, NULL, 10);
    unsigned char reg_no = 0;
    unsigned char dir_bank = 0;
    unsigned char input_bank = 0;
    unsigned char output_bank = 0;
    int portNum = 0;
    const char *name = dev_name(dev);
    struct i2c_client *i2c_sfp_client = NULL;

    sscanf(name, "sfp%d", &portNum);

    if ((portNum < 1) || (portNum > PORT_NUM))
    {
        printk(KERN_CRIT "sfp read presence, invalid port number!\n");
        return size;
    }

    if ((portNum >= 1) && (portNum <= 8))
    {
        reg_no = portNum + 23;/*24-31*/
        i2c_sfp_client = i2c_client_gpio0;
    }
    else if ((portNum >= 9) && (portNum <= 16))
    {
        reg_no = portNum - 1;/*8-15*/
        i2c_sfp_client = i2c_client_gpio1;
    }
    else if ((portNum >= 17) && (portNum <= 24))
    {
        reg_no = portNum + 15;/*32-39*/
        i2c_sfp_client = i2c_client_gpio1;
    }
    else if ((portNum >= 25) && (portNum <= 32))
    {
        reg_no = portNum - 9;/*16-23*/
        i2c_sfp_client = i2c_client_gpio2;
    }
    else if ((portNum >= 33) && (portNum <= 40))
    {
        reg_no = portNum - 33;/*0-7*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 41) && (portNum <= 48))
    {
        reg_no = portNum - 17;/*24-31*/
        i2c_sfp_client = i2c_client_gpio3;
    }
    else if ((portNum >= 49) && (portNum <= 54))
    {
        printk(KERN_INFO "%s not supported!\n", name);
        return size;
    }

    set_value = ((set_value > 0) ? 0 : 1 );

    dir_bank = (reg_no/8) + 0x18;
    ret = e582_48x6q_smbus_write_reg(i2c_sfp_client, dir_bank, 0x0);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: read sfp data:%s set dir-ctl failed\n", attr->attr.name);
        return size;
    }
    
    input_bank = (reg_no/8) + 0x8;
    ret = e582_48x6q_smbus_read_reg(i2c_sfp_client, input_bank, &value);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: read %s presence failed\n", name);
        return size;
    }

    if (set_value)
    {
        value = (value | (1<<(reg_no % 8)));
    }
    else
    {
        value = (value & (~(1<<(reg_no % 8))));
    }
    
    output_bank = (reg_no/8) + 0x8;
    ret = e582_48x6q_smbus_write_reg(i2c_sfp_client, output_bank, value);
    if (ret != 0)
    {
        printk(KERN_CRIT "Error: write %s presence failed\n", name);
        return size;
    }
    
    return size;
}

static DEVICE_ATTR(sfp_presence, S_IRUGO, e582_48x6q_sfp_read_presence, NULL);
static DEVICE_ATTR(sfp_enable, S_IRUGO|S_IWUSR, e582_48x6q_sfp_read_enable, e582_48x6q_sfp_write_enable);
static int e582_48x6q_init_sfp(void)
{
    int ret = 0;
    int i = 0;
    
    sfp_class = class_create(THIS_MODULE, "sfp");
    if (IS_INVALID_PTR(sfp_class))
    {
        sfp_class = NULL;
        printk(KERN_CRIT "create e582_48x6q class sfp failed\n");
        return -1;
    }

    for (i=1; i<=PORT_NUM; i++)
    {
        sfp_dev[i] = device_create(sfp_class, NULL, MKDEV(223,i), NULL, "sfp%d", i);
        if (IS_INVALID_PTR(sfp_dev[i]))
        {
            sfp_dev[i] = NULL;
            printk(KERN_CRIT "create e582_48x6q sfp[%d] device failed\n", i);
            continue;
        }

        ret = device_create_file(sfp_dev[i], &dev_attr_sfp_presence);
        if (ret != 0)
        {
            printk(KERN_CRIT "create e582_48x6q sfp[%d] device attr:presence failed\n", i);
            continue;
        }

        ret = device_create_file(sfp_dev[i], &dev_attr_sfp_enable);
        if (ret != 0)
        {
            printk(KERN_CRIT "create e582_48x6q sfp[%d] device attr:enable failed\n", i);
            continue;
        }
    }
    
    return ret;
}

static int e582_48x6q_exit_sfp(void)
{
    int i = 0;

    for (i=1; i<=PORT_NUM; i++)
    {
        if (IS_VALID_PTR(sfp_dev[i]))
        {
            device_remove_file(sfp_dev[i], &dev_attr_sfp_presence);
            device_remove_file(sfp_dev[i], &dev_attr_sfp_enable);
            device_destroy(sfp_class, MKDEV(223,i));
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

static int e582_48x6q_init(void)
{
    int ret = 0;
    int failed = 0;
    
    printk(KERN_ALERT "install e582_48x6q board dirver...\n");
    
    ret = e582_48x6q_init_i2c_master();
    if (ret != 0)
    {
        failed = 1;
    }
    
    ret = e582_48x6q_init_i2c_pca9548();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_i2c_adt7470();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_i2c_psu();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_i2c_epld();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_i2c_gpio();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_psu();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_led();
    if (ret != 0)
    {
        failed = 1;
    }

    ret = e582_48x6q_init_sfp();
    if (ret != 0)
    {
        failed = 1;
    }

    if (failed)
        printk(KERN_INFO "install e582_48x6q board driver failed\n");
    else
        printk(KERN_ALERT "install e582_48x6q board dirver...ok\n");
    
    return 0;
}

static void e582_48x6q_exit(void)
{
    printk(KERN_INFO "uninstall e582_48x6q board dirver...\n");
    
    e582_48x6q_exit_sfp();
    e582_48x6q_exit_led();
    e582_48x6q_exit_psu();
    e582_48x6q_exit_i2c_gpio();
    e582_48x6q_exit_i2c_epld();
    e582_48x6q_exit_i2c_psu();
    e582_48x6q_exit_i2c_adt7470();
    e582_48x6q_exit_i2c_pca9548();
    e582_48x6q_exit_i2c_master();
}

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("yangbs centecNetworks, Inc");
MODULE_DESCRIPTION("e582-48x6q board driver");
module_init(e582_48x6q_init);
module_exit(e582_48x6q_exit);

