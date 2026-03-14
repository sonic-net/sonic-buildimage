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
#ifndef _BF_IOCTL_H_
#define _BF_IOCTL_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>

#ifndef phys_addr_t
typedef uint64_t phys_addr_t;
#endif

#endif /* __KERNEL__ */

#define BF_IOC_MAGIC 'b'
#define BF_TBUS_MSIX_INDICES_MAX   3
#define BF_TBUS_MSIX_INDICES_MIN   1  

typedef struct bf_dma_bus_map_s
{
  phys_addr_t phy_addr;
  void *dma_addr;
  size_t size;
} bf_dma_bus_map_t;

typedef struct bf_tbus_msix_indices_s
{
  int cnt;
  int indices[BF_TBUS_MSIX_INDICES_MAX];
} bf_tbus_msix_indices_t;

enum bf_intr_mode {
  BF_INTR_MODE_NONE = 0,
  BF_INTR_MODE_LEGACY,
  BF_INTR_MODE_MSI,
  BF_INTR_MODE_MSIX,
};

typedef struct bf_intr_mode_s {
  enum bf_intr_mode intr_mode;
} bf_intr_mode_t;

#define BF_IOCMAPDMAADDR    _IOWR(BF_IOC_MAGIC, 0, bf_dma_bus_map_t)
#define BF_IOCUNMAPDMAADDR  _IOW(BF_IOC_MAGIC, 1, bf_dma_bus_map_t)
#define BF_TBUS_MSIX_INDEX  _IOW(BF_IOC_MAGIC, 2, bf_tbus_msix_indices_t)
#define BF_GET_INTR_MODE    _IOR(BF_IOC_MAGIC, 3, bf_intr_mode_t)

#endif /* _BF_IOCTL_H_ */
