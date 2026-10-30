/*
 * wb_fpga_seu_drv.c
 * ko to create fpga seu device
 */
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/version.h>

#include <wb_logic_dev_common.h>
#include <wb_bsp_kernel_debug.h>
#include "wb_fpga_seu.h"

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define DRV_NAME                "wb-fpga-seu"
#define DRV_TYPE                "fpga_seu"

typedef struct wb_fpga_seu_s {
    struct kobject kobj;
    const struct attribute_group *sysfs_group;
    struct device *dev;
    struct mutex update_lock;
    const char *name;
    const char *dev_name;
    uint32_t logic_func_mode;
    uint32_t seu_data_reg;
    uint32_t seu_data_status_reg;
    uint32_t seu_data_ctrl_reg;
    uint32_t seu_data_cnt_reg;
    uint32_t seu_data_len;
    uint8_t seu_status_valid_mask;
    uint8_t seu_ctrl_clear_mask;
    unsigned long write_intf_addr;
    unsigned long read_intf_addr;
} wb_fpga_seu_t;

static int fpga_seu_device_write(wb_fpga_seu_t *fpga_seu, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_write pfunc;

    pfunc = (device_func_write)fpga_seu->write_intf_addr;
    return pfunc(fpga_seu->dev_name, pos, val, size);
}

static int fpga_seu_device_read(wb_fpga_seu_t *fpga_seu, uint32_t pos, uint8_t *val, size_t size)
{
    device_func_read pfunc;

    pfunc = (device_func_read)fpga_seu->read_intf_addr;
    return pfunc(fpga_seu->dev_name, pos, val, size);
}

static int fpga_seu_reg_read(wb_fpga_seu_t *fpga_seu, uint32_t addr, uint8_t *val)
{
    int ret;

    ret = fpga_seu_device_read(fpga_seu, addr, val, sizeof(uint8_t));
    if (ret < 0) {
        DEBUG_ERROR("fpga seu reg read failed, dev name:%s, offset:0x%x\n",
            fpga_seu->dev_name, addr);
        return -EIO;
    }

    return 0;
}

static int fpga_seu_reg_write(wb_fpga_seu_t *fpga_seu, uint32_t addr, uint8_t val)
{
    int ret;

    ret = fpga_seu_device_write(fpga_seu, addr, &val, sizeof(uint8_t));
    if (ret < 0) {
        DEBUG_ERROR("fpga seu reg write failed, dev name:%s, offset:0x%x, value:0x%x\n",
            fpga_seu->dev_name, addr, val);
        return -EIO;
    }

    return 0;
}

static int fpga_seu_data_read(wb_fpga_seu_t *fpga_seu, uint32_t addr, uint8_t *val, size_t size)
{
    int ret;

    ret = fpga_seu_device_read(fpga_seu, addr, val, size);
    if (ret < 0) {
        DEBUG_ERROR("fpga seu data read failed, dev name:%s, offset:0x%x, size:%zu\n",
            fpga_seu->dev_name, addr, size);
        return -EIO;
    }

    return 0;
}

static int fpga_seu_get_valid_flag(wb_fpga_seu_t *fpga_seu, uint8_t *valid)
{
    int ret;
    uint8_t status;

    if (!fpga_seu || !valid) {
        if (fpga_seu && fpga_seu->dev) {
            DEBUG_ERROR("fpga_seu_get_valid_flag invalid param, fpga_seu:%p, valid:%p\n",
                fpga_seu, valid);
        }
        return -EINVAL;
    }

    status = 0;
    ret = fpga_seu_reg_read(fpga_seu, fpga_seu->seu_data_status_reg, &status);
    if (ret < 0) {
        DEBUG_ERROR("fpga_seu_get_valid_flag read status failed, reg:0x%x, ret:%d\n",
            fpga_seu->seu_data_status_reg, ret);
        return ret;
    }

    *valid = !!(status & fpga_seu->seu_status_valid_mask);
    return 0;
}

