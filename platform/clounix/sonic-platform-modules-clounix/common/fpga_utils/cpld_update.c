#include "fpga_pci.h"
#include "driver/xilinx/xspi.h"
#include "cpld_update_lattice.h"

struct cpld_fn_if *cpld_func;
extern int lattice_cpld_update_init(uint32_t index,void **cpld_op);
extern void lattice_cpld_update_exit(uint32_t index);
extern int anlogic_cpld_update_init(uint32_t index,void **cpld_op);
extern void anlogic_cpld_update_exit(uint32_t index);
static struct func_map cpld_update_map[] = {
    {CLX8000_PLATFORM_STRING, lattice_cpld_update_init, lattice_cpld_update_exit},
    {CLX25600_H_PLATFORM_STRING, lattice_cpld_update_init, lattice_cpld_update_exit},
    {CLX25600_L_PLATFORM_STRING, lattice_cpld_update_init, lattice_cpld_update_exit},
    {CLX12800_PLATFORM_STRING, anlogic_cpld_update_init, anlogic_cpld_update_exit},
};

int cpld_update_init(const char* platform,uint32_t region)
{
	struct func_map *it;
	int i;
    for (i = 0; i < sizeof(cpld_update_map)/sizeof(cpld_update_map[0]); i++)
    {
	    it = &cpld_update_map[i];
	    if(strcmp(platform, (const char*)it->name) == 0)
	    {
		    return it->init(region,(void *)&cpld_func);
	    }
    }
    return CLX_FAILURE;
}	
int update_cpld(char *file_name, uint32_t region,unsigned char update_cpld_golden_flag)
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

    buffer_size = (size / (cpld_func->page_size) + ((size % (cpld_func->page_size) ) ? 1 : 0))*(cpld_func->page_size);
    image = malloc(buffer_size );
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

    CHECK_RC(cpld_func->program_image(image, buffer_size,update_cpld_golden_flag));
    printf("\nProgram done!! \n"); 
	return CLX_SUCCESS;
}
int stress_read_cpld_test(char *file_name, uint32_t region,int loops)
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

    buffer_size = (size / (cpld_func->page_size) + ((size % (cpld_func->page_size)) ? 1 : 0))*(cpld_func->page_size);
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
    CHECK_RC(cpld_func->verify_image(image,size,loops));
    printf("\nVerify done!!\n"); 
    return CLX_SUCCESS;
}

void cpld_update_exit(const char* platform,uint32_t region)
{
	struct func_map *it;
	int i;
    for (i = 0; i < sizeof(cpld_update_map)/sizeof(cpld_update_map[0]); i++)
    {
	    it = &cpld_update_map[i];
	    if(strcmp(platform, (const char*)it->name) == 0)
	    {
            if(it->exit)
		        it->exit(region);
	    }
    }
}	
