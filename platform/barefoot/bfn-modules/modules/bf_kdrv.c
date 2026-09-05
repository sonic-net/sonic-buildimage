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
/* bf_drv kernel module
 *
 * This is kernel mode driver for Tofino chip.
 * Provides user space mmap service and user space "wait for interrupt"
 * and "enable interrupt" services.
 */

#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/dma-mapping.h>
#include "bf_ioctl.h"
#include "bf_kdrv.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
  #include <linux/sched/signal.h>
#else
  #include <linux/sched.h>
#endif
#include <linux/cdev.h>
#include <linux/aer.h>
#include <linux/string.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 16, 0)
//#error unsupported linux kernel version
#endif

#ifdef BF_INCLUDE_KPKT
/* kernel pkt driver entry/exit APIs */
extern int bf_kpkt_init(struct pci_dev *pdev,
                        u8 *bar0_vaddr,
                        void **adapter_ptr,
                        int dev_id,
                        int subdev_id,
                        int pci_use_highmem,
                        unsigned long head_room,
                        int kpkt_dr_int_en,
                        unsigned long rx_desc_countp);
extern void bf_kpkt_remove(void *adapter_ptr);
extern void bf_kpkt_irqhandler(int irq, void *adapter_ptr);
extern void bf_kpkt_set_pci_error(void *adapter_ptr, u8 pci_error);
#endif

/* Keep any global information here that must survive even after the
 * bf_pci_dev is free-ed up.
 */
struct bf_global_s {
  struct bf_pci_dev *bfdev;
  struct cdev *bf_cdev;
  struct fasync_struct *async_queue;
  /* Pending user space error signal. */
  bool pending_signal;
};

static int bf_minor[BF_MAX_DEV_SUBDEV_CNT] = {0};
static struct class *bf_class = NULL;
static char *intr_mode = NULL;
static int kpkt_mode = 0;
static int kpkt_hd_room = 32;
static int kpkt_rx_count = 256;
static int kpkt_dr_int_en = 1;

static enum bf_intr_mode bf_intr_mode_default = BF_INTR_MODE_MSI;
static spinlock_t bf_nonisr_lock;

/* dev->minor should index into this array */
static struct bf_global_s bf_global[BF_MAX_DEV_SUBDEV_CNT];

/* global tofino3 info to group subdevices to a parent device */
static struct bf_tof3_info_s bf_tof3_info[BF_MAX_DEVICE_CNT];

static void bf_add_listener(struct bf_pci_dev *bfdev,
                            struct bf_listener *listener) {
  struct bf_listener **cur_listener = &bfdev->listener_head;

  if (!listener) {
    return;
  }
  spin_lock(&bf_nonisr_lock);

  while (*cur_listener) {
    cur_listener = &((*cur_listener)->next);
  }
  *cur_listener = listener;
  listener->next = NULL;

  spin_unlock(&bf_nonisr_lock);
}

static void bf_remove_listener(struct bf_pci_dev *bfdev,
                               struct bf_listener *listener) {
  struct bf_listener **cur_listener = &bfdev->listener_head;

  /* in case of certain error conditions, this function might be called after
   * bf_pci_remove()
  */
  if (!bfdev || !listener) {
    return;
  }
  spin_lock(&bf_nonisr_lock);

  if (*cur_listener == listener) {
    *cur_listener = listener->next;
  } else {
    while (*cur_listener) {
      if ((*cur_listener)->next == listener) {
        (*cur_listener)->next = listener->next;
        break;
      }
      cur_listener = &((*cur_listener)->next);
    }
    listener->next = NULL;
  }

  spin_unlock(&bf_nonisr_lock);
}

/* a pool of minor numbers is maintained */
/* return the first available minor number */
static int bf_get_next_minor_no(int *minor) {
  int i;

  spin_lock(&bf_nonisr_lock);
  for (i = 0; i < BF_MAX_DEV_SUBDEV_CNT; i++) {
    if (bf_minor[i] == 0) {
      *minor = i;
      bf_minor[i] = 1; /* mark it as taken */
      spin_unlock(&bf_nonisr_lock);
      return 0;
    }
  }
  *minor = -1;
  spin_unlock(&bf_nonisr_lock);
  return -1;
}

/* return a minor number back to the pool  for recycling */
static int bf_return_minor_no(int minor) {
  int err;

  spin_lock(&bf_nonisr_lock);
  if (bf_minor[minor] == 0) { /* was already returned */
    err = -1;                 /* don't change anything, but return error */
  } else {
    bf_minor[minor] = 0; /* mark it as available */
    err = 0;
  }
  spin_unlock(&bf_nonisr_lock);
  return err;
}

static inline struct bf_pci_dev *bf_get_pci_dev(struct bf_dev_info *info) {
  return container_of(info, struct bf_pci_dev, info);
}

/*
 * It masks the msix on/off of generating MSI-X messages.
 */
static void bf_msix_mask_irq(struct msi_desc *desc, int32_t state) {
  u32 mask_bits = desc->masked;
  unsigned offset = desc->msi_attrib.entry_nr * PCI_MSIX_ENTRY_SIZE +
                    PCI_MSIX_ENTRY_VECTOR_CTRL;

  if (state != 0) {
    mask_bits &= ~PCI_MSIX_ENTRY_CTRL_MASKBIT;
  } else {
    mask_bits |= PCI_MSIX_ENTRY_CTRL_MASKBIT;
  }

  if (mask_bits != desc->masked) {
    writel(mask_bits, desc->mask_base + offset);
    readl(desc->mask_base);
    desc->masked = mask_bits;
  }
}

/**
 * irqcontrol can be used to disable/enable interrupt from user space processes.
 *
 * @param bf_dev
 *  pointer to bf_pci_dev
 * @param irq_state
 *  state value. 1 to enable interrupt, 0 to disable interrupt.
 *
 * @return
 *  - On success, 0.
 *  - On failure, a negative value.
 */
static int bf_pci_irqcontrol(struct bf_pci_dev *bfdev, s32 irq_state) {
  struct pci_dev *pdev = bfdev->pdev;

  pci_cfg_access_lock(pdev);
  if (bfdev->mode == BF_INTR_MODE_LEGACY) {
    pci_intx(pdev, !!irq_state);
  } else if (bfdev->mode == BF_INTR_MODE_MSIX) {
    struct msi_desc *desc;
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 2, 0)
    list_for_each_entry(desc, &pdev->msi_list, list)
        bf_msix_mask_irq(desc, irq_state);
#else
    for_each_pci_msi_entry(desc, pdev) bf_msix_mask_irq(desc, irq_state);
#endif
  }
  pci_cfg_access_unlock(pdev);

  return 0;
}

#ifdef BF_INCLUDE_KPKT
/* there are three TBUS MSIX vectors */
static int bf_irq_is_tbus_msix(struct bf_pci_dev *bfdev, int irq) {
  struct bf_dev_info *info = &bfdev->info;

  if (!info->tbus_msix_map_enable) {
    return 0;
  }
  if (irq == info->msix_entries[info->tbus_msix_ind[0]].vector ||
      irq == info->msix_entries[info->tbus_msix_ind[1]].vector) {
    return 1;
  } else if (irq == info->msix_entries[info->tbus_msix_ind[2]].vector) {
    /* log error */
    printk(KERN_ALERT "bf_tbus error msix\n");
    return 1;
  }
  return 0;
}
#endif

/**
 * interrupt handler which will check if the interrupt is from the right
 * device. If so, disable it here and will be enabled later.
 */
