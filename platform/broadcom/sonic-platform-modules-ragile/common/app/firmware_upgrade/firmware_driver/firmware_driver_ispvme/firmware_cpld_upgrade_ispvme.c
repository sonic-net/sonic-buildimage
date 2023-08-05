#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <firmware_ispvme.h>
#include <firmware_cpld_ispvme.h>
#include <firmware_upgrade.h>

/* TCK clock MAX 16MHz */
#define    TCK_DELAY                         (current_fmw_cpld->tck_delay)

#if 0
static firmware_cpld_t default_fmw_cpld;
#endif

static firmware_cpld_t *current_fmw_cpld;

static int TDI_PULL_UP(void);
static int TDI_PULL_DOWN(void);
static int TMS_PULL_UP(void);
static int TMS_PULL_DOWN(void);
static int TCK_PULL_UP(void);
static int TCK_PULL_DOWN(void);

/*
 * set_currrent_cpld_info
 * function: Save the current device information
 * @info: param[in] Information about the device to be updated
 */
static void set_currrent_cpld_info(firmware_cpld_t *info)
{
    current_fmw_cpld = info;
}

static int firmware_file_read(const char *path, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t pos;

    filp = filp_open(path, O_RDONLY, 0);
    if (IS_ERR(filp)) {
        FIRMWARE_DRIVER_DEBUG_ERROR("read open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    pos = (loff_t)addr;
    ret = kernel_read(filp, val, size, &pos);
    if (ret != size) {
        FIRMWARE_DRIVER_DEBUG_ERROR("read kernel_read failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, addr, size, ret);
        goto exit;
    }
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

static int firmware_file_write(const char *path, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;
    struct file *filp;
    loff_t pos;

    filp = filp_open(path, O_RDWR, 777);
    if (IS_ERR(filp)) {
        FIRMWARE_DRIVER_DEBUG_ERROR("write open failed errno = %ld\r\n", -PTR_ERR(filp));
        filp = NULL;
        goto exit;
    }

    pos = (loff_t)addr;
    ret = kernel_write(filp, (void*)val, size, &pos);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("write kernel_write failed, path=%s, addr=%d, size=%ld, ret=%d\r\n", path, addr, size, ret);
        goto exit;
    }
    vfs_fsync(filp, 1);
    filp_close(filp, NULL);

    return ret;

exit:
    if (filp != NULL) {
        filp_close(filp, NULL);
    }

    return -1;
}

/*
 * firmware_file_do_work
 * function: Sets logical register values
 * @path:param[in] Logic device descriptor
 * @addr:param[in] Logic device address
 * @value:param[in] the register value needs to be set
 * @mask:param[in] register mask
 * @width:param[in] register bit width
 * return: 0:success, <0:failed
 */
static int firmware_file_do_work(char *path, uint32_t addr, uint32_t value, uint32_t mask,
                    int32_t width)
{
    int ret;
    uint8_t read_value[4], write_value[4];
    uint8_t tmp_read8, tmp_write8, tmp_mask8;
    uint32_t tmp_read32, tmp_write32;

    FIRMWARE_DRIVER_DEBUG_VERBOSE("path=%s, addr=0x%x, value=0x%x mask=0x%x\r\n", path, addr, value, mask);
    if ((width > 4) || (width < 0)) {
        FIRMWARE_DRIVER_DEBUG_ERROR("width %d is not support.\r\n", width);
        return -1;
    }
    ret = 0;
    mem_clear(read_value, sizeof(read_value));
    mem_clear(write_value, sizeof(write_value));
    ret = firmware_file_read(path, addr, read_value, width);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("firmware sysfs read.\r\n");
        return -1;
    }

    switch (width) {
    case 1:
        tmp_read8 = read_value[0];
        tmp_mask8 = (uint8_t)(mask) & 0xFF;
        tmp_write8 = (uint8_t)value & 0xFF;
        write_value[0] = (tmp_read8 & tmp_mask8) | tmp_write8;
        FIRMWARE_DRIVER_DEBUG_VERBOSE("1 byte write val[0]:0x%x", write_value[0]);
        break;
    case 2:
        FIRMWARE_DRIVER_DEBUG_ERROR("width %d is not support.\r\n", width);
        return -1;
    case 4:
        memcpy((uint8_t *)&tmp_read32, read_value, 4);
        tmp_write32 = (tmp_read32 & mask) | value;
        memcpy(write_value, (uint8_t *)&tmp_write32, 4);
        FIRMWARE_DRIVER_DEBUG_VERBOSE("4 byte write val[0]:0x%x, val[1]:0x%x, val[2]:0x%x, val[3]:0x%x",
            write_value[0], write_value[1], write_value[2], write_value[3]);
        break;
    default:
        FIRMWARE_DRIVER_DEBUG_ERROR("width %d is not support.\r\n", width);
        return -1;
    }

    FIRMWARE_DRIVER_DEBUG_VERBOSE("write logic dev[%s] addr[0x%x].\r\n", path, addr);
    ret = firmware_file_write(path, addr, write_value, width);
    if (ret < 0) {
        FIRMWARE_DRIVER_DEBUG_ERROR("firmware_file_write %s addr 0x%x failed, ret=%d.\r\n", path, addr, ret);
        return -1;
    }

    return 0;
}

/*
 * firmware_upgrade_en
 * function: Upgrade access enabling switch
 * @flag: !0:enable 0:disable
 */
static int firmware_upgrade_en(int flag)
{
    int i;
    firmware_logic_dev_en_t *firmware_logic_dev_en_info;
    int ret, rv;
    char *dev_name;

    ret = 0;
    FIRMWARE_DRIVER_DEBUG_VERBOSE("%s en switch: gpio en num %d, logic reg en num %d.\n",
            flag ? "Open" : "Close", current_fmw_cpld->gpio_en_info_num, current_fmw_cpld->logic_dev_en_num);
    for (i = 0; i < current_fmw_cpld->gpio_en_info_num; i++) {
        if (flag) {
            ret = gpio_request(current_fmw_cpld->gpio_en_info[i].en_gpio, "cpld_ispvme_upgrade");
            if (ret) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade EN[%d] GPIO[%d] failed!\n",
                        i, current_fmw_cpld->gpio_en_info[i].en_gpio);
                goto free_gpio;
            }
            gpio_direction_output(current_fmw_cpld->gpio_en_info[i].en_gpio, current_fmw_cpld->gpio_en_info[i].en_level);
            current_fmw_cpld->gpio_en_info[i].flag = 1;
        } else {
            gpio_set_value(current_fmw_cpld->gpio_en_info[i].en_gpio, !current_fmw_cpld->gpio_en_info[i].en_level);
            gpio_free(current_fmw_cpld->gpio_en_info[i].en_gpio);
            current_fmw_cpld->gpio_en_info[i].flag = 0;
        }
    }

    for (i = 0; i < current_fmw_cpld->logic_dev_en_num; i++) {
        firmware_logic_dev_en_info = &current_fmw_cpld->logic_dev_en_info[i];
        dev_name = firmware_logic_dev_en_info->dev_name;
        FIRMWARE_DRIVER_DEBUG_VERBOSE("firmware sysfs [%d] dev_name[%s] addr[0x%x] mask[0x%x]"
            " en_val[0x%x] dis_val[0x%x] width[%d]\n",
            i , firmware_logic_dev_en_info->dev_name, firmware_logic_dev_en_info->addr,
            firmware_logic_dev_en_info->mask, firmware_logic_dev_en_info->en_val,
            firmware_logic_dev_en_info->dis_val, firmware_logic_dev_en_info->width);
        if (flag) {
            ret = firmware_file_do_work(dev_name, firmware_logic_dev_en_info->addr,
                    firmware_logic_dev_en_info->en_val, firmware_logic_dev_en_info->mask,
                    firmware_logic_dev_en_info->width);
            if (ret < 0) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Open logic register [%d] EN failed, ret %d.\n", i, ret);
                goto free_logic_dev;
            } else {
                firmware_logic_dev_en_info->flag = 1;
            }
        } else {
            rv = firmware_file_do_work(dev_name, firmware_logic_dev_en_info->addr,
                    firmware_logic_dev_en_info->dis_val, firmware_logic_dev_en_info->mask,
                    firmware_logic_dev_en_info->width);
            if (rv < 0) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Close logic register [%d] EN failed, ret %d.\n", i, rv);
                ret = -1;
            }
            firmware_logic_dev_en_info->flag = 0;
        }
    }

    return ret;
