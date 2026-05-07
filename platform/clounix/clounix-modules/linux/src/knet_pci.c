/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

#include <linux/pci.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/uaccess.h>

#include "knet_pci.h"
#include "knet_dev.h"

static uint32_t
clx_get_pci_mmio_info(struct pci_dev *dev)
{
    uint32_t rc = 0;
    unsigned long bar_start;
    unsigned long bar_length;
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);

    bar_start = pci_resource_start(dev, clx_pci_cb(pci_dev_data->unit)->mmio_bar);
    bar_length = pci_resource_len(dev, clx_pci_cb(pci_dev_data->unit)->mmio_bar);

    if (0 == pci_request_region(dev, clx_pci_cb(pci_dev_data->unit)->mmio_bar, CLX_DRIVER_NAME)) {
        pci_dev_data->bar_virt = IOREMAP_API(bar_start, bar_length);
        if (pci_dev_data->bar_virt == NULL) {
            rc = -1;
            dbg_print(DBG_INFO, "enable pci dev failed, rc=%d\n", rc);
        }
    }

    pci_dev_data->bar_len = bar_length;
    pci_dev_data->bar_phys = bar_start;

    return (rc);
}

static int
clx_pci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    int rc = 0;
    struct clx_pci_dev_s *pci_dev_data;

    rc = pci_enable_device(dev);
    if (rc != 0) {
        dbg_print(DBG_CRIT, "enable pci dev failed, rc=%d\n", rc);
    }

    pci_dev_data = (struct clx_pci_dev_s *)kmalloc(sizeof(struct clx_pci_dev_s), GFP_ATOMIC);

    pci_read_config_word(dev, PCI_DEVICE_ID, &pci_dev_data->device_id);
    pci_read_config_word(dev, PCI_VENDOR_ID, &pci_dev_data->vendor_id);
    pci_read_config_byte(dev, PCI_REVISION_ID, &pci_dev_data->revision);

    pci_dev_data->unit = clx_misc_dev->pci_dev_num;
    pci_dev_data->pci_dev = dev;
    pci_set_drvdata(dev, pci_dev_data);
    clx_misc_dev->clx_pci_dev[clx_misc_dev->pci_dev_num++] = pci_dev_data;

    rc = clx_drv_init(pci_dev_data->unit, dev);
    if (rc != 0) {
        kfree(pci_dev_data);
        return rc;
    }
    clx_get_pci_mmio_info(dev);
    pci_dev_data->read_cb = clx_read_pci_reg;
    pci_dev_data->write_cb = clx_write_pci_reg;

    if (dma_set_mask_and_coherent(&dev->dev,
                                  DMA_BIT_MASK(clx_pci_cb(pci_dev_data->unit)->dma_bit_mask))) {
        dbg_print(DBG_ERR, "dma_set_mask_and_coherent failed.dma_bit_mask:%d\n",
                  clx_pci_cb(pci_dev_data->unit)->dma_bit_mask);
    }

    pci_set_master(dev);

    return rc;
}

static void
clx_pci_remove(struct pci_dev *dev)
{
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);
    pci_release_region(dev, clx_pci_cb(pci_dev_data->unit)->mmio_bar);
    pci_disable_device(dev);
    clx_drv_cleanup(pci_dev_data->unit, dev);
    kfree(pci_dev_data);
    clx_misc_dev->pci_dev_num--;
}

static struct pci_device_id clx_id_tables[] = {
    {PCI_DEVICE(CLX_PCIE_VENDOR_ID_0, PCI_ANY_ID)},
    {PCI_DEVICE(CLX_PCIE_VENDOR_ID_1, PCI_ANY_ID)},
};

static struct pci_driver clx_pci_driver = {
    .name = CLX_DRIVER_NAME,
    .id_table = clx_id_tables,
    .probe = clx_pci_probe,
    .remove = clx_pci_remove,
};

