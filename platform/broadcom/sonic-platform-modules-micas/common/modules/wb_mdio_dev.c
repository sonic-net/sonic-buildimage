#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mdio.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/phy.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/version.h>
#include "wb_mdio_dev.h"

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int phy_read_ext(mdio_dev_info_t *phy, u32 reg_addr)
{
	int ret;

    ret = mdiobus_write(phy->bus, phy->addr, PHY_EXTERN_ADDR_REG, reg_addr);
    if (ret < 0)
    {
        return ret;
    }

    return mdiobus_read(phy->bus, phy->addr, PHY_EXTERN_DATA_REG);
}

static int phy_write_ext(mdio_dev_info_t *phy, u32 reg_addr, u16 val)
{
	int ret;

    ret = mdiobus_write(phy->bus, phy->addr, PHY_EXTERN_ADDR_REG, reg_addr);
    if (ret < 0)
    {
        return ret;
    }

    return mdiobus_write(phy->bus, phy->addr, PHY_EXTERN_DATA_REG, val);
}

static int phy_dev_read(mdio_dev_info_t *phy, u32 reg_addr)
{
	if (reg_addr > PHY_EXTERN_DATA_REG) {
        return phy_read_ext(phy, reg_addr);
    }

    return mdiobus_read(phy->bus, phy->addr, reg_addr);
}

static int phy_dev_write(mdio_dev_info_t *phy, u32 reg_addr, u16 val)
{
	if (reg_addr > PHY_EXTERN_DATA_REG) {
        return phy_write_ext(phy, reg_addr, val);
    }

    return mdiobus_write(phy->bus, phy->addr, reg_addr, val);
}

static bool ends_with_suffix(const char *str, const char *suffix) {
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);

    if (str_len < suffix_len) {
        return false;
    }

    return strncmp(str + str_len - suffix_len, suffix, suffix_len) == 0;
}

static int of_mdio_dev_config_init(mdio_dev_info_t *mdio_dev)
{
    struct device *dev;
    int num_regs, i, ret = 0;
    dev = mdio_dev->dev;

    ret += of_property_read_string(dev->of_node, "mdio_name", &mdio_dev->name);
    ret += of_property_read_string(dev->of_node, "mdio_bus_name", &mdio_dev->mdio_bus_name);
    ret += of_property_read_u32(dev->of_node, "addr", &mdio_dev->addr);
    if (ret != 0) {
        DEBUG_ERROR("Missing required properties\n");
        return -EINVAL;
    }

    if (of_property_read_string(dev->of_node, "dev_alias", &mdio_dev->alias)) {
        mdio_dev->alias = mdio_dev->name;
    }

    num_regs = of_property_count_elems_of_size(dev->of_node, "phy_init_regs", sizeof(u32));
    if (num_regs < 0) {
        mdio_dev->phy_need_init = 0;
        dev_info(dev, "Not need init.\n");
        return 0;
    }
    if (num_regs == 0 || num_regs > PHY_REG_INIT_INFO_MAX) {
        dev_err(dev, "Invalid reg count %d\n", num_regs);
        return -EINVAL;
    }

    mdio_dev->phy_need_init = 1;
    mdio_dev->reg_init_num = 0;
    for (i = 0; i < num_regs; i++) {
        ret = of_property_read_u32_index(dev->of_node, "phy_init_regs", i,
                                    &mdio_dev->phy_reg_init[i].phy_reg);
        ret += of_property_read_u32_index(dev->of_node, "init_values", i,
                                    &mdio_dev->phy_reg_init[i].init_value);
        if (ret) {
            dev_err(dev, "Failed to get config%d ret=%d.\n", i, ret);
            return ret;
        }

        mdio_dev->phy_reg_init[i].mask = 0xffff;
        of_property_read_u32_index(dev->of_node, "masks", i,
                                &mdio_dev->phy_reg_init[i].mask);

        mdio_dev->phy_reg_init[i].delay = 0;
        of_property_read_u32_index(dev->of_node, "delays", i,
                                &mdio_dev->phy_reg_init[i].delay);

        mdio_dev->reg_init_num++;
    }

    return 0;
}

