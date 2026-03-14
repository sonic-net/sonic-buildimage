#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
// #include <linux/platform_data/pca954x.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio/machine.h>

// #define GPIO_BASE   0 // in kernel 4.x GPIO_BASE =0 ; in kernel 3.16.x , GPIO_BASE=180
struct net_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
    int probe;
};

#define bus_id(id)  (id)
#define SCL_PIN  58
#define SDA_PIN  75

struct pca954x_platform_mode {
        int             adap_id;
        unsigned int    deselect_on_exit:1;
        unsigned int    class;
};

/* Per mux/switch data, used with i2c_register_board_info */
struct pca954x_platform_data {
        struct pca954x_platform_mode *modes;
        int num_modes;
};

static struct pca954x_platform_mode mux_modes_0[] = {
    {.adap_id = bus_id(2),},    {.adap_id = bus_id(3),},
    {.adap_id = bus_id(4),},    {.adap_id = bus_id(5),},
    {.adap_id = bus_id(6),},    {.adap_id = bus_id(7),},
    {.adap_id = bus_id(8),},    {.adap_id = bus_id(9),},
};
static struct pca954x_platform_mode mux_modes_0_0[] = {
    {.adap_id = bus_id(10),},    {.adap_id = bus_id(11),},
    {.adap_id = bus_id(12),},    {.adap_id = bus_id(13),},
    {.adap_id = bus_id(14),},    {.adap_id = bus_id(15),},
    {.adap_id = bus_id(16),},    {.adap_id = bus_id(17),},
};

static struct pca954x_platform_mode mux_modes_0_1[] = {
    {.adap_id = bus_id(18),},    {.adap_id = bus_id(19),},
    {.adap_id = bus_id(20),},    {.adap_id = bus_id(21),},
    {.adap_id = bus_id(22),},    {.adap_id = bus_id(23),},
    {.adap_id = bus_id(24),},    {.adap_id = bus_id(25),},
};

static struct pca954x_platform_mode mux_modes_0_2[] = {
    {.adap_id = bus_id(26),},    {.adap_id = bus_id(27),},
    {.adap_id = bus_id(28),},    {.adap_id = bus_id(29),},
    {.adap_id = bus_id(30),},    {.adap_id = bus_id(31),},
    {.adap_id = bus_id(32),},    {.adap_id = bus_id(33),},
};

static struct pca954x_platform_mode mux_modes_0_3[] = {
    {.adap_id = bus_id(34),},    {.adap_id = bus_id(35),},
    {.adap_id = bus_id(36),},    {.adap_id = bus_id(37),},
    {.adap_id = bus_id(38),},    {.adap_id = bus_id(39),},
    {.adap_id = bus_id(40),},    {.adap_id = bus_id(41),},
};

static struct pca954x_platform_mode mux_modes_0_4[] = {
    {.adap_id = bus_id(42),},    {.adap_id = bus_id(43),},
    {.adap_id = bus_id(44),},    {.adap_id = bus_id(45),},
    {.adap_id = bus_id(46),},    {.adap_id = bus_id(47),},
    {.adap_id = bus_id(48),},    {.adap_id = bus_id(49),},
};

static struct pca954x_platform_mode mux_modes_0_5[] = {
    {.adap_id = bus_id(50),},    {.adap_id = bus_id(51),},
    {.adap_id = bus_id(52),},    {.adap_id = bus_id(53),},
    {.adap_id = bus_id(54),},    {.adap_id = bus_id(55),},
    {.adap_id = bus_id(56),},    {.adap_id = bus_id(57),},
};

static struct pca954x_platform_mode mux_modes_0_6[] = {
    {.adap_id = bus_id(58),},    {.adap_id = bus_id(59),},
    {.adap_id = bus_id(60),},    {.adap_id = bus_id(61),},
    {.adap_id = bus_id(62),},    {.adap_id = bus_id(63),},
    {.adap_id = bus_id(64),},    {.adap_id = bus_id(65),},
};

//no i2c device driver attach to mux 7


