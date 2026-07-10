#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/platform_data/pca954x.h>
#include <linux/platform_data/at24.h>
#include <linux/gpio/machine.h>

struct inv_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
    int probe;
};

#define bus_id(id)  (id)
#define SCL_PIN  58
#define SDA_PIN  75

static struct pca954x_platform_mode mux_modes_0[] = {
    {.adap_id = bus_id(2),},    {.adap_id = bus_id(3),},
};

static struct pca954x_platform_mode mux_modes_1[] = {
    {.adap_id = bus_id(4),},    {.adap_id = bus_id(5),},
    {.adap_id = bus_id(6),},    {.adap_id = bus_id(7),},
    {.adap_id = bus_id(8),},    {.adap_id = bus_id(9),},
    {.adap_id = bus_id(10),},   {.adap_id = bus_id(11),},
};

static struct pca954x_platform_mode mux_modes_2_0[] = {
    {.adap_id = bus_id(12),},    {.adap_id = bus_id(13),},
    {.adap_id = bus_id(14),},    {.adap_id = bus_id(15),},
    {.adap_id = bus_id(16),},    {.adap_id = bus_id(17),},
    {.adap_id = bus_id(18),},    {.adap_id = bus_id(19),},
};

static struct pca954x_platform_mode mux_modes_2_1[] = {
    {.adap_id = bus_id(20),},    {.adap_id = bus_id(21),},
    {.adap_id = bus_id(22),},    {.adap_id = bus_id(23),},
    {.adap_id = bus_id(24),},    {.adap_id = bus_id(25),},
    {.adap_id = bus_id(26),},    {.adap_id = bus_id(27),},
};

static struct pca954x_platform_mode mux_modes_2_2[] = {
    {.adap_id = bus_id(28),},    {.adap_id = bus_id(29),},
    {.adap_id = bus_id(30),},    {.adap_id = bus_id(31),},
    {.adap_id = bus_id(32),},    {.adap_id = bus_id(33),},
    {.adap_id = bus_id(34),},    {.adap_id = bus_id(35),},
};

static struct pca954x_platform_mode mux_modes_2_3[] = {
    {.adap_id = bus_id(36),},    {.adap_id = bus_id(37),},
    {.adap_id = bus_id(38),},    {.adap_id = bus_id(39),},
    {.adap_id = bus_id(40),},    {.adap_id = bus_id(41),},
    {.adap_id = bus_id(42),},    {.adap_id = bus_id(43),},
};


static struct pca954x_platform_data mux_data_0 = {
        .modes          = mux_modes_0,
        .num_modes      = 2,
};

static struct pca954x_platform_data mux_data_1 = {
        .modes          = mux_modes_1,
        .num_modes      = 8,
};