static int mdio_dev_config_init(mdio_dev_info_t *mdio_dev)
{
    mdio_dev_device_t *mdio_dev_device;

    if (mdio_dev->dev->platform_data == NULL) {
        dev_err(mdio_dev->dev, "Failed to get platform data config.\n");
        return -ENXIO;
    }
    mdio_dev_device = mdio_dev->dev->platform_data;
    mdio_dev->name = mdio_dev_device->dev_name;
    mdio_dev->alias = mdio_dev_device->dev_name;
    mdio_dev->mdio_bus_name = mdio_dev_device->mdio_bus_name;
    mdio_dev->addr = mdio_dev_device->addr;
    mdio_dev->phy_need_init = 0;

    return 0;
}

static int mdio_phy_reg_init(mdio_dev_info_t *mdio_dev)
{
    int ret, i;
    int value;

    if (!mdio_dev->phy_need_init) {
        dev_info(mdio_dev->dev, "Not need init, do nothing\n");
        return 0;
    }

    for (i = 0; i < mdio_dev->reg_init_num; i++) {
        /* read origin register value */
        DEBUG_VERBOSE("mdio_phy_reg_init: phy_reg:0x%x, init_value:0x%x, mask:0x%x",
            mdio_dev->phy_reg_init[i].phy_reg, mdio_dev->phy_reg_init[i].init_value, mdio_dev->phy_reg_init[i].mask);

        if (mdio_dev->phy_reg_init[i].delay != 0) {
            usleep_range(mdio_dev->phy_reg_init[i].delay, mdio_dev->phy_reg_init[i].delay + 1);
        }

        value = phy_dev_read(mdio_dev, mdio_dev->phy_reg_init[i].phy_reg);
        if (value < 0) {
            DEBUG_ERROR("read phy addr: 0x%x failed, ret: %d\n", mdio_dev->phy_reg_init[i].phy_reg, value);
            return -EIO;
        }
        DEBUG_VERBOSE("read phy addr 0x%x success, value: 0x%x\n", mdio_dev->phy_reg_init[i].phy_reg, value);
        DEBUG_VERBOSE("write to phy raw value: 0x%x\n", mdio_dev->phy_reg_init[i].init_value);
        value &= ~(mdio_dev->phy_reg_init[i].mask);
        value |= mdio_dev->phy_reg_init[i].init_value;

        ret = phy_dev_write(mdio_dev, mdio_dev->phy_reg_init[i].phy_reg, value);
        if (ret < 0) {
            dev_err(mdio_dev->dev, "write phy addr: 0x%x failed, ret: %d\n", mdio_dev->phy_reg_init[i].phy_reg, ret);
            return -EIO;
        }
        dev_info(mdio_dev->dev, "write phy addr 0x%x success, value: 0x%x\n", mdio_dev->phy_reg_init[i].phy_reg, value);
    }
    return 0;
}

static ssize_t alias_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);

    if (!mdio_dev) {
        DEBUG_ERROR("alias show mdio dev is null.\n");
        return -ENODEV;
    }
    return snprintf(buf, PAGE_SIZE, "%s\n", mdio_dev->alias);
}

static int phy_id_read(mdio_dev_info_t *mdio_dev, uint32_t *phy_id)
{
    int id1, id2;

    id1 = phy_dev_read(mdio_dev, MII_PHYSID1);
    id2 = phy_dev_read(mdio_dev, MII_PHYSID2);
    if (id1 < 0 || id2 < 0) {
        DEBUG_ERROR("phy_id read fail, ret1:%d, ret2:%d.\n",id1, id2);
        return -EIO;
    }

    *phy_id = ((id1 << 16) | id2);
    if (*phy_id == 0 || *phy_id == 0xffffffff) {
        DEBUG_ERROR("illegal phy_id :0x%x.\n", *phy_id);
        return -EINVAL;
    }

    return 0;
}

static ssize_t phy_id_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int ret;
    uint32_t phy_id;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_id show mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    ret = phy_id_read(mdio_dev, &phy_id);
    mutex_unlock(&mdio_dev->lock);
    if (ret < 0) {
        DEBUG_ERROR("phy_id read fail, ret:%d.\n", ret);
        return ret;
    }

    return snprintf(buf, PAGE_SIZE, "0x%x\n", phy_id);
}

