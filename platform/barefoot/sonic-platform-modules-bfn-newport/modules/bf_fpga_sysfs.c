/*******************************************************************************
 Barefoot Networks FPGA Linux driver
 Copyright(c) 2018 - 2019 Barefoot Networks, Inc.

 This program is free software; you can redistribute it and/or modify it
 under the terms and conditions of the GNU General Public License,
 version 2, as published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

 The full GNU General Public License is included in this distribution in
 the file called "COPYING".

 Contact Information:
 info@barefootnetworks.com
 Barefoot Networks, 4750 Patrick Henry Drive, Santa Clara CA 95054

*******************************************************************************/
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/pci.h>
#include <linux/mutex.h>
#include "bf_fpga_priv.h"
#include "bf_fpga_ioctl.h"
#include "i2c/bf_fpga_i2c.h"

/* reads 1 byte from the i2c device */
static ssize_t bf_fpga_sysfs_i2c_get(struct device *dev,
                                     struct device_attribute *attr,
                                     char *buf) {
  bf_fpga_i2c_t i2c_op;
  ssize_t size, cur_size;
  struct bf_fpga_sysfs_buff *sysfs_buf =
      container_of(attr, struct bf_fpga_sysfs_buff, dev_attr);

  if (!sysfs_buf) {
    printk(KERN_ERR "fpga-i2c bad attr pointer in sysfs_read\n");
    return -ENXIO; /* something not quite right here; but, don't panic */
  }
  i2c_op.num_i2c = 1;
  i2c_op.one_time = 1;
  i2c_op.inst_hndl.bus_id = sysfs_buf->bus_id;
  i2c_op.i2c_inst[0].preemt = false;
  i2c_op.i2c_inst[0].en = true;
  i2c_op.i2c_inst[0].i2c_addr = sysfs_buf->i2c_addr;
  i2c_op.i2c_inst[0].i2c_type = BF_FPGA_I2C_READ;
  i2c_op.i2c_inst[0].delay = 0;
  i2c_op.i2c_inst[0].wr_cnt = 0;
  cur_size = sysfs_buf->i2c_rd_size;
  /* limit to PAGE_SIZE per the sysfs contract */
  if (cur_size >= PAGE_SIZE) {
    cur_size = PAGE_SIZE;
  }
  size = 0;
  while (cur_size > 0) {
    unsigned char cur_cnt;
    if (cur_size > 64) {
      cur_cnt = 64;
    } else {
      cur_cnt = (unsigned char)cur_size;
    }
    i2c_op.i2c_inst[0].rd_cnt = cur_cnt;
    if (fpga_i2c_oneshot(&i2c_op)) {
      printk(KERN_ERR
             "fpga-i2c read one-shot error bus %d addr 0x%hhx status 0x%hhx\n",
             i2c_op.inst_hndl.bus_id,
             i2c_op.i2c_inst[0].i2c_addr,
             i2c_op.i2c_inst[0].status);
      return -EIO;
    }
    memcpy(buf, i2c_op.i2c_inst[0].fpga_i2c_buf.rd_buf, cur_cnt);
    buf += cur_cnt;
    size += cur_cnt;
    cur_size -= cur_cnt;
  }
  return size;
}

/* write the number of bytes supplied to the i2c device, 1st byte has to be
   the count(max 8) */
static ssize_t bf_fpga_sysfs_i2c_set(struct device *dev,
                                     struct device_attribute *attr,
                                     const char *buf,
                                     size_t count) {
  bf_fpga_i2c_t i2c_op;
  size_t size, cur_cnt;
  struct bf_fpga_sysfs_buff *sysfs_buf =
      container_of(attr, struct bf_fpga_sysfs_buff, dev_attr);

  if (!sysfs_buf || (count == 0)) {
    printk(KERN_ERR "fpga-i2c bad attr pointer in sysfs_write\n");
    return -ENXIO; /* something not quite right here; but, don't panic */
  }
  size = 0;
  while (count > 0) {
    if (count > 64) {
      cur_cnt = 64;
    } else {
      cur_cnt = count;
    }
    i2c_op.i2c_inst[0].wr_cnt = cur_cnt;
    i2c_op.i2c_inst[0].rd_cnt = 0;
    memcpy(i2c_op.i2c_inst[0].fpga_i2c_buf.wr_buf, buf, cur_cnt);
    i2c_op.num_i2c = 1;
    i2c_op.one_time = 1;
    i2c_op.inst_hndl.bus_id = sysfs_buf->bus_id;
    i2c_op.i2c_inst[0].preemt = false;
    i2c_op.i2c_inst[0].en = true;
    i2c_op.i2c_inst[0].i2c_addr = sysfs_buf->i2c_addr;
    i2c_op.i2c_inst[0].i2c_type = BF_FPGA_I2C_WRITE;
    i2c_op.i2c_inst[0].delay = 0;
    if (fpga_i2c_oneshot(&i2c_op)) {
      printk(
          KERN_ERR
          "fpga-i2c write one-shot error bus %d addr 0x%hhx status  0x%hhx\n",
          i2c_op.inst_hndl.bus_id,
          i2c_op.i2c_inst[0].i2c_addr,
          i2c_op.i2c_inst[0].status);
      return -EIO;
    }
    buf += cur_cnt;
    size += cur_cnt;
    count -= cur_cnt;
  }
  return size;
}

