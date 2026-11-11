#ifdef __cplusplus
extern "C"    {
#endif /* __cplusplus */

#include <linux/device.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/version.h>
#include <linux/interrupt.h>
#include <asm/io.h>

#define DAL_VENDOR_VID      0x2072
#define DAL_WLAKE_DEVICE_ID 0x2021
#define DAL_QLAKE_DEVICE_ID 0x360A

typedef struct gt_drv_pcie_info_s
{
    uint32_t pci_id;
    bool msi;                           /*0:msi, 1:msi-x*/
    uint32_t uio_mem_num;
    char *uio_mem_name[MAX_UIO_MAPS];
    uint8_t uiotobar_map[MAX_UIO_MAPS];
}gt_drv_pcie_info_t;

gt_drv_pcie_info_t g_drv_pcie_info_table[] =
{
    {DAL_WLAKE_DEVICE_ID, 0, 4, {"share_memory", "register", "dma_reg", "dma_mem"}, {0, 4, 5, 0xff}},
    {DAL_QLAKE_DEVICE_ID, 1, 5, {"msi_x", "register", "dma_reg", "pkt_mem_dma", "dma_mem"}, {2, 4, 5, 1, 0xff}},
};

typedef struct uio_priv_data_s
{
    uint32_t dma_mem_size;
    struct pci_dev *pdev;
    struct pci_device_id *id;
    int irq_enabled;
}uio_priv_data_t;

uint32_t dma_mem_size;
static char *p_dma_mem_size = NULL;

int linux_dal_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id);
static void linux_dal_pcie_remove(struct pci_dev *pdev);

module_param(p_dma_mem_size, charp, 0);
MODULE_PARM_DESC(p_dma_mem_size, "Specify DMA memory pool size (default 2MB)");

static struct pci_device_id dal_id_table[] =
{
    {PCI_DEVICE(DAL_VENDOR_VID, DAL_WLAKE_DEVICE_ID)},
    {PCI_DEVICE(DAL_VENDOR_VID, DAL_QLAKE_DEVICE_ID)},
};

static struct pci_driver linux_dal_pci_driver =
{
    .name     = "gt",
    .id_table = dal_id_table,
    .probe    = linux_dal_pcie_probe,
    .remove   = linux_dal_pcie_remove,
};

static irqreturn_t dal_handler(int32_t irq, struct uio_info *uio_info)
{
    return IRQ_HANDLED;
}

static int dal_irq_control(struct uio_info *info, int irq_on)
{
    uio_priv_data_t *priv = (uio_priv_data_t *)info->priv;

    if (irq_on)
    {
        if (!priv->irq_enabled)
        {
            enable_irq(info->irq);
            priv->irq_enabled = 1;
        }
    }
    else
    {
        if (priv->irq_enabled)
        {
            disable_irq(info->irq);
            priv->irq_enabled = 0;
        }
    }

    return 0;
}

#define GT_ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
gt_drv_pcie_info_t *drv_pcie_info_get(uint32_t pci_id)
{
    uint32_t i;
    gt_drv_pcie_info_t *info;

    for (i = 0; i < GT_ARRAY_SIZE(g_drv_pcie_info_table); i++)
    {
        info = &g_drv_pcie_info_table[i];
        if (info->pci_id == pci_id)
        {
            return info;
        }
    }

    return NULL;
}

