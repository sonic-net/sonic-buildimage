#ifndef _FPGA_PCIE_UART_H_
#define _FPGA_PCIE_UART_H_

#ifdef __KERNEL__
#include <linux/types.h>

#else
#include <stdint.h>

#endif

struct mc_uart_platform_data {
    int baud_rate;
    int id;
    int div;
    int clk;
};

struct mc_ocores_uart_i2c_platform_data {
    u32 reg_shift; /* register offset shift value */
    u32 reg_io_width; /* register io read/write width */
    u32 clock_khz; /* input clock in kHz */
    u8 num_devices; /* number of devices in the devices list */
    struct i2c_board_info const *devices; /* devices connected to the bus */
    int nr;                               /* i2c bus num */
};
#define BUFF_SIZE         (256)
/* FPGA Register Address */
#define CTLOFFSET_START                         (0x00)
#define CTLOFFSET_END                           (0x0C)
#define CTLSIZE                                 (CTLOFFSET_END - CTLOFFSET_START)

#define VERSION_OFFSET                          (0x00)
#define DATE_OFFSET                             (0x04)
#define GOLDON_OFFSET                           (0x0C)

#define UART_I2C_MSI_START                      (1)
#define UART_I2C_MSI_NUM                        (32)

/* UART Interrupts */
#define FPGA_MSI_IRQ_NUM                        (14)
#define FPGA_MSI_IRQ_BEGIN                      (0)
#define FPGA_MSI_IRQ_END                        ((FPGA_MSI_IRQ_BEGIN) + (FPGA_MSI_IRQ_NUM))
#define FPGA_UART_CTRL_IRQ(id)                  ((id % 2 == 0) ? (id / 2) : ((id - 1) / 2))

/* UART Registers */
#define FPGA_UART_START_BASE                    (0x200)
#define FPGA_UART_END_BASE                      (0x21c)
#define FPGA_UART_CTRL_SIZE                     (0x20)

/* UART Configuration */
#define UART_BARD_RATE                          (9600)
#define UART_DIVISOR                            (16)
#define UART_CLOCK                              (125000000)
#define FPGA_UART_CTRL_START(id)                ((FPGA_UART_START_BASE) + \
                                                    (id) * (FPGA_UART_CTRL_SIZE))

#define FPGA_UART_CTRL_END(id)                  ((FPGA_UART_END_BASE) + \
                                                    (id) * (FPGA_UART_CTRL_SIZE))

/* UART Present Address */
#define FPGA_UART_PRE_BASE                      (0xf00)
#define FPGA_UART_PRE_SIZE                      (0x04)
#define FPGA_UART_FREEZE_BASE                   (0xf18)
#define FPGA_UART_FREEZE_SIZE                   (0x04)
#define FPGA_UART_LED_BASE                      (0xf30)
#define FPGA_UART_LED_SIZE                      (0x04)
#define FPGA_UART_BAUDRATE_BASE                 (0xe00)
#define FPGA_UART_BAUDRATE_SIZE                 (0x04)

/* I2C */
#define FPGA_I2C_OCORE_START_BASE               (0x800)
#define FPGA_I2C_OCORE_END_BASE                 (0x81f)
#define FPGA_I2C_OCORE_CTRL_SIZE                (0x20)
#define FPGA_I2C_OCORE_CTRL_IRQ(id)             (24 + (id - 11))
#define FPGA_I2C_OCORE_CTRL_START(id)           ((FPGA_I2C_OCORE_START_BASE) + \
                                                    (id - 11) * (FPGA_I2C_OCORE_CTRL_SIZE))

#define FPGA_I2C_OCORE_CTRL_END(id)             ((FPGA_I2C_OCORE_END_BASE) + \
                                                    (id - 11) * (FPGA_I2C_OCORE_CTRL_SIZE))

#define I2C_REG_SHIFT                           (0)
#define I2C_REG_IO_WIDTH                        (4)
#define I2C_CLOCK                               (125000)
#define I2C_DEV_NUM                             (0)
/* Vendor ID */
#define PCI_VENDOR_ID_ALIBABA                   (0x1ded)
#define PCI_VENDOR_ID_XILINX                     (0x10ee)
/* Device ID */
#define PCI_DEVICE_ID_MASS                      (0x7022) /* JAWS, etc */  
#define PCI_DEVICE_ID_AS14_40D                  (0x5203) /* Shamu */
#define PCI_DEVICE_ID_AS24_128D                 (0x5201) /* Migaloo */
#define PCI_DEVICE_ID_AC51_48C2G                (0x5204) /* ATS */
#define PCI_DEVICE_ID_W5000_48CON2SFP           (0x7011)

#endif /* _FPGA_PCIE_I2C_H_ */
