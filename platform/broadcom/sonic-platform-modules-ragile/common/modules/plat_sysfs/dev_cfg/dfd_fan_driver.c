#include <linux/module.h>
#include <linux/slab.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "../dev_sysfs/include/sysfs_common.h"

#define FAN_SIZE                          (256)

int g_dfd_fan_dbg_level = 0;
module_param(g_dfd_fan_dbg_level, int, S_IRUGO | S_IWUSR);

int dfd_get_fan_roll_status(unsigned int fan_index, unsigned int motor_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_ROLL_STATUS, fan_index, motor_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan roll status error, fan:%d,motor:%d\n",
            fan_index, motor_index);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan%u motor%u get fan roll status success, status:%d.\n",
        fan_index, motor_index, status);
    return status;
}

int dfd_get_fan_present_status(unsigned int fan_index)
{
    int key, ret;
    int status;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_DEV_PRESENT_STATUS, WB_MAIN_DEV_FAN, fan_index);
    ret = dfd_info_get_int(key, &status, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan%u get present status error, key:0x%x\n", fan_index, key);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan%u get present status success, status:%d.\n", fan_index, status);
    return status;
}

ssize_t dfd_get_fan_speed(unsigned int fan_index, unsigned int motor_index,unsigned int *speed)
{
    int key, ret, speed_tmp;

    if (speed == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_SPEED, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_tmp, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "get fan speed error, key:0x%x,ret:%d\n",key, ret);
        return ret;
    }

    if (speed_tmp == 0 || speed_tmp == 0xffff) {
        *speed = 0;
    } else {
        *speed = 15000000 / speed_tmp;
    }
    return DFD_RV_OK;
}

int dfd_set_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int level)
{
    int key, ret;

    if (level < 0 || level > 0xff) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, can not set fan speed level: %d.\n",
            fan_index, motor_index, level);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, motor_index);
    ret = dfd_info_set_int(key, level);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, set fan level 0x%02x error, key:0x%x,ret:%d\n",
            fan_index, motor_index, level, key, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, set fan speed level 0x%02x success.\n",
        fan_index, motor_index, level);
    return DFD_RV_OK;
}

int dfd_set_fan_pwm(unsigned int fan_index, unsigned int motor_index, int pwm)
{
    int ret, data;

    if (pwm < 0 || pwm > 100) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, can't set pwm: %d.\n",
            fan_index, motor_index, pwm);
        return -DFD_RV_INVALID_VALUE;
    }

    data = pwm * 255 / 100;
    ret = dfd_set_fan_speed_level(fan_index, motor_index, data);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, set fan ratio:%d error, ret:%d\n",
            fan_index, motor_index, data, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, set fan ratio %d success.\n",
        fan_index, motor_index, data);
    return DFD_RV_OK;
}

int dfd_get_fan_speed_level(unsigned int fan_index, unsigned int motor_index, int *level)
{
    int key, ret, speed_level;

    if (level == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_FAN_RATIO, fan_index, motor_index);
    ret = dfd_info_get_int(key, &speed_level, NULL);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, get fan speed level error, key:0x%x,ret:%d\n",
            fan_index, motor_index, key, ret);
        return ret;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, get fan speed level success, value:0x%02x.\n",
        fan_index, motor_index, speed_level);
    *level = speed_level;
    return DFD_RV_OK;
}

int dfd_get_fan_pwm(unsigned int fan_index, unsigned int motor_index, int *pwm)
{
    int ret, level;

    if (pwm == NULL) {
        DFD_FAN_DEBUG(DBG_ERROR, "param error. fan index:%d, motor index:%d.\n",
            fan_index, motor_index);
        return -DFD_RV_INVALID_VALUE;
    }

    ret = dfd_get_fan_speed_level(fan_index, motor_index, &level);
    if (ret < 0) {
        DFD_FAN_DEBUG(DBG_ERROR, "fan:%u, motor:%u, get fan pwm error, ret:%d\n",
            fan_index, motor_index, ret);
        return ret;
    }

    if ((level * 100) % 255 > 0) {
        *pwm = level * 100 / 255 + 1;
    } else {
        *pwm = level * 100 / 255;
    }

    DFD_FAN_DEBUG(DBG_VERBOSE, "fan:%u, motor:%u, get fan pwm success, value:%d.\n",
        fan_index, motor_index, *pwm);
    return DFD_RV_OK;
}
