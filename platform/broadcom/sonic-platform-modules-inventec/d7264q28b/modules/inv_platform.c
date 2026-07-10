#include <linux/i2c.h>
//#include <linux/i2c-algo-bit.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/platform_data/pca954x.h>
//#include <linux/i2c/pca953x.h>
//#include <linux/i2c/at24.h>

//#include <asm/gpio.h>
//#define IO_EXPAND_BASE    64
//#define IO_EXPAND_NGPIO   16

struct inv_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
};

#define bus_id(id)  (id)
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

static struct pca954x_platform_mode mux_modes_0_7[] = {
    {.adap_id = bus_id(66),},    {.adap_id = bus_id(67),},
    {.adap_id = bus_id(68),},    {.adap_id = bus_id(69),},
    {.adap_id = bus_id(70),},    {.adap_id = bus_id(71),},
    {.adap_id = bus_id(72),},    {.adap_id = bus_id(73),},
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
static struct pca954x_platform_data mux_data_0_7 = {
        .modes          = mux_modes_0_7,
        .num_modes      = 8,
};

static struct i2c_board_info i2c_device_info0[] __initdata = {
//        {"inv_psoc",         0, 0x66, 0, 0, 0},//psoc
        {"inv_cpld",         0, 0x55, 0, 0, 0},//cpld
        {"inv_cpld2",        0, 0x77, 0, 0, 0},//cpld2
        {"pca9548",          0, 0x70, &mux_data_0, 0, 0},	
};

static struct i2c_board_info i2c_device_info2[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_0, 0, 0},	
};
static struct i2c_board_info i2c_device_info3[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_1, 0, 0},	
};
static struct i2c_board_info i2c_device_info4[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_2, 0, 0},	
};
static struct i2c_board_info i2c_device_info5[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_3, 0, 0},	
};
static struct i2c_board_info i2c_device_info6[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_4, 0, 0},	
};
static struct i2c_board_info i2c_device_info7[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_5, 0, 0},	
};
static struct i2c_board_info i2c_device_info8[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_6, 0, 0},	
};
static struct i2c_board_info i2c_device_info9[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_7, 0, 0},	
};


static struct inv_i2c_board_info i2cdev_list[] = {
    {0, ARRAY_SIZE(i2c_device_info0),  i2c_device_info0 },  //smbus 0
    
    {bus_id(2), ARRAY_SIZE(i2c_device_info2),  i2c_device_info2 },  //mux 0
    {bus_id(3), ARRAY_SIZE(i2c_device_info3),  i2c_device_info3 },  //mux 1
    {bus_id(4), ARRAY_SIZE(i2c_device_info4),  i2c_device_info4 },  //mux 2
    {bus_id(5), ARRAY_SIZE(i2c_device_info5),  i2c_device_info5 },  //mux 3
    {bus_id(6), ARRAY_SIZE(i2c_device_info6),  i2c_device_info6 },  //mux 4
    {bus_id(7), ARRAY_SIZE(i2c_device_info7),  i2c_device_info7 },  //mux 5
    {bus_id(8), ARRAY_SIZE(i2c_device_info8),  i2c_device_info8 },  //mux 6
    {bus_id(9), ARRAY_SIZE(i2c_device_info9),  i2c_device_info9 },  //mux 7
    
};

/////////////////////////////////////////////////////////////////////////////////////////
#if 0
static struct 	i2c_gpio_platform_data 	i2c_gpio_platdata0 = {
	.scl_pin = 58,
	.sda_pin = 75,
    
	.udelay  = 5, //5:100kHz
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.scl_is_output_only = 0
};

static struct 	platform_device 	device_i2c_gpio0 = {
	.name 	= "i2c-gpio",
	.id  	= 1, // adapter number
	.dev.platform_data = &i2c_gpio_platdata0,
};
#endif
static int __init inv_platform_init(void)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *e = NULL;
    int ret = 0;
    int i,j,k;

    printk("%s  \n", __func__);

#if 0
    //use i2c-gpio    
    //register i2c gpio
    //config gpio58,75 to gpio function 58=32+3*8+2 75=32*2+8*1+3 gpio69=32*2+8*0+5
    outl( inl(0x533) | (1<<2), 0x533);
    outl( inl(0x541) | (1<<3), 0x541);
    outl( inl(0x540) | (1<<5), 0x540);  //RST_I2C_MUX_N (GPIO69)
    outl( inl(0x500) | (1<<7), 0x500);  //SYS_RDY_N (GPIO7)
    outl( inl(0x501) | (1<<7), 0x501);  //BMC_HEART_BEAT (GPIO15)
    outl( inl(0x503) | (1<<2)|(1<<3), 0x503); //PSOC_HEART_BEAT(26),CPLD_HEART_BEAT(27)
   
	ret = platform_device_register(&device_i2c_gpio0);
	if (ret) {
		printk(KERN_ERR "i2c-gpio: device_i2c_gpio0 register fail %d\n", ret);
	}
#endif 
   for(i=0; i<ARRAY_SIZE(i2cdev_list); i++) {
        adap = i2c_get_adapter( i2cdev_list[i].ch );
        if (adap == NULL) {
                printk("sequoia get channel %d adapter fail\n", i);
                continue;
        }
        i2c_put_adapter(adap);
        for(j=0; j<i2cdev_list[i].size; j++) {
                for(k=0; k<300; k++) {
                        e = i2c_new_device(adap, &i2cdev_list[i].board_info[j] );
                        if(e == NULL) msleep(10); else break;
                }
        }
    }

    return ret;    
}

module_init(inv_platform_init);
//arch_initcall(inv_platform_init);

MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("Sequoia Platform devices");
MODULE_LICENSE("GPL");
