
#ifndef ASPI_L_H		/* prevent circular inclusions */
#define ASPI_L_H		/* by using protection macros */


/***************************** Include Files *********************************/

/****************************************************************************/
/**
*
* Read from the specified Spi device register.
*
* @param	BaseAddress contains the base address of the device.
* @param	RegOffset contains the offset from the 1st register of the
*		device to select the specific register.
*
* @return	The value read from the register.
*
* @note		C-Style signature:
*		u32 ASpi_ReadReg(u32 BaseAddress, u32 RegOffset);
*
******************************************************************************/
#define ASpi_ReadReg(BaseAddress, RegOffset) \
	Spi_In32((BaseAddress) + (RegOffset))

/***************************************************************************/
/**
*
* Write to the specified Spi device register.
*
* @param	BaseAddress contains the base address of the device.
* @param	RegOffset contains the offset from the 1st register of the
*		device to select the specific register.
* @param	RegisterValue is the value to be written to the register.
*
* @return	None.
*
* @note		C-Style signature:
*		void ASpi_WriteReg(u32 BaseAddress, u32 RegOffset,
*					u32 RegisterValue);
******************************************************************************/
#define ASpi_WriteReg(BaseAddress, RegOffset, RegisterValue) \
	Spi_Out32((BaseAddress) + (RegOffset), (RegisterValue))

/************************** Function Prototypes ******************************/

/************************** Constant Definitions *****************************/

/**
 * ASPI register offsets
 */
/** @name Register Map
 *
 * Register offsets for the ASpi device.
 * @{
 */
#define ASP_CFG_OFFSET		0x0	/**< spi_flash_mgr_cfg Reg */
#define ASP_ADDR_OFFSET		0x4	/**< spi_flash _mgr_addr Reg*/
#define ASP_CTRL_OFFSET	 	0x8	/**spi_flash_mgr_ctrl Reg*/
#define ASP_SR_OFFSET		0x10	/**spi_flash _mgr_stat1 Reg*/
#define ASP_DRR_OFFSET		0x14	/**< Data recieve v*/
#define ASP_MUX_OFFSET      0x80 /*flash_mux*/


/** @name SPI Control Register (CFG) masks
 *
 * @{
 */
/* Bitfields in ASP_CFG_OFFSET */
#define ASP_CFG_RST_OFFSET 31
#define ASP_CFG_RST_SIZE 1
#define ASP_CFG_ENA_OFFSET 30
#define ASP_CFG_RST_SIZE 1
#define ASP_CFG_ERROR_CLEAR_OFFSET 29
#define ASP_CFG_ERROR_CLEAR_SIZE 1
#define ASP_CFG_RESERVED_OFFSET 16
#define ASP_CFG_RESERVED_SIZE 13
#define ASP_CFG_RW_CNT_OFFSET 8
#define ASP_CFG_ERROR_RW_CNT_SIZE 8
#define ASP_CFG_RW_CNT_OFFSET 8
#define ASP_CFG_RW_CNT_SIZE 8
#define ASP_CFG_ERASE_TYPE_OFFSET 4
#define ASP_CFG_ERASE_TYPE_SIZE 2
#define ASP_CFG_FLASH_CMD_OFFSET 0
#define ASP_CFG_FLASH_CMD_SIZE 4

/* Constants for ASP_ERASE_TYPE  */
#define SECTOR_ERASE 0x1
#define BLOCK_ERASE 0x2
#define CHIP_ERASE 0x3
/* Constants for ASP_FLASH_CMD  */
#define READ_FLASH 0x1
#define WRITE_FLASH 0x2
#define ERASE_FLASH 0x3
#define READ_STATUS_REG0 0x4
#define READ_STATUS_REG1 0x5
#define QUAD_READ 0x6
#define WRITE_STATUS_REG 0x7
#define ENABLE_WRITE 0x8

/* Bitfields in ASP_ADDR_OFFSET */
#define ASP_FLASH_ADDR_OFFSET 0
#define ASP_FLASH_ADDR_SIZE 24

/* Bitfields in ASP_CTRL_OFFSET */
#define ASP_CTRL_DATA_OFFSET 16
#define ASP_CTRL_DATA_SIZE 8
#define ASP_CTRL_REG_OFFSET 0
#define ASP_CTRL_REG_SIZE 16

/* Bitfields in ASP_STATUS1 */
#define ASP_SR_BUSY_OFFSET 31
#define ASP_SR_BUSY_SIZE 1
#define ASP_SR_RESERVED_OFFSET 30
#define ASP_SR_RESERVED_SIZE 1
#define ASP_SR_FLASH_ERROR_OFFSET 16
#define ASP_SR_FLASH_ERROR_SIZE 8
#define ASP_SR_REG1_OFFSET 8
#define ASP_SR_REG1_SIZE 8
#define ASP_SR_REG0_OFFSET 0
#define ASP_SR_REG0_SIZE 8

/* Bitfields in ASP_DRR */
#define ASP_DRR_DATA_OFFSET 0
#define ASP_DRR_DATA_SIZE 8

/* Bitfields in ASP_MUX */
#define ASP_MUX_CS_OFFSET 0
#define ASP_MUX_CS_SIZE 8
#define ASP_MUX_CLK_OFFSET 8
#define ASP_MUX_CLK_SIZE 8

#define ASP_MUX_CS_FPGA 0
#define ASP_MUX_CS_CPLD1 1
#define ASP_MUX_CS_CPLD2 2
#define ASP_MUX_CS_SYSCPLD 3
/*spi input CLK=125M*/
#if 0
#define ASP_MUX_CLK_64M 0
#define ASP_MUX_CLK_32M 1
#define ASP_MUX_CLK_16M 2
#define ASP_MUX_CLK_8M 3
#define ASP_MUX_CLK_4M 4
#define ASP_MUX_CLK_2M 5
#define ASP_MUX_CLK_1M 6
#define ASP_MUX_CLK_500K 7
#endif

/*spi input CLK= 50MHz */
#define ASP_MUX_CLK_25M 2
#define ASP_MUX_CLK_12500K 3
#define ASP_MUX_CLK_62500K 4
#define ASP_MUX_CLK_31250K 5

/* Bit manipulation macros */
#define ASP_BIT(name)					\
	(1 << ASP_##name##_OFFSET)
#define ASP_BF(name,value)				\
	(((value) & ((1 << ASP_##name##_SIZE) - 1))	\
	 << ASP_##name##_OFFSET)
#define ASP_BFEXT(name,value)\
	(((value) >> ASP_##name##_OFFSET)		\
	 & ((1 << ASP_##name##_SIZE) - 1))
#define ASP_BFINS(name,value,old)			\
	(((old) & ~(((1 << ASP_##name##_SIZE) - 1)	\
		    << ASP_##name##_OFFSET))		\
	 | ASP_BF(name,value))

#define ASP_DATAWIDTH_BYTE	 8  /**< Tx/Rx Reg is Byte Wide */
#define ASP_DATAWIDTH_HALF_WORD	16  /**< Tx/Rx Reg is Half Word (16 bit)
						Wide */
#define ASP_DATAWIDTH_WORD	32  /**< Tx/Rx Reg is Word (32 bit)  Wide */


#endif /* end of protection macro */
/** @} */
