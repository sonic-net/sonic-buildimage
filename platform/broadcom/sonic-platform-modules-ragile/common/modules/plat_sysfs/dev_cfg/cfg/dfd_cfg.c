#include <linux/list.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "../include/dfd_module.h"
#include "../include/dfd_cfg_file.h"
#include "../include/dfd_cfg_listnode.h"
#include "../include/dfd_cfg_info.h"
#include "../include/dfd_cfg_adapter.h"
#include "../include/dfd_cfg.h"
#include "../../dev_sysfs/include/sysfs_common.h"

#ifdef DFD_CFG_ITEM
#undef DFD_CFG_ITEM
#endif
#define DFD_CFG_ITEM(_id, _name, _index_min, _index_max)    _name,
static char *dfd_cfg_item_name[] = {
    DFD_CFG_ITEM_ALL
};

#ifdef DFD_CFG_ITEM
#undef DFD_CFG_ITEM
#endif
#define DFD_CFG_ITEM(_id, _name, _index_min, _index_max)    {_index_min, _index_max},
static index_range_t dfd_cfg_item_index_range[] = {
    DFD_CFG_ITEM_ALL
};

static lnode_root_t dfd_ko_cfg_list_root;

static void dfd_ko_cfg_del_space_lf_cr(char *str)
{
    int i, j;
    int len;

    len = strlen(str);
    for (i = 0; i < len; i++) {
        if (str[i] == '\r' || str[i] == '\n' || str[i] == ' ') {
            for (j = i; j < len - 1; j++) {
                str[j] = str[j + 1];
            }
            str[j] = '\0';
            len--;
            i--;
        }
    }
}

static int dfd_ko_cfg_get_value_from_char(char *value_str, int32_t *value, int line_num)
{
    int value_tmp = 0;

    if (strlen(value_str) == 0) {
        DBG_DEBUG(DBG_WARN, "line%d: value str is empty\n", line_num);
        *value = DFD_CFG_EMPTY_VALUE;
        return 0;
    }

    if ((strlen(value_str) > 2) && (value_str[0] == '0')
            && (value_str[1] == 'x' || value_str[1] == 'X')) {
        value_tmp = (int32_t)simple_strtol(value_str, NULL, 16);
    } else {
        value_tmp = (int32_t)simple_strtol(value_str, NULL, 10);
    }

    *value = value_tmp;
    return 0;
}

static int dfd_ko_cfg_analyse_index(char *index_str, int *index1, int *index2, int line_num)
{
    int rv;
    char *index1_begin_char, *index2_begin_char;

    if (index_str[0] != '_') {
        DBG_DEBUG(DBG_ERROR, "line%d: no '-' between name and index1\n", line_num);
        return -1;
    }

    index1_begin_char = index_str;
    rv = dfd_ko_cfg_get_value_from_char(++index1_begin_char, index1, line_num);
    if (rv < 0) {
        return -1;
    }

    if (index2 == NULL) {
        return 0;
    }

    index2_begin_char = strchr(index1_begin_char, '_');
    if (index2_begin_char == NULL) {
        DBG_DEBUG(DBG_ERROR, "line%d: no '-' between index1 and index2\n", line_num);
        return -1;
    } else {
        rv = dfd_ko_cfg_get_value_from_char(++index2_begin_char, index2, line_num);
        if (rv < 0) {
            return -1;
        }
    }

    return 0;
}