static int find_matching_sysfs_buf(struct bf_pci_dev *fpgadev,
                                   int bus_id,
                                   unsigned char i2c_addr) {
  int i;

  /* check if a sysfs entry already exists */
  for (i = 0; i < BF_FPGA_SYSFS_CNT; i++) {
    if (fpgadev->fpga_sysfs_buff[i].bus_id == bus_id &&
        fpgadev->fpga_sysfs_buff[i].i2c_addr == i2c_addr) {
      return i;
    }
  }
  /* could not find a matching sysfs buffer */
  return -1;
}

static ssize_t bf_fpga_sysfs_fixed_get(struct device *dev,
                                       struct device_attribute *attr,
                                       char *buf) {
  (void)dev;
  (void)attr;
  (void)buf;
  return -ENOSYS;
}

static ssize_t bf_fpga_sysfs_fixed_set(struct device *dev,
                                       struct device_attribute *attr,
                                       const char *buf,
                                       size_t count) {
  struct bf_pci_dev *fpgadev;
  int i, en, bus_id, ret, rd_size;
  char fname[BF_FPGA_SYSFS_NAME_SIZE];
  unsigned char i2c_addr;
  struct bf_fpga_sysfs_buff *new_buf;
  struct bf_fpga_sysfs_buff *sysfs_buf =
      container_of(attr, struct bf_fpga_sysfs_buff, dev_attr);

  if (!sysfs_buf || (count == 0)) {
    printk(KERN_ERR "fpga i2c bad attr pointer in fixed_sysfs_write\n");
    return -ENXIO; /* something not quite right here; but, don't panic */
  }
  fpgadev = sysfs_buf->fpgadev;

  switch (sysfs_buf->sysfs_code) {
    case BF_SYSFS_NEW_DEVICE: /* new_device request */
      ret = sscanf(buf, "%s %d %hhx %d", fname, &bus_id, &i2c_addr, &rd_size);
      /* default rd_size to 1 if not supplied or invalid */
      if (ret < 3) {
        return -EINVAL;
      }
      if (ret < 4 || rd_size > PAGE_SIZE) {
        rd_size = 1;
      }
      if (bus_id >= BF_I2C_FPGA_NUM_CTRL || i2c_addr >= 0x80) {
        return -EINVAL;
      }
      /* find out the free sysfs_buffer to use */
      spin_lock(&fpgadev->sysfs_slock);
      if (find_matching_sysfs_buf(fpgadev, bus_id, i2c_addr) != -1) {
        /* there is already an matching entry */
        spin_unlock(&fpgadev->sysfs_slock);
        return -ENOSPC;
      }
      for (i = 0; i < BF_FPGA_SYSFS_CNT; i++) {
        if (!fpgadev->fpga_sysfs_buff[i].in_use) {
          fpgadev->fpga_sysfs_buff[i].in_use = true;
          new_buf = &fpgadev->fpga_sysfs_buff[i];
          new_buf->i2c_addr = i2c_addr;
          new_buf->i2c_rd_size = (size_t)rd_size;
          new_buf->bus_id = bus_id;
          break;
        }
      }
      spin_unlock(&fpgadev->sysfs_slock);
      if (i >= BF_FPGA_SYSFS_CNT) {
        /* no free buffer available, return with ERROR */
        return -ENOSPC;
      }
      /* create a new sysfs entry now */
      new_buf->dev_attr.show = bf_fpga_sysfs_i2c_get;
      new_buf->dev_attr.store = bf_fpga_sysfs_i2c_set;
      new_buf->fpgadev = fpgadev;
      new_buf->dev_attr.attr.mode = S_IWUSR | S_IRUGO;
      new_buf->sysfs_code = 0;
      snprintf(new_buf->name, BF_FPGA_SYSFS_NAME_SIZE, "%s", fname);
      new_buf->dev_attr.attr.name = new_buf->name;
      ret = device_create_file(&(fpgadev->pdev->dev), &new_buf->dev_attr);
      break;

    case BF_SYSFS_RM_DEVICE: /* remove device request */
      ret = sscanf(buf, "%d %hhx", &bus_id, &i2c_addr);
      if (ret < 2) {
        return -EINVAL;
      }
      if (bus_id >= BF_I2C_FPGA_NUM_CTRL || i2c_addr >= 0x80) {
        return -EINVAL;
      }
      /* delete the sysfs file corresponding to the i2c address */
      spin_lock(&fpgadev->sysfs_slock);
      i = find_matching_sysfs_buf(fpgadev, bus_id, i2c_addr);
      if (i == -1) {
        /* there is no matching entry */
        spin_unlock(&fpgadev->sysfs_slock);
        return -EINVAL;
      }
      /* must invalidate bus_id and i2c_addr when marking the buffer
       * not-in-use
       */
      new_buf = &fpgadev->fpga_sysfs_buff[i];
      new_buf->i2c_addr = 0xff;
      new_buf->bus_id = -1;
      fpgadev->fpga_sysfs_buff[i].in_use = false;
      spin_unlock(&fpgadev->sysfs_slock);
      device_remove_file(&fpgadev->pdev->dev, &new_buf->dev_attr);
      new_buf->name[0] = 0; /* nullify the name */
      ret = 0;
      break;

    case BF_SYSFS_I2C_START: /* start-stop i2c request */
      ret = sscanf(buf, "%d %d", &bus_id, &en);
      if (ret < 2) {
        return -EINVAL;
      }
      if (bus_id >= BF_I2C_FPGA_NUM_CTRL) {
        return -EINVAL;
      }
      if (en) {
        ret = fpga_i2c_start(bus_id);
      } else {
        ret = fpga_i2c_stop(bus_id);
      }
      break;

    default:
      ret = -EINVAL;
  }
  return ((ret == 0) ? count : ret);
}