free_logic_dev:
    for (i = 0; i < current_fmw_cpld->logic_dev_en_num; i++) {
        firmware_logic_dev_en_info = &current_fmw_cpld->logic_dev_en_info[i];
        dev_name = firmware_logic_dev_en_info->dev_name;
        if (firmware_logic_dev_en_info->flag == 1) {
            ret = firmware_file_do_work(dev_name, firmware_logic_dev_en_info->addr,
                    firmware_logic_dev_en_info->dis_val, firmware_logic_dev_en_info->mask,
                    firmware_logic_dev_en_info->width);
            if (ret < 0) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Close logic register [%d] EN failed, ret %d.\n", i, ret);
            }
            firmware_logic_dev_en_info->flag = 0;
        } else {
            break;
        }
    }
free_gpio:
    for (i = 0; i < current_fmw_cpld->gpio_en_info_num; i++) {
        if (current_fmw_cpld->gpio_en_info[i].flag == 1) {
            gpio_set_value(current_fmw_cpld->gpio_en_info[i].en_gpio, !current_fmw_cpld->gpio_en_info[i].en_level);
            gpio_free(current_fmw_cpld->gpio_en_info[i].en_gpio);
            current_fmw_cpld->gpio_en_info[i].flag = 0;
        } else {
            break;
        }
    }

    return -1;
}

