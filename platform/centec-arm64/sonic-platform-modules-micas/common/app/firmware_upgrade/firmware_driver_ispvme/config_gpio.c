/*
 * config_gpio.c
 * Original Author : support <support> 2013-10-25
 *
 * gpio cpld driver
 *
 * History
 *    v1.0    support <support> 2013-10-25  Initial version.
 *
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <firmware_ispvme.h>
#include <firmware_cpld_ispvme.h>
#include "config_gpio.h"

/* extern void cmic_gpio_set_output_value(int gpio, int val); */
firmware_device_info_t *current_info = NULL;
static firmware_device_info_t set_gpio_info;
static int set_gpio_info_flag = 0;
firmware_device_info_t default_cpld_info = {
    .type      = 0,
    .tdi       = JTAG_TDI,
    .tck       = JTAG_TCK,
    .tms       = JTAG_TMS,
    .tdo       = JTAG_TDO,
    .jtag_en   = JTAG_EN,
    .select    = -1,
    .cmic_start_gpio = -1,
    .cmic_end_gpio = -1,
};

static int firmware_cpld_gpio_is_from_cmic(int gpio)
{
    if (current_info == NULL) {
        return -1;
    }

    dev_debug(firmware_debug(), "gpio %d current_info.cmic_start_gpio %d current_info.cmic_end_gpio %d.\n",
        gpio, current_info->cmic_start_gpio, current_info->cmic_end_gpio);

    if ((current_info->cmic_start_gpio == -1) || (current_info->cmic_end_gpio == -1)) {
        return 0;
    }

    if ((gpio >= current_info->cmic_start_gpio) && (gpio <= current_info->cmic_end_gpio)) {
        return 1;
    }

    return 0;
}

static int firmware_cpld_gpio_get_value(int gpio)
{
#if 0
    int ret;

    ret = firmware_cpld_gpio_is_from_cmic(gpio);
    if (ret < 0) {
        dev_debug(firmware_debug(), "firmware_cpld_gpio_is_from_cmic gpio %d failed ret %d.\n",
            gpio, ret);
        return -1;
    }
    
    if (ret == 1) {
        /* Not currently supported */
        dev_debug(firmware_debug(), "gpio %d not support to get value.\n", gpio);
        return -1;
    } else {
        return __gpio_get_value(gpio);
    }
#endif

    return __gpio_get_value(gpio);
}

static void firmware_cpld_gpio_set_output_value(int gpio, int val)
{
#if 0
    int ret;

    ret = firmware_cpld_gpio_is_from_cmic(gpio);
    if (ret < 0) {
        dev_debug(firmware_debug(), "firmware_cpld_gpio_is_from_cmic gpio %d failed ret %d.\n",
            gpio, ret);
        return;
    }
    
    if (ret == 1) {
        __gpio_set_value(gpio, val);
        /*cmic_gpio_set_output_value(gpio, val);*/
    } else {
        __gpio_set_value(gpio, val);
    }
#endif

    __gpio_set_value(gpio, val);
}

static void firmware_cpld_gpio_set_direction(int gpio, int out)
{
    if (out) {
        gpio_direction_output(gpio, 1);
    } else {
        gpio_direction_input(gpio);
    }
    
    return;
}

static void firmware_cpld_gpio_request(int gpio, char *name)
{
    int ret;
    
    ret = firmware_cpld_gpio_is_from_cmic(gpio);
    if (ret < 0) {
        dev_debug(firmware_debug(), "firmware_cpld_gpio_is_from_cmic gpio %d failed ret %d.\n",
            gpio, ret);
        return;
    }

    if (ret == 1) {
        /* do nothing */
    } else {
        gpio_request(gpio, name);
    }
        
    return;
}

static void firmware_cpld_gpio_free(int gpio)
{
    int ret;
    
    ret = firmware_cpld_gpio_is_from_cmic(gpio);
    if (ret < 0) {
        dev_debug(firmware_debug(), "firmware_cpld_gpio_is_from_cmic gpio %d failed ret %d.\n",
            gpio, ret);
        return;
    }

    if (ret == 1) {
        /* do nothing */
    } else {
        gpio_free(gpio);
    }
        
    return;
}

