/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:  osal_mdc.c
 * PURPOSE:
 * 1. Provide device operate from AML interface
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */
#include <linux/version.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/wait.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>

#include <nps_error.h>
#include <nps_types.h>
#include <osal_mdc.h>
#include <hal_dev.h>

#if defined(NPS_LINUX_USER_MODE)
#include <linux/miscdevice.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/bitmap.h>
#include <linux/list.h>
#endif

/* #define OSAL_MDC_EN_MSI */
/* #define OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE */
/* #define OSAL_MDC_EN_TEST */

/* NAMING CONSTANT DECLARATIONS
 */
#define OSAL_MDC_PCI_BAR0_OFFSET            (0x0)
#define OSAL_MDC_ERR                        printk

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef struct
{
    UI32_T                      unit;
    struct pci_dev              *ptr_pci_dev;
    UI32_T                      *ptr_mmio_virt_addr;
    int                         irq;
    AML_DEV_ISR_FUNC_T          isr_callback;
    void                        *ptr_isr_data;

} OSAL_MDC_DEV_T;

typedef struct
{
    OSAL_MDC_DEV_T              dev[NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    UI32_T                      dev_num;
    OSAL_MDC_DMA_INFO_T         dma_info;

} OSAL_MDC_CB_T;

#if defined(NPS_LINUX_USER_MODE)

typedef struct
{
    OSAL_MDC_IOCTL_CALLBACK_FUNC_T   callback[OSAL_MDC_IOCTL_TYPE_LAST];

} OSAL_MDC_IOCTL_CB_T;

#if !defined(NPS_EN_DMA_RESERVED)
typedef struct
{
    NPS_ADDR_T                  phy_addr;
    UI32_T                      size;
    struct list_head            list;

} OSAL_MDC_USER_MODE_DMA_NODE_T;
#endif

#endif


#if defined(NPS_LINUX_KERNEL_MODE)

/* re-define the interface to align OSAL_MDC's implementation with the prototype of CMLIB */
#define osal_mdc_list_create(__capa__, __type__, __name__, __list__)    _osal_mdc_list_create(__capa__, __list__)
#define osal_mdc_list_destroy(__list__, __callback__)                   _osal_mdc_list_destroy(__list__)
#define osal_mdc_list_getNodeData(__list__, __node__, __data__)         _osal_mdc_list_getNodeData(__list__, __node__, __data__)
#define osal_mdc_list_next(__list__, __node__, __next__)                _osal_mdc_list_next(__list__, __node__, __next__)
#define osal_mdc_list_locateHead(__list__, __node__)                    _osal_mdc_list_locateHead(__list__, __node__)
#define osal_mdc_list_insertToHead(__list__, __data__)                  _osal_mdc_list_insertToHead(__list__, __data__)
#define osal_mdc_list_deleteByData(__list__, __data__)                  _osal_mdc_list_deleteByData(__list__, __data__)

#if defined(NPS_EN_DMA_RESERVED)
#define osal_mdc_list_insertBefore(__list__, __node__, __data__)        _osal_mdc_list_insertBefore(__list__, __node__, __data__)
#define osal_mdc_list_prev(__list__, __node__, __prev__)                _osal_mdc_list_prev(__list__, __node__, __prev__)
#endif

#define OSAL_MDC_LIST_TYPE_DOUBLE           (1)     /* don't care the type, always be double */
#define OSAL_MDC_LIST_TYPE_SINGLE           (0)     /* don't care the type, always be double */

static NPS_ERROR_NO_T
_osal_mdc_list_create(
    const UI32_T            capacity,
    OSAL_MDC_LIST_T         **pptr_list )
{
    NPS_ERROR_NO_T          rc = NPS_E_NO_MEMORY;

    *pptr_list = NULL;

    *pptr_list = osal_alloc(sizeof(OSAL_MDC_LIST_T));
    if (NULL != *pptr_list)
    {
        (*pptr_list)->capacity = capacity;
        (*pptr_list)->node_cnt = 0;
        (*pptr_list)->ptr_head_node = NULL;
        (*pptr_list)->ptr_tail_node = NULL;
        rc = NPS_E_OK;
    }

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_list_destroy(
    OSAL_MDC_LIST_T         *ptr_list )
{
    OSAL_MDC_LIST_NODE_T    *ptr_cur_node, *ptr_next_node;
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        if (ptr_list->node_cnt != 0)
        {
            OSAL_MDC_ERR("dma list not empty, node num=%d\n",
                         ptr_list->node_cnt);
            ptr_cur_node = ptr_list->ptr_head_node;
            while(NULL != ptr_cur_node)
            {
                ptr_next_node = ptr_cur_node->ptr_next;
                osal_free(ptr_cur_node);
                ptr_list->node_cnt--;
                ptr_cur_node = ptr_next_node;
            }
        }

        osal_free(ptr_list);
        rc = NPS_E_OK;
    }

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_list_getNodeData(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    *ptr_node,
    void                    **pptr_node_data )
{
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        if (NULL != ptr_node)
        {
            *pptr_node_data = ptr_node->ptr_data;
            rc = NPS_E_OK;
        }
    }

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_list_insertToHead(
    OSAL_MDC_LIST_T         *ptr_list,
    void                    *ptr_data )
{
    OSAL_MDC_LIST_NODE_T    *ptr_new_node = NULL;
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        ptr_new_node = osal_alloc(sizeof(OSAL_MDC_LIST_NODE_T));
        if (NULL != ptr_new_node)
        {
            ptr_new_node->ptr_data = ptr_data;

            /* no former node */
            ptr_new_node->ptr_prev = NULL;

            if (NULL != ptr_list->ptr_head_node)
            {
                ptr_list->ptr_head_node->ptr_prev = ptr_new_node;
                ptr_new_node->ptr_next = ptr_list->ptr_head_node;
            }
            else
            {
                /* 1st node insertion */
                ptr_list->ptr_tail_node = ptr_new_node;
                ptr_new_node->ptr_next = NULL;
            }

            ptr_list->ptr_head_node = ptr_new_node;
            ptr_list->node_cnt++;
            rc = NPS_E_OK;
        }
    }

    return (rc);
}

#if defined(NPS_EN_DMA_RESERVED)
static NPS_ERROR_NO_T
_osal_mdc_list_insertBefore(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    *ptr_node,
    void                    *ptr_data )
{
    OSAL_MDC_LIST_NODE_T    *ptr_new_node = NULL;
    OSAL_MDC_LIST_NODE_T    *ptr_prev_node = ptr_node->ptr_prev;
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        if (NULL != ptr_node)
        {
            ptr_new_node = osal_alloc(sizeof(OSAL_MDC_LIST_NODE_T));
            if (NULL != ptr_new_node)
            {
                ptr_new_node->ptr_data = ptr_data;

                /* location */
                if (NULL != ptr_prev_node)
                {
                    ptr_prev_node->ptr_next = ptr_new_node;
                }
                ptr_new_node->ptr_prev = ptr_prev_node;
                ptr_new_node->ptr_next = ptr_node;
                ptr_node->ptr_prev = ptr_new_node;

                /* update head if necessary  */
                if (ptr_list->ptr_head_node == ptr_node)
                {
                    ptr_list->ptr_head_node = ptr_new_node;
                }

                ptr_list->node_cnt++;
                rc = NPS_E_OK;
            }
        }
    }

    return (rc);
}
#endif

static NPS_ERROR_NO_T
_osal_mdc_list_deleteTargetNode(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    *ptr_target_node)
{
    OSAL_MDC_LIST_NODE_T    *ptr_prev_node = ptr_target_node->ptr_prev;
    OSAL_MDC_LIST_NODE_T    *ptr_next_node = ptr_target_node->ptr_next;

    if (ptr_target_node == ptr_list->ptr_head_node)
    {
        ptr_list->ptr_head_node = ptr_next_node;
        if (NULL != ptr_next_node)
        {
            ptr_next_node->ptr_prev = NULL;
        }
        else
        {
            /* there's only 1 node in the list, and it gets removed */
            ptr_list->ptr_tail_node = NULL;
        }
    }
    else if (ptr_target_node == ptr_list->ptr_tail_node)
    {
        /* at least 2 nodes in the list, and the target node locates tail */
        ptr_list->ptr_tail_node = ptr_prev_node;
        ptr_prev_node->ptr_next = NULL;
    }
    else
    {
        /* intermediate node */
        ptr_prev_node->ptr_next = ptr_next_node;
        ptr_next_node->ptr_prev = ptr_prev_node;
    }

    osal_free(ptr_target_node);
    ptr_list->node_cnt--;

    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_list_deleteByData(
    OSAL_MDC_LIST_T         *ptr_list,
    void                    *ptr_delete_data)
{
    OSAL_MDC_LIST_NODE_T    *ptr_tmp_node;
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        ptr_tmp_node = ptr_list->ptr_head_node;
        while (NULL != ptr_tmp_node)
        {
            if (ptr_tmp_node->ptr_data == ptr_delete_data)
            {
                rc = _osal_mdc_list_deleteTargetNode(ptr_list, ptr_tmp_node);
                break;
            }
            else
            {
                ptr_tmp_node = ptr_tmp_node->ptr_next;
            }
        }
    }

    return (rc);
}


static NPS_ERROR_NO_T
_osal_mdc_list_locateHead(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    **pptr_node )
{
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        *pptr_node = ptr_list->ptr_head_node;
        if (NULL != *pptr_node)
        {
            rc = NPS_E_OK;
        }
    }

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_list_next(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    *ptr_node,
    OSAL_MDC_LIST_NODE_T    **pptr_next_node )
{
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        if (NULL != ptr_node)
        {
            *pptr_next_node = ptr_node->ptr_next;
            if (NULL != *pptr_next_node)
            {
                rc = NPS_E_OK;
            }
        }
    }

    return (rc);
}

