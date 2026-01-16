#include "fpga_pci.h"
#include "cpld_update_lattice.h"

static int gCmdExBytes = CMD_EXTRA_BYTES;

extern int xilinx_spi_transfer(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt);
extern int xilinx_cpld_update_spi_init(uint32_t region);
extern int cpld_update_i2c_init(uint32_t region);
extern void cpld_update_i2c_exit(uint32_t region);
extern int i2c_transfer(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt);

static struct cpld_update_info lattice_cpld_update_map[] = {
	{CPLD_IDX_1, CMD_EXTRA_BYTES,xilinx_cpld_update_spi_init, xilinx_spi_transfer,NULL},
    {CPLD_IDX_2, CMD_EXTRA_BYTES,xilinx_cpld_update_spi_init, xilinx_spi_transfer,NULL},
    {CPLD_IDX_3, 0,cpld_update_i2c_init, i2c_transfer,cpld_update_i2c_exit}
};
static struct cpld_fn_if lattice_cpld_update_op;

static int cmdxfer(uint8_t *SendBufPtr,uint32_t wcnt,uint8_t *RecvBufPtr,uint32_t rcnt)
{   
    return lattice_cpld_update_op.cmdxfer(SendBufPtr,wcnt,RecvBufPtr,rcnt);
}
uint32_t readDeviceID(uint8_t *ibuf) {
     uint8_t obuf[4] = {0xE0, 0x00, 0x00, 0x00};
     return cmdxfer(obuf, 4, ibuf, 4);
}

uint32_t readUserCode(uint8_t *ibuf) {
    uint8_t obuf[4] = {0xC0, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, ibuf, 4);
}

uint32_t readStatus(uint8_t *ibuf) {
    uint8_t obuf[4] = {0x3C, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, ibuf, 4);
}

uint32_t readFeatureBits(uint8_t *ibuf) {
    uint8_t obuf[4] = {0xFB, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, ibuf, 2);
}


uint32_t readOTPFuses(uint8_t *ibuf) {
    uint8_t obuf[4] = {0xFA, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, ibuf, 1);
}