static int fpga_seu_get_valid_count(wb_fpga_seu_t *fpga_seu, uint8_t *valid, uint32_t *valid_count)
{
    int ret;
    uint8_t count;

    ret = fpga_seu_get_valid_flag(fpga_seu, valid);
    if (ret < 0) {
        DEBUG_ERROR("fpga seu get valid flag failed, dev name:%s\n", fpga_seu->dev_name);
        return ret;
    }

    if (!(*valid)) {
        *valid_count = 0;
        return 0;
    }

    count = 0;
    ret = fpga_seu_reg_read(fpga_seu, fpga_seu->seu_data_cnt_reg, &count);
    if (ret < 0) {
        DEBUG_ERROR("fpga seu get valid count failed, dev name:%s\n", fpga_seu->dev_name);
        return ret;
    }

    if (count == 0) {
        *valid_count = fpga_seu->seu_data_len;
    } else {
        *valid_count = count;
    }

    if (*valid_count > fpga_seu->seu_data_len) {
        *valid_count = fpga_seu->seu_data_len;
    }

    return 0;
}

static int fpga_seu_clear_data(wb_fpga_seu_t *fpga_seu)
{
    return fpga_seu_reg_write(fpga_seu, fpga_seu->seu_data_ctrl_reg, fpga_seu->seu_ctrl_clear_mask);
}

static int fpga_seu_get_data_locked(wb_fpga_seu_t *fpga_seu, uint8_t *data_buf,
    size_t data_buf_size, uint32_t *valid_count)
{
    uint8_t valid;
    int ret;

    if (!fpga_seu || !data_buf || !valid_count || data_buf_size == 0) {
        if (fpga_seu && fpga_seu->dev) {
            DEBUG_ERROR("fpga_seu_get_data_locked invalid param, fpga_seu:%p, data_buf:%p, valid_count:%p, data_buf_size:%zu\n",
                fpga_seu, data_buf, valid_count, data_buf_size);
        }
        return -EINVAL;
    }

    valid = 0;
    ret = fpga_seu_get_valid_count(fpga_seu, &valid, valid_count);
    if (ret < 0) {
        DEBUG_ERROR("fpga_seu_get_data_locked get_valid_count failed, ret:%d\n", ret);
        return ret;
    }

    if (*valid_count > data_buf_size) {
        *valid_count = data_buf_size;
    }

    mem_clear(data_buf, data_buf_size);
    if (valid && *valid_count > 0) {
        ret = fpga_seu_data_read(fpga_seu, fpga_seu->seu_data_reg, data_buf, *valid_count);
        if (ret < 0) {
            DEBUG_ERROR("fpga_seu_get_data_locked data_read failed, reg:0x%x, valid_count:%u, ret:%d\n",
                fpga_seu->seu_data_reg, *valid_count, ret);
            return ret;
        }
    }

    return 0;
}

static int fpga_seu_get_data_string(wb_fpga_seu_t *fpga_seu, char *data_buf, size_t data_buf_size,
    uint32_t *valid_count)
{
    uint32_t i;
    int ret;

    if (!fpga_seu || !data_buf || !valid_count || data_buf_size == 0) {
        if (fpga_seu && fpga_seu->dev) {
            DEBUG_ERROR("fpga_seu_get_data_string invalid param, fpga_seu:%p, data_buf:%p, valid_count:%p, data_buf_size:%zu\n",
                fpga_seu, data_buf, valid_count, data_buf_size);
        }
        return -EINVAL;
    }

    if (data_buf_size == 1) {
        data_buf[0] = '\0';
        *valid_count = 0;
        return 0;
    }

    mutex_lock(&fpga_seu->update_lock);
    ret = fpga_seu_get_data_locked(fpga_seu, (uint8_t *)data_buf, data_buf_size - 1, valid_count);
    mutex_unlock(&fpga_seu->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("fpga_seu_get_data_string get_data_locked failed, ret:%d\n", ret);
        return ret;
    }

    for (i = 0; i < *valid_count; i++) {
        if (data_buf[i] == '\r') {
            data_buf[i] = '\n';
        }
    }

    data_buf[*valid_count] = '\0';
    return 0;
}

static bool fpga_seu_line_has_cram_keyword(const char *line)
{
    if (!line) {
        DEBUG_ERROR("fpga_seu_line_has_cram_keyword invalid param, line:%p\n", line);
        return false;
    }

    return strstr(line, "SED") || strstr(line, "DED") || strstr(line, "PA") ||
        strstr(line, "LA") || strstr(line, "WD");
}

