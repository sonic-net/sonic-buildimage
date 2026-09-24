#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/sysfs.h>
#include "fpga.h"
#include "fpga_attr.h"
#include "fpga_reg.h"

static struct kobject *delta_fpga;
struct attribute **attrs;
struct attribute_group attr_grp; 
void *__iomem bar;

struct mutex fpga_attr_lock;

static ssize_t delta_fpga_reg_show(struct device *dev,
                               struct device_attribute *attr, char *buf)
{
    fpga_sysfs_attr_st *fpga_container = TO_FPGA_SYSFS_ATTR(attr);
    const delta_fpga_reg_st *fpga_reg = fpga_container->fpga_reg;
    int offset = fpga_reg->offset;
    int bit_offset = fpga_reg->bit_offset;
    int n_bits = fpga_reg->n_bits;
    unsigned int val, val_mask, reg_val;

    if (!fpga_reg->show) {
        return -EOPNOTSUPP;
    }

    if (fpga_reg->show != I2C_DEV_ATTR_SHOW_DEFAULT) {
        return fpga_reg->show(dev, attr, buf);
    }

    mutex_lock(&fpga_attr_lock);

    val_mask = ((unsigned long long)1 << (n_bits)) - 1;
    reg_val  = ioread32(bar + offset);
    val = (reg_val >> bit_offset) & val_mask;

    mutex_unlock(&fpga_attr_lock);

    return scnprintf(buf, PAGE_SIZE,
                    "%#x\n\nNote:\n%s\n"
                    "Bit[%d:%d] @ register %#x, register value %#x\n",
                    val, (fpga_reg->help) ? fpga_reg->help : "",
                    bit_offset + n_bits - 1,
                    bit_offset, offset, reg_val);

}

static ssize_t delta_fpga_reg_store(struct device *dev,
                                struct device_attribute *attr, const char *buf,
                                size_t count)
{
    fpga_sysfs_attr_st *fpga_container = TO_FPGA_SYSFS_ATTR(attr);
    const delta_fpga_reg_st *fpga_reg = fpga_container->fpga_reg;
    int offset = fpga_reg->offset;
    int bit_offset = fpga_reg->bit_offset;
    int n_bits = fpga_reg->n_bits;
    unsigned int val;
    int req_val;
    unsigned int req_val_mask;
    unsigned int max = (((unsigned long long)1<<n_bits)) -1;

    if (!fpga_reg->store) {
        return -EOPNOTSUPP;
    }

    if (fpga_reg->store != I2C_DEV_ATTR_STORE_DEFAULT) {
        return fpga_reg->store(dev, attr, buf, count);
    }

    if (sscanf(buf, "%i", &req_val) <= 0) {
        return -EINVAL;
    }
    if (req_val > max)
    {
        printk(KERN_ERR "maximum data is = 0x%x, but input data is 0x%x\n", max, req_val);
        return -EINVAL;
    }

    mutex_lock(&fpga_attr_lock);

    req_val_mask = max;
    req_val &= req_val_mask;
    val = ioread32(bar + offset);

    /* mask out all bits for the value requested */
    val &= ~(req_val_mask << bit_offset);
    val |= req_val << bit_offset;
    iowrite32(val, bar + offset);

    mutex_unlock(&fpga_attr_lock);
    //return sprintf(buf, "%d\n", val);
    return count;

}

static int fpga_attr_create(void)
{
    fpga_sysfs_attr_st *fpga_container;
    int i;
    int len = sizeof(sysfpga_reg_table)/sizeof(delta_fpga_reg_st);

    fpga_container = kzalloc(sizeof(fpga_sysfs_attr_st) * len ,GFP_KERNEL);

    attrs = kzalloc(sizeof(struct attribute *) * len+1 ,GFP_KERNEL);
    
    for(i = 0; i < len; i++, fpga_container++)
    {
        fpga_container->dev_attr.attr.name = sysfpga_reg_table[i].name;
        fpga_container->dev_attr.attr.mode = 0660;
        fpga_container->dev_attr.show = (void *)delta_fpga_reg_show;
        fpga_container->dev_attr.store = (void *)delta_fpga_reg_store;
        attrs[i] = &fpga_container->dev_attr.attr;
        fpga_container->fpga_reg = &sysfpga_reg_table[i];
    }

    attr_grp.attrs = attrs;

    return 0;
}

int fpga_attr_init(struct pci_dev *dev, struct fpga_dev *fpga)
{
    int error;
    bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));
    delta_fpga = kobject_create_and_add("delta_fpga", kernel_kobj);
    if (!delta_fpga)
        return -ENOMEM;
    fpga_attr_create();
    
    mutex_init(&fpga_attr_lock);
    error = sysfs_create_group(delta_fpga,&attr_grp);
    if (error)
    {
        kobject_put(delta_fpga);
        pr_info("failed to create the delta_fpga_reg file "
                "in /sys/kernel/delta_fpga\n");
    }
    return 0;
}
int fpga_attr_exit(void)
{
    kobject_put(delta_fpga);
    return 0;
}

