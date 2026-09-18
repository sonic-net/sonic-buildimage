#include "fpga_pci.h"
#include "cpld_update_lattice.h"
#include "fpga_pci_xilinx.h"
#include "driver/anlogic/aspi.h"
#include "driver/xilinx/xspi.h"

ASpi  ASpi_cpld;
static XSpi  XSpi_cpld;
static u8 WriteBuffer[PAGE_SIZE + CMD_EXTRA_BYTES];

int xilinx_cpld_update_spi_init(uint32_t region)
{
	int Status;
	/*
	 * Run the Spi Polled example.
	 */
    switch(region){
        case 1:
            Status = read_write_flash(&XSpi_cpld, XPAR_SPI_1_DEVICE_ID);
            break;
        case 2:
            Status = read_write_flash(&XSpi_cpld, XPAR_SPI_2_DEVICE_ID);
            break;
        default:
            xil_printf("region=%d,Please Enter the right region!!!\r\n",region);
            return CLX_FAILURE;
    }
    if (Status != XST_SUCCESS) {
		xil_printf("cpld_init region %d ,Status %d\r\n",region,Status);
		return CLX_FAILURE;
	}
    return Status;
}
int xilinx_spi_transfer(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt)
{
    memset(WriteBuffer,0,(wcnt+rcnt));
    memcpy(WriteBuffer,SendBufPtr,wcnt);
    return XSpi_Transfer(&XSpi_cpld,WriteBuffer,RecvBufPtr,wcnt+rcnt);

}

int anlogic_cpld_update_spi_init(uint32_t region)
{
	int Status;
	/*
	 * Run the Spi Polled example.
	 */
    switch(region){
        case CPLD_IDX_1:
            Status = ASpi_Initialize(&ASpi_cpld, APAR_SPI_1_DEVICE_ID);
            ASpi_SetCS_CLK(&ASpi_cpld, ASP_MUX_CS_CPLD1, ASP_MUX_CLK_12500K);
            break;
        case CPLD_IDX_2:
            Status = ASpi_Initialize(&ASpi_cpld, APAR_SPI_2_DEVICE_ID);
            ASpi_SetCS_CLK(&ASpi_cpld, ASP_MUX_CS_CPLD2, ASP_MUX_CLK_12500K);
            break;
        case CPLD_IDX_3:
            Status =ASpi_Initialize(&ASpi_cpld,APAR_SPI_3_DEVICE_ID);
            ASpi_SetCS_CLK(&ASpi_cpld, ASP_MUX_CS_SYSCPLD, ASP_MUX_CLK_12500K);
            break;
        default:
            xil_printf("region=%d,Please Enter the right region!!!\r\n",region);
            return CLX_FAILURE;
    }
   if (Status != CLX_SUCCESS) {
		printf("cpld_init region %d ,Status %d\r\n",region,Status);
		return CLX_FAILURE;
	}
    return Status;
}

void anlogic_cpld_update_spi_exit(uint32_t region)
{
    ASpi_Stop(&ASpi_cpld);
    return;
}