/*
 * init_cpld
 * function:Initialize CPLD
 * return value: 0 success ; -1 fail
 */
static int init_cpld(void)
{
    int ret;
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    mdelay(10);
    ret = 0;
    ret = gpio_request(current_fmw_cpld->tdi, "cpld_ispvme_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade TDI GPIO failed!\n");
        return ret;
    }
    ret = gpio_request(current_fmw_cpld->tck, "cpld_ispvme_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade TCK GPIO failed!\n");
        goto free_tdi;
    }
    ret = gpio_request(current_fmw_cpld->tms, "cpld_ispvme_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade TMS GPIO failed!\n");
        goto free_tck;
    }
    ret = gpio_request(current_fmw_cpld->tdo, "cpld_ispvme_upgrade");
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade TDO GPIO failed!\n");
        goto free_tms;
    }

    gpio_direction_output(current_fmw_cpld->tdi, 1);
    gpio_direction_output(current_fmw_cpld->tck, 1);
    gpio_direction_output(current_fmw_cpld->tms, 1);

    gpio_direction_input(current_fmw_cpld->tdo);
    ret = firmware_upgrade_en(1);
    if (ret) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: open firmware upgrade en failed, ret %d.\n", ret);
        goto free_tdo;
    }
#if 0
    /* test GPIO */
    if (TDI_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TDI_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TDI_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TDI_PULL_DOWN failed.\n");
        goto free_tdo;
    }
    if (TMS_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TMS_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TMS_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TMS_PULL_DOWN failed.\n");
        goto free_tdo;
    }
    if (TCK_PULL_UP() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TCK_PULL_UP failed.\n");
        goto free_tdo;
    }
    if (TCK_PULL_DOWN() < 0 ) {
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: TCK_PULL_DOWN failed.\n");
        goto free_tdo;
    }
