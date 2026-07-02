#include "fpga_pci.h"
#include "fpga_pci_xilinx.h"
#include "driver/xilinx/xspi.h"

/************************** Constant Definitions *****************************/

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define SPI_DEVICE_ID		XPAR_SPI_0_DEVICE_ID

/*
 *  This is the size of the buffer to be transmitted/received in this example.
 */
#define BUFFER_SIZE		12

/*
 * The following constant defines the slave select signal that is used to
 * to select the Flash device on the SPI bus, this signal is typically
 * connected to the chip select of the device.
 */
#define SPI_SELECT 0x01
#define UPDATE_ADDR 0x400000
#define MAX_IMAGE_SIZE 0x400000
#define GOLDEN_ADDR    0x00

/*
 * Definitions of the commands shown in this example.
 */
#define COMMAND_RANDOM_READ		0x03 /* Random read command */
#define COMMAND_PAGEPROGRAM_WRITE	0x02 /* Page Program command */
#define	COMMAND_WRITE_ENABLE	0x06 /* Write Enable command */
#define COMMAND_SECTOR_ERASE	0xD8 /* Sector Erase command */
#define COMMAND_BULK_ERASE		0xC7 /* Bulk Erase command */
#define COMMAND_STATUSREG_READ	0x05 /* Status read command */
#define COMMAND_ID_READ         0x9F /* ID read command */


/**
 * This definitions specify the EXTRA bytes in each of the command
 * transactions. This count includes Command byte, address bytes and any
 * don't care bytes needed.
 */
#define READ_WRITE_EXTRA_BYTES	4 /* Read/Write extra bytes */
#define	WRITE_ENABLE_BYTES		1 /* Write Enable bytes */
#define SECTOR_ERASE_BYTES		4 /* Sector erase extra bytes */
#define BULK_ERASE_BYTES		1 /* Bulk erase extra bytes */
#define STATUS_READ_BYTES		2 /* Status read bytes count */
#define STATUS_WRITE_BYTES		2 /* Status write bytes count */
#define ID_READ_BYTES           10 /* ID read bytes count */
#define SECTOR_SIZE             0x10000

/*
 * Flash not busy mask in the status register of the flash device.
 */
#define FLASH_SR_IS_READY_MASK 0x01 /* Ready mask */

/*
 * Number of bytes per page in the flash device.
 */
#define PAGE_SIZE		256

/*
 * Address of the page to perform Erase, Write and Read operations.
 */
#define FLASH_TEST_ADDRESS	0x300000

/*
 * Byte offset value written to Flash. This needs to redefined for writing
 * different patterns of data to the Flash device.
 */
#define FLASH_TEST_BYTE	0x20

/*
 * Byte Positions.
 */
#define BYTE1				0 /* Byte 1 position */
#define BYTE2				1 /* Byte 2 position */
#define BYTE3				2 /* Byte 3 position */
#define BYTE4				3 /* Byte 4 position */
#define BYTE5				4 /* Byte 5 position */

#define DUMMYBYTE			0xFF /* Dummy byte */

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/

int SpiFlashWriteEnable(XSpi *SpiPtr);
int SpiFlashWrite(XSpi *SpiPtr, u32 Addr, uint8_t *buf, u32 ByteCount);
int SpiFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount);
int SpiFlashBulkErase(XSpi *SpiPtr);
int SpiFlashSectorErase(XSpi *SpiPtr, u32 Addr);
int SpiFlashGetStatus(XSpi *SpiPtr);
static int SpiFlashWaitForFlashNotBusy(XSpi *SpiPtr);
int SpiFlashGetID(XSpi *SpiPtr);
int read_write_flash(XSpi *SpiInstancePtr, u16 SpiDeviceId);

/************************** Variable Definitions *****************************/

/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/

int SpiPolledExample(XSpi *SpiInstancePtr, u16 SpiDeviceId);

/************************** Variable Definitions *****************************/

/*
 * The instances to support the device drivers are global such that the
 * are initialized to zero each time the program runs.
 */
static XSpi  Spi;	 /* The instance of the SPI device */

/*
 * The following variables are used to read and write to the  Spi device, they
 * are global to avoid having large buffers on the stack.
 */
u8 ReadBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES];
static u8 WriteBuffer[PAGE_SIZE + READ_WRITE_EXTRA_BYTES];


