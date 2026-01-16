#include "fpga_pci.h"
#include <unistd.h>
#include <stdio.h>

char      sysfs_path[35]      = "/sys/bus/pci/devices/0000:07:00.0/";
int       access_mode = ONE_BYTE;
int       read_only   = 1;
int       bus         = DEV_PCI_MEM;
int       verbose     = 0;
int       enable_stress_test = 0;
uint32_t  offset      = 0;
uint32_t  length      = 1;
char      *fpga_or_cpld_image = "fpga.bin";
uint32_t  fpga_or_cpld_region = 1;
uint32_t  reg_value = 0;
uint8_t   erase_flag = 0;
uint8_t   update_cpld_golden_flag = 0;
char *version = "2.1";
/*v2.0 support anlogic fpga and cpld upgrade*/
/*V2.1 support anlogic fpga and cpld dual boot func*/

static char g_platform[MAX_PLATFORM_NAME_LEN] = "x86_64-clounix_clx8000_48c8d-r1";

extern int stress_read_fpga_test(char *file_name, uint32_t region,int loops,uint8_t erase_flag);
extern int stress_read_cpld_test(char *file_name, uint32_t region,int loops);

extern int cpld_update_init(const char* platform,uint32_t region);
extern void cpld_update_exit(const char* platform,uint32_t region);
extern int update_cpld(char *file_name, uint32_t region,unsigned char update_cpld_golden_flag);

extern int fpga_update_init(const char* platform,uint32_t region);
extern void fpga_update_exit(const char* platform,uint32_t region);
extern int update_fpga(char *file_name, uint32_t region,uint8_t erase_flag);


void usage(char *prog)
{
    printf("%s verison %s usage: \n", prog,version);
    printf("          -s <sysfs path>, sysfs file for the pci resource\n");
    printf("                           The default path is %s\n", sysfs_path);
    printf("          -w <write>,      Write operation\n");
    printf("          -m <mode>,       Access operation mode: \n");
    printf("                             1: byte\n");
    printf("                             2: halfword\n");
    printf("                             4: word\n");
    printf("                             8: double-word\n");
    printf("                           The default is %d bytes accessing\n", access_mode);
    printf("          -b <bus>,        Select bus: \n");
    printf("                             0: PCI memory bar\n");
    printf("                             3: FPGA cfg Flash\n");
    printf("                             5: CPLD cfg Flash\n");
    printf("          -o <offset>,    Select memory offset\n");
    printf("                           The default is %d \n", offset);
    printf("                           The default is %d \n", bus);
    printf("          -l <length>,    Select memory length: \n");
    printf("                           The default is %d \n", length);
    printf("          -v <value>,     Value to be set into register\n");
    printf("                           The default is 0x%x \n", reg_value);
    printf("          -f <file>,      FPGA(CPLD) image file name \n");
    printf("                           The default is %s \n", fpga_or_cpld_image);
    printf("          -r <region>,    FPGA(CPLD) Region \n");
    printf("                           The default is %d \n", fpga_or_cpld_region);
    printf("          -g,             update(CPLD) golden Region \n");
    printf("                           The default is %d \n", update_cpld_golden_flag);
    printf("          -e <erase_flag>,  FPGA erase the reserved Region \n");
    printf("                           The default is %d \n", erase_flag);
    printf("          -V <verbose>,   Verbosety \n");
    printf("                           The default is %d \n", verbose);
    printf("          -t,             enable stress test \n");
    exit(0);
}