static int dfd_ko_cfg_check_array_index(index_range_t *index_range, int *index1, int *index2,
            int line_num)
{

    if ((*index1 < 0) || (*index1 > index_range->index1_max)) {
        DBG_DEBUG(DBG_ERROR, "line%d: index1[%d] invalid, max=%d\n", line_num, *index1,
            index_range->index1_max);
        return -1;
    }

    if (index2 == NULL) {
        return 0;
    }

    if ((*index2 < 0) || (*index2 > index_range->index2_max)) {
        DBG_DEBUG(DBG_ERROR, "line%d: index2[%d] invalid, max=%d\n", line_num, *index2,
            index_range->index2_max);
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_get_index(char *index_str, index_range_t *index_range, int *index1,
            int *index2, int line_num)
{
    int rv;

    if (index_range->index2_max == INDEX_NOT_EXIST) {
        index2 = NULL;
    }

    rv = dfd_ko_cfg_analyse_index(index_str, index1, index2, line_num);
    if (rv < 0) {
        return -1;
    }

    rv = dfd_ko_cfg_check_array_index(index_range, index1, index2, line_num);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_add_int_item(int key, int value, int line_num)
{
    int rv;
    int *int_cfg;

    int_cfg = lnode_find_node(&dfd_ko_cfg_list_root, key);
    if (int_cfg == NULL) {

        int_cfg = (int *)kmalloc(sizeof(int), GFP_KERNEL);
        if (int_cfg == NULL) {
            DBG_DEBUG(DBG_ERROR, "line%d: kmalloc int fail\n", line_num);
            return -1;
        }

        *int_cfg = value;
        rv = lnode_insert_node(&dfd_ko_cfg_list_root, key, int_cfg);
        if (rv == 0) {
            DBG_DEBUG(DBG_VERBOSE, "line%d: add int item[%d] success, key=0x%08x\n", line_num, value, key);
        } else {
            kfree(int_cfg);
            int_cfg = NULL;
            DBG_DEBUG(DBG_ERROR, "line%d: add int item[%d] fail, key=0x%08x rv=%d \n", line_num, value, key, rv);
            return -1;
        }
    } else {

        DBG_DEBUG(DBG_WARN, "line%d: replace int item[%d->%d], key=0x%08x\n", line_num, *int_cfg, value, key);
        *int_cfg = value;
    }

    return 0;
}

static int dfd_ko_cfg_analyse_int_item(dfd_cfg_item_id_t cfg_item_id, char *arg_name, char *arg_value,
            char *cfg_pre, index_range_t *index_range, int line_num)
{
    int rv;
    int index1 = 0, index2 = 0;
    int value, key;
    char *arg_name_tmp;

    if (index_range->index1_max != INDEX_NOT_EXIST) {
        arg_name_tmp = arg_name + strlen(cfg_pre);
        rv = dfd_ko_cfg_get_index(arg_name_tmp, index_range, &index1, &index2, line_num);
        if (rv < 0) {
            return -1;
        }
    }

    rv = dfd_ko_cfg_get_value_from_char(arg_value, &value, line_num);
    if (rv < 0) {
        return -1;
    }

    key = DFD_CFG_KEY(cfg_item_id, index1, index2);
    rv = dfd_ko_cfg_add_int_item(key, value, line_num);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_add_str_item(int key, char *str, int line_num)
{
    int rv;
    char *str_cfg;

    str_cfg = lnode_find_node(&dfd_ko_cfg_list_root, key);
    if (str_cfg == NULL) {

        str_cfg = (char *)kmalloc(DFD_CFG_STR_MAX_LEN, GFP_KERNEL);
        if (str_cfg == NULL) {
            DBG_DEBUG(DBG_ERROR, "line%d: kmalloc str[%lu] fail\n", line_num, strlen(str));
            return -1;
        }
        mem_clear(str_cfg, DFD_CFG_STR_MAX_LEN);
        strncpy(str_cfg, str, DFD_CFG_STR_MAX_LEN - 1);

        rv = lnode_insert_node(&dfd_ko_cfg_list_root, key, str_cfg);
        if (rv == 0) {
            DBG_DEBUG(DBG_VERBOSE, "line%d: add string item[%s] success, key=0x%08x\n", line_num, str_cfg, key);
        } else {
            kfree(str_cfg);
            str_cfg = NULL;
            DBG_DEBUG(DBG_ERROR, "line%d: add string item[%s] fail, key=0x%08x rv=%d \n", line_num, str_cfg, key, rv);
            return -1;
        }
    } else {
        DBG_DEBUG(DBG_WARN, "line%d: replace string item[%s->%s], key=0x%08x\n", line_num, str_cfg, str, key);
        mem_clear(str_cfg, DFD_CFG_STR_MAX_LEN);
        strncpy(str_cfg, str, DFD_CFG_STR_MAX_LEN - 1);
    }

    return 0;
}

static int dfd_ko_cfg_analyse_str_item(dfd_cfg_item_id_t cfg_item_id, char *arg_name, char *arg_value,
            char *cfg_pre, index_range_t *index_range, int line_num)
{
    int rv;
    int index1 = 0, index2 = 0;
    int btree_key;
    char *arg_name_tmp;

    if (index_range->index1_max != INDEX_NOT_EXIST) {
        arg_name_tmp = arg_name + strlen(cfg_pre);
        rv = dfd_ko_cfg_get_index(arg_name_tmp, index_range, &index1, &index2, line_num);
        if (rv < 0) {
            return -1;
        }
    }

    if (strlen(arg_value) >= DFD_CFG_STR_MAX_LEN) {
        DBG_DEBUG(DBG_ERROR, "line%d: string item[%s] is too long \n", line_num, arg_value);
        return -1;
    }

    btree_key = DFD_CFG_KEY(cfg_item_id, index1, index2);
    rv = dfd_ko_cfg_add_str_item(btree_key, arg_value, line_num);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_get_i2c_dev_member(char *member_str, dfd_i2c_dev_mem_t *member, int line_num)
{
    dfd_i2c_dev_mem_t mem_index;

    for (mem_index = DFD_I2C_DEV_MEM_BUS; mem_index < DFD_I2C_DEV_MEM_END; mem_index++) {
        if (memcmp(member_str, g_dfd_i2c_dev_mem_str[mem_index],
                strlen(g_dfd_i2c_dev_mem_str[mem_index])) == 0) {
            *member =  mem_index;
            return 0;
        }
    }

    DBG_DEBUG(DBG_ERROR, "line%d: i2c dev member[%s] invalid\n", line_num, member_str);
    return -1;
}

static void dfd_ko_cfg_set_i2c_dev_mem_value(dfd_i2c_dev_t *i2c_dev, dfd_i2c_dev_mem_t member,
                int value)
{
    switch (member) {
    case DFD_I2C_DEV_MEM_BUS:
        i2c_dev->bus = value;
        break;
    case DFD_I2C_DEV_MEM_ADDR:
        i2c_dev->addr = value;
        break;
    default:
        break;
    }
}

static int dfd_ko_cfg_add_i2c_dev_item(int key, dfd_i2c_dev_mem_t member, int value, int line_num)
{
    int rv;
    dfd_i2c_dev_t *i2c_dev_cfg;

    i2c_dev_cfg = lnode_find_node(&dfd_ko_cfg_list_root, key);
    if (i2c_dev_cfg == NULL) {

        i2c_dev_cfg = (dfd_i2c_dev_t *)kmalloc(sizeof(dfd_i2c_dev_t), GFP_KERNEL);
        if (i2c_dev_cfg == NULL) {
            DBG_DEBUG(DBG_ERROR, "line%d: kmalloc i2c_dev fail\n", line_num);
            return -1;
        }
        mem_clear(i2c_dev_cfg, sizeof(dfd_i2c_dev_t));

        dfd_ko_cfg_set_i2c_dev_mem_value(i2c_dev_cfg, member, value);
        rv = lnode_insert_node(&dfd_ko_cfg_list_root, key, i2c_dev_cfg);
        if (rv == 0) {
            DBG_DEBUG(DBG_VERBOSE, "line%d: add i2c_dev item[%s=%d] success, key=0x%08x\n", line_num,
                g_dfd_i2c_dev_mem_str[member], value, key);
        } else {
            kfree(i2c_dev_cfg);
            i2c_dev_cfg = NULL;
            DBG_DEBUG(DBG_ERROR, "line%d: add i2c_dev item[%s=%d] fail, key=0x%08x rv=%d\n", line_num,
                g_dfd_i2c_dev_mem_str[member], value, key, rv);
            return -1;
        }
    } else {

        DBG_DEBUG(DBG_VERBOSE, "line%d: replace i2c_dev item[%s=%d], key=0x%08x\n", line_num,
            g_dfd_i2c_dev_mem_str[member], value, key);
        dfd_ko_cfg_set_i2c_dev_mem_value(i2c_dev_cfg, member, value);
    }

    return 0;
}

static int dfd_ko_cfg_analyse_i2c_dev_item(dfd_cfg_item_id_t cfg_item_id, char *arg_name,
            char *arg_value, char *cfg_pre, index_range_t *index_range, int line_num)
{
    int rv;
    int index1 = 0, index2 = 0;
    int value, key;
    char *arg_name_tmp;
    dfd_i2c_dev_mem_t member;

    arg_name_tmp = arg_name + strlen(cfg_pre);
    rv = dfd_ko_cfg_get_i2c_dev_member(arg_name_tmp, &member, line_num);
    if (rv < 0) {
        return -1;
    }

    if (index_range->index1_max != INDEX_NOT_EXIST) {
        arg_name_tmp += strlen(g_dfd_i2c_dev_mem_str[member]);
        rv = dfd_ko_cfg_get_index(arg_name_tmp, index_range, &index1, &index2, line_num);
        if (rv < 0) {
            return -1;
        }
    }

    rv = dfd_ko_cfg_get_value_from_char(arg_value, &value, line_num);
    if (rv < 0) {
        return -1;
    }

    key = DFD_CFG_KEY(cfg_item_id, index1, index2);
    rv = dfd_ko_cfg_add_i2c_dev_item(key, member, value, line_num);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_get_enum_value_by_str(char *enum_val_str[], int enum_val_end, char *buf)
{
    int i;
    int enum_val;

    enum_val = DFD_CFG_INVALID_VALUE;
    for (i = 0; i < enum_val_end; i++) {
        if (memcmp(buf, enum_val_str[i], strlen(enum_val_str[i])) == 0) {
            enum_val = i;
            break;
        }
    }

    return enum_val;
}

static int dfd_ko_cfg_get_info_ctrl_member(char *member_str, info_ctrl_mem_t *member, int line_num)
{
    info_ctrl_mem_t mem_index;

    for (mem_index = INFO_CTRL_MEM_MODE; mem_index < INFO_CTRL_MEM_END; mem_index++) {
        if (memcmp(member_str, g_info_ctrl_mem_str[mem_index],
                strlen(g_info_ctrl_mem_str[mem_index])) == 0) {
            *member =  mem_index;
            return 0;
        }
    }

    DBG_DEBUG(DBG_ERROR, "line%d: info ctrl member[%s] invalid\n", line_num, member_str);
    return -1;
}

static void dfd_ko_cfg_set_info_ctrl_mem_value(info_ctrl_t *info_ctrl, info_ctrl_mem_t member,
                char *buf_val, int line_num)
{
    switch (member) {
    case INFO_CTRL_MEM_MODE:
        info_ctrl->mode = dfd_ko_cfg_get_enum_value_by_str(g_info_ctrl_mode_str, INFO_CTRL_MODE_END, buf_val);;
        break;
    case INFO_CTRL_MEM_INT_CONS:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->int_cons), line_num);
        break;
    case INFO_CTRL_MEM_SRC:
        info_ctrl->src = dfd_ko_cfg_get_enum_value_by_str(g_info_src_str, INFO_SRC_END, buf_val);
        break;
    case INFO_CTRL_MEM_FRMT:
        info_ctrl->frmt = dfd_ko_cfg_get_enum_value_by_str(g_info_frmt_str, INFO_FRMT_END, buf_val);
        break;
    case INFO_CTRL_MEM_POLA:
        info_ctrl->pola = dfd_ko_cfg_get_enum_value_by_str(g_info_pola_str, INFO_POLA_END, buf_val);
        break;
    case INFO_CTRL_MEM_FPATH:
        mem_clear(info_ctrl->fpath, sizeof(info_ctrl->fpath));
        strncpy(info_ctrl->fpath, buf_val, sizeof(info_ctrl->fpath) - 1);
        break;
    case INFO_CTRL_MEM_ADDR:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->addr), line_num);
        break;
    case INFO_CTRL_MEM_LEN:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->len), line_num);
        break;
    case INFO_CTRL_MEM_BIT_OFFSET:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->bit_offset), line_num);
        break;
    case INFO_CTRL_MEM_STR_CONS:
        mem_clear(info_ctrl->str_cons, sizeof(info_ctrl->str_cons));
        strncpy(info_ctrl->str_cons, buf_val, sizeof(info_ctrl->str_cons) - 1);
        break;
    case INFO_CTRL_MEM_INT_EXTRA1:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->int_extra1), line_num);
        break;
    case INFO_CTRL_MEM_INT_EXTRA2:
        dfd_ko_cfg_get_value_from_char(buf_val, &(info_ctrl->int_extra2), line_num);
        break;
    default:
        break;
    }
}

