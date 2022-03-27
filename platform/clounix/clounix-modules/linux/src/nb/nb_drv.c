#include "knet_dev.h"
#include "knet_pci.h"
#include "nb_drv.h"
#include "nb_chip.h"

extern clx_netif_drv_cb_t nb_pkt_driver;
extern clx_dma_drv_cb_t nb_dma_driver;
extern clx_intr_drv_cb_t nb_intr_driver;
extern clx_pci_drv_cb_t nb_pci_driver;

static clx_drv_cb_t nb_driver = {
    .pci_drv = &nb_pci_driver,
    .pkt_drv = &nb_pkt_driver,
    .intr_drv = &nb_intr_driver,
    .dma_drv = &nb_dma_driver,
};

int
nb_driver_init(uint32_t unit, clx_drv_cb_t **pptr_clx_drv)
{
    int rc = 0;

    rc = nb_init_dma_driver(unit);
    if (0 != rc) {
        dbg_print(DBG_ERR, "Failed to init the dma driver. unit=%d\n", unit);
        return rc;
    }

    *pptr_clx_drv = (clx_drv_cb_t *)&nb_driver;
    return 0;
}
int
nb_driver_deinit(uint32_t unit, clx_drv_cb_t **pptr_clx_drv)
{
    nb_cleanup_dma_driver(unit);
    *pptr_clx_drv = NULL;
    return 0;
}
