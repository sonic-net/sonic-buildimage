#ifndef FPGA_PCI_XILLIX_H_
#define FPGA_PCI_XILLIX_H_

#define XST_SUCCESS                         (0)
#define XST_FAILURE                         (-1)
#define xil_printf                          printf
/*fpga*/
#define XPAR_SPI_0_DEVICE_ID                (0x1234)
#define XPAR_SPI_0_BASEADDR                 (0x1000000)
#define XPAR_SPI_0_FIFO_EXIST               (1)
#define XPAR_SPI_0_SPI_SLAVE_ONLY           (0)
#define XPAR_SPI_0_NUM_SS_BITS              (1)
#define XPAR_SPI_0_NUM_TRANSFER_BITS        (8)
#define XPAR_SPI_0_SPI_MODE                 (0)
#define XPAR_SPI_0_TYPE_OF_AXI4_INTERFACE   (1)
#define XPAR_SPI_0_AXI4_BASEADDR            (0)
#define XPAR_SPI_0_XIP_MODE                 (0)
#define XPAR_SPI_0_USE_STARTUP              (0)
#define XPAR_SPI_0_FIFO_DEPTH               (16)
/*CPLD0 spi*/
#define XPAR_SPI_1_DEVICE_ID                (0x1235)
#define XPAR_SPI_1_BASEADDR                 (0x1010000)
#define XPAR_SPI_1_FIFO_EXIST               (1)
#define XPAR_SPI_1_SPI_SLAVE_ONLY           (0)
#define XPAR_SPI_1_NUM_SS_BITS              (1)
#define XPAR_SPI_1_NUM_TRANSFER_BITS        (8)
#define XPAR_SPI_1_SPI_MODE                 (0)
#define XPAR_SPI_1_TYPE_OF_AXI4_INTERFACE   (1)
#define XPAR_SPI_1_AXI4_BASEADDR            (0)
#define XPAR_SPI_1_XIP_MODE                 (0)
#define XPAR_SPI_1_USE_STARTUP              (0)
#define XPAR_SPI_1_FIFO_DEPTH               (16)
/*CPLD1 spi*/
#define XPAR_SPI_2_DEVICE_ID                (0x1236)
#define XPAR_SPI_2_BASEADDR                 (0x1020000)
#define XPAR_SPI_2_FIFO_EXIST               (1)
#define XPAR_SPI_2_SPI_SLAVE_ONLY           (0)
#define XPAR_SPI_2_NUM_SS_BITS              (1)
#define XPAR_SPI_2_NUM_TRANSFER_BITS        (8)
#define XPAR_SPI_2_SPI_MODE                 (0)
#define XPAR_SPI_2_TYPE_OF_AXI4_INTERFACE   (1)
#define XPAR_SPI_2_AXI4_BASEADDR            (0)
#define XPAR_SPI_2_XIP_MODE                 (0)
#define XPAR_SPI_2_USE_STARTUP              (0)
#define XPAR_SPI_2_FIFO_DEPTH               (16)

#define XST_OPEN_DEVICE_FAILED              3
#define XST_DEVICE_BUSY                     21L
#define XST_REGISTER_ERROR                  14L

#define XST_LOOPBACK_ERROR              17L
#define XPAR_XSPI_NUM_INSTANCES        3

#define XST_SPI_MODE_FAULT          1151	/*!< master was selected as slave */
#define XST_SPI_TRANSFER_DONE       1152	/*!< data transfer is complete */
#define XST_SPI_TRANSMIT_UNDERRUN   1153	/*!< slave underruns transmit register */
#define XST_SPI_RECEIVE_OVERRUN     1154	/*!< device overruns receive register */
#define XST_SPI_NO_SLAVE            1155	/*!< no slave has been selected yet */
#define XST_SPI_TOO_MANY_SLAVES     1156	/*!< more than one slave is being
                                             * selected */
#define XST_SPI_NOT_MASTER          1157	/*!< operation is valid only as master */
#define XST_SPI_SLAVE_ONLY          1158	/*!< device is configured as slave-only
						 */
#define XST_SPI_SLAVE_MODE_FAULT    1159	/*!< slave was selected while disabled */
#define XST_SPI_SLAVE_MODE          1160	/*!< device has been addressed as slave */
#define XST_SPI_RECEIVE_NOT_EMPTY   1161	/*!< device received data in slave mode */

#define XST_SPI_COMMAND_ERROR       1162	/*!< unrecognised command - qspi only */
#define XST_SPI_POLL_DONE           1163        /*!< controller completed polling the
					                     	   device for status */

#define XIL_COMPONENT_IS_READY     0x11111111U  /**< In device drivers, This macro will be
                                                 assigend to "IsReady" member of driver
												 instance to indicate that driver
												 instance is initialized and ready to use. */
#define XIL_COMPONENT_IS_STARTED   0x22222222U  /**< In device drivers, This macro will be assigend to
                                                 "IsStarted" member of driver instance
												 to indicate that driver instance is
												 started and it can be enabled. */
#define XST_DEVICE_NOT_FOUND            2L
#define XST_DEVICE_BLOCK_NOT_FOUND      3L
#define XST_INVALID_VERSION             4L
#define XST_DEVICE_IS_STARTED           5L
#define XST_DEVICE_IS_STOPPED           6L

#endif