#if defined(NPS_EN_DMA_RESERVED)
static NPS_ERROR_NO_T
_osal_mdc_list_prev(
    OSAL_MDC_LIST_T         *ptr_list,
    OSAL_MDC_LIST_NODE_T    *ptr_node,
    OSAL_MDC_LIST_NODE_T    **pptr_prev_node )
{
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (NULL != ptr_list)
    {
        if (NULL != ptr_node)
        {
            *pptr_prev_node = ptr_node->ptr_prev;
            if (NULL != *pptr_prev_node)
            {
                rc = NPS_E_OK;
            }
        }
    }

    return (rc);
}
#endif /* End of defined(NPS_EN_DMA_RESERVED) */

#endif /* End if defined(NPS_LINUX_KERNEL_MODE) */

/* GLOBAL VARIABLE DECLARATIONS
 */
static OSAL_MDC_CB_T            _osal_mdc_cb;

/* To let system callback function to access AML database */
static AML_DEV_T                *_ptr_osal_mdc_dev;

/* Interface */
struct pci_dev                  *_ptr_ext_pci_dev;


/* STATIC VARIABLE DECLARATIONS
 */
/* --------------------------------------------------------------------------- I2C interface */
#if defined(AML_EN_I2C)
extern NPS_ERROR_NO_T
dev_switch_readBuffer(
    const UI32_T        addr,
    const UI32_T        addr_len,
    UI8_T               *ptr_buf,
    const UI32_T        buf_len);

extern NPS_ERROR_NO_T
dev_switch_writeBuffer(
    const UI32_T        addr,
    const UI32_T        addr_len,
    const UI8_T         *ptr_buf,
    const UI32_T        buf_len);

static NPS_ERROR_NO_T
_osal_mdc_readI2cReg(
    const UI32_T        unit,
    const UI32_T        offset,
    UI32_T              *ptr_data,
    const UI32_T        len)
{
    return dev_switch_readBuffer(offset, sizeof(offset), (UI8_T *)ptr_data, len);
}

static NPS_ERROR_NO_T
_osal_mdc_writeI2cReg(
    const UI32_T        unit,
    const UI32_T        offset,
    const UI32_T        *ptr_data,
    const UI32_T        len)
{
    return dev_switch_writeBuffer(offset, sizeof(offset), (UI8_T *)ptr_data, len);
}

static NPS_ERROR_NO_T
_osal_mdc_probeI2cDevice(void)
{
    /* I2C interface will be probed in BSP. */
    _ptr_osal_mdc_dev->if_type     = AML_DEV_TYPE_I2C;
    _ptr_osal_mdc_dev->access.read_callback  = _osal_mdc_readI2cReg;
    _ptr_osal_mdc_dev->access.write_callback = _osal_mdc_writeI2cReg;

    _ptr_osal_mdc_dev->id.device   = HAL_DEVICE_ID_MT3258;
    _ptr_osal_mdc_dev->id.vendor   = HAL_MTK_VENDOR_ID;
    _ptr_osal_mdc_dev->id.revision = HAL_REVISION_ID_E2;

    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_removeI2cDevice(void)
{
    /* I2C interface will be removed in BSP. */
    return (NPS_E_OK);
}

/* --------------------------------------------------------------------------- PCI interface */
#else

static NPS_ERROR_NO_T
_osal_mdc_getPciMmioInfo(
    struct pci_dev      *pdev,
    UI32_T              **pptr_base_addr)
{
    NPS_ERROR_NO_T      rc = NPS_E_OTHERS;
    NPS_ADDR_T          phy_addr;
    UI32_T              reg_space_sz;

    phy_addr     = pci_resource_start(pdev, OSAL_MDC_PCI_BAR0_OFFSET);
    reg_space_sz = pci_resource_len(pdev, OSAL_MDC_PCI_BAR0_OFFSET);

    if (0 == pci_request_region(pdev, OSAL_MDC_PCI_BAR0_OFFSET, OSAL_MDC_DRIVER_NAME))
    {
        *pptr_base_addr = ioremap_nocache(phy_addr, reg_space_sz);
        if (NULL != *pptr_base_addr)
        {
            rc = NPS_E_OK;
        }
    }
    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_readPciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    UI32_T              *ptr_data,
    const UI32_T        len)
{
    NPS_ERROR_NO_T      rc = NPS_E_OK;
    UI32_T              idx;
    UI32_T              count;
    volatile UI32_T     *ptr_base_addr = _osal_mdc_cb.dev[unit].ptr_mmio_virt_addr;

    if (NULL != ptr_base_addr)
    {
        if (OSAL_MDC_PCI_BUS_WIDTH == len)
        {
            *ptr_data = *((UI32_T *)((NPS_HUGE_T)ptr_base_addr + offset));
        }
        else
        {
            if (0 == (len % OSAL_MDC_PCI_BUS_WIDTH))
            {
                count = len / OSAL_MDC_PCI_BUS_WIDTH;
                for (idx = 0; idx < count; idx++)
                {
                    *(ptr_data + idx) = *((UI32_T *)((NPS_HUGE_T)ptr_base_addr + offset + idx * 4));
                }
            }
            else
            {
                rc = NPS_E_OTHERS;
            }
        }
    }
    else
    {
        rc = NPS_E_NOT_INITED;
    }

    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_writePciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    const UI32_T        *ptr_data,
    const UI32_T        len)
{
    UI32_T              idx;
    UI32_T              count;
    volatile UI32_T     *ptr_base_addr = _osal_mdc_cb.dev[unit].ptr_mmio_virt_addr;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

    if (NULL != ptr_base_addr)
    {
        if (OSAL_MDC_PCI_BUS_WIDTH == len)
        {
            *((UI32_T *)((NPS_HUGE_T)ptr_base_addr + offset)) = *ptr_data;
        }
        else
        {
            if (0 == (len % OSAL_MDC_PCI_BUS_WIDTH))
            {
                count = len / OSAL_MDC_PCI_BUS_WIDTH;
                for (idx = 0; idx < count; idx++)
                {
                    *((UI32_T *)((NPS_HUGE_T)ptr_base_addr + offset + idx * 4)) = *(ptr_data + idx);
                }
            }
            else
            {
                rc = NPS_E_OTHERS;
            }
        }
    }
    else
    {
        rc = NPS_E_NOT_INITED;
    }

    return (rc);
}

static int
_osal_mdc_probePciCallback(
    struct pci_dev                  *pdev,
    const struct pci_device_id      *id)
{
    int                 linux_rc;
    UI16_T              device_id;
    UI16_T              vendor_id;
    UI8_T               revision_id;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

    linux_rc = pci_enable_device(pdev);
    if (0 == linux_rc)
    {
        _ptr_osal_mdc_dev->if_type = AML_DEV_TYPE_PCI;

        pci_read_config_word(pdev, PCI_DEVICE_ID, &device_id);
        pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor_id);
        pci_read_config_byte(pdev, PCI_REVISION_ID, &revision_id);

        _ptr_osal_mdc_dev->id.device   = (UI32_T)device_id;
        _ptr_osal_mdc_dev->id.vendor   = (UI32_T)vendor_id;
        _ptr_osal_mdc_dev->id.revision = (UI32_T)revision_id;

#if defined(NPS_LINUX_KERNEL_MODE)
        _ptr_osal_mdc_dev->access.read_callback  = osal_mdc_readPciReg;
        _ptr_osal_mdc_dev->access.write_callback = osal_mdc_writePciReg;
#endif

        rc = _osal_mdc_getPciMmioInfo(pdev, &_osal_mdc_cb.dev[_osal_mdc_cb.dev_num].ptr_mmio_virt_addr);
        if (NPS_E_OK == rc)
        {
            /* Save the database to pdev structure for system callback to release recource,
             * such like disconnecting ISR etc.
             */
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].irq          = pdev->irq;
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].ptr_pci_dev  = pdev;
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].unit         = _osal_mdc_cb.dev_num;

            pci_set_drvdata(pdev, &_osal_mdc_cb.dev[_osal_mdc_cb.dev_num]);

            /* To set the bus master bit on device to enable the DMA transaction from PCIe EP to RC
             * The bus master bit gets cleared when pci_disable_device() is called
             */
            pci_set_master(pdev);

#if !defined(NPS_EN_DMA_RESERVED)
            if (NULL == _osal_mdc_cb.dma_info.ptr_dma_dev)
            {
                /* This variable is for dma_alloc_coherent */
                _osal_mdc_cb.dma_info.ptr_dma_dev = &pdev->dev;
            }
#endif
            _osal_mdc_cb.dev_num++;
            _ptr_osal_mdc_dev++;
        }
    }
    else
    {
        OSAL_MDC_ERR("enable pci dev failed, linux_rc=%d\n", linux_rc);
    }

    return (0);
}

static void
_osal_mdc_removePciCallback(
    struct pci_dev      *pdev)
{
    OSAL_MDC_DEV_T      *ptr_dev = (OSAL_MDC_DEV_T *)pci_get_drvdata(pdev);

    iounmap(ptr_dev->ptr_mmio_virt_addr);
    pci_release_region(pdev, OSAL_MDC_PCI_BAR0_OFFSET);
    pci_disable_device(pdev);
    _osal_mdc_cb.dev_num--;
}

static struct pci_device_id _osal_mdc_id_table[] =
{
    {PCI_DEVICE(HAL_MTK_VENDOR_ID, PCI_ANY_ID)},
    {PCI_DEVICE(HAL_NP_VENDOR_ID, PCI_ANY_ID)},
};

