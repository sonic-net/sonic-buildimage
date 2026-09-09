#ifndef FPGA_PCI_ANLOGIC_H_
#define FPGA_PCI_ANLOGIC_H_

/*fpga*/
#define APAR_SPI_0_DEVICE_ID                (0x1234)
#define APAR_SPI_0_BASEADDR                 (0xa00)
#define APAR_SPI_0_FIFO_EXIST               (0)
#define APAR_SPI_0_SPI_SLAVE_ONLY           (0)
#define APAR_SPI_0_NUM_SS_BITS              (1)
#define APAR_SPI_0_NUM_TRANSFER_BITS        (8)
#define APAR_SPI_0_SPI_MODE                 (0)
#define APAR_SPI_0_TYPE_OF_AXI4_INTERFACE   (1)
#define APAR_SPI_0_AXI4_BASEADDR            (0)
#define APAR_SPI_0_XIP_MODE                 (0)
#define APAR_SPI_0_USE_STARTUP              (0)
#define APAR_SPI_0_FIFO_DEPTH               (0)
/*CPLD 1 spi*/
#define APAR_SPI_1_DEVICE_ID                (0x1235)
#define APAR_SPI_1_BASEADDR                 (0xa00)
#define APAR_SPI_1_FIFO_EXIST               (0)
#define APAR_SPI_1_SPI_SLAVE_ONLY           (0)
#define APAR_SPI_1_NUM_SS_BITS              (1)
#define APAR_SPI_1_NUM_TRANSFER_BITS        (8)
#define APAR_SPI_1_SPI_MODE                 (0)
#define APAR_SPI_1_TYPE_OF_AXI4_INTERFACE   (1)
#define APAR_SPI_1_AXI4_BASEADDR            (0)
#define APAR_SPI_1_XIP_MODE                 (0)
#define APAR_SPI_1_USE_STARTUP              (0)
#define APAR_SPI_1_FIFO_DEPTH               (0)
/*CPLD2 spi*/
#define APAR_SPI_2_DEVICE_ID                (0x1236)
#define APAR_SPI_2_BASEADDR                 (0xa00)
#define APAR_SPI_2_FIFO_EXIST               (0)
#define APAR_SPI_2_SPI_SLAVE_ONLY           (0)
#define APAR_SPI_2_NUM_SS_BITS              (1)
#define APAR_SPI_2_NUM_TRANSFER_BITS        (8)
#define APAR_SPI_2_SPI_MODE                 (0)
#define APAR_SPI_2_TYPE_OF_AXI4_INTERFACE   (1)
#define APAR_SPI_2_AXI4_BASEADDR            (0)
#define APAR_SPI_2_XIP_MODE                 (0)
#define APAR_SPI_2_USE_STARTUP              (0)
#define APAR_SPI_2_FIFO_DEPTH               (0)
/* sys CPLD */
#define APAR_SPI_3_DEVICE_ID                (0x1336)
#define APAR_SPI_3_BASEADDR                 (0xa00)
#define APAR_SPI_3_FIFO_EXIST               (0)
#define APAR_SPI_3_SPI_SLAVE_ONLY           (0)
#define APAR_SPI_3_NUM_SS_BITS              (1)
#define APAR_SPI_3_NUM_TRANSFER_BITS        (8)
#define APAR_SPI_3_SPI_MODE                 (0)
#define APAR_SPI_3_TYPE_OF_AXI4_INTERFACE   (1)
#define APAR_SPI_3_AXI4_BASEADDR            (0)
#define APAR_SPI_3_XIP_MODE                 (0)
#define APAR_SPI_3_USE_STARTUP              (0)
#define APAR_SPI_3_FIFO_DEPTH               (0)
/* fan CPLD */
#define APAR_SPI_4_DEVICE_ID                (0x1446)
#define APAR_SPI_4_BASEADDR                 (0xa00)
#define APAR_SPI_4_FIFO_EXIST               (0)
#define APAR_SPI_4_SPI_SLAVE_ONLY           (0)
#define APAR_SPI_4_NUM_SS_BITS              (1)
#define APAR_SPI_4_NUM_TRANSFER_BITS        (8)
#define APAR_SPI_4_SPI_MODE                 (0)
#define APAR_SPI_4_TYPE_OF_AXI4_INTERFACE   (1)
#define APAR_SPI_4_AXI4_BASEADDR            (0)
#define APAR_SPI_4_XIP_MODE                 (0)
#define APAR_SPI_4_USE_STARTUP              (0)
#define APAR_SPI_4_FIFO_DEPTH               (0)

#define APAR_SPI_NUM_INSTANCES        5

#define ANL_OPEN_DEVICE_FAILED              3
#define ANL_DEVICE_BUSY                     21L
#define ANL_REGISTER_ERROR                  14L

#define ANL_COMPONENT_IS_READY     0x11111111U  /**< In device drivers, This macro will be
                                                 assigend to "IsReady" member of driver
												 instance to indicate that driver
												 instance is initialized and ready to use. */
#define ANL_COMPONENT_IS_STARTED   0x22222222U  /**< In device drivers, This macro will be assigend to
                                                 "IsStarted" member of driver instance
												 to indicate that driver instance is
												 started and it can be enabled. */
#define ANL_DEVICE_NOT_FOUND            2L
#define ANL_DEVICE_BLOCK_NOT_FOUND      3L
#define ANL_INVALID_VERSION             4L
#define ANL_DEVICE_IS_STARTED           5L
#define ANL_DEVICE_IS_STOPPED           6L
#endif