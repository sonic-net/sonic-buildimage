/*
 *
 * Description:
 *  This is the required header file for customed i2c algorithms
 */

#ifndef __DS5001_I2C_ALGO_H__
#define __DS5001_I2C_ALGO_H__
#include "../../../../pddf/i2c/modules/include/pddf_client_defs.h"

/* max number of adapters */
#define I2C_PCI_MAX_BUS 20

/**
 * struct fpgapci_devdata - PCI device data structure
 * support one device per PCIe
 */
struct fpgapci_devdata {
  struct pci_dev *pci_dev;

  /* kernels virtual addr for fpga_data_base_addr */
  void * __iomem fpga_data_base_addr;

  /* kernels virtual addr. for the i2c_ch_base_addr */
  void * __iomem fpga_i2c_ch_base_addr;

  /* size per i2c_ch */
  int  fpga_i2c_ch_size;

  /* number of supported virtual i2c buses */
  int  max_fpga_i2c_ch;

  size_t bar_length;
};


typedef struct
{
  uint32_t vendor_id;
  uint32_t device_id;
  uint32_t virt_bus;
  uint32_t virt_i2c_ch;
  uint32_t data_base_offset;
  uint32_t data_size;
  uint32_t i2c_ch_base_offset;
  uint32_t i2c_ch_size;
} FPGA_OPS_DATA;

/*****************************************
 *  kobj list
 *****************************************/

struct kobject *fpgapci_kobj=NULL;


#define NAME_SIZE 32




#endif