static ssize_t phy_type_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int ret, phy_id, i;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_type show mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    ret = phy_id_read(mdio_dev, &phy_id);
    mutex_unlock(&mdio_dev->lock);
    if (ret < 0) {
        DEBUG_ERROR("phy_id read fail, ret:%d.\n", ret);
        return ret;
    }

    for (i = 0; i < ARRAY_SIZE(phy_type_infos); i++) {
        if (phy_type_infos[i].phy_id == phy_id) {
            return snprintf(buf, PAGE_SIZE, "%s\n", phy_type_infos[i].phy_type);
        }
    }

    return snprintf(buf, PAGE_SIZE, "%s\n", UNKNOWN_PHY_TYPE);
}

static int phy_status_read(mdio_dev_info_t *mdio_dev)
{
    int ret, i;

    /* link status need read twice */
    for (i = 0; i < 2; i++) {
        ret = phy_dev_read(mdio_dev, MII_BMSR);
        if (ret < 0) {
            DEBUG_ERROR("link_status read fail, ret:%d.\n", ret);
            return -EIO;
        }
    }
    DEBUG_VERBOSE("link_status read success, ret:0x%x.\n", ret);
    return ret;
}

static ssize_t phy_link_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int phy_status;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_link_status show mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    phy_status = phy_status_read(mdio_dev);
    mutex_unlock(&mdio_dev->lock);
    if (phy_status < 0) {
        DEBUG_ERROR("phy_status read fail, ret:%d.\n", phy_status);
        return -EIO;
    }

    if (phy_status & BMSR_LSTATUS) {
        return snprintf(buf, PAGE_SIZE, "%d\n", PHY_LINK_STS_UP);
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", PHY_LINK_STS_DOWN);
}

static ssize_t phy_reset_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int phy_control;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_control show mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    phy_control = phy_dev_read(mdio_dev, MII_BMCR);
    mutex_unlock(&mdio_dev->lock);
    if (phy_control < 0) {
        DEBUG_ERROR("phy_control read fail, ret:%d.\n", phy_control);
        return -EIO;
    }

    if (phy_control & PHY_RESET_BIT) {
        return snprintf(buf, PAGE_SIZE, "%d\n", PHY_RESET_ENABLE);
    }
    return snprintf(buf, PAGE_SIZE, "%d\n", PHY_RESET_DISABLE);
}

static ssize_t phy_reset_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    u8 val;
    int ret, phy_control;
    u16 wr_val = 0;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_reset_store mdio dev is null.\n");
        return -ENODEV;
    }

    val = 0;
    ret = kstrtou8(buf, 0, &val);
    if (ret) {
        DEBUG_ERROR("Invaild input value [%s], errno: %d\n", buf, ret);
        return -EINVAL;
    }

    if ((val != PHY_RESET_ENABLE) && (val != PHY_RESET_DISABLE)) {
        DEBUG_ERROR("input value [%d] not equal 0 or 1\n", val);
        return -EINVAL;
    }

    mutex_lock(&mdio_dev->lock);
    phy_control = phy_dev_read(mdio_dev, MII_BMCR);
    if (phy_control < 0) {
        DEBUG_ERROR("phy_control read fail, ret:%d.\n", phy_control);
        mutex_unlock(&mdio_dev->lock);
        return -EIO;
    }

    if (val == PHY_RESET_ENABLE) {
        wr_val = phy_control | PHY_RESET_BIT;
    } else {
        wr_val = phy_control & ~PHY_RESET_BIT;
    }

    ret = phy_dev_write(mdio_dev, MII_BMCR, wr_val);
    mutex_unlock(&mdio_dev->lock);
    if (ret < 0) {
        DEBUG_ERROR("phy_reset failed, wr_val:0x%x, ret: %d\n", wr_val, ret);
        return -EIO;
    }
    DEBUG_VERBOSE("phy_reset success, wr_val:0x%x\n", wr_val);
    return count;
}