static struct pci_driver    _osal_mdc_pci_driver =
{
    .name     = OSAL_MDC_DRIVER_NAME,
    .id_table = _osal_mdc_id_table,
    .probe    = _osal_mdc_probePciCallback,
    .remove   = _osal_mdc_removePciCallback,
};

static NPS_ERROR_NO_T
_osal_mdc_probePciDevice(void)
{
    NPS_ERROR_NO_T  rc = NPS_E_OK;

    if (pci_register_driver(&_osal_mdc_pci_driver) < 0)
    {
        OSAL_MDC_ERR("Cannot find PCI device\n");
        rc = NPS_E_OTHERS;
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_removePciDevice(void)
{
    pci_unregister_driver(&_osal_mdc_pci_driver);
    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_maskStatus(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_ep_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    struct pci_dev      *ptr_rc_dev = ptr_ep_dev->bus->self;
    int                 ext_cap = 0;
    UI32_T              data_32 = 0;

    ext_cap = pci_find_ext_capability(ptr_rc_dev, 0x1);
    if (0 != ext_cap)
    {
        /* Mask */
        pci_read_config_dword(ptr_rc_dev, ext_cap + 0x8, &data_32);
        data_32 |= 0x20;
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x8, data_32);
    }

    return NPS_E_OK;
}

static NPS_ERROR_NO_T
_osal_mdc_clearStatus(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_ep_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    struct pci_dev      *ptr_rc_dev = ptr_ep_dev->bus->self;
    int                 ext_cap = 0;
    UI32_T              data_32 = 0;

    ext_cap = pci_find_ext_capability(ptr_rc_dev, 0x1);
    if (0 != ext_cap)
    {
        /* Clear */
        pci_write_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0xa, 0x04);
        pci_write_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0x12, 0x8000);
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x4, 0x20);

        /* UnMask */
        pci_read_config_dword(ptr_rc_dev, ext_cap + 0x8, &data_32);
        data_32 &= ~0x20;
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x8, data_32);
    }

    return NPS_E_OK;
}

static NPS_ERROR_NO_T
_osal_mdc_savePciConfig(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

    rc = _osal_mdc_maskStatus(unit);

    if (NPS_E_OK == rc)
    {
        pci_save_state(ptr_dev);
    }

    return rc;
}