static int dfd_ko_cfg_add_info_ctrl_item(int key, info_ctrl_mem_t member, char *buf_val,
            int line_num)
{
    int rv;
    info_ctrl_t *info_ctrl_cfg;

    info_ctrl_cfg = lnode_find_node(&dfd_ko_cfg_list_root, key);
    if (info_ctrl_cfg == NULL) {

        info_ctrl_cfg = (info_ctrl_t *)kmalloc(sizeof(info_ctrl_t), GFP_KERNEL);
        if (info_ctrl_cfg == NULL) {
            DBG_DEBUG(DBG_ERROR, "line%d: kmalloc info_ctrl fail\n", line_num);
            return -1;
        }
        mem_clear(info_ctrl_cfg, sizeof(info_ctrl_t));

        dfd_ko_cfg_set_info_ctrl_mem_value(info_ctrl_cfg, member, buf_val, line_num);
        rv = lnode_insert_node(&dfd_ko_cfg_list_root, key, info_ctrl_cfg);
        if (rv == 0) {
            DBG_DEBUG(DBG_VERBOSE, "line%d: add info_ctrl item[%s=%s] success, key=0x%08x\n", line_num,
                g_info_ctrl_mem_str[member], buf_val, key);
        } else {
            kfree(info_ctrl_cfg);
            info_ctrl_cfg = NULL;
            DBG_DEBUG(DBG_ERROR, "line%d: add info_ctrl item[%s=%s] fail, key=0x%08x rv=%d\n", line_num,
                g_info_ctrl_mem_str[member], buf_val, key, rv);
            return -1;
        }
    } else {

        DBG_DEBUG(DBG_VERBOSE, "line%d: replace info_ctrl item[%s=%s], key=0x%08x\n", line_num,
            g_info_ctrl_mem_str[member], buf_val, key);
        dfd_ko_cfg_set_info_ctrl_mem_value(info_ctrl_cfg, member, buf_val, line_num);
    }

    return 0;
}

