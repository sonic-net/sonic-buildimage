#ifndef __WB_LPC_DRV_H__
#define __WB_LPC_DRV_H__

#define LPC_IO_NAME_MAX_LEN (64)

typedef struct lpc_drv_device_s {
    char lpc_io_name[LPC_IO_NAME_MAX_LEN];
    int pci_domain;
    int pci_bus;
    int pci_slot;
    int pci_fn;
    int lpc_io_base;
    int lpc_io_size;
    int lpc_gen_dec;
    int device_flag;
} lpc_drv_device_t;

#endif