int linux_dal_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct uio_info *uio_info       = NULL;
    struct uio_info *uio_info_tmp   = NULL;
    uio_priv_data_t *uio_priv       = NULL;
    uint32_t i;
    gt_drv_pcie_info_t *info;
    uint32_t bar_index;
    void *dma_virt_addr;
    dma_addr_t dma_phy_addr;
    char proc_name[128];
    int ret;
    int32_t func;

    printk("Probe vendor:%#x device:%#x subvendor:%#x subdevice:%#x class:%#x mask:%#x\n",
                    id->vendor, id->device, id->subvendor, id->subdevice, id->class, id->class_mask);

    func = PCI_FUNC(pdev->devfn);
    if (func != 0)
    {
        printk("Probe vendor:%#x device:%#x subvendor:%#x subdevice:%#x class:%#x mask:%#x skip\n",
                    id->vendor, id->device, id->subvendor, id->subdevice, id->class, id->class_mask);
        return 0;
    }

    uio_info = (struct uio_info *)kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    if (!uio_info)
    {
        printk("kzalloc(sizeof(struct uio_info)) failed\n");
        goto out_free;
    }
    uio_priv = (uio_priv_data_t *)kzalloc(sizeof(uio_priv_data_t), GFP_KERNEL);
    if (!uio_priv)
    {
        printk("kzalloc(sizeof(struct uio_info)) failed\n");
        goto out_free;
    }

    if (pci_enable_device(pdev))
    {
        printk("pci_enable_device failed\n");
        goto out_disable;
    }

    if (pci_request_regions(pdev, "dal_kernel"))
    {
        printk("pci_request_regions failed\n");
        goto out_unmap;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0))
    ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(64));
    if (ret)
    {
        ret = dma_set_mask(&pdev->dev, DMA_BIT_MASK(32));
        if (ret)
        {
            printk("Could not set PCI DMA Mask\n");
            return ret;
        }
    }
#else
    ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(64));
    if (ret)
    {
        ret = pci_set_dma_mask(pdev, DMA_BIT_MASK(32));
        if (ret)
        {
            printk("Could not set PCI DMA Mask\n");
            return ret;
        }
    }
#endif
    info = drv_pcie_info_get((uint32_t)pdev->device);
    for (i = 0; i < info->uio_mem_num; i++)
    {
        bar_index = info->uiotobar_map[i];
        if (0xff != bar_index)
        {
            uio_info->mem[i].addr = pci_resource_start(pdev, bar_index);
            if (!uio_info->mem[i].addr)
            {
                printk("pci_resource_start failed\n");
                goto out_unmap;
            }
            uio_info->mem[i].internal_addr = pci_ioremap_bar(pdev, bar_index);
            if (!uio_info->mem[i].internal_addr)
            {
                printk("pci_ioremap_bar(BAR0) failed\n");
                goto out_unmap;
            }
            uio_info->mem[i].size    = pci_resource_len(pdev, bar_index);
            uio_info->mem[i].memtype = UIO_MEM_PHYS;
            uio_info->mem[i].name    = info->uio_mem_name[i];
        }
        else
        {
            uio_priv->dma_mem_size         = dma_mem_size;
            dma_virt_addr = dma_alloc_coherent(&pdev->dev, dma_mem_size, &dma_phy_addr, GFP_KERNEL);
            if (!dma_virt_addr)
            {
                printk("pci_alloc_consistent failed\n");
                goto out_dma;
            }

            uio_info->mem[i].addr          = (phys_addr_t)dma_phy_addr;
            uio_info->mem[i].size          = uio_priv->dma_mem_size;
            uio_info->mem[i].memtype       = UIO_MEM_PHYS;
            uio_info->mem[i].internal_addr = dma_virt_addr;
            uio_info->mem[i].name           = info->uio_mem_name[i];
        }
    }

    /**wlake only support msi mode */
    if (0 == info->msi)
    {
        if (pci_enable_msi(pdev))
        {
            printk("pci_enable_msi failed\n");
            goto out_disable_msi;
        }
    }
    else
    {
        /**msi-x */
    }

    uio_priv->dma_mem_size = dma_mem_size;
    uio_priv->pdev = pdev;
    uio_priv->id = id;

    uio_info->name       = "gt_uio";
    uio_info->version    = "1.0";
    uio_info->priv       = uio_priv;
    uio_info->irq        = pdev->irq;
    uio_info->irq_flags  = 0;
    uio_info->handler    = dal_handler;
    uio_info->irqcontrol = dal_irq_control;

    pci_set_drvdata(pdev, uio_info);
    printk("pdev:%lx uio_info:%lx\n", pdev, uio_info);
    uio_info_tmp = pci_get_drvdata(pdev);
    printk("pdev:%lx uio_info_tmp:%lx\n", pdev, uio_info_tmp);


    if (uio_register_device(&pdev->dev, uio_info))
    {
        printk("uio_register_device failed\n");
        goto out_dma;
    }

    /* request_irq() inside uio_register_device() leaves the IRQ enabled */
    uio_priv->irq_enabled = 1;

    printk("pcie probe pass\n");

    return 0;