static int fpga_seu_get_cram_status(const char *data_buf)
{
    if (!data_buf) {
        DEBUG_ERROR("fpga_seu_get_cram_status invalid param, data_buf:%p\n", data_buf);
        return 0;
    }

    if (strstr(data_buf, "SED")) {
        return 1;
    }

    if (strstr(data_buf, "DED")) {
        return 2;
    }

    return 0;
}

static bool fpga_seu_cram_info_has_line(const char *buf, const char *line)
{
    const char *token;
    size_t line_len;

    if (!buf || !line || line[0] == '\0') {
        DEBUG_ERROR("fpga_seu_cram_info_has_line invalid param, buf:%p, line:%p\n", buf, line);
        return false;
    }

    line_len = strlen(line);
    token = buf;
    while (*token != '\0') {
        const char *token_end;
        size_t token_len;

        token_end = strchr(token, ',');
        if (token_end) {
            token_len = token_end - token;
        } else {
            token_len = strlen(token);
        }

        if (token_len == line_len && strncmp(token, line, line_len) == 0) {
            return true;
        }

        if (!token_end) {
            break;
        }
        token = token_end + 1;
    }

    return false;
}

static ssize_t fpga_seu_format_cram_info(const char *data_buf, char *buf, size_t buf_size)
{
    char cram_line_buf[FPGA_SEU_DATA_LEN_DEFAULT + 1];
    char line_buf[FPGA_SEU_DATA_LEN_DEFAULT + 1];
    char *trimmed_line;
    const char *line;
    const char *line_end;
    size_t cram_line_len;
    size_t line_len;
    size_t offset;
    size_t i;
    bool first_line;

    if (!data_buf || !buf || buf_size == 0) {
        DEBUG_ERROR("fpga_seu_format_cram_info invalid param, data_buf:%p, buf:%p, buf_size:%zu\n",
            data_buf, buf, buf_size);
        return -EINVAL;
    }

    mem_clear(buf, buf_size);
    offset = 0;
    first_line = true;
    line = data_buf;
    while (*line != '\0' && offset < (buf_size - 1)) {
        while (*line == '\r' || *line == '\n') {
            line++;
        }
        if (*line == '\0') {
            break;
        }

        line_end = strpbrk(line, "\r\n");
        if (line_end) {
            line_len = line_end - line;
        } else {
            line_len = strlen(line);
        }

        if (line_len > FPGA_SEU_DATA_LEN_DEFAULT) {
            line_len = FPGA_SEU_DATA_LEN_DEFAULT;
        }

        mem_clear(cram_line_buf, sizeof(cram_line_buf));
        mem_clear(line_buf, sizeof(line_buf));
        memcpy(line_buf, line, line_len);
        if (fpga_seu_line_has_cram_keyword(line_buf)) {
            cram_line_len = 0;
            for (i = 0; i < line_len && offset < (buf_size - 1); i++) {
                cram_line_buf[cram_line_len++] = line_buf[i];
            }

            trimmed_line = strim(cram_line_buf);
            if (trimmed_line[0] != '\0' && !fpga_seu_cram_info_has_line(buf, trimmed_line)) {
                if (!first_line && offset < (buf_size - 1)) {
                    buf[offset++] = ',';
                }

                for (i = 0; trimmed_line[i] != '\0' && offset < (buf_size - 1); i++) {
                    buf[offset++] = trimmed_line[i];
                }
                first_line = false;
            }
        }

        if (!line_end) {
            break;
        }
        line = line_end + 1;
    }

    buf[offset] = '\0';
    return offset;
}

static ssize_t fpga_seu_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);
    if (!attribute->show) {
        DEBUG_ERROR("fpga_seu_attr_show invalid param, attribute:%p\n", attribute);
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t fpga_seu_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf, size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);
    if (!attribute->store) {
        DEBUG_ERROR("fpga_seu_attr_store invalid param, attribute:%p\n", attribute);
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops fpga_seu_sysfs_ops = {
    .show = fpga_seu_attr_show,
    .store = fpga_seu_attr_store,
};

static void fpga_seu_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type fpga_seu_ktype = {
    .sysfs_ops = &fpga_seu_sysfs_ops,
    .release = fpga_seu_obj_release,
};

static ssize_t type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%s\n", DRV_TYPE);
}