static NPS_ERROR_NO_T
_osal_mdc_restorePciConfig(
    const UI32_T        unit)
{
#define OSAL_MDC_PCI_PRESENT_POLL_CNT           (100)
#define OSAL_MDC_PCI_PRESENT_POLL_INTERVAL      (10)   /* ms */

    struct pci_dev      *ptr_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    UI32_T              poll_cnt = 0;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

    /* standard: at least 100ms for link recovery */
    msleep(100);

    /* make sure pci device is there before restoring the config space */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
    while ((0 == pci_device_is_present(ptr_dev)) &&
#else
    while ((0 == pci_dev_present(_osal_mdc_id_table)) &&
#endif
           (poll_cnt < OSAL_MDC_PCI_PRESENT_POLL_CNT))
    {
        msleep(OSAL_MDC_PCI_PRESENT_POLL_INTERVAL);
        poll_cnt++;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
    if (1 == pci_device_is_present(ptr_dev))
#else
    if (1 == pci_dev_present(_osal_mdc_id_table))
#endif
    {
        pci_restore_state(ptr_dev);
        rc = NPS_E_OK;
    }
    else
    {
        OSAL_MDC_ERR("detect pci device failed\n");
        rc = NPS_E_OTHERS;
    }

    if (NPS_E_OK == rc)
    {
        rc = _osal_mdc_clearStatus(unit);
    }

    return (rc);
}

#endif /* End of AML_EN_I2C */

/* --------------------------------------------------------------------------- DMA */
#if defined(NPS_LINUX_KERNEL_MODE)

static NPS_ERROR_NO_T
_osal_mdc_searchDmaVirtAddr(
    OSAL_MDC_LIST_T         *ptr_dma_list,
    const void              *ptr_virt_addr,
    OSAL_MDC_LIST_NODE_T    **pptr_node,
    OSAL_MDC_DMA_NODE_T     **pptr_node_data)
{
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_locateHead(ptr_dma_list, &ptr_curr_node);
    while (NPS_E_OK == rc)
    {
        rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
        if (NPS_E_OK == rc)
        {
            if (ptr_curr_node_data->ptr_virt_addr == ptr_virt_addr)
            {
                *pptr_node      = ptr_curr_node;
                *pptr_node_data = ptr_curr_node_data;
                break;
            }
            rc = osal_mdc_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
        }
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_destroyDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    OSAL_MDC_LIST_T         *ptr_dma_list = (OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list;
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data = NULL;
    NPS_ERROR_NO_T          rc = NPS_E_NOT_INITED;

    if (NULL != ptr_dma_list)
    {
        rc = osal_mdc_list_locateHead(ptr_dma_list, &ptr_curr_node);
        while (NPS_E_OK == rc)
        {
            rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
            if ((NPS_E_OK == rc) && (NULL != ptr_curr_node_data))
            {
                rc = osal_mdc_list_deleteByData(ptr_dma_list, ptr_curr_node_data);
                if (NPS_E_OK == rc)
                {
                    kfree(ptr_curr_node_data);
                }
            }
            rc = osal_mdc_list_locateHead(ptr_dma_list, &ptr_curr_node);
        }
        rc = osal_mdc_list_destroy(ptr_dma_list, NULL);
        if (NPS_E_OK == rc)
        {
            ptr_dma_info->ptr_dma_list = NULL;
        }
    }
    return (rc);
}

#endif  /* End of NPS_LINUX_KERNEL_MODE */

#if defined(NPS_EN_DMA_RESERVED)

#if defined(NPS_LINUX_KERNEL_MODE)

#if defined(OSAL_MDC_EN_TEST)
static NPS_ERROR_NO_T
_osal_mdc_dumpRsrvDmaList(void)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    UI32_T                  node = 0;
    NPS_ERROR_NO_T          rc = NPS_E_OK;

    rc = osal_mdc_list_locateHead((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                               &ptr_curr_node);
    while (NPS_E_OK == rc)
    {
        rc = osal_mdc_list_getNodeData((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                                    ptr_curr_node, (void **)&ptr_curr_node_data);
        if (NPS_E_OK == rc)
        {
            OSAL_MDC_ERR(
                "node %d. virt addr=%p, phy addr=%p, size=%d, avbl=%d\n", node,
                ptr_curr_node_data->ptr_virt_addr, ptr_curr_node_data->phy_addr,
                ptr_curr_node_data->size, ptr_curr_node_data->available);
        }
        rc = osal_mdc_list_next((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                             ptr_curr_node, &ptr_curr_node);
        node++;
    }
    return (rc);
}
#endif

static NPS_ERROR_NO_T
_osal_mdc_createRsrvDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    OSAL_MDC_DMA_NODE_T     *ptr_node_data;
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_create(OSAL_MDC_DMA_LIST_SZ_UNLIMITED,
                              OSAL_MDC_LIST_TYPE_DOUBLE,
                              OSAL_MDC_DMA_LIST_NAME,
                              (OSAL_MDC_LIST_T **)&ptr_dma_info->ptr_dma_list);
    if (NPS_E_OK == rc)
    {
        /* The first node, which contains all of the reserved memory */
        ptr_node_data = kmalloc(sizeof(OSAL_MDC_DMA_NODE_T), GFP_KERNEL);
        if (NULL != ptr_node_data)
        {
            ptr_node_data->ptr_virt_addr = ptr_dma_info->ptr_rsrv_virt_addr;
            ptr_node_data->phy_addr      = ptr_dma_info->rsrv_phy_addr;
            ptr_node_data->size          = ptr_dma_info->rsrv_size;
            ptr_node_data->available     = TRUE;
            rc = osal_mdc_list_insertToHead((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                                            ptr_node_data);
        }
        else
        {
            rc = NPS_E_NO_MEMORY;
        }
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_searchAvblRsrvDmaNode(
    OSAL_MDC_LIST_T         *ptr_dma_list,
    const UI32_T            size,
    OSAL_MDC_LIST_NODE_T    **pptr_avbl_node)
{
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_locateHead(ptr_dma_list, &ptr_curr_node);
    while (NPS_E_OK == rc)
    {
        rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
        if (NPS_E_OK == rc)
        {
            if ((TRUE == ptr_curr_node_data->available) && (ptr_curr_node_data->size >= size))
            {
                *pptr_avbl_node = ptr_curr_node;
                break;
            }
        }
        rc = osal_mdc_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_splitRsrvDmaNodes(
    OSAL_MDC_LIST_T         *ptr_dma_list,
    OSAL_MDC_LIST_NODE_T    *ptr_ori_node,
    const UI32_T            size,
    OSAL_MDC_DMA_NODE_T     **pptr_new_node_data)
{
    OSAL_MDC_DMA_NODE_T     *ptr_ori_node_data;
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_ori_node, (void **)&ptr_ori_node_data);

    if (NPS_E_OK == rc)
    {
        *pptr_new_node_data = kmalloc(sizeof(OSAL_MDC_DMA_NODE_T), GFP_KERNEL);

        /* Create a new node */
        (*pptr_new_node_data)->size          = size;
        (*pptr_new_node_data)->phy_addr      = ptr_ori_node_data->phy_addr;
        (*pptr_new_node_data)->ptr_virt_addr = ptr_ori_node_data->ptr_virt_addr;
        (*pptr_new_node_data)->available     = TRUE;

        /* Update the original node */
        ptr_ori_node_data->size              -= size;
        ptr_ori_node_data->phy_addr          += size;
        ptr_ori_node_data->ptr_virt_addr =
            (void *)((NPS_HUGE_T)ptr_ori_node_data->ptr_virt_addr + (NPS_HUGE_T)size);

        rc = osal_mdc_list_insertBefore(ptr_dma_list, ptr_ori_node, (void *)*pptr_new_node_data);
        if (NPS_E_OK != rc)
        {
            OSAL_MDC_ERR("insert rsrv dma node to list failed, size=%d, rc=%d\n", size, rc);
            /* Recovery */
            ptr_ori_node_data->size          += size;
            ptr_ori_node_data->phy_addr      -= size;
            ptr_ori_node_data->ptr_virt_addr =
                (void *)((NPS_HUGE_T)ptr_ori_node_data->ptr_virt_addr - (NPS_HUGE_T)size);
            kfree(*pptr_new_node_data);
        }
    }
    return (rc);
}

static void *
_osal_mdc_allocRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    const UI32_T            size)
{
    OSAL_MDC_LIST_T         *ptr_dma_list = (OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list;
    OSAL_MDC_LIST_NODE_T    *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_new_node_data;
    void                    *ptr_virt_addr = NULL;
    NPS_ERROR_NO_T          rc;

    rc = _osal_mdc_searchAvblRsrvDmaNode(ptr_dma_list, size, &ptr_node);
    if (NPS_E_OK == rc)
    {
        rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_node, (void **)&ptr_node_data);
        if (NPS_E_OK == rc)
        {
            /* If the node size just fit the user's requirement, just give it to user */
            if (ptr_node_data->size == size)
            {
                ptr_node_data->available = FALSE;
                ptr_virt_addr = ptr_node_data->ptr_virt_addr;
            }
            /* or split a new node with user required size. */
            else
            {
                rc = _osal_mdc_splitRsrvDmaNodes(ptr_dma_list, ptr_node, size, &ptr_new_node_data);
                if (NPS_E_OK == rc)
                {
                    ptr_new_node_data->available = FALSE;
                    ptr_virt_addr = ptr_new_node_data->ptr_virt_addr;
                }
            }
        }
    }
    return (ptr_virt_addr);
}

static NPS_ERROR_NO_T
_osal_mdc_mergeTwoRsrvDmaNodes(
    OSAL_MDC_LIST_T         *ptr_dma_list,
    OSAL_MDC_DMA_NODE_T     *ptr_first_node_data,
    OSAL_MDC_DMA_NODE_T     *ptr_second_node_data)
{
    ptr_first_node_data->size += ptr_second_node_data->size;

    osal_mdc_list_deleteByData(ptr_dma_list, ptr_second_node_data);
    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_mergeRsrvDmaNodes(
    OSAL_MDC_LIST_T         *ptr_dma_list,
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node)
{
    OSAL_MDC_LIST_NODE_T    *ptr_prev_node;
    OSAL_MDC_LIST_NODE_T    *ptr_next_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_prev_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_next_node_data;
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
    if (NPS_E_OK == rc)
    {
        /* First, check if the previous node is available */
        rc = osal_mdc_list_prev(ptr_dma_list, ptr_curr_node, &ptr_prev_node);
        if (NPS_E_OK == rc)
        {
            osal_mdc_list_getNodeData(ptr_dma_list, ptr_prev_node, (void **)&ptr_prev_node_data);
            if (TRUE == ptr_prev_node_data->available)
            {
                _osal_mdc_mergeTwoRsrvDmaNodes(ptr_dma_list, ptr_prev_node_data, ptr_curr_node_data);
                ptr_curr_node      = ptr_prev_node;
                ptr_curr_node_data = ptr_prev_node_data;
            }
        }

        /* then, check if the next node is available */
        rc = osal_mdc_list_next(ptr_dma_list, ptr_curr_node, &ptr_next_node);
        if (NPS_E_OK == rc)
        {
            rc = osal_mdc_list_getNodeData(ptr_dma_list, ptr_next_node, (void **)&ptr_next_node_data);
            if (NPS_E_OK == rc)
            {
                if (TRUE == ptr_next_node_data->available)
                {
                    _osal_mdc_mergeTwoRsrvDmaNodes(ptr_dma_list, ptr_curr_node_data, ptr_next_node_data);
                }
            }
        }
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_freeRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    void                    *ptr_virt_addr)
{
    OSAL_MDC_LIST_T         *ptr_dma_list = (OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list;
    OSAL_MDC_LIST_NODE_T    *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data = NULL;
    NPS_ERROR_NO_T          rc;

    rc = _osal_mdc_searchDmaVirtAddr(ptr_dma_list, ptr_virt_addr, &ptr_node, &ptr_node_data);
    if (NPS_E_OK == rc)
    {
        ptr_node_data->available = TRUE;
        _osal_mdc_mergeRsrvDmaNodes(ptr_dma_list, ptr_node);
    }
    return (rc);
}

#endif /* End of NPS_LINUX_KERNEL_MODE */

static NPS_ERROR_NO_T
_osal_mdc_initRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    struct resource         *ptr_res;
    NPS_ERROR_NO_T          rc = NPS_E_OK;

    ptr_dma_info->rsrv_size     = (NPS_ADDR_T)NPS_DMA_RESERVED_SZ * 1024 * 1024;
    ptr_dma_info->rsrv_phy_addr = (NPS_ADDR_T)NPS_OS_MEMORY_SZ * 1024 * 1024;
    ptr_res = request_mem_region(ptr_dma_info->rsrv_phy_addr,
                                 ptr_dma_info->rsrv_size, "nps_rsrv_mem");
    if (NULL != ptr_res)
    {
#if defined(OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE)
        ptr_dma_info->ptr_rsrv_virt_addr = ioremap(ptr_dma_info->rsrv_phy_addr,
                                           ptr_dma_info->rsrv_size);
#else
        ptr_dma_info->ptr_rsrv_virt_addr = ioremap_nocache(ptr_dma_info->rsrv_phy_addr,
                                           ptr_dma_info->rsrv_size);
#endif
        if (NULL == ptr_dma_info->ptr_rsrv_virt_addr)
        {
#if defined(NPS_EN_64BIT_ADDR) || defined(NPS_EN_HOST_64_BIT_BIG_ENDIAN) || defined(NPS_EN_HOST_64_BIT_LITTLE_ENDIAN)
            OSAL_MDC_ERR(
                "ioremap_nocache() failed, phy addr=0x%llx, size=%llu\n",
                ptr_dma_info->rsrv_phy_addr, ptr_dma_info->rsrv_size);
#else
            OSAL_MDC_ERR(
                "ioremap_nocache() failed, phy addr=0x%x, size=%u\n",
                ptr_dma_info->rsrv_phy_addr, ptr_dma_info->rsrv_size);
#endif
            rc = NPS_E_OTHERS;
        }
    }
    else
    {
#if defined(NPS_EN_64BIT_ADDR) || defined(NPS_EN_HOST_64_BIT_BIG_ENDIAN) || defined(NPS_EN_HOST_64_BIT_LITTLE_ENDIAN)
        OSAL_MDC_ERR(
            "request_mem_region() failed, phy addr=0x%llx, size=%llu\n",
            ptr_dma_info->rsrv_phy_addr, ptr_dma_info->rsrv_size);
#else
        OSAL_MDC_ERR(
            "request_mem_region() failed, phy addr=0x%x, size=%u\n",
            ptr_dma_info->rsrv_phy_addr, ptr_dma_info->rsrv_size);
#endif
        rc = NPS_E_OTHERS;
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_deinitRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    if (NULL != ptr_dma_info->ptr_rsrv_virt_addr)
    {
        iounmap(ptr_dma_info->ptr_rsrv_virt_addr);
    }
    if (0x0 != ptr_dma_info->rsrv_phy_addr)
    {
        release_mem_region(ptr_dma_info->rsrv_phy_addr,
                           ptr_dma_info->rsrv_size);
    }
    return (NPS_E_OK);
}


#else /* Else of NPS_EN_DMA_RESERVED */


#if defined(NPS_LINUX_KERNEL_MODE)

#if defined(OSAL_MDC_EN_TEST)
static NPS_ERROR_NO_T
_osal_mdc_dumpSysDmaList(void)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_LIST_NODE_T    *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    UI32_T                  node = 0;
    NPS_ERROR_NO_T          rc = NPS_E_OK;

    rc = osal_mdc_list_locateHead((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                               &ptr_curr_node);
    while (NPS_E_OK == rc)
    {
        rc = osal_mdc_list_getNodeData((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                                    ptr_curr_node, (void **)&ptr_curr_node_data);
        if (NPS_E_OK == rc)
        {
            OSAL_MDC_ERR(
                "node %d. virt addr=%p, phy addr=%p, size=%d\n", node,
                ptr_curr_node_data->ptr_virt_addr, ptr_curr_node_data->phy_addr,
                ptr_curr_node_data->size);
        }

        rc = osal_mdc_list_next((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                             ptr_curr_node, &ptr_curr_node);
        node++;

    }
    return (rc);
}
#endif

static NPS_ERROR_NO_T
_osal_mdc_createSysDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    NPS_ERROR_NO_T          rc;

    rc = osal_mdc_list_create(OSAL_MDC_DMA_LIST_SZ_UNLIMITED,
                           OSAL_MDC_LIST_TYPE_SINGLY,
                           OSAL_MDC_DMA_LIST_NAME,
                           (OSAL_MDC_LIST_T **)&ptr_dma_info->ptr_dma_list);
    return (rc);
}
#if !defined(NPS_LAMP)

static void *
_osal_mdc_allocSysDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    const UI32_T            size)
{
    dma_addr_t              phy_addr;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data;
    void                    *ptr_virt_addr = NULL;
    NPS_ERROR_NO_T          rc = NPS_E_OK;

    ptr_virt_addr = dma_alloc_coherent(ptr_dma_info->ptr_dma_dev, size, &phy_addr, GFP_ATOMIC);
    if (NULL != ptr_virt_addr)
    {
        ptr_node_data = kmalloc(sizeof(OSAL_MDC_DMA_NODE_T), GFP_KERNEL);
        ptr_node_data->phy_addr      = (NPS_ADDR_T)phy_addr;
        ptr_node_data->ptr_virt_addr = ptr_virt_addr;
        ptr_node_data->size          = size;

        rc = osal_mdc_list_insertToHead((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list, ptr_node_data);
        if (NPS_E_OK != rc)
        {
            kfree(ptr_node_data);
            dma_free_coherent(ptr_dma_info->ptr_dma_dev, size,
                              ptr_virt_addr, phy_addr);
            ptr_virt_addr = NULL;
        }
    }
    return (ptr_virt_addr);
}

static NPS_ERROR_NO_T
_osal_mdc_freeSysDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    void                    *ptr_virt_addr)
{
    OSAL_MDC_LIST_NODE_T    *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data = NULL;
    NPS_ERROR_NO_T          rc;

    rc = _osal_mdc_searchDmaVirtAddr((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                                     ptr_virt_addr, &ptr_node, &ptr_node_data);
    if (NPS_E_OK == rc)
    {
        dma_free_coherent(ptr_dma_info->ptr_dma_dev, ptr_node_data->size,
                          ptr_virt_addr, ptr_node_data->phy_addr);

        osal_mdc_list_deleteByData((OSAL_MDC_LIST_T *)ptr_dma_info->ptr_dma_list,
                                   ptr_node_data);
        kfree(ptr_node_data);
    }
    return (rc);
}
#endif
#endif /* End of NPS_LINUX_KERNEL_MODE */

#endif /* End of NPS_EN_DMA_RESERVED */

#if defined(NPS_LINUX_KERNEL_MODE)

void *
osal_mdc_allocDmaMem(
    const UI32_T            size)
{
    void                    *ptr_virt_addr = NULL;

#if defined(NPS_LAMP)
    ptr_virt_addr =  osal_alloc(size);
#else
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    osal_takeSemaphore(&ptr_dma_info->sema, NPS_SEMAPHORE_WAIT_FOREVER);

#if defined(NPS_EN_DMA_RESERVED)
    ptr_virt_addr = _osal_mdc_allocRsrvDmaMem(ptr_dma_info, size);
#else
    ptr_virt_addr = _osal_mdc_allocSysDmaMem(ptr_dma_info, size);
#endif

    osal_giveSemaphore(&ptr_dma_info->sema);
#endif

    return ptr_virt_addr;
}

NPS_ERROR_NO_T
osal_mdc_freeDmaMem(
    void                    *ptr_virt_addr)
{
    NPS_ERROR_NO_T          rc = NPS_E_OK;

#if defined(NPS_LAMP)
    osal_free(ptr_virt_addr);
#else
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;

    osal_takeSemaphore(&ptr_dma_info->sema, NPS_SEMAPHORE_WAIT_FOREVER);

#if defined(NPS_EN_DMA_RESERVED)
    rc = _osal_mdc_freeRsrvDmaMem(ptr_dma_info, ptr_virt_addr);
#else

    rc = _osal_mdc_freeSysDmaMem(ptr_dma_info, ptr_virt_addr);
#endif
    osal_giveSemaphore(&ptr_dma_info->sema);

    if (NPS_E_OK != rc)
    {
        OSAL_MDC_ERR("free dma mem failed, virt addr=%p\n", ptr_virt_addr);
    }
#endif

    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_convertPhyToVirt(
    const NPS_ADDR_T        phy_addr,
    void                    **pptr_virt_addr)
{
    NPS_ERROR_NO_T          rc = NPS_E_OK;

#if defined(NPS_EN_DMA_RESERVED)
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info  = &_osal_mdc_cb.dma_info;
    NPS_HUGE_T              rsrv_virt_base = (NPS_HUGE_T)ptr_dma_info->ptr_rsrv_virt_addr;
    NPS_ADDR_T              rsrv_phy_base  = ptr_dma_info->rsrv_phy_addr;
#endif

#if defined(NPS_EN_DMA_RESERVED)
    *pptr_virt_addr = (void *)(rsrv_virt_base + (NPS_HUGE_T)(phy_addr - rsrv_phy_base));
#else
    *pptr_virt_addr = NULL;
    *pptr_virt_addr = phys_to_virt(phy_addr);
    rc = (NULL == *pptr_virt_addr) ? (NPS_E_ENTRY_NOT_FOUND) : (NPS_E_OK);
#endif

#if defined(AML_EN_CUSTOM_DMA_ADDR)
    if (NPS_E_OK != rc)
    {
        /* Here the user may invoke the API for their private DMA
         * address conversion.
         */
    }
#endif
    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_convertVirtToPhy(
    void                    *ptr_virt_addr,
    NPS_ADDR_T              *ptr_phy_addr)
{
    NPS_ERROR_NO_T          rc = NPS_E_OK;

#if defined(NPS_EN_DMA_RESERVED)
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info  = &_osal_mdc_cb.dma_info;
    NPS_HUGE_T              rsrv_virt_base = (NPS_HUGE_T)ptr_dma_info->ptr_rsrv_virt_addr;
    NPS_ADDR_T              rsrv_phy_base  = ptr_dma_info->rsrv_phy_addr;
#endif

#if defined(NPS_EN_DMA_RESERVED)
    *ptr_phy_addr = (NPS_ADDR_T)((NPS_HUGE_T)rsrv_phy_base
                                 + (NPS_HUGE_T)ptr_virt_addr - rsrv_virt_base);
#else
    *ptr_phy_addr = 0x0;
    *ptr_phy_addr = virt_to_phys(ptr_virt_addr);
    rc = (0x0 == *ptr_phy_addr) ? (NPS_E_ENTRY_NOT_FOUND) : (NPS_E_OK);
#endif

#if defined(AML_EN_CUSTOM_DMA_ADDR)
    if (NPS_E_OK != rc)
    {
        /* Here the user may invoke the API for their private DMA
         * address conversion.
         */
    }
#endif
    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_initDmaMem(void)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    NPS_ERROR_NO_T          rc = NPS_E_OK;

    rc = osal_createSemaphore(OSAL_MDC_DMA_SEMAPHORE_NAME,
                              NPS_SEMAPHORE_BINARY, &ptr_dma_info->sema);
    if (NPS_E_OK == rc)
    {
#if defined(NPS_EN_DMA_RESERVED)
        rc = _osal_mdc_initRsrvDmaMem(ptr_dma_info);
        if (NPS_E_OK == rc)
        {
            rc = _osal_mdc_createRsrvDmaNodeList(ptr_dma_info);
        }
#else
        rc = _osal_mdc_createSysDmaNodeList(ptr_dma_info);
#endif
    }
    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_deinitDmaMem(void)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;

    /* Common function for both reserved/system memory. */
    _osal_mdc_destroyDmaNodeList(ptr_dma_info);

#if defined(NPS_EN_DMA_RESERVED)
    _osal_mdc_deinitRsrvDmaMem(ptr_dma_info);
#endif

    osal_destroySemaphore(&ptr_dma_info->sema);

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_mdc_flushCache(
    void            *ptr_virt_addr,
    const UI32_T    size)
{
#if defined(CONFIG_NOT_COHERENT_CACHE) || defined(CONFIG_DMA_NONCOHERENT)

#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((NPS_HUGE_T)ptr_virt_addr, size);
#else
    dma_cache_sync(NULL, ptr_virt_addr, size, DMA_TO_DEVICE);
#endif

#endif
    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_mdc_invalidateCache(
    void            *ptr_virt_addr,
    const UI32_T    size)
{
#if defined(CONFIG_NOT_COHERENT_CACHE) || defined(CONFIG_DMA_NONCOHERENT)

#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((NPS_HUGE_T)ptr_virt_addr, size);
#else
    dma_cache_sync(NULL, ptr_virt_addr, size, DMA_FROM_DEVICE);
#endif

#endif
    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_mdc_savePciConfig(
    const UI32_T            unit)
{
    return _osal_mdc_savePciConfig(unit);
}

NPS_ERROR_NO_T
osal_mdc_restorePciConfig(
    const UI32_T            unit)
{
    return _osal_mdc_restorePciConfig(unit);
}

#endif /* End of NPS_LINUX_KERNEL_MODE */

/* --------------------------------------------------------------------------- Interrupt */
#if defined(NPS_LINUX_USER_MODE)
static UI32_T                   _osal_mdc_isr_init_bitmap = 0;  /* To record the dev request_irq */
static UI32_T                   _osal_mdc_isr_dev_bitmap;       /* To record the dev bitmap      */
static spinlock_t               _osal_mdc_isr_dev_bitmap_lock;
static wait_queue_head_t        _osal_mdc_isr_wait;
static UI32_T                   _osal_mdc_isr_mask_addr;
static UI32_T                   _osal_mdc_isr_mask_val;


static inline NPS_ERROR_NO_T
_osal_mdc_initInterrupt(void)
{
    /* init top and bottom halves */
    init_waitqueue_head(&_osal_mdc_isr_wait);

    /* init lock and clear device bitmap */
    spin_lock_init(&_osal_mdc_isr_dev_bitmap_lock);
    _osal_mdc_isr_dev_bitmap = 0;
    _osal_mdc_isr_init_bitmap = 0;

    /* clear chip interrupt mask address and value */
    _osal_mdc_isr_mask_addr = 0;
    _osal_mdc_isr_mask_val = 0;

    return (NPS_E_OK);
}

/* top half */
static inline NPS_ERROR_NO_T
_osal_mdc_notifyUserProcess(
    const UI32_T        unit)
{
    unsigned long       flags = 0;

    /* mask chip interrupt */
    osal_mdc_writePciReg(unit, _osal_mdc_isr_mask_addr,
        &_osal_mdc_isr_mask_val, sizeof(UI32_T));

    /* set the device bitmap. */
    spin_lock_irqsave(&_osal_mdc_isr_dev_bitmap_lock, flags);
    _osal_mdc_isr_dev_bitmap |= (1U << unit);
    spin_unlock_irqrestore(&_osal_mdc_isr_dev_bitmap_lock, flags);

    /* notify user process. */
    wake_up_interruptible(&_osal_mdc_isr_wait);

    return (NPS_E_OK);
}

static inline NPS_ERROR_NO_T
_osal_mdc_waitEvent(
    UI32_T              *ptr_dev_bitmap)
{
    unsigned long       flags = 0;

    wait_event_interruptible(_osal_mdc_isr_wait, (0 != _osal_mdc_isr_dev_bitmap));

    /* save and clear the device bitmap. */
    spin_lock_irqsave(&_osal_mdc_isr_dev_bitmap_lock, flags);
    *ptr_dev_bitmap = _osal_mdc_isr_dev_bitmap;
    _osal_mdc_isr_dev_bitmap = 0;
    spin_unlock_irqrestore(&_osal_mdc_isr_dev_bitmap_lock, flags);

    return (NPS_E_OK);
}

static ssize_t
_osal_mdc_read(
    struct file         *filep,
    char __user         *buf,
    size_t              count,
    loff_t              *ppos)
{
    ssize_t             ret;
    UI32_T              dev_bitmap = 0;

    if (count != sizeof(UI32_T))
    {
        return -EINVAL;
    }

    /* check if request_irq is inited. */
    if (0 != _osal_mdc_isr_init_bitmap)
    {
        _osal_mdc_waitEvent(&dev_bitmap);
    }

    /* copy the device bitmap to user process. */
    if (0 != copy_to_user(buf, &dev_bitmap, count))
    {
        ret = -EFAULT;
    }
    else
    {
        ret = count;
    }

    return ret;
}
#endif /* End of NPS_LINUX_USER_MODE */

static irqreturn_t
_osal_mdc_systemIntrCallback(
    int                 irq,
    void                *ptr_cookie)
{
    NPS_ERROR_NO_T      rc = NPS_E_OK;
    int                 linux_rc = IRQ_HANDLED;
    OSAL_MDC_DEV_T      *ptr_dev = (OSAL_MDC_DEV_T *)ptr_cookie;

    /* Invoke kernel callback, the callback function exist only in below cases:
     * 1. SDK built in kernel mode
     * 2. SDK built in user mode, NetIF kernel module is enabled
     */
    if (NULL != ptr_dev->isr_callback)
    {
        rc = ptr_dev->isr_callback(ptr_dev->ptr_isr_data);
        if (NPS_E_OK != rc)
        {
            OSAL_MDC_ERR("handle irq failed, rc=%d\n", rc);
            linux_rc = IRQ_NONE;
        }
    }

#if defined(NPS_LINUX_USER_MODE)
    /* Notify user process */
    rc = _osal_mdc_notifyUserProcess(ptr_dev->unit);
    if (NPS_E_OK != rc)
    {
        OSAL_MDC_ERR("notify intr to usr failed, rc=%d\n", rc);
        linux_rc = IRQ_NONE;
    }
#endif

    return (linux_rc);
}

NPS_ERROR_NO_T
osal_mdc_registerIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    void                *ptr_cookie)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];

    ptr_dev->isr_callback = handler;
    ptr_dev->ptr_isr_data = (void *)((NPS_HUGE_T)unit);

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_mdc_connectIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    AML_DEV_ISR_DATA_T  *ptr_cookie)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];
    int                 linux_rc = 0;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

#if defined(NPS_LINUX_USER_MODE)
    if (NULL != ptr_cookie)
    {
        _osal_mdc_isr_mask_addr = ptr_cookie->mask_addr;
        _osal_mdc_isr_mask_val  = ptr_cookie->mask_val;
    }
#endif

    if (NULL == ptr_dev->isr_callback)
    {
#if defined(NPS_LINUX_KERNEL_MODE)
        /* In user mode, the following database is created in user space. */
        ptr_dev->isr_callback = handler;
        ptr_dev->ptr_isr_data = (void *)((NPS_HUGE_T)unit);
#endif

#if defined(OSAL_MDC_EN_MSI)
        /* If "no_msi" flag is set, it means the device doesn't support MSI.  */
        if (1 != ptr_dev->ptr_pci_dev->no_msi)
        {
            linux_rc = pci_enable_msi(ptr_dev->ptr_pci_dev);
            if (0 == linux_rc)
            {
                /* The system gives a new irq number if MSI is enabled sucessfully. */
                ptr_dev->irq = ptr_dev->ptr_pci_dev->irq;
            }
            else
            {
                OSAL_MDC_ERR("pci_enable_msi() failed, rc=%d\n", linux_rc);
                rc = NPS_E_OTHERS;
            }
        }
#endif

        linux_rc = request_irq(ptr_dev->irq, _osal_mdc_systemIntrCallback,
                               0, OSAL_MDC_DRIVER_NAME, (void *)ptr_dev);

        if (0 != linux_rc)
        {
            OSAL_MDC_ERR("request_irq() failed, rc=%d\n", linux_rc);
            rc = NPS_E_OTHERS;
        }
    }
    else
    {
        OSAL_MDC_ERR("double req isr err\n");
        rc = NPS_E_OTHERS;
    }
    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_disconnectIsr(
    const UI32_T        unit)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];

    free_irq(ptr_dev->irq, (void *)ptr_dev);

#if defined(OSAL_MDC_EN_MSI)
    /* Must free the irq before disabling MSI */
    pci_disable_msi(ptr_dev->ptr_pci_dev);
#endif

#if defined(NPS_LINUX_KERNEL_MODE)
    ptr_dev->isr_callback = NULL;
    ptr_dev->ptr_isr_data = NULL;
#endif

#if defined(NPS_LINUX_USER_MODE)
    _osal_mdc_isr_mask_addr = 0x0;
    _osal_mdc_isr_mask_val  = 0x0;
#endif

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_mdc_initDevice(
    AML_DEV_T           *ptr_dev_list,
    UI32_T              *ptr_dev_num)
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    NPS_ERROR_NO_T      rc = NPS_E_OK;

    _ptr_osal_mdc_dev = ptr_dev_list;

    memset(ptr_cb, 0x0, sizeof(OSAL_MDC_CB_T));

#if defined(AML_EN_I2C)
    rc = _osal_mdc_probeI2cDevice();
    *ptr_dev_num = 1;
#else
    rc = _osal_mdc_probePciDevice();
    *ptr_dev_num = ptr_cb->dev_num;

    _ptr_ext_pci_dev = _osal_mdc_cb.dev[0].ptr_pci_dev;
#endif /* End of AML_EN_I2C */

    return (rc);
}

NPS_ERROR_NO_T
osal_mdc_deinitDevice(void)
{
    NPS_ERROR_NO_T      rc = NPS_E_OK;

#if defined(AML_EN_I2C)
    rc = _osal_mdc_removeI2cDevice();
#else
    if (NULL != _ptr_ext_pci_dev)
    {
        rc = _osal_mdc_removePciDevice();
        _ptr_ext_pci_dev = NULL;
    }
#endif

    return (rc);
}

/*****************************************************************************/
#if defined(NPS_LINUX_USER_MODE)
/* Interface */
static UI32_T                   _osal_mdc_devInited = 0;

/* DMA */
#if defined(NPS_EN_DMA_RESERVED)
static UI32_T                   _osal_mdc_rsvDmaInited = 0;
#else
static struct list_head         _osal_mdc_sysDmaList[2];       /* To avoid memory corruption when cold-boot */
static UI32_T                   _osal_mdc_sysCurDmaListIdx = 0;
#endif

/* IOCTL */
static OSAL_MDC_IOCTL_CB_T      _osal_mdc_ioctl_cb;
static AML_DEV_T                _osal_mdc_ioctl_dev[NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM] = {};

static int
_osal_mdc_open(
    struct inode    *ptr_inode,
    struct file     *ptr_file)
{
    return (0);
}

static int
_osal_mdc_release(
    struct inode    *ptr_inode,
    struct file     *ptr_file)
{
    return (0);
}

static struct vm_operations_struct  _osal_mdc_remap_vm_ops =
{
    .open  = NULL,
    .close = NULL,
};

static int
_osal_mdc_mmap(
    struct file             *filp,
    struct vm_area_struct   *vma)
{
    size_t  size     = vma->vm_end - vma->vm_start;
    int     linux_rc = 0;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,0,48)
    pgprot_val(vma->vm_page_prot) |= (_PAGE_NO_CACHE | _PAGE_GUARDED);
#else
    UI32_T          dev_idx;
    OSAL_MDC_DEV_T  *ptr_dev;
    NPS_ADDR_T      phy_addr = vma->vm_pgoff << PAGE_SHIFT;

    /* check mmio base phy addr */
    for (dev_idx = 0, ptr_dev = &_osal_mdc_cb.dev[0];
         dev_idx < _osal_mdc_cb.dev_num;
         dev_idx++, ptr_dev++)
    {
        if ((NULL != ptr_dev->ptr_pci_dev) &&
            (phy_addr == pci_resource_start(ptr_dev->ptr_pci_dev, OSAL_MDC_PCI_BAR0_OFFSET)))
        {
            vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            break;
        }
    }
#endif

    vma->vm_flags |= VM_IO;
    if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                           size, vma->vm_page_prot))
    {
        linux_rc = -EAGAIN;
    }
    vma->vm_ops = &_osal_mdc_remap_vm_ops;
    return (linux_rc);
}

#if defined(NPS_EN_DMA_RESERVED)

static NPS_ERROR_NO_T
_osal_mdc_ioctl_initRsrvDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_IOCTL_DMA_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DMA_DATA_T *)ptr_data;
    NPS_ERROR_NO_T  rc = NPS_E_OK;

    if (0 == _osal_mdc_rsvDmaInited)
    {
        rc = _osal_mdc_initRsrvDmaMem(ptr_dma_info);
        _osal_mdc_rsvDmaInited = 1;
    }
    ptr_ioctl_data->rsrv_dma_phy_addr = ptr_dma_info->rsrv_phy_addr;
    ptr_ioctl_data->rsrv_dma_size     = ptr_dma_info->rsrv_size;

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_deinitRsrvDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    NPS_ERROR_NO_T  rc = NPS_E_OK;

    rc = _osal_mdc_deinitRsrvDmaMem(&_osal_mdc_cb.dma_info);
    _osal_mdc_rsvDmaInited = 0;

    return (rc);
}

#else

static NPS_ERROR_NO_T
_osal_mdc_clearSysDmaList(
    UI32_T          dmaListIdx)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_curr_node_data = NULL;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_next_node_data = NULL;
    void                            *ptr_virt_addr;

    list_for_each_entry_safe(ptr_curr_node_data, ptr_next_node_data,
                             &_osal_mdc_sysDmaList[dmaListIdx], list)
    {
        list_del(&(ptr_curr_node_data->list));
        ptr_virt_addr = phys_to_virt(ptr_curr_node_data->phy_addr);
        dma_free_coherent(ptr_dma_info->ptr_dma_dev,
                          ptr_curr_node_data->size, ptr_virt_addr,
                          ptr_curr_node_data->phy_addr);
        kfree(ptr_curr_node_data);
    }

    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_allocSysDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_IOCTL_DMA_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DMA_DATA_T *)ptr_data;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_node_data = NULL;

/* To defense the compatible data type of 32bit and 64bit are not synchronized */
#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT)    /* Bus addressing is 64-bit */\
&& !defined(NPS_EN_64BIT_ADDR)               /* SDK follows HOST with 32-bit addr*/\
&& (defined(NPS_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(NPS_EN_HOST_32_BIT_BIG_ENDIAN))/* HOST is 32-bit */
#error "The DMA address of OS is 64bit. Please enable NPS_EN_64BIT_ADDR in SDK."
#endif

#if !defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(NPS_EN_64BIT_ADDR)
#error "The DMA address of OS is not 64bit. Please disable NPS_EN_64BIT_ADDR in SDK."
#endif

    if (NULL == dma_alloc_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                                   (dma_addr_t *)&ptr_ioctl_data->phy_addr, GFP_ATOMIC))
    {
        return (NPS_E_NO_MEMORY);
    }

    ptr_node_data = kmalloc(sizeof(OSAL_MDC_USER_MODE_DMA_NODE_T), GFP_KERNEL);
    if (NULL != ptr_node_data)
    {
        memset(ptr_node_data, 0, sizeof(OSAL_MDC_USER_MODE_DMA_NODE_T));
        ptr_node_data->phy_addr      = ptr_ioctl_data->phy_addr;
        ptr_node_data->size          = ptr_ioctl_data->size;
        list_add(&(ptr_node_data->list), &_osal_mdc_sysDmaList[_osal_mdc_sysCurDmaListIdx]);
    }
    else
    {
        dma_free_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                          phys_to_virt(ptr_ioctl_data->phy_addr), ptr_ioctl_data->phy_addr);

        return (NPS_E_NO_MEMORY);
    }

    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_freeSysDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_IOCTL_DMA_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DMA_DATA_T *)ptr_data;
    void                            *ptr_virt_addr;

    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_curr_node_data = NULL;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_next_node_data = NULL;


    list_for_each_entry_safe(ptr_curr_node_data, ptr_next_node_data,
                             &_osal_mdc_sysDmaList[_osal_mdc_sysCurDmaListIdx], list)
    {
        if (ptr_curr_node_data->phy_addr == ptr_ioctl_data->phy_addr)
        {
            list_del(&(ptr_curr_node_data->list));
            kfree(ptr_curr_node_data);
            break;
        }
    }

    ptr_virt_addr = phys_to_virt(ptr_ioctl_data->phy_addr);
    dma_free_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                      ptr_virt_addr, ptr_ioctl_data->phy_addr);

    return (NPS_E_OK);
}

#endif

static NPS_ERROR_NO_T
_osal_mdc_getPciInfoToIoctlData(
    const OSAL_MDC_DEV_T        *ptr_dev_list,
    OSAL_MDC_IOCTL_DEV_DATA_T   *ptr_dev_data)
{
    UI32_T  idx;

    /* Search for PCIe device and get the MMIO base address. */
    for (idx = 0; idx < _osal_mdc_cb.dev_num; idx++)
    {
        if (NULL != ptr_dev_list[idx].ptr_pci_dev)
        {
            ptr_dev_data->pci_mmio_phy_start[idx] =
                pci_resource_start(ptr_dev_list[idx].ptr_pci_dev, OSAL_MDC_PCI_BAR0_OFFSET);

            ptr_dev_data->pci_mmio_size[idx] =
                pci_resource_len(ptr_dev_list[idx].ptr_pci_dev, OSAL_MDC_PCI_BAR0_OFFSET);
        }
    }
    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_getDeviceIdToIoctlData(
    AML_DEV_T                   *ptr_dev,
    OSAL_MDC_IOCTL_DEV_DATA_T   *ptr_dev_data,
    const UI32_T                dev_num)
{
    UI32_T  idx;

    for (idx = 0; idx < ptr_dev_data->dev_num; idx++)
    {
        ptr_dev_data->id[idx].device   = ptr_dev[idx].id.device;
        ptr_dev_data->id[idx].vendor   = ptr_dev[idx].id.vendor;
        ptr_dev_data->id[idx].revision = ptr_dev[idx].id.revision;
    }
    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_initDeviceCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_CB_T                   *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DEV_T                  *ptr_dev_list = _osal_mdc_cb.dev;
    OSAL_MDC_IOCTL_DEV_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DEV_DATA_T *)ptr_data;

    /* "dev" is just created for invoking  osal_mdc_initDevice,
     * it is no use once the device IDs are copy to ptr_ioctl_data.
     */

    NPS_ERROR_NO_T                  rc = NPS_E_OK;

    if (0 == _osal_mdc_devInited)
    {
        rc = osal_mdc_initDevice(_osal_mdc_ioctl_dev, &ptr_ioctl_data->dev_num);
    }
    else
    {
        /* ptr_cb->dev_num was initialized in osal_mdc_initDevice(); */
        ptr_ioctl_data->dev_num = ptr_cb->dev_num;
    }

    if (ptr_cb->dev_num >= NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM)
    {
        OSAL_MDC_ERR("dev num=%d > max support num=%d\n",
                     ptr_ioctl_data->dev_num, NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM);
    }

#if !defined(NPS_EN_DMA_RESERVED)
    if (0 == _osal_mdc_devInited)
    {
        /* Create two DMA memory lists and use 1st. */
        INIT_LIST_HEAD(&_osal_mdc_sysDmaList[0]);
        INIT_LIST_HEAD(&_osal_mdc_sysDmaList[1]);
        _osal_mdc_sysCurDmaListIdx = 0;
    }
    else
    {
        /* Delay free the old list until the chip is reset.
         * When we kill the process, the chip continues to write to the DMA memory.
         * If we free the old DMA memory before stopping the chip, there could be memory corruption.
         */
        _osal_mdc_sysCurDmaListIdx = ((_osal_mdc_sysCurDmaListIdx + 1) & 0x1);
        rc = _osal_mdc_clearSysDmaList(_osal_mdc_sysCurDmaListIdx);
    }
#endif

    if (NPS_E_OK == rc)
    {
        rc = _osal_mdc_getDeviceIdToIoctlData(_osal_mdc_ioctl_dev, ptr_ioctl_data, ptr_ioctl_data->dev_num);
    }
    if (NPS_E_OK == rc)
    {
        rc = _osal_mdc_getPciInfoToIoctlData(ptr_dev_list, ptr_ioctl_data);
    }

    _osal_mdc_devInited = 1;
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_deinitDeviceCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    NPS_ERROR_NO_T  rc = NPS_E_OK;

#if !defined(NPS_EN_DMA_RESERVED)
    _osal_mdc_clearSysDmaList(0);
    _osal_mdc_clearSysDmaList(1);
#endif

    if (0 != _osal_mdc_devInited)
    {
        rc = osal_mdc_deinitDevice();
        _osal_mdc_devInited = 0;
    }

    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_connectIsrCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    NPS_ERROR_NO_T  rc = NPS_E_OK;

    if (0 == (_osal_mdc_isr_init_bitmap & (1U << unit)))
    {
        rc = osal_mdc_connectIsr(unit, NULL, ptr_data);
        if (NPS_E_OK == rc)
        {
            _osal_mdc_isr_init_bitmap |= (1U << unit);
        }
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_disconnectIsrCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    /* To make the user-space polling task return from read.  */
    _osal_mdc_notifyUserProcess(unit);

    osal_mdc_disconnectIsr(unit);
    _osal_mdc_isr_init_bitmap &= ~(1U << unit);

    return (NPS_E_OK);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_savePciConfigCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    return _osal_mdc_savePciConfig(unit);
}

static NPS_ERROR_NO_T
_osal_mdc_ioctl_restorePciConfigCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    return _osal_mdc_restorePciConfig(unit);
}

static NPS_ERROR_NO_T
_osal_mdc_registerIoctlCallback(
    const OSAL_MDC_IOCTL_TYPE_T             type,
    const OSAL_MDC_IOCTL_CALLBACK_FUNC_T    func)
{
    OSAL_MDC_IOCTL_CB_T     *ptr_cb = &_osal_mdc_ioctl_cb;
    NPS_ERROR_NO_T          rc = NPS_E_OTHERS;

    if (type < OSAL_MDC_IOCTL_TYPE_LAST)
    {
        if (NULL == ptr_cb->callback[type])
        {
            ptr_cb->callback[type] = func;
            rc = NPS_E_OK;
        }
        else
        {
            OSAL_MDC_ERR("register ioctl callback failed, type=%d exist\n", type);
        }
    }
    else
    {
        OSAL_MDC_ERR("register ioctl callback failed, type=%d >= max=%d\n",
                     type, OSAL_MDC_IOCTL_TYPE_LAST);
    }
    return (rc);
}

static NPS_ERROR_NO_T
_osal_mdc_initIoctl(void)
{
    memset(&_osal_mdc_ioctl_cb, 0x0, sizeof(OSAL_MDC_IOCTL_CB_T));

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_INIT_DEV,
                                    _osal_mdc_ioctl_initDeviceCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_DEV,
                                    _osal_mdc_ioctl_deinitDeviceCallback);
#if defined(NPS_EN_DMA_RESERVED)
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_INIT_RSRV_DMA_MEM,
                                    _osal_mdc_ioctl_initRsrvDmaMemCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_RSRV_DMA_MEM,
                                    _osal_mdc_ioctl_deinitRsrvDmaMemCallback);