#endif
    mdelay(10);
    return 0;

free_tdo:
    gpio_free(current_fmw_cpld->tdo);
free_tms:
    gpio_free(current_fmw_cpld->tms);
free_tck:
    gpio_free(current_fmw_cpld->tck);
free_tdi:
    gpio_free(current_fmw_cpld->tdi);
    return ret;
}

/*
 * finish_cpld
 * function: finish  CPLD upgrade operation
 * return value: 0 success ; -1 fail
 */
static int finish_cpld(void)
{
    int ret;

    if (current_fmw_cpld == NULL) {
        return -1;
    }
    mdelay(10);
    ret = firmware_upgrade_en(0);
    if (ret < 0){
        FIRMWARE_DRIVER_DEBUG_ERROR("Error: close firmware upgrade en failed, ret %d.\r\n", ret);
    }

    gpio_free(current_fmw_cpld->tdi);
    gpio_free(current_fmw_cpld->tck);
    gpio_free(current_fmw_cpld->tms);
    gpio_free(current_fmw_cpld->tdo);
    mdelay(10);
    return 0;
}

/* Loop waiting for */
static int pull_wait(int gpio, int value) {
    int i, j;
    /* Timeout time is two seconds */
    for (i = 0; i < 20; i++) {
        for (j = 0; j < 100; j++) {
            if (!!gpio_get_value(gpio) == !!value ) {
                return 0;
            }
            /* The first loop does not delay, normally the first loop can immediately return the result */
            if (i) {
                mdelay(1);
            }
        }
        /* The CPU is released every 100ms */
        schedule();
    }
    /* timeout */
    FIRMWARE_DRIVER_DEBUG_ERROR("Error: Wait gpio %d pull to %d failed.\n", gpio, value);
    return -1;
}

/* TDI pull-up */
static int pull_tdi_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tdi, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tdi, 1);
}

/* TDI pull-down */
static int pull_tdi_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tdi, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tdi, 0);
}

/* TCK pull-up */
static int pull_tck_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tck, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tck, 1);
}

/* TCK pull-down */
static int pull_tck_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tck, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tck, 0);
}

/* TMS pull-up */
static int pull_tms_up(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tms, 1);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tms, 1);
}

/* TMS pull-down */
static int pull_tms_down(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    gpio_set_value(current_fmw_cpld->tms, 0);

    /* Wait for the GPIO value to be set successfully */
    return pull_wait(current_fmw_cpld->tms, 0);
}

/* Read TDO */
static int read_tdo(void)
{
    if (current_fmw_cpld == NULL) {
        return -1;
    }
    return gpio_get_value(current_fmw_cpld->tdo);
}

static firmware_cpld_function_t function_fmw_cpld = {
      .pull_tdi_up = pull_tdi_up,
      .pull_tdi_down = pull_tdi_down,
      .pull_tck_up = pull_tck_up,
      .pull_tck_down = pull_tck_down,
      .pull_tms_up = pull_tms_up,
      .pull_tms_down = pull_tms_down,
      .read_tdo = read_tdo,
      .init_cpld = init_cpld,
      .finish_cpld = finish_cpld,
};

/*
 * TDI_PULL_DOWN
 * function: Lower TDI
 */
static int TDI_PULL_DOWN(void)
{
    if ( function_fmw_cpld.pull_tdi_down != NULL) {
        return function_fmw_cpld.pull_tdi_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDI_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TDI_PULL_UP
 * function: High TDI
 */
static int TDI_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tdi_up != NULL) {
        return function_fmw_cpld.pull_tdi_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDI_PULL_UP.\n");
        return -1;
    }
}

/*
 * TCK_PULL_DOWN
 * function: Lower TCK
 */
