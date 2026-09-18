#include "fpga_pci.h"
#include "cpld_update_anlogic.h"
#include "driver/anlogic/aspi.h"
extern ASpi  ASpi_cpld;	 /* The instance of the SPI device */
#define UPDATE_ADDR 0
#define MAX_IMAGE_SIZE 0x40000
#define GOLDEN_ADDR    0x40000
#define BLOCK_SIZE             0x10000
#define PAGE_SIZE		256
extern int anlogic_cpld_update_spi_init(uint32_t region);
extern void anlogic_cpld_update_spi_exit(uint32_t region);
extern int cpld_update_i2c_init(uint32_t region);
extern void cpld_update_i2c_exit(uint32_t region);
static struct cpld_update_info anlogic_cpld_update_map[] = {
	{CPLD_IDX_1, 0,anlogic_cpld_update_spi_init, NULL,anlogic_cpld_update_spi_exit},
    {CPLD_IDX_2, 0,anlogic_cpld_update_spi_init, NULL,anlogic_cpld_update_spi_exit},
    {CPLD_IDX_3, 0,anlogic_cpld_update_spi_init, NULL,anlogic_cpld_update_spi_exit},
    {CPLD_IDX_4, 0,cpld_update_i2c_init, NULL,cpld_update_i2c_exit}
};
static struct cpld_fn_if anlogic_cpld_update_op;

int anlogic_program_cpld_image(uint8_t *image, size_t size,uint8_t update_cpld_golden_flag)
{
    uint32_t addr = update_cpld_golden_flag ? GOLDEN_ADDR :UPDATE_ADDR;
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
    printf("Erase flash\n"); 

    block_num = MAX_IMAGE_SIZE / BLOCK_SIZE;

    for(i = 0; i < block_num; i++)
    {
        printf("Erasing 0x%x\r", addr + i*BLOCK_SIZE); 
        CHECK_RC(ASpi_Erase(&ASpi_cpld, addr + i*BLOCK_SIZE));
    }

    //2. Program flash
    printf("\nProgram flash\n"); 
    size =  size < max_size  ? size : max_size;
    for(i = 0; i < size; i++)
    {
        if(!(i%0x10))
            printf("Programing 0x%x\r", addr + i); 
       
        CHECK_RC(ASpi_Write(&ASpi_cpld, addr + i, 
                    *(image + i)));
    }

    //3. Read back and check data
    printf("\nVerify flash\n"); 
  
    for(i = 0; i < size; i++)
    {
            if(!(i%0x10))
                printf("Verifying 0x%x\r", addr + i); 
            CHECK_RC(ASpi_Read(&ASpi_cpld, addr + i, &data));
           
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

int anlogic_verify_cpld_image(uint8_t *image, size_t size,uint32_t loops)
{
    uint32_t addr = 0;
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
            CHECK_RC(ASpi_Read(&ASpi_cpld, addr + i, &data));
           
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
int anlogic_cpld_update_init(uint32_t index,void **cpld_op)
{
    int i;
    anlogic_cpld_update_op.program_image = anlogic_program_cpld_image;
    anlogic_cpld_update_op.verify_image = anlogic_verify_cpld_image;
    anlogic_cpld_update_op.page_size = PAGE_SIZE;
    *cpld_op = &anlogic_cpld_update_op;
    for(i= 0; i < sizeof(anlogic_cpld_update_map)/sizeof(anlogic_cpld_update_map[0]); i++)
    {
	    if(index == anlogic_cpld_update_map[i].idx )
	    {   
            anlogic_cpld_update_op.cmdxfer =anlogic_cpld_update_map[i].cmdxfer;
            if(NULL!= anlogic_cpld_update_map[i].init)
		        return anlogic_cpld_update_map[i].init(index);
	    }
    } 
    return CLX_FAILURE;
}
void anlogic_cpld_update_exit(uint32_t index)
{
    int i;
    for(i= 0; i < sizeof(anlogic_cpld_update_map)/sizeof(anlogic_cpld_update_map[0]); i++)
    {
	    if(index == anlogic_cpld_update_map[i].idx)
	    {
            if(NULL!= anlogic_cpld_update_map[i].exit)
		        anlogic_cpld_update_map[i].exit(index);
	    }
    }
}