/* CPLD upgrade initialization operation */
static int init_cpld(void)
{
    if (current_info == NULL) {
        return -1;
    }

    firmware_cpld_gpio_request(current_info->tdi, "cpld_upgrade");
    firmware_cpld_gpio_request(current_info->tck, "cpld_upgrade");
    firmware_cpld_gpio_request(current_info->tms, "cpld_upgrade");
    firmware_cpld_gpio_request(current_info->jtag_en, "cpld_upgrade");
    if (current_info->select >= 0) {
        firmware_cpld_gpio_request(current_info->select, "cpld_upgrade");
    }   
    firmware_cpld_gpio_request(current_info->tdo, "cpld_upgrade");
    if (current_info->jtag_4.pin >= 0){
        firmware_cpld_gpio_request(current_info->jtag_1.pin, "cpld_upgrade");
        firmware_cpld_gpio_request(current_info->jtag_2.pin, "cpld_upgrade");
        firmware_cpld_gpio_request(current_info->jtag_3.pin, "cpld_upgrade");
        firmware_cpld_gpio_request(current_info->jtag_4.pin, "cpld_upgrade");
        firmware_cpld_gpio_request(current_info->jtag_5.pin, "cpld_upgrade");
    }
    return 0;
}

/* CPLD upgrade completion operation */
static int finish_cpld(void)
{
    if (current_info == NULL) {
        return -1;
    }

    firmware_cpld_gpio_set_output_value(current_info->jtag_en, 0);	
    if (current_info->select >= 0) {
        firmware_cpld_gpio_set_output_value(current_info->select, 0);
    }
    if (current_info->jtag_4.pin >= 0) {
        gpio_direction_input(current_info->jtag_4.pin);
        gpio_direction_input(current_info->jtag_1.pin);
        gpio_direction_input(current_info->jtag_2.pin);
        gpio_direction_input(current_info->jtag_3.pin);
        gpio_direction_input(current_info->jtag_5.pin);
    }
    firmware_cpld_gpio_free(current_info->tdi);
    firmware_cpld_gpio_free(current_info->tck);
    firmware_cpld_gpio_free(current_info->tms);
    firmware_cpld_gpio_free(current_info->jtag_en);
    firmware_cpld_gpio_free(current_info->tdo);
    if (current_info->jtag_4.pin >= 0) {
        firmware_cpld_gpio_free(current_info->jtag_1.pin);
        firmware_cpld_gpio_free(current_info->jtag_2.pin);
        firmware_cpld_gpio_free(current_info->jtag_3.pin);
        firmware_cpld_gpio_free(current_info->jtag_4.pin);
        firmware_cpld_gpio_free(current_info->jtag_5.pin);
    }

    if (current_info->select >= 0) {
        firmware_cpld_gpio_free(current_info->select);
    }

    if (set_gpio_info_flag == 1) {
        memcpy(current_info, &set_gpio_info, sizeof(firmware_device_info_t));
        set_gpio_info_flag = 0;
    }
    
    dev_debug(firmware_debug(), "%s %d\n", __func__, __LINE__);

    return 0;
}