static irqreturn_t bf_pci_irqhandler(int irq, struct bf_pci_dev *bfdev) {
  /* Legacy mode need to mask in hardware */
  if (bfdev->mode == BF_INTR_MODE_LEGACY &&
      !pci_check_and_mask_intx(bfdev->pdev)) {
    return IRQ_NONE;
  }

  /* NOTE : if bfdev->info.pci_error_state == 1, then do not access the
   * device and return IRQ_NOTHANDLED.
   */
#ifdef BF_INCLUDE_KPKT
  /* handle pkt DR interrpt (MSI vect-1) if it has to be in kernel */
  if (kpkt_dr_int_en && bfdev->info.irq != 0) {
    if (bfdev->mode == BF_INTR_MODE_LEGACY) {
      bf_kpkt_irqhandler(irq, bfdev->adapter_ptr);
    } else if (bfdev->mode == BF_INTR_MODE_MSI) {
      /* do not process packet unless the MSI interrupt is from tbus */
      /* all BF interrupts arrive on one single MSI if "1" MSI is configured */
      if (bfdev->info.num_irq == 1 || (irq == (bfdev->info.irq + BF_MSI_INT_TBUS))) {
        bf_kpkt_irqhandler(irq, bfdev->adapter_ptr);
      }
    } else if (bfdev->mode == BF_INTR_MODE_MSIX) {
      if ((bfdev->info.tof_type == BF_TOFINO_2 || bfdev->info.tof_type == BF_TOFINO_3) && bf_irq_is_tbus_msix(bfdev, irq)) {
        bf_kpkt_irqhandler(irq, bfdev->adapter_ptr);
      }
    }
  }
#endif
  /* Message signal mode, no share IRQ and automasked */
  return IRQ_HANDLED;
}

/* Remap pci resources described by bar #pci_bar */
static int bf_pci_setup_iomem(struct pci_dev *dev,
                              struct bf_dev_info *info,
                              int n,
                              int pci_bar,
                              const char *name) {
  unsigned long addr, len;
  void *internal_addr;

  if (sizeof(info->mem) / sizeof(info->mem[0]) <= n) {
    return -EINVAL;
  }

  addr = pci_resource_start(dev, pci_bar);
  len = pci_resource_len(dev, pci_bar);
  if (addr == 0 || len == 0) {
    return -1;
  }
  internal_addr = pci_ioremap_bar(dev, pci_bar);
  if (internal_addr == NULL) {
    return -1;
  }
  info->mem[n].name = name;
  info->mem[n].addr = addr;
  info->mem[n].internal_addr = internal_addr;
  info->mem[n].size = len;
  return 0;
}

/* Unmap previously ioremap'd resources */
static void bf_pci_release_iomem(struct bf_dev_info *info) {
  int i;

  for (i = 0; i < BF_MAX_BAR_MAPS; i++) {
    if (info->mem[i].internal_addr) {
      iounmap(info->mem[i].internal_addr);
    }
  }
}

static int bf_setup_bars(struct pci_dev *dev, struct bf_dev_info *info) {
  int i, iom, ret;
  unsigned long flags;
  static const char *bar_names[BF_MAX_BAR_MAPS] = {
      "BAR0", "BAR1", "BAR2", "BAR3", "BAR4", "BAR5",
  };

  iom = 0;

  for (i = 0; i < BF_MAX_BAR_MAPS; i++) {
    if (pci_resource_len(dev, i) != 0 && pci_resource_start(dev, i) != 0) {
      flags = pci_resource_flags(dev, i);
      if (flags & IORESOURCE_MEM) {
        ret = bf_pci_setup_iomem(dev, info, iom, i, bar_names[i]);
        if (ret != 0) {
          return ret;
        }
        iom++;
      }
    }
  }
  return (iom != 0) ? ret : -ENOENT;
}

static irqreturn_t bf_interrupt(int irq, void *bfdev_id) {
  struct bf_pci_dev *bfdev = ((struct bf_int_vector *)bfdev_id)->bf_dev;
  int vect_off = ((struct bf_int_vector *)bfdev_id)->int_vec_offset;

  irqreturn_t ret = bf_pci_irqhandler(irq, bfdev);

  if (ret == IRQ_HANDLED) {
    atomic_inc(&(bfdev->info.event[vect_off]));
  }
  return ret;
}

static unsigned int bf_poll(struct file *filep, poll_table *wait) {
  struct bf_listener *listener = (struct bf_listener *)filep->private_data;
  struct bf_pci_dev *bfdev = listener->bfdev;
  int i;

  if (!bfdev) {
    return -ENODEV;
  }
  if (!bfdev->info.irq) {
    return -EIO;
  }

  poll_wait(filep, &bfdev->info.wait, wait);

  for (i = 0; i < BF_MSIX_ENTRY_CNT; i++) {
    if (listener->event_count[i] != atomic_read(&bfdev->info.event[i])) {
      return POLLIN | POLLRDNORM;
    }
  }
  return 0;
}

static int bf_find_mem_index(struct vm_area_struct *vma) {
  struct bf_pci_dev *bfdev = vma->vm_private_data;
  if (vma->vm_pgoff < BF_MAX_BAR_MAPS) {
    if (bfdev->info.mem[vma->vm_pgoff].size == 0) {
      return -1;
    }
    return (int)vma->vm_pgoff;
  }
  return -1;
}

static const struct vm_operations_struct bf_physical_vm_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
    .access = generic_access_phys,
#endif
};

static int bf_mmap_physical(struct vm_area_struct *vma) {
  struct bf_pci_dev *bfdev = vma->vm_private_data;
  int bar = bf_find_mem_index(vma);
  struct bf_dev_mem *mem;
  if (bar < 0) {
    return -EINVAL;
  }

  mem = bfdev->info.mem + bar;

  if (mem->addr & ~PAGE_MASK) {
    return -ENODEV;
  }
  if (vma->vm_end - vma->vm_start > mem->size) {
    return -EINVAL;
  }

  vma->vm_ops = &bf_physical_vm_ops;
  vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

  /*
   * We cannot use the vm_iomap_memory() helper here,
   * because vma->vm_pgoff is the map index we looked
   * up above in bf_find_mem_index(), rather than an
   * actual page offset into the mmap.
   *
   * So we just do the physical mmap without a page
   * offset.
   */
  return remap_pfn_range(vma,
                         vma->vm_start,
                         mem->addr >> PAGE_SHIFT,
                         vma->vm_end - vma->vm_start,
                         vma->vm_page_prot);
}

static int bf_mmap(struct file *filep, struct vm_area_struct *vma) {
  struct bf_listener *listener = filep->private_data;
  struct bf_pci_dev *bfdev = listener->bfdev;
  int bar;
  unsigned long requested_pages, actual_pages;

  if (!bfdev) {
    return -ENODEV;
  }
  if (vma->vm_end < vma->vm_start) {
    return -EINVAL;
  }

  vma->vm_private_data = bfdev;

  bar = bf_find_mem_index(vma);
  if (bar < 0) {
    return -EINVAL;
  }

  requested_pages = vma_pages(vma);
  actual_pages = ((bfdev->info.mem[bar].addr & ~PAGE_MASK) +
                  bfdev->info.mem[bar].size + PAGE_SIZE - 1) >>
                 PAGE_SHIFT;
  if (requested_pages > actual_pages) {
    return -EINVAL;
  }

  return bf_mmap_physical(vma);
}