int bf_fpga_sysfs_add(struct bf_pci_dev *fpgadev) {
  int rc = 0;
  u8 *name;

  spin_lock_init(&fpgadev->sysfs_slock);
  /* Add two sysfs files statically, new_device and remove_device.
   * Handlers of these two fles can dynamically add more sysfs
   * files (or remove files) based on the platform.
   */
  fpgadev->fpga_sysfs_new_device.dev_attr.show = bf_fpga_sysfs_fixed_get;
  fpgadev->fpga_sysfs_new_device.dev_attr.store = bf_fpga_sysfs_fixed_set;
  fpgadev->fpga_sysfs_new_device.fpgadev = fpgadev;
  fpgadev->fpga_sysfs_new_device.dev_attr.attr.mode = S_IWUSR | S_IRUGO;
  fpgadev->fpga_sysfs_new_device.sysfs_code = BF_SYSFS_NEW_DEVICE;
  name = fpgadev->fpga_sysfs_new_device.name;
  snprintf(name, BF_FPGA_SYSFS_NAME_SIZE, "new_device");
  fpgadev->fpga_sysfs_new_device.dev_attr.attr.name = name;
  rc |= device_create_file(&(fpgadev->pdev->dev),
                           &fpgadev->fpga_sysfs_new_device.dev_attr);

  fpgadev->fpga_sysfs_rm_device.dev_attr.show = bf_fpga_sysfs_fixed_get;
  fpgadev->fpga_sysfs_rm_device.dev_attr.store = bf_fpga_sysfs_fixed_set;
  fpgadev->fpga_sysfs_rm_device.fpgadev = fpgadev;
  fpgadev->fpga_sysfs_rm_device.dev_attr.attr.mode = S_IWUSR | S_IRUGO;
  fpgadev->fpga_sysfs_rm_device.sysfs_code = BF_SYSFS_RM_DEVICE;
  name = fpgadev->fpga_sysfs_rm_device.name;
  snprintf(name, BF_FPGA_SYSFS_NAME_SIZE, "remove_device");
  fpgadev->fpga_sysfs_rm_device.dev_attr.attr.name = name;
  rc |= device_create_file(&(fpgadev->pdev->dev),
                           &fpgadev->fpga_sysfs_rm_device.dev_attr);

  /* sysfs for i2c start-stop control */
  fpgadev->fpga_sysfs_st_i2c.dev_attr.show = bf_fpga_sysfs_fixed_get;
  fpgadev->fpga_sysfs_st_i2c.dev_attr.store = bf_fpga_sysfs_fixed_set;
  fpgadev->fpga_sysfs_st_i2c.fpgadev = fpgadev;
  fpgadev->fpga_sysfs_st_i2c.dev_attr.attr.mode = S_IWUSR | S_IRUGO;
  fpgadev->fpga_sysfs_st_i2c.sysfs_code = BF_SYSFS_I2C_START;
  name = fpgadev->fpga_sysfs_st_i2c.name;
  snprintf(name, BF_FPGA_SYSFS_NAME_SIZE, "i2c_start");
  fpgadev->fpga_sysfs_st_i2c.dev_attr.attr.name = name;
  rc |= device_create_file(&(fpgadev->pdev->dev),
                           &fpgadev->fpga_sysfs_st_i2c.dev_attr);

  return rc;
}

void bf_fpga_sysfs_del(struct bf_pci_dev *fpgadev) {
  int i;

  device_remove_file(&fpgadev->pdev->dev,
                     &fpgadev->fpga_sysfs_new_device.dev_attr);
  device_remove_file(&fpgadev->pdev->dev,
                     &fpgadev->fpga_sysfs_rm_device.dev_attr);
  device_remove_file(&fpgadev->pdev->dev, &fpgadev->fpga_sysfs_st_i2c.dev_attr);
  for (i = 0; i < BF_FPGA_SYSFS_CNT; i++) {
    if (fpgadev->fpga_sysfs_buff[i].in_use) {
      device_remove_file(&fpgadev->pdev->dev,
                         &fpgadev->fpga_sysfs_buff[i].dev_attr);
    }
  }
}
