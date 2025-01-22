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
#ifndef _BF_FPGA_H_
#define _BF_FPGA_H_

#define PCI_VENDOR_ID_BF 0x1d1c
#define BF_FPGA_DEV_ID_JBAY_0 0x01F0
#define BF_FPGA_DEV_ID_CB_0 0x01F1

#ifndef PCI_MSIX_ENTRY_SIZE
#define PCI_MSIX_ENTRY_SIZE 16
#define PCI_MSIX_ENTRY_LOWER_ADDR 0
#define PCI_MSIX_ENTRY_UPPER_ADDR 4
#define PCI_MSIX_ENTRY_DATA 8
#define PCI_MSIX_ENTRY_VECTOR_CTRL 12
#define PCI_MSIX_ENTRY_CTRL_MASKBIT 1
#endif

#define BF_CLASS_NAME "bf_fpga"
#define BF_FPGA_MAX_DEVICE_CNT 1
#define BF_INTR_MODE_NONE_NAME "none"
#define BF_INTR_MODE_LEGACY_NAME "legacy"
#define BF_INTR_MODE_MSI_NAME "msi"
#define BF_INTR_MODE_MSIX_NAME "msix"
#define BF_MAX_BAR_MAPS 6
#define BF_MSIX_ENTRY_CNT 1
#define BF_MSI_ENTRY_CNT 1

/* sysfs codes */
#define BF_SYSFS_NEW_DEVICE 1
#define BF_SYSFS_RM_DEVICE 0
#define BF_SYSFS_I2C_START 2

/* interrupt mode */
enum bf_intr_mode {
  BF_INTR_MODE_NONE = 0,
  BF_INTR_MODE_LEGACY,
  BF_INTR_MODE_MSI,
  BF_INTR_MODE_MSIX
};

/* device memory */
struct bf_dev_mem {
  const char *name;
  phys_addr_t addr;
  resource_size_t size;
  void __iomem *internal_addr;
};

struct bf_listener {
  struct bf_pci_dev *bfdev;
  s32 event_count[BF_MSIX_ENTRY_CNT];
  int minor;
  struct bf_listener *next;
};

/* device information */
struct bf_dev_info {
  struct module *owner;
  struct device *dev;
  int minor;
  atomic_t event[BF_MSIX_ENTRY_CNT];
  wait_queue_head_t wait;
  const char *version;
  struct bf_dev_mem mem[BF_MAX_BAR_MAPS];
  struct msix_entry *msix_entries;
  long irq;                /* first irq vector */
  int num_irq;             /* number of irq vectors */
  unsigned long irq_flags; /* sharable ?? */
  int pci_error_state;     /* was there a pci bus error */
};

/* cookie to be passed to IRQ handler, useful especially with MSIX */
struct bf_int_vector {
  struct bf_pci_dev *bf_dev;
  int int_vec_offset;
};

/* sysfs related structs */
#define BF_FPGA_SYSFS_CNT 64
#define BF_FPGA_SYSFS_NAME_SIZE 32

struct bf_fpga_sysfs_buff {
  struct device_attribute dev_attr;
  char name[BF_FPGA_SYSFS_NAME_SIZE];
  int bus_id;
  unsigned char i2c_addr;
  size_t i2c_rd_size;         /* bytes to read from the device */
  int sysfs_code;             /* unique code for each sysfs file */
  struct bf_pci_dev *fpgadev; /* back pointer */
  bool in_use;
};

/**
 * structure describing the private information for a BF pcie device.
 */
struct bf_pci_dev {
  struct bf_dev_info info;
  struct pci_dev *pdev;
  enum bf_intr_mode mode;
  u8 instance;
  char name[16];
  struct bf_int_vector bf_int_vec[BF_MSIX_ENTRY_CNT];
  struct bf_listener
      *listener_head; /* head of a singly linked list of listeners */
  struct bf_fpga_sysfs_buff fpga_sysfs_buff[BF_FPGA_SYSFS_CNT];
  struct bf_fpga_sysfs_buff fpga_sysfs_new_device;
  struct bf_fpga_sysfs_buff fpga_sysfs_rm_device;
  struct bf_fpga_sysfs_buff fpga_sysfs_st_i2c;
  spinlock_t sysfs_slock;
};

int bf_fpga_ioctl(struct bf_pci_dev *bfdev,
                  unsigned int cmd,
                  unsigned long arg);
int bf_fpga_sysfs_add(struct bf_pci_dev *fpgadev);
void bf_fpga_sysfs_del(struct bf_pci_dev *fpgadev);

#endif /* _BF_FPGA_H_ */