static int bf_fasync(int fd, struct file *filep, int mode) {
  int minor;
  int res;

  if (!filep->private_data) {
    return (-EINVAL);
  }
  minor = ((struct bf_listener *)filep->private_data)->minor;
  if (minor >= BF_MAX_DEVICE_CNT) {
    return (-EINVAL);
  }
  if (mode == 0 && &bf_global[minor].async_queue == NULL) {
    bf_global[minor].pending_signal = false;
    return 0; /* nothing to do */
  }
  res = fasync_helper(fd, filep, mode, &bf_global[minor].async_queue);
  if (bf_global[minor].pending_signal && bf_global[minor].async_queue) {
    kill_fasync(&bf_global[minor].async_queue, SIGIO, POLL_ERR);
  }
  bf_global[minor].pending_signal = false;
  return res;
}

static int bf_open(struct inode *inode, struct file *filep) {
  unsigned id;
  struct bf_pci_dev *bfdev;
  struct bf_listener *listener;
  int i;

  id = iminor(inode);
  if (id >= BF_MAX_DEVICE_CNT) {
    return (-EINVAL);
  }

  bfdev = bf_global[id].bfdev;
  if (bfdev->in_use) {
    return (-EBUSY);
  }
  listener = kmalloc(sizeof(*listener), GFP_KERNEL);
  if (listener) {
    listener->bfdev = bfdev;
    listener->minor = bfdev->info.minor;
    listener->next = NULL;
    bf_add_listener(bfdev, listener);
    for (i = 0; i < BF_MSIX_ENTRY_CNT; i++) {
      listener->event_count[i] = atomic_read(&bfdev->info.event[i]);
    }
    filep->private_data = listener;
    bfdev->in_use = 1;
    return 0;
  } else {
    return (-ENOMEM);
  }
}

static int bf_release(struct inode *inode, struct file *filep) {
  struct bf_listener *listener = filep->private_data;

  bf_fasync(-1, filep, 0); /* empty any process id in the notification list */
  if (listener->bfdev) {
    bf_remove_listener(listener->bfdev, listener);
    listener->bfdev->in_use = 0;
  }
  kfree(listener);
  return 0;
}

/* user space support: make read() system call after poll() of select() */
static ssize_t bf_read(struct file *filep,
                       char __user *buf,
                       size_t count,
                       loff_t *ppos) {
  struct bf_listener *listener = filep->private_data;
  struct bf_pci_dev *bfdev = listener->bfdev;
  int retval, event_count[BF_MSIX_ENTRY_CNT];
  int i, mismatch_found = 0;                  /* OR of per vector mismatch */
  unsigned char cnt_match[BF_MSIX_ENTRY_CNT]; /* per vector mismatch */

  if (!bfdev) {
    return -ENODEV;
  }
  /* irq must be setup for read() to work */
  if (!bfdev->info.irq) {
    return -EIO;
  }

  /* ensure that there is enough space on user buffer for the given interrupt
   * mode */
  if (bfdev->mode == BF_INTR_MODE_MSIX) {
    if (count < sizeof(s32) * BF_MSIX_ENTRY_CNT) {
      return -EINVAL;
    }
    count = sizeof(s32) * BF_MSIX_ENTRY_CNT;
  } else if (bfdev->mode == BF_INTR_MODE_MSI) {
    if (count < sizeof(s32) * BF_MSI_ENTRY_CNT) {
      return -EINVAL;
    }
    count = sizeof(s32) * BF_MSI_ENTRY_CNT;
  } else {
    if (count < sizeof(s32)) {
      return -EINVAL;
    }
    count = sizeof(s32);
  }

  do {
    set_current_state(TASK_INTERRUPTIBLE);

    for (i = 0; i < (count / sizeof(s32)); i++) {
      event_count[i] = atomic_read(&(bfdev->info.event[i]));
      if (event_count[i] != listener->event_count[i]) {
        mismatch_found |= 1;
        cnt_match[i] = 1;
      } else {
        event_count[i] = 0;
        cnt_match[i] = 0;
      }
    }
    if (mismatch_found) {
      __set_current_state(TASK_RUNNING);
      if (copy_to_user(buf, &event_count, count)) {
        retval = -EFAULT;
      } else { /* adjust the listener->event_count; */
        for (i = 0; i < (count / sizeof(s32)); i++) {
          if (cnt_match[i]) {
            listener->event_count[i] = event_count[i];
          }
        }
        retval = count;
      }
      break;
    }

    if (filep->f_flags & O_NONBLOCK) {
      retval = -EAGAIN;
      break;
    }

    if (signal_pending(current)) {
      retval = -ERESTARTSYS;
      break;
    }
    schedule();
  } while (1);

  __set_current_state(TASK_RUNNING);

  return retval;
}

/* user space is supposed to call this after it is done with interrupt
 * processing
 */
static ssize_t bf_write(struct file *filep,
                        const char __user *buf,
                        size_t count,
                        loff_t *ppos) {
  struct bf_listener *listener = filep->private_data;
  struct bf_pci_dev *bfdev = listener->bfdev;
  ssize_t ret;
  s32 int_en;

  if (!bfdev || !bfdev->info.irq) {
    return -EIO;
  }

  if (count != sizeof(s32)) {
    return -EINVAL;
  }

  if (copy_from_user(&int_en, buf, count)) {
    return -EFAULT;
  }

  /* clear pci_error_state */
  bfdev->info.pci_error_state = 0;

  ret = bf_pci_irqcontrol(bfdev, int_en);

  return ret ? ret : sizeof(s32);
}