out_dma:
    //pci_free_consistent(pdev, pstPriv->iDmaSize, pstPriv->pvVirtualDmaAddr, pstPriv->stDmaAddr);

out_unmap:
    for (i = 0; i < info->uio_mem_num; i++)
    {
        if (uio_info->mem[i].internal_addr && (0xff != info->uiotobar_map[i]))
        {
            iounmap(uio_info->mem[i].internal_addr);
        }
    }

    pci_release_regions(pdev);

out_disable_msi:
    pci_disable_msi(pdev);
out_disable:
    pci_disable_device(pdev);

out_free:
    if (uio_priv)
    {
        kfree(uio_priv);
    }

    if (uio_info)
    {
        kfree(uio_info);
    }

    printk("pcie probe fail\n");

    return -ENODEV;
}

static void linux_dal_pcie_remove(struct pci_dev *pdev)
{
    struct uio_info *uio_info = NULL;
    uio_priv_data_t *uio_priv = NULL;

    printk("linux_dal_pcie_remove start\n");

    uio_info = pci_get_drvdata(pdev);
    if (NULL == uio_info)
    {
        printk("uio_info is null, pdev:%lx\n", pdev);
        return;
    }
    uio_priv = (uio_priv_data_t *)uio_info->priv;
    if (NULL == uio_priv)
    {
        printk("uio_priv is null\n");
        return;
    }

    uio_unregister_device(uio_info);

    pci_release_regions(pdev);

    pci_disable_msi(pdev);

#ifdef PCIE_EN_MSIX

#endif

    pci_disable_device(pdev);

    //pci_free_consistent(pdev, pPriv->iDmaSize, pPriv->pvVirtualDmaAddr, pPriv->stDmaAddr);

    if (uio_info->mem[0].internal_addr)
    {
        iounmap(uio_info->mem[0].internal_addr);
    }

    if (uio_info->mem[1].internal_addr)
    {
        iounmap(uio_info->mem[1].internal_addr);
    }

    if (uio_info->mem[2].internal_addr)
    {
        iounmap(uio_info->mem[2].internal_addr);
    }


    kfree(uio_priv);
    kfree(uio_info);

    printk("pcie remove pass\n");
}

static int __init linux_dal_init(void)
{
    dma_mem_size = 0x200000;

    if (p_dma_mem_size)
    {
        if ((p_dma_mem_size[strlen(p_dma_mem_size) - 1] & ~0x20) == 'M')
        {
            dma_mem_size = (uint32_t)memparse(p_dma_mem_size, NULL);
            printk("dma_mem_size: 0x%x \n", dma_mem_size);
        }
        else
        {
            printk("DMA memory pool size must be specified as e.g. dma_mem_size=1M\n");
        }

        if (dma_mem_size & (dma_mem_size - 1))
        {
            printk("dma_mem_size must be a power of 2 (1M, 2M, 4M, 8M etc.)\n");
            dma_mem_size = 0;
        }
    }

    //proc_dir = proc_mkdir("gt_uio", NULL);
    //if (!proc_dir)
    {
        //return -1;
    }

    return pci_register_driver(&linux_dal_pci_driver);
}

static void __exit linux_dal_exit(void)
{
    printk("linux_dal_exit\n");
    pci_unregister_driver(&linux_dal_pci_driver);
}


module_init(linux_dal_init);
module_exit(linux_dal_exit);


MODULE_DESCRIPTION ("drv kernel module");
MODULE_AUTHOR ("Grandtrans");
MODULE_LICENSE ("GPL");
#ifdef __cplusplus
}
#endif /* __cplusplus */
