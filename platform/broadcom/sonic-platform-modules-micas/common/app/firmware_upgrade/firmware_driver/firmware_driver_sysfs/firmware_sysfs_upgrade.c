#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/ctype.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <firmware_sysfs.h>
#include <firmware_sysfs_upgrade.h>
#include <firmware_upgrade.h>

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
 * function:param[in] Upgrade access enabling switch
 * @flag:param[in] !0:enable 0:disable
 * return: 0:success, <0:failed
 */
static int firmware_upgrade_en(firmware_sysfs_t *sysfs_info, int flag)
{
    int i;
    firmware_logic_dev_en_t *firmware_logic_dev_en_info;
    int ret, rv;
    char *dev_name;

    ret = 0;
    FIRMWARE_DRIVER_DEBUG_VERBOSE("%s en switch: gpio en num %d, logic reg en num %d.\n",
            flag ? "Open" : "Close", sysfs_info->gpio_en_info_num, sysfs_info->logic_dev_en_num);
    for (i = 0; i < sysfs_info->gpio_en_info_num; i++) {
        FIRMWARE_DRIVER_DEBUG_VERBOSE("firmware sysfs [%d] gpio[%d] en_level[%d]\n",
                i, sysfs_info->gpio_en_info[i].en_gpio, sysfs_info->gpio_en_info[i].en_level);
        if (flag) {
            ret = gpio_request(sysfs_info->gpio_en_info[i].en_gpio, "sysfs_upgrade_gpio_en");
            if (ret) {
                FIRMWARE_DRIVER_DEBUG_ERROR("Requesting cpld_ispvme_upgrade EN[%d] GPIO[%d] failed!\n",
                        i, sysfs_info->gpio_en_info[i].en_gpio);
                goto free_gpio;
            }
            gpio_direction_output(sysfs_info->gpio_en_info[i].en_gpio, sysfs_info->gpio_en_info[i].en_level);
            sysfs_info->gpio_en_info[i].flag = 1;
        } else {
            gpio_set_value(sysfs_info->gpio_en_info[i].en_gpio, !sysfs_info->gpio_en_info[i].en_level);
            gpio_free(sysfs_info->gpio_en_info[i].en_gpio);
            sysfs_info->gpio_en_info[i].flag = 0;
        }
    }

    for (i = 0; i < sysfs_info->logic_dev_en_num; i++) {
        firmware_logic_dev_en_info = &sysfs_info->logic_dev_en_info[i];
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
    for (i = 0; i < sysfs_info->logic_dev_en_num; i++) {
        firmware_logic_dev_en_info = &sysfs_info->logic_dev_en_info[i];
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
    for (i = 0; i < sysfs_info->gpio_en_info_num; i++) {
        if (sysfs_info->gpio_en_info[i].flag == 1) {
            gpio_set_value(sysfs_info->gpio_en_info[i].en_gpio, !sysfs_info->gpio_en_info[i].en_level);
            gpio_free(sysfs_info->gpio_en_info[i].en_gpio);
            sysfs_info->gpio_en_info[i].flag = 0;
        } else {
            break;
        }
    }

    return -1;
}

/*
 * firmware_init_dev_loc
 * function: init logic device, enable upgrade access
 * return: 0:success, <0:failed
 */
int firmware_init_dev_loc(firmware_sysfs_t *sysfs_info)
{
    int ret;

    ret = firmware_upgrade_en(sysfs_info, 1);
    return ret;
}

/*
 * firmware_finish_dev_loc
 * function: finish logic device, disable upgrade access
 * return: 0:success, <0:failed
 */
int firmware_finish_dev_loc(firmware_sysfs_t *sysfs_info){
    int ret;
    ret = firmware_upgrade_en(sysfs_info, 0);
    return ret;
}