static long bf_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
  struct bf_listener *listener = filep->private_data;
  struct bf_pci_dev *bfdev = listener->bfdev;
  bf_dma_bus_map_t dma_map;
  void *addr = (void __user *)arg;
  dma_addr_t dma_hndl;

  if (!bfdev || !addr) {
    return EFAULT;
  }
  switch(cmd) {
  case BF_IOCMAPDMAADDR:
#if defined(RHEL_RELEASE_CODE)
#if defined(RHEL_RELEASE_VERSION)
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 9)
    if (access_ok(addr, sizeof(bf_dma_bus_map_t))) {
#else
    if (access_ok(VERIFY_WRITE, addr, sizeof(bf_dma_bus_map_t))) {
#endif
#endif /* RHEL_RELEASE_CODE && RHEL_RELEASE_VERSION */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    if (access_ok(addr, sizeof(bf_dma_bus_map_t))) {
#else
    if (access_ok(VERIFY_WRITE, addr, sizeof(bf_dma_bus_map_t))) {
#endif
      if (copy_from_user(&dma_map, addr, sizeof(bf_dma_bus_map_t))) {
        return EFAULT;
      }
      if (!dma_map.phy_addr || !dma_map.size) {
        return EFAULT;
      }
      dma_hndl = dma_map_single(&bfdev->pdev->dev, phys_to_virt(dma_map.phy_addr), dma_map.size, DMA_BIDIRECTIONAL);
      if (dma_mapping_error(&bfdev->pdev->dev, dma_hndl)) {
        return EFAULT;
      }
      dma_map.dma_addr = (void *)(uintptr_t)dma_hndl;
      if (copy_to_user(addr, &dma_map, sizeof(bf_dma_bus_map_t))) {
        return EFAULT;
      }
    } else {
      return EFAULT;
    }
    break;
  case BF_IOCUNMAPDMAADDR:
#if defined(RHEL_RELEASE_CODE)
#if defined(RHEL_RELEASE_VERSION)
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 9)
    if (access_ok(addr, sizeof(bf_dma_bus_map_t))) {
#else
    if (access_ok(VERIFY_WRITE, addr, sizeof(bf_dma_bus_map_t))) {
#endif
#endif /* RHEL_RELEASE_CODE && RHEL_RELEASE_VERSION */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
    if (access_ok(addr, sizeof(bf_dma_bus_map_t))) {
#else
    if (access_ok(VERIFY_READ, addr, sizeof(bf_dma_bus_map_t))) {
#endif
      if (copy_from_user(&dma_map, addr, sizeof(bf_dma_bus_map_t))) {
        return EFAULT;
      }
      if (!dma_map.dma_addr || !dma_map.size) {
        return EFAULT;
      }
      dma_unmap_single(&bfdev->pdev->dev, (dma_addr_t)(uintptr_t)(dma_map.dma_addr), dma_map.size, DMA_BIDIRECTIONAL);
    } else {
      return EFAULT;
    }
    break;
  case BF_TBUS_MSIX_INDEX:
    /* not supported for Tofino-1 */
    if (bfdev->info.tof_type == BF_TOFINO_1) {
      return EINVAL;
    } else {
      int i;
      bf_tbus_msix_indices_t msix_ind;
#if defined(RHEL_RELEASE_CODE)
#if defined(RHEL_RELEASE_VERSION)
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 9)
      if (access_ok(addr, sizeof(bf_tbus_msix_indices_t))) {
#else
      if (access_ok(VERIFY_WRITE, addr, sizeof(bf_tbus_msix_indices_t))) {
#endif
#endif /* RHEL_RELEASE_CODE && RHEL_RELEASE_VERSION */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0)
      if (access_ok(addr, sizeof(bf_tbus_msix_indices_t))) {
#else
      if (access_ok(VERIFY_READ, addr, sizeof(bf_tbus_msix_indices_t))) {
#endif
        if (copy_from_user(&msix_ind, addr, sizeof(bf_tbus_msix_indices_t))) {
          return EFAULT;
        }
        if (msix_ind.cnt < BF_TBUS_MSIX_INDICES_MIN ||
            msix_ind.cnt > BF_TBUS_MSIX_INDICES_MAX) {
          return EINVAL;
        }
        for (i = 0; i < msix_ind.cnt; i++) {
          if (msix_ind.indices[i] <  0 ||
              msix_ind.indices[i] >= BF_MSIX_ENTRY_CNT) {
            return EINVAL;
          }
        }
        for (i = 0; i < msix_ind.cnt; i++) {
          bfdev->info.tbus_msix_ind[i] = msix_ind.indices[i];
        }
        bfdev->info.tbus_msix_map_enable = 1;
      } else {
	    return EFAULT;
      }
    }
    break;
  case BF_GET_INTR_MODE:
    {
      bf_intr_mode_t i_mode;
      i_mode.intr_mode = bfdev->mode;
      if (copy_to_user(addr, &i_mode, sizeof(bf_intr_mode_t))) {
        return EFAULT;
      }
    }
    break;
  default:
    return EINVAL;
  }
  return 0;
}

static const struct file_operations bf_fops = {
    .owner = THIS_MODULE,
    .open = bf_open,
    .release = bf_release,
    .unlocked_ioctl = bf_ioctl,
    .read = bf_read,
    .write = bf_write,
    .mmap = bf_mmap,
    .poll = bf_poll,
    .fasync = bf_fasync,
};

static int bf_major_init(struct bf_pci_dev *bfdev, int minor) {
  struct cdev *cdev;
  static const char name[] = "bf";
  dev_t bf_dev = 0;
  int result;

  result = alloc_chrdev_region(&bf_dev, 0, BF_MAX_DEVICE_CNT, name);
  if (result) {
    return result;
  }

  result = -ENOMEM;
  cdev = cdev_alloc();
  if (!cdev) {
    goto fail_dev_add;
  }
  cdev->ops = &bf_fops;
  cdev->owner = THIS_MODULE;
  kobject_set_name(&cdev->kobj, "%s", name);
  result = cdev_add(cdev, bf_dev, BF_MAX_DEVICE_CNT);

  if (result) {
    goto fail_dev_add;
  }

  bfdev->info.major = MAJOR(bf_dev);
  bf_global[minor].bf_cdev = cdev;
  return 0;

fail_dev_add:
  unregister_chrdev_region(bf_dev, BF_MAX_DEVICE_CNT);
  return result;
}

static void bf_major_cleanup(struct bf_pci_dev *bfdev, int minor) {
  unregister_chrdev_region(MKDEV(bfdev->info.major, 0), BF_MAX_DEVICE_CNT);
  cdev_del(bf_global[minor].bf_cdev);
}

static int bf_init_cdev(struct bf_pci_dev *bfdev, int minor) {
  int ret;
  ret = bf_major_init(bfdev, minor);
  return ret;
}

static void bf_remove_cdev(struct bf_pci_dev *bfdev) {
  bf_major_cleanup(bfdev, bfdev->info.minor);
}

/* return the first unused dev_id based on invalid chip_id */
static int bf_multisub_tof_unused_devid_get(void) {
  int i;
  for (i = 0; i < BF_MAX_DEVICE_CNT; i++) {
    if ((bf_tof3_info[i]).minor[0] == -1 &&
        (bf_tof3_info[i]).minor[1] == -1) {
      return i;
    }
  }
  return -1;
}

/* special case handling for TOF3. each subdevice creates its own device node
 * device node is named as /dev/bf<devid>s<subdevid 0-3>
*/
static int bf_tof3_register_device(struct device *parent,
                                   struct bf_pci_dev *bfdev) {
  struct bf_dev_info *info = &bfdev->info;
  int minor = 0;
  u8 *bf_base_addr;
  volatile u32 *bf_addr;
  int dev_id = 0, subdev_id = 0, ret = 0;

  if (!info || !info->version) {
    return -EINVAL;
  }
  bf_base_addr = (info->mem[0].internal_addr);
  if (!bf_base_addr) {
    return -ENODEV;
  }
  /* *** TBD  for multi Tofino(with 2 subdevices) systems *** */
  /* We must be able to identify multiple sub devices as  belonging to one
     physical Tofino(3) device. We have not figured that out yet.
     until then, we support only one CB device per host CPU */
  bf_addr = (u32 *)((u8 *)bf_base_addr + TOFINO3_MISC_PAD_STATUS_OFFSET);
#if 1 /* USING EMULATOR where subdevice info is not possible to have */
  bf_multisub_tof_unused_devid_get(); /* keep compiler happy */
  subdev_id = 0;
  if (bf_get_next_minor_no(&minor)) {
    return -EINVAL;
  }
  dev_id = minor;
#else
  subdev_id = (int)(*bf_addr & TOFINO3_MISC_PAD_STATUS_DIEID0);
  if (bf_get_next_minor_no(&minor)) {
    return -EINVAL;
  }
  /* we cannot assume the order in which sub devices are probed */
  if (subdev_id == 0) {
    dev_id = bf_multisub_tof_unused_devid_get();
    bf_tof3_info[dev_id].dev_id = dev_id; /* back reference */
    (bf_tof3_info[dev_id].minor)[subdev_id] = minor;
  } else {
    dev_id = 0; /* TBD : for Tofino with multi sub devices */
    (bf_tof3_info[dev_id].minor)[subdev_id] = minor;
  }
#endif
  ret = bf_init_cdev(bfdev, minor);
  if (ret) {
    printk(KERN_ERR "BF: device cdev creation failed dev_id %d\n", dev_id);
    return ret;
  }
  info->tof3_info = &(bf_tof3_info[dev_id]);
  info->dev_id = dev_id;
  info->subdev_id = subdev_id;
  printk(KERN_NOTICE "BF: registering dev_id %d subdev_id %d\n",
         dev_id, subdev_id);
  info->dev = device_create(bf_class, parent, MKDEV(bfdev->info.major, minor),
                            bfdev, "bf%ds%1d", dev_id, subdev_id);
  info->minor = minor;
  return 0;
}

/**
 * bf_register_device - register a new userspace mem device
 * @parent:     parent device
 * @bfdev:      bf pci device
 *
 * returns zero on success or a negative error code.
 */
int bf_register_device(struct device *parent, struct bf_pci_dev *bfdev) {
  struct bf_dev_info *info = &bfdev->info;
  int i, j, ret = 0;
  int minor = 0;

  if (!parent || !info || !info->version) {
    return -EINVAL;
  }

  init_waitqueue_head(&info->wait);

  for (i = 0; i < BF_MSIX_ENTRY_CNT; i++) {
    atomic_set(&info->event[i], 0);
  }

  if (info->tof_type == BF_TOFINO_3) {
    if ((ret = bf_tof3_register_device(parent, bfdev)) != 0) {
      printk(KERN_ERR "BF: TOF3 device cdev creation failed %d\n", ret);
      return ret;
    }
  } else {
    if (bf_get_next_minor_no(&minor)) {
      return -EINVAL;
    }

    ret = bf_init_cdev(bfdev, minor);
    if (ret) {
      printk(KERN_ERR "BF: device cdev creation failed\n");
      return ret;
    }

    info->dev = device_create(
      bf_class, parent, MKDEV(bfdev->info.major, minor), bfdev, "bf%d", minor);
    info->minor = minor;
    info->dev_id = minor;
    info->subdev_id = 0;
  }
  if (!info->dev) {
    printk(KERN_ERR "BF: device creation failed\n");
    return -ENODEV;
  }

  /* bind ISRs and request interrupts */
  if (info->irq && (bfdev->mode != BF_INTR_MODE_NONE)) {
    /*
     * Note that we deliberately don't use devm_request_irq
     * here. The parent module can unregister the UIO device
     * and call pci_disable_msi, which requires that this
     * irq has been freed. However, the device may have open
     * FDs at the time of unregister and therefore may not be
     * freed until they are released.
     */
    if (bfdev->mode == BF_INTR_MODE_LEGACY) {
      ret = request_irq(info->irq,
                        bf_interrupt,
                        info->irq_flags,
                        bfdev->name,
                        (void *)&(bfdev->bf_int_vec[0]));
      if (ret) {
        printk(KERN_ERR "bf failed to request legacy irq %ld error %d\n",
               info->irq,
               ret);
        return ret;
      }
      printk(KERN_NOTICE "BF allocating legacy int vector %ld\n", info->irq);
    } else if (bfdev->mode == BF_INTR_MODE_MSIX) {
      for (i = 0; i < info->num_irq; i++) {
        ret = request_irq(info->msix_entries[i].vector,
                          bf_interrupt,
                          info->irq_flags,
                          bfdev->name,
                          (void *)&(bfdev->bf_int_vec[i]));
        if (ret) {
          /* undo all other previous bindings */
          printk(KERN_ERR "bf failed to request MSIX ret %d itr %d\n", ret, i);
          for (j = i - 1; j >= 0; j--) {
            free_irq(info->msix_entries[j].vector,
                     (void *)&(bfdev->bf_int_vec[j]));
          }
          return ret;
        }
      }
      printk(KERN_NOTICE "BF allocating %d MSIx vectors from  %ld\n",
             info->num_irq,
             info->irq);
    } else if (bfdev->mode == BF_INTR_MODE_MSI) {
      for (i = 0; i < info->num_irq; i++) {
        ret = request_irq(info->irq + i,
                          bf_interrupt,
                          info->irq_flags,
                          bfdev->name,
                          (void *)&(bfdev->bf_int_vec[i]));
        if (ret) {
          /* undo all other previous bindings */
          printk(KERN_ERR "bf failed to request MSI ret %d itr %d\n", ret, i);
          for (j = i - 1; j >= 0; j--) {
            free_irq(info->irq + j, (void *)&(bfdev->bf_int_vec[j]));
          }
          return ret;
        }
      }
      printk(KERN_NOTICE "BF allocating %d MSI vectors from  %ld\n",
             info->num_irq,
             info->irq);
    }
  }
  return 0;
}

/* special case handling for TOF3.  return minor number only after all
 * sub devices using the minor number are unregistered */
static int bf_tof3_unregister_device(struct bf_pci_dev *bfdev) {
  struct bf_dev_info *info = &bfdev->info;
#if 1 //HACK until emulator implements efuse
  bf_return_minor_no(info->minor);
#else
  int j, dev_id, subdev_id, found;

  if (!info->tof3_info) {
    printk(KERN_ERR "BF TOF3 bad info in tof3_unregister_device\n");
    return -1;
  }
  dev_id = info->tof3_info->dev_id;
  subdev_id = info->subdev_id;
  if (dev_id >= BF_MAX_DEVICE_CNT || subdev_id >= BF_MAX_SUBDEV_CNT) {
    return -1;
  }
  /* update bf_tof3_info structure for the device being unregistered */
  (bf_tof3_info[dev_id].minor)[subdev_id] = -1;
  /* return the minor number */
  bf_return_minor_no(info->minor);
  info->subdev_id = -1;
#endif
  return 0;
}

/**
 * bf_unregister_device - register a new userspace mem device
 * @bfdev:      bf pci device
 *
 * returns none
 */
void bf_unregister_device(struct bf_pci_dev *bfdev) {
  struct bf_dev_info *info = &bfdev->info;
  int i, ret;

  if (!info) {
    printk(KERN_ERR "BF: bad data in device unregister\n");
    return;
  }
  if (info->irq) {
    if (bfdev->mode == BF_INTR_MODE_LEGACY) {
      free_irq(info->irq, (void *)&(bfdev->bf_int_vec[0]));
    } else if (bfdev->mode == BF_INTR_MODE_MSIX) {
      for (i = 0; i < info->num_irq; i++) {
        free_irq(info->msix_entries[i].vector, (void *)&(bfdev->bf_int_vec[i]));
      }
    } else if (bfdev->mode == BF_INTR_MODE_MSI) {
      for (i = 0; i < info->num_irq; i++) {
        free_irq(info->irq + i, (void *)&(bfdev->bf_int_vec[i]));
      }
    }
  }
  device_destroy(bf_class, MKDEV(info->major, info->minor));
  bf_remove_cdev(bfdev);
  if (bfdev->info.tof_type == BF_TOFINO_3) {
    if ((ret = bf_tof3_unregister_device(bfdev)) != 0) {
      printk(KERN_ERR "BF: TOF3 device cdev unregister failed %d\n", ret);
      return;
    }
  } else {
    bf_return_minor_no(info->minor);
  }
  return;
}

static inline struct device *pci_dev_to_dev(struct pci_dev *pdev) {
  return &pdev->dev;
}

static void bf_disable_int_dma(struct bf_pci_dev *bfdev) {
  u8 *bf_base_addr, i;
  u32 *bf_addr;
  volatile u32 val;

  /* mask interrupts and DMA */
  bf_base_addr = (bfdev->info.mem[0].internal_addr);
  /* return if called before mmap */
  if (!bf_base_addr) {
    return;
  }
  /* mask interrupts at shadow level */
  bf_addr = (u32 *)((u8 *)bf_base_addr + 0xc0);
  for (i = 0; i < 16; i++) {
    *bf_addr = 0xffffffffUL;
    bf_addr++;
  }
  /* mask DMA */
  bf_addr = (u32 *)((u8 *)bf_base_addr + 0x14);
  val = *bf_addr;
  val &= 0xfffffffeUL;
  *bf_addr = val;
}

static int bf_pci_probe(struct pci_dev *pdev, const struct pci_device_id *id) {
  struct bf_pci_dev *bfdev;
  int err, pci_use_highmem;
  int i, num_irq;

  bfdev = kzalloc(sizeof(struct bf_pci_dev), GFP_KERNEL);
  if (!bfdev) {
    return -ENOMEM;
  }

  /* init the cookies to be passed to ISRs */
  for (i = 0; i < BF_MSIX_ENTRY_CNT; i++) {
    bfdev->bf_int_vec[i].int_vec_offset = i;
    bfdev->bf_int_vec[i].bf_dev = bfdev;
  }

  /* initialize intr_mode to none */
  bfdev->mode = BF_INTR_MODE_NONE;

  /* clear pci_error_state */
  bfdev->info.pci_error_state = 0;

  /* mark pci device_id type */
  bfdev->info.pci_dev_id = pdev->device;
  switch (pdev->device) {
  case TOFINO2_DEV_ID_A0:
  case TOFINO2_DEV_ID_A00:
  case TOFINO2_DEV_ID_B0:
    bfdev->info.tof_type = BF_TOFINO_2;
    break;
  case TOFINO3_DEV_ID_A0:
    bfdev->info.tof_type = BF_TOFINO_3;
    break;
  default:
    bfdev->info.tof_type = BF_TOFINO_1;
    break;
  }
  /* intialize TBUS MSIX indices */
  for (i = 0; i < BF_TBUS_MSIX_INDICES_MAX; i++) {
    if (bfdev->info.tof_type == BF_TOFINO_1) {
      bfdev->info.tbus_msix_ind[i] = BF_TBUS_MSIX_BASE_INDEX_TOF1 + i;
    } else if (bfdev->info.tof_type == BF_TOFINO_2) {
      bfdev->info.tbus_msix_ind[i] = BF_TBUS_MSIX_INDEX_INVALID;
    } else if (bfdev->info.tof_type == BF_TOFINO_3) {
      bfdev->info.tbus_msix_ind[i] = BF_TBUS_MSIX_INDEX_INVALID;
    }
  }
  /*
   * enable device
   */
  err = pci_enable_device(pdev);
  if (err != 0) {
    printk(KERN_ERR "bf cannot enable PCI device\n");
    goto fail_free;
  }

  /*
   * reserve device's PCI memory regions for use by this
   * module
   */
  err = pci_request_regions(pdev, "bf_umem");
  if (err != 0) {
    printk(KERN_ERR "bf Cannot request regions\n");
    goto fail_pci_disable;
  }
  /* remap IO memory */
  err = bf_setup_bars(pdev, &bfdev->info);
  if (err != 0) {
    printk(KERN_ERR "bf Cannot setup BARs\n");
    goto fail_release_iomem;
  }

  if (!dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64)) &&
      !dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(64))) {
    pci_use_highmem = 1;
  } else {
    err = dma_set_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
    if (err) {
      err = dma_set_coherent_mask(pci_dev_to_dev(pdev), DMA_BIT_MASK(32));
      if (err) {
        printk(KERN_ERR "bf no usable DMA configuration, aborting\n");
        goto fail_release_iomem;
      }
    }
    pci_use_highmem = 0;
  }

  /* enable pci error reporting */
  /* for the current kernel version, kernel config must have set the followings:
   * CONFIG_PCIEPORTBUS=y and CONFIG_PCIEAER = y
   * we have pci_error_handlers defined that gets invoked by kernel AER module
   * upon detecting the pcie error on this device's addresses.
   * However, there seems no way that AER would pass the offending addresses
   * to the callback functions. AER logs the error messages on the console.
   * This driver's calback function send the SIGIO signal to the user space
   * to indicate the error condition.
   */
  pci_enable_pcie_error_reporting(pdev);

  bf_disable_int_dma(bfdev);

  /* enable bus mastering on the device */
  pci_set_master(pdev);

  /* fill in bfdev info */
  bfdev->info.version = "0.2";
  bfdev->info.owner = THIS_MODULE;
  bfdev->pdev = pdev;
  switch (bf_intr_mode_default) {
#ifdef CONFIG_PCI_MSI
    case BF_INTR_MODE_MSIX:
      /* Only 1 msi-x vector needed */
      bfdev->info.msix_entries =
          kcalloc(BF_MSIX_ENTRY_CNT, sizeof(struct msix_entry), GFP_KERNEL);
      if (!bfdev->info.msix_entries) {
        err = -ENOMEM;
        goto fail_clear_pci_master;
      }
      for (i = 0; i < BF_MSIX_ENTRY_CNT; i++) {
        bfdev->info.msix_entries[i].entry = i;
      }
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
      num_irq = pci_enable_msix(pdev, bfdev->info.msix_entries,
                                BF_MSIX_ENTRY_CNT);
      if (num_irq == 0) {
        bfdev->info.num_irq = BF_MSIX_ENTRY_CNT;
        bfdev->info.irq = bfdev->info.msix_entries[0].vector;
        bfdev->mode = BF_INTR_MODE_MSIX;
        printk(KERN_DEBUG "bf using %d MSIX irq from %ld\n", num_irq,
               bfdev->info.irq);
        break;
      }
#else
      num_irq = pci_enable_msix_range(
          pdev, bfdev->info.msix_entries, BF_MSIX_ENTRY_CNT, BF_MSIX_ENTRY_CNT);
      if (num_irq == BF_MSIX_ENTRY_CNT) {
        bfdev->info.num_irq = num_irq;
        bfdev->info.irq = bfdev->info.msix_entries[0].vector;
        bfdev->mode = BF_INTR_MODE_MSIX;
        printk(KERN_DEBUG "bf using %d MSIX irq from %ld\n",
               num_irq,
               bfdev->info.irq);
        break;
      } else {
        if (num_irq) pci_disable_msix(pdev);
        kfree(bfdev->info.msix_entries);
        bfdev->info.msix_entries = NULL;
        printk(KERN_ERR "bf error allocating MSIX vectors. Trying MSI...\n");
        /* and, fall back to MSI */
      }
#endif /* LINUX_VERSION_CODE */
    /* ** intentional no-break */
    /* FALLTHRU */
    case BF_INTR_MODE_MSI:
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 14, 0)
      num_irq = pci_enable_msi_block(pdev, BF_MSI_ENTRY_CNT);
      /* we must get requested number of MSI vectors enabled */
      if (num_irq == 0) {
        bfdev->info.num_irq = BF_MSI_ENTRY_CNT;
        bfdev->info.irq = pdev->irq;
        bfdev->mode = BF_INTR_MODE_MSI;
        printk(KERN_DEBUG "bf using %d MSI irq from %ld\n", bfdev->info.num_irq,
             bfdev->info.irq);
        break;
      }
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
      num_irq = pci_enable_msi_range(pdev, BF_MSI_ENTRY_CNT, BF_MSI_ENTRY_CNT);
      if (num_irq > 0) {
        bfdev->info.num_irq = num_irq;
        bfdev->info.irq = pdev->irq;
        bfdev->mode = BF_INTR_MODE_MSI;
        printk(KERN_DEBUG "bf using %d MSI irq from %ld\n",
               bfdev->info.num_irq,
               bfdev->info.irq);
        break;
      }
#else
      num_irq = pci_alloc_irq_vectors_affinity(pdev, BF_MSI_ENTRY_CNT,
                    BF_MSI_ENTRY_CNT, PCI_IRQ_MSI | PCI_IRQ_AFFINITY, NULL);
      if (num_irq > 0) {
        bfdev->info.num_irq = num_irq;
        bfdev->info.irq = pci_irq_vector(pdev, 0);
        bfdev->mode = BF_INTR_MODE_MSI;
        printk(KERN_DEBUG "bf using %d MSI irq from %ld\n", bfdev->info.num_irq,
             bfdev->info.irq);
        break;
      }
#endif /* LINUX_VERSION_CODE */
#endif /* CONFIG_PCI_MSI */
    /* fall back to Legacy Interrupt, intentional no-break */
    /* FALLTHRU */

    case BF_INTR_MODE_LEGACY:
      if (pci_intx_mask_supported(pdev)) {
        bfdev->info.irq_flags = IRQF_SHARED;
        bfdev->info.irq = pdev->irq;
        bfdev->mode = BF_INTR_MODE_LEGACY;
        printk(KERN_DEBUG "bf using LEGACY irq %ld\n", bfdev->info.irq);
        break;
      }
      printk(KERN_NOTICE " bf PCI INTx mask not supported\n");
    /* fall back to no Interrupt, intentional no-break */
    /* FALLTHRU */
    case BF_INTR_MODE_NONE:
      bfdev->info.irq = 0;
      bfdev->info.num_irq = 0;
      bfdev->mode = BF_INTR_MODE_NONE;
      break;

    default:
      printk(KERN_DEBUG "bf invalid IRQ mode %u", bf_intr_mode_default);
      err = -EINVAL;
      goto fail_clear_pci_master;
  }

  pci_set_drvdata(pdev, bfdev);
  sprintf(bfdev->name, "bf_%d", bfdev->info.minor);
  /* register bf driver */
  err = bf_register_device(&pdev->dev, bfdev);
  if (err != 0) {
    goto fail_release_irq;
  }

  bf_global[bfdev->info.minor].async_queue = NULL;
  bf_global[bfdev->info.minor].bfdev = bfdev;
  bf_global[bfdev->info.minor].pending_signal = false;

  dev_info(&pdev->dev,
           "bf device %d registered with irq %ld\n",
           bfdev->instance,
           bfdev->info.irq);
  printk(KERN_ALERT "bf probe ok\n");
#ifdef BF_INCLUDE_KPKT
  if (kpkt_mode) {
    err = bf_kpkt_init(pdev,
                       bfdev->info.mem[0].internal_addr,
                       &bfdev->adapter_ptr,
                       bfdev->info.dev_id,
                       bfdev->info.subdev_id,
                       pci_use_highmem,
                       kpkt_hd_room,
                       kpkt_dr_int_en,
                       kpkt_rx_count);
    if (err == 0) {
      printk(KERN_ALERT "bf_kpkt kernel processing enabled for dev %d subdev_id %d\n",
             bfdev->info.dev_id, bfdev->info.subdev_id);
    } else {
      printk(KERN_ERR "error starting bf_kpkt kernel processing for dev %d subdev_id %d\n",
             bfdev->info.dev_id, bfdev->info.subdev_id);
      bfdev->adapter_ptr = NULL;
    }
  }
#endif
  return 0;

fail_release_irq:
  pci_set_drvdata(pdev, NULL);
  if (bfdev->mode == BF_INTR_MODE_MSIX) {
    pci_disable_msix(bfdev->pdev);
    kfree(bfdev->info.msix_entries);
    bfdev->info.msix_entries = NULL;
  } else if (bfdev->mode == BF_INTR_MODE_MSI) {
    pci_disable_msi(bfdev->pdev);
  }
fail_clear_pci_master:
  pci_clear_master(pdev);
fail_release_iomem:
  bf_pci_release_iomem(&bfdev->info);
  pci_release_regions(pdev);
fail_pci_disable:
  pci_disable_device(pdev);
fail_free:
  kfree(bfdev);

  printk(KERN_ERR "bf probe not ok\n");
  return err;
}

