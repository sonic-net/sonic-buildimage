#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "cameo_mdio.h"
/*****************/

/*#define DEBUG*/
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...)  printf("[DEBUG] %s, %s(), line:%d, msg:" fmt, __FILE__, __func__, __LINE__, ##__VA_ARGS__)

#else
#define DEBUG_PRINT(fmt, ...) 
#endif



#define SUCCESS 1
#define FAIL 0
#define READ_BACK_CHECK
#define READ_BACK_CHECK_MS          1
#define MDIO_0                      0x0
#define MDIO_1                      0x10
#define MDIO_2                      0x20
#define MDIO_3                      0x30
#define PCI_DEV_MEM_SIZE            0x5000
#define OP_WRITE                    0x400
#define OP_READ                     0xC00
#define OP_ADDR                     0x0
/*
#define ST_OP_PRTAD_DEVAD_REG       0x30
#define PHY_DATA_REG                0x32
#define GO_AND_DONE_REG             0x34
*/
#define ST_OP_PRTAD_DEVAD_REG       0x0
#define PHY_DATA_REG                0x2
#define GO_AND_DONE_REG             0x4
/*tested 90ms is good!*/
#define MDIO_0_1                      0x0
#define MDIO_1_1                      0x10
#define MDIO_2_1                      0x20
#define MDIO_3_1                      0x30

#define MDIO_0_3                      0x4
#define MDIO_1_3                      0x14
#define MDIO_2_3                      0x24
#define MDIO_3_3                      0x33

size_t BarSize;
int Board_fd = -1;
uint32_t *gP0Mmap;
int gTest = 0;
uint32_t gMdio_Reg_1 = MDIO_1 + ST_OP_PRTAD_DEVAD_REG;
uint32_t gMdio_Reg_2 = MDIO_1 + PHY_DATA_REG;
uint32_t gMdio_Reg_3 = MDIO_1 + GO_AND_DONE_REG;

uint8_t port_addr[2] = {0,0};

uint8_t total_port_addr[16] = {0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,    //card 1 3 5 7 
                                0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17};   //card 2 4 6 8

void cameo_switch_phy_id(uint32_t id,uint32_t card)
{    
    uint32_t index = (id-1)*2;
    if (card % 2 == 0)
    {
        index = index + 8;
    }    
    
    switch (card) {
        case 1:	
        case 2:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_0 ;     
            break;
        case 3:	
        case 4:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_1 ;       
            break;
        case 5:	
        case 6:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_2 ;        
            break;
        case 7:	
        case 8:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_3 ;          
            break;        
        default:            printf("unknown"); break;
    }      
    gMdio_Reg_1 = gMdio_Reg_1 + ST_OP_PRTAD_DEVAD_REG;
    gMdio_Reg_2 = gMdio_Reg_2 + PHY_DATA_REG;
    gMdio_Reg_3 = gMdio_Reg_3 + GO_AND_DONE_REG;
    DEBUG_PRINT("Using MDIO Reg 1: 0x%08x MDIO Reg 2: 0x%08x\n",gMdio_Reg_1,gMdio_Reg_3);
    port_addr[0] = total_port_addr[index];
    port_addr[1] = total_port_addr[index+1];  
    DEBUG_PRINT("\n-------+------------\n");
    DEBUG_PRINT(" Die 0 | 0x%08x  \n",port_addr[0]);
    DEBUG_PRINT(" Die 1 | 0x%08x  \n",port_addr[1]);
    DEBUG_PRINT("-------+------------\n");
    DEBUG_PRINT("gMdio_Reg_1 0x%02x gMdio_Reg_3 0x%02x \n",gMdio_Reg_1,gMdio_Reg_3);
}
void cm_sw_phy_card(uint32_t id,uint32_t card)
{    
    uint32_t index = (id-1)*2;
    if (card % 2 == 0)
    {
        index = index + 8;
    }    
    
    switch (card) {
        case 1:	
        case 2:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_0 ;     
            break;
        case 3:	
        case 4:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_1 ;       
            break;
        case 5:	
        case 6:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_2 ;        
            break;
        case 7:	
        case 8:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_3 ;          
            break;        
        default:            printf("unknown"); break;
    }      
    gMdio_Reg_1 = gMdio_Reg_1 + ST_OP_PRTAD_DEVAD_REG;
    gMdio_Reg_2 = gMdio_Reg_2 + PHY_DATA_REG;
    gMdio_Reg_3 = gMdio_Reg_3 + GO_AND_DONE_REG;
    DEBUG_PRINT("Using MDIO Reg 1: 0x%08x MDIO Reg 2: 0x%08x\n",gMdio_Reg_1,gMdio_Reg_3);
    port_addr[0] = total_port_addr[index];
    port_addr[1] = total_port_addr[index+1];  
    DEBUG_PRINT("\n-------+------------\n");
    DEBUG_PRINT(" Die 0 | 0x%08x  \n",port_addr[0]);
    DEBUG_PRINT(" Die 1 | 0x%08x  \n",port_addr[1]);
    DEBUG_PRINT("-------+------------\n");
    DEBUG_PRINT("gMdio_Reg_1 0x%02x gMdio_Reg_3 0x%02x \n",gMdio_Reg_1,gMdio_Reg_3);
}

void mdio_write(int phyad, int devad, int offset_in_mmd, int data)
{
    DEBUG_PRINT("mdio_write phyad %x devad %x mmd %x data %x \n",phyad,devad,offset_in_mmd,data);

    int output=0;        
    output = port_addr[phyad];
    int addr = 0;   
    int action = 0;
    output = (offset_in_mmd << 16) | OP_ADDR | (output << 5) | devad;   
    
    DEBUG_PRINT("mdio_write 1 output 0x%x \n",output);
    
    
    addr = gMdio_Reg_1;
	*(gP0Mmap + addr/4) = output;    
        
    output = 0x1;
    addr = gMdio_Reg_3;
    *(gP0Mmap + addr/4) = output;  
    
#ifdef READ_BACK_CHECK    
    int i=0;
    while(i<100)
    {
        i++;
        addr = gMdio_Reg_3;
        action = 0;
        action = *(gP0Mmap + addr/4); 
        usleep(READ_BACK_CHECK_MS);
        if ((action & 0x1) == 0x1)
        {            
            break;
        }
    }
#endif    
    output = 0;        
    output = port_addr[phyad];
    output = (data << 16) | OP_WRITE | (output << 5) | devad; 
    
    DEBUG_PRINT("mdio_write 2 output 0x%x \n",output); 
        
    addr = gMdio_Reg_1;
	*(gP0Mmap + addr/4) = output;
    
      
    output = 0x1;
    addr = gMdio_Reg_3;
    
    *(gP0Mmap + addr/4) = output;    

#ifdef READ_BACK_CHECK     
    i=0;
    while(i<100)
    {
        i++;
        addr = gMdio_Reg_3;
        action = 0;
        action = *(gP0Mmap + addr/4); 
        usleep(READ_BACK_CHECK_MS);
        if ((action & 0x1)== 0x1)
        {
            break;
        }
    }    
#endif    
}

unsigned short mdio_read(int phyad, int devad, int offset_in_mmd)
{
    DEBUG_PRINT("mdio_read phyad %x devad %x mmd %x \n",phyad,devad,offset_in_mmd);
    
    int addr = 0;
    int output= 0; 
    int read_result = 0,action = 0;  
    unsigned short data = 0;
    output = port_addr[phyad];
    output = (offset_in_mmd << 16) | OP_ADDR | (output << 5) | devad;    
    
    DEBUG_PRINT("mdio_read 1 output 0x%x \n",output);    
    
    
    addr = gMdio_Reg_1;
	*(gP0Mmap + addr/4) = output;
    
    
    output = 0x1;
    addr = gMdio_Reg_3;
    *(gP0Mmap + addr/4) = output;  
    
#ifdef READ_BACK_CHECK     
    int i=0;
    while(i<100)
    {
        i++;
        addr = gMdio_Reg_3;
        action = 0;
        action = *(gP0Mmap + addr/4); 
        usleep(READ_BACK_CHECK_MS);
        if ((action & 0x1) == 0x1)
        {
            break;
        }
    } 
#endif    

    output = 0;        
    output = port_addr[phyad];
    output = OP_READ|(output << 5)|devad;      
    
    addr = gMdio_Reg_1;
    *(gP0Mmap + addr/4) = output;
    
    
    output = 0x2;
    addr = gMdio_Reg_3;
    *(gP0Mmap + addr/4) = output;
    
    DEBUG_PRINT("mdio_read 2 output 0x%x \n",output); 
#ifdef READ_BACK_CHECK     
    i=0;
    while(i<100)
    {
        i++;
        addr = gMdio_Reg_3;
        action = 0;
        action = *(gP0Mmap + addr/4); 
        usleep(READ_BACK_CHECK_MS);
        if ((action & 0x2) == 0x2)
        {
            break;
        }
    }     
#endif    
    
    addr = gMdio_Reg_1;
    read_result = 0;
	read_result = *(gP0Mmap + addr/4); 
    data = (read_result>>16);
    
    DEBUG_PRINT("mdio_read 3 output 0x%08x data 0x%08x \n",output,read_result);
    
    
    return data;
}
bool slot_addr_check(unsigned short card,unsigned short slot_addr)
{
    switch (card) {
        case 1:	
        case 3:	
        case 5:	
        case 7:	                     
            if (slot_addr==0x18 || slot_addr==0x1A || slot_addr==0x1C || slot_addr==0x1E) 
            {
                return true;
            }
            break;    
        case 2:	
        case 4:	
        case 6:	
        case 8:	
            if (slot_addr==0x10 || slot_addr==0x12 || slot_addr==0x14 || slot_addr==0x16) 
            {
                return true;
            }       
            break;               
        default:            printf("unknown"); return true;
    }  
    
    return false;
    
}
unsigned short mdio_read_slot(unsigned short card,unsigned short prtad, unsigned short devad, unsigned int reg)
{       
    if (!slot_addr_check(card,prtad)){    
        printf("Error! mdio_read_slot(card,prtad,devad,reg)\n");    
        return 0;
    }
    switch (card) {
        case 1:	
        case 2:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_0 ;     
            break;
        case 3:	
        case 4:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_1 ;       
            break;
        case 5:	
        case 6:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_2 ;        
            break;
        case 7:	
        case 8:	
            gMdio_Reg_1=gMdio_Reg_2=gMdio_Reg_3=MDIO_3 ;          
            break;        
        default:            printf("unknown"); break;
    }              
    gMdio_Reg_1 = gMdio_Reg_1 + ST_OP_PRTAD_DEVAD_REG;
    gMdio_Reg_2 = gMdio_Reg_2 + PHY_DATA_REG;
    gMdio_Reg_3 = gMdio_Reg_3 + GO_AND_DONE_REG;    
    port_addr[0] = prtad;
    port_addr[1] = prtad+1;  
    return mdio_read(0, devad, reg);
}


int lscpcie_open()
{
	int fd;		
	char filename[256];   
    BarSize = PCI_DEV_MEM_SIZE;   // default for our demo BAR1
    
	sprintf(filename, "/dev/ECP3_Basic_1");
    
	/* Open the kernel mem object to gain access
	 */
	/*fd = open(region, O_RDWR, 0666); */
    fd = open(filename, O_RDWR | O_SYNC);
	if (fd == -1)
	{
		perror("ERROR open(): ");
		return FAIL;
	}
    	


	gP0Mmap = mmap(0,             /* choose any user address */
		    BarSize,           /* This big */
		    PROT_READ | PROT_WRITE, /* access control */
		    MAP_SHARED,             /* access control */
		    fd,                  /* the object */
		    0);                  /* the offset from beginning */
	if (gP0Mmap == MAP_FAILED)
	{
		perror("mmap: ");
		return FAIL;
	}	

	Board_fd = fd;    
    DEBUG_PRINT("lscpcie_open Seccess.\n");
    return SUCCESS;
}


uint32_t lscpcie_close()
{
	int sysErr;

	/* Release the shared memory.  It won't go away but we're done with it */
	sysErr = munmap(gP0Mmap, BarSize);
	if (sysErr == -1)
	{
		perror("munmap: ");
		return FAIL;
	}

	close(Board_fd);

	return SUCCESS;
}