static int dfd_ko_cfg_analyse_info_ctrl_item(dfd_cfg_item_id_t cfg_item_id, char *arg_name,
            char *arg_value, char *cfg_pre, index_range_t *index_range, int line_num)
{
    int rv;
    int index1 = 0, index2 = 0;
    int key;
    char *arg_name_tmp;
    info_ctrl_mem_t member;

    arg_name_tmp = arg_name + strlen(cfg_pre);
    rv = dfd_ko_cfg_get_info_ctrl_member(arg_name_tmp, &member, line_num);
    if (rv < 0) {
        return -1;
    }

    if (index_range->index1_max != INDEX_NOT_EXIST) {
        arg_name_tmp += strlen(g_info_ctrl_mem_str[member]);
        rv = dfd_ko_cfg_get_index(arg_name_tmp, index_range, &index1, &index2, line_num);
        if (rv < 0) {
            return -1;
        }
    }

    key = DFD_CFG_KEY(cfg_item_id, index1, index2);
    rv = dfd_ko_cfg_add_info_ctrl_item(key, member, arg_value, line_num);
    if (rv < 0) {
        return -1;
    }

    return 0;
}

static int dfd_ko_cfg_analyse_config(char *arg_name, char*arg_value, int line_num)
{
    int i, rv = 0;
    int cfg_item_num;

    cfg_item_num = sizeof(dfd_cfg_item_name) / sizeof(dfd_cfg_item_name[0]);
    for (i = 0; i < cfg_item_num; i++) {
        if (memcmp(arg_name, dfd_cfg_item_name[i], strlen(dfd_cfg_item_name[i])) == 0){
            if (DFD_CFG_ITEM_IS_INT(i)) {
                rv = dfd_ko_cfg_analyse_int_item(i, arg_name, arg_value, dfd_cfg_item_name[i],
                    &(dfd_cfg_item_index_range[i]), line_num);
            } else if (DFD_CFG_ITEM_IS_STRING(i)) {
                rv = dfd_ko_cfg_analyse_str_item(i, arg_name, arg_value, dfd_cfg_item_name[i],
                    &(dfd_cfg_item_index_range[i]), line_num);
            } else if (DFD_CFG_ITEM_IS_I2C_DEV(i)) {
                rv = dfd_ko_cfg_analyse_i2c_dev_item(i, arg_name, arg_value, dfd_cfg_item_name[i],
                    &(dfd_cfg_item_index_range[i]), line_num);
            } else if (DFD_CFG_ITEM_IS_INFO_CTRL(i)) {
                rv = dfd_ko_cfg_analyse_info_ctrl_item(i, arg_name, arg_value, dfd_cfg_item_name[i],
                    &(dfd_cfg_item_index_range[i]), line_num);
            } else {
                rv = -1;
            }
            break;
        }
    }

    return rv;
}