void parse_cli(int argc, char **argv)
{
    int c;
    opterr = 0;
    int option_index = 0;

    struct option long_options[] =
    {
        {"verbose",         no_argument,       0,          'V'},
        {"write",           no_argument,       0,          'w'},
        {"sysfs_path",      required_argument, 0,          's'},
        {"bus",             required_argument, 0,          'b'},
        {"offset",          required_argument, 0,          'o'},
        {"length",          required_argument, 0,          'l'},
        {"mode",            required_argument, 0,          'm'},
        {"file",            required_argument, 0,          'f'},
        {"region",          required_argument, 0,          'r'},
        {"value",           required_argument, 0,          'v'},
        {"erase_flag",      no_argument, 0,               'e'},
        {"enable_stress_test",  no_argument,   0,          't'},
        {"update_cpld_golden",  no_argument,   0,          'g'},
        {"help",            no_argument,       0,          0},
        {0, 0, 0, 0}
    };
    
    while(1)
    {
        option_index = 0;
        
        c = getopt_long (argc, argv, "s:wb:f:m:i:o:l:r:v:a:Vh:teg",
                long_options, &option_index);
        if (c == -1)
            break;

        switch (c)
        {
            case 's':
                strcpy(sysfs_path,optarg);
                break;
            case 'f':
                fpga_or_cpld_image = optarg;
                break;
            case 'v':
                reg_value = strtoul(optarg, NULL, 0);;
                break;
            case 'r':
                fpga_or_cpld_region = strtoul(optarg, NULL, 0);;
                break;
            case 'b':
                bus = strtoul(optarg, NULL, 0);;
                break;
            case 'o':
                offset = strtoul(optarg, NULL, 0);;
                break;
            case 'm':
                access_mode = strtoul(optarg, NULL, 0);;
                break;
            case 'l':
                length = strtoul(optarg, NULL, 0);;
                break; 
            case 'w':
                read_only = 0;
                break;
            case 'V':
                verbose = 1;
                break;
            case 't':
                enable_stress_test = 1;
                break;
            case 'e':
                erase_flag = 1;
                break;    
            case 'g':
                update_cpld_golden_flag = 1;
                break;
            case '?':
            case 'h':
                usage(argv[0]);
            default:
                if (isprint(optopt))
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf(stderr,
                            "Unknown option character `\\x%x'.\n",
                            optopt);
                usage(argv[0]);
        }
    }
}
static void parse_platform_info()
{
    FILE * fp;
    int count = 0;
    char buffer[MAX_PLATFORM_NAME_LEN];
    fp=popen("cat /host/machine.conf | grep onie_platform | awk  -F '='  '{print $2}'","r");
    count = fread(buffer, 1,sizeof(buffer),fp);
 
    if(count> 0)
    {    
        strncpy(g_platform,buffer,count-1);
        g_platform[count-1] = '\0';       
    }
    pclose(fp); 
    return;
}
#if 1
static void parse_fpga_sysfs()
{
    FILE * fp;
    char  buffer[80];
    int count = 0;
    int i = 0;
    if((0 == strcmp(g_platform,CLX8000_PLATFORM_STRING))
       ||(0 == strcmp(g_platform,CLX25600_H_PLATFORM_STRING))
       ||(0 == strcmp(g_platform,CLX25600_BRB_PLATFORM_STRING))
       ||(0 == strcmp(g_platform,CLX25600_SLT_PLATFORM_STRING))
       ||(0 == strcmp(g_platform,CLX25600_L_PLATFORM_STRING))
       )
        fp=popen("lspci | grep Xilinx | awk  -F ' '  '{print $1}'","r");
    else if (0 == strcmp(g_platform,CLX12800_PLATFORM_STRING))
    {
        fp=popen("lspci | grep Synopsys | awk  -F ' '  '{print $1}'","r");
    }else{
        printf("Not support this platform(%s)!!\n",g_platform);
        return;
    }

    count = fread(buffer, 1,sizeof(buffer),fp);
    if(count > 0)
    {  
        for(i = 0; i < count-2; i++)
            sysfs_path[strlen(sysfs_path)-count+i] = buffer[i];
    }
    pclose(fp); 
    return;
}
#endif
uint32_t map_size = 0;
void *map_base = NULL;
int map_fd = 0;
unsigned long map_phyaddr = 0;
/*parse bar memsize*/
int parse_fpga_resource()
{
    int resource_fd = 0;
    char buffer[100];
    ssize_t read_count;
    int i = 0,j=0,tmp=0;
    unsigned long bar_start_addr,bar_end_addr;
    unsigned int bar_size;

    char bar_attr[3][20];
    char *resource = malloc(strlen(sysfs_path)+strlen("resource")+1);
    strcpy(resource,sysfs_path);
    strcat(resource,"resource");
 
    if((resource_fd = open(resource, O_RDONLY)) == -1)
        FATAL;
    read_count = read(resource_fd,buffer,100);
    if(read_count < 0)
    {
        printf("read %s failed\n",resource);
        return -1;
    }
    
    /*BAR*/
    for(i = 0; i < 100;i++)
    {
        if(buffer[i] == 0x20)
        {
            memcpy(bar_attr[j],&buffer[tmp],18);
            bar_attr[j][i] = '\n';
            j++;
            tmp = i+1;
        }
        /*BAR0 end*/
        if(buffer[i] == '\n')
            break;
    }
    bar_start_addr = strtoul(bar_attr[0],NULL,16);
    bar_end_addr = strtoul(bar_attr[1],NULL,16);
    bar_size = bar_end_addr - bar_start_addr + 1;
    
    DBG("bar_start:%lx,bar_end:%lx,size:0x%x\n",bar_start_addr,bar_end_addr,bar_size);
    map_phyaddr = bar_start_addr;
    map_size = bar_size;
    close(resource_fd);
    return 0;

}

