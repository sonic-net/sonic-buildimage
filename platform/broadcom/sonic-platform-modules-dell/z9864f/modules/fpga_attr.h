#ifndef __FGPA_ATTR_H__
#define __FGPA_ATTR_H__

typedef ssize_t (*i2c_dev_attr_show_fn)(struct device *dev,
                                        struct device_attribute *attr,
                                        char *buf);
typedef ssize_t (*i2c_dev_attr_store_fn)(struct device *dev,
                                         struct device_attribute *attr,
                                         const char *buf, size_t count);

#define I2C_DEV_ATTR_SHOW_DEFAULT (i2c_dev_attr_show_fn)(1)
#define I2C_DEV_ATTR_STORE_DEFAULT (i2c_dev_attr_store_fn)(1)

typedef struct delta_fpga_reg_st_ {
  const char *name;
  const char *help;
  i2c_dev_attr_show_fn show;
  i2c_dev_attr_store_fn store;
  int offset;
  int bit_offset;
  int n_bits;
} delta_fpga_reg_st;

typedef struct fpga_sysfs_attr_st_{
	struct device_attribute dev_attr;
    const delta_fpga_reg_st *fpga_reg;
} fpga_sysfs_attr_st;

#define TO_FPGA_SYSFS_ATTR(_attr) \
	container_of(_attr, fpga_sysfs_attr_st, dev_attr)

int fpga_attr_init(struct pci_dev *dev, struct fpga_dev *fpga);
int fpga_attr_exit(void);

#endif /* __FGPA_ATTR_H__ */