static struct pca954x_platform_data mux_data_2_0 = {
        .modes          = mux_modes_2_0,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_2_1 = {
        .modes          = mux_modes_2_1,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_2_2 = {
        .modes          = mux_modes_2_2,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_2_3 = {
        .modes          = mux_modes_2_3,
        .num_modes      = 8,
};

static struct i2c_board_info i2c_device_info0[] __initdata = {
        {"inv_cpld",         0, 0x77, "inv_cpld",  0, 0, 0},			// CPLD driver
//	{"24c512",	     0, 0x55, 0, 0, 0},			// OEM1 VPD
//	{"24c512",           0, 0x54, 0, 0, 0},                 // OEM2 VPD
        {"pca9543",          0, 0x73, "pca9543",  &mux_data_0, 0, 0},       // MUX
};

static struct i2c_board_info i2c_device_info1[] __initdata = {
        {"pca9548",          0, 0x70, "pca9548-1",  &mux_data_1, 0, 0},
};

static struct i2c_board_info i2c_device_info2[] __initdata ={
        {"ucd90160",         0, 0x34, "ucd90160",  0, 0 ,0},
};

static struct i2c_board_info i2c_device_info11[] __initdata ={
        {"pmbus",            0, 0x5A, "pmbus-1",  0, 0 ,0},         //PSU1 DVT
        {"pmbus",            0, 0x5B, "pmbus-2",  0, 0 ,0},         //PSU2 DVT
};

static struct i2c_board_info i2c_device_info3[] __initdata ={
	    {"tmp75",            0, 0x48, "tmp75-1",  0, 0 ,0},		//CPU Board Temperature
        {"tmp75",            0, 0x4A, "tmp75-2",  0, 0 ,0},		//Front Panel Inlet Temperature
        {"tmp75",            0, 0x4D, "tmp75-3",  0, 0 ,0},
        {"tmp75",             0, 0x4E, "tmp75-4",  0, 0 ,0},		//Near ASIC Temperature
};

static struct i2c_board_info i2c_device_info4[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-2",  &mux_data_2_0, 0, 0},
};
static struct i2c_board_info i2c_device_info5[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-3",  &mux_data_2_1, 0, 0},
};
static struct i2c_board_info i2c_device_info6[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-4",  &mux_data_2_2, 0, 0},
};
static struct i2c_board_info i2c_device_info7[] __initdata = {
        {"pca9548",         0, 0x72, "pca9548-5",  &mux_data_2_3, 0, 0},
};

static struct inv_i2c_board_info i2cdev_list[] = {
    {bus_id(0),  ARRAY_SIZE(i2c_device_info0),  i2c_device_info0 , 0},  //i2c-0 smbus
    {bus_id(0),  ARRAY_SIZE(i2c_device_info1),  i2c_device_info1 , 1},  //i2c-0 smbus    (for DVT)
    {bus_id(1),  ARRAY_SIZE(i2c_device_info1),  i2c_device_info1 , 1},  //i2c-1 i2c-gpio (for EVT)
    {bus_id(3),  ARRAY_SIZE(i2c_device_info3),  i2c_device_info3 , 0},  //mux 0x73 channel 1
    {bus_id(2),  ARRAY_SIZE(i2c_device_info2),  i2c_device_info2 , 0},  //mux 0x73 channel 0
    {bus_id(2),  ARRAY_SIZE(i2c_device_info11), i2c_device_info11, 1},  //mux 0x73 channel 0
    {bus_id(4),  ARRAY_SIZE(i2c_device_info4),  i2c_device_info4 , 1},  //mux 0x70 channel 0
    {bus_id(5),  ARRAY_SIZE(i2c_device_info5),  i2c_device_info5 , 1},  //mux 0x70 channel 1
    {bus_id(6),  ARRAY_SIZE(i2c_device_info6),  i2c_device_info6 , 1},  //mux 0x70 channel 2
    {bus_id(7),  ARRAY_SIZE(i2c_device_info7),  i2c_device_info7 , 1},  //mux 0x70 channel 3
};

static struct gpiod_lookup_table inv_i2c_gpiod_table = {
    .dev_id = "i2c-gpio.1",
    .table = {
        GPIO_LOOKUP_IDX("gpio_ich", SDA_PIN, NULL, 0, GPIO_OPEN_DRAIN), //I2C_SDA
        GPIO_LOOKUP_IDX("gpio_ich", SCL_PIN, NULL, 1, GPIO_OPEN_DRAIN), //I2C_SCL
    },
};

static struct i2c_gpio_platform_data i2c_platform_data = {
    .udelay  = 5, //5:100kHz
    //.sda_is_open_drain = 0,
    //.scl_is_open_drain = 0,
    //.scl_is_output_only = 0,
};

static void i2cgpio_device_release(struct device *dev)
{
    gpiod_remove_lookup_table(&inv_i2c_gpiod_table);
}

static struct platform_device device_i2c_gpio0 = {
	.name = "i2c-gpio",
	.id = 1,
	.dev = {
		.platform_data = &i2c_platform_data,
		.release = i2cgpio_device_release,
	},
};

static int __init inv_platform_init(void)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *e = NULL;
    int ret = 0;
    int i,j,k;

    //use i2c-gpio
    //register gpio
    outl( inl(0x533) | (1<<2), 0x533);            //i2c-gpio sdl(58)
    outl( inl(0x541) | (1<<3), 0x541);            //i2c gpio sda(75)
    outl( inl(0x540) | (1<<5), 0x540);            //RST_I2C_MUX_N(69)
    outl( inl(0x503) | (1)|(1<<2)|(1<<3), 0x503); //WDT_IRQ_N(24) PSOC_HEART_BEAT(26) CPLD_HEART_BEAT(27)
    outl( inl(0x501) | (1<<4), 0x501);		  //RSTBTN_IN_N(12)
    outl( inl(0x533) | (1<<5), 0x533);		  //RST_BTN_5S_N(61)

    gpiod_add_lookup_table(&inv_i2c_gpiod_table);

    ret = platform_device_register(&device_i2c_gpio0);
    if (ret) {
        printk(KERN_ERR "i2c-gpio: platform_device_register fail %d\n", ret);
    }
    msleep(10);

    for(i=0; i<ARRAY_SIZE(i2cdev_list); i++) {
        adap = i2c_get_adapter( i2cdev_list[i].ch );
        if (adap == NULL) {
                continue;
        }
        i2c_put_adapter(adap);
        for(j=0; j<i2cdev_list[i].size; j++) {
                for(k=0; k<3; k++) {
			if (i2cdev_list[i].probe == 1) {
				short unsigned int i2c_address[2]={i2cdev_list[i].board_info[j].addr, I2C_CLIENT_END};
				e = i2c_new_probed_device(adap, &i2cdev_list[i].board_info[j] ,i2c_address, NULL);
			} else {
				e = i2c_new_device(adap, &i2cdev_list[i].board_info[j]);
			}
                        if(e == NULL) msleep(10); else break;
                }
        }
    }

    return ret;
}

static void __exit inv_platform_exit(void)
{
        platform_device_unregister(&device_i2c_gpio0);
}

module_init(inv_platform_init);
module_exit(inv_platform_exit);

MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("Switch Platform devices");
MODULE_LICENSE("GPL");