static ssize_t info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    int offset;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu info_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "dev_name: %s\n", fpga_seu->dev_name);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "logic_func_mode: %u\n", fpga_seu->logic_func_mode);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "seu_data_reg: 0x%x\n", fpga_seu->seu_data_reg);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "seu_data_status_reg: 0x%x\n", fpga_seu->seu_data_status_reg);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "seu_data_ctrl_reg: 0x%x\n", fpga_seu->seu_data_ctrl_reg);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "seu_data_cnt_reg: 0x%x\n", fpga_seu->seu_data_cnt_reg);
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "seu_data_len: %u\n", fpga_seu->seu_data_len);
    return offset;
}

static ssize_t valid_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    uint8_t valid;
    int ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu valid_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    mutex_lock(&fpga_seu->update_lock);
    ret = fpga_seu_get_valid_flag(fpga_seu, &valid);
    mutex_unlock(&fpga_seu->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("valid_show get_valid_flag failed, ret:%d\n", ret);
        return ret;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", valid);
}

static ssize_t count_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    uint8_t valid;
    uint32_t valid_count;
    int ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu count_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    mutex_lock(&fpga_seu->update_lock);
    ret = fpga_seu_get_valid_count(fpga_seu, &valid, &valid_count);
    mutex_unlock(&fpga_seu->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("count_show get_valid_count failed, ret:%d\n", ret);
        return ret;
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%u\n", valid_count);
}

static ssize_t data_hex_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    uint8_t data_buf[FPGA_SEU_DATA_LEN_DEFAULT];
    uint32_t valid_count;
    int ret;
    int i;
    int offset;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu data_hex_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    mutex_lock(&fpga_seu->update_lock);
    ret = fpga_seu_get_data_locked(fpga_seu, data_buf, sizeof(data_buf), &valid_count);
    mutex_unlock(&fpga_seu->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("data_hex_show get_data_locked failed, ret:%d\n", ret);
        return ret;
    }

    mem_clear(buf, PAGE_SIZE);
    offset = 0;
    for (i = 0; i < valid_count; i++) {
        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%02x", data_buf[i]);
        if (i != (valid_count - 1)) {
            offset += scnprintf(buf + offset, PAGE_SIZE - offset, " ");
        }
    }
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\n");
    return offset;
}

static ssize_t data_str_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    char data_buf[FPGA_SEU_DATA_LEN_DEFAULT + 1];
    uint32_t valid_count;
    size_t copy_len;
    int ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu data_str_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    ret = fpga_seu_get_data_string(fpga_seu, data_buf, sizeof(data_buf), &valid_count);
    if (ret < 0) {
        DEBUG_ERROR("data_str_show get_data_string failed, ret:%d\n", ret);
        return ret;
    }

    mem_clear(buf, PAGE_SIZE);
    copy_len = min_t(size_t, valid_count, PAGE_SIZE - 2);
    memcpy(buf, data_buf, copy_len);
    buf[copy_len] = '\n';
    buf[copy_len + 1] = '\0';
    return copy_len + 1;
}

static ssize_t cram_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    char data_buf[FPGA_SEU_DATA_LEN_DEFAULT + 1];
    uint32_t valid_count;
    int cram_status;
    int ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu cram_status_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    ret = fpga_seu_get_data_string(fpga_seu, data_buf, sizeof(data_buf), &valid_count);
    if (ret < 0) {
        DEBUG_ERROR("cram_status_show get_data_string failed, ret:%d\n", ret);
        return ret;
    }

    cram_status = fpga_seu_get_cram_status(data_buf);
    if (cram_status == 0) {
        mutex_lock(&fpga_seu->update_lock);
        ret = fpga_seu_clear_data(fpga_seu);
        mutex_unlock(&fpga_seu->update_lock);
        if (ret < 0) {
            DEBUG_ERROR("cram_status_show clear_data failed, ret:%d\n", ret);
            return ret;
        }
    }

    mem_clear(buf, PAGE_SIZE);
    return snprintf(buf, PAGE_SIZE, "%d\n", cram_status);
}

