/*
 * Author: Wangyb <wangyb@centecnetworks.com>
 *
 * Copyright 2005-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 */

#define CTC_IC_CON_MASTER		0x1
#define CTC_IC_CON_SPEED_STD		0x2
#define CTC_IC_CON_SPEED_FAST		0x4
#define CTC_IC_CON_SPEED_MASK		0x6
#define CTC_IC_CON_10BITADDR_MASTER	0x10
#define CTC_IC_CON_RESTART_EN		0x20
#define CTC_IC_CON_SLAVE_DISABLE		0x40

#define CTC_SCL_Timing(T)	(1000000000 / T)	/* in nanoseconds */

/*
 * Registers offset
 */
#define CTC_IC_CON		0x0
#define CTC_IC_TAR		0x4
#define CTC_IC_DATA_CMD		0x10
#define CTC_IC_SS_SCL_HCNT	0x14
#define CTC_IC_SS_SCL_LCNT	0x18
#define CTC_IC_FS_SCL_HCNT	0x1c
#define CTC_IC_FS_SCL_LCNT	0x20
#define CTC_IC_INTR_STAT		0x2c
#define CTC_IC_INTR_MASK		0x30
#define CTC_IC_RAW_INTR_STAT	0x34
#define CTC_IC_RX_TL		0x38
#define CTC_IC_TX_TL		0x3c
#define CTC_IC_CLR_INTR		0x40
#define CTC_IC_CLR_RX_UNDER	0x44
#define CTC_IC_CLR_RX_OVER	0x48
#define CTC_IC_CLR_TX_OVER	0x4c
#define CTC_IC_CLR_RD_REQ	0x50
#define CTC_IC_CLR_TX_ABRT	0x54
#define CTC_IC_CLR_RX_DONE	0x58
#define CTC_IC_CLR_ACTIVITY	0x5c
#define CTC_IC_CLR_STOP_DET	0x60
#define CTC_IC_CLR_START_DET	0x64
#define CTC_IC_CLR_GEN_CALL	0x68
#define CTC_IC_ENABLE		0x6c
#define CTC_IC_STATUS		0x70
#define CTC_IC_TXFLR		0x74
#define CTC_IC_RXFLR		0x78
#define CTC_IC_SDA_HOLD		0x7c
#define CTC_IC_TX_ABRT_SOURCE	0x80
#define CTC_IC_ENABLE_STATUS	0x9c
#define CTC_IC_COMP_PARAM_1	0xf4
#define CTC_IC_COMP_VERSION	0xf8
#define CTC_IC_SDA_HOLD_MIN_VERS	0x3131312A
#define CTC_IC_COMP_TYPE		0xfc
#define CTC_IC_COMP_TYPE_VALUE	0x44570140

#define CTC_IC_STATUS_ACTIVITY	0x1
#define CTC_IC_TAR_10BITADDR_MASTER BIT(12)
#define CTC_IC_ERR_TX_ABRT	0x1

#define CTC_IC_INTR_RX_UNDER	0x001
#define CTC_IC_INTR_RX_OVER	0x002
#define CTC_IC_INTR_RX_FULL	0x004
#define CTC_IC_INTR_TX_OVER	0x008
#define CTC_IC_INTR_TX_EMPTY	0x010
#define CTC_IC_INTR_RD_REQ	0x020
#define CTC_IC_INTR_TX_ABRT	0x040
#define CTC_IC_INTR_RX_DONE	0x080
#define CTC_IC_INTR_ACTIVITY	0x100
#define CTC_IC_INTR_STOP_DET	0x200
#define CTC_IC_INTR_START_DET	0x400
#define CTC_IC_INTR_GEN_CALL	0x800

#define CTC_IC_INTR_DEFAULT_MASK		(CTC_IC_INTR_RX_FULL | \
					 CTC_IC_INTR_TX_ABRT | \
					 CTC_IC_INTR_STOP_DET)

/*
 * status codes
 */
#define STATUS_IDLE			0x0
#define STATUS_WRITE_IN_PROGRESS	0x1
#define STATUS_READ_IN_PROGRESS		0x2

#define ABRT_7B_ADDR_NOACK	0
#define ABRT_10ADDR1_NOACK	1
#define ABRT_10ADDR2_NOACK	2
#define ABRT_TXDATA_NOACK	3
#define ABRT_GCALL_NOACK	4
#define ABRT_GCALL_READ		5
#define ABRT_SBYTE_ACKDET	7
#define ABRT_SBYTE_NORSTRT	9
#define ABRT_10B_RD_NORSTRT	10
#define ABRT_MASTER_DIS		11
#define ARB_LOST		12