static struct pca954x_platform_data mux_data_0 = {
        .modes          = mux_modes_0,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_0 = {
        .modes          = mux_modes_0_0,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_1 = {
        .modes          = mux_modes_0_1,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_2 = {
        .modes          = mux_modes_0_2,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_3 = {
        .modes          = mux_modes_0_3,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_4 = {
        .modes          = mux_modes_0_4,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_5 = {
        .modes          = mux_modes_0_5,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_6 = {
        .modes          = mux_modes_0_6,
        .num_modes      = 8,
};

static struct i2c_board_info i2c_device_info0[] __initdata = {
//        {"net_cpld",         0, 0x77, 0, 0, 0},//cpld
        {"pca9548",          0, 0x70, "pca9548-1", (void *)&mux_data_0, 0, 0},	
};

static struct i2c_board_info i2c_device_info2[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-2", (void *)&mux_data_0_0, 0, 0},	
};
static struct i2c_board_info i2c_device_info3[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-3", (void *)&mux_data_0_1, 0, 0},	
};
static struct i2c_board_info i2c_device_info4[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-4", (void *)&mux_data_0_2, 0, 0},	
};
static struct i2c_board_info i2c_device_info5[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-5", (void *)&mux_data_0_3, 0, 0},	
};
static struct i2c_board_info i2c_device_info6[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-6", (void *)&mux_data_0_4, 0, 0},	
};
static struct i2c_board_info i2c_device_info7[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-7", (void *)&mux_data_0_5, 0, 0},	
};
static struct i2c_board_info i2c_device_info8[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-8", (void *)&mux_data_0_6, 0, 0},	
};


static struct net_i2c_board_info i2cdev_list[] = {
    {bus_id(0), ARRAY_SIZE(i2c_device_info0),  i2c_device_info0, 1},  //mux root
    {bus_id(1), ARRAY_SIZE(i2c_device_info0),  i2c_device_info0, 1},  //mux root
    {bus_id(2), ARRAY_SIZE(i2c_device_info2),  i2c_device_info2, 0},  //mux 0
    {bus_id(3), ARRAY_SIZE(i2c_device_info3),  i2c_device_info3, 0},  //mux 1
    {bus_id(4), ARRAY_SIZE(i2c_device_info4),  i2c_device_info4, 0},  //mux 2
    {bus_id(5), ARRAY_SIZE(i2c_device_info5),  i2c_device_info5, 0},  //mux 3
    {bus_id(6), ARRAY_SIZE(i2c_device_info6),  i2c_device_info6, 0},  //mux 4
    {bus_id(7), ARRAY_SIZE(i2c_device_info7),  i2c_device_info7, 0},  //mux 5
    {bus_id(8), ARRAY_SIZE(i2c_device_info8),  i2c_device_info8, 0},  //mux 6  
};

static struct gpiod_lookup_table net_i2c_gpiod_table = {
    .dev_id = "i2c-gpio.1",
    .table = {
        GPIO_LOOKUP_IDX("gpio_ich", SDA_PIN, NULL, 0, GPIO_OPEN_DRAIN), //I2C_SDA
        GPIO_LOOKUP_IDX("gpio_ich", SCL_PIN, NULL, 1, GPIO_OPEN_DRAIN), //I2C_SCL
    },
};

#define NET_PLATFORM_CLIENT_MAX_NUM 100 /*A big enough number for sum of i2cdev_list[i].size */
static int client_list_index = 0;
static struct i2c_client *client_list[NET_PLATFORM_CLIENT_MAX_NUM] = {0};

static void i2cgpio_device_release(struct device *dev)
{
    gpiod_remove_lookup_table(&net_i2c_gpiod_table);
}

static struct 	i2c_gpio_platform_data 	i2c_platform_data = {
    .udelay  = 5, //5:100kHz
//    .sda_is_open_drain = 0,
//    .scl_is_open_drain = 0,
//    .scl_is_output_only = 0
};

static struct platform_device device_i2c_gpio0 = {
        .name = "i2c-gpio",
        .id = 1,
        .dev = {
                .platform_data = &i2c_platform_data,
                .release = i2cgpio_device_release,
        },
};


static int __init net_platform_init(void)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *e = NULL;
    int ret = 0;
    int i,j,k;

    //printk("%s  \n", __func__);

    //use i2c-gpio    
    //register i2c gpio
    //config gpio58,75 to gpio function 58=32+3*8+2 75=32*2+8*1+3 gpio69=32*2+8*0+5
    outl( inl(0x533) | (1<<2), 0x533);  //i2c-gpio sdl (GPIO58)
    outl( inl(0x541) | (1<<3), 0x541);  //i2c-gpio sda (GPIO75)
    outl( inl(0x540) | (1<<5), 0x540);  //RST_I2C_MUX_N (GPIO69)
    outl( inl(0x500) | (1<<7), 0x500);  //SYS_RDY_N (GPIO7)
    outl( inl(0x501) | (1<<7), 0x501);  //BMC_HEART_BEAT (GPIO15)
    outl( inl(0x503) | (1<<2)|(1<<3), 0x503); //PSOC_HEART_BEAT(26),CPLD_HEART_BEAT(27)
   
    gpiod_add_lookup_table(&net_i2c_gpiod_table);

    ret = platform_device_register(&device_i2c_gpio0);
    if (ret) {
        printk(KERN_ERR "i2c-gpio: platform_device_register fail %d\n", ret);
    }
    msleep(10);

    for(i=0; i<ARRAY_SIZE(i2cdev_list); i++) {
        adap = i2c_get_adapter( i2cdev_list[i].ch );
        if (adap == NULL) {
            printk("platform get channel %d adapter fail\n", i);
            continue;
        }
        // i2c_put_adapter(adap);
        for(j=0; j<i2cdev_list[i].size; j++) {
            for(k=0; k<300; k++) {
                if (i2cdev_list[i].probe == 1) {
                    short unsigned int i2c_address[2]={i2cdev_list[i].board_info[j].addr, I2C_CLIENT_END};
                    e = i2c_new_scanned_device(adap, &i2cdev_list[i].board_info[j] ,i2c_address, NULL);
                } else {
                    e = i2c_new_client_device(adap, &i2cdev_list[i].board_info[j]);
                }

                if(e == NULL) {
                    msleep(10);
                } else {
                    client_list[client_list_index] = e;
                    client_list_index++;
                    break;
                }
            }
            if(k==300) {
                printk("[%d][%d] i2c device load fail\n",i,j);
            }
        }
        i2c_put_adapter(adap);
    }

    printk("net_platform module initialized\n");
    return ret;    
}

static void __exit net_platform_exit(void)
{
    int i;

    for(i=client_list_index-1; i>=0; i--) {
        i2c_unregister_device(client_list[i]);
    }

    device_i2c_gpio0.dev.platform_data = NULL;
    platform_device_unregister(&device_i2c_gpio0);
    printk("net_platform_exit done\n");
}

module_init(net_platform_init);
module_exit(net_platform_exit);

MODULE_AUTHOR("Netberg <support@netbergtw.com>");
MODULE_DESCRIPTION("Netberg Platform devices");
MODULE_LICENSE("GPL");