static ssize_t cram_info_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    wb_fpga_seu_t *fpga_seu;
    char data_buf[FPGA_SEU_DATA_LEN_DEFAULT + 1];
    uint32_t valid_count;
    ssize_t ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu cram_info_show failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    ret = fpga_seu_get_data_string(fpga_seu, data_buf, sizeof(data_buf), &valid_count);
    if (ret < 0) {
        DEBUG_ERROR("cram_info_show get_data_string failed, ret:%zd\n", ret);
        return ret;
    }

    ret = fpga_seu_format_cram_info(data_buf, buf, PAGE_SIZE - 1);
    if (ret < 0) {
        DEBUG_ERROR("cram_info_show format_cram_info failed, ret:%zd\n", ret);
        return ret;
    }

    if (ret >= (PAGE_SIZE - 1)) {
        ret = PAGE_SIZE - 2;
    }
    buf[ret++] = '\n';
    buf[ret] = '\0';
    return ret;
}

static ssize_t clear_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    wb_fpga_seu_t *fpga_seu;
    uint8_t value;
    int ret;

    fpga_seu = container_of(kobj, wb_fpga_seu_t, kobj);
    if (!fpga_seu) {
        DEBUG_ERROR("fpga_seu clear_store failed, fpga_seu is NULL\n");
        return -ENODEV;
    }

    value = 0;
    ret = kstrtou8(buf, 0, &value);
    if (ret) {
        DEBUG_ERROR("clear_store parse value failed, buf:%s, ret:%d\n", buf, ret);
        return -EINVAL;
    }

    if (value == 0) {
        return count;
    }

    mutex_lock(&fpga_seu->update_lock);
    ret = fpga_seu_clear_data(fpga_seu);
    mutex_unlock(&fpga_seu->update_lock);
    if (ret < 0) {
        DEBUG_ERROR("clear_store clear_data failed, ret:%d\n", ret);
        return ret;
    }

    return count;
}

static struct kobj_attribute type_attribute = __ATTR(type, S_IRUGO, type_show, NULL);
static struct kobj_attribute info_attribute = __ATTR(info, S_IRUGO, info_show, NULL);
static struct kobj_attribute valid_attribute = __ATTR(valid, S_IRUGO, valid_show, NULL);
static struct kobj_attribute count_attribute = __ATTR(count, S_IRUGO, count_show, NULL);
static struct kobj_attribute data_hex_attribute = __ATTR(data_hex, S_IRUGO, data_hex_show, NULL);
static struct kobj_attribute data_str_attribute = __ATTR(data_str, S_IRUGO, data_str_show, NULL);
static struct kobj_attribute cram_status_attribute = __ATTR(cram_status, S_IRUGO, cram_status_show, NULL);
static struct kobj_attribute cram_info_attribute = __ATTR(cram_info, S_IRUGO, cram_info_show, NULL);
static struct kobj_attribute clear_attribute = __ATTR(clear, S_IWUSR, NULL, clear_store);

static struct attribute *fpga_seu_attrs[] = {
    &type_attribute.attr,
    &info_attribute.attr,
    &valid_attribute.attr,
    &count_attribute.attr,
    &data_hex_attribute.attr,
    &data_str_attribute.attr,
    &cram_status_attribute.attr,
    &cram_info_attribute.attr,
    &clear_attribute.attr,
    NULL,
};

static struct attribute_group fpga_seu_attr_group = {
    .attrs = fpga_seu_attrs,
};