static void init_chip_pre(void)
{

    dev_debug(firmware_debug(), "%s %d\n", __func__, __LINE__);

    /* to be reset every time when upgrade,solve the use of MAC side GPIO,
    During the startup process, the MAC terminal will be reset, causing the problem of invalid settings */
    firmware_cpld_gpio_set_direction(current_info->tdi, 1);
    firmware_cpld_gpio_set_direction(current_info->tck, 1);
    firmware_cpld_gpio_set_direction(current_info->tms, 1);
    firmware_cpld_gpio_set_direction(current_info->jtag_en, 1);
    if (current_info->select >= 0) {
        firmware_cpld_gpio_set_direction(current_info->select, 1);
    }
    if (current_info->jtag_4.pin >= 0) {
        firmware_cpld_gpio_set_direction(current_info->jtag_4.pin, 1);
        firmware_cpld_gpio_set_direction(current_info->jtag_3.pin, 1);
        firmware_cpld_gpio_set_direction(current_info->jtag_2.pin, 1);
        firmware_cpld_gpio_set_direction(current_info->jtag_1.pin, 1);
        firmware_cpld_gpio_set_direction(current_info->jtag_5.pin, 1);
    }
    
    firmware_cpld_gpio_set_output_value(current_info->tdi, 1);
    firmware_cpld_gpio_set_output_value(current_info->tck, 1);
    firmware_cpld_gpio_set_output_value(current_info->tms, 1);
    firmware_cpld_gpio_set_output_value(current_info->jtag_en, 1);
    if (current_info->jtag_4.pin >= 0) {
        firmware_cpld_gpio_set_output_value(current_info->jtag_1.pin, current_info->jtag_1.val);
        firmware_cpld_gpio_set_output_value(current_info->jtag_2.pin, current_info->jtag_2.val);
        firmware_cpld_gpio_set_output_value(current_info->jtag_3.pin, current_info->jtag_3.val);
        firmware_cpld_gpio_set_output_value(current_info->jtag_4.pin, current_info->jtag_4.val);
        firmware_cpld_gpio_set_output_value(current_info->jtag_5.pin, current_info->jtag_5.val);
    }
    if (current_info->select >= 0) {
        firmware_cpld_gpio_set_output_value(current_info->select, 1);
    }
    
    firmware_cpld_gpio_set_direction(current_info->tdo, 0);
    return;
}

static int init_chip(int slot)
{

    dev_debug(firmware_debug(), "%s %d\n", __func__, __LINE__);

    if (current_info == NULL) {
        return -1;
    }
    init_chip_pre();
    
    dev_debug(firmware_debug(), "tdi %d %d\n",current_info->tdi, firmware_cpld_gpio_get_value(current_info->tdi));
    dev_debug(firmware_debug(), "tdo %d %d\n",current_info->tdo, firmware_cpld_gpio_get_value(current_info->tdo));
    dev_debug(firmware_debug(), "tck %d %d\n",current_info->tck, firmware_cpld_gpio_get_value(current_info->tck));
    dev_debug(firmware_debug(), "tms %d %d\n",current_info->tms, firmware_cpld_gpio_get_value(current_info->tms));
    dev_debug(firmware_debug(), " jtag_en:%d %d\n",current_info->jtag_en, firmware_cpld_gpio_get_value(current_info->jtag_en));
    if (current_info->select >= 0) {
        dev_debug(firmware_debug(), " select:%d %d\n",current_info->select, firmware_cpld_gpio_get_value(current_info->select));
    }

    return 0;
}

static int finish_chip(int slot)
{
    if (current_info == NULL) {
        return -1;
    }
    
    firmware_cpld_gpio_set_output_value(current_info->jtag_en, 0);
    if (current_info->select >= 0) {
        firmware_cpld_gpio_set_output_value(current_info->select, 0);
    }
    if (current_info->jtag_4.pin >= 0) {
        firmware_cpld_gpio_set_output_value(current_info->jtag_4.pin, 1);
    }
    
    return 0;
}

/* TDI pull up */
static void pull_tdi_up(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tdi, 1);
}

/* TDI pull dowm */
static void pull_tdi_down(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tdi, 0);
}

/* TCK pull up */
static void pull_tck_up(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tck, 1);
}

/* TCK pull down */
static void pull_tck_down(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tck, 0);
}

/* TMS pull up */
static void pull_tms_up(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tms, 1);
}

/* TCK pull dowm */
static void pull_tms_down(void)
{
    if (current_info == NULL) {
        return;
    }
    firmware_cpld_gpio_set_output_value(current_info->tms, 0);
}

/* read TDO */
static int read_tdo(void)
{
    if (current_info == NULL) {
        return -1;
    }
    return firmware_cpld_gpio_get_value(current_info->tdo);
}