static int dfd_ko_cfg_cut_config_line(char *config_line, char *arg_name, char *arg_value)
{
    int i, j = 0, k = 0;
    int len, name_value_flag = 0;

    len = strlen(config_line);
    for (i = 0; i < len; i++) {
        if (config_line[i] == '=') {
            name_value_flag = 1;
            continue;
        }

        if (name_value_flag == 0) {
            arg_name[j++] = config_line[i];
        } else {
            arg_value[k++] = config_line[i];
        }
    }

    if (name_value_flag == 0) {
        return -1;
    } else {
        return 0;
    }
}

static int dfd_ko_cfg_analyse_config_line(char *config_line, int line_num)
{
    int rv;
    char arg_name[DFD_CFG_NAME_MAX_LEN] = {0};
    char arg_value[DFD_CFG_VALUE_MAX_LEN] = {0};

    dfd_ko_cfg_del_space_lf_cr(config_line);

    if (strlen(config_line) == 0) {
        DBG_DEBUG(DBG_VERBOSE, "line%d: space line\n", line_num);
        return 0;
    }

    if (config_line[0] == '#') {
        DBG_DEBUG(DBG_VERBOSE, "line%d: comment line[%s]\n", line_num, config_line);
        return 0;
    }

    rv = dfd_ko_cfg_cut_config_line(config_line, arg_name, arg_value);
    if (rv < 0) {
        DBG_DEBUG(DBG_VERBOSE, "line%d: [%s]no '=' between name and value\n", line_num, config_line);
        return -1;
    }

    DBG_DEBUG(DBG_VERBOSE, "line%d: config_line[%s] name[%s] value[%s]\n", line_num, config_line, arg_name, arg_value);
    return dfd_ko_cfg_analyse_config(arg_name, arg_value, line_num);
}

