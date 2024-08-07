#include <linux/module.h>
#include <linux/slab.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_adapter.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_cfg_file.h"
#include "../dev_sysfs/include/sysfs_common.h"

#define DFD_GET_TEMP_SENSOR_KEY1(dev_index, temp_index) \
    (((dev_index & 0xff) << 8) | (temp_index & 0xff))
#define DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, temp_type) \
        (((main_dev_id & 0x0f) << 4) | (temp_type & 0x0f))
#define DFD_FORMAT_STR_MAX_LEN          (32)

int g_dfd_sensor_dbg_level = 0;
module_param(g_dfd_sensor_dbg_level, int, S_IRUGO | S_IWUSR);

static int dfd_deal_hwmon_buf(uint8_t *buf, int buf_len, uint8_t *buf_new, int *buf_len_new, info_ctrl_t *info_ctrl)
{
    int i, tmp_len;
    int exp, decimal, divisor;
    int org_value, tmp_value;
    int div_result, div_mod;
    char fmt_str[DFD_FORMAT_STR_MAX_LEN];

    exp = info_ctrl->int_cons;
    decimal = info_ctrl->bit_offset;

    if (exp <= 0) {
        DBG_DEBUG(DBG_VERBOSE, "exponent %d, don't need transform. buf_len:%d, buf_len_new:%d\n",
            exp, buf_len, *buf_len_new);
        snprintf(buf_new, *buf_len_new, "%s", buf);
        *buf_len_new = strlen(buf_new);
        return DFD_RV_OK;
    }
    divisor = 1;
    for (i = 0; i < exp; i++) {
        divisor *= 10;
    }
    org_value = simple_strtol(buf, NULL, 10);
    if (org_value < 0) {
        tmp_value = 0 - org_value;
    } else {
        tmp_value = org_value;
    }
    div_result = tmp_value / divisor;
    div_mod = tmp_value % divisor;
    DBG_DEBUG(DBG_VERBOSE, "exp:%d, decimal:%d, original value:%d, divisor:%d, result :%d, mod:%d\n",
        exp, decimal, org_value, divisor, div_result, div_mod);

    mem_clear(fmt_str, sizeof(fmt_str));
    if (org_value < 0) {
        snprintf(fmt_str, sizeof(fmt_str), "-%%d.%%0%dd\n",exp);
    } else {
        snprintf(fmt_str, sizeof(fmt_str), "%%d.%%0%dd\n",exp);
    }
    DBG_DEBUG(DBG_VERBOSE, "format string:%s",fmt_str);
    snprintf(buf_new, *buf_len_new, fmt_str, div_result, div_mod);
    *buf_len_new = strlen(buf_new);
    tmp_len = *buf_len_new;

    if ( decimal > 0) {
        for(i = 0; i < *buf_len_new; i++) {
            if (buf_new[i] == '.') {
                if( i + decimal + 2 <= *buf_len_new ) {
                    buf_new[i + decimal + 1 ] = '\n';
                    buf_new[i + decimal + 2 ] = '\0';
                    *buf_len_new = strlen(buf_new);
                    DBG_DEBUG(DBG_VERBOSE, "deal decimal[%d] ok, str len:%d, value:%s\n",
                        decimal, *buf_len_new, buf_new);
                }
                break;
            }
        }
        if (tmp_len == *buf_len_new) {
            DBG_DEBUG(DBG_WARN, "deal decimal[%d] failed, use original value:%s\n", decimal, buf_new);
        }
    }
    return DFD_RV_OK;
}

static int dfd_get_sensor_info(uint8_t main_dev_id, uint8_t dev_index, uint8_t sensor_type,
            uint8_t sensor_index, uint8_t sensor_attr, char *buf)
{
    uint32_t key;
    uint16_t key_index1;
    uint8_t key_index2;
    int rv;
    info_hwmon_buf_f pfunc;

    key_index1 = DFD_GET_TEMP_SENSOR_KEY1(dev_index, sensor_index);
    key_index2 = DFD_GET_TEMP_SENSOR_KEY2(main_dev_id, sensor_attr);
    if (sensor_type == WB_MINOR_DEV_TEMP ) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_TEMP, key_index1, key_index2);
    } else if (sensor_type == WB_MINOR_DEV_IN) {
        key = DFD_CFG_KEY(DFD_CFG_ITEM_HWMON_IN, key_index1, key_index2);
    } else {
        DFD_SENSOR_DEBUG(DBG_ERROR, "unknow sensor type:%d.\n",sensor_type);
        return -DFD_RV_INVALID_VALUE;
    }

    DFD_SENSOR_DEBUG(DBG_VERBOSE, "get sensor info.main_dev_id:%d, dev_index:0x%x, sensor_index:0x%x, sensor_attr:0x%x, key:0x%x,\n",
        main_dev_id, dev_index, sensor_index, sensor_attr, key);

    pfunc = dfd_deal_hwmon_buf;
    mem_clear(buf, PAGE_SIZE);
    rv = dfd_info_get_sensor(key, buf, PAGE_SIZE, pfunc);
    return rv;
}

ssize_t dfd_get_temp_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t temp_index, uint8_t temp_attr, char *buf)
{
    int rv;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error. buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_TEMP, temp_index, temp_attr, buf);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get temp info error. rv:%d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get temp info ok.value:%s\n", buf);
    }
    return rv;
}

ssize_t dfd_get_voltage_info(uint8_t main_dev_id, uint8_t dev_index,
            uint8_t in_index, uint8_t in_attr, char *buf)
{
    int rv;

    if (buf == NULL) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "param error. buf is NULL.\n");
        return -DFD_RV_INVALID_VALUE;
    }

    rv = dfd_get_sensor_info(main_dev_id, dev_index, WB_MINOR_DEV_IN, in_index, in_attr, buf);
    if (rv < 0) {
        DFD_SENSOR_DEBUG(DBG_ERROR, "get voltage info error. rv:%d\n", rv);
    } else {
        DFD_SENSOR_DEBUG(DBG_VERBOSE, "get voltage info ok.value:%s\n", buf);
    }
    return rv;
}
