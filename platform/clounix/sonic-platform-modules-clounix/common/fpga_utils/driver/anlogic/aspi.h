
#ifndef ASPI_H			/* prevent circular inclusions */
#define ASPI_H			/* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

/***************************** Include Files *********************************/

#include "../../fpga_pci.h"
#include "../../fpga_pci_anlogic.h"
#include "aspi_l.h"

/**
 * ASpi statistics
 */
typedef struct {
	u32 ModeFaults;		/**< Number of mode fault errors */
	u32 XmitUnderruns;	/**< Number of transmit underruns */
	u32 RecvOverruns;	/**< Number of receive overruns */
	u32 SlaveModeFaults;	/**< Num of selects as slave while disabled */
	u32 BytesTransferred;	/**< Number of bytes transferred */
	u32 NumInterrupts;	/**< Number of transmit/receive interrupts */
} ASpi_Stats;

/**
 * This typedef contains configuration information for the device.
 */
typedef struct {
	u16 DeviceId;		/**< Unique ID  of device */
	UINTPTR BaseAddress;	/**< Base address of the device */
	int HasFifos;		/**< Does device have FIFOs? */
	u32 SlaveOnly;		/**< Is the device slave only? */
	u8 NumSlaveBits;	/**< Num of slave select bits on the device */
	u8 DataWidth;		/**< Data transfer Width */
	u8 SpiMode;		/**< Standard/Dual/Quad mode */
	u8 AxiInterface;	/**< AXI-Lite/AXI Full Interface */
	u32 AxiFullBaseAddress;	/**< AXI Full Interface Base address of
					the device */
	u8 XipMode;             /**< 0 if Non-XIP, 1 if XIP Mode */
	u8 Use_Startup;		/**< 1 if Starup block is used in h/w */
	u16 FifosDepth;		/**< TX and RX FIFO Depth */
} ASpi_Config;

/**
 * The ASpi driver instance data. The user is required to allocate a
 * variable of this type for every SPI device in the system. A pointer
 * to a variable of this type is then passed to the driver API functions.
 */
typedef struct {
	ASpi_Stats Stats;	/**< Statistics */
	UINTPTR BaseAddr;		/**< Base address of device (IPIF) */
	int IsReady;		/**< Device is initialized and ready */
	int IsStarted;		/**< Device has been started */
	int HasFifos;		/**< Device is configured with FIFOs or not */
	u32 SlaveOnly;		/**< Device is configured to be slave only */
	u8 NumSlaveBits;	/**< Number of slave selects for this device */
	u8 DataWidth;		/**< Data Transfer Width 8 or 16 or 32 */
	u8 SpiMode;		/**< Standard/Dual/Quad mode */
	u32 SlaveSelectValue;	/**< Value that matches the number of SS bits */
	u32 SlaveSelectReg;	/**< Slave select register */

	u8 *SendBufferPtr;	/**< Buffer to send  */
	u8 *RecvBufferPtr;	/**< Buffer to receive */
	unsigned int RequestedBytes; /**< Total bytes to transfer (state) */
	unsigned int RemainingBytes; /**< Bytes left to transfer (state) */
	int IsBusy;		/**< A transfer is in progress (state) */

	//ASpi_StatusHandler StatusHandler; /**< Status Handler */
	void *StatusRef;	/**< Callback reference for status handler */
	u32 FlashBaseAddr;    	/**< Used in XIP Mode */
	u8 XipMode;             /**< 0 if Non-XIP, 1 if XIP Mode */
	u16 FifosDepth;		/**< TX and RX FIFO Depth */
} ASpi;

/***************** Macros (Inline Functions) Definitions *********************/

/****************************************************************************/
/**
*
* Set the contents of the control register. Use the XSP_CR_* constants defined
* above to create the bit-Value to be written to the register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Value is the 32-bit value to write to the Config register.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_SetControlReg(ASpi *InstancePtr, u32 Value);
*
*****************************************************************************/
#define ASpi_SetConfigReg(InstancePtr, Value) \
	ASpi_WriteReg(((InstancePtr)->BaseAddr), ASP_CFG_OFFSET, (Value))

/****************************************************************************/
/**
*
* Get the contents of the control register. Use the XSP_CR_* constants defined
* above to interpret the bit-Value returned.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	A 32-bit value representing the contents of the control
*		register.
*
* @note		C-Style signature:
* 		u32 ASpi_GetControlReg(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_GetConfigReg(InstancePtr) \
	ASpi_ReadReg(((InstancePtr)->BaseAddr), ASP_CFG_OFFSET)

/***************************************************************************/
/**
*
* Get the contents of the status register. Use the XSP_SR_* constants defined
* above to interpret the bit-Value returned.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	An 32-bit value representing the contents of the status
*		register.
*
* @note		C-Style signature:
* 		u8 ASpi_GetStatusReg(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_GetStatusReg(InstancePtr) \
	ASpi_ReadReg(((InstancePtr)->BaseAddr), ASP_SR_OFFSET)

/****************************************************************************/
/**
*
* Set the contents of the XIP control register. Use the XSP_CR_XIP_* constants
* defined above to create the bit-Value to be written to the register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Value is the 32-bit value to write to the control register.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_SetControlReg(ASpi *InstancePtr, u32 vlaue);
*
*****************************************************************************/
#define ASpi_SetControlReg(InstancePtr, Vlaue) \
	ASpi_WriteReg(((InstancePtr)->BaseAddr), ASP_CTRL_OFFSET, (Vlaue))

