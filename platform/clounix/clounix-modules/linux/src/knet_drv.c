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
#include <knet_dev.h>
#include <knet_pci.h>

#include <nb_drv.h>

clx_netif_drv_cb_t default_pkt_driver;
clx_dma_drv_cb_t default_dma_driver;
clx_intr_drv_cb_t default_intr_driver;
clx_pci_drv_cb_t default_pci_driver = {
    .dma_bit_mask = 48,
    .mmio_bar = 0,
};

static clx_drv_cb_t default_driver = {
    .pci_drv = &default_pci_driver,
    .pkt_drv = &default_pkt_driver,
    .intr_drv = &default_intr_driver,
    .dma_drv = &default_dma_driver,
};

int
clx_pkt_drv_init(uint32_t unit)
{
    uint32_t array_elem_num = 0;

    array_elem_num = clx_netif_drv(unit)->unit_num * clx_netif_drv(unit)->slices_per_unit *
        clx_netif_drv(unit)->ports_per_slice;

    clx_netif_drv(unit)->ptr_port_map_db =
        (uint32_t *)kmalloc_array(array_elem_num, sizeof(uint32_t), GFP_ATOMIC);
    if (!clx_netif_drv(unit)->ptr_port_map_db) {
        return -ENOMEM;
    }

    memset(clx_netif_drv(unit)->ptr_port_map_db, (uint32_t)-1, array_elem_num * sizeof(uint32_t));

    return 0;
}

void
clx_pkt_drv_deinit(uint32_t unit)
{
    if (!clx_netif_drv(unit)->ptr_port_map_db) {
        dbg_print(DBG_WARN, "ptr_port_map_db pointer is NULL !!!\n");
        return;
    }

    kfree(clx_netif_drv(unit)->ptr_port_map_db);

    return;
}

int
clx_drv_init(uint32_t unit, struct pci_dev *dev)
{
    int rc = 0;
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);

    if (CLX_DEVICE_IS_NAMCHABARWA(pci_dev_data->device_id)) {
        nb_driver_init(unit, &pci_dev_data->clx_drv);
    } else {
        dbg_print(DBG_WARN, "Unknown chip, use default config !!!\n");
        pci_dev_data->clx_drv = (clx_drv_cb_t *)&default_driver;
        return 0;
    }

    rc = clx_pkt_drv_init(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the pkt driver. unit=%d\n", unit);
        return rc;
    }

    return 0;
}

int
clx_drv_cleanup(uint32_t unit, struct pci_dev *dev)
{
    struct clx_pci_dev_s *pci_dev_data = (struct clx_pci_dev_s *)pci_get_drvdata(dev);

    clx_pkt_drv_deinit(unit);

    if (CLX_DEVICE_IS_NAMCHABARWA(pci_dev_data->device_id)) {
        nb_driver_deinit(unit, &pci_dev_data->clx_drv);
    } else {
        dbg_print(DBG_WARN, "Unknown chip\n");
        return 0;
    }

    return 0;
}