static void bf_pci_remove(struct pci_dev *pdev) {
  struct bf_pci_dev *bfdev = pci_get_drvdata(pdev);
  struct bf_listener *cur_listener;

#ifdef BF_INCLUDE_KPKT
  if (kpkt_mode) {
    bf_kpkt_remove(bfdev->adapter_ptr);
  }
#endif
  bf_disable_int_dma(bfdev);
  bf_unregister_device(bfdev);
  if (bfdev->mode == BF_INTR_MODE_MSIX) {
    pci_disable_msix(pdev);
    kfree(bfdev->info.msix_entries);
    bfdev->info.msix_entries = NULL;
  } else if (bfdev->mode == BF_INTR_MODE_MSI) {
    pci_disable_msi(pdev);
  }
  pci_clear_master(pdev);
  bf_pci_release_iomem(&bfdev->info);
  pci_release_regions(pdev);
  pci_disable_pcie_error_reporting(pdev);
  pci_disable_device(pdev);
  pci_set_drvdata(pdev, NULL);
  bf_global[bfdev->info.minor].bfdev = NULL;
  /* existing filep structures in open file(s) must be informed that
   * bf_pci_dev is no longer valid */
  spin_lock(&bf_nonisr_lock);
  cur_listener = bfdev->listener_head;
  while (cur_listener) {
    cur_listener->bfdev = NULL;
    cur_listener = cur_listener->next;
  }
  spin_unlock(&bf_nonisr_lock);
  kfree(bfdev);
}

