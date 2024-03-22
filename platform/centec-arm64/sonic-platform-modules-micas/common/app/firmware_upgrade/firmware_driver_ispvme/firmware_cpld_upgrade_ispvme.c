/*
 * firmware_cpld_upgrade.c
 * Original Author : support <support>2013-10-25
 *
 * CPLD upgrade driver
 *
 * History
 *    v1.0    support <support  2013-10-25  Initial version.
 *
 */
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <firmware_ispvme.h>
#include <firmware_cpld_ispvme.h>
#include <config_ispvme.h>

/* TCK clock MAX 16MHz */
#define    TCK_DELAY                         (current_fmw_cpld->tck_delay)
#if 0
static firmware_cpld_t default_fmw_cpld;
#endif
static firmware_cpld_t fmw_cpld[FIRMWARE_MAX_CPLD_NUM];
static firmware_cpld_t *current_fmw_cpld;

static firmware_set_gpio_info_func_t g_set_gpio_info_func = NULL;

void set_currrent_cpld_info(firmware_cpld_t *info)
{
    current_fmw_cpld = info;
}

static void TDI_PULL_DOWN(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tdi_down) {
        current_fmw_cpld->pull_tdi_down();
    } else {
        dev_debug(firmware_debug(), "NO support TDI_PULL_DOWN.\n");
    }
}

static void TDI_PULL_UP(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tdi_up) {
        current_fmw_cpld->pull_tdi_up();
    } else {
        dev_debug(firmware_debug(), "NO support TDI_PULL_UP.\n");
    }
}

static void TCK_PULL_DOWN(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tck_down) {
        current_fmw_cpld->pull_tck_down();
    } else {
        dev_debug(firmware_debug(), "NO support TCK_PULL_DOWN.\n");
    }
}

static void TCK_PULL_UP(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tck_up) {
        current_fmw_cpld->pull_tck_up();
    } else {
        dev_debug(firmware_debug(), "NO support TCK_PULL_UP.\n");
    }
}

static void TMS_PULL_DOWN(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tms_down) {
        current_fmw_cpld->pull_tms_down();
    } else {
        dev_debug(firmware_debug(), "NO support TMS_PULL_DOWN.\n");
    }
}

static void TMS_PULL_UP(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->pull_tms_up) {
        current_fmw_cpld->pull_tms_up();
    } else {
        dev_debug(firmware_debug(), "NO support TMS_PULL_UP.\n");
    }
}

static int TDO_READ(void)
{
    if (current_fmw_cpld != NULL && current_fmw_cpld->read_tdo) {
        return current_fmw_cpld->read_tdo();
    } else {
        dev_debug(firmware_debug(), "NO support TDO_READ.\n");
        return -1;
    }
}

firmware_cpld_t *fmw_cpld_upg_get_cpld(char *name)
{
    int i;

    for (i = 0; i < FIRMWARE_MAX_CPLD_NUM; i++) {
        if (fmw_cpld[i].is_used == 1 && strcmp(name, fmw_cpld[i].devname) == 0) {
            return &fmw_cpld[i];
        }
    }

    return NULL;
}

int fmw_cpld_upg_copy_firmware_info(firmware_cpld_t *info)
{
    int i;

    for (i = 0; i < FIRMWARE_MAX_CPLD_NUM; i++) {
        if (fmw_cpld[i].is_used == 1) {
            continue;
        } else {
            strncpy(fmw_cpld[i].devname, info->devname, FIRMWARE_DEV_NAME_LEN);
            fmw_cpld[i].slot            = info->slot;
            fmw_cpld[i].chip_index      = info->chip_index;
            fmw_cpld[i].is_used         = info->is_used;
            fmw_cpld[i].tck_delay       = info->tck_delay;
            fmw_cpld[i].pull_tdi_up     = info->pull_tdi_up;
            fmw_cpld[i].pull_tdi_down   = info->pull_tdi_down;
            fmw_cpld[i].pull_tck_up     = info->pull_tck_up;
            fmw_cpld[i].pull_tck_down   = info->pull_tck_down;
            fmw_cpld[i].pull_tms_up     = info->pull_tms_up;
            fmw_cpld[i].pull_tms_down   = info->pull_tms_down;
            fmw_cpld[i].read_tdo        = info->read_tdo;
            fmw_cpld[i].init_cpld       = info->init_cpld;
            fmw_cpld[i].init_chip       = info->init_chip;
            fmw_cpld[i].finish_chip     = info->finish_chip;
            fmw_cpld[i].finish_cpld     = info->finish_cpld;
            fmw_cpld[i].touch_watch_dog = info->touch_watch_dog;
            fmw_cpld[i].keeplive        = info->keeplive;
            fmw_cpld[i].get_version     = info->get_version;
            fmw_cpld[i].get_card_name   = info->get_card_name;
            return 0;
        }
    }
    return -1;
}

int fmw_cpld_set_gpio_info(firmware_upg_gpio_info_t *info)
{
    if (g_set_gpio_info_func == NULL) {
        dev_debug(firmware_debug(), "g_set_gpio_info_func is null.\n");
        return -1;
    }

    return g_set_gpio_info_func(info);
}