#else
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_ALLOC_SYS_DMA_MEM,
                                    _osal_mdc_ioctl_allocSysDmaMemCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
                                    _osal_mdc_ioctl_freeSysDmaMemCallback);
#endif
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_CONNECT_ISR,
                                    _osal_mdc_ioctl_connectIsrCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_DISCONNECT_ISR,
                                    _osal_mdc_ioctl_disconnectIsrCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_SAVE_PCI_CONFIG,
                                    _osal_mdc_ioctl_savePciConfigCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_RESTORE_PCI_CONFIG,
                                    _osal_mdc_ioctl_restorePciConfigCallback);
    return (NPS_E_OK);
}

static long
_osal_mdc_ioctl(
    struct file     *filp,
    unsigned int    cmd,
    unsigned long   arg)
{
#define OSAL_MDC_IOCTL_LOCAL_BUF_SIZE           (128)

    OSAL_MDC_IOCTL_CB_T     *ptr_cb   = &_osal_mdc_ioctl_cb;
    OSAL_MDC_IOCTL_CMD_T    *ptr_cmd  = (OSAL_MDC_IOCTL_CMD_T *)&cmd;
    UI32_T                  unit      = ptr_cmd->field.unit;
    OSAL_MDC_IOCTL_TYPE_T   type      = ptr_cmd->field.type;
    OSAL_MDC_IOCTL_ACCESS_T access    = ptr_cmd->field.access;
    UI32_T                  data_size = ptr_cmd->field.size;
    UI8_T                   temp_buf[OSAL_MDC_IOCTL_LOCAL_BUF_SIZE];
    UI8_T                   *ptr_temp_buf;
    int                     linux_rc  = 0;

    if (NULL != ptr_cb->callback[type])
    {
        if (data_size > OSAL_MDC_IOCTL_LOCAL_BUF_SIZE)
        {
            ptr_temp_buf = kmalloc(data_size, GFP_KERNEL);
        }
        else
        {
            ptr_temp_buf = temp_buf;
        }

        /*************************************************************/
        if (OSAL_MDC_IOCTL_ACCESS_WRITE == access)
        {
            /* type: FREE_SYS_DMA_MEM   : DMA physical address
             *       CONNECT_ISR        : Chip interrupt mask address and value
             */
            if (copy_from_user(ptr_temp_buf, (int __user *)arg, data_size))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (NPS_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
                {
                    linux_rc = -EFAULT;
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_READ == access)
        {
            /* type: INIT_DEV           : PCIe device and vendor ID, mmio address and size
             *       INIT_RSRV_DMA_MEM  : Reserved DMA physical address and size
             */
            if (NPS_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (copy_to_user((int __user *)arg, ptr_temp_buf, data_size))
                {
                    linux_rc = -EFAULT;
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_READ_WRITE == access)
        {
            /* type: ALLOC_SYS_DMA_MEM  : DMA physical address
             */
            if (copy_from_user(ptr_temp_buf, (int __user *)arg, data_size))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (NPS_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
                {
                    linux_rc = -EFAULT;
                }
                else
                {
                    if (copy_to_user((int __user *)arg, ptr_temp_buf, data_size))
                    {
                        linux_rc = -EFAULT;
                    }
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_NONE == access)
        {
            /* type: DEINIT_DEV
             *       DEINIT_RSRV_DMA_MEM
             *       DISCONNECT_ISR
             *       SAVE_PCI_CONFIG
             *       RESTORE_PCI_CONFIG
             */
            if (NPS_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
            {
                linux_rc = -EFAULT;
            }
        }
        /*************************************************************/

        if (data_size > OSAL_MDC_IOCTL_LOCAL_BUF_SIZE)
        {
            kfree(ptr_temp_buf);
        }
    }
    else
    {
        OSAL_MDC_ERR("invalid ioctl, cmd=%u, arg=%lu, type=%d\n", cmd, arg, type);
    }
    return (linux_rc);
}

#ifdef CONFIG_COMPAT
static long
_osal_mdc_compat_ioctl(
    struct file     *filp,
    unsigned int    cmd,
    unsigned long   arg)
{
    return _osal_mdc_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static struct file_operations _osal_mdc_fops =
{
    .owner          = THIS_MODULE,
    .open           = _osal_mdc_open,
    .read           = _osal_mdc_read,
    .release        = _osal_mdc_release,
    .unlocked_ioctl = _osal_mdc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = _osal_mdc_compat_ioctl,
#endif
    .mmap           = _osal_mdc_mmap,
};

static struct miscdevice _osal_mdc_misc =
{
    .minor  = OSAL_MDC_DRIVER_MISC_MINOR_NUM,
    .name   = OSAL_MDC_DRIVER_NAME,
    .fops   = & _osal_mdc_fops,
};

static int __init
osal_mdc_module_init(void)
{
    int     linux_rc;

    _osal_mdc_initIoctl();      /* To register IOCTL callback functions. */
    _osal_mdc_initInterrupt();  /* To init structs for top and bottom halves */

    linux_rc = misc_register(&_osal_mdc_misc);
    if (0 != linux_rc)
    {
        OSAL_MDC_ERR("register dev %s failed, linux_rc=%d\n", OSAL_MDC_DRIVER_NAME, linux_rc);
    }
    return (linux_rc);
}

static void __exit
osal_mdc_module_exit(void)
{
    int     unit = 0;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,8)
    int     linux_rc;

    linux_rc = misc_deregister(&_osal_mdc_misc);
    if (0 != linux_rc)
    {
        OSAL_MDC_ERR("de-register dev %s failed, linux_rc=%d\n", OSAL_MDC_DRIVER_NAME, linux_rc);
    }
#else
    misc_deregister(&_osal_mdc_misc);
#endif

    /* ref: _osal_mdc_ioctl_disconnectIsrCallback */
    for (unit = 0; unit < NPS_CFG_MAXIMUM_CHIPS_PER_SYSTEM; unit++)
    {
        if (0 != (_osal_mdc_isr_init_bitmap & (1U << unit)))
        {
            osal_mdc_disconnectIsr(unit);
            _osal_mdc_isr_init_bitmap &= ~(1U << unit);
        }
    }

    /* ref: _osal_mdc_ioctl_deinitRsrvDmaMemCallback */
#if defined(NPS_EN_DMA_RESERVED)
    if (1 == _osal_mdc_rsvDmaInited)
    {
        _osal_mdc_deinitRsrvDmaMem(&_osal_mdc_cb.dma_info);
        _osal_mdc_rsvDmaInited = 0;
    }
#endif

    /* ref: _osal_mdc_ioctl_deinitDeviceCallback */
    if (1 == _osal_mdc_devInited)
    {
#if !defined(NPS_EN_DMA_RESERVED)
        _osal_mdc_clearSysDmaList(0);
        _osal_mdc_clearSysDmaList(1);
#endif
        osal_mdc_deinitDevice();
        _osal_mdc_devInited = 0;
    }
}

#else

static int __init
osal_mdc_module_init(void)
{
    return (0);
}

static void __exit
osal_mdc_module_exit(void)
{

}

#endif /* End of NPS_LINUX_USER_MODE */

module_init(osal_mdc_module_init);
module_exit(osal_mdc_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MediaTek");
MODULE_DESCRIPTION("SDK Kernel Module");
