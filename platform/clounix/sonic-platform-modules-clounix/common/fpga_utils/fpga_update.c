#include "fpga_pci.h"

struct fpga_fn_if *fpga_func = NULL;
extern int xilinx_fpga_update_init(uint32_t index,void **fpga_op);
extern void xilinx_fpga_update_exit(uint32_t index);
extern int anlogic_fpga_update_init(uint32_t index,void **fpga_op);
extern void anlogic_fpga_update_exit(uint32_t index);
static struct func_map fpga_update_map[] = {
	{CLX8000_PLATFORM_STRING, xilinx_fpga_update_init, xilinx_fpga_update_exit},
    {CLX25600_H_PLATFORM_STRING, xilinx_fpga_update_init, xilinx_fpga_update_exit},
    {CLX25600_BRB_PLATFORM_STRING, xilinx_fpga_update_init, xilinx_fpga_update_exit},
    {CLX25600_SLT_PLATFORM_STRING, xilinx_fpga_update_init, xilinx_fpga_update_exit},
    {CLX25600_L_PLATFORM_STRING, xilinx_fpga_update_init, xilinx_fpga_update_exit},
    {CLX12800_PLATFORM_STRING, anlogic_fpga_update_init, anlogic_fpga_update_exit},
};

int fpga_update_init(const char* platform,uint32_t region)
{
	struct func_map *it;
	int i;
    for (i = 0; i < sizeof(fpga_update_map)/sizeof(fpga_update_map[0]); i++)
    {
	    it = &fpga_update_map[i];
	    if(strcmp(platform, (const char*)it->name) == 0)
	    {
		    return it->init(region,(void *)&fpga_func);
	    }
    }
    return CLX_FAILURE;;
}	
int update_fpga(char *file_name, uint32_t region,uint8_t erase_flag)
{
    uint8_t *image = NULL;
    FILE *fd;
    size_t size, read_bytes, buffer_size;

    fd = fopen(file_name,"rb");
    if(!fd)
        FATAL;

    fseek(fd, 0, SEEK_END);
    size = ftell(fd);

    fseek(fd, 0, SEEK_SET);
    if(!size){
        printf("ERROR: empty image file\n");
        FATAL;
    }

    buffer_size = (size / (fpga_func->page_size) + ((size % (fpga_func->page_size)) ? 1 : 0))*(fpga_func->page_size);
    image = malloc(buffer_size + 1);
    if(!image){
        printf("ERROR: failed to alloc memory buffer\n");
        FATAL;
    }

    memset(image, 0xFF, buffer_size);
    read_bytes = fread(image, 1, size, fd);
    if(size != read_bytes){
        printf("ERROR: failed to read image to memory buffer, exp %ld, act %ld\n", size, read_bytes);
        FATAL;
    }
    fclose(fd);

    CHECK_RC(fpga_func->program_image(image, buffer_size, region,erase_flag));
    printf("\nDone!!\n"); 
	return CLX_SUCCESS;
}
int stress_read_fpga_test(char *file_name, uint32_t region,int loops,uint8_t erase_flag)
{
    uint8_t *image = NULL;
    FILE *fd;
    size_t size= 0, read_bytes, buffer_size;

    fd = fopen(file_name,"rb");
    if(!fd)
        FATAL;

    fseek(fd, 0, SEEK_END);
    size = ftell(fd);
    fseek(fd, 0, SEEK_SET);
    if(!size){
        printf("ERROR: empty image file\n");
        FATAL;
    }
    
    buffer_size = (size / (fpga_func->page_size) + ((size % (fpga_func->page_size)) ? 1 : 0))*(fpga_func->page_size);
    image = malloc(buffer_size + 1);
    if(!image){
        printf("ERROR: failed to alloc memory buffer\n");
        FATAL;
    }

    memset(image, 0xFF, buffer_size);
    read_bytes = fread(image, 1, size, fd);
    if(size != read_bytes){
        printf("ERROR: failed to read image to memory buffer, exp %ld, act %ld\n", size, read_bytes);
        FATAL;
    }
    fclose(fd);
    printf("\nloops: %d, Verify flash......\n",loops); 
    CHECK_RC(fpga_func->verify_image(image,size,region,loops));
    printf("\nVerify done!!\n"); 
    return CLX_SUCCESS;
}

void fpga_update_exit(const char* platform,uint32_t region)
{
	struct func_map *it;
	int i;
    for (i = 0; i < sizeof(fpga_update_map)/sizeof(fpga_update_map[0]); i++)
    {
	    it = &fpga_update_map[i];
	    if(strcmp(platform, (const char*)it->name) == 0)
	    {
            if(it->exit)
		        it->exit(region);
	    }
    }
}	