/* AER support callbacks. Refer to:
 * https://www.kernel.org/doc/Documentation/PCI/pcieaer-howto.txt
 * and
 * https://www.kernel.org/doc/Documentation/PCI/pci-error-recovery.txt
 *
 * from bf_kdrv point of view, AER uncorrected errors (fatal and non-fatal)
 * should not cause pci link reset (upstream port AER callbacks must also
 * support this requirements of bf_kdrv)
 * Device, however, is not expected to function after uncorrected errors
 * but, application has chance to perform diags without resetting pci link
 */
/**
 * bf_pci_error_detected - called when PCI error is detected
 * @pdev: Pointer to PCI device
 * @state: The current pci connection state
 *
 * called when root complex detects pci error associated with the device
 */
static pci_ers_result_t bf_pci_error_detected(struct pci_dev *pdev,
                                              pci_channel_state_t state) {
  struct bf_pci_dev *bfdev = pci_get_drvdata(pdev);

  if (!bfdev) {
    return PCI_ERS_RESULT_NONE;
  }
  printk(KERN_ERR "pci_err_detected state %d\n", state);
  if (state == pci_channel_io_perm_failure || state == pci_channel_io_frozen) {
    bfdev->info.pci_error_state = 1;
#ifdef BF_INCLUDE_KPKT
    if (kpkt_mode) {
      bf_kpkt_set_pci_error(bfdev->adapter_ptr, 1);
    }
#endif
    /* we do not want pci link to go down. The user space application
     * should collect the diag info, terminate the application and unload the
     * kernel module
     */
    return PCI_ERS_RESULT_CAN_RECOVER; /* to prevent pci link down */
  } else {
    return PCI_ERS_RESULT_CAN_RECOVER;
  }
}

