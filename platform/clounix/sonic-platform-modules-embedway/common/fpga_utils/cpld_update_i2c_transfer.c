
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include "fpga_pci.h"
#include "cpld_update_lattice.h"

static int gFd = -1;
extern char g_platform[20];
/* EVB */
 #define IIC_ADAPTER_CPLD 105
 #define IIC_ADDRESS_CPLD 0x40
 
/*hengwei */
#define IIC_ADAPTER_CPLD_CB 0
#define IIC_ADDRESS_CPLD_CB 0x30
#define IIC_ADAPTER_CPLD_FB 1
#define IIC_ADDRESS_CPLD_FB 0x40

static uint8_t i2c_adapter;
static uint8_t slave_addr;
	
static void get_i2c_adapter_and_slave_addr(uint32_t region,uint8_t *i2c_adapter,uint8_t *slave_addr)
{
    if(strcmp(g_platform, ESX25600_64D_PLATFORM_STRING) == 0)
    {
        if (region == CPLD_IDX_3) 
	{
	    *i2c_adapter = IIC_ADAPTER_CPLD_CB;
	    *slave_addr = IIC_ADDRESS_CPLD_CB;
	} else
	{
            *i2c_adapter = IIC_ADAPTER_CPLD_FB;
            *slave_addr = IIC_ADDRESS_CPLD_FB;	
        }
    }else
    {
	*i2c_adapter = IIC_ADAPTER_CPLD;
        *slave_addr = IIC_ADDRESS_CPLD;
    }
}

int cpld_update_i2c_init(uint32_t region)
{   
    char filename[20];

    get_i2c_adapter_and_slave_addr(region,&i2c_adapter,&slave_addr);
    //printf("adpater:%d slave_addr:0x%x\n",i2c_adapter,slave_addr);
    printf("\n==lattice_cpld_update_i2c_init==\n");
    sprintf(filename, "/dev/i2c-%d", i2c_adapter);
    gFd = open(filename, O_RDWR);
    if(gFd < 0)
    {
        printf("open %s error\n",filename);
        return -ENOENT;
    }
    ioctl(gFd,I2C_TIMEOUT,1);
    ioctl(gFd,I2C_RETRIES,2);	

    return 0;
}
int i2c_transfer(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt)
{
    struct i2c_rdwr_ioctl_data i2c_data;
    int ret = 0;
	//printf("rcnt is%d wcnt %d \n",rcnt,wcnt);
    if(0 == rcnt ) /*write*/
    {
		i2c_data.nmsgs = 1;
		i2c_data.msgs = (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));

		i2c_data.msgs[0].buf  =  SendBufPtr;
		i2c_data.msgs[0].len   = wcnt;
		i2c_data.msgs[0].addr  = slave_addr;
		i2c_data.msgs[0].flags = 0;
    }else{ /*read*/

        i2c_data.nmsgs = 2;
		i2c_data.msgs = (struct i2c_msg *)malloc(i2c_data.nmsgs * sizeof(struct i2c_msg));
		i2c_data.msgs[0].len   = wcnt;
		i2c_data.msgs[0].addr  = slave_addr;
		i2c_data.msgs[0].flags = 0;
		i2c_data.msgs[0].buf  =  SendBufPtr;

		i2c_data.msgs[1].len	= rcnt;
		i2c_data.msgs[1].addr	= slave_addr;
		i2c_data.msgs[1].flags	= I2C_M_RD;
		i2c_data.msgs[1].buf	= RecvBufPtr;
    }
    ret = ioctl(gFd, I2C_RDWR, (unsigned long)&i2c_data);
	free(i2c_data.msgs);
	if(ret<0)
	{
		return ret;
	}	
	else
	{  	
	    return 0;
	}

}

void cpld_update_i2c_exit(uint32_t region)
{
    if( gFd >= 0)
       close(gFd);       
}
