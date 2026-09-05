/*
 * Copyright(C) 2001-2012 Ruijie Network. All rights reserved.
 */

#include <linux/module.h>

#include "./include/dfd_module.h"
#include "./include/dfd_cfg.h"
#include "./include/dfd_cfg_info.h"
#include "./include/dfd_cfg_adapter.h"
#include "../rg_dev_sysfs/include/rg_sysfs_common.h"

int g_dfd_sff_dbg_level = 0;
module_param(g_dfd_sff_dbg_level, int, S_IRUGO | S_IWUSR);

extern ssize_t sfp_show_atrr(int sfp_bus, int sfp_mode, const char *attr_name, char *buf, int buf_len);

int dfd_get_sff_id(unsigned int sff_index)
{
    int key;
    int *p_sff_id;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_ID, sff_index, 0);
    p_sff_id = dfd_ko_cfg_get_item(key);
    if (p_sff_id == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff id error, key:0x%x\n", key);
        return -DFD_RV_NO_NODE;
    }
    DFD_SFF_DEBUG(DBG_VERBOSE, "get sff id ok, sff index:%d, id:0x%x.\n", sff_index, *p_sff_id);
    return *p_sff_id;
}

int dfd_set_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, int value)
{
    int key, ret;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_set_int(key, value);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "set sff cpld reg error, key:0x%x,ret:%d.\n", key, ret);
        return ret;
    }

    return DFD_RV_OK;
}

ssize_t dfd_get_sff_cpld_info(unsigned int sff_index, int cpld_reg_type, char *buf, int len)
{
    int key, ret, value;

    if(buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf is NULL. sff_index:%d, cpld_reg_type:%d.\n",
            sff_index, cpld_reg_type);
        return -DFD_RV_INVALID_VALUE;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_CPLD_REG, sff_index, cpld_reg_type);
    ret = dfd_info_get_int(key, &value, NULL);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff cpld reg error, key:0x%x,ret:%d.\n", key, ret);
        return ret;
    }

    memset(buf, 0 , len);
    return (ssize_t)snprintf(buf, len, "%d\n", value);
}

ssize_t dfd_get_sff_eeprom_info(unsigned int sff_index, const char *attr_name, char *buf, int buf_len)
{
    int ret;
    int sff_id, sff_bus, sff_speed;

    if(!buf || !attr_name || buf_len <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error, buf or attr_name is NULL. sff index:%d, buf len:%d.\n",
            sff_index, buf_len);
        return -DFD_RV_INVALID_VALUE;
    }

    sff_id = dfd_get_sff_id(sff_index);
    if (sff_id < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff id error, sff_index:%d, ret:%d\n", sff_index, sff_id);
        return -DFD_RV_NODE_FAIL;

    }
    sff_bus = RG_GET_SFF_I2C_ADAPTER(sff_id);
    sff_speed = RG_GET_SFF_SPEED_MODE(sff_id);

    memset(buf, 0 , buf_len);
    ret = sfp_show_atrr(sff_bus, sff_speed, attr_name, buf, buf_len);
    if (ret < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "prase sff eeprom info error. sff index:%d, bus:%d, speed mode:%d, attr_name:%s.\n",
            sff_index, sff_bus, sff_speed, attr_name);
    }

    return ret;
}

ssize_t dfd_get_sff_dir_name(unsigned int sff_index, char *buf, int buf_len)
{
    int key;
    char *sff_dir_name;

    if (buf == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "param error. buf is NULL.sff index:%d", sff_index);
        return -DFD_RV_INVALID_VALUE;
    }

    memset(buf, 0, buf_len);

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_DIR_NAME, sff_index, 0);
    sff_dir_name = dfd_ko_cfg_get_item(key);
    if (sff_dir_name == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "sff dir name config error, key=0x%08x\n", key);
        return -DFD_RV_NODE_FAIL;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "%s\n", sff_dir_name);
    snprintf(buf, buf_len, "%s", sff_dir_name);
    return strlen(buf);

}

int dfd_get_sff_polling_size(void)
{
    int key;
    int *p_eeprom_size;

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POLLING_SIZE, 0, 0);

    p_eeprom_size = dfd_ko_cfg_get_item(key);
    if (p_eeprom_size == NULL) {
        DFD_SFF_DEBUG(DBG_VERBOSE, "get sff polling size error, maybe unsupport sff polling. key:0x%x\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    return *p_eeprom_size;
}

ssize_t dfd_get_sff_polling_data(unsigned int sff_index, char *buf, loff_t offset, size_t count)
{
    int key, eeprom_size;
    ssize_t rd_len;
    loff_t rd_addr;
    char *logic_dev_path;
    int *p_base_addr;

    if(buf == NULL || offset < 0 || count <= 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "params error, offset:%lld, count:%lu.\n", offset, count);
        return -DFD_RV_INVALID_VALUE;
    }

    eeprom_size = dfd_get_sff_polling_size();
    if (eeprom_size < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff polling size error, ret:%d.\n", eeprom_size);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POLLING_DATA_BASE_ADDR, sff_index, 0);
    p_base_addr = dfd_ko_cfg_get_item(key);
    if (p_base_addr == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff polling data base addr error, sff_index:%u, key:0x%x.\n",
            sff_index, key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    key = DFD_CFG_KEY(DFD_CFG_ITEM_SFF_POLLING_LOGIC_DEV_PATH, sff_index, 0);
    logic_dev_path = dfd_ko_cfg_get_item(key);
    if (logic_dev_path == NULL) {
        DFD_SFF_DEBUG(DBG_ERROR, "get sff polling logic device path error, key=0x%08x.\n", key);
        return -DFD_RV_DEV_NOTSUPPORT;
    }

    DFD_SFF_DEBUG(DBG_VERBOSE, "sff:%u, eeprom size:0x%x, base addr:0x%x, path:%s, offset:0x%llx, rd_len:%lu.\n",
        sff_index, eeprom_size, *p_base_addr, logic_dev_path, offset, count);

    if (offset >= eeprom_size) {
        DFD_SFF_DEBUG(DBG_VERBOSE, "offset:0x%llx, eeprom size:0x%x, EOF.\n",
            offset, eeprom_size);
        return 0;
    }

    if (offset + count > eeprom_size) {
        DFD_SFF_DEBUG(DBG_VERBOSE, "read count out of range. input len:%lu, read len:%llu.\n",
            count, eeprom_size - offset);
        count = eeprom_size - offset;
    }

    rd_addr = *p_base_addr + offset;
    rd_len = dfd_ko_read_file(logic_dev_path, rd_addr, buf, count);
    if (rd_len < 0) {
        DFD_SFF_DEBUG(DBG_ERROR, "read sff polling data failed, loc:%s, offset:0x%llx, rd_len:%lu, ret:%ld,\n",
            logic_dev_path, rd_addr, count, rd_len);
    } else {
        DFD_SFF_DEBUG(DBG_VERBOSE, "read sff polling data success, loc:%s, offset:0x%llx, rd_len:%lu, rd_len:%ld,\n",
            logic_dev_path, rd_addr, count, rd_len);
    }

    return rd_len;
}
