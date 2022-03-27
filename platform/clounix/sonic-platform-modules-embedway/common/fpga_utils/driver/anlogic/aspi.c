#include "aspi.h"
#include "aspi_l.h"
#define step printf("==========%s:%d==============\n", __FUNCTION__, __LINE__);
/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/


/*****************************************************************************/
/**
*
* Initializes a specific ASpi instance such that the driver is ready to use.
*
* The state of the device after initialization is:
*	- Device is disabled
*	- Slave mode
*	- Active high clock polarity
*	- Clock phase 0
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Config is a reference to a structure containing information
*		about a specific SPI device. This function initializes an
*		InstancePtr object for a specific device specified by the
*		contents of Config. This function can initialize multiple
*		instance objects with the use of multiple calls giving
		different Config information on each call.
* @param	EffectiveAddr is the device base address in the virtual memory
*		address space. The caller is responsible for keeping the
*		address mapping from EffectiveAddr to the device physical base
*		address unchanged once this function is invoked. Unexpected
*		errors may occur if the address mapping changes after this
*		function is called. If address translation is not used, use
*		Config->BaseAddress for this parameters, passing the physical
*		address instead.
*
* @return
*		- ANL_SUCCESS if successful.
*		- ANL_DEVICE_IS_STARTED if the device is started. It must be
*		  stopped to re-initialize.
*
* @note		None.
*
******************************************************************************/
int ASpi_CfgInitialize(ASpi *InstancePtr, ASpi_Config *Config,
			UINTPTR EffectiveAddr)
{

	
	Clx_AssertNonvoid(InstancePtr != NULL);

	/*
	 * If the device is started, disallow the initialize and return a status
	 * indicating it is started.  This allows the user to stop the device
	 * and reinitialize, but prevents a user from inadvertently
	 * initializing.
	 */
	if (InstancePtr->IsStarted == ANL_COMPONENT_IS_STARTED) {
		return ANL_DEVICE_IS_STARTED;
	}

	/*
	 * Set some default values.
	 */
	InstancePtr->IsStarted = 0;
	InstancePtr->IsBusy = FALSE;

	InstancePtr->SendBufferPtr = NULL;
	InstancePtr->RecvBufferPtr = NULL;
	InstancePtr->RequestedBytes = 0;
	InstancePtr->RemainingBytes = 0;
	InstancePtr->BaseAddr = EffectiveAddr;
	InstancePtr->HasFifos = Config->HasFifos;
	InstancePtr->FifosDepth = Config->FifosDepth;
	InstancePtr->SlaveOnly = Config->SlaveOnly;
	InstancePtr->NumSlaveBits = Config->NumSlaveBits;
	if (Config->DataWidth == 0) {
		InstancePtr->DataWidth = 8;
	} else {
		InstancePtr->DataWidth = Config->DataWidth;
	}

	InstancePtr->SpiMode = Config->SpiMode;

	InstancePtr->FlashBaseAddr = Config->AxiFullBaseAddress;
	InstancePtr->XipMode = Config->XipMode;

	InstancePtr->IsReady = ANL_COMPONENT_IS_READY;


	/*
	 * Clear the statistics for this driver.
	 */
	InstancePtr->Stats.ModeFaults = 0;
	InstancePtr->Stats.XmitUnderruns = 0;
	InstancePtr->Stats.RecvOverruns = 0;
	InstancePtr->Stats.SlaveModeFaults = 0;
	InstancePtr->Stats.BytesTransferred = 0;
	InstancePtr->Stats.NumInterrupts = 0;
	
	/*
	 * Reset the SPI device to get it into its initial state. It is expected
	 * that device configuration will take place after this initialization
	 * is done, but before the device is started.
	 */
	ASpi_Reset(InstancePtr);

    ASpi_Start(InstancePtr);

	return CLX_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables interrupts for the SPI device. If the Spi driver is used
* in interrupt mode, it is up to the user to connect the SPI interrupt handler
* to the interrupt controller before this function is called. If the Spi driver
* is used in polled mode the user has to disable the Global Interrupts after
* this function is called. If the device is configured with FIFOs, the FIFOs are
* reset at this time.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return
*		- ANL_SUCCESS if the device is successfully started
*		- ANL_DEVICE_IS_STARTED if the device was already started.
*
* @note		None.
*
******************************************************************************/
int ASpi_Start(ASpi *InstancePtr)
{
	u32 ConfigReg;

	Clx_AssertNonvoid(InstancePtr != NULL);
	Clx_AssertNonvoid(InstancePtr->IsReady == ANL_COMPONENT_IS_READY);

	/*
	 * If it is already started, return a status indicating so.
	 */
	if (InstancePtr->IsStarted == ANL_COMPONENT_IS_STARTED) {
		return ANL_DEVICE_IS_STARTED;
	}

	
	/*
	 * Indicate that the device is started before we enable the transmitter
	 * or receiver or interrupts.
	 */
	InstancePtr->IsStarted = ANL_COMPONENT_IS_STARTED;

	/*
	 * Reset the transmit and receive FIFOs if present. There is a critical
	 * section here since this register is also modified during interrupt
	 * context. So we wait until after the r/m/w of the control register to
	 * enable the Global Interrupt Enable.
	 */
	ConfigReg = ASpi_GetConfigReg(InstancePtr);
	ConfigReg |= ASP_BIT(CFG_ENA);
	ASpi_SetConfigReg(InstancePtr, ConfigReg);

	return CLX_SUCCESS;
}

int ASpi_Stop(ASpi *InstancePtr)
{
	u32 ConfigReg = 0;

	Clx_AssertNonvoid(InstancePtr != NULL);
	Clx_AssertNonvoid(InstancePtr->IsReady == ANL_COMPONENT_IS_READY);
	/*
	 * Do not allow the user to stop the device while a transfer is in
	 * progress.
	 */
	if (InstancePtr->IsBusy) {
		return ANL_DEVICE_BUSY;
	}
	/*
	 * Disable the device. First disable the interrupts since there is
	 * a critical section here because this register is also modified during
	 * interrupt context. The device is likely disabled already since there
	 * is no transfer in progress, but we do it again just to be sure.
	 */

	ConfigReg = ASpi_GetConfigReg(InstancePtr);
	ASpi_SetConfigReg(InstancePtr, ConfigReg & ~ASP_BIT(CFG_ENA));

	InstancePtr->IsStarted = 0;

	return CLX_SUCCESS;
}

/** @} */
void ASpi_Reset(ASpi *InstancePtr)
{
	Clx_AssertVoid(InstancePtr != NULL);
	Clx_AssertVoid(InstancePtr->IsReady == ANL_COMPONENT_IS_READY);

	/*
	 * Reset any values that are not reset by the hardware reset such that
	 * the software state matches the hardware device.
	 */
	InstancePtr->IsStarted = 0;

	/*
	 * Reset the device.
	 */
	ASpi_WriteReg(InstancePtr->BaseAddr, ASP_CFG_OFFSET,
			ASP_BIT(CFG_RST));	
	ASpi_WriteReg(InstancePtr->BaseAddr, ASP_CFG_OFFSET,
			ASP_BF(CFG_RST,0));	
}
void ASpi_SetCS_CLK(ASpi *InstancePtr,unsigned char cs,unsigned char clk)
{
	u32 MuxReg = 0;
	Clx_AssertVoid(InstancePtr != NULL);
	Clx_AssertVoid(InstancePtr->IsReady == ANL_COMPONENT_IS_READY);

	/*
	 * Reset any values that are not reset by the hardware reset such that
	 * the software state matches the hardware device.
	 */
	MuxReg = ASpi_ReadReg(InstancePtr->BaseAddr, ASP_MUX_OFFSET);
	MuxReg = ASP_BFINS(MUX_CS,cs,MuxReg);
	MuxReg = ASP_BFINS(MUX_CLK,clk,MuxReg);

	ASpi_WriteReg(InstancePtr->BaseAddr, ASP_MUX_OFFSET, MuxReg);	
}
int ASpi_Erase(ASpi *InstancePtr,unsigned int flash_addr)
{
	u32 ConfigReg = 0, StatusReg = 0;
	u32 cnt = 0;

	ASpi_SetAddrReg(InstancePtr,ASP_BF(FLASH_ADDR,flash_addr));
	ConfigReg = ASpi_GetConfigReg(InstancePtr);
	ConfigReg = ASP_BFINS(CFG_FLASH_CMD,ERASE_FLASH,ConfigReg);
	ConfigReg = ASP_BFINS(CFG_ERASE_TYPE,BLOCK_ERASE,ConfigReg);
    ASpi_SetConfigReg(InstancePtr,ConfigReg);
	//usleep(300);
	
	StatusReg = ASpi_GetStatusReg(InstancePtr);    /*flash_busy==0 &&  flash_error==0*/
	while(ASP_BFEXT(SR_BUSY,StatusReg))
	{
		//printf("busy cnt=%d\n",cnt); 
		//usleep(100);
		cnt++;
	    StatusReg = ASpi_GetStatusReg(InstancePtr);
	}
	if(ASP_BFEXT(SR_FLASH_ERROR,StatusReg))
	{
        printf("ASpi_Erase error!!!\n");
		return CLX_FAILURE;
	}    
	else
	    return CLX_SUCCESS;
}
int ASpi_Write(ASpi *InstancePtr,unsigned int flash_addr,u8 data)
{
	u32 ConfigReg = 0,StatusReg= 0;
	u32 cnt = 0;
	ASpi_SetAddrReg(InstancePtr,ASP_BF(FLASH_ADDR,flash_addr));
	ASpi_SetDTRReg(InstancePtr,data);
	ConfigReg = ASpi_GetConfigReg(InstancePtr);
	ConfigReg = ASP_BFINS(CFG_FLASH_CMD,WRITE_FLASH,ConfigReg);
    ASpi_SetConfigReg(InstancePtr,ConfigReg);
	//usleep(10);
	StatusReg = ASpi_GetStatusReg(InstancePtr);    /*flash_busy==0 &&  flash_error==0*/
	while(ASP_BFEXT(SR_BUSY,StatusReg))
	{  	
		//printf("busy cnt=%d\n",cnt); 
	//	usleep(20);
		StatusReg = ASpi_GetStatusReg(InstancePtr);
		cnt++;
	}
	if(ASP_BFEXT(SR_FLASH_ERROR,StatusReg))
	{
		printf("ASpi_Write error!!!\n");
	    return CLX_FAILURE;
	}
	else
	    return CLX_SUCCESS;
}
int ASpi_Read(ASpi *InstancePtr,unsigned int flash_addr,u8 *pdata)
{
	u32 ConfigReg = 0, StatusReg = 0;
	u32 cnt = 0;

	ASpi_SetAddrReg(InstancePtr,ASP_BF(FLASH_ADDR,flash_addr));
	
	ConfigReg = ASpi_GetConfigReg(InstancePtr);
	ConfigReg = ASP_BFINS(CFG_FLASH_CMD,READ_FLASH,ConfigReg);
    ASpi_SetConfigReg(InstancePtr,ConfigReg);
	//usleep(1);
	StatusReg = ASpi_GetStatusReg(InstancePtr);    /*flash_busy==0 &&  flash_error==0*/
	while(ASP_BFEXT(SR_BUSY,StatusReg))
	{  
		//printf("busy cnt=%d\n",cnt); 
	//	usleep(20);
		StatusReg = ASpi_GetStatusReg(InstancePtr);
		cnt++;
	}
	if(ASP_BFEXT(SR_FLASH_ERROR,StatusReg))
	{   
		printf("ASpi_Read error!!!\n");
		return CLX_FAILURE;
	}
	else
	{
		*pdata = ASpi_GetDRRReg(InstancePtr);
	    return CLX_SUCCESS;
	}
}
int ASpi_Initialize(ASpi *InstancePtr, u16 DeviceId)
{
	ASpi_Config *ConfigPtr;	/* Pointer to Configuration ROM data */

	Clx_AssertNonvoid(InstancePtr != NULL);

	/*
	 * Lookup the device configuration in the temporary CROM table. Use this
	 * configuration info down below when initializing this component.
	 */
	ConfigPtr = ASpi_LookupConfig(DeviceId);
	if (ConfigPtr == NULL) {
		return ANL_DEVICE_NOT_FOUND;
	}

	return ASpi_CfgInitialize(InstancePtr, ConfigPtr,
				  ConfigPtr->BaseAddress);
}