#define CTC_IC_TX_ABRT_7B_ADDR_NOACK	(1UL << ABRT_7B_ADDR_NOACK)
#define CTC_IC_TX_ABRT_10ADDR1_NOACK	(1UL << ABRT_10ADDR1_NOACK)
#define CTC_IC_TX_ABRT_10ADDR2_NOACK	(1UL << ABRT_10ADDR2_NOACK)
#define CTC_IC_TX_ABRT_TXDATA_NOACK	(1UL << ABRT_TXDATA_NOACK)
#define CTC_IC_TX_ABRT_GCALL_NOACK	(1UL << ABRT_GCALL_NOACK)
#define CTC_IC_TX_ABRT_GCALL_READ	(1UL << ABRT_GCALL_READ)
#define CTC_IC_TX_ABRT_SBYTE_ACKDET	(1UL << ABRT_SBYTE_ACKDET)
#define CTC_IC_TX_ABRT_SBYTE_NORSTRT	(1UL << ABRT_SBYTE_NORSTRT)
#define CTC_IC_TX_ABRT_10B_RD_NORSTRT	(1UL << ABRT_10B_RD_NORSTRT)
#define CTC_IC_TX_ABRT_MASTER_DIS	(1UL << ABRT_MASTER_DIS)
#define CTC_IC_TX_ARB_LOST		(1UL << ARB_LOST)

#define CTC_IC_TX_ABRT_NOACK		(CTC_IC_TX_ABRT_7B_ADDR_NOACK | \
					 CTC_IC_TX_ABRT_10ADDR1_NOACK | \
					 CTC_IC_TX_ABRT_10ADDR2_NOACK | \
					 CTC_IC_TX_ABRT_TXDATA_NOACK | \
					 CTC_IC_TX_ABRT_GCALL_NOACK)

#define CTC_CMD_READ   	0x0100
#define CTC_STOP		0x0200
#define CTC_RESTART		0x0400

#define CTC_IC_STATUS_RFNE		0x0008
#define CTC_IC_STATUS_TFNF		0x0002

#define I2C_BYTE_TO			1	/* means 1 jiffies = 4ms */
#define I2C_STOPDET_TO		1	/* means 1 jiffies = 4ms */

#define CTC_IC_STATUS_MA		0x0020
#define CTC_IC_STATUS_TFE		0x0004

#define CTC_IC_DEFAULT_FUNCTIONALITY (I2C_FUNC_I2C |			\
					I2C_FUNC_SMBUS_BYTE |		\
					I2C_FUNC_SMBUS_BYTE_DATA |	\
					I2C_FUNC_SMBUS_WORD_DATA |	\
					I2C_FUNC_SMBUS_BLOCK_DATA |	\
					I2C_FUNC_SMBUS_I2C_BLOCK)

#define CTC_IC_INTR_MASTER_MASK		(CTC_IC_INTR_DEFAULT_MASK | \
					 CTC_IC_INTR_TX_EMPTY)

#define CTC_IC_INTR_SLAVE_MASK		(CTC_IC_INTR_DEFAULT_MASK | \
					CTC_IC_INTR_RX_DONE | \
					CTC_IC_INTR_RX_UNDER | \
					CTC_IC_INTR_RD_REQ)

#define CTC_IC_CON_STOP_DET_IFADDRESSED		0x80
#define CTC_IC_CON_TX_EMPTY_CTRL	0x100
#define CTC_IC_CON_RX_FIFO_FULL_HLD_CTRL	0x200

#define CTC_IC_SAR	0x8

#define CTC_IC_STATUS_SLAVE_ACTIVITY	BIT(6)

#define CTC_IC_BUS_CLEAR_EN 	0xb0
#define CTC_IC_BUS_CLEAR_THRD   0xb4

/*
 * operation modes
 */
#define CTC_IC_MASTER		0
#define CTC_IC_SLAVE		1

enum xfer_type_e {
	CTC_IC_INTERRUPT_TRANSFER,
	CTC_IC_POLLING_TRANSFER
};

struct ctc_i2c_dev {
	struct device *dev;
	void __iomem *base;
	struct completion cmd_complete;
	struct clk *clk;
	struct mutex lock;
	u32 master_cfg;
	int irq;
	unsigned int rx_fifo_depth;
	unsigned int tx_fifo_depth;
	struct i2c_adapter adapter;
	struct i2c_msg *msgs;
	int msgs_num;
	int msg_err;
	int cmd_err;
	unsigned int status;
	u32 abort_source;
	int msg_read_idx;
	int rx_outstanding;
	int msg_write_idx;
	u32 tx_buf_len;
	u8 *tx_buf;
	u32 rx_buf_len;
	u8 *rx_buf;
	u32 accessor_flags;
	u32 functionality;
	u32 clk_freq;
	u32 sda_hold_time;
	u32 xfer_type;
	u32 mode;
	u32 slave_cfg;
	void (*disable) (struct ctc_i2c_dev * dev);
	void (*disable_int) (struct ctc_i2c_dev * dev);
	int (*init) (struct ctc_i2c_dev * dev);
	struct i2c_client *slave;
	struct regmap *regmap_base;
	u32 soc_ver;
#define CTC_REV_TM_1_0 0x0
#define CTC_REV_TM_1_1 0x1
	u32 i2c_num;
};