void fpga_pci_init()
{
    int ret = CLX_SUCCESS;
    parse_fpga_sysfs();
    parse_fpga_resource();
    if((map_fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1)
        FATAL;

    map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, map_fd, map_phyaddr);
    if(map_base == MAP_FAILED)
        FATAL;

    DBG("PCI Memory mapped to address 0x%08lx.\n", (unsigned long)map_base);
    switch(bus)
    {
        case DEV_PCI_MEM:
             break;
        case DEV_FPGA:
            ret = fpga_update_init(g_platform,fpga_or_cpld_region);
            break;
        case DEV_CPLD:
            ret = cpld_update_init(g_platform,fpga_or_cpld_region);
            break;  
        default:
           break;
    } 
    if(CLX_FAILURE == ret)
    {
        printf("Init fail!!! Please check if this platform supports upgrade?\n");
        exit(-1);
    }
    return;
}
void fpga_pci_exit()
{

    switch(bus)
    {
        case DEV_PCI_MEM:
             break;
        case DEV_FPGA:
            fpga_update_exit(g_platform,fpga_or_cpld_region);
            break;
        case DEV_CPLD:
            cpld_update_exit(g_platform,fpga_or_cpld_region);
            break;  
        default:
            printf("Error: wrong bus type %d\n", bus);
            break;
    }
    munmap(map_base, map_size);
    close(map_fd);
}
int reg_read(uint32_t reg, void *data)
{
    void *virt_addr = map_base + reg;

    if(reg > map_size){
        printf("Wrong register offset 0x%x\n", reg);
        return -1;
    }

    switch(FPGA_REG_WIDTH) {
        case ONE_BYTE:
            *((uint8_t *) data) = *((uint8_t volatile *) virt_addr);
            DBG("Read reg 0x%08x = 0x%x.\n", reg, *((uint8_t *) data));
            break;
        case TWO_BYTE:
            *((uint16_t *) data) = *((uint16_t volatile *) virt_addr);
            DBG("Read reg 0x%08x = 0x%x.\n", reg, *((uint16_t *) data));
            break;
        case FOUR_BYTE:
            *((uint32_t *) data) = *((uint32_t volatile*) virt_addr);
            DBG("Read reg 0x%08x = 0x%x.\n", reg, *((uint32_t *) data));
            break;
        case EIGHT_BYTE:
            *((uint64_t *) data) = *((uint64_t volatile*) virt_addr);
            DBG("Read reg 0x%08x = 0x%lx.\n", reg, *((uint64_t *) data));
            break;
    }
    return 0;
}

