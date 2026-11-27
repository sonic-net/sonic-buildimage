#include <linux/pci.h>
#include <linux/slab.h>
#include "netif_dev.h"
#include "netif_knl.h"

struct pci_dev *_ptr_ext_pci_dev;

gt_netif_drv_cb_t default_pkt_driver;


int
gt_pkt_drv_init(uint32_t unit)
{
    uint32_t array_elem_num = 0;

    array_elem_num = GT_NETIF_MAX_NUM;

    gt_netif_obj(unit)->ptr_port_map_db =
        (uint32_t *)kmalloc_array(array_elem_num, sizeof(uint32_t), GFP_ATOMIC);
    if (!gt_netif_obj(unit)->ptr_port_map_db) {
        return -ENOMEM;
    }

    memset(gt_netif_obj(unit)->ptr_port_map_db, (uint32_t)-1, array_elem_num * sizeof(uint32_t));
    memset(gt_netif_obj(unit)->netif_di2id_map, (uint32_t)-1, sizeof(gt_netif_obj(unit)->netif_di2id_map));

    return 0;
}

void
gt_pkt_drv_deinit(uint32_t unit)
{
    if (!gt_netif_obj(unit)->ptr_port_map_db) {
        dbg_print(DBG_WARN, "ptr_port_map_db pointer is NULL. unit=%u\n", unit);
        return;
    }

    kfree(gt_netif_obj(unit)->ptr_port_map_db);

    return;
}

int
gt_drv_init(uint32_t unit, struct pci_dev *dev)
{
    int rc = 0;
    struct gt_pci_dev_s *pci_dev_data = (struct gt_pci_dev_s *)pci_get_drvdata(dev);

    pci_dev_data->gt_drv = (gt_drv_cb_t *)kmalloc(sizeof(gt_drv_cb_t), GFP_KERNEL);
    if (!pci_dev_data->gt_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for gt_drv. unit=%u\n", unit);
        return -ENOMEM;
    }

    memset(pci_dev_data->gt_drv, 0, sizeof(gt_drv_cb_t));
    /* identify the chip type and init the driver */
    rc = gt_pkt_drv_init(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the pkt driver. unit=%u\n", unit);
        return rc;
    }

    return 0;
}

struct gt_virt_dev_s netif_virt_drvdata;
EXPORT_SYMBOL(netif_virt_drvdata);

int
gt_virt_dev_init(uint32_t unit)
{
    struct gt_virt_dev_s *dev_data = (struct gt_virt_dev_s *)&netif_virt_drvdata;

    dev_data->chip = unit;

    dev_data->gt_drv = (gt_drv_cb_t *)kmalloc(sizeof(gt_drv_cb_t), GFP_KERNEL);
    if (!dev_data->gt_drv) {
        dbg_print(DBG_ERR, "Failed to allocate memory for gt_drv. unit=%u\n", unit);
        return -ENOMEM;
    }
    netif_virt_init(unit, dev_data->gt_drv);

    gt_netif_obj->gt_virt_dev[unit] = dev_data;
    gt_netif_obj->virt_dev_num++;

    return 0;
}

int netif_obj_init(uint32_t unit, struct pci_dev *dev)
{
    int rc = 0;

    if (dev != NULL) {
        rc = gt_drv_init(unit, dev);
    }
    else {
        gt_virt_dev_init(unit);
    }

    rc = gt_pkt_drv_init(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the pkt driver. unit=%u\n", unit);
        return rc;
    }
    return rc;
}

int
gt_drv_deinit(uint32_t unit, struct pci_dev *dev)
{
    struct gt_pci_dev_s *pci_dev_data = (struct gt_pci_dev_s *)pci_get_drvdata(dev);

    gt_pkt_drv_deinit(unit);

    kfree(pci_dev_data->gt_drv);
    pci_dev_data->gt_drv = NULL;

    return 0;
}