uint32_t eraseUFM() {
    uint8_t obuf[4] = {0xCB, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t erase() {
    uint8_t obuf[4] = {0x0E, 0x04, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t enableConfigTransparent() {
    uint8_t obuf[4] = {0x74, 0x08, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t enableConfigOffline() {
    uint8_t obuf[4] = {0xC6, 0x08, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);

}
uint32_t disableTransmit() {
    uint8_t obuf[4] = {0x26, 0x08, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);

}
uint32_t isBusy() {
  uint8_t ibuf[1+CMD_EXTRA_BYTES];
  uint8_t obuf[4] = {0xF0, 0x00, 0x00, 0x00};
  cmdxfer(obuf, 4, ibuf, 1);
  return ((ibuf[gCmdExBytes] & 0x80) ? 1 : 0); 
}

uint32_t waitBusy() {
    uint32_t waitCnt = 0;
    while (isBusy()) {
	    usleep(5);
        waitCnt += 1;
    }
    return (waitCnt);
}

uint32_t resetConfigAddress() {
    uint8_t obuf[4] = {0x46, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t resetUFMAddress() {
    uint8_t obuf[4] = {0x47, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t setConfigAddress(uint32_t page) {
    uint8_t obuf[8];
    obuf[0] = 0xB4;
    obuf[1] = 0x00;
    obuf[2] = 0x00;
    obuf[3] = 0x00;
    obuf[4] = 0x00;
    obuf[5] = 0x00;
    obuf[6] = (page >> 8) & 0xFF;
    obuf[7] = (page)&0xFF;
    return cmdxfer(obuf, 8, NULL, 0);
}

uint32_t setUFMAddress(uint32_t page) {
    uint8_t obuf[8];
    obuf[0] = 0xB4;
    obuf[1] = 0x00;
    obuf[2] = 0x00;
    obuf[3] = 0x00;
    obuf[4] = 0x40;
    obuf[5] = 0x00;
    obuf[6] = (page >> 8) & 0xFF;
    obuf[7] = (page) & 0xFF;
    return cmdxfer(obuf, 8, NULL, 0);
}

uint32_t programPage(uint8_t *obuf) {
    cmdxfer(obuf, 20, NULL, 0);
    waitBusy(); // a 200us delay is also acceptable, should not be needed with I2C
    return 0;
}
uint32_t programDone() {
    uint8_t obuf[4] = {0x5E, 0x00, 0x00, 0x00};
    return cmdxfer(obuf, 4, NULL, 0);
}

uint32_t refresh() {
    uint8_t obuf[3] = {0x79, 0x00, 0x00};
    return cmdxfer(obuf, 3, NULL, 0); 
}

uint32_t wakeup() {
    uint8_t obuf[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    return cmdxfer(obuf, 4, NULL, 0);
}


static int lattice_read_page(unsigned char *buf)
{
    int ret = CLX_SUCCESS;
    uint8_t obuf[4] = {0x73, 0x00, 0x00, 0x01};
    ret = cmdxfer(obuf, 4, buf, PAGE_SIZE);
    waitBusy(); // a 300ns delay is also acceptable, should not be needed with I2C	 
    return ret;
}
static int lattice_program_page(unsigned char *buf)
{
    unsigned char pageBuf[4+PAGE_SIZE] = {0x70,0x00,0x0,0x01};	 
    (void)memcpy(&pageBuf[4], buf, PAGE_SIZE);	 
    return programPage(pageBuf);
}
static int lattice_erase_flash()
{
    int ret = CLX_SUCCESS;
    unsigned char rxbuf[4+CMD_EXTRA_BYTES] ={0};
	int status = 0;	 
    ret = erase();
	if(ret)
	      return CLX_FAILURE;
    waitBusy();
    ret = readStatus(rxbuf);
    if(CLX_SUCCESS == ret) {
        status = (rxbuf[gCmdExBytes] <<24)|(rxbuf[gCmdExBytes+1] <<16)|(rxbuf[gCmdExBytes+2] <<8)|rxbuf[gCmdExBytes+3];
        if(0== (status&STATUS_BUSY_FLAG))
            return CLX_SUCCESS; 
    } 
    return ret;
}
static int lattice_wait_program_done()
{
    unsigned char rxbuf[4+CMD_EXTRA_BYTES] = {0};
    unsigned int status = 0;	 
    //int retry = 3;	 

    CHECK_RC(programDone());
    usleep(200);	

    CHECK_RC(readStatus(rxbuf));
    status = (rxbuf[gCmdExBytes] <<24)|(rxbuf[gCmdExBytes+1] <<16)|(rxbuf[gCmdExBytes+2] <<8)|rxbuf[gCmdExBytes+3];
      /*done*/
    if(status&STATUS_FLASH_DONE_FLAG){
        printf("CPLD update is successfull!\n");
        return 0; /*succeed*/
        #if 0
        do{	 
	        ret = refresh();
	        if(ret)
		 	    continue;
	        usleep(100*1000);
	        CHECK_RC(readStatus(rxbuf));
            status = (rxbuf[0] <<24)|(rxbuf[1] <<16)|(rxbuf[2] <<8)|rxbuf[3];
	
	        if((0== (status&STATUS_BUSY_FLAG))&&(status&STATUS_FLASH_DONE_FLAG)&&(0 == (status&STATUS_CONFIG_CHECK))){
                printf("CPLD update is successfull!\n");
                return 0; /*succeed*/
	        }else{
                printf("refresh cpld status 0x%x error\n",status);
	        }
        }while(retry--);
        #endif
    }
	/*clean up*/
    CHECK_RC(lattice_erase_flash());
    CHECK_RC(refresh());
    return CLX_FAILURE;
}

static int lattice_reset_flash_addr()
{
    int ret = CLX_SUCCESS; 
    ret =  resetConfigAddress();
    if (ret < 0) {
        printf("resetConfigAddress\n");
        return ret;
    }
    waitBusy();
    return ret;
}

static int lattice_config_tranparentmode()
{
    int ret = CLX_SUCCESS;
    ret =  enableConfigTransparent();
	if(ret)
	    return CLX_FAILURE;
    usleep(5);
    return isBusy();
}

static int lattice_check_device_id()
{
    int ret = CLX_SUCCESS;
    uint8_t rxbuf[4+CMD_EXTRA_BYTES] ={0};
    uint32_t device_id = 0;	 

    ret = readDeviceID(rxbuf);
    device_id = (rxbuf[gCmdExBytes] <<24)|(rxbuf[gCmdExBytes+1] <<16)|(rxbuf[gCmdExBytes+2] <<8)|rxbuf[gCmdExBytes+3];
    if((DEVICE_ID_6900 != device_id)&&((DEVICE_ID_1300 != device_id)))
    {
        printf("device is is %x not MachXO3LF-6900 or MachXO3LF-1300\n",device_id); 
	    return CLX_DEVICE_NOT_FOUND;	
    }
    return ret;
}

int lattice_program_cpld_image(uint8_t *image, size_t size,unsigned char update_cpld_golden_flag)
{
    int i = 0;
    int page_num = 0;
    int Index = 0;
    unsigned char *temp_buf = NULL; 
    //0, Check image size
    if(size > MAX_CPLD_IMAGE_SIZE){
        printf("Image size(0x%lx) is lager than 0x%x\n", size, MAX_CPLD_IMAGE_SIZE); 
        return -1;
    }
   
	//1,Check Device Id
    CHECK_RC(lattice_check_device_id());

	//2,Set Transparent mode
	//printf("Set tranparent mode\n");
	CHECK_RC(lattice_config_tranparentmode());
     
    //3. Erase flash
    printf("Erase flash......\n"); 
    CHECK_RC(lattice_erase_flash());  
    
    //4. Reset flash address
    //printf("\nreset flash address\n");
    CHECK_RC(lattice_reset_flash_addr());
     
    //5. Program flash
    printf("\nProgram flash......\n"); 
    page_num = size / PAGE_SIZE + ((size % PAGE_SIZE) ? 1 : 0);
    for(i = 0; i < page_num; i++){
        if(!(i%0x10))
            printf("Programing 0x%x\r", i*PAGE_SIZE); 
        CHECK_RC(lattice_program_page(image + i*PAGE_SIZE));
    }   

   //6. Program  flash done 
    printf("\nProgram flash done\n"); 
   
    CHECK_RC(lattice_wait_program_done());
   
    //7. Read back and check data
    printf("\nVerify flash......\n"); 
    temp_buf = malloc(PAGE_SIZE+CMD_EXTRA_BYTES);  
    if(!temp_buf){
        printf("ERROR: failed to alloc memory buffer\n");
        FATAL;
    }
    memset(temp_buf,0,PAGE_SIZE+CMD_EXTRA_BYTES);
    CHECK_RC(lattice_reset_flash_addr());
    for(i = 0; i < page_num; i++){
        if(!(i%0x10))
            printf("Verifying 0x%x\r",i*PAGE_SIZE); 
        CHECK_RC(lattice_read_page(temp_buf));
        for(Index = 0; Index < PAGE_SIZE; Index++) {
            if(temp_buf[Index+gCmdExBytes] != *(image + (i*PAGE_SIZE) + Index)) {
                printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        (i*PAGE_SIZE) + Index,
                        *(image + (i*PAGE_SIZE) + Index),
                        temp_buf[Index+gCmdExBytes]);
                free(temp_buf);         
                return CLX_FAILURE;
            }
        }
    }
    free(temp_buf);  
    //8. disable Transmit
    CHECK_RC(disableTransmit());
	return CLX_SUCCESS;
}

int lattice_verify_cpld_image(uint8_t *image, size_t size,uint32_t loops)
{
    int i = 0,loop = 0,index = 0,page_num = 0;
    unsigned char *temp_buf = NULL;
	//Set Transparent mode
    CHECK_RC(lattice_config_tranparentmode()); 
    page_num = size / PAGE_SIZE + ((size % PAGE_SIZE) ? 1 : 0);
    temp_buf = malloc(PAGE_SIZE+CMD_EXTRA_BYTES);  
    if(!temp_buf){
        printf("ERROR: failed to alloc memory buffer\n");
        FATAL;
    }
    memset(temp_buf,0,PAGE_SIZE+CMD_EXTRA_BYTES);
    for(loop=0;loop < loops;loop++){
        printf("=========loop=%d=========\n",loop);
        CHECK_RC(lattice_reset_flash_addr());
        for(i = 0; i < page_num; i++){
            if(!(i%0x10))
                printf("Verifying 0x%x\r",i*PAGE_SIZE); 
            CHECK_RC(lattice_read_page(temp_buf));
            for(index = 0; index < PAGE_SIZE; index++) {
                if(temp_buf[index+gCmdExBytes] != *(image + (i*PAGE_SIZE) + index)) {
                    printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        (i*PAGE_SIZE) + index,
                        *(image + (i*PAGE_SIZE) + index),
                        temp_buf[index+gCmdExBytes]);
                    free(temp_buf);         
                    return CLX_FAILURE;
                }
            }
        }
    }
    free(temp_buf);  
     
    // disable Transmit
    CHECK_RC(disableTransmit());
    return CLX_SUCCESS;
}
int lattice_cpld_update_init(uint32_t index,void **cpld_op)
{
    int i;
    lattice_cpld_update_op.program_image = lattice_program_cpld_image;
    lattice_cpld_update_op.verify_image = lattice_verify_cpld_image;
    lattice_cpld_update_op.page_size = PAGE_SIZE;
    *cpld_op = &lattice_cpld_update_op;
    for(i= 0; i < sizeof(lattice_cpld_update_map)/sizeof(lattice_cpld_update_map[0]); i++)
    {
	    if(index == lattice_cpld_update_map[i].idx )
	    {   
            gCmdExBytes = lattice_cpld_update_map[i].g_extra_bytes;
            lattice_cpld_update_op.cmdxfer = lattice_cpld_update_map[i].cmdxfer;
            if(NULL!= lattice_cpld_update_map[i].init)
		        return lattice_cpld_update_map[i].init(index);
	    }
    }
    return CLX_FAILURE;
}
void lattice_cpld_update_exit(uint32_t index)
{
    int i;
    for(i= 0; i < sizeof(lattice_cpld_update_map)/sizeof(lattice_cpld_update_map[0]); i++)
    {
	    if(i == lattice_cpld_update_map[i].idx)
	    {
            if(NULL!= lattice_cpld_update_map[i].exit)
		        lattice_cpld_update_map[i].exit(index);
	    }
    }
}