static int TCK_PULL_DOWN(void)
{
    if (function_fmw_cpld.pull_tck_down != NULL) {
        return function_fmw_cpld.pull_tck_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TCK_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TCK_PULL_UP
 * function: High TCK
 */
static int TCK_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tck_up != NULL) {
        return function_fmw_cpld.pull_tck_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TCK_PULL_UP.\n");
        return -1;
    }
}

/*
 * TMS_PULL_DOWN
 * function: Lower TMS
 */
static int TMS_PULL_DOWN(void)
{
    if (function_fmw_cpld.pull_tms_down != NULL) {
        return function_fmw_cpld.pull_tms_down();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TMS_PULL_DOWN.\n");
        return -1;
    }
}

/*
 * TMS_PULL_UP
 * function: High TMS
 */
static int TMS_PULL_UP(void)
{
    if (function_fmw_cpld.pull_tms_up != NULL) {
        return function_fmw_cpld.pull_tms_up();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TMS_PULL_UP.\n");
        return -1;
    }
}

/*
 * TDO_READ
 * function:Read the TDO level
 */
static int TDO_READ(void)
{
    if (function_fmw_cpld.read_tdo != NULL) {
        return function_fmw_cpld.read_tdo();
    } else {
        FIRMWARE_DRIVER_DEBUG_ERROR("NO support TDO_READ.\n");
        return -1;
    }
}

/*
 * cpld_upgrade_init
 * function:Initialize GPIO and CPLD
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int cpld_upgrade_init(void)
{
    int ret;

    if (function_fmw_cpld.init_cpld != NULL) {
        ret = function_fmw_cpld.init_cpld();
        if (ret != FIRMWARE_SUCCESS) {
            return ret;
        }
    }

    return FIRMWARE_SUCCESS;
}

/*
 * cpld_upgrade_finish
 * function:Release GPIO and CPLD
 * return value: success--FIRMWARE_SUCCESS; fail--FIRMWARE_FAILED
 */
static int cpld_upgrade_finish(void)
{
    int ret;

    if (function_fmw_cpld.finish_cpld != NULL) {
        ret = function_fmw_cpld.finish_cpld();
        if (ret != FIRMWARE_SUCCESS) {
            return ret;
        }
    }

    return FIRMWARE_SUCCESS;
}

/**
 * firmware_init_vme
 * function: Initialize GPIO,
 * @cpld_info: param[in] Information about the device to be written to
 */
int firmware_init_vme(firmware_cpld_t *cpld_info){
    int ret;
    set_currrent_cpld_info(cpld_info);
    /* Initialize GPIO and CPLD */
    ret = cpld_upgrade_init();
    return ret;
}

/**
 * firmware_finish_vme
 * function: Release GPIO
 * @cpld_info: param[in] Information about the device to be written to
 */
int firmware_finish_vme(firmware_cpld_t *cpld_info){
    int ret;
    set_currrent_cpld_info(cpld_info);
    ret = cpld_upgrade_finish();
    return ret;
}

/**
 * fwm_cpld_tdi_op
 * function: Operate TDI
 * @value: param[in] TDI level */
int fwm_cpld_tdi_op(int value)
{
    if (value) {
        return TDI_PULL_UP();
    } else {
        return TDI_PULL_DOWN();
    }
}

/**
 * fwm_cpld_tck_op
 * function: Operate TCK
 * @value: param[in] TCK level */
int fwm_cpld_tck_op(int value)
{
    if (value) {
        return TCK_PULL_UP();
    } else {
        return TCK_PULL_DOWN();
    }
}

/**
 * fwm_cpld_tms_op
 * function: Operate TMS
 * value: param[in] TMS level */
int fwm_cpld_tms_op(int value)
{
    if (value) {
        return TMS_PULL_UP();
    } else {
        return TMS_PULL_DOWN();
    }
}

/**
 * fwm_cpld_tdo_op
 * function: Read TDO
 */
int fwm_cpld_tdo_op()
{
    return TDO_READ();
}