static int dfd_ko_cfg_analyse_config_file(char *fpath)
{
    int rv;
    int line_num = 1;
    kfile_ctrl_t kfile_ctrl;
    char config_line[DFD_CFG_CMDLINE_MAX_LEN] = {0};

    rv = kfile_open(fpath, &kfile_ctrl);
    if (rv != KFILE_RV_OK) {
        DBG_DEBUG(DBG_ERROR, "open config file[%s] fail, rv=%d\n", fpath, rv);
        return -1;
    }

    while(kfile_gets(config_line, sizeof(config_line), &kfile_ctrl) > 0){
        rv = dfd_ko_cfg_analyse_config_line(config_line, line_num++);
        if (rv < 0) {
            DBG_DEBUG(DBG_ERROR, "!!!!file[%s] config line[%d %s] analyse fail\n", fpath, line_num - 1,
                config_line);
            break;
        }

        (void)mem_clear(config_line, sizeof(config_line));

    }
    kfile_close(&kfile_ctrl);

    return rv;
}

void *dfd_ko_cfg_get_item(int key)
{
    return lnode_find_node(&dfd_ko_cfg_list_root, key);
}

static void dfd_ko_cfg_print_item(int key, const void *cfg)
{
    int item_id;
    dfd_i2c_dev_t *i2c_dev;
    info_ctrl_t *info_ctrl;

    if (cfg == NULL) {
        DBG_DEBUG(DBG_ERROR, "input arguments error\n");
        return;
    }
    printk(KERN_INFO "**************************\n");
    printk(KERN_INFO "key=0x%08x\n", key);

    item_id = DFD_CFG_ITEM_ID(key);
    if (DFD_CFG_ITEM_IS_INT(item_id)) {
        printk(KERN_INFO "int=%d\n", *((int *)cfg));
    } else if (DFD_CFG_ITEM_IS_I2C_DEV(item_id)) {
        i2c_dev = (dfd_i2c_dev_t *)cfg;
        printk(KERN_INFO ".bus=0x%02x\n", i2c_dev->bus);
        printk(KERN_INFO ".addr=0x%02x\n", i2c_dev->addr);
    } else if (DFD_CFG_ITEM_IS_INFO_CTRL(item_id)) {
        info_ctrl = (info_ctrl_t *)cfg;
        printk(KERN_INFO ".mode=%s\n", g_info_ctrl_mode_str[info_ctrl->mode]);
        printk(KERN_INFO ".int_cons=%d\n", info_ctrl->int_cons);
        printk(KERN_INFO ".src=%s\n", g_info_src_str[info_ctrl->src]);
        printk(KERN_INFO ".frmt=%s\n", g_info_frmt_str[info_ctrl->frmt]);
        printk(KERN_INFO ".pola=%s\n", g_info_pola_str[info_ctrl->pola]);
        printk(KERN_INFO ".fpath=%s\n", info_ctrl->fpath);
        printk(KERN_INFO ".addr=0x%02x\n", info_ctrl->addr);
        printk(KERN_INFO ".len=%d\n", info_ctrl->len);
        printk(KERN_INFO ".bit_offset=%d\n", info_ctrl->bit_offset);
    } else {
        printk(KERN_INFO "item[%d] error!\n", item_id);
    }
}