static int fpga_seu_config_init(struct platform_device *pdev, wb_fpga_seu_t *fpga_seu)
{
    fpga_seu_device_t *fpga_seu_device;
    int ret;

    ret = 0;
    if (pdev->dev.of_node) {
        ret += of_property_read_string(pdev->dev.of_node, "seu_name", &fpga_seu->name);
        ret += of_property_read_string(pdev->dev.of_node, "dev_name", &fpga_seu->dev_name);
        ret += of_property_read_u32(pdev->dev.of_node, "logic_func_mode", &fpga_seu->logic_func_mode);
        ret += of_property_read_u32(pdev->dev.of_node, "seu_data_reg", &fpga_seu->seu_data_reg);
        ret += of_property_read_u32(pdev->dev.of_node, "seu_data_status_reg", &fpga_seu->seu_data_status_reg);
        ret += of_property_read_u32(pdev->dev.of_node, "seu_data_ctrl_reg", &fpga_seu->seu_data_ctrl_reg);
        ret += of_property_read_u32(pdev->dev.of_node, "seu_data_cnt_reg", &fpga_seu->seu_data_cnt_reg);
        if (ret != 0) {
            dev_err(&pdev->dev, "read fpga seu dt required properties failed, ret:%d\n", ret);
            return -EINVAL;
        }
        if (of_property_read_u32(pdev->dev.of_node, "seu_data_len", &fpga_seu->seu_data_len) != 0) {
            fpga_seu->seu_data_len = FPGA_SEU_DATA_LEN_DEFAULT;
        }
        if (of_property_read_u8(pdev->dev.of_node, "seu_status_valid_mask", &fpga_seu->seu_status_valid_mask) != 0) {
            fpga_seu->seu_status_valid_mask = FPGA_SEU_STATUS_VALID_MASK;
        }
        if (of_property_read_u8(pdev->dev.of_node, "seu_ctrl_clear_mask", &fpga_seu->seu_ctrl_clear_mask) != 0) {
            fpga_seu->seu_ctrl_clear_mask = FPGA_SEU_CTRL_CLEAR_MASK;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "fpga seu platform_data is NULL\n");
            return -ENXIO;
        }

        fpga_seu_device = pdev->dev.platform_data;
        fpga_seu->name = fpga_seu_device->seu_name;
        fpga_seu->dev_name = fpga_seu_device->dev_name;
        fpga_seu->logic_func_mode = fpga_seu_device->logic_func_mode;
        fpga_seu->seu_data_reg = fpga_seu_device->seu_data_reg;
        fpga_seu->seu_data_status_reg = fpga_seu_device->seu_data_status_reg;
        fpga_seu->seu_data_ctrl_reg = fpga_seu_device->seu_data_ctrl_reg;
        fpga_seu->seu_data_cnt_reg = fpga_seu_device->seu_data_cnt_reg;
        fpga_seu->seu_data_len = fpga_seu_device->seu_data_len;
        fpga_seu->seu_status_valid_mask = fpga_seu_device->seu_status_valid_mask;
        fpga_seu->seu_ctrl_clear_mask = fpga_seu_device->seu_ctrl_clear_mask;
    }

    if (fpga_seu->seu_data_len == 0 || fpga_seu->seu_data_len > FPGA_SEU_DATA_LEN_DEFAULT) {
        fpga_seu->seu_data_len = FPGA_SEU_DATA_LEN_DEFAULT;
    }
    if (fpga_seu->seu_status_valid_mask == 0) {
        fpga_seu->seu_status_valid_mask = FPGA_SEU_STATUS_VALID_MASK;
    }
    if (fpga_seu->seu_ctrl_clear_mask == 0) {
        fpga_seu->seu_ctrl_clear_mask = FPGA_SEU_CTRL_CLEAR_MASK;
    }

    ret = find_intf_addr(&fpga_seu->write_intf_addr, &fpga_seu->read_intf_addr, fpga_seu->logic_func_mode);
    if (ret) {
        dev_err(&pdev->dev, "find_intf_addr func mode %u fail, ret: %d\n", fpga_seu->logic_func_mode, ret);
        return ret;
    }

    if (!fpga_seu->write_intf_addr || !fpga_seu->read_intf_addr) {
        dev_err(&pdev->dev, "Fail: func mode %u rw symbol undefined\n", fpga_seu->logic_func_mode);
        return -ENOSYS;
    }

    DEBUG_VERBOSE("fpga seu config init success, name:%s, dev_name:%s, logic_func_mode:%u, seu_data_reg:0x%x, seu_data_status_reg:0x%x, seu_data_ctrl_reg:0x%x, seu_data_cnt_reg:0x%x, seu_data_len:%u, seu_status_valid_mask:0x%x, seu_ctrl_clear_mask:0x%x\n",
        fpga_seu->name, fpga_seu->dev_name, fpga_seu->logic_func_mode, fpga_seu->seu_data_reg,
        fpga_seu->seu_data_status_reg, fpga_seu->seu_data_ctrl_reg, fpga_seu->seu_data_cnt_reg,
        fpga_seu->seu_data_len, fpga_seu->seu_status_valid_mask, fpga_seu->seu_ctrl_clear_mask);
    return 0;
}