static pci_ers_result_t bf_pci_mmio_enabled(struct pci_dev *dev) {
  struct bf_pci_dev *bfdev = pci_get_drvdata(dev);

  printk(KERN_ERR "BF pci_mmio_enabled invoked after pci error\n");
#if defined(RHEL_RELEASE_CODE)
#if defined(RHEL_RELEASE_VERSION)
#if RHEL_RELEASE_CODE > RHEL_RELEASE_VERSION(7, 9)
  pci_aer_clear_nonfatal_status(dev);
#else
  pci_cleanup_aer_uncorrect_error_status(dev);
#endif
#endif /* RHEL_RELEASE_CODE && RHEL_RELEASE_VERSION */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(5, 8, 0)
  pci_aer_clear_nonfatal_status(dev);
#else
  pci_cleanup_aer_uncorrect_error_status(dev);
#endif

  if (bfdev) {
    /* send a signal to the user space program of the error */
    int minor = bfdev->info.minor;
    if (minor < BF_MAX_DEVICE_CNT) {
      if (bf_global[minor].async_queue) {
        kill_fasync(&bf_global[minor].async_queue, SIGIO, POLL_ERR);
      } else {
        bf_global[minor].pending_signal = true;
      }
    }
  }
  return PCI_ERS_RESULT_RECOVERED;
}