void dfd_ko_cfg_show_item(int key)
{
    void *cfg;

    cfg = lnode_find_node(&dfd_ko_cfg_list_root, key);
    if (cfg == 0) {
        printk(KERN_INFO "item[0x%08x] not exist\n", key);
        return;
    }

    dfd_ko_cfg_print_item(key, cfg);
}

static int dfd_get_my_dev_type_by_file(void)
{
    struct file *fp;
    loff_t pos;
    int card_type;
    char buf[DFD_PID_BUF_LEN];
    int ret;

    fp= filp_open(DFD_PUB_CARDTYPE_FILE, O_RDONLY, 0);
    if (IS_ERR(fp)) {
        DBG_DEBUG(DBG_VERBOSE, "open file fail!\n");
        return -1;
    }
    mem_clear(buf, DFD_PID_BUF_LEN);
    pos = 0;
    ret = kernel_read(fp, buf, DFD_PRODUCT_ID_LENGTH + 1, &pos);
    if (ret < 0) {
        DBG_DEBUG(DBG_VERBOSE, "kernel_read failed, path=%s, addr=0, size=%d, ret=%d\n",
            DFD_PUB_CARDTYPE_FILE, DFD_PRODUCT_ID_LENGTH + 1, ret);
        filp_close(fp, NULL);
        return -1;
    }

    card_type = simple_strtoul(buf, NULL, 10);
    DBG_DEBUG(DBG_VERBOSE, "card_type 0x%x.\n", card_type);

    filp_close(fp, NULL);
    return card_type;
}