static ssize_t phy_reg_dump(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int i, offset = 0;
    int rd_val;

    if (!mdio_dev) {
        DEBUG_ERROR("phy_reg_dump mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    for (i = 0; i < ARRAY_SIZE(phy_reg_infos); i++) {
        rd_val = phy_dev_read(mdio_dev, phy_reg_infos[i].reg_addr);
        if (rd_val < 0) {
            offset += scnprintf(buf + offset, PAGE_SIZE - offset, "(0x%02x)%-20s: fail(%d)\n", phy_reg_infos[i].reg_addr, phy_reg_infos[i].reg_name, rd_val);
        } else {
            offset += scnprintf(buf + offset, PAGE_SIZE - offset, "(0x%02x)%-20s: 0x%04x\n", phy_reg_infos[i].reg_addr, phy_reg_infos[i].reg_name, rd_val);
        }
    }
    mutex_unlock(&mdio_dev->lock);
    return offset;
}

static ssize_t mdio_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    mdio_dev_info_t *mdio_dev = container_of(kobj, mdio_dev_info_t, kobj);
    int ret, i, mdio_status;
    uint32_t phy_id;

    if (!mdio_dev) {
        DEBUG_ERROR("mdio_status show mdio dev is null.\n");
        return -ENODEV;
    }

    mutex_lock(&mdio_dev->lock);
    for (i = 0; i < MAX_RETRY; i++) {
        ret = phy_id_read(mdio_dev, &phy_id);
        if (ret < 0) {
            mdio_status = MDIO_STATUS_NOT_OK;
            DEBUG_ERROR("phy_id read fail, ret:%d.\n", ret);
        } else {
            mdio_status = MDIO_STATUS_OK;
            break;
        }
    }
    mutex_unlock(&mdio_dev->lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", mdio_status);
}

static struct kobj_attribute alias_attribute = __ATTR(alias, S_IRUGO, alias_show, NULL);
static struct kobj_attribute phy_id_attribute = __ATTR(phy_id, S_IRUGO, phy_id_show, NULL);
static struct kobj_attribute phy_type_attribute = __ATTR(phy_type, S_IRUGO, phy_type_show, NULL);
static struct kobj_attribute link_status_attribute = __ATTR(link_status, S_IRUGO, phy_link_status_show, NULL);
static struct kobj_attribute reset_attribute = __ATTR(reset, S_IRUGO | S_IWUSR, phy_reset_show, phy_reset_store);
static struct kobj_attribute phy_reg_dump_attribute = __ATTR(phy_reg_dump, S_IRUGO, phy_reg_dump, NULL);
static struct kobj_attribute mdio_status_attribute = __ATTR(mdio_status, S_IRUGO, mdio_status_show, NULL);

static struct attribute *mdio_dev_attrs[] = {
    &alias_attribute.attr,
    &phy_id_attribute.attr,
    &link_status_attribute.attr,
    &reset_attribute.attr,
    &phy_reg_dump_attribute.attr,
    &phy_type_attribute.attr,
    &mdio_status_attribute.attr,
    NULL,
};

static struct attribute_group mdio_dev_attr_group = {
    .attrs = mdio_dev_attrs,
};

static ssize_t mdio_dev_attr_show(struct kobject *kobj, struct attribute *attr, char *buf)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->show) {
        DEBUG_ERROR("mdio dev attr show is null.\n");
        return -ENOSYS;
    }

    return attribute->show(kobj, attribute, buf);
}

static ssize_t mdio_dev_attr_store(struct kobject *kobj, struct attribute *attr, const char *buf,
                   size_t len)
{
    struct kobj_attribute *attribute;

    attribute = container_of(attr, struct kobj_attribute, attr);

    if (!attribute->store) {
        DEBUG_ERROR("mdio dev attr store is null.\n");
        return -ENOSYS;
    }

    return attribute->store(kobj, attribute, buf, len);
}

static const struct sysfs_ops mdio_dev_sysfs_ops = {
    .show = mdio_dev_attr_show,
    .store = mdio_dev_attr_store,
};

static void mdio_dev_obj_release(struct kobject *kobj)
{
    return;
}

