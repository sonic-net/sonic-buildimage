/*******************************************************************************
 Barefoot Networks Switch ASIC Linux driver
 Copyright(c) 2015 - 2019 Barefoot Networks, Inc.
 
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
#ifndef _BF_KDRV_H_
#define _BF_KDRV_H_

#include <linux/pci.h>
#include <linux/msi.h>
#include <linux/version.h>

#ifndef phys_addr_t
typedef uint64_t phys_addr_t;
#endif

#define PCI_VENDOR_ID_BF 0x1d1c
#define PCI_VENDOR_ID_INTEL 0x8086
#define TOFINO_DEV_ID_A0 0x01
#define TOFINO_DEV_ID_B0 0x10
#define TOFINO2_DEV_ID_A0 0x0100
#define TOFINO2_DEV_ID_A00 0x0000
#define TOFINO2_DEV_ID_B0 0x0110
#define TOFINO3_DEV_ID_A0 0x0DA2

#ifndef PCI_MSIX_ENTRY_SIZE
#define PCI_MSIX_ENTRY_SIZE 16
#define PCI_MSIX_ENTRY_LOWER_ADDR 0
#define PCI_MSIX_ENTRY_UPPER_ADDR 4
#define PCI_MSIX_ENTRY_DATA 8
#define PCI_MSIX_ENTRY_VECTOR_CTRL 12
#define PCI_MSIX_ENTRY_CTRL_MASKBIT 1
#endif

#define BF_CLASS_NAME "bf"
#define BF_MAX_DEVICE_CNT 8
#define BF_MAX_SUBDEV_CNT 2
#define BF_MAX_DEV_SUBDEV_CNT (BF_MAX_DEVICE_CNT * BF_MAX_SUBDEV_CNT)
#define BF_INTR_MODE_NONE_NAME "none"
#define BF_INTR_MODE_LEGACY_NAME "legacy"
#define BF_INTR_MODE_MSI_NAME "msi"
#define BF_INTR_MODE_MSIX_NAME "msix"
#define BF_MAX_BAR_MAPS 6
#define BF_MSIX_ENTRY_CNT 32 /* 512  for tofino-1 */
#define BF_MSI_ENTRY_CNT 2
#define BF_MSI_INT_TBUS 1

#define BF_TBUS_MSIX_INDEX_INVALID (0)
#define BF_TBUS_MSIX_BASE_INDEX_TOF1 (32)

#define TOFINO3_MISC_PAD_STATUS_OFFSET 0x80238UL
#define TOFINO3_MISC_PAD_STATUS_DIEID0 (1 << 3)

#define DRV_NAME(kpkt_mode) (kpkt_mode ? "bf_kpkt" : "bf_kdrv")
#define DRV_DESCRIPTION(kpkt_mode) (kpkt_mode ? \
                "Intel(R) Switch ASIC Linux Packet Driver" : \
                "Intel(R) Switch ASIC Linux Driver" )
#define DRV_COPYRIGHT "Copyright (c) 2015-2022 Intel Corporation."
#define DRV_VERSION "1.0"

/* Tofino generation type */
typedef enum {
  BF_TOFINO_NONE = 0,
  BF_TOFINO_1,
  BF_TOFINO_2,
  BF_TOFINO_3,
} bf_tof_type;

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

struct bf_tof3_info_s {
  int dev_id;
  int minor[BF_MAX_SUBDEV_CNT];
};

/* device information */
struct bf_dev_info {
  struct module *owner;
  struct device *dev;
  int major;
  int minor;
  atomic_t event[BF_MSIX_ENTRY_CNT];
  wait_queue_head_t wait;
  const char *version;
  struct bf_dev_mem mem[BF_MAX_BAR_MAPS];
  struct msix_entry *msix_entries;
  long irq;                /* first irq vector */
  int num_irq;             /* number of irq vectors */
  unsigned long irq_flags; /* sharable ?? */
  uint16_t pci_dev_id;     /* generation type of BF ASIC */
  bf_tof_type tof_type;    /* Tofino generation type */
  /* msix index assigned to tbus MSIX for Tofino-2 only */
  int tbus_msix_ind[BF_TBUS_MSIX_INDICES_MAX];
  int tbus_msix_map_enable;
  struct bf_tof3_info_s *tof3_info;
  int dev_id;  /* same as minor number for T1 and T2 */
  int subdev_id;
  int pci_error_state;     /* was there a pci bus error */
};

/* cookie to be passed to IRQ handler, useful especially with MSIX */
struct bf_int_vector {
  struct bf_pci_dev *bf_dev;
  int int_vec_offset;
};

/**
 * A structure describing the private information for a BF pcie device.
 */
struct bf_pci_dev {
  struct bf_dev_info info;
  struct pci_dev *pdev;
  enum bf_intr_mode mode;
  u8 instance;
  char name[16];
  struct bf_int_vector bf_int_vec[BF_MSIX_ENTRY_CNT];
  struct bf_listener *
      listener_head; /* head of a singly linked list of listeners */
  void *adapter_ptr; /* pkt processing adapter */
  int  in_use; /* indicates a user space process is using the device */
};

/* TBD: Need to build with CONFIG_PCI_MSI */
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
#if defined(RHEL_RELEASE_CODE)
#else
extern int pci_enable_msi_block(struct pci_dev *dev, unsigned int nvec);
#endif /* defined(RHEL_RELEASE_CODE) */
extern int pci_enable_msix(struct pci_dev *dev, struct msix_entry *entries, int nvec);
#else
extern int pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec);
extern int pci_enable_msix_range(struct pci_dev *dev,
                                 struct msix_entry *entries,
                                 int minvec,
                                 int maxvec);
#endif

#endif /* _BF_KDRV_H_ */