static int drv_get_my_dev_type(void)
{
    static int type = -1;

    if (type > 0) {
        return type;
    }
    type = dfd_get_my_dev_type_by_file();
    DBG_DEBUG(DBG_VERBOSE, "ko board type %d\n", type);
    return type;
}

static int dfd_ko_cfg_init(void)
{
    int rv;
    int card_type;
    char file_name[32] = {0};
    char fpath[128] = {0};
    kfile_ctrl_t kfile_ctrl;

    rv = lnode_init_root(&dfd_ko_cfg_list_root);
    if (rv < 0) {
        DBG_DEBUG(DBG_ERROR, "init list root fail, rv=%d\n", rv);
        return -1;
    }

    card_type = drv_get_my_dev_type();
    if (card_type > 0) {
        snprintf(fpath, sizeof(fpath), "%s0x%x", DFD_KO_CFG_FILE_DIR, card_type);
        rv = kfile_open(fpath, &kfile_ctrl);
        if (rv != KFILE_RV_OK) {
            DBG_DEBUG(DBG_VERBOSE, "open config file[%s] fail, rv=%d, maybe not exist\n",
                fpath, rv);

            rv = kfile_open(DFD_KO_CFG_FILE_NAME, &kfile_ctrl);
            if (rv != KFILE_RV_OK) {
                DBG_DEBUG(DBG_ERROR, "open config file[%s] fail, rv=%d\n", DFD_KO_CFG_FILE_NAME,
                    rv);
                return -1;
            }
            DBG_DEBUG(DBG_ERROR, "get config file from: %s, success.\n", DFD_KO_CFG_FILE_NAME);
        } else {
            DBG_DEBUG(DBG_VERBOSE, "get config file from: %s, success.\n", fpath);
        }
    } else {
        DBG_DEBUG(DBG_VERBOSE, "get board id failed, try to get config file from: %s\n",
            DFD_KO_CFG_FILE_NAME);

        rv = kfile_open(DFD_KO_CFG_FILE_NAME, &kfile_ctrl);
        if (rv != KFILE_RV_OK) {
            DBG_DEBUG(DBG_ERROR, "open config file[%s] fail, rv=%d\n", DFD_KO_CFG_FILE_NAME, rv);
            return -1;
        }
        DBG_DEBUG(DBG_ERROR, "get config file from: %s, success.\n", DFD_KO_CFG_FILE_NAME);
    }

    while (kfile_gets(file_name, sizeof(file_name), &kfile_ctrl) > 0) {

        dfd_ko_cfg_del_space_lf_cr(file_name);
        mem_clear(fpath, sizeof(fpath));
        snprintf(fpath, sizeof(fpath), "%s%s.cfg", DFD_KO_CFG_FILE_DIR, file_name);
        DBG_DEBUG(DBG_VERBOSE, ">>>>start parsing config file[%s]\n", fpath);

        rv = dfd_ko_cfg_analyse_config_file(fpath);
        if (rv < 0) {
            break;
        }
    }
    kfile_close(&kfile_ctrl);

    return 0;
}

int32_t dfd_dev_cfg_init(void)
{
    return dfd_ko_cfg_init();
}

void dfd_dev_cfg_exit(void)
{
    lnode_free_list(&dfd_ko_cfg_list_root);
    return;
}