static int fpga_seu_probe(struct platform_device *pdev)
{
    int ret;
    wb_fpga_seu_t *fpga_seu;

    fpga_seu = devm_kzalloc(&pdev->dev, sizeof(wb_fpga_seu_t), GFP_KERNEL);
    if (!fpga_seu) {
        dev_err(&pdev->dev, "alloc fpga_seu failed\n");
        return -ENOMEM;
    }

    fpga_seu->dev = &pdev->dev;

    ret = fpga_seu_config_init(pdev, fpga_seu);
    if (ret < 0) {
        dev_err(&pdev->dev, "fpga_seu_config_init failed, ret:%d\n", ret);
        return ret;
    }

    mutex_init(&fpga_seu->update_lock);

    ret = kobject_init_and_add(&fpga_seu->kobj, &fpga_seu_ktype, logic_dev_kobj, "%s", fpga_seu->name);
    if (ret) {
        kobject_put(&fpga_seu->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", fpga_seu->name, ret);
        return ret;
    }

    fpga_seu->sysfs_group = &fpga_seu_attr_group;
    ret = sysfs_create_group(&fpga_seu->kobj, fpga_seu->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", fpga_seu->name, ret);
        goto remove_parent_kobj;
    }

    platform_set_drvdata(pdev, fpga_seu);
    dev_info(&pdev->dev, "register %s device, dev_name: %s, mode: %u, data_reg: 0x%x, len: %u success\n",
        fpga_seu->name, fpga_seu->dev_name, fpga_seu->logic_func_mode, fpga_seu->seu_data_reg, fpga_seu->seu_data_len);
    return 0;

remove_parent_kobj:
    kobject_put(&fpga_seu->kobj);
    platform_set_drvdata(pdev, NULL);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void fpga_seu_remove(struct platform_device *pdev)
{
    wb_fpga_seu_t *fpga_seu;

    fpga_seu = platform_get_drvdata(pdev);
    if (!fpga_seu) {
        dev_err(&pdev->dev, "fpga_seu_remove skip, drvdata is NULL\n");
        return;
    }

    if (fpga_seu->sysfs_group) {
        sysfs_remove_group(&fpga_seu->kobj, (const struct attribute_group *)fpga_seu->sysfs_group);
        kobject_put(&fpga_seu->kobj);
    }

    platform_set_drvdata(pdev, NULL);
}
#else
static int fpga_seu_remove(struct platform_device *pdev)
{
    wb_fpga_seu_t *fpga_seu;

    fpga_seu = platform_get_drvdata(pdev);
    if (!fpga_seu) {
        dev_err(&pdev->dev, "fpga_seu_remove skip, drvdata is NULL\n");
        return 0;
    }

    if (fpga_seu->sysfs_group) {
        sysfs_remove_group(&fpga_seu->kobj, (const struct attribute_group *)fpga_seu->sysfs_group);
        kobject_put(&fpga_seu->kobj);
    }

    platform_set_drvdata(pdev, NULL);
    return 0;
}
#endif

static struct of_device_id fpga_seu_match[] = {
    {
        .compatible = "wb-fpga-seu",
    },
    {},
};
MODULE_DEVICE_TABLE(of, fpga_seu_match);

static struct platform_driver wb_fpga_seu_driver = {
    .probe = fpga_seu_probe,
    .remove = fpga_seu_remove,
    .driver = {
        .owner = THIS_MODULE,
        .name = DRV_NAME,
        .of_match_table = fpga_seu_match,
    },
};

static int __init wb_fpga_seu_init(void)
{
    return platform_driver_register(&wb_fpga_seu_driver);
}

static void __exit wb_fpga_seu_exit(void)
{
    platform_driver_unregister(&wb_fpga_seu_driver);
}

module_init(wb_fpga_seu_init);
module_exit(wb_fpga_seu_exit);
MODULE_DESCRIPTION("fpga seu driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");