int reg_write(uint32_t reg, void *data)
{
    void *virt_addr = map_base + reg;

    if(reg > map_size){
        printf("Wrong register offset 0x%x\n", reg);
        return -1;
    }
    switch(FPGA_REG_WIDTH) {
        case ONE_BYTE:
            DBG("Write reg 0x%08x = 0x%x.\n", reg, *((uint8_t *) data));
            *((uint8_t volatile*) virt_addr) = *((uint8_t *) data);
            break;
        case TWO_BYTE:
            DBG("Write reg 0x%08x = 0x%x.\n", reg, *((uint16_t *) data));
            *((uint16_t volatile*) virt_addr) = *((uint16_t *) data);
            break;
        case FOUR_BYTE:
            DBG("Write reg 0x%08x = 0x%x.\n", reg, *((uint32_t *) data));
            *((uint32_t volatile*) virt_addr) = *((uint32_t *) data);
            break;
        case EIGHT_BYTE:
            DBG("Write reg 0x%08x = 0x%lx.\n", reg, *((uint64_t *) data));
            *((uint64_t volatile*) virt_addr) = *((uint64_t *) data);
            break;
    }
    return 0;
}

int fpga_rw(uint32_t reg, int is_write, void *data, uint32_t len)
{
    int i = 0;
    while(i < (len / FPGA_REG_WIDTH)){
        if(is_write) {
            CHECK_RC(reg_write(reg + i*FPGA_REG_WIDTH,  data + i*FPGA_REG_WIDTH));
            printf("Write reg 0x%08x = 0x%x.\n", reg + i*FPGA_REG_WIDTH, *((uint32_t *) (data + i*FPGA_REG_WIDTH)));
        } else {
            CHECK_RC(reg_read(reg + i*FPGA_REG_WIDTH, data + i*FPGA_REG_WIDTH));
            printf("Read reg 0x%08x = 0x%x.\n", reg + i*FPGA_REG_WIDTH, *((uint32_t *) (data + i*FPGA_REG_WIDTH)));
        }
        i++;
    }
    return 0;
}

int pci_reg_read(uint32_t reg,void *data)
{
    return reg_read(reg, data);
}
int pci_reg_write(uint32_t reg, void *value)
{
    return reg_write(reg, value);
}

uint32_t Spi_In32(uint32_t reg)
{
    uint32_t data32 = 0;
    CHECK_RC(reg_read(reg, &data32));
    return data32;
}

void Spi_Out32(uint32_t reg, uint32_t value)
{
    uint32_t data32 = value;
    reg_write(reg, &data32);
}
const char *clx_get_platform(void)
{
    return g_platform;
}

int action_handler()
{
    switch( bus ) {
        case DEV_PCI_MEM:
            CHECK_RC(fpga_rw(offset, !read_only, (uint8_t *)&reg_value, length));
            break;
        case DEV_FPGA:
            CHECK_RC(update_fpga(fpga_or_cpld_image, fpga_or_cpld_region,erase_flag));
            break;
        case DEV_CPLD:
            CHECK_RC(update_cpld(fpga_or_cpld_image, fpga_or_cpld_region,update_cpld_golden_flag));
            break;
        default:
            printf("Error: wrong bus type %d\n", bus);
            return -1;
    }
    return 0;
}

void stress_handler()
{
    uint32_t loops = 0;
    int ret = 0;
    do {
        printf("input loops:");
        ret = scanf("%d",&loops);
    } while (1 != ret);
    if(DEV_FPGA == bus){
        stress_read_fpga_test(fpga_or_cpld_image, fpga_or_cpld_region,loops,erase_flag);
    }else if(DEV_CPLD == bus){
        stress_read_cpld_test(fpga_or_cpld_image, fpga_or_cpld_region,loops);  
    } 
    
}

int main(int argc, char **argv) {
    parse_cli(argc, argv);
    parse_platform_info();
    fpga_pci_init();
    if(enable_stress_test == 1){
        stress_handler();
    }else {
        action_handler();
    }
    fpga_pci_exit();
	
    exit(EXIT_SUCCESS);
}