void fmw_cpld_reg_gpio_info_set_func(firmware_set_gpio_info_func_t func)
{
    if (func == NULL) {
        dev_debug(firmware_debug(), "fmw_cpld_register_gpio_info_set_func func = NULL.\n");
        return;
    }
    g_set_gpio_info_func = func;
    return;

}
#if 0
/* CPLD upgrade initialization operation */
static int fmw_cpld_upg_init_cpld(void)
{
    gpio_request(JTAG_TDI, "cpld_upgrade");
    gpio_request(JTAG_TCK, "cpld_upgrade");
    gpio_request(JTAG_TMS, "cpld_upgrade");
    gpio_request(JTAG_EN, "cpld_upgrade");
    gpio_request(JTAG_TDO, "cpld_upgrade");

    gpio_direction_output(JTAG_TDI, 1);
    gpio_direction_output(JTAG_TCK, 1);
    gpio_direction_output(JTAG_TMS, 1);
    gpio_direction_output(JTAG_EN, 1);

    gpio_direction_input(JTAG_TDO);
    return 0;
}

/* CPLD upgrade completion operation */
static int fmw_cpld_upg_finish_cpld(void)
{
    gpio_direction_output(JTAG_EN, 0);

    gpio_free(JTAG_TDI);
    gpio_free(JTAG_TCK);
    gpio_free(JTAG_TMS);
    gpio_free(JTAG_EN);
    gpio_free(JTAG_TDO);
    return 0;
}

/* TDI pull up */
static void fmw_cpld_upg_pull_tdi_up(void)
{
    __gpio_set_value(JTAG_TDI, 1);
}

/* TDI pull down */
static void fmw_cpld_upg_pull_tdi_down(void)
{
    __gpio_set_value(JTAG_TDI, 0);
}

/* TCK pull up */
static void fmw_cpld_upg_pull_tck_up(void)
{
    __gpio_set_value(JTAG_TCK, 1);
}

/* TCK pull down */
static void fmw_cpld_upg_pull_tck_down(void)
{
    __gpio_set_value(JTAG_TCK, 0);
}

/* TMS pull up */
static void fmw_cpld_upg_pull_tms_up(void)
{
    __gpio_set_value(JTAG_TMS, 1);
}

/* TCK pull down */
static void fmw_cpld_upg_pull_tms_down(void)
{
    __gpio_set_value(JTAG_TMS, 0);
}

/* read TDO */
static int fmw_cpld_upg_read_tdo(void)
{
    return __gpio_get_value(JTAG_TDO);
}
#endif
#if 0
static firmware_cpld_t default_fmw_cpld = {
    .devname = "default_firmware_cpld",
    .slot = 1,
    .is_used = 1,
    .tck_delay = 50,
    .pull_tdi_up = fmw_cpld_upg_pull_tdi_up,
    .pull_tdi_down = fmw_cpld_upg_pull_tdi_down,
    .pull_tck_up = fmw_cpld_upg_pull_tck_up,
    .pull_tck_down = fmw_cpld_upg_pull_tck_down,
    .pull_tms_up = fmw_cpld_upg_pull_tms_up,
    .pull_tms_down = fmw_cpld_upg_pull_tms_down,
    .read_tdo = fmw_cpld_upg_read_tdo,
    .init_cpld = fmw_cpld_upg_init_cpld,
    .finish_cpld = fmw_cpld_upg_finish_cpld,
};
#endif

/**
 *  Each product initializes its own related CPLD driver and needs to re-define the interface
 *  In the new interface, assign the relevant driver to fmw_cpld through the fmw_cpld_upg_copy_firmware_info interface
 */
int __attribute__ ((weak))fmw_cpld_product_init(void)
{
    dev_debug(firmware_debug(), "Nothing cpld init for this product.\n");
    return 0;
}

void __attribute__ ((weak))fmw_cpld_product_exit(void)
{
    dev_debug(firmware_debug(), "Nothing exit init for this product.\n");
    return;
}

int fmw_cpld_upg_init(void)
{
    int ret;
    mem_clear(fmw_cpld, FIRMWARE_MAX_CPLD_NUM * sizeof(firmware_cpld_t));
    ret = fmw_cpld_product_init();
    if (ret < 0) {
        return ret;
    }
#if 0
    set_currrent_cpld_info(&default_fmw_cpld);
#endif

    return 0;
}

void fmw_cpld_upg_exit(void)
{
    fmw_cpld_product_exit();
    return;
}

void fwm_cpld_tdi_op(int value)
{
    if (value) {
        TDI_PULL_UP();
    } else {
        TDI_PULL_DOWN();
    }
}

void fwm_cpld_tck_op(int value)
{
    if (value) {
        TCK_PULL_UP();
    } else {
        TCK_PULL_DOWN();
    }
}

void fwm_cpld_tms_op(int value)
{
    if (value) {
        TMS_PULL_UP();
    } else {
        TMS_PULL_DOWN();
    }
}

int fwm_cpld_tdo_op()
{
    return TDO_READ();
}
