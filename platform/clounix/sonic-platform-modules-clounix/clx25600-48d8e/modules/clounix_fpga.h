#ifndef _CLOUNIX_FPGA_H_
#define _CLOUNIX_FPGA_H_

#define FPGA_VENDOR_ID 0x10ee
#define FPGA_DEVICE_ID 0x7021

/*power cycle*/
#define FPGA_GLOBAL_CFG_BASE 0x100
#define FPGA_RESET_CFG_BASE  (FPGA_GLOBAL_CFG_BASE+0) 
#define P12V_STBY_EN   1
#define RESET_MUX_BIT  4

/*PVT mgr*/
#define FPGA_PVT_BASE  0x900
#define FPGA_PVT_MGR_CFG (FPGA_PVT_BASE +0x0)
#define FPGA_PVT_MGR_DATA (FPGA_PVT_BASE +0x4)
#define FPGA_PVT_MGR_CFG_RST_BIT 31
#define FPGA_PVT_MGR_CFG_EN_BIT  30
#define FPGA_PVT_MGR_DATA_MASK 0x00000FFF

#define GET_BIT(data, bit, value)   value = (data >> bit) & 0x1
#define SET_BIT(data, bit)          data |= (1 << bit)
#define CLEAR_BIT(data, bit)        data &= ~(1 << bit)
/*CPLD init register*/
#define CPLD_BASE_ADDRESS           (0x0300)
#define CPLD_INTF_CONFIG            CPLD_BASE_ADDRESS 

#define CPLD0_RST_BIT      31
#define CPLD0_EN_BIT       30
#define CPLD1_RST_BIT      29
#define CPLD1_EN_BIT       28

extern void __iomem * fpga_ctl_addr;
#endif