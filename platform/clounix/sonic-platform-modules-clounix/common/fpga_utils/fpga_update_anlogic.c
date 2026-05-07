#include "fpga_pci.h"
#include "fpga_pci_anlogic.h"
#include "driver/anlogic/aspi.h"

#define UPDATE_ADDR 0
#define MAX_IMAGE_SIZE 0x400000
#define GOLDEN_ADDR    0x800000

#define BLOCK_SIZE     0x10000

/*
 * Number of bytes per page in the flash device.
 */
#define PAGE_SIZE		256
static ASpi  Spi;	 /* The instance of the SPI device */

int anlogic_program_fpga_image(uint8_t *image, size_t size, uint32_t region,uint8_t erase_flag)
{
    uint32_t addr = region ? UPDATE_ADDR : GOLDEN_ADDR;
    int i = 0;
    int block_num = 0;
    int max_size = MAX_IMAGE_SIZE;
    u8 data = 0;
    //0, Check image size
    if(size > MAX_IMAGE_SIZE){
        printf("Image size(0x%lx) is lager than 0x%x\n", size, MAX_IMAGE_SIZE); 
        return -1;
    }

    //1. Erase flash
    printf("Erase flash......\r\n"); 
    block_num = MAX_IMAGE_SIZE / BLOCK_SIZE;
    for(i = 0; i < block_num; i++)
    {
        printf("Erasing 0x%x\r", addr + i*BLOCK_SIZE); 
        CHECK_RC(ASpi_Erase(&Spi, addr + i*BLOCK_SIZE));
    }
 
    //2. Program flash
    printf("\nProgram flash......\n"); 
    size =  size < max_size  ? size : max_size;
    for(i = 0; i < size; i++)
    {
        if(!(i%0x10))
            printf("Programing 0x%x\r", addr + i); 
       
        CHECK_RC(ASpi_Write(&Spi, addr + i, 
                    *(image + i)));
    }

    //3. Read back and check data
    printf("\nVerify flash\n"); 
    for(i = 0; i < size; i++)
    {
        if(!(i%0x10))
            printf("Verifying 0x%x\r", addr + i); 
        CHECK_RC(ASpi_Read(&Spi, addr + i, &data));
           
        if(data != *(image + i)) {
                    printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        addr + i,
                        *(image + i),
                        data);
                return CLX_FAILURE;
        }
    }
	return CLX_SUCCESS;
}
int anlogic_verify_fpga_image(uint8_t *image, size_t size,uint32_t region,uint32_t loops)
{
    uint32_t addr = region ?   GOLDEN_ADDR : UPDATE_ADDR;
    int i = 0,loop =0;
    int max_size = MAX_IMAGE_SIZE;
    u8 data;
	size =  size < max_size  ? size : max_size;
    for(loop=0;loop < loops;loop++){
        printf("=========loop=%d=========\n",loop+1);
        for(i = 0; i < size; i++)
        {
            if(!(i%0x10))
                printf("Verifying 0x%x\r", addr + i); 
            CHECK_RC(ASpi_Read(&Spi, addr + i, &data));
           
            if(data != *(image + i)) {
                    printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        addr + i,
                        *(image + i),
                        data);
                return CLX_FAILURE;
            }
        }
	}   
    return CLX_SUCCESS;
}
static struct fpga_fn_if anlogic_fpga_update_op;
int anlogic_fpga_update_init(uint32_t region,void **fpga_op)
{
    CHECK_RC(ASpi_Initialize(&Spi,APAR_SPI_0_DEVICE_ID));
    ASpi_SetCS_CLK(&Spi, ASP_MUX_CS_FPGA,ASP_MUX_CLK_25M);
    anlogic_fpga_update_op.program_image = anlogic_program_fpga_image;
    anlogic_fpga_update_op.verify_image = anlogic_verify_fpga_image;  
    anlogic_fpga_update_op.page_size = PAGE_SIZE;
    *fpga_op = &anlogic_fpga_update_op;
    return CLX_SUCCESS;
}
void anlogic_fpga_update_exit(uint32_t region)
{
    ASpi_Stop(&Spi);
    return;
}