static struct kobj_type mdio_dev_ktype = {
    .sysfs_ops = &mdio_dev_sysfs_ops,
    .release = mdio_dev_obj_release,
#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    .default_attrs = NULL,
#endif
};

static int mdio_dev_probe(struct platform_device *pdev)
{
    mdio_dev_info_t *mdio_dev;
    int ret;

    mdio_dev = devm_kzalloc(&pdev->dev, sizeof(mdio_dev_info_t), GFP_KERNEL);
    if (!mdio_dev) {
        dev_err(&pdev->dev, "mdio_dev memory allocation failed\n");
        return -ENOMEM;
    }
    mdio_dev->dev = &pdev->dev;
    mutex_init(&mdio_dev->lock);

    if (pdev->dev.of_node) {
        ret = of_mdio_dev_config_init(mdio_dev);
    } else {
        ret = mdio_dev_config_init(mdio_dev);
    }
    if (ret) {
        dev_err(&pdev->dev, "mdio_dev_config_init fail, ret: %d\n", ret);
        return -EINVAL;
    }

    if (!ends_with_suffix(mdio_dev->name, MDIO_NAME_SUFFIX)) {
        dev_err(&pdev->dev, "mdio_name:%s config err, not end with phy:%s.\n", mdio_dev->name, MDIO_NAME_SUFFIX);
        return -EINVAL;
    }

    mdio_dev->bus = mdio_find_bus(mdio_dev->mdio_bus_name);
    if (!mdio_dev->bus) {
        dev_err(&pdev->dev, "find mdio bus name: %s fail.\n", mdio_dev->mdio_bus_name);
        return -ENOMEM;
    }

    ret = mdio_phy_reg_init(mdio_dev);
    if (ret < 0) {
        dev_err(&pdev->dev, "mdio_phy_reg_init fail, ret: %d\n", ret);
        return ret;
    }

    /* creat parent dir by dev name in /sys/logic_dev */
    ret = kobject_init_and_add(&mdio_dev->kobj, &mdio_dev_ktype, logic_dev_kobj, "%s", mdio_dev->name);
    if (ret) {
        kobject_put(&mdio_dev->kobj);
        dev_err(&pdev->dev, "Failed to creat parent dir: %s, ret: %d\n", mdio_dev->name, ret);
        return ret;
    }

    mdio_dev->sysfs_group = &mdio_dev_attr_group;
    ret = sysfs_create_group(&mdio_dev->kobj, mdio_dev->sysfs_group);
    if (ret) {
        dev_err(&pdev->dev, "Failed to create %s sysfs group, ret: %d\n", mdio_dev->name, ret);
        goto remove_parent_kobj;
    }
    platform_set_drvdata(pdev, mdio_dev);

    dev_info(&pdev->dev, "registered %s with mdio bus: %s, phy address: 0x%x success\n",
        mdio_dev->name, mdio_dev->mdio_bus_name, mdio_dev->addr);
    return 0;
remove_parent_kobj:
    kobject_put(&mdio_dev->kobj);
    return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,11,0)
static void mdio_dev_remove(struct platform_device *pdev)
#else
static int mdio_dev_remove(struct platform_device *pdev)
#endif
{
    mdio_dev_info_t *mdio_dev;
    mdio_dev = platform_get_drvdata(pdev);

    if (mdio_dev->sysfs_group) {
        dev_dbg(&pdev->dev, "Unregister %s mdio_dev sysfs group\n", mdio_dev->name);
        sysfs_remove_group(&mdio_dev->kobj, (const struct attribute_group *)mdio_dev->sysfs_group);
        kobject_put(&mdio_dev->kobj);
        mdio_dev->sysfs_group = NULL;
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,11,0)
    return 0;
#endif
}

static const struct of_device_id mdio_dev_of_match[] = {
    { .compatible = MDIO_DEV_COMPATIBLE_NAME },
    {}
};

static struct platform_driver mdio_dev_driver = {
    .probe = mdio_dev_probe,
    .remove = mdio_dev_remove,
    .driver = {
        .name = "wb_mdio_dev",
        .owner = THIS_MODULE,
        .of_match_table = mdio_dev_of_match,
    },
};

module_platform_driver(mdio_dev_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("MDIO Device Driver");