int B6510_fmw_set_gpio_info(firmware_upg_gpio_info_t *info)
{
    if (info == NULL) {
         dev_debug(firmware_debug(), "set gpio info info %p is null.\n", info);
        return -1;
    }

    set_gpio_info.tdi = info->tdi;
    set_gpio_info.tck = info->tck;
    set_gpio_info.tms = info->tms;
    set_gpio_info.tdo = info->tdo;
    set_gpio_info.jtag_en = info->jtag_en;
    set_gpio_info.select= info->select;
    set_gpio_info.jtag_5.pin = info->jtag_5.pin;
    set_gpio_info.jtag_4.pin = info->jtag_4.pin;
    set_gpio_info.jtag_3.pin = info->jtag_3.pin;
    set_gpio_info.jtag_2.pin = info->jtag_2.pin;
    set_gpio_info.jtag_1.pin = info->jtag_1.pin;
    set_gpio_info.jtag_5.val = info->jtag_5.val;
    set_gpio_info.jtag_4.val = info->jtag_4.val;
    set_gpio_info.jtag_3.val = info->jtag_3.val;
    set_gpio_info.jtag_2.val = info->jtag_2.val;
    set_gpio_info.jtag_1.val = info->jtag_1.val;
    set_gpio_info_flag = 1;
    dev_debug(firmware_debug(), "set gpio info[tdi:%d tck:%d tms:%d tdo:%d jtag_en:%d select:%d].\n",
        info->tdi, info->tck, info->tms, info->tdo, info->jtag_en, info->select);
    return 0;
}

firmware_cpld_t fmw_cpld0 = {
    .devname = "firmware_cpld_ispvme0",
    .slot = 1,
    .chip_index = 1,
    .is_used = 1,
    .tck_delay = 60,
    .pull_tdi_up = pull_tdi_up,
    .pull_tdi_down = pull_tdi_down,
    .pull_tck_up = pull_tck_up,
    .pull_tck_down = pull_tck_down,
    .pull_tms_up = pull_tms_up,
    .pull_tms_down = pull_tms_down,
    .read_tdo = read_tdo,
    .init_cpld = init_cpld,
    .init_chip = init_chip,
    .finish_chip = finish_chip,
    .finish_cpld = finish_cpld,
    .get_version = NULL,
};

static void cpld_release(struct device *dev)
{
    return;
}

static struct platform_device cpld = {
    .name               = "firmware_cpld_ispvme",
    .id                 = 0,
    .num_resources      = 0,
    .dev = {
        .release = cpld_release,
    }
};

extern void set_currrent_cpld_info(firmware_cpld_t *info);
extern int dfd_get_my_card_type(void);

int  fmw_cpld_product_init(void)
{
	current_info = NULL;
#if 0
    int dev_type;
    int i;

    dev_type = drv_get_my_dev_type();
    if (dev_type < 0) {
        printk(KERN_ERR "Failed to get device type, when upgrade cpld.\n");
        return FIRMWARE_FAILED;
    }

    for (i = 0; i < sizeof(cpld_info)/sizeof(cpld_info[0]); i++) {
        if (cpld_info[i].type == dev_type) {
            current_info = &cpld_info[i];
            printk(KERN_ERR "device type 0x%x match i %d.\n", dev_type, i);
            printk(KERN_ERR "tdi[%d] tck[%d] tms[%d] tdo[%d] jtat_en[%d].\n", current_info->tdi,
                current_info->tck, current_info->tms, current_info->tdo, current_info->jtag_en);
        }
    }
#endif

    if (current_info == NULL) {
        current_info = &default_cpld_info;
    }

    platform_device_register(&cpld);
    fmw_cpld_upg_copy_firmware_info(&fmw_cpld0);

    /* fmw_cpld0.init_cpld(); */
    set_currrent_cpld_info(&fmw_cpld0);

	fmw_cpld_reg_gpio_info_set_func(B6510_fmw_set_gpio_info);

    return FIRMWARE_SUCCESS;
}

void fmw_cpld_product_exit(void)
{
    platform_device_unregister(&cpld);
    return;
}