#define DGB() xil_printf("%s(%d)\n", __FUNCTION__, __LINE__);
/*****************************************************************************/
/**
*
* Main function to call the Spi Polled example.
*
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int spi_init(void)
{
	int Status;

	/*
	 * Run the Spi Polled example.
	 */
	Status = read_write_flash(&Spi, SPI_DEVICE_ID);
	if (Status != XST_SUCCESS) {
		xil_printf("Spi self-test Failed\r\n");
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/************************** Function Definitions ******************************/


/*****************************************************************************/
/**
*
* This function enables writes to the STM Serial Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWriteEnable(XSpi *SpiPtr)
{
	int Status;
	
    /*
	 * Wait till the Flash is not Busy.
	 */
	Status = SpiFlashWaitForFlashNotBusy(SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_WRITE_ENABLE;

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				WRITE_ENABLE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function writes the data to the specified locations in the STM Serial
* Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address in the Buffer, where to write the data.
* @param	ByteCount is the number of bytes to be written.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashWrite(XSpi *SpiPtr, u32 Addr, uint8_t *buf, u32 ByteCount)
{
	u32 Index;
	int Status;

    /*
	 * Wait till the Flash is not Busy.
	 */
	Status = SpiFlashWaitForFlashNotBusy(SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_PAGEPROGRAM_WRITE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;

	for(Index = 4; Index < ByteCount + READ_WRITE_EXTRA_BYTES;
								Index++) {
		WriteBuffer[Index] = *(buf + (Index - 4));
	}

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the data from the STM Serial Flash Memory
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the starting address in the Flash Memory from which the
*		data is to be read.
* @param	ByteCount is the number of bytes to be read.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount)
{
	int Status;

    /*
	 * Wait till the Flash is not Busy.
	 */
	Status = SpiFlashWaitForFlashNotBusy(SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_RANDOM_READ;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer( SpiPtr, WriteBuffer, ReadBuffer,
				(ByteCount + READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the entire contents of the STM Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will read as 0xFF.
*
******************************************************************************/
int SpiFlashBulkErase(XSpi *SpiPtr)
{
	int Status;

    /*
	 * Wait till the Flash is not Busy.
	 */
	Status = SpiFlashWaitForFlashNotBusy(SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_BULK_ERASE;

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
						BULK_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the contents of the specified Sector in the STM Serial
* Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address within a sector of the Buffer, which is to
*		be erased.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will be read back as 0xFF.
*
******************************************************************************/
int SpiFlashSectorErase(XSpi *SpiPtr, u32 Addr)
{
	int Status;

    /*
	 * Wait till the Flash is not Busy.
	 */
	Status = SpiFlashWaitForFlashNotBusy(SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_SECTOR_ERASE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) (Addr);

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
					SECTOR_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the Status register of the STM Flash.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The status register content is stored at the second byte pointed
*		by the ReadBuffer.
*
******************************************************************************/
int SpiFlashGetStatus(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_STATUSREG_READ;

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						STATUS_READ_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int SpiFlashGetID(XSpi *SpiPtr)
{
	int Status;
    int i = 0;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = COMMAND_ID_READ;

	/*
	 * Initiate the Transfer.
	 */
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						ID_READ_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

    xil_printf("Flash ID: ");
    for(i = 0; i < ID_READ_BYTES; i++)
    {
		xil_printf("%x\t", ReadBuffer[i]);
    }
	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function waits till the serial Flash is ready to accept next command.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		This function reads the status register of the Buffer and waits
*.		till the WIP bit of the status register becomes 0.
*
******************************************************************************/
int SpiFlashWaitForFlashNotBusy(XSpi *SpiPtr)
{
	int Status;
	u8 StatusReg;

	while(1) {

		/*
		 * Get the Status Register.
		 */
		Status = SpiFlashGetStatus(SpiPtr);
		if(Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		/*
		 * Check if the flash is ready to accept the next command.
		 * If so break.
		 */
		StatusReg = ReadBuffer[1];
		if((StatusReg & FLASH_SR_IS_READY_MASK) == 0) {
			break;
		}
	}

	return XST_SUCCESS;
}


int xilinx_program_fpga_image(uint8_t *image, size_t size, uint32_t region,uint8_t erase_flag)
{
    uint32_t addr = region ? UPDATE_ADDR :  GOLDEN_ADDR;
    int i = 0;
    int sector_num = 0;
    int page_num = 0;
    int max_page_num = 0;
    int Index = 0;

    //0, Check image size
    if(size > MAX_IMAGE_SIZE){
        xil_printf("Image size(0x%lx) is lager than 0x%x\n", size, MAX_IMAGE_SIZE); 
        return -1;
    }
    //1. Erase flash
    xil_printf("Erase flash......\n"); 
    //sector_num = (size / SECTOR_SIZE) + ((size % SECTOR_SIZE) ? 1 : 0);
    sector_num = MAX_IMAGE_SIZE / SECTOR_SIZE;

    if(0 == erase_flag)
	    sector_num = sector_num-1; /*reserved last sector*/
    for(i = 0; i < sector_num; i++)
    {
        xil_printf("Erasing 0x%x\r", addr + i*SECTOR_SIZE); 
        CHECK_RC(SpiFlashWriteEnable(&Spi));
        CHECK_RC(SpiFlashSectorErase(&Spi, addr + i*SECTOR_SIZE));
    }

    //2. Program flash
    xil_printf("\nProgram flash......\n"); 
    page_num = size / PAGE_SIZE + ((size % PAGE_SIZE) ? 1 : 0);
    if(0 == erase_flag)
    {   
        max_page_num = (MAX_IMAGE_SIZE - SECTOR_SIZE)/PAGE_SIZE;
        page_num =  page_num < max_page_num  ? page_num : max_page_num;;
    }
    for(i = 0; i < page_num; i++)
    {
        if(!(i%0x10))
            xil_printf("Programing 0x%x\r", addr + i*PAGE_SIZE); 
        CHECK_RC(SpiFlashWriteEnable(&Spi));
        CHECK_RC(SpiFlashWrite(&Spi, addr + i*PAGE_SIZE, 
                    image + i*PAGE_SIZE, 
                    ((size - (i + 1)*PAGE_SIZE) >= 0) ? PAGE_SIZE : (size - (i*PAGE_SIZE))));
    }

    //3. Read back and check data
    xil_printf("\nVerify flash\n"); 
    //page_num = size / PAGE_SIZE + ((size % PAGE_SIZE) ? 1 : 0);
    for(i = 0; i < page_num; i++)
    {
        if(!(i%0x10))
            xil_printf("Verifying 0x%x\r", addr + i*PAGE_SIZE); 
        CHECK_RC(SpiFlashRead(&Spi, addr + i*PAGE_SIZE, 
                ((size - (i + 1)*PAGE_SIZE) >= 0) ? PAGE_SIZE : (size - (i*PAGE_SIZE))));
        for(Index = 0; Index < (((size - (i + 1)*PAGE_SIZE) >= 0) ? PAGE_SIZE : (size - (i*PAGE_SIZE))); Index++) {
            if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES] !=
                    *(image + (i*PAGE_SIZE) + Index)) {
                xil_printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        addr + (i*PAGE_SIZE) + Index,
                        *(image + (i*PAGE_SIZE) + Index),
                        ReadBuffer[Index + READ_WRITE_EXTRA_BYTES]);
                return XST_FAILURE;
            }
        }
    }
 
	return XST_SUCCESS;
}
int xilinx_verify_fpga_image(uint8_t *image, size_t size,uint32_t region,uint32_t loops)
{
    uint32_t addr = region ? UPDATE_ADDR :  GOLDEN_ADDR;
    int i = 0,loop =0;
    int page_num = 0;
    int max_page_num = 0;
    int Index = 0;

	page_num = size / PAGE_SIZE + ((size % PAGE_SIZE) ? 1 : 0);
	max_page_num = (MAX_IMAGE_SIZE - SECTOR_SIZE)/PAGE_SIZE;
	page_num =  page_num < max_page_num  ? page_num : max_page_num;

    for(loop=0;loop < loops;loop++){
        printf("=========loop=%d=========\n",loop+1);
        for(i = 0; i < page_num; i++)
        {
            if(!(i%0x10))
                printf("Verifying 0x%x\r", addr + i*PAGE_SIZE); 
            CHECK_RC(SpiFlashRead(&Spi, addr + i*PAGE_SIZE, 
                ((size - (i + 1)*PAGE_SIZE) >= 0) ? PAGE_SIZE : (size - (i*PAGE_SIZE))));
            for(Index = 0; Index < (((size - (i + 1)*PAGE_SIZE) >= 0) ? PAGE_SIZE : (size - (i*PAGE_SIZE))); Index++) {
                if(ReadBuffer[Index + READ_WRITE_EXTRA_BYTES] !=
                    *(image + (i*PAGE_SIZE) + Index)) {
                    printf("Error: offset 0x%x, exp 0x%x, act 0x%x\n", 
                        addr + (i*PAGE_SIZE) + Index,
                        *(image + (i*PAGE_SIZE) + Index),
                        ReadBuffer[Index + READ_WRITE_EXTRA_BYTES]);
                    return XST_FAILURE;
                }
			}
        }
	}
    return CLX_SUCCESS;
}
static struct fpga_fn_if xilinx_fpga_update_op;
int xilinx_fpga_update_init(uint32_t region, void **fpga_op)
{
	CHECK_RC(spi_init());
    xilinx_fpga_update_op.program_image = xilinx_program_fpga_image;
    xilinx_fpga_update_op.verify_image = xilinx_verify_fpga_image;
	xilinx_fpga_update_op.page_size = PAGE_SIZE;
    *fpga_op = &xilinx_fpga_update_op;   
    return CLX_SUCCESS;
}
void xilinx_fpga_update_exit(uint32_t region)
{
    return;
}