/****************************************************************************/
/**
*
* Get the contents of the XIP control register. Use the XSP_CR_XIP_* constants
* defined above to interpret the bit-Value returned.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	A 32-bit value representing the contents of the control
*		register.
*
* @note		C-Style signature:
* 		u32 ASpi_GetControlReg(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_GetControlReg(InstancePtr) \
	ASpi_ReadReg(((InstancePtr)->BaseAddr), ASP_CTRL_OFFSET)

/****************************************************************************/
/**
*
* Set the contents of the XIP control register. Use the XSP_CR_XIP_* constants
* defined above to create the bit-Value to be written to the register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Value is the 32-bit value to write to the addr register.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_SetControlReg(ASpi *InstancePtr, u32 vlaue);
*
*****************************************************************************/
#define ASpi_SetAddrReg(InstancePtr, Vlaue) \
	ASpi_WriteReg(((InstancePtr)->BaseAddr), ASP_ADDR_OFFSET, (Vlaue))

/****************************************************************************/
/**
*
* Get the contents of the slave select register. Each bit in the Value
* corresponds to a slave select line. Only one slave should be selected at
* any one time.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	The 32-bit value in the addr register.
*
* @note		C-Style signature:
* 		u32 ASpi_GetSlaveSelectReg(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_GetAddressReg(InstancePtr) 			\
	ASpi_ReadReg((InstancePtr)->BaseAddr, ASP_ADDR_OFFSET)
/****************************************************************************/
/**
*
* Set the contents of the XIP control register. Use the XSP_CR_XIP_* constants
* defined above to create the bit-Value to be written to the register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Value is the 32-bit value to write to the addr register.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_SetDTRReg(ASpi *InstancePtr, u32 vlaue);
*
*****************************************************************************/
#define ASpi_SetDTRReg(InstancePtr, Vlaue) \
	ASpi_WriteReg(((InstancePtr)->BaseAddr), ASP_CTRL_OFFSET, ASP_BF(CTRL_DATA,Vlaue))
/****************************************************************************/
/**
*
* Set the contents of the XIP control register. Use the XSP_CR_XIP_* constants
* defined above to create the bit-Value to be written to the register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
* @param	Value is the 32-bit value to write to the addr register.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_GetDTRReg(ASpi *InstancePtr, u32 vlaue);
*
*****************************************************************************/
#define ASpi_GetDTRReg(InstancePtr) \
	ASP_BFEXT(CTRL_DATA,ASpi_ReadReg((InstancePtr)->BaseAddr, ASP_CTRL_OFFSET))

/****************************************************************************/
/**
*
* Get the contents of the slave select register. Each bit in the Value
* corresponds to a slave select line. Only one slave should be selected at
* any one time.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	The 32-bit value in the DRR register.
*
* @note		C-Style signature:
* 		u32 ASpi_GetSlaveSelectReg(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_GetDRRReg(InstancePtr) 			\
	ASpi_ReadReg((InstancePtr)->BaseAddr, ASP_DRR_OFFSET)

/****************************************************************************/
/**
*
* Enable the device and uninhibit master transactions. Preserves the current
* contents of the control register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_Enable(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_Enable(InstancePtr) \
{ \
	u16 Control; \
	Control = ASpi_GetControlReg((InstancePtr)); \
	Control |= XSP_CR_ENABLE_Value; \
	Control &= ~XSP_CR_TRANS_INHIBIT_Value; \
	ASpi_SetControlReg((InstancePtr), Control); \
}

/****************************************************************************/
/**
*
* Disable the device. Preserves the current contents of the control register.
*
* @param	InstancePtr is a pointer to the ASpi instance to be worked on.
*
* @return	None.
*
* @note		C-Style signature:
* 		void ASpi_Disable(ASpi *InstancePtr);
*
*****************************************************************************/
#define ASpi_Disable(InstancePtr) \
	ASpi_SetControlReg((InstancePtr), \
	ASpi_GetControlReg((InstancePtr)) & ~XSP_CR_ENABLE_Value)

/************************** Function Prototypes ******************************/

/*
 * Initialization functions in ASpi_sinit.c
 */
int ASpi_Initialize(ASpi *InstancePtr, u16 DeviceId);
ASpi_Config *ASpi_LookupConfig(u16 DeviceId);

/*
 * Functions, in ASpi.c
 */
int ASpi_CfgInitialize(ASpi *InstancePtr, ASpi_Config * Config,
		       UINTPTR EffectiveAddr);

int ASpi_Start(ASpi *InstancePtr);
int ASpi_Stop(ASpi *InstancePtr);

void ASpi_Reset(ASpi *InstancePtr);

void ASpi_SetCS_CLK(ASpi *InstancePtr,unsigned char cs, unsigned char clk);

int ASpi_Erase(ASpi *InstancePtr,unsigned int flash_addr);
int ASpi_Write(ASpi *InstancePtr,unsigned int flash_addr,u8 data);
int ASpi_Read(ASpi *InstancePtr,unsigned int flash_addr,u8 *pdata);
int ASpi_Transfer(ASpi *InstancePtr, u8 *SendBufPtr, u8 *RecvBufPtr,
		  unsigned int ByteCount);

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
/** @} */