/**
 * bf_pci_slot_reset - called after the pci bus has been reset.
 * @pdev: Pointer to PCI device
 *
 * Restart the card from scratch, as if from a cold-boot.
 */
static pci_ers_result_t bf_pci_slot_reset(struct pci_dev *pdev) {
  /* nothing to do for now as we do not expect to get backto normal after
   * a pcie link reset. Not expected to be invoked.
   * TBD: fill in this function if tofino can recover after an error
   */
  printk(KERN_ERR "BF pci_slot_reset invoked after pci error\n");
  return PCI_ERS_RESULT_RECOVERED;
}

/**
 * bf_pci_resume - called when kernel thinks the device is up on PCIe.
 * @pdev: Pointer to PCI device
 *
 * This callback is called when the error recovery driver tells us that
 * its OK to resume normal operation.
 */
static void bf_pci_resume(struct pci_dev *pdev) {
  printk(KERN_ERR "BF io_resume invoked after pci error\n");
}

static int bf_config_intr_mode(char *intr_str) {
  if (!intr_str) {
    pr_info("Use MSI interrupt by default\n");
    return 0;
  }

  if (!strcmp(intr_str, BF_INTR_MODE_MSIX_NAME)) {
    bf_intr_mode_default = BF_INTR_MODE_MSIX;
    pr_info("Use MSIX interrupt\n");
  } else if (!strcmp(intr_str, BF_INTR_MODE_MSI_NAME)) {
    bf_intr_mode_default = BF_INTR_MODE_MSI;
    pr_info("Use MSI interrupt\n");
  } else if (!strcmp(intr_str, BF_INTR_MODE_LEGACY_NAME)) {
    bf_intr_mode_default = BF_INTR_MODE_LEGACY;
    pr_info("Use legacy interrupt\n");
  } else if (!strcmp(intr_str, BF_INTR_MODE_NONE_NAME)) {
    bf_intr_mode_default = BF_INTR_MODE_NONE;
    pr_info("BF interrupt disabled\n");
  } else {
    pr_info("Error: bad intr_mode parameter - %s\n", intr_str);
    return -EINVAL;
  }

  return 0;
}

static const struct pci_device_id bf_pci_tbl[] = {
    {PCI_VDEVICE(BF, TOFINO_DEV_ID_A0), 0},
    {PCI_VDEVICE(BF, TOFINO_DEV_ID_B0), 0},
    {PCI_VDEVICE(BF, TOFINO2_DEV_ID_A0), 0},
    {PCI_VDEVICE(BF, TOFINO2_DEV_ID_A00), 0},
    {PCI_VDEVICE(BF, TOFINO2_DEV_ID_B0), 0},
    {PCI_VDEVICE(INTEL, TOFINO3_DEV_ID_A0), 0},
    /* required last entry */
    {.device = 0}};

/* PCI bus error handlers */
static struct pci_error_handlers bf_pci_err_handler = {
    .error_detected = bf_pci_error_detected,
    .mmio_enabled = bf_pci_mmio_enabled,
    .slot_reset = bf_pci_slot_reset,
    .resume = bf_pci_resume,
};

static struct pci_driver bf_pci_driver = {.name = "bf",
                                          .id_table = bf_pci_tbl,
                                          .probe = bf_pci_probe,
                                          .remove = bf_pci_remove,
                                          .err_handler = &bf_pci_err_handler};

static int __init bfdrv_init(void) {
  int ret;

  pr_info("%s: %s - version %s\n", DRV_NAME(kpkt_mode),
            DRV_DESCRIPTION(kpkt_mode),DRV_VERSION);
  pr_info("%s: %s\n", DRV_NAME(kpkt_mode),DRV_COPYRIGHT);
  memset(bf_global, 0, sizeof(bf_global));
  memset(bf_tof3_info, 0xff, sizeof(bf_tof3_info));
  bf_class = class_create(THIS_MODULE, BF_CLASS_NAME);
  if (!bf_class) {
    printk(KERN_ERR "create_class failed for bf device\n");
    return -ENODEV;
  } else {
    printk(KERN_NOTICE "bf device class created\n");
  }

  ret = bf_config_intr_mode(intr_mode);

  if (ret < 0) {
    printk(KERN_ERR "config interrupt mode failed for bf device\n");
    if (bf_class) {
      class_destroy(bf_class);
      bf_class = NULL;
    }
    return ret;
  }

  /* do not enable DR interrupt if not using MSI or not in kpkt mode */
  if ((bf_intr_mode_default != BF_INTR_MODE_MSI &&
       bf_intr_mode_default != BF_INTR_MODE_LEGACY) || kpkt_mode == 0) {
    kpkt_dr_int_en = 0;
  }

  if (kpkt_mode) {
    printk(KERN_NOTICE "kpkt_mode %d hd_room %d dr_int_en %d rx_count %d\n",
           kpkt_mode,
           kpkt_hd_room,
           kpkt_dr_int_en,
           kpkt_rx_count);
  }

  spin_lock_init(&bf_nonisr_lock);
  return pci_register_driver(&bf_pci_driver);
}

static void __exit bfdrv_exit(void) {
  pr_info("%s: module unloading ...\n", DRV_NAME(kpkt_mode));
  pci_unregister_driver(&bf_pci_driver);
  class_destroy(bf_class);
  pr_info("%s: module unloaded successfully\n", DRV_NAME(kpkt_mode));
  bf_class = NULL;
  intr_mode = NULL;
  kpkt_mode = 0;
}

module_init(bfdrv_init);
module_exit(bfdrv_exit);

module_param(kpkt_mode, int, S_IRUGO);
MODULE_PARM_DESC(kpkt_mode,
                 "bf kernel mode pkt processing (default=off):\n"
                 " 1 Use kernel mode bf_pkt processing\n"
                 " 0 Do not use kernel mode bf_pkt processing\n"
                 "\n");

module_param(kpkt_hd_room, int, S_IRUGO);
MODULE_PARM_DESC(kpkt_hd_room,
                 "head room to reserve when receiving packets (default=32):\n"
                 "\n");

module_param(kpkt_rx_count, int, S_IRUGO);
MODULE_PARM_DESC(kpkt_rx_count,
                 "number of buffers per rx pkt ring (default=256):\n"
                 "\n");
/* dr_int_en is applicable only if MSI interrupt mode is selected */
module_param(kpkt_dr_int_en, int, S_IRUGO);
MODULE_PARM_DESC(kpkt_dr_int_en,
                 "bf pkt Interrupt enable (default=1):\n"
                 " 1 use interrupt\n"
                 " 0 Do not use interrupt\n"
                 "\n");

module_param(intr_mode, charp, S_IRUGO);
MODULE_PARM_DESC(intr_mode,
                 "bf interrupt mode (default=msix):\n"
                 "    " BF_INTR_MODE_MSIX_NAME
                 "       Use MSIX interrupt\n"
                 "    " BF_INTR_MODE_MSI_NAME
                 "        Use MSI interrupt\n"
                 "    " BF_INTR_MODE_LEGACY_NAME
                 "     Use Legacy interrupt\n"
                 "    " BF_INTR_MODE_NONE_NAME
                 "       Use no interrupt\n"
                 "\n");

MODULE_DEVICE_TABLE(pci, bf_pci_tbl);
MODULE_DESCRIPTION("Barefoot Tofino PCI device");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Barefoot Networks");