int
clx_read_pci_reg(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len)
{
    uint32_t idx;
    uint32_t count;
    volatile void *ptr_base_addr = clx_misc_dev->clx_pci_dev[unit]->bar_virt;

    if ((!ptr_base_addr || !ptr_data || (offset >= clx_misc_dev->clx_pci_dev[unit]->bar_len)) !=
        0) {
        dbg_print(DBG_CRIT, "Bad parameter!\n");
        return -EINVAL;
    }

    if (0 != ((len % CLX_PCI_BUS_WIDTH))) {
        dbg_print(DBG_CRIT, "Must be 4-byte alignment, %d\n", len);
        return -EINVAL;
    }

    if (CLX_PCI_BUS_WIDTH == len) {
        *ptr_data = readl(ptr_base_addr + offset);
    } else {
        count = len / CLX_PCI_BUS_WIDTH;
        for (idx = 0; idx < count; idx++) {
            *(ptr_data + idx) = readl(ptr_base_addr + offset + idx * sizeof(uint32_t));
        }
    }

    return 0;
}

int
clx_write_pci_reg(int unit, uint32_t offset, uint32_t *ptr_data, uint32_t len)
{
    uint32_t idx;
    uint32_t count;
    volatile void *ptr_base_addr = clx_misc_dev->clx_pci_dev[unit]->bar_virt;

    if ((!ptr_base_addr || !ptr_data || (len % sizeof(uint32_t)) ||
         (offset >= clx_misc_dev->clx_pci_dev[unit]->bar_len)) != 0) {
        dbg_print(DBG_CRIT, "Bad parameter!\n");
        return -EINVAL;
    }

    count = len / CLX_PCI_BUS_WIDTH;
    if (sizeof(uint32_t) == len) {
        // *(ptr_base_addr + offset) = *ptr_data;
        writel(*ptr_data, ptr_base_addr + offset);
    } else {
        for (idx = 0; idx < count; idx++) {
            // *((uint32_t *)(ptr_base_addr + offset + idx * 4)) = *(ptr_data + idx);
            writel(*(ptr_data + idx), ptr_base_addr + offset + idx * 4);
            dbg_print(DBG_DEBUG, "write offset:0x%x, len:0x%x, value:0x%x\n", offset + idx * 4, len,
                      *(ptr_data + idx));
        }
    }
    return 0;
}

int
clx_ioctl_get_pci_dev_info(uint32_t unit, unsigned long arg)
{
    struct clx_pci_dev_s *ptr_pci_dev;
    struct clx_dev_info_s ioc_dev_info;

    for (unit = 0; unit < CLX_MAX_CHIP_NUM; unit++) {
        ptr_pci_dev = clx_misc_dev->clx_pci_dev[unit];
        if (ptr_pci_dev == NULL) {
            break;
        }
        ioc_dev_info.pci_info[unit].vendor_id = ptr_pci_dev->vendor_id;
        ioc_dev_info.pci_info[unit].device_id = ptr_pci_dev->device_id;
        ioc_dev_info.pci_info[unit].revision = ptr_pci_dev->revision;
        ioc_dev_info.pci_info[unit].pci_mmio_start =
            pci_resource_start(ptr_pci_dev->pci_dev, clx_pci_cb(unit)->mmio_bar);
        ioc_dev_info.pci_info[unit].pci_mmio_size =
            pci_resource_len(ptr_pci_dev->pci_dev, clx_pci_cb(unit)->mmio_bar);
        dbg_print(DBG_DEBUG, "start:0x%llx, size:0x%x, bar:%d\n",
                  ioc_dev_info.pci_info[unit].pci_mmio_start,
                  ioc_dev_info.pci_info[unit].pci_mmio_size, clx_pci_cb(unit)->mmio_bar);
    }

    ioc_dev_info.pci_dev_num = unit;

    // Copy data back to user space
    if (copy_to_user((void __user *)arg, &ioc_dev_info, sizeof(ioc_dev_info)))
        return -EFAULT;

    return 0;
}

int
clx_pci_init(void)
{
    int rc = 0;
    rc = pci_register_driver(&clx_pci_driver);
    if (rc < 0) {
        dbg_print(DBG_ERR, "Failed to register PCI driver, rc = %d\n", rc);
    }

    return rc;
}

int
clx_pci_deinit(void)
{
    pci_unregister_driver(&clx_pci_driver);

    return 0;
}
