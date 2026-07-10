#ifndef __CTCMAC_H__
#define __CTCMAC_H__

#define CPUMAC_MEM_BASE   0x00004000
#define CPUMAC_REG_BASE   0x00000000

struct CpuMac_regs {
	u32 CpuMacAxiCfg;	/* 0x00000000 */
	u32 CpuMacAxiMon;	/* 0x00000004 */
	u32 CpuMacDescCfg[2];	/* 0x00000008 */
	u32 CpuMacBufferCfg[3];	/* 0x00000010 */
	u32 rsv7;
	u32 CpuMacDebugStats[3];	/* 0x00000020 */
	u32 rsv11;
	u32 CpuMacDescMon[3];	/* 0x00000030 */
	u32 rsv15;
	u32 CpuMacDmaWeightCfg;	/* 0x00000040 */
	u32 CpuMacInit;		/* 0x00000044 */
	u32 CpuMacInitDone;	/* 0x00000048 */
	u32 CpuMacParityCtl;	/* 0x0000004c */
	u32 CpuMacExtRamCfg[2];	/* 0x00000050 */
	u32 rsv22;
	u32 rsv23;
	u32 CpuMacFifoCtl[4];	/* 0x00000060 */
	u32 CpuMacGmacCfg[4];	/* 0x00000070 */
	u32 CpuMacRamChkRec;	/* 0x00000080 */
	u32 CpuMacReset;	/* 0x00000084 */
	u32 CpuMacSgmiiAutoNegCfg;	/* 0x00000088 */
	u32 CpuMacReserved;	/* 0x0000008c */
	u32 CpuMacGmacMon[2];	/* 0x00000090 */
	u32 CpuMacCreditCtl;	/* 0x00000098 */
	u32 CpuMacCreditStatus;	/* 0x0000009c */
	u32 CpuMacPauseCfg[4];	/* 0x000000a0 */
	u32 CpuMacPauseMon[3];	/* 0x000000b0 */
	u32 rsv47;
	u32 CpuMacSgmiiCfg[2];	/* 0x000000c0 */
	u32 CpuMacSgmiiMon[2];	/* 0x000000c8 */
	u32 CpuMacStatsCfg[2];	/* 0x000000d0 */
	u32 rsv54;
	u32 rsv55;
	u32 CpuMacInterruptFunc[4];	/* 0x000000e0 */
	u32 CpuMacInterruptNormal[4];	/* 0x000000f0 */
	u32 CpuMacFifoStatus[3];	/* 0x00000100 */
	u32 rsv67;
	u32 CpuMacDescCfg1[3];	/* 0x00000110 */
	u32 rsv71;
	u32 CpuMacInterruptFunc0[4];	/* 0x00000120 */
	u32 CpuMacInterruptFunc1[4];	/* 0x00000130 */
};

/* ################################################################################
 * # CpuMacAxiCfg Definition */
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_ADDR_ALIGN_EN_BIT              20
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_BURST_LEN_BIT                  8
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_D_WORD_SWAP_EN_BIT             17
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_WORD_SWAP_EN_BIT               16
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_BURST_LEN_BIT                  0
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_D_WORD_SWAP_EN_BIT             19
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_WORD_SWAP_EN_BIT               18

#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_ADDR_ALIGN_EN_MASK             0x00100000
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_BURST_LEN_MASK                 0x0000ff00
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_D_WORD_SWAP_EN_MASK            0x00020000
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_WORD_SWAP_EN_MASK              0x00010000
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_BURST_LEN_MASK                 0x000000ff
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_D_WORD_SWAP_EN_MASK            0x00080000
#define CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_WORD_SWAP_EN_MASK              0x00040000

/* ################################################################################
 * # CpuMacAxiMon Definition */
#define CPU_MAC_AXI_MON_W0_MON_AXI_RD_RESP_BIT                       0
#define CPU_MAC_AXI_MON_W0_MON_AXI_WR_RESP_BIT                       4

#define CPU_MAC_AXI_MON_W0_MON_AXI_RD_RESP_MASK                      0x00000007
#define CPU_MAC_AXI_MON_W0_MON_AXI_WR_RESP_MASK                      0x00000070

/* ################################################################################
 * # CpuMacDescCfg Definition */
#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC0_DONE_INTR_THRD_BIT          8
#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC1_DONE_INTR_THRD_BIT          16
#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC_DONE_INTR_EOP_EN_BIT         29
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_ACK_EN_BIT                   31
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_DONE_INTR_EOP_EN_BIT         30
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_DONE_INTR_THRD_BIT           0
#define CPU_MAC_DESC_CFG_W1_CFG_RX_DESC0_REQ_INTR_THRD_BIT           0
#define CPU_MAC_DESC_CFG_W1_CFG_RX_DESC1_REQ_INTR_THRD_BIT           8

#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC0_DONE_INTR_THRD_MASK         0x0000ff00
#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC1_DONE_INTR_THRD_MASK         0x00ff0000
#define CPU_MAC_DESC_CFG_W0_CFG_RX_DESC_DONE_INTR_EOP_EN_MASK        0x20000000
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_ACK_EN_MASK                  0x80000000
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_DONE_INTR_EOP_EN_MASK        0x40000000
#define CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_DONE_INTR_THRD_MASK          0x000000ff
#define CPU_MAC_DESC_CFG_W1_CFG_RX_DESC0_REQ_INTR_THRD_MASK          0x000000ff
#define CPU_MAC_DESC_CFG_W1_CFG_RX_DESC1_REQ_INTR_THRD_MASK          0x0000ff00

/* ################################################################################
 * # CpuMacBufferCfg Definition */
#define CPU_MAC_BUFFER_CFG_W0_CFG_RX_X_OFF_THRD_BIT                  16
#define CPU_MAC_BUFFER_CFG_W0_CFG_RX_X_ON_THRD_BIT                   0
#define CPU_MAC_BUFFER_CFG_W1_CFG_RX_FLUSH_THRD_BIT                  16
#define CPU_MAC_BUFFER_CFG_W1_CFG_RX_RSV_THRD_BIT                    0
#define CPU_MAC_BUFFER_CFG_W2_CFG_RX_FLUSH_EN_BIT                    0

#define CPU_MAC_BUFFER_CFG_W0_CFG_RX_X_OFF_THRD_MASK                 0x3fff0000
#define CPU_MAC_BUFFER_CFG_W0_CFG_RX_X_ON_THRD_MASK                  0x00003fff
#define CPU_MAC_BUFFER_CFG_W1_CFG_RX_FLUSH_THRD_MASK                 0x3fff0000
#define CPU_MAC_BUFFER_CFG_W1_CFG_RX_RSV_THRD_MASK                   0x00003fff
#define CPU_MAC_BUFFER_CFG_W2_CFG_RX_FLUSH_EN_MASK                   0x00000001

/* ################################################################################
 * # CpuMacDebugStats Definition */
#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_EOP_CNT_BIT                    16
#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_ERROR_CNT_BIT                  20
#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_SOP_CNT_BIT                    12
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_EOP_CNT_BIT                   4
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_ERROR_CNT_BIT                 8
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_SOP_CNT_BIT                   0
#define CPU_MAC_DEBUG_STATS_W0_GMAC_TX_EOP_CNT_BIT                   28
#define CPU_MAC_DEBUG_STATS_W0_GMAC_TX_SOP_CNT_BIT                   24
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_EOP_CNT_BIT                    8
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_ERROR_CNT_BIT                  12
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_SOP_CNT_BIT                    4
#define CPU_MAC_DEBUG_STATS_W1_GMAC_TX_ERROR_CNT_BIT                 0
#define CPU_MAC_DEBUG_STATS_W1_GMAC_TX_PTP_ERR_CNT_BIT               16
#define CPU_MAC_DEBUG_STATS_W2_AXI_RD_BURST_CNT_BIT                  8
#define CPU_MAC_DEBUG_STATS_W2_AXI_RD_BURST_ERR_CNT_BIT              12
#define CPU_MAC_DEBUG_STATS_W2_AXI_WR_BURST_CNT_BIT                  0
#define CPU_MAC_DEBUG_STATS_W2_AXI_WR_BURST_ERR_CNT_BIT              4
#define CPU_MAC_DEBUG_STATS_W2_EXT_RAM_SBE_CNT_BIT                   16

#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_EOP_CNT_MASK                   0x000f0000
#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_ERROR_CNT_MASK                 0x00f00000
#define CPU_MAC_DEBUG_STATS_W0_DMA_RX_SOP_CNT_MASK                   0x0000f000
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_EOP_CNT_MASK                  0x000000f0
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_ERROR_CNT_MASK                0x00000f00
#define CPU_MAC_DEBUG_STATS_W0_GMAC_RX_SOP_CNT_MASK                  0x0000000f
#define CPU_MAC_DEBUG_STATS_W0_GMAC_TX_EOP_CNT_MASK                  0xf0000000
#define CPU_MAC_DEBUG_STATS_W0_GMAC_TX_SOP_CNT_MASK                  0x0f000000
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_EOP_CNT_MASK                   0x00000f00
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_ERROR_CNT_MASK                 0x0000f000
#define CPU_MAC_DEBUG_STATS_W1_DMA_TX_SOP_CNT_MASK                   0x000000f0
#define CPU_MAC_DEBUG_STATS_W1_GMAC_TX_ERROR_CNT_MASK                0x0000000f
#define CPU_MAC_DEBUG_STATS_W1_GMAC_TX_PTP_ERR_CNT_MASK              0x000f0000
#define CPU_MAC_DEBUG_STATS_W2_AXI_RD_BURST_CNT_MASK                 0x00000f00
#define CPU_MAC_DEBUG_STATS_W2_AXI_RD_BURST_ERR_CNT_MASK             0x0000f000
#define CPU_MAC_DEBUG_STATS_W2_AXI_WR_BURST_CNT_MASK                 0x0000000f
#define CPU_MAC_DEBUG_STATS_W2_AXI_WR_BURST_ERR_CNT_MASK             0x000000f0
#define CPU_MAC_DEBUG_STATS_W2_EXT_RAM_SBE_CNT_MASK                  0x000f0000

/* ################################################################################
 * # CpuMacDescMon Definition */
#define CPU_MAC_DESC_MON_W0_MON_DESC_AVAIL_NUM0_BIT                  0
#define CPU_MAC_DESC_MON_W0_MON_DESC_AVAIL_NUM1_BIT                  16
#define CPU_MAC_DESC_MON_W1_MON_DESC_AVAIL_NUM2_BIT                  0
#define CPU_MAC_DESC_MON_W1_MON_DESC_DONE_NUM0_BIT                   16
#define CPU_MAC_DESC_MON_W2_MON_DESC_DONE_NUM1_BIT                   0
#define CPU_MAC_DESC_MON_W2_MON_DESC_DONE_NUM2_BIT                   16

#define CPU_MAC_DESC_MON_W0_MON_DESC_AVAIL_NUM0_MASK                 0x0000ffff
#define CPU_MAC_DESC_MON_W0_MON_DESC_AVAIL_NUM1_MASK                 0xffff0000
#define CPU_MAC_DESC_MON_W1_MON_DESC_AVAIL_NUM2_MASK                 0x0000ffff
#define CPU_MAC_DESC_MON_W1_MON_DESC_DONE_NUM0_MASK                  0xffff0000
#define CPU_MAC_DESC_MON_W2_MON_DESC_DONE_NUM1_MASK                  0x0000ffff
#define CPU_MAC_DESC_MON_W2_MON_DESC_DONE_NUM2_MASK                  0xffff0000

/* ################################################################################
 * # CpuMacDmaWeightCfg Definition */
#define CPU_MAC_DMA_WEIGHT_CFG_W0_CFG_DMA_RX_WEIGHT0_BIT             0
#define CPU_MAC_DMA_WEIGHT_CFG_W0_CFG_DMA_RX_WEIGHT1_BIT             8

#define CPU_MAC_DMA_WEIGHT_CFG_W0_CFG_DMA_RX_WEIGHT0_MASK            0x0000000f
#define CPU_MAC_DMA_WEIGHT_CFG_W0_CFG_DMA_RX_WEIGHT1_MASK            0x00000f00

/* ################################################################################
 * # CpuMacInit Definition */
#define CPU_MAC_INIT_W0_INIT_BIT                                     0

#define CPU_MAC_INIT_W0_INIT_MASK                                    0x00000001

/* ################################################################################
 * # CpuMacInitDone Definition */
#define CPU_MAC_INIT_DONE_W0_INIT_DONE_BIT                           0

#define CPU_MAC_INIT_DONE_W0_INIT_DONE_MASK                          0x00000001

/* ################################################################################
 * # CpuMacParityCtl Definition */
#define CPU_MAC_PARITY_CTL_W0_CPU_MAC_STATS_RAM_PARITY_EN_BIT        0
#define CPU_MAC_PARITY_CTL_W0_EXT_RAM_ECC_CORRECT_DIS_BIT            9
#define CPU_MAC_PARITY_CTL_W0_EXT_RAM_ECC_DETECT_DIS_BIT             8
#define CPU_MAC_PARITY_CTL_W0_RX_DESC0_CFG_FIFO_PARITY_EN_BIT        5
#define CPU_MAC_PARITY_CTL_W0_RX_DESC1_CFG_FIFO_PARITY_EN_BIT        6
#define CPU_MAC_PARITY_CTL_W0_RX_PKT_DATA_FIFO_PARITY_EN_BIT         1
#define CPU_MAC_PARITY_CTL_W0_RX_PKT_MSG_FIFO_PARITY_EN_BIT          2
#define CPU_MAC_PARITY_CTL_W0_TX_DESC_CFG_FIFO_PARITY_EN_BIT         7
#define CPU_MAC_PARITY_CTL_W0_TX_PKT_FIFO_PARITY_EN_BIT              4

#define CPU_MAC_PARITY_CTL_W0_CPU_MAC_STATS_RAM_PARITY_EN_MASK       0x00000001
#define CPU_MAC_PARITY_CTL_W0_EXT_RAM_ECC_CORRECT_DIS_MASK           0x00000200
#define CPU_MAC_PARITY_CTL_W0_EXT_RAM_ECC_DETECT_DIS_MASK            0x00000100
#define CPU_MAC_PARITY_CTL_W0_RX_DESC0_CFG_FIFO_PARITY_EN_MASK       0x00000020
#define CPU_MAC_PARITY_CTL_W0_RX_DESC1_CFG_FIFO_PARITY_EN_MASK       0x00000040
#define CPU_MAC_PARITY_CTL_W0_RX_PKT_DATA_FIFO_PARITY_EN_MASK        0x00000002
#define CPU_MAC_PARITY_CTL_W0_RX_PKT_MSG_FIFO_PARITY_EN_MASK         0x00000004
#define CPU_MAC_PARITY_CTL_W0_TX_DESC_CFG_FIFO_PARITY_EN_MASK        0x00000080
#define CPU_MAC_PARITY_CTL_W0_TX_PKT_FIFO_PARITY_EN_MASK             0x00000010

/* ################################################################################
 * # CpuMacExtRamCfg Definition */
#define CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_BASE_SIZE_BIT             0
#define CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_EN_BIT                    31
#define CPU_MAC_EXT_RAM_CFG_W1_CFG_EXT_RAM_BASE_ADDR_BIT             0

#define CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_BASE_SIZE_MASK            0x000003ff
#define CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_EN_MASK                   0x80000000
#define CPU_MAC_EXT_RAM_CFG_W1_CFG_EXT_RAM_BASE_ADDR_MASK            0x000fffff

/* ################################################################################
 * # CpuMacFifoCtl Definition */
#define CPU_MAC_FIFO_CTL_W0_RX_PKT_DATA_FIFO_A_FULL_THRD_BIT         0
#define CPU_MAC_FIFO_CTL_W1_RX_PKT_MSG_FIFO_A_FULL_THRD_BIT          0
#define CPU_MAC_FIFO_CTL_W2_TX_PKT_FIFO_A_FULL_THRD_BIT              0
#define CPU_MAC_FIFO_CTL_W3_RX_PKT_FIFO_A_FULL_THRD_BIT              0

#define CPU_MAC_FIFO_CTL_W0_RX_PKT_DATA_FIFO_A_FULL_THRD_MASK        0x00001fff
#define CPU_MAC_FIFO_CTL_W1_RX_PKT_MSG_FIFO_A_FULL_THRD_MASK         0x000003ff
#define CPU_MAC_FIFO_CTL_W2_TX_PKT_FIFO_A_FULL_THRD_MASK             0x000007ff
#define CPU_MAC_FIFO_CTL_W3_RX_PKT_FIFO_A_FULL_THRD_MASK             0x000003ff

/* ################################################################################
 * # CpuMacGmacCfg Definition */
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_CUT_THROUGH_EN_BIT                4
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_ERROR_DROP_EN_BIT                 5
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERRUN_DROP_EN_BIT               1
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERSIZE_DROP_EN_BIT              2
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_PAUSE_DROP_EN_BIT                 6
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_TS_EN_BIT                         0
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_UNDERSIZE_DROP_EN_BIT             3
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MAX_PKT_LEN_BIT                   1
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MAX_PKT_LEN_CHK_EN_BIT            0
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MIN_PKT_LEN_BIT                   17
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MIN_PKT_LEN_CHK_EN_BIT            16
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_APPEND_CRC_EN_BIT                 0
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_CUT_THROUGH_EN_BIT                10
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_ERR_MASK_OFF_BIT                  1
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_HDR_INFO_EN_BIT                   11
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_KEEP_TS_EN_BIT                    12
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_LPI_EN_BIT                        2
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PAD_EN_BIT                        3
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PAUSE_STALL_EN_BIT                4
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PKT_EN_BIT                        5
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PTP_ERR_EN_BIT                    6
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_START_THRD_BIT                    16
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_STRIP_CRC_EN_BIT                  8
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_WAIT_CAPTURE_TS_BIT               9
#define CPU_MAC_GMAC_CFG_W3_CFG_TX_SLEEP_TIMER_BIT                   0
#define CPU_MAC_GMAC_CFG_W3_CFG_TX_WAKEUP_TIMER_BIT                  16

#define CPU_MAC_GMAC_CFG_W0_CFG_RX_CUT_THROUGH_EN_MASK               0x00000010
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_ERROR_DROP_EN_MASK                0x00000020
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERRUN_DROP_EN_MASK              0x00000002
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERSIZE_DROP_EN_MASK             0x00000004
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_PAUSE_DROP_EN_MASK                0x00000040
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_TS_EN_MASK                        0x00000001
#define CPU_MAC_GMAC_CFG_W0_CFG_RX_UNDERSIZE_DROP_EN_MASK            0x00000008
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MAX_PKT_LEN_MASK                  0x0000fffe
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MAX_PKT_LEN_CHK_EN_MASK           0x00000001
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MIN_PKT_LEN_MASK                  0x00fe0000
#define CPU_MAC_GMAC_CFG_W1_CFG_RX_MIN_PKT_LEN_CHK_EN_MASK           0x00010000
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_APPEND_CRC_EN_MASK                0x00000001
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_CUT_THROUGH_EN_MASK               0x00000400
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_ERR_MASK_OFF_MASK                 0x00000002
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_HDR_INFO_EN_MASK                  0x00000800
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_KEEP_TS_EN_MASK                   0x00001000
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_LPI_EN_MASK                       0x00000004
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PAD_EN_MASK                       0x00000008
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PAUSE_STALL_EN_MASK               0x00000010
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PKT_EN_MASK                       0x00000020
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_PTP_ERR_EN_MASK                   0x00000040
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_START_THRD_MASK                   0x07ff0000
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_STRIP_CRC_EN_MASK                 0x00000100
#define CPU_MAC_GMAC_CFG_W2_CFG_TX_WAIT_CAPTURE_TS_MASK              0x00000200
#define CPU_MAC_GMAC_CFG_W3_CFG_TX_SLEEP_TIMER_MASK                  0x0000ffff
#define CPU_MAC_GMAC_CFG_W3_CFG_TX_WAKEUP_TIMER_MASK                 0xffff0000

/* ################################################################################
 * # CpuMacRamChkRec Definition */
#define CPU_MAC_RAM_CHK_REC_W0_CPU_MAC_STATS_RAM_PARITY_FAIL_BIT     31
#define CPU_MAC_RAM_CHK_REC_W0_CPU_MAC_STATS_RAM_PARITY_FAIL_ADDR_BIT 0

#define CPU_MAC_RAM_CHK_REC_W0_CPU_MAC_STATS_RAM_PARITY_FAIL_MASK    0x80000000
#define CPU_MAC_RAM_CHK_REC_W0_CPU_MAC_STATS_RAM_PARITY_FAIL_ADDR_MASK 0x0000003f

/* ################################################################################
 * # CpuMacReset Definition */
#define CPU_MAC_RESET_W0_SOFT_RST_RX_BIT                             0
#define CPU_MAC_RESET_W0_SOFT_RST_TX_BIT                             1

#define CPU_MAC_RESET_W0_SOFT_RST_RX_MASK                            0x00000001
#define CPU_MAC_RESET_W0_SOFT_RST_TX_MASK                            0x00000002

/* ################################################################################
 * # CpuMacSgmiiAutoNegCfg Definition */
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_ENABLE_BIT              0
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_MODE_BIT                2
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_PARALLEL_DETECT_EN_BIT  24
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_REMOTE_LINKUP_EN_BIT    25
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_RESTART_BIT             1
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_IGNORE_ANEG_ERR_BIT        4
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_IGNORE_LINK_FAILURE_BIT    5
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LINK_TIMER_CNT_BIT         16
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_EEE_ABILITY_BIT      9
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_EEE_CLK_STOP_BIT     10
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_OFFLINE_BIT          6
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_PAUSE_ABILITY_BIT    11
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_SPEED_MODE_BIT       7
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_TX_CONFIG_REG_BIT14_BIT    13
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_TX_XMIT_LOAD_USE_ASYNC_FIFO_BIT 14

#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_ENABLE_MASK             0x00000001
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_MODE_MASK               0x0000000c
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_PARALLEL_DETECT_EN_MASK 0x01000000
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_REMOTE_LINKUP_EN_MASK   0x02000000
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_RESTART_MASK            0x00000002
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_IGNORE_ANEG_ERR_MASK       0x00000010
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_IGNORE_LINK_FAILURE_MASK   0x00000020
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LINK_TIMER_CNT_MASK        0x00ff0000
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_EEE_ABILITY_MASK     0x00000200
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_EEE_CLK_STOP_MASK    0x00000400
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_OFFLINE_MASK         0x00000040
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_PAUSE_ABILITY_MASK   0x00001800
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_LOCAL_SPEED_MODE_MASK      0x00000180
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_TX_CONFIG_REG_BIT14_MASK   0x00002000
#define CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_TX_XMIT_LOAD_USE_ASYNC_FIFO_MASK 0x00004000

/* ################################################################################
 * # CpuMacReserved Definition */
#define CPU_MAC_RESERVED_W0_RESERVED_BIT                             0

#define CPU_MAC_RESERVED_W0_RESERVED_MASK                            0xffffffff

/* ################################################################################
 * # CpuMacGmacMon Definition */
#define CPU_MAC_GMAC_MON_W0_MON_RX_METER_TOKEN_BIT                   8
#define CPU_MAC_GMAC_MON_W0_MON_RX_PKT_STATE_BIT                     0
#define CPU_MAC_GMAC_MON_W0_MON_TX_LPI_STATE_BIT                     6
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_BUF_FULL_DROP_CNT_BIT            20
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_FILTER_DROP_CNT_BIT              24
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_PAUSE_DROP_CNT_BIT               28
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_RUNT_DROP_CNT_BIT                16
#define CPU_MAC_GMAC_MON_W1_MON_RX_BUF_RD_STATE_BIT                  2
#define CPU_MAC_GMAC_MON_W1_MON_RX_PKT_LEN_BIT                       8
#define CPU_MAC_GMAC_MON_W1_MON_TX_LPI_STATUS_BIT                    0
#define CPU_MAC_GMAC_MON_W1_MON_TX_POP_STATE_BIT                     5
#define CPU_MAC_GMAC_MON_W1_MON_TX_STATS_UNDERRUN_BIT                1

#define CPU_MAC_GMAC_MON_W0_MON_RX_METER_TOKEN_MASK                  0xffffff00
#define CPU_MAC_GMAC_MON_W0_MON_RX_PKT_STATE_MASK                    0x0000000f
#define CPU_MAC_GMAC_MON_W0_MON_TX_LPI_STATE_MASK                    0x000000c0
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_BUF_FULL_DROP_CNT_MASK           0x00f00000
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_FILTER_DROP_CNT_MASK             0x0f000000
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_PAUSE_DROP_CNT_MASK              0xf0000000
#define CPU_MAC_GMAC_MON_W1_GMAC_RX_RUNT_DROP_CNT_MASK               0x000f0000
#define CPU_MAC_GMAC_MON_W1_MON_RX_BUF_RD_STATE_MASK                 0x0000001c
#define CPU_MAC_GMAC_MON_W1_MON_RX_PKT_LEN_MASK                      0x0000ff00
#define CPU_MAC_GMAC_MON_W1_MON_TX_LPI_STATUS_MASK                   0x00000001
#define CPU_MAC_GMAC_MON_W1_MON_TX_POP_STATE_MASK                    0x000000e0
#define CPU_MAC_GMAC_MON_W1_MON_TX_STATS_UNDERRUN_MASK               0x00000002

/* ################################################################################
 * # CpuMacCreditCtl Definition */
#define CPU_MAC_CREDIT_CTL_W0_CPU_MAC_CPU_RAM_CREDIT_THRD_BIT        0

#define CPU_MAC_CREDIT_CTL_W0_CPU_MAC_CPU_RAM_CREDIT_THRD_MASK       0x000000ff

/* ################################################################################
 * # CpuMacCreditStatus Definition */
#define CPU_MAC_CREDIT_STATUS_W0_CPU_MAC_CPU_RAM_CREDIT_USED_BIT     0

#define CPU_MAC_CREDIT_STATUS_W0_CPU_MAC_CPU_RAM_CREDIT_USED_MASK    0x000000ff

/* ################################################################################
 * # CpuMacPauseCfg Definition */
#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_NORM_PAUSE_EN_BIT                0
#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_PAUSE_TIMER_ADJ_VALUE_BIT        1
#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_PAUSE_TIMER_DEC_VALUE_BIT        11
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_EN_BIT                     0
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_ADJ_VALUE_BIT        1
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_DEC_VALUE_BIT        18
#define CPU_MAC_PAUSE_CFG_W2_CFG_TX_PAUSE_MAC_SA_31_0_BIT            0
#define CPU_MAC_PAUSE_CFG_W3_CFG_TX_PAUSE_MAC_SA_47_32_BIT           0
#define CPU_MAC_PAUSE_CFG_W3_CFG_TX_PAUSE_QUANTA_BIT                 16

#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_NORM_PAUSE_EN_MASK               0x00000001
#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_PAUSE_TIMER_ADJ_VALUE_MASK       0x000007fe
#define CPU_MAC_PAUSE_CFG_W0_CFG_RX_PAUSE_TIMER_DEC_VALUE_MASK       0x0007f800
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_EN_MASK                    0x00000001
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_ADJ_VALUE_MASK       0x0003fffe
#define CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_DEC_VALUE_MASK       0x03fc0000
#define CPU_MAC_PAUSE_CFG_W2_CFG_TX_PAUSE_MAC_SA_31_0_MASK           0xffffffff
#define CPU_MAC_PAUSE_CFG_W3_CFG_TX_PAUSE_MAC_SA_47_32_MASK          0x0000ffff
#define CPU_MAC_PAUSE_CFG_W3_CFG_TX_PAUSE_QUANTA_MASK                0xffff0000

/* ################################################################################
 * # CpuMacPauseMon Definition */
#define CPU_MAC_PAUSE_MON_W0_MON_TX_PAUSE_TIMER_BIT                  0
#define CPU_MAC_PAUSE_MON_W1_MON_RX_PAUSE_TIMER_BIT                  0
#define CPU_MAC_PAUSE_MON_W2_MON_TX_PAUSE_CUR_STATUS_BIT             0
#define CPU_MAC_PAUSE_MON_W2_MON_TX_PAUSE_LOG_STATUS_BIT             1

#define CPU_MAC_PAUSE_MON_W0_MON_TX_PAUSE_TIMER_MASK                 0x0001ffff
#define CPU_MAC_PAUSE_MON_W1_MON_RX_PAUSE_TIMER_MASK                 0x0000ffff
#define CPU_MAC_PAUSE_MON_W2_MON_TX_PAUSE_CUR_STATUS_MASK            0x00000001
#define CPU_MAC_PAUSE_MON_W2_MON_TX_PAUSE_LOG_STATUS_MASK            0x00000002

/* ################################################################################
 * # CpuMacSgmiiCfg Definition */
#define CPU_MAC_SGMII_CFG_W0_CFG_FORCE_RELOCK_BIT                    0
#define CPU_MAC_SGMII_CFG_W0_CFG_FORCE_SIGNAL_DETECT_BIT             1
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_CHK_LINK_FOR_SOP_BIT         31
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_LINK_FILTER_EN_BIT           5
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_LINK_FILTER_TIMER_BIT        6
#define CPU_MAC_SGMII_CFG_W0_CFG_SIG_DET_ACTIVE_VALUE_BIT            2
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_EVEN_IGNORE_BIT                  3
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_IPG_LEN_BIT                      14
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_PREAMBLE_LEN_BIT                 18
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_REPLICATE_CNT_BIT                22
#define CPU_MAC_SGMII_CFG_W0_CFG_UNIDIRECTION_EN_BIT                 4
#define CPU_MAC_SGMII_CFG_W1_CFG_MII_RX_SAMPLE_CNT_BIT               22
#define CPU_MAC_SGMII_CFG_W1_CFG_MII_TX_A_FULL_THRD_BIT              5
#define CPU_MAC_SGMII_CFG_W1_CFG_TX_THRESHOLD_BIT                    0

#define CPU_MAC_SGMII_CFG_W0_CFG_FORCE_RELOCK_MASK                   0x00000001
#define CPU_MAC_SGMII_CFG_W0_CFG_FORCE_SIGNAL_DETECT_MASK            0x00000002
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_CHK_LINK_FOR_SOP_MASK        0x80000000
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_LINK_FILTER_EN_MASK          0x00000020
#define CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_LINK_FILTER_TIMER_MASK       0x00003fc0
#define CPU_MAC_SGMII_CFG_W0_CFG_SIG_DET_ACTIVE_VALUE_MASK           0x00000004
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_EVEN_IGNORE_MASK                 0x00000008
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_IPG_LEN_MASK                     0x0003c000
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_PREAMBLE_LEN_MASK                0x003c0000
#define CPU_MAC_SGMII_CFG_W0_CFG_TX_REPLICATE_CNT_MASK               0x1fc00000
#define CPU_MAC_SGMII_CFG_W0_CFG_UNIDIRECTION_EN_MASK                0x00000010
#define CPU_MAC_SGMII_CFG_W1_CFG_MII_RX_SAMPLE_CNT_MASK              0x1fc00000
#define CPU_MAC_SGMII_CFG_W1_CFG_MII_TX_A_FULL_THRD_MASK             0x000003e0
#define CPU_MAC_SGMII_CFG_W1_CFG_TX_THRESHOLD_MASK                   0x0000001f

/* ################################################################################
 * # CpuMacSgmiiMon Definition */
#define CPU_MAC_SGMII_MON_W0_MON_ANEG_STATE_BIT                      0
#define CPU_MAC_SGMII_MON_W0_MON_CODE_ERR_CNT_BIT                    4
#define CPU_MAC_SGMII_MON_W0_MON_LINK_STATUS_BIT                     8
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_EOP_FLAG_BIT               9
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_LPI_STATE_BIT              12
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_PKT_STATE_BIT              15
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_SFD_FLAG_BIT               11
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_SOP_FLAG_BIT               10
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_CODE_STATE_BIT             23
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_EOP_FLAG_BIT               19
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_LPI_STATE_BIT              20
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_SOP_FLAG_BIT               18
#define CPU_MAC_SGMII_MON_W1_MON_AN_RX_REMOTE_CFG_BIT                0

#define CPU_MAC_SGMII_MON_W0_MON_ANEG_STATE_MASK                     0x00000007
#define CPU_MAC_SGMII_MON_W0_MON_CODE_ERR_CNT_MASK                   0x000000f0
#define CPU_MAC_SGMII_MON_W0_MON_LINK_STATUS_MASK                    0x00000100
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_EOP_FLAG_MASK              0x00000200
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_LPI_STATE_MASK             0x00007000
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_PKT_STATE_MASK             0x00038000
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_SFD_FLAG_MASK              0x00000800
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_RX_SOP_FLAG_MASK              0x00000400
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_CODE_STATE_MASK            0x03800000
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_EOP_FLAG_MASK              0x00080000
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_LPI_STATE_MASK             0x00700000
#define CPU_MAC_SGMII_MON_W0_MON_SGMII_TX_SOP_FLAG_MASK              0x00040000
#define CPU_MAC_SGMII_MON_W1_MON_AN_RX_REMOTE_CFG_MASK               0x0000ffff

/* ################################################################################
 * # CpuMacStatsCfg Definition */
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS64_B_PKT_HI_PRI_BIT       4
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_CLEAR_ON_READ_BIT        0
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_INCR_HOLD_BIT            2
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_INCR_SATURATE_BIT        1
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_OVER_WRITE_EN_BIT        3
#define CPU_MAC_STATS_CFG_W1_CFG_GMAC_STATS_MTU1_BIT                 0
#define CPU_MAC_STATS_CFG_W1_CFG_GMAC_STATS_MTU2_BIT                 16

#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS64_B_PKT_HI_PRI_MASK      0x00000010
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_CLEAR_ON_READ_MASK       0x00000001
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_INCR_HOLD_MASK           0x00000004
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_INCR_SATURATE_MASK       0x00000002
#define CPU_MAC_STATS_CFG_W0_CFG_GMAC_STATS_OVER_WRITE_EN_MASK       0x00000008
#define CPU_MAC_STATS_CFG_W1_CFG_GMAC_STATS_MTU1_MASK                0x00003fff
#define CPU_MAC_STATS_CFG_W1_CFG_GMAC_STATS_MTU2_MASK                0x3fff0000

/* ################################################################################
 * # CpuMacInterruptFunc Definition */
#define CPU_MAC_INTERRUPT_FUNC_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_INTERRUPT_FUNC_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_INTERRUPT_FUNC_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_INTERRUPT_FUNC_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC_BIT 0

#define CPU_MAC_INTERRUPT_FUNC_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC_MASK 0x000000ff
#define CPU_MAC_INTERRUPT_FUNC_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC_MASK 0x000000ff
#define CPU_MAC_INTERRUPT_FUNC_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC_MASK 0x000000ff
#define CPU_MAC_INTERRUPT_FUNC_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC_MASK 0x000000ff

/* ################################################################################
 * # CpuMacInterruptNormal Definition */
#define CPU_MAC_INTERRUPT_NORMAL_W0_VALUE_SET0_CPU_MAC_INTERRUPT_NORMAL_BIT 0
#define CPU_MAC_INTERRUPT_NORMAL_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_NORMAL_BIT 0
#define CPU_MAC_INTERRUPT_NORMAL_W2_MASK_SET0_CPU_MAC_INTERRUPT_NORMAL_BIT 0
#define CPU_MAC_INTERRUPT_NORMAL_W3_MASK_RESET0_CPU_MAC_INTERRUPT_NORMAL_BIT 0

#define CPU_MAC_INTERRUPT_NORMAL_W0_VALUE_SET0_CPU_MAC_INTERRUPT_NORMAL_MASK 0xffffffff
#define CPU_MAC_INTERRUPT_NORMAL_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_NORMAL_MASK 0xffffffff
#define CPU_MAC_INTERRUPT_NORMAL_W2_MASK_SET0_CPU_MAC_INTERRUPT_NORMAL_MASK 0xffffffff
#define CPU_MAC_INTERRUPT_NORMAL_W3_MASK_RESET0_CPU_MAC_INTERRUPT_NORMAL_MASK 0xffffffff

/* ################################################################################
 * # CpuMacFifoStatus Definition */
#define CPU_MAC_FIFO_STATUS_W0_EXT_RAM_RD_TRACK_FIFO_FIFO_DEPTH_BIT  16
#define CPU_MAC_FIFO_STATUS_W0_RX_PKT_DATA_FIFO_FIFO_DEPTH_BIT       20
#define CPU_MAC_FIFO_STATUS_W0_RX_PKT_MSG_FIFO_FIFO_DEPTH_BIT        0
#define CPU_MAC_FIFO_STATUS_W0_TX_DESC_ACK_FIFO_FIFO_DEPTH_BIT       10
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC0_ACK_FIFO_FIFO_DEPTH_BIT      12
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC0_CFG_FIFO_FIFO_DEPTH_BIT      6
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC1_ACK_FIFO_FIFO_DEPTH_BIT      24
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC1_CFG_FIFO_FIFO_DEPTH_BIT      18
#define CPU_MAC_FIFO_STATUS_W1_TX_DESC_CFG_FIFO_FIFO_DEPTH_BIT       0
#define CPU_MAC_FIFO_STATUS_W2_RX_PKT_FIFO_FIFO_DEPTH_BIT            0
#define CPU_MAC_FIFO_STATUS_W2_TX_PKT_FIFO_FIFO_DEPTH_BIT            16

#define CPU_MAC_FIFO_STATUS_W0_EXT_RAM_RD_TRACK_FIFO_FIFO_DEPTH_MASK 0x000f0000
#define CPU_MAC_FIFO_STATUS_W0_RX_PKT_DATA_FIFO_FIFO_DEPTH_MASK      0xfff00000
#define CPU_MAC_FIFO_STATUS_W0_RX_PKT_MSG_FIFO_FIFO_DEPTH_MASK       0x000003ff
#define CPU_MAC_FIFO_STATUS_W0_TX_DESC_ACK_FIFO_FIFO_DEPTH_MASK      0x0000fc00
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC0_ACK_FIFO_FIFO_DEPTH_MASK     0x0003f000
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC0_CFG_FIFO_FIFO_DEPTH_MASK     0x00000fc0
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC1_ACK_FIFO_FIFO_DEPTH_MASK     0x3f000000
#define CPU_MAC_FIFO_STATUS_W1_RX_DESC1_CFG_FIFO_FIFO_DEPTH_MASK     0x00fc0000
#define CPU_MAC_FIFO_STATUS_W1_TX_DESC_CFG_FIFO_FIFO_DEPTH_MASK      0x0000003f
#define CPU_MAC_FIFO_STATUS_W2_RX_PKT_FIFO_FIFO_DEPTH_MASK           0x000003ff
#define CPU_MAC_FIFO_STATUS_W2_TX_PKT_FIFO_FIFO_DEPTH_MASK           0x03ff0000

/* ################################################################################
 * # CpuMacDescCfg1 Definition
 */
#define CPU_MAC_DESC_CFG1_W0_CFG_DESC_SLICE_EN                       2
#define CPU_MAC_DESC_CFG1_W0_CFG_RX_DESC_DONE_INTR_TIMER_EN          0
#define CPU_MAC_DESC_CFG1_W0_CFG_TX_DESC_DONE_INTR_TIMER_EN          1
#define CPU_MAC_DESC_CFG1_W1_CFG_RX_DESC_DONE_TIMER_THRD             0
#define CPU_MAC_DESC_CFG1_W2_CFG_TX_DESC_DONE_TIMER_THRD             0

#define CPU_MAC_DESC_CFG1_W0_CFG_DESC_SLICE_EN_MASK                  0x00000004
#define CPU_MAC_DESC_CFG1_W0_CFG_RX_DESC_DONE_INTR_TIMER_EN_MASK     0x00000001
#define CPU_MAC_DESC_CFG1_W0_CFG_TX_DESC_DONE_INTR_TIMER_EN_MASK     0x00000002
#define CPU_MAC_DESC_CFG1_W1_CFG_RX_DESC_DONE_TIMER_THRD_MASK        0xffffffff
#define CPU_MAC_DESC_CFG1_W2_CFG_TX_DESC_DONE_TIMER_THRD_MASK        0xffffffff

/* ################################################################################
 * # CpuMacInterruptFunc0 Definition
 */
#define CPU_MAC_INTERRUPT_FUNC0_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC0 0
#define CPU_MAC_INTERRUPT_FUNC0_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC0 0
#define CPU_MAC_INTERRUPT_FUNC0_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC0 0
#define CPU_MAC_INTERRUPT_FUNC0_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC0 0

#define CPU_MAC_INTERRUPT_FUNC0_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC0_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC0_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC0_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC0_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC0_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC0_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC0_MASK 0x00000003

/* ################################################################################
 * # CpuMacInterruptFunc1 Definition
 */
#define CPU_MAC_INTERRUPT_FUNC1_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC1 0
#define CPU_MAC_INTERRUPT_FUNC1_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC1 0
#define CPU_MAC_INTERRUPT_FUNC1_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC1 0
#define CPU_MAC_INTERRUPT_FUNC1_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC1 0

#define CPU_MAC_INTERRUPT_FUNC1_W0_VALUE_SET0_CPU_MAC_INTERRUPT_FUNC1_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC1_W1_VALUE_RESET0_CPU_MAC_INTERRUPT_FUNC1_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC1_W2_MASK_SET0_CPU_MAC_INTERRUPT_FUNC1_MASK 0x00000003
#define CPU_MAC_INTERRUPT_FUNC1_W3_MASK_RESET0_CPU_MAC_INTERRUPT_FUNC1_MASK 0x00000003

struct CpuMac_mems {
	u32 TxPktFifo0[3];	/* 0x00004000 */
	u32 TxPktFifo0_rsv3;
	u32 TxPktFifo1[3];	/* 0x00004010 */
	u32 TxPktFifo1_rsv3;
	u32 TxPktFifo2[3];	/* 0x00004020 */
	u32 TxPktFifo2_rsv3;
	u32 TxPktFifo3[3];	/* 0x00004030 */
	u32 TxPktFifo3_rsv3;
	u32 TxPktFifo4[3];	/* 0x00004040 */
	u32 TxPktFifo4_rsv3;
	u32 TxPktFifo5[3];	/* 0x00004050 */
	u32 TxPktFifo5_rsv3;
	u32 TxPktFifo6[3];	/* 0x00004060 */
	u32 TxPktFifo6_rsv3;
	u32 TxPktFifo7[3];	/* 0x00004070 */
	u32 TxPktFifo7_rsv3;
	u32 TxPktFifo8[3];	/* 0x00004080 */
	u32 TxPktFifo8_rsv3;
	u32 TxPktFifo9[3];	/* 0x00004090 */
	u32 TxPktFifo9_rsv3;
	u32 TxPktFifo10[3];	/* 0x000040a0 */
	u32 TxPktFifo10_rsv3;
	u32 TxPktFifo11[3];	/* 0x000040b0 */
	u32 TxPktFifo11_rsv3;
	u32 TxPktFifo12[3];	/* 0x000040c0 */
	u32 TxPktFifo12_rsv3;
	u32 TxPktFifo13[3];	/* 0x000040d0 */
	u32 TxPktFifo13_rsv3;
	u32 TxPktFifo14[3];	/* 0x000040e0 */
	u32 TxPktFifo14_rsv3;
	u32 TxPktFifo15[3];	/* 0x000040f0 */
	u32 TxPktFifo15_rsv3;
	u32 TxPktFifo16[3];	/* 0x00004100 */
	u32 TxPktFifo16_rsv3;
	u32 TxPktFifo17[3];	/* 0x00004110 */
	u32 TxPktFifo17_rsv3;
	u32 TxPktFifo18[3];	/* 0x00004120 */
	u32 TxPktFifo18_rsv3;
	u32 TxPktFifo19[3];	/* 0x00004130 */
	u32 TxPktFifo19_rsv3;
	u32 TxPktFifo20[3];	/* 0x00004140 */
	u32 TxPktFifo20_rsv3;
	u32 TxPktFifo21[3];	/* 0x00004150 */
	u32 TxPktFifo21_rsv3;
	u32 TxPktFifo22[3];	/* 0x00004160 */
	u32 TxPktFifo22_rsv3;
	u32 TxPktFifo23[3];	/* 0x00004170 */
	u32 TxPktFifo23_rsv3;
	u32 TxPktFifo24[3];	/* 0x00004180 */
	u32 TxPktFifo24_rsv3;
	u32 TxPktFifo25[3];	/* 0x00004190 */
	u32 TxPktFifo25_rsv3;
	u32 TxPktFifo26[3];	/* 0x000041a0 */
	u32 TxPktFifo26_rsv3;
	u32 TxPktFifo27[3];	/* 0x000041b0 */
	u32 TxPktFifo27_rsv3;
	u32 TxPktFifo28[3];	/* 0x000041c0 */
	u32 TxPktFifo28_rsv3;
	u32 TxPktFifo29[3];	/* 0x000041d0 */
	u32 TxPktFifo29_rsv3;
	u32 TxPktFifo30[3];	/* 0x000041e0 */
	u32 TxPktFifo30_rsv3;
	u32 TxPktFifo31[3];	/* 0x000041f0 */
	u32 TxPktFifo31_rsv3;
	u32 TxPktFifo32[3];	/* 0x00004200 */
	u32 TxPktFifo32_rsv3;
	u32 TxPktFifo33[3];	/* 0x00004210 */
	u32 TxPktFifo33_rsv3;
	u32 TxPktFifo34[3];	/* 0x00004220 */
	u32 TxPktFifo34_rsv3;
	u32 TxPktFifo35[3];	/* 0x00004230 */
	u32 TxPktFifo35_rsv3;
	u32 TxPktFifo36[3];	/* 0x00004240 */
	u32 TxPktFifo36_rsv3;
	u32 TxPktFifo37[3];	/* 0x00004250 */
	u32 TxPktFifo37_rsv3;
	u32 TxPktFifo38[3];	/* 0x00004260 */
	u32 TxPktFifo38_rsv3;
	u32 TxPktFifo39[3];	/* 0x00004270 */
	u32 TxPktFifo39_rsv3;
	u32 TxPktFifo40[3];	/* 0x00004280 */
	u32 TxPktFifo40_rsv3;
	u32 TxPktFifo41[3];	/* 0x00004290 */
	u32 TxPktFifo41_rsv3;
	u32 TxPktFifo42[3];	/* 0x000042a0 */
	u32 TxPktFifo42_rsv3;
	u32 TxPktFifo43[3];	/* 0x000042b0 */
	u32 TxPktFifo43_rsv3;
	u32 TxPktFifo44[3];	/* 0x000042c0 */
	u32 TxPktFifo44_rsv3;
	u32 TxPktFifo45[3];	/* 0x000042d0 */
	u32 TxPktFifo45_rsv3;
	u32 TxPktFifo46[3];	/* 0x000042e0 */
	u32 TxPktFifo46_rsv3;
	u32 TxPktFifo47[3];	/* 0x000042f0 */
	u32 TxPktFifo47_rsv3;
	u32 TxPktFifo48[3];	/* 0x00004300 */
	u32 TxPktFifo48_rsv3;
	u32 TxPktFifo49[3];	/* 0x00004310 */
	u32 TxPktFifo49_rsv3;
	u32 TxPktFifo50[3];	/* 0x00004320 */
	u32 TxPktFifo50_rsv3;
	u32 TxPktFifo51[3];	/* 0x00004330 */
	u32 TxPktFifo51_rsv3;
	u32 TxPktFifo52[3];	/* 0x00004340 */
	u32 TxPktFifo52_rsv3;
	u32 TxPktFifo53[3];	/* 0x00004350 */
	u32 TxPktFifo53_rsv3;
	u32 TxPktFifo54[3];	/* 0x00004360 */
	u32 TxPktFifo54_rsv3;
	u32 TxPktFifo55[3];	/* 0x00004370 */
	u32 TxPktFifo55_rsv3;
	u32 TxPktFifo56[3];	/* 0x00004380 */
	u32 TxPktFifo56_rsv3;
	u32 TxPktFifo57[3];	/* 0x00004390 */
	u32 TxPktFifo57_rsv3;
	u32 TxPktFifo58[3];	/* 0x000043a0 */
	u32 TxPktFifo58_rsv3;
	u32 TxPktFifo59[3];	/* 0x000043b0 */
	u32 TxPktFifo59_rsv3;
	u32 TxPktFifo60[3];	/* 0x000043c0 */
	u32 TxPktFifo60_rsv3;
	u32 TxPktFifo61[3];	/* 0x000043d0 */
	u32 TxPktFifo61_rsv3;
	u32 TxPktFifo62[3];	/* 0x000043e0 */
	u32 TxPktFifo62_rsv3;
	u32 TxPktFifo63[3];	/* 0x000043f0 */
	u32 TxPktFifo63_rsv3;
	u32 TxPktFifo64[3];	/* 0x00004400 */
	u32 TxPktFifo64_rsv3;
	u32 TxPktFifo65[3];	/* 0x00004410 */
	u32 TxPktFifo65_rsv3;
	u32 TxPktFifo66[3];	/* 0x00004420 */
	u32 TxPktFifo66_rsv3;
	u32 TxPktFifo67[3];	/* 0x00004430 */
	u32 TxPktFifo67_rsv3;
	u32 TxPktFifo68[3];	/* 0x00004440 */
	u32 TxPktFifo68_rsv3;
	u32 TxPktFifo69[3];	/* 0x00004450 */
	u32 TxPktFifo69_rsv3;
	u32 TxPktFifo70[3];	/* 0x00004460 */
	u32 TxPktFifo70_rsv3;
	u32 TxPktFifo71[3];	/* 0x00004470 */
	u32 TxPktFifo71_rsv3;
	u32 TxPktFifo72[3];	/* 0x00004480 */
	u32 TxPktFifo72_rsv3;
	u32 TxPktFifo73[3];	/* 0x00004490 */
	u32 TxPktFifo73_rsv3;
	u32 TxPktFifo74[3];	/* 0x000044a0 */
	u32 TxPktFifo74_rsv3;
	u32 TxPktFifo75[3];	/* 0x000044b0 */
	u32 TxPktFifo75_rsv3;
	u32 TxPktFifo76[3];	/* 0x000044c0 */
	u32 TxPktFifo76_rsv3;
	u32 TxPktFifo77[3];	/* 0x000044d0 */
	u32 TxPktFifo77_rsv3;
	u32 TxPktFifo78[3];	/* 0x000044e0 */
	u32 TxPktFifo78_rsv3;
	u32 TxPktFifo79[3];	/* 0x000044f0 */
	u32 TxPktFifo79_rsv3;
	u32 TxPktFifo80[3];	/* 0x00004500 */
	u32 TxPktFifo80_rsv3;
	u32 TxPktFifo81[3];	/* 0x00004510 */
	u32 TxPktFifo81_rsv3;
	u32 TxPktFifo82[3];	/* 0x00004520 */
	u32 TxPktFifo82_rsv3;
	u32 TxPktFifo83[3];	/* 0x00004530 */
	u32 TxPktFifo83_rsv3;
	u32 TxPktFifo84[3];	/* 0x00004540 */
	u32 TxPktFifo84_rsv3;
	u32 TxPktFifo85[3];	/* 0x00004550 */
	u32 TxPktFifo85_rsv3;
	u32 TxPktFifo86[3];	/* 0x00004560 */
	u32 TxPktFifo86_rsv3;
	u32 TxPktFifo87[3];	/* 0x00004570 */
	u32 TxPktFifo87_rsv3;
	u32 TxPktFifo88[3];	/* 0x00004580 */
	u32 TxPktFifo88_rsv3;
	u32 TxPktFifo89[3];	/* 0x00004590 */
	u32 TxPktFifo89_rsv3;
	u32 TxPktFifo90[3];	/* 0x000045a0 */
	u32 TxPktFifo90_rsv3;
	u32 TxPktFifo91[3];	/* 0x000045b0 */
	u32 TxPktFifo91_rsv3;
	u32 TxPktFifo92[3];	/* 0x000045c0 */
	u32 TxPktFifo92_rsv3;
	u32 TxPktFifo93[3];	/* 0x000045d0 */
	u32 TxPktFifo93_rsv3;
	u32 TxPktFifo94[3];	/* 0x000045e0 */
	u32 TxPktFifo94_rsv3;
	u32 TxPktFifo95[3];	/* 0x000045f0 */
	u32 TxPktFifo95_rsv3;
	u32 TxPktFifo96[3];	/* 0x00004600 */
	u32 TxPktFifo96_rsv3;
	u32 TxPktFifo97[3];	/* 0x00004610 */
	u32 TxPktFifo97_rsv3;
	u32 TxPktFifo98[3];	/* 0x00004620 */
	u32 TxPktFifo98_rsv3;
	u32 TxPktFifo99[3];	/* 0x00004630 */
	u32 TxPktFifo99_rsv3;
	u32 TxPktFifo100[3];	/* 0x00004640 */
	u32 TxPktFifo100_rsv3;
	u32 TxPktFifo101[3];	/* 0x00004650 */
	u32 TxPktFifo101_rsv3;
	u32 TxPktFifo102[3];	/* 0x00004660 */
	u32 TxPktFifo102_rsv3;
	u32 TxPktFifo103[3];	/* 0x00004670 */
	u32 TxPktFifo103_rsv3;
	u32 TxPktFifo104[3];	/* 0x00004680 */
	u32 TxPktFifo104_rsv3;
	u32 TxPktFifo105[3];	/* 0x00004690 */
	u32 TxPktFifo105_rsv3;
	u32 TxPktFifo106[3];	/* 0x000046a0 */
	u32 TxPktFifo106_rsv3;
	u32 TxPktFifo107[3];	/* 0x000046b0 */
	u32 TxPktFifo107_rsv3;
	u32 TxPktFifo108[3];	/* 0x000046c0 */
	u32 TxPktFifo108_rsv3;
	u32 TxPktFifo109[3];	/* 0x000046d0 */
	u32 TxPktFifo109_rsv3;
	u32 TxPktFifo110[3];	/* 0x000046e0 */
	u32 TxPktFifo110_rsv3;
	u32 TxPktFifo111[3];	/* 0x000046f0 */
	u32 TxPktFifo111_rsv3;
	u32 TxPktFifo112[3];	/* 0x00004700 */
	u32 TxPktFifo112_rsv3;
	u32 TxPktFifo113[3];	/* 0x00004710 */
	u32 TxPktFifo113_rsv3;
	u32 TxPktFifo114[3];	/* 0x00004720 */
	u32 TxPktFifo114_rsv3;
	u32 TxPktFifo115[3];	/* 0x00004730 */
	u32 TxPktFifo115_rsv3;
	u32 TxPktFifo116[3];	/* 0x00004740 */
	u32 TxPktFifo116_rsv3;
	u32 TxPktFifo117[3];	/* 0x00004750 */
	u32 TxPktFifo117_rsv3;
	u32 TxPktFifo118[3];	/* 0x00004760 */
	u32 TxPktFifo118_rsv3;
	u32 TxPktFifo119[3];	/* 0x00004770 */
	u32 TxPktFifo119_rsv3;
	u32 TxPktFifo120[3];	/* 0x00004780 */
	u32 TxPktFifo120_rsv3;
	u32 TxPktFifo121[3];	/* 0x00004790 */
	u32 TxPktFifo121_rsv3;
	u32 TxPktFifo122[3];	/* 0x000047a0 */
	u32 TxPktFifo122_rsv3;
	u32 TxPktFifo123[3];	/* 0x000047b0 */
	u32 TxPktFifo123_rsv3;
	u32 TxPktFifo124[3];	/* 0x000047c0 */
	u32 TxPktFifo124_rsv3;
	u32 TxPktFifo125[3];	/* 0x000047d0 */
	u32 TxPktFifo125_rsv3;
	u32 TxPktFifo126[3];	/* 0x000047e0 */
	u32 TxPktFifo126_rsv3;
	u32 TxPktFifo127[3];	/* 0x000047f0 */
	u32 TxPktFifo127_rsv3;
	u32 TxPktFifo128[3];	/* 0x00004800 */
	u32 TxPktFifo128_rsv3;
	u32 TxPktFifo129[3];	/* 0x00004810 */
	u32 TxPktFifo129_rsv3;
	u32 TxPktFifo130[3];	/* 0x00004820 */
	u32 TxPktFifo130_rsv3;
	u32 TxPktFifo131[3];	/* 0x00004830 */
	u32 TxPktFifo131_rsv3;
	u32 TxPktFifo132[3];	/* 0x00004840 */
	u32 TxPktFifo132_rsv3;
	u32 TxPktFifo133[3];	/* 0x00004850 */
	u32 TxPktFifo133_rsv3;
	u32 TxPktFifo134[3];	/* 0x00004860 */
	u32 TxPktFifo134_rsv3;
	u32 TxPktFifo135[3];	/* 0x00004870 */
	u32 TxPktFifo135_rsv3;
	u32 TxPktFifo136[3];	/* 0x00004880 */
	u32 TxPktFifo136_rsv3;
	u32 TxPktFifo137[3];	/* 0x00004890 */
	u32 TxPktFifo137_rsv3;
	u32 TxPktFifo138[3];	/* 0x000048a0 */
	u32 TxPktFifo138_rsv3;
	u32 TxPktFifo139[3];	/* 0x000048b0 */
	u32 TxPktFifo139_rsv3;
	u32 TxPktFifo140[3];	/* 0x000048c0 */
	u32 TxPktFifo140_rsv3;
	u32 TxPktFifo141[3];	/* 0x000048d0 */
	u32 TxPktFifo141_rsv3;
	u32 TxPktFifo142[3];	/* 0x000048e0 */
	u32 TxPktFifo142_rsv3;
	u32 TxPktFifo143[3];	/* 0x000048f0 */
	u32 TxPktFifo143_rsv3;
	u32 TxPktFifo144[3];	/* 0x00004900 */
	u32 TxPktFifo144_rsv3;
	u32 TxPktFifo145[3];	/* 0x00004910 */
	u32 TxPktFifo145_rsv3;
	u32 TxPktFifo146[3];	/* 0x00004920 */
	u32 TxPktFifo146_rsv3;
	u32 TxPktFifo147[3];	/* 0x00004930 */
	u32 TxPktFifo147_rsv3;
	u32 TxPktFifo148[3];	/* 0x00004940 */
	u32 TxPktFifo148_rsv3;
	u32 TxPktFifo149[3];	/* 0x00004950 */
	u32 TxPktFifo149_rsv3;
	u32 TxPktFifo150[3];	/* 0x00004960 */
	u32 TxPktFifo150_rsv3;
	u32 TxPktFifo151[3];	/* 0x00004970 */
	u32 TxPktFifo151_rsv3;
	u32 TxPktFifo152[3];	/* 0x00004980 */
	u32 TxPktFifo152_rsv3;
	u32 TxPktFifo153[3];	/* 0x00004990 */
	u32 TxPktFifo153_rsv3;
	u32 TxPktFifo154[3];	/* 0x000049a0 */
	u32 TxPktFifo154_rsv3;
	u32 TxPktFifo155[3];	/* 0x000049b0 */
	u32 TxPktFifo155_rsv3;
	u32 TxPktFifo156[3];	/* 0x000049c0 */
	u32 TxPktFifo156_rsv3;
	u32 TxPktFifo157[3];	/* 0x000049d0 */
	u32 TxPktFifo157_rsv3;
	u32 TxPktFifo158[3];	/* 0x000049e0 */
	u32 TxPktFifo158_rsv3;
	u32 TxPktFifo159[3];	/* 0x000049f0 */
	u32 TxPktFifo159_rsv3;
	u32 TxPktFifo160[3];	/* 0x00004a00 */
	u32 TxPktFifo160_rsv3;
	u32 TxPktFifo161[3];	/* 0x00004a10 */
	u32 TxPktFifo161_rsv3;
	u32 TxPktFifo162[3];	/* 0x00004a20 */
	u32 TxPktFifo162_rsv3;
	u32 TxPktFifo163[3];	/* 0x00004a30 */
	u32 TxPktFifo163_rsv3;
	u32 TxPktFifo164[3];	/* 0x00004a40 */
	u32 TxPktFifo164_rsv3;
	u32 TxPktFifo165[3];	/* 0x00004a50 */
	u32 TxPktFifo165_rsv3;
	u32 TxPktFifo166[3];	/* 0x00004a60 */
	u32 TxPktFifo166_rsv3;
	u32 TxPktFifo167[3];	/* 0x00004a70 */
	u32 TxPktFifo167_rsv3;
	u32 TxPktFifo168[3];	/* 0x00004a80 */
	u32 TxPktFifo168_rsv3;
	u32 TxPktFifo169[3];	/* 0x00004a90 */
	u32 TxPktFifo169_rsv3;
	u32 TxPktFifo170[3];	/* 0x00004aa0 */
	u32 TxPktFifo170_rsv3;
	u32 TxPktFifo171[3];	/* 0x00004ab0 */
	u32 TxPktFifo171_rsv3;
	u32 TxPktFifo172[3];	/* 0x00004ac0 */
	u32 TxPktFifo172_rsv3;
	u32 TxPktFifo173[3];	/* 0x00004ad0 */
	u32 TxPktFifo173_rsv3;
	u32 TxPktFifo174[3];	/* 0x00004ae0 */
	u32 TxPktFifo174_rsv3;
	u32 TxPktFifo175[3];	/* 0x00004af0 */
	u32 TxPktFifo175_rsv3;
	u32 TxPktFifo176[3];	/* 0x00004b00 */
	u32 TxPktFifo176_rsv3;
	u32 TxPktFifo177[3];	/* 0x00004b10 */
	u32 TxPktFifo177_rsv3;
	u32 TxPktFifo178[3];	/* 0x00004b20 */
	u32 TxPktFifo178_rsv3;
	u32 TxPktFifo179[3];	/* 0x00004b30 */
	u32 TxPktFifo179_rsv3;
	u32 TxPktFifo180[3];	/* 0x00004b40 */
	u32 TxPktFifo180_rsv3;
	u32 TxPktFifo181[3];	/* 0x00004b50 */
	u32 TxPktFifo181_rsv3;
	u32 TxPktFifo182[3];	/* 0x00004b60 */
	u32 TxPktFifo182_rsv3;
	u32 TxPktFifo183[3];	/* 0x00004b70 */
	u32 TxPktFifo183_rsv3;
	u32 TxPktFifo184[3];	/* 0x00004b80 */
	u32 TxPktFifo184_rsv3;
	u32 TxPktFifo185[3];	/* 0x00004b90 */
	u32 TxPktFifo185_rsv3;
	u32 TxPktFifo186[3];	/* 0x00004ba0 */
	u32 TxPktFifo186_rsv3;
	u32 TxPktFifo187[3];	/* 0x00004bb0 */
	u32 TxPktFifo187_rsv3;
	u32 TxPktFifo188[3];	/* 0x00004bc0 */
	u32 TxPktFifo188_rsv3;
	u32 TxPktFifo189[3];	/* 0x00004bd0 */
	u32 TxPktFifo189_rsv3;
	u32 TxPktFifo190[3];	/* 0x00004be0 */
	u32 TxPktFifo190_rsv3;
	u32 TxPktFifo191[3];	/* 0x00004bf0 */
	u32 TxPktFifo191_rsv3;
	u32 TxPktFifo192[3];	/* 0x00004c00 */
	u32 TxPktFifo192_rsv3;
	u32 TxPktFifo193[3];	/* 0x00004c10 */
	u32 TxPktFifo193_rsv3;
	u32 TxPktFifo194[3];	/* 0x00004c20 */
	u32 TxPktFifo194_rsv3;
	u32 TxPktFifo195[3];	/* 0x00004c30 */
	u32 TxPktFifo195_rsv3;
	u32 TxPktFifo196[3];	/* 0x00004c40 */
	u32 TxPktFifo196_rsv3;
	u32 TxPktFifo197[3];	/* 0x00004c50 */
	u32 TxPktFifo197_rsv3;
	u32 TxPktFifo198[3];	/* 0x00004c60 */
	u32 TxPktFifo198_rsv3;
	u32 TxPktFifo199[3];	/* 0x00004c70 */
	u32 TxPktFifo199_rsv3;
	u32 TxPktFifo200[3];	/* 0x00004c80 */
	u32 TxPktFifo200_rsv3;
	u32 TxPktFifo201[3];	/* 0x00004c90 */
	u32 TxPktFifo201_rsv3;
	u32 TxPktFifo202[3];	/* 0x00004ca0 */
	u32 TxPktFifo202_rsv3;
	u32 TxPktFifo203[3];	/* 0x00004cb0 */
	u32 TxPktFifo203_rsv3;
	u32 TxPktFifo204[3];	/* 0x00004cc0 */
	u32 TxPktFifo204_rsv3;
	u32 TxPktFifo205[3];	/* 0x00004cd0 */
	u32 TxPktFifo205_rsv3;
	u32 TxPktFifo206[3];	/* 0x00004ce0 */
	u32 TxPktFifo206_rsv3;
	u32 TxPktFifo207[3];	/* 0x00004cf0 */
	u32 TxPktFifo207_rsv3;
	u32 TxPktFifo208[3];	/* 0x00004d00 */
	u32 TxPktFifo208_rsv3;
	u32 TxPktFifo209[3];	/* 0x00004d10 */
	u32 TxPktFifo209_rsv3;
	u32 TxPktFifo210[3];	/* 0x00004d20 */
	u32 TxPktFifo210_rsv3;
	u32 TxPktFifo211[3];	/* 0x00004d30 */
	u32 TxPktFifo211_rsv3;
	u32 TxPktFifo212[3];	/* 0x00004d40 */
	u32 TxPktFifo212_rsv3;
	u32 TxPktFifo213[3];	/* 0x00004d50 */
	u32 TxPktFifo213_rsv3;
	u32 TxPktFifo214[3];	/* 0x00004d60 */
	u32 TxPktFifo214_rsv3;
	u32 TxPktFifo215[3];	/* 0x00004d70 */
	u32 TxPktFifo215_rsv3;
	u32 TxPktFifo216[3];	/* 0x00004d80 */
	u32 TxPktFifo216_rsv3;
	u32 TxPktFifo217[3];	/* 0x00004d90 */
	u32 TxPktFifo217_rsv3;
	u32 TxPktFifo218[3];	/* 0x00004da0 */
	u32 TxPktFifo218_rsv3;
	u32 TxPktFifo219[3];	/* 0x00004db0 */
	u32 TxPktFifo219_rsv3;
	u32 TxPktFifo220[3];	/* 0x00004dc0 */
	u32 TxPktFifo220_rsv3;
	u32 TxPktFifo221[3];	/* 0x00004dd0 */
	u32 TxPktFifo221_rsv3;
	u32 TxPktFifo222[3];	/* 0x00004de0 */
	u32 TxPktFifo222_rsv3;
	u32 TxPktFifo223[3];	/* 0x00004df0 */
	u32 TxPktFifo223_rsv3;
	u32 TxPktFifo224[3];	/* 0x00004e00 */
	u32 TxPktFifo224_rsv3;
	u32 TxPktFifo225[3];	/* 0x00004e10 */
	u32 TxPktFifo225_rsv3;
	u32 TxPktFifo226[3];	/* 0x00004e20 */
	u32 TxPktFifo226_rsv3;
	u32 TxPktFifo227[3];	/* 0x00004e30 */
	u32 TxPktFifo227_rsv3;
	u32 TxPktFifo228[3];	/* 0x00004e40 */
	u32 TxPktFifo228_rsv3;
	u32 TxPktFifo229[3];	/* 0x00004e50 */
	u32 TxPktFifo229_rsv3;
	u32 TxPktFifo230[3];	/* 0x00004e60 */
	u32 TxPktFifo230_rsv3;
	u32 TxPktFifo231[3];	/* 0x00004e70 */
	u32 TxPktFifo231_rsv3;
	u32 TxPktFifo232[3];	/* 0x00004e80 */
	u32 TxPktFifo232_rsv3;
	u32 TxPktFifo233[3];	/* 0x00004e90 */
	u32 TxPktFifo233_rsv3;
	u32 TxPktFifo234[3];	/* 0x00004ea0 */
	u32 TxPktFifo234_rsv3;
	u32 TxPktFifo235[3];	/* 0x00004eb0 */
	u32 TxPktFifo235_rsv3;
	u32 TxPktFifo236[3];	/* 0x00004ec0 */
	u32 TxPktFifo236_rsv3;
	u32 TxPktFifo237[3];	/* 0x00004ed0 */
	u32 TxPktFifo237_rsv3;
	u32 TxPktFifo238[3];	/* 0x00004ee0 */
	u32 TxPktFifo238_rsv3;
	u32 TxPktFifo239[3];	/* 0x00004ef0 */
	u32 TxPktFifo239_rsv3;
	u32 TxPktFifo240[3];	/* 0x00004f00 */
	u32 TxPktFifo240_rsv3;
	u32 TxPktFifo241[3];	/* 0x00004f10 */
	u32 TxPktFifo241_rsv3;
	u32 TxPktFifo242[3];	/* 0x00004f20 */
	u32 TxPktFifo242_rsv3;
	u32 TxPktFifo243[3];	/* 0x00004f30 */
	u32 TxPktFifo243_rsv3;
	u32 TxPktFifo244[3];	/* 0x00004f40 */
	u32 TxPktFifo244_rsv3;
	u32 TxPktFifo245[3];	/* 0x00004f50 */
	u32 TxPktFifo245_rsv3;
	u32 TxPktFifo246[3];	/* 0x00004f60 */
	u32 TxPktFifo246_rsv3;
	u32 TxPktFifo247[3];	/* 0x00004f70 */
	u32 TxPktFifo247_rsv3;
	u32 TxPktFifo248[3];	/* 0x00004f80 */
	u32 TxPktFifo248_rsv3;
	u32 TxPktFifo249[3];	/* 0x00004f90 */
	u32 TxPktFifo249_rsv3;
	u32 TxPktFifo250[3];	/* 0x00004fa0 */
	u32 TxPktFifo250_rsv3;
	u32 TxPktFifo251[3];	/* 0x00004fb0 */
	u32 TxPktFifo251_rsv3;
	u32 TxPktFifo252[3];	/* 0x00004fc0 */
	u32 TxPktFifo252_rsv3;
	u32 TxPktFifo253[3];	/* 0x00004fd0 */
	u32 TxPktFifo253_rsv3;
	u32 TxPktFifo254[3];	/* 0x00004fe0 */
	u32 TxPktFifo254_rsv3;
	u32 TxPktFifo255[3];	/* 0x00004ff0 */
	u32 TxPktFifo255_rsv3;
	u32 TxPktFifo256[3];	/* 0x00005000 */
	u32 TxPktFifo256_rsv3;
	u32 TxPktFifo257[3];	/* 0x00005010 */
	u32 TxPktFifo257_rsv3;
	u32 TxPktFifo258[3];	/* 0x00005020 */
	u32 TxPktFifo258_rsv3;
	u32 TxPktFifo259[3];	/* 0x00005030 */
	u32 TxPktFifo259_rsv3;
	u32 TxPktFifo260[3];	/* 0x00005040 */
	u32 TxPktFifo260_rsv3;
	u32 TxPktFifo261[3];	/* 0x00005050 */
	u32 TxPktFifo261_rsv3;
	u32 TxPktFifo262[3];	/* 0x00005060 */
	u32 TxPktFifo262_rsv3;
	u32 TxPktFifo263[3];	/* 0x00005070 */
	u32 TxPktFifo263_rsv3;
	u32 TxPktFifo264[3];	/* 0x00005080 */
	u32 TxPktFifo264_rsv3;
	u32 TxPktFifo265[3];	/* 0x00005090 */
	u32 TxPktFifo265_rsv3;
	u32 TxPktFifo266[3];	/* 0x000050a0 */
	u32 TxPktFifo266_rsv3;
	u32 TxPktFifo267[3];	/* 0x000050b0 */
	u32 TxPktFifo267_rsv3;
	u32 TxPktFifo268[3];	/* 0x000050c0 */
	u32 TxPktFifo268_rsv3;
	u32 TxPktFifo269[3];	/* 0x000050d0 */
	u32 TxPktFifo269_rsv3;
	u32 TxPktFifo270[3];	/* 0x000050e0 */
	u32 TxPktFifo270_rsv3;
	u32 TxPktFifo271[3];	/* 0x000050f0 */
	u32 TxPktFifo271_rsv3;
	u32 TxPktFifo272[3];	/* 0x00005100 */
	u32 TxPktFifo272_rsv3;
	u32 TxPktFifo273[3];	/* 0x00005110 */
	u32 TxPktFifo273_rsv3;
	u32 TxPktFifo274[3];	/* 0x00005120 */
	u32 TxPktFifo274_rsv3;
	u32 TxPktFifo275[3];	/* 0x00005130 */
	u32 TxPktFifo275_rsv3;
	u32 TxPktFifo276[3];	/* 0x00005140 */
	u32 TxPktFifo276_rsv3;
	u32 TxPktFifo277[3];	/* 0x00005150 */
	u32 TxPktFifo277_rsv3;
	u32 TxPktFifo278[3];	/* 0x00005160 */
	u32 TxPktFifo278_rsv3;
	u32 TxPktFifo279[3];	/* 0x00005170 */
	u32 TxPktFifo279_rsv3;
	u32 TxPktFifo280[3];	/* 0x00005180 */
	u32 TxPktFifo280_rsv3;
	u32 TxPktFifo281[3];	/* 0x00005190 */
	u32 TxPktFifo281_rsv3;
	u32 TxPktFifo282[3];	/* 0x000051a0 */
	u32 TxPktFifo282_rsv3;
	u32 TxPktFifo283[3];	/* 0x000051b0 */
	u32 TxPktFifo283_rsv3;
	u32 TxPktFifo284[3];	/* 0x000051c0 */
	u32 TxPktFifo284_rsv3;
	u32 TxPktFifo285[3];	/* 0x000051d0 */
	u32 TxPktFifo285_rsv3;
	u32 TxPktFifo286[3];	/* 0x000051e0 */
	u32 TxPktFifo286_rsv3;
	u32 TxPktFifo287[3];	/* 0x000051f0 */
	u32 TxPktFifo287_rsv3;
	u32 TxPktFifo288[3];	/* 0x00005200 */
	u32 TxPktFifo288_rsv3;
	u32 TxPktFifo289[3];	/* 0x00005210 */
	u32 TxPktFifo289_rsv3;
	u32 TxPktFifo290[3];	/* 0x00005220 */
	u32 TxPktFifo290_rsv3;
	u32 TxPktFifo291[3];	/* 0x00005230 */
	u32 TxPktFifo291_rsv3;
	u32 TxPktFifo292[3];	/* 0x00005240 */
	u32 TxPktFifo292_rsv3;
	u32 TxPktFifo293[3];	/* 0x00005250 */
	u32 TxPktFifo293_rsv3;
	u32 TxPktFifo294[3];	/* 0x00005260 */
	u32 TxPktFifo294_rsv3;
	u32 TxPktFifo295[3];	/* 0x00005270 */
	u32 TxPktFifo295_rsv3;
	u32 TxPktFifo296[3];	/* 0x00005280 */
	u32 TxPktFifo296_rsv3;
	u32 TxPktFifo297[3];	/* 0x00005290 */
	u32 TxPktFifo297_rsv3;
	u32 TxPktFifo298[3];	/* 0x000052a0 */
	u32 TxPktFifo298_rsv3;
	u32 TxPktFifo299[3];	/* 0x000052b0 */
	u32 TxPktFifo299_rsv3;
	u32 TxPktFifo300[3];	/* 0x000052c0 */
	u32 TxPktFifo300_rsv3;
	u32 TxPktFifo301[3];	/* 0x000052d0 */
	u32 TxPktFifo301_rsv3;
	u32 TxPktFifo302[3];	/* 0x000052e0 */
	u32 TxPktFifo302_rsv3;
	u32 TxPktFifo303[3];	/* 0x000052f0 */
	u32 TxPktFifo303_rsv3;
	u32 TxPktFifo304[3];	/* 0x00005300 */
	u32 TxPktFifo304_rsv3;
	u32 TxPktFifo305[3];	/* 0x00005310 */
	u32 TxPktFifo305_rsv3;
	u32 TxPktFifo306[3];	/* 0x00005320 */
	u32 TxPktFifo306_rsv3;
	u32 TxPktFifo307[3];	/* 0x00005330 */
	u32 TxPktFifo307_rsv3;
	u32 TxPktFifo308[3];	/* 0x00005340 */
	u32 TxPktFifo308_rsv3;
	u32 TxPktFifo309[3];	/* 0x00005350 */
	u32 TxPktFifo309_rsv3;
	u32 TxPktFifo310[3];	/* 0x00005360 */
	u32 TxPktFifo310_rsv3;
	u32 TxPktFifo311[3];	/* 0x00005370 */
	u32 TxPktFifo311_rsv3;
	u32 TxPktFifo312[3];	/* 0x00005380 */
	u32 TxPktFifo312_rsv3;
	u32 TxPktFifo313[3];	/* 0x00005390 */
	u32 TxPktFifo313_rsv3;
	u32 TxPktFifo314[3];	/* 0x000053a0 */
	u32 TxPktFifo314_rsv3;
	u32 TxPktFifo315[3];	/* 0x000053b0 */
	u32 TxPktFifo315_rsv3;
	u32 TxPktFifo316[3];	/* 0x000053c0 */
	u32 TxPktFifo316_rsv3;
	u32 TxPktFifo317[3];	/* 0x000053d0 */
	u32 TxPktFifo317_rsv3;
	u32 TxPktFifo318[3];	/* 0x000053e0 */
	u32 TxPktFifo318_rsv3;
	u32 TxPktFifo319[3];	/* 0x000053f0 */
	u32 TxPktFifo319_rsv3;
	u32 TxPktFifo320[3];	/* 0x00005400 */
	u32 TxPktFifo320_rsv3;
	u32 TxPktFifo321[3];	/* 0x00005410 */
	u32 TxPktFifo321_rsv3;
	u32 TxPktFifo322[3];	/* 0x00005420 */
	u32 TxPktFifo322_rsv3;
	u32 TxPktFifo323[3];	/* 0x00005430 */
	u32 TxPktFifo323_rsv3;
	u32 TxPktFifo324[3];	/* 0x00005440 */
	u32 TxPktFifo324_rsv3;
	u32 TxPktFifo325[3];	/* 0x00005450 */
	u32 TxPktFifo325_rsv3;
	u32 TxPktFifo326[3];	/* 0x00005460 */
	u32 TxPktFifo326_rsv3;
	u32 TxPktFifo327[3];	/* 0x00005470 */
	u32 TxPktFifo327_rsv3;
	u32 TxPktFifo328[3];	/* 0x00005480 */
	u32 TxPktFifo328_rsv3;
	u32 TxPktFifo329[3];	/* 0x00005490 */
	u32 TxPktFifo329_rsv3;
	u32 TxPktFifo330[3];	/* 0x000054a0 */
	u32 TxPktFifo330_rsv3;
	u32 TxPktFifo331[3];	/* 0x000054b0 */
	u32 TxPktFifo331_rsv3;
	u32 TxPktFifo332[3];	/* 0x000054c0 */
	u32 TxPktFifo332_rsv3;
	u32 TxPktFifo333[3];	/* 0x000054d0 */
	u32 TxPktFifo333_rsv3;
	u32 TxPktFifo334[3];	/* 0x000054e0 */
	u32 TxPktFifo334_rsv3;
	u32 TxPktFifo335[3];	/* 0x000054f0 */
	u32 TxPktFifo335_rsv3;
	u32 TxPktFifo336[3];	/* 0x00005500 */
	u32 TxPktFifo336_rsv3;
	u32 TxPktFifo337[3];	/* 0x00005510 */
	u32 TxPktFifo337_rsv3;
	u32 TxPktFifo338[3];	/* 0x00005520 */
	u32 TxPktFifo338_rsv3;
	u32 TxPktFifo339[3];	/* 0x00005530 */
	u32 TxPktFifo339_rsv3;
	u32 TxPktFifo340[3];	/* 0x00005540 */
	u32 TxPktFifo340_rsv3;
	u32 TxPktFifo341[3];	/* 0x00005550 */
	u32 TxPktFifo341_rsv3;
	u32 TxPktFifo342[3];	/* 0x00005560 */
	u32 TxPktFifo342_rsv3;
	u32 TxPktFifo343[3];	/* 0x00005570 */
	u32 TxPktFifo343_rsv3;
	u32 TxPktFifo344[3];	/* 0x00005580 */
	u32 TxPktFifo344_rsv3;
	u32 TxPktFifo345[3];	/* 0x00005590 */
	u32 TxPktFifo345_rsv3;
	u32 TxPktFifo346[3];	/* 0x000055a0 */
	u32 TxPktFifo346_rsv3;
	u32 TxPktFifo347[3];	/* 0x000055b0 */
	u32 TxPktFifo347_rsv3;
	u32 TxPktFifo348[3];	/* 0x000055c0 */
	u32 TxPktFifo348_rsv3;
	u32 TxPktFifo349[3];	/* 0x000055d0 */
	u32 TxPktFifo349_rsv3;
	u32 TxPktFifo350[3];	/* 0x000055e0 */
	u32 TxPktFifo350_rsv3;
	u32 TxPktFifo351[3];	/* 0x000055f0 */
	u32 TxPktFifo351_rsv3;
	u32 TxPktFifo352[3];	/* 0x00005600 */
	u32 TxPktFifo352_rsv3;
	u32 TxPktFifo353[3];	/* 0x00005610 */
	u32 TxPktFifo353_rsv3;
	u32 TxPktFifo354[3];	/* 0x00005620 */
	u32 TxPktFifo354_rsv3;
	u32 TxPktFifo355[3];	/* 0x00005630 */
	u32 TxPktFifo355_rsv3;
	u32 TxPktFifo356[3];	/* 0x00005640 */
	u32 TxPktFifo356_rsv3;
	u32 TxPktFifo357[3];	/* 0x00005650 */
	u32 TxPktFifo357_rsv3;
	u32 TxPktFifo358[3];	/* 0x00005660 */
	u32 TxPktFifo358_rsv3;
	u32 TxPktFifo359[3];	/* 0x00005670 */
	u32 TxPktFifo359_rsv3;
	u32 TxPktFifo360[3];	/* 0x00005680 */
	u32 TxPktFifo360_rsv3;
	u32 TxPktFifo361[3];	/* 0x00005690 */
	u32 TxPktFifo361_rsv3;
	u32 TxPktFifo362[3];	/* 0x000056a0 */
	u32 TxPktFifo362_rsv3;
	u32 TxPktFifo363[3];	/* 0x000056b0 */
	u32 TxPktFifo363_rsv3;
	u32 TxPktFifo364[3];	/* 0x000056c0 */
	u32 TxPktFifo364_rsv3;
	u32 TxPktFifo365[3];	/* 0x000056d0 */
	u32 TxPktFifo365_rsv3;
	u32 TxPktFifo366[3];	/* 0x000056e0 */
	u32 TxPktFifo366_rsv3;
	u32 TxPktFifo367[3];	/* 0x000056f0 */
	u32 TxPktFifo367_rsv3;
	u32 TxPktFifo368[3];	/* 0x00005700 */
	u32 TxPktFifo368_rsv3;
	u32 TxPktFifo369[3];	/* 0x00005710 */
	u32 TxPktFifo369_rsv3;
	u32 TxPktFifo370[3];	/* 0x00005720 */
	u32 TxPktFifo370_rsv3;
	u32 TxPktFifo371[3];	/* 0x00005730 */
	u32 TxPktFifo371_rsv3;
	u32 TxPktFifo372[3];	/* 0x00005740 */
	u32 TxPktFifo372_rsv3;
	u32 TxPktFifo373[3];	/* 0x00005750 */
	u32 TxPktFifo373_rsv3;
	u32 TxPktFifo374[3];	/* 0x00005760 */
	u32 TxPktFifo374_rsv3;
	u32 TxPktFifo375[3];	/* 0x00005770 */
	u32 TxPktFifo375_rsv3;
	u32 TxPktFifo376[3];	/* 0x00005780 */
	u32 TxPktFifo376_rsv3;
	u32 TxPktFifo377[3];	/* 0x00005790 */
	u32 TxPktFifo377_rsv3;
	u32 TxPktFifo378[3];	/* 0x000057a0 */
	u32 TxPktFifo378_rsv3;
	u32 TxPktFifo379[3];	/* 0x000057b0 */
	u32 TxPktFifo379_rsv3;
	u32 TxPktFifo380[3];	/* 0x000057c0 */
	u32 TxPktFifo380_rsv3;
	u32 TxPktFifo381[3];	/* 0x000057d0 */
	u32 TxPktFifo381_rsv3;
	u32 TxPktFifo382[3];	/* 0x000057e0 */
	u32 TxPktFifo382_rsv3;
	u32 TxPktFifo383[3];	/* 0x000057f0 */
	u32 TxPktFifo383_rsv3;
	u32 TxPktFifo384[3];	/* 0x00005800 */
	u32 TxPktFifo384_rsv3;
	u32 TxPktFifo385[3];	/* 0x00005810 */
	u32 TxPktFifo385_rsv3;
	u32 TxPktFifo386[3];	/* 0x00005820 */
	u32 TxPktFifo386_rsv3;
	u32 TxPktFifo387[3];	/* 0x00005830 */
	u32 TxPktFifo387_rsv3;
	u32 TxPktFifo388[3];	/* 0x00005840 */
	u32 TxPktFifo388_rsv3;
	u32 TxPktFifo389[3];	/* 0x00005850 */
	u32 TxPktFifo389_rsv3;
	u32 TxPktFifo390[3];	/* 0x00005860 */
	u32 TxPktFifo390_rsv3;
	u32 TxPktFifo391[3];	/* 0x00005870 */
	u32 TxPktFifo391_rsv3;
	u32 TxPktFifo392[3];	/* 0x00005880 */
	u32 TxPktFifo392_rsv3;
	u32 TxPktFifo393[3];	/* 0x00005890 */
	u32 TxPktFifo393_rsv3;
	u32 TxPktFifo394[3];	/* 0x000058a0 */
	u32 TxPktFifo394_rsv3;
	u32 TxPktFifo395[3];	/* 0x000058b0 */
	u32 TxPktFifo395_rsv3;
	u32 TxPktFifo396[3];	/* 0x000058c0 */
	u32 TxPktFifo396_rsv3;
	u32 TxPktFifo397[3];	/* 0x000058d0 */
	u32 TxPktFifo397_rsv3;
	u32 TxPktFifo398[3];	/* 0x000058e0 */
	u32 TxPktFifo398_rsv3;
	u32 TxPktFifo399[3];	/* 0x000058f0 */
	u32 TxPktFifo399_rsv3;
	u32 TxPktFifo400[3];	/* 0x00005900 */
	u32 TxPktFifo400_rsv3;
	u32 TxPktFifo401[3];	/* 0x00005910 */
	u32 TxPktFifo401_rsv3;
	u32 TxPktFifo402[3];	/* 0x00005920 */
	u32 TxPktFifo402_rsv3;
	u32 TxPktFifo403[3];	/* 0x00005930 */
	u32 TxPktFifo403_rsv3;
	u32 TxPktFifo404[3];	/* 0x00005940 */
	u32 TxPktFifo404_rsv3;
	u32 TxPktFifo405[3];	/* 0x00005950 */
	u32 TxPktFifo405_rsv3;
	u32 TxPktFifo406[3];	/* 0x00005960 */
	u32 TxPktFifo406_rsv3;
	u32 TxPktFifo407[3];	/* 0x00005970 */
	u32 TxPktFifo407_rsv3;
	u32 TxPktFifo408[3];	/* 0x00005980 */
	u32 TxPktFifo408_rsv3;
	u32 TxPktFifo409[3];	/* 0x00005990 */
	u32 TxPktFifo409_rsv3;
	u32 TxPktFifo410[3];	/* 0x000059a0 */
	u32 TxPktFifo410_rsv3;
	u32 TxPktFifo411[3];	/* 0x000059b0 */
	u32 TxPktFifo411_rsv3;
	u32 TxPktFifo412[3];	/* 0x000059c0 */
	u32 TxPktFifo412_rsv3;
	u32 TxPktFifo413[3];	/* 0x000059d0 */
	u32 TxPktFifo413_rsv3;
	u32 TxPktFifo414[3];	/* 0x000059e0 */
	u32 TxPktFifo414_rsv3;
	u32 TxPktFifo415[3];	/* 0x000059f0 */
	u32 TxPktFifo415_rsv3;
	u32 TxPktFifo416[3];	/* 0x00005a00 */
	u32 TxPktFifo416_rsv3;
	u32 TxPktFifo417[3];	/* 0x00005a10 */
	u32 TxPktFifo417_rsv3;
	u32 TxPktFifo418[3];	/* 0x00005a20 */
	u32 TxPktFifo418_rsv3;
	u32 TxPktFifo419[3];	/* 0x00005a30 */
	u32 TxPktFifo419_rsv3;
	u32 TxPktFifo420[3];	/* 0x00005a40 */
	u32 TxPktFifo420_rsv3;
	u32 TxPktFifo421[3];	/* 0x00005a50 */
	u32 TxPktFifo421_rsv3;
	u32 TxPktFifo422[3];	/* 0x00005a60 */
	u32 TxPktFifo422_rsv3;
	u32 TxPktFifo423[3];	/* 0x00005a70 */
	u32 TxPktFifo423_rsv3;
	u32 TxPktFifo424[3];	/* 0x00005a80 */
	u32 TxPktFifo424_rsv3;
	u32 TxPktFifo425[3];	/* 0x00005a90 */
	u32 TxPktFifo425_rsv3;
	u32 TxPktFifo426[3];	/* 0x00005aa0 */
	u32 TxPktFifo426_rsv3;
	u32 TxPktFifo427[3];	/* 0x00005ab0 */
	u32 TxPktFifo427_rsv3;
	u32 TxPktFifo428[3];	/* 0x00005ac0 */
	u32 TxPktFifo428_rsv3;
	u32 TxPktFifo429[3];	/* 0x00005ad0 */
	u32 TxPktFifo429_rsv3;
	u32 TxPktFifo430[3];	/* 0x00005ae0 */
	u32 TxPktFifo430_rsv3;
	u32 TxPktFifo431[3];	/* 0x00005af0 */
	u32 TxPktFifo431_rsv3;
	u32 TxPktFifo432[3];	/* 0x00005b00 */
	u32 TxPktFifo432_rsv3;
	u32 TxPktFifo433[3];	/* 0x00005b10 */
	u32 TxPktFifo433_rsv3;
	u32 TxPktFifo434[3];	/* 0x00005b20 */
	u32 TxPktFifo434_rsv3;
	u32 TxPktFifo435[3];	/* 0x00005b30 */
	u32 TxPktFifo435_rsv3;
	u32 TxPktFifo436[3];	/* 0x00005b40 */
	u32 TxPktFifo436_rsv3;
	u32 TxPktFifo437[3];	/* 0x00005b50 */
	u32 TxPktFifo437_rsv3;
	u32 TxPktFifo438[3];	/* 0x00005b60 */
	u32 TxPktFifo438_rsv3;
	u32 TxPktFifo439[3];	/* 0x00005b70 */
	u32 TxPktFifo439_rsv3;
	u32 TxPktFifo440[3];	/* 0x00005b80 */
	u32 TxPktFifo440_rsv3;
	u32 TxPktFifo441[3];	/* 0x00005b90 */
	u32 TxPktFifo441_rsv3;
	u32 TxPktFifo442[3];	/* 0x00005ba0 */
	u32 TxPktFifo442_rsv3;
	u32 TxPktFifo443[3];	/* 0x00005bb0 */
	u32 TxPktFifo443_rsv3;
	u32 TxPktFifo444[3];	/* 0x00005bc0 */
	u32 TxPktFifo444_rsv3;
	u32 TxPktFifo445[3];	/* 0x00005bd0 */
	u32 TxPktFifo445_rsv3;
	u32 TxPktFifo446[3];	/* 0x00005be0 */
	u32 TxPktFifo446_rsv3;
	u32 TxPktFifo447[3];	/* 0x00005bf0 */
	u32 TxPktFifo447_rsv3;
	u32 TxPktFifo448[3];	/* 0x00005c00 */
	u32 TxPktFifo448_rsv3;
	u32 TxPktFifo449[3];	/* 0x00005c10 */
	u32 TxPktFifo449_rsv3;
	u32 TxPktFifo450[3];	/* 0x00005c20 */
	u32 TxPktFifo450_rsv3;
	u32 TxPktFifo451[3];	/* 0x00005c30 */
	u32 TxPktFifo451_rsv3;
	u32 TxPktFifo452[3];	/* 0x00005c40 */
	u32 TxPktFifo452_rsv3;
	u32 TxPktFifo453[3];	/* 0x00005c50 */
	u32 TxPktFifo453_rsv3;
	u32 TxPktFifo454[3];	/* 0x00005c60 */
	u32 TxPktFifo454_rsv3;
	u32 TxPktFifo455[3];	/* 0x00005c70 */
	u32 TxPktFifo455_rsv3;
	u32 TxPktFifo456[3];	/* 0x00005c80 */
	u32 TxPktFifo456_rsv3;
	u32 TxPktFifo457[3];	/* 0x00005c90 */
	u32 TxPktFifo457_rsv3;
	u32 TxPktFifo458[3];	/* 0x00005ca0 */
	u32 TxPktFifo458_rsv3;
	u32 TxPktFifo459[3];	/* 0x00005cb0 */
	u32 TxPktFifo459_rsv3;
	u32 TxPktFifo460[3];	/* 0x00005cc0 */
	u32 TxPktFifo460_rsv3;
	u32 TxPktFifo461[3];	/* 0x00005cd0 */
	u32 TxPktFifo461_rsv3;
	u32 TxPktFifo462[3];	/* 0x00005ce0 */
	u32 TxPktFifo462_rsv3;
	u32 TxPktFifo463[3];	/* 0x00005cf0 */
	u32 TxPktFifo463_rsv3;
	u32 TxPktFifo464[3];	/* 0x00005d00 */
	u32 TxPktFifo464_rsv3;
	u32 TxPktFifo465[3];	/* 0x00005d10 */
	u32 TxPktFifo465_rsv3;
	u32 TxPktFifo466[3];	/* 0x00005d20 */
	u32 TxPktFifo466_rsv3;
	u32 TxPktFifo467[3];	/* 0x00005d30 */
	u32 TxPktFifo467_rsv3;
	u32 TxPktFifo468[3];	/* 0x00005d40 */
	u32 TxPktFifo468_rsv3;
	u32 TxPktFifo469[3];	/* 0x00005d50 */
	u32 TxPktFifo469_rsv3;
	u32 TxPktFifo470[3];	/* 0x00005d60 */
	u32 TxPktFifo470_rsv3;
	u32 TxPktFifo471[3];	/* 0x00005d70 */
	u32 TxPktFifo471_rsv3;
	u32 TxPktFifo472[3];	/* 0x00005d80 */
	u32 TxPktFifo472_rsv3;
	u32 TxPktFifo473[3];	/* 0x00005d90 */
	u32 TxPktFifo473_rsv3;
	u32 TxPktFifo474[3];	/* 0x00005da0 */
	u32 TxPktFifo474_rsv3;
	u32 TxPktFifo475[3];	/* 0x00005db0 */
	u32 TxPktFifo475_rsv3;
	u32 TxPktFifo476[3];	/* 0x00005dc0 */
	u32 TxPktFifo476_rsv3;
	u32 TxPktFifo477[3];	/* 0x00005dd0 */
	u32 TxPktFifo477_rsv3;
	u32 TxPktFifo478[3];	/* 0x00005de0 */
	u32 TxPktFifo478_rsv3;
	u32 TxPktFifo479[3];	/* 0x00005df0 */
	u32 TxPktFifo479_rsv3;
	u32 TxPktFifo480[3];	/* 0x00005e00 */
	u32 TxPktFifo480_rsv3;
	u32 TxPktFifo481[3];	/* 0x00005e10 */
	u32 TxPktFifo481_rsv3;
	u32 TxPktFifo482[3];	/* 0x00005e20 */
	u32 TxPktFifo482_rsv3;
	u32 TxPktFifo483[3];	/* 0x00005e30 */
	u32 TxPktFifo483_rsv3;
	u32 TxPktFifo484[3];	/* 0x00005e40 */
	u32 TxPktFifo484_rsv3;
	u32 TxPktFifo485[3];	/* 0x00005e50 */
	u32 TxPktFifo485_rsv3;
	u32 TxPktFifo486[3];	/* 0x00005e60 */
	u32 TxPktFifo486_rsv3;
	u32 TxPktFifo487[3];	/* 0x00005e70 */
	u32 TxPktFifo487_rsv3;
	u32 TxPktFifo488[3];	/* 0x00005e80 */
	u32 TxPktFifo488_rsv3;
	u32 TxPktFifo489[3];	/* 0x00005e90 */
	u32 TxPktFifo489_rsv3;
	u32 TxPktFifo490[3];	/* 0x00005ea0 */
	u32 TxPktFifo490_rsv3;
	u32 TxPktFifo491[3];	/* 0x00005eb0 */
	u32 TxPktFifo491_rsv3;
	u32 TxPktFifo492[3];	/* 0x00005ec0 */
	u32 TxPktFifo492_rsv3;
	u32 TxPktFifo493[3];	/* 0x00005ed0 */
	u32 TxPktFifo493_rsv3;
	u32 TxPktFifo494[3];	/* 0x00005ee0 */
	u32 TxPktFifo494_rsv3;
	u32 TxPktFifo495[3];	/* 0x00005ef0 */
	u32 TxPktFifo495_rsv3;
	u32 TxPktFifo496[3];	/* 0x00005f00 */
	u32 TxPktFifo496_rsv3;
	u32 TxPktFifo497[3];	/* 0x00005f10 */
	u32 TxPktFifo497_rsv3;
	u32 TxPktFifo498[3];	/* 0x00005f20 */
	u32 TxPktFifo498_rsv3;
	u32 TxPktFifo499[3];	/* 0x00005f30 */
	u32 TxPktFifo499_rsv3;
	u32 TxPktFifo500[3];	/* 0x00005f40 */
	u32 TxPktFifo500_rsv3;
	u32 TxPktFifo501[3];	/* 0x00005f50 */
	u32 TxPktFifo501_rsv3;
	u32 TxPktFifo502[3];	/* 0x00005f60 */
	u32 TxPktFifo502_rsv3;
	u32 TxPktFifo503[3];	/* 0x00005f70 */
	u32 TxPktFifo503_rsv3;
	u32 TxPktFifo504[3];	/* 0x00005f80 */
	u32 TxPktFifo504_rsv3;
	u32 TxPktFifo505[3];	/* 0x00005f90 */
	u32 TxPktFifo505_rsv3;
	u32 TxPktFifo506[3];	/* 0x00005fa0 */
	u32 TxPktFifo506_rsv3;
	u32 TxPktFifo507[3];	/* 0x00005fb0 */
	u32 TxPktFifo507_rsv3;
	u32 TxPktFifo508[3];	/* 0x00005fc0 */
	u32 TxPktFifo508_rsv3;
	u32 TxPktFifo509[3];	/* 0x00005fd0 */
	u32 TxPktFifo509_rsv3;
	u32 TxPktFifo510[3];	/* 0x00005fe0 */
	u32 TxPktFifo510_rsv3;
	u32 TxPktFifo511[3];	/* 0x00005ff0 */
	u32 TxPktFifo511_rsv3;
	u32 TxPktFifo512[3];	/* 0x00006000 */
	u32 TxPktFifo512_rsv3;
	u32 TxPktFifo513[3];	/* 0x00006010 */
	u32 TxPktFifo513_rsv3;
	u32 TxPktFifo514[3];	/* 0x00006020 */
	u32 TxPktFifo514_rsv3;
	u32 TxPktFifo515[3];	/* 0x00006030 */
	u32 TxPktFifo515_rsv3;
	u32 TxPktFifo516[3];	/* 0x00006040 */
	u32 TxPktFifo516_rsv3;
	u32 TxPktFifo517[3];	/* 0x00006050 */
	u32 TxPktFifo517_rsv3;
	u32 TxPktFifo518[3];	/* 0x00006060 */
	u32 TxPktFifo518_rsv3;
	u32 TxPktFifo519[3];	/* 0x00006070 */
	u32 TxPktFifo519_rsv3;
	u32 TxPktFifo520[3];	/* 0x00006080 */
	u32 TxPktFifo520_rsv3;
	u32 TxPktFifo521[3];	/* 0x00006090 */
	u32 TxPktFifo521_rsv3;
	u32 TxPktFifo522[3];	/* 0x000060a0 */
	u32 TxPktFifo522_rsv3;
	u32 TxPktFifo523[3];	/* 0x000060b0 */
	u32 TxPktFifo523_rsv3;
	u32 TxPktFifo524[3];	/* 0x000060c0 */
	u32 TxPktFifo524_rsv3;
	u32 TxPktFifo525[3];	/* 0x000060d0 */
	u32 TxPktFifo525_rsv3;
	u32 TxPktFifo526[3];	/* 0x000060e0 */
	u32 TxPktFifo526_rsv3;
	u32 TxPktFifo527[3];	/* 0x000060f0 */
	u32 TxPktFifo527_rsv3;
	u32 TxPktFifo528[3];	/* 0x00006100 */
	u32 TxPktFifo528_rsv3;
	u32 TxPktFifo529[3];	/* 0x00006110 */
	u32 TxPktFifo529_rsv3;
	u32 TxPktFifo530[3];	/* 0x00006120 */
	u32 TxPktFifo530_rsv3;
	u32 TxPktFifo531[3];	/* 0x00006130 */
	u32 TxPktFifo531_rsv3;
	u32 TxPktFifo532[3];	/* 0x00006140 */
	u32 TxPktFifo532_rsv3;
	u32 TxPktFifo533[3];	/* 0x00006150 */
	u32 TxPktFifo533_rsv3;
	u32 TxPktFifo534[3];	/* 0x00006160 */
	u32 TxPktFifo534_rsv3;
	u32 TxPktFifo535[3];	/* 0x00006170 */
	u32 TxPktFifo535_rsv3;
	u32 TxPktFifo536[3];	/* 0x00006180 */
	u32 TxPktFifo536_rsv3;
	u32 TxPktFifo537[3];	/* 0x00006190 */
	u32 TxPktFifo537_rsv3;
	u32 TxPktFifo538[3];	/* 0x000061a0 */
	u32 TxPktFifo538_rsv3;
	u32 TxPktFifo539[3];	/* 0x000061b0 */
	u32 TxPktFifo539_rsv3;
	u32 TxPktFifo540[3];	/* 0x000061c0 */
	u32 TxPktFifo540_rsv3;
	u32 TxPktFifo541[3];	/* 0x000061d0 */
	u32 TxPktFifo541_rsv3;
	u32 TxPktFifo542[3];	/* 0x000061e0 */
	u32 TxPktFifo542_rsv3;
	u32 TxPktFifo543[3];	/* 0x000061f0 */
	u32 TxPktFifo543_rsv3;
	u32 TxPktFifo544[3];	/* 0x00006200 */
	u32 TxPktFifo544_rsv3;
	u32 TxPktFifo545[3];	/* 0x00006210 */
	u32 TxPktFifo545_rsv3;
	u32 TxPktFifo546[3];	/* 0x00006220 */
	u32 TxPktFifo546_rsv3;
	u32 TxPktFifo547[3];	/* 0x00006230 */
	u32 TxPktFifo547_rsv3;
	u32 TxPktFifo548[3];	/* 0x00006240 */
	u32 TxPktFifo548_rsv3;
	u32 TxPktFifo549[3];	/* 0x00006250 */
	u32 TxPktFifo549_rsv3;
	u32 TxPktFifo550[3];	/* 0x00006260 */
	u32 TxPktFifo550_rsv3;
	u32 TxPktFifo551[3];	/* 0x00006270 */
	u32 TxPktFifo551_rsv3;
	u32 TxPktFifo552[3];	/* 0x00006280 */
	u32 TxPktFifo552_rsv3;
	u32 TxPktFifo553[3];	/* 0x00006290 */
	u32 TxPktFifo553_rsv3;
	u32 TxPktFifo554[3];	/* 0x000062a0 */
	u32 TxPktFifo554_rsv3;
	u32 TxPktFifo555[3];	/* 0x000062b0 */
	u32 TxPktFifo555_rsv3;
	u32 TxPktFifo556[3];	/* 0x000062c0 */
	u32 TxPktFifo556_rsv3;
	u32 TxPktFifo557[3];	/* 0x000062d0 */
	u32 TxPktFifo557_rsv3;
	u32 TxPktFifo558[3];	/* 0x000062e0 */
	u32 TxPktFifo558_rsv3;
	u32 TxPktFifo559[3];	/* 0x000062f0 */
	u32 TxPktFifo559_rsv3;
	u32 TxPktFifo560[3];	/* 0x00006300 */
	u32 TxPktFifo560_rsv3;
	u32 TxPktFifo561[3];	/* 0x00006310 */
	u32 TxPktFifo561_rsv3;
	u32 TxPktFifo562[3];	/* 0x00006320 */
	u32 TxPktFifo562_rsv3;
	u32 TxPktFifo563[3];	/* 0x00006330 */
	u32 TxPktFifo563_rsv3;
	u32 TxPktFifo564[3];	/* 0x00006340 */
	u32 TxPktFifo564_rsv3;
	u32 TxPktFifo565[3];	/* 0x00006350 */
	u32 TxPktFifo565_rsv3;
	u32 TxPktFifo566[3];	/* 0x00006360 */
	u32 TxPktFifo566_rsv3;
	u32 TxPktFifo567[3];	/* 0x00006370 */
	u32 TxPktFifo567_rsv3;
	u32 TxPktFifo568[3];	/* 0x00006380 */
	u32 TxPktFifo568_rsv3;
	u32 TxPktFifo569[3];	/* 0x00006390 */
	u32 TxPktFifo569_rsv3;
	u32 TxPktFifo570[3];	/* 0x000063a0 */
	u32 TxPktFifo570_rsv3;
	u32 TxPktFifo571[3];	/* 0x000063b0 */
	u32 TxPktFifo571_rsv3;
	u32 TxPktFifo572[3];	/* 0x000063c0 */
	u32 TxPktFifo572_rsv3;
	u32 TxPktFifo573[3];	/* 0x000063d0 */
	u32 TxPktFifo573_rsv3;
	u32 TxPktFifo574[3];	/* 0x000063e0 */
	u32 TxPktFifo574_rsv3;
	u32 TxPktFifo575[3];	/* 0x000063f0 */
	u32 TxPktFifo575_rsv3;
	u32 TxPktFifo576[3];	/* 0x00006400 */
	u32 TxPktFifo576_rsv3;
	u32 TxPktFifo577[3];	/* 0x00006410 */
	u32 TxPktFifo577_rsv3;
	u32 TxPktFifo578[3];	/* 0x00006420 */
	u32 TxPktFifo578_rsv3;
	u32 TxPktFifo579[3];	/* 0x00006430 */
	u32 TxPktFifo579_rsv3;
	u32 TxPktFifo580[3];	/* 0x00006440 */
	u32 TxPktFifo580_rsv3;
	u32 TxPktFifo581[3];	/* 0x00006450 */
	u32 TxPktFifo581_rsv3;
	u32 TxPktFifo582[3];	/* 0x00006460 */
	u32 TxPktFifo582_rsv3;
	u32 TxPktFifo583[3];	/* 0x00006470 */
	u32 TxPktFifo583_rsv3;
	u32 TxPktFifo584[3];	/* 0x00006480 */
	u32 TxPktFifo584_rsv3;
	u32 TxPktFifo585[3];	/* 0x00006490 */
	u32 TxPktFifo585_rsv3;
	u32 TxPktFifo586[3];	/* 0x000064a0 */
	u32 TxPktFifo586_rsv3;
	u32 TxPktFifo587[3];	/* 0x000064b0 */
	u32 TxPktFifo587_rsv3;
	u32 TxPktFifo588[3];	/* 0x000064c0 */
	u32 TxPktFifo588_rsv3;
	u32 TxPktFifo589[3];	/* 0x000064d0 */
	u32 TxPktFifo589_rsv3;
	u32 TxPktFifo590[3];	/* 0x000064e0 */
	u32 TxPktFifo590_rsv3;
	u32 TxPktFifo591[3];	/* 0x000064f0 */
	u32 TxPktFifo591_rsv3;
	u32 TxPktFifo592[3];	/* 0x00006500 */
	u32 TxPktFifo592_rsv3;
	u32 TxPktFifo593[3];	/* 0x00006510 */
	u32 TxPktFifo593_rsv3;
	u32 TxPktFifo594[3];	/* 0x00006520 */
	u32 TxPktFifo594_rsv3;
	u32 TxPktFifo595[3];	/* 0x00006530 */
	u32 TxPktFifo595_rsv3;
	u32 TxPktFifo596[3];	/* 0x00006540 */
	u32 TxPktFifo596_rsv3;
	u32 TxPktFifo597[3];	/* 0x00006550 */
	u32 TxPktFifo597_rsv3;
	u32 TxPktFifo598[3];	/* 0x00006560 */
	u32 TxPktFifo598_rsv3;
	u32 TxPktFifo599[3];	/* 0x00006570 */
	u32 TxPktFifo599_rsv3;
	u32 TxPktFifo600[3];	/* 0x00006580 */
	u32 TxPktFifo600_rsv3;
	u32 TxPktFifo601[3];	/* 0x00006590 */
	u32 TxPktFifo601_rsv3;
	u32 TxPktFifo602[3];	/* 0x000065a0 */
	u32 TxPktFifo602_rsv3;
	u32 TxPktFifo603[3];	/* 0x000065b0 */
	u32 TxPktFifo603_rsv3;
	u32 TxPktFifo604[3];	/* 0x000065c0 */
	u32 TxPktFifo604_rsv3;
	u32 TxPktFifo605[3];	/* 0x000065d0 */
	u32 TxPktFifo605_rsv3;
	u32 TxPktFifo606[3];	/* 0x000065e0 */
	u32 TxPktFifo606_rsv3;
	u32 TxPktFifo607[3];	/* 0x000065f0 */
	u32 TxPktFifo607_rsv3;
	u32 TxPktFifo608[3];	/* 0x00006600 */
	u32 TxPktFifo608_rsv3;
	u32 TxPktFifo609[3];	/* 0x00006610 */
	u32 TxPktFifo609_rsv3;
	u32 TxPktFifo610[3];	/* 0x00006620 */
	u32 TxPktFifo610_rsv3;
	u32 TxPktFifo611[3];	/* 0x00006630 */
	u32 TxPktFifo611_rsv3;
	u32 TxPktFifo612[3];	/* 0x00006640 */
	u32 TxPktFifo612_rsv3;
	u32 TxPktFifo613[3];	/* 0x00006650 */
	u32 TxPktFifo613_rsv3;
	u32 TxPktFifo614[3];	/* 0x00006660 */
	u32 TxPktFifo614_rsv3;
	u32 TxPktFifo615[3];	/* 0x00006670 */
	u32 TxPktFifo615_rsv3;
	u32 TxPktFifo616[3];	/* 0x00006680 */
	u32 TxPktFifo616_rsv3;
	u32 TxPktFifo617[3];	/* 0x00006690 */
	u32 TxPktFifo617_rsv3;
	u32 TxPktFifo618[3];	/* 0x000066a0 */
	u32 TxPktFifo618_rsv3;
	u32 TxPktFifo619[3];	/* 0x000066b0 */
	u32 TxPktFifo619_rsv3;
	u32 TxPktFifo620[3];	/* 0x000066c0 */
	u32 TxPktFifo620_rsv3;
	u32 TxPktFifo621[3];	/* 0x000066d0 */
	u32 TxPktFifo621_rsv3;
	u32 TxPktFifo622[3];	/* 0x000066e0 */
	u32 TxPktFifo622_rsv3;
	u32 TxPktFifo623[3];	/* 0x000066f0 */
	u32 TxPktFifo623_rsv3;
	u32 TxPktFifo624[3];	/* 0x00006700 */
	u32 TxPktFifo624_rsv3;
	u32 TxPktFifo625[3];	/* 0x00006710 */
	u32 TxPktFifo625_rsv3;
	u32 TxPktFifo626[3];	/* 0x00006720 */
	u32 TxPktFifo626_rsv3;
	u32 TxPktFifo627[3];	/* 0x00006730 */
	u32 TxPktFifo627_rsv3;
	u32 TxPktFifo628[3];	/* 0x00006740 */
	u32 TxPktFifo628_rsv3;
	u32 TxPktFifo629[3];	/* 0x00006750 */
	u32 TxPktFifo629_rsv3;
	u32 TxPktFifo630[3];	/* 0x00006760 */
	u32 TxPktFifo630_rsv3;
	u32 TxPktFifo631[3];	/* 0x00006770 */
	u32 TxPktFifo631_rsv3;
	u32 TxPktFifo632[3];	/* 0x00006780 */
	u32 TxPktFifo632_rsv3;
	u32 TxPktFifo633[3];	/* 0x00006790 */
	u32 TxPktFifo633_rsv3;
	u32 TxPktFifo634[3];	/* 0x000067a0 */
	u32 TxPktFifo634_rsv3;
	u32 TxPktFifo635[3];	/* 0x000067b0 */
	u32 TxPktFifo635_rsv3;
	u32 TxPktFifo636[3];	/* 0x000067c0 */
	u32 TxPktFifo636_rsv3;
	u32 TxPktFifo637[3];	/* 0x000067d0 */
	u32 TxPktFifo637_rsv3;
	u32 TxPktFifo638[3];	/* 0x000067e0 */
	u32 TxPktFifo638_rsv3;
	u32 TxPktFifo639[3];	/* 0x000067f0 */
	u32 TxPktFifo639_rsv3;
	u32 rsv6656;
	u32 rsv6657;
	u32 rsv6658;
	u32 rsv6659;
	u32 rsv6660;
	u32 rsv6661;
	u32 rsv6662;
	u32 rsv6663;
	u32 rsv6664;
	u32 rsv6665;
	u32 rsv6666;
	u32 rsv6667;
	u32 rsv6668;
	u32 rsv6669;
	u32 rsv6670;
	u32 rsv6671;
	u32 rsv6672;
	u32 rsv6673;
	u32 rsv6674;
	u32 rsv6675;
	u32 rsv6676;
	u32 rsv6677;
	u32 rsv6678;
	u32 rsv6679;
	u32 rsv6680;
	u32 rsv6681;
	u32 rsv6682;
	u32 rsv6683;
	u32 rsv6684;
	u32 rsv6685;
	u32 rsv6686;
	u32 rsv6687;
	u32 rsv6688;
	u32 rsv6689;
	u32 rsv6690;
	u32 rsv6691;
	u32 rsv6692;
	u32 rsv6693;
	u32 rsv6694;
	u32 rsv6695;
	u32 rsv6696;
	u32 rsv6697;
	u32 rsv6698;
	u32 rsv6699;
	u32 rsv6700;
	u32 rsv6701;
	u32 rsv6702;
	u32 rsv6703;
	u32 rsv6704;
	u32 rsv6705;
	u32 rsv6706;
	u32 rsv6707;
	u32 rsv6708;
	u32 rsv6709;
	u32 rsv6710;
	u32 rsv6711;
	u32 rsv6712;
	u32 rsv6713;
	u32 rsv6714;
	u32 rsv6715;
	u32 rsv6716;
	u32 rsv6717;
	u32 rsv6718;
	u32 rsv6719;
	u32 rsv6720;
	u32 rsv6721;
	u32 rsv6722;
	u32 rsv6723;
	u32 rsv6724;
	u32 rsv6725;
	u32 rsv6726;
	u32 rsv6727;
	u32 rsv6728;
	u32 rsv6729;
	u32 rsv6730;
	u32 rsv6731;
	u32 rsv6732;
	u32 rsv6733;
	u32 rsv6734;
	u32 rsv6735;
	u32 rsv6736;
	u32 rsv6737;
	u32 rsv6738;
	u32 rsv6739;
	u32 rsv6740;
	u32 rsv6741;
	u32 rsv6742;
	u32 rsv6743;
	u32 rsv6744;
	u32 rsv6745;
	u32 rsv6746;
	u32 rsv6747;
	u32 rsv6748;
	u32 rsv6749;
	u32 rsv6750;
	u32 rsv6751;
	u32 rsv6752;
	u32 rsv6753;
	u32 rsv6754;
	u32 rsv6755;
	u32 rsv6756;
	u32 rsv6757;
	u32 rsv6758;
	u32 rsv6759;
	u32 rsv6760;
	u32 rsv6761;
	u32 rsv6762;
	u32 rsv6763;
	u32 rsv6764;
	u32 rsv6765;
	u32 rsv6766;
	u32 rsv6767;
	u32 rsv6768;
	u32 rsv6769;
	u32 rsv6770;
	u32 rsv6771;
	u32 rsv6772;
	u32 rsv6773;
	u32 rsv6774;
	u32 rsv6775;
	u32 rsv6776;
	u32 rsv6777;
	u32 rsv6778;
	u32 rsv6779;
	u32 rsv6780;
	u32 rsv6781;
	u32 rsv6782;
	u32 rsv6783;
	u32 rsv6784;
	u32 rsv6785;
	u32 rsv6786;
	u32 rsv6787;
	u32 rsv6788;
	u32 rsv6789;
	u32 rsv6790;
	u32 rsv6791;
	u32 rsv6792;
	u32 rsv6793;
	u32 rsv6794;
	u32 rsv6795;
	u32 rsv6796;
	u32 rsv6797;
	u32 rsv6798;
	u32 rsv6799;
	u32 rsv6800;
	u32 rsv6801;
	u32 rsv6802;
	u32 rsv6803;
	u32 rsv6804;
	u32 rsv6805;
	u32 rsv6806;
	u32 rsv6807;
	u32 rsv6808;
	u32 rsv6809;
	u32 rsv6810;
	u32 rsv6811;
	u32 rsv6812;
	u32 rsv6813;
	u32 rsv6814;
	u32 rsv6815;
	u32 rsv6816;
	u32 rsv6817;
	u32 rsv6818;
	u32 rsv6819;
	u32 rsv6820;
	u32 rsv6821;
	u32 rsv6822;
	u32 rsv6823;
	u32 rsv6824;
	u32 rsv6825;
	u32 rsv6826;
	u32 rsv6827;
	u32 rsv6828;
	u32 rsv6829;
	u32 rsv6830;
	u32 rsv6831;
	u32 rsv6832;
	u32 rsv6833;
	u32 rsv6834;
	u32 rsv6835;
	u32 rsv6836;
	u32 rsv6837;
	u32 rsv6838;
	u32 rsv6839;
	u32 rsv6840;
	u32 rsv6841;
	u32 rsv6842;
	u32 rsv6843;
	u32 rsv6844;
	u32 rsv6845;
	u32 rsv6846;
	u32 rsv6847;
	u32 rsv6848;
	u32 rsv6849;
	u32 rsv6850;
	u32 rsv6851;
	u32 rsv6852;
	u32 rsv6853;
	u32 rsv6854;
	u32 rsv6855;
	u32 rsv6856;
	u32 rsv6857;
	u32 rsv6858;
	u32 rsv6859;
	u32 rsv6860;
	u32 rsv6861;
	u32 rsv6862;
	u32 rsv6863;
	u32 rsv6864;
	u32 rsv6865;
	u32 rsv6866;
	u32 rsv6867;
	u32 rsv6868;
	u32 rsv6869;
	u32 rsv6870;
	u32 rsv6871;
	u32 rsv6872;
	u32 rsv6873;
	u32 rsv6874;
	u32 rsv6875;
	u32 rsv6876;
	u32 rsv6877;
	u32 rsv6878;
	u32 rsv6879;
	u32 rsv6880;
	u32 rsv6881;
	u32 rsv6882;
	u32 rsv6883;
	u32 rsv6884;
	u32 rsv6885;
	u32 rsv6886;
	u32 rsv6887;
	u32 rsv6888;
	u32 rsv6889;
	u32 rsv6890;
	u32 rsv6891;
	u32 rsv6892;
	u32 rsv6893;
	u32 rsv6894;
	u32 rsv6895;
	u32 rsv6896;
	u32 rsv6897;
	u32 rsv6898;
	u32 rsv6899;
	u32 rsv6900;
	u32 rsv6901;
	u32 rsv6902;
	u32 rsv6903;
	u32 rsv6904;
	u32 rsv6905;
	u32 rsv6906;
	u32 rsv6907;
	u32 rsv6908;
	u32 rsv6909;
	u32 rsv6910;
	u32 rsv6911;
	u32 rsv6912;
	u32 rsv6913;
	u32 rsv6914;
	u32 rsv6915;
	u32 rsv6916;
	u32 rsv6917;
	u32 rsv6918;
	u32 rsv6919;
	u32 rsv6920;
	u32 rsv6921;
	u32 rsv6922;
	u32 rsv6923;
	u32 rsv6924;
	u32 rsv6925;
	u32 rsv6926;
	u32 rsv6927;
	u32 rsv6928;
	u32 rsv6929;
	u32 rsv6930;
	u32 rsv6931;
	u32 rsv6932;
	u32 rsv6933;
	u32 rsv6934;
	u32 rsv6935;
	u32 rsv6936;
	u32 rsv6937;
	u32 rsv6938;
	u32 rsv6939;
	u32 rsv6940;
	u32 rsv6941;
	u32 rsv6942;
	u32 rsv6943;
	u32 rsv6944;
	u32 rsv6945;
	u32 rsv6946;
	u32 rsv6947;
	u32 rsv6948;
	u32 rsv6949;
	u32 rsv6950;
	u32 rsv6951;
	u32 rsv6952;
	u32 rsv6953;
	u32 rsv6954;
	u32 rsv6955;
	u32 rsv6956;
	u32 rsv6957;
	u32 rsv6958;
	u32 rsv6959;
	u32 rsv6960;
	u32 rsv6961;
	u32 rsv6962;
	u32 rsv6963;
	u32 rsv6964;
	u32 rsv6965;
	u32 rsv6966;
	u32 rsv6967;
	u32 rsv6968;
	u32 rsv6969;
	u32 rsv6970;
	u32 rsv6971;
	u32 rsv6972;
	u32 rsv6973;
	u32 rsv6974;
	u32 rsv6975;
	u32 rsv6976;
	u32 rsv6977;
	u32 rsv6978;
	u32 rsv6979;
	u32 rsv6980;
	u32 rsv6981;
	u32 rsv6982;
	u32 rsv6983;
	u32 rsv6984;
	u32 rsv6985;
	u32 rsv6986;
	u32 rsv6987;
	u32 rsv6988;
	u32 rsv6989;
	u32 rsv6990;
	u32 rsv6991;
	u32 rsv6992;
	u32 rsv6993;
	u32 rsv6994;
	u32 rsv6995;
	u32 rsv6996;
	u32 rsv6997;
	u32 rsv6998;
	u32 rsv6999;
	u32 rsv7000;
	u32 rsv7001;
	u32 rsv7002;
	u32 rsv7003;
	u32 rsv7004;
	u32 rsv7005;
	u32 rsv7006;
	u32 rsv7007;
	u32 rsv7008;
	u32 rsv7009;
	u32 rsv7010;
	u32 rsv7011;
	u32 rsv7012;
	u32 rsv7013;
	u32 rsv7014;
	u32 rsv7015;
	u32 rsv7016;
	u32 rsv7017;
	u32 rsv7018;
	u32 rsv7019;
	u32 rsv7020;
	u32 rsv7021;
	u32 rsv7022;
	u32 rsv7023;
	u32 rsv7024;
	u32 rsv7025;
	u32 rsv7026;
	u32 rsv7027;
	u32 rsv7028;
	u32 rsv7029;
	u32 rsv7030;
	u32 rsv7031;
	u32 rsv7032;
	u32 rsv7033;
	u32 rsv7034;
	u32 rsv7035;
	u32 rsv7036;
	u32 rsv7037;
	u32 rsv7038;
	u32 rsv7039;
	u32 rsv7040;
	u32 rsv7041;
	u32 rsv7042;
	u32 rsv7043;
	u32 rsv7044;
	u32 rsv7045;
	u32 rsv7046;
	u32 rsv7047;
	u32 rsv7048;
	u32 rsv7049;
	u32 rsv7050;
	u32 rsv7051;
	u32 rsv7052;
	u32 rsv7053;
	u32 rsv7054;
	u32 rsv7055;
	u32 rsv7056;
	u32 rsv7057;
	u32 rsv7058;
	u32 rsv7059;
	u32 rsv7060;
	u32 rsv7061;
	u32 rsv7062;
	u32 rsv7063;
	u32 rsv7064;
	u32 rsv7065;
	u32 rsv7066;
	u32 rsv7067;
	u32 rsv7068;
	u32 rsv7069;
	u32 rsv7070;
	u32 rsv7071;
	u32 rsv7072;
	u32 rsv7073;
	u32 rsv7074;
	u32 rsv7075;
	u32 rsv7076;
	u32 rsv7077;
	u32 rsv7078;
	u32 rsv7079;
	u32 rsv7080;
	u32 rsv7081;
	u32 rsv7082;
	u32 rsv7083;
	u32 rsv7084;
	u32 rsv7085;
	u32 rsv7086;
	u32 rsv7087;
	u32 rsv7088;
	u32 rsv7089;
	u32 rsv7090;
	u32 rsv7091;
	u32 rsv7092;
	u32 rsv7093;
	u32 rsv7094;
	u32 rsv7095;
	u32 rsv7096;
	u32 rsv7097;
	u32 rsv7098;
	u32 rsv7099;
	u32 rsv7100;
	u32 rsv7101;
	u32 rsv7102;
	u32 rsv7103;
	u32 rsv7104;
	u32 rsv7105;
	u32 rsv7106;
	u32 rsv7107;
	u32 rsv7108;
	u32 rsv7109;
	u32 rsv7110;
	u32 rsv7111;
	u32 rsv7112;
	u32 rsv7113;
	u32 rsv7114;
	u32 rsv7115;
	u32 rsv7116;
	u32 rsv7117;
	u32 rsv7118;
	u32 rsv7119;
	u32 rsv7120;
	u32 rsv7121;
	u32 rsv7122;
	u32 rsv7123;
	u32 rsv7124;
	u32 rsv7125;
	u32 rsv7126;
	u32 rsv7127;
	u32 rsv7128;
	u32 rsv7129;
	u32 rsv7130;
	u32 rsv7131;
	u32 rsv7132;
	u32 rsv7133;
	u32 rsv7134;
	u32 rsv7135;
	u32 rsv7136;
	u32 rsv7137;
	u32 rsv7138;
	u32 rsv7139;
	u32 rsv7140;
	u32 rsv7141;
	u32 rsv7142;
	u32 rsv7143;
	u32 rsv7144;
	u32 rsv7145;
	u32 rsv7146;
	u32 rsv7147;
	u32 rsv7148;
	u32 rsv7149;
	u32 rsv7150;
	u32 rsv7151;
	u32 rsv7152;
	u32 rsv7153;
	u32 rsv7154;
	u32 rsv7155;
	u32 rsv7156;
	u32 rsv7157;
	u32 rsv7158;
	u32 rsv7159;
	u32 rsv7160;
	u32 rsv7161;
	u32 rsv7162;
	u32 rsv7163;
	u32 rsv7164;
	u32 rsv7165;
	u32 rsv7166;
	u32 rsv7167;
	u32 rsv7168;
	u32 rsv7169;
	u32 rsv7170;
	u32 rsv7171;
	u32 rsv7172;
	u32 rsv7173;
	u32 rsv7174;
	u32 rsv7175;
	u32 rsv7176;
	u32 rsv7177;
	u32 rsv7178;
	u32 rsv7179;
	u32 rsv7180;
	u32 rsv7181;
	u32 rsv7182;
	u32 rsv7183;
	u32 rsv7184;
	u32 rsv7185;
	u32 rsv7186;
	u32 rsv7187;
	u32 rsv7188;
	u32 rsv7189;
	u32 rsv7190;
	u32 rsv7191;
	u32 rsv7192;
	u32 rsv7193;
	u32 rsv7194;
	u32 rsv7195;
	u32 rsv7196;
	u32 rsv7197;
	u32 rsv7198;
	u32 rsv7199;
	u32 rsv7200;
	u32 rsv7201;
	u32 rsv7202;
	u32 rsv7203;
	u32 rsv7204;
	u32 rsv7205;
	u32 rsv7206;
	u32 rsv7207;
	u32 rsv7208;
	u32 rsv7209;
	u32 rsv7210;
	u32 rsv7211;
	u32 rsv7212;
	u32 rsv7213;
	u32 rsv7214;
	u32 rsv7215;
	u32 rsv7216;
	u32 rsv7217;
	u32 rsv7218;
	u32 rsv7219;
	u32 rsv7220;
	u32 rsv7221;
	u32 rsv7222;
	u32 rsv7223;
	u32 rsv7224;
	u32 rsv7225;
	u32 rsv7226;
	u32 rsv7227;
	u32 rsv7228;
	u32 rsv7229;
	u32 rsv7230;
	u32 rsv7231;
	u32 rsv7232;
	u32 rsv7233;
	u32 rsv7234;
	u32 rsv7235;
	u32 rsv7236;
	u32 rsv7237;
	u32 rsv7238;
	u32 rsv7239;
	u32 rsv7240;
	u32 rsv7241;
	u32 rsv7242;
	u32 rsv7243;
	u32 rsv7244;
	u32 rsv7245;
	u32 rsv7246;
	u32 rsv7247;
	u32 rsv7248;
	u32 rsv7249;
	u32 rsv7250;
	u32 rsv7251;
	u32 rsv7252;
	u32 rsv7253;
	u32 rsv7254;
	u32 rsv7255;
	u32 rsv7256;
	u32 rsv7257;
	u32 rsv7258;
	u32 rsv7259;
	u32 rsv7260;
	u32 rsv7261;
	u32 rsv7262;
	u32 rsv7263;
	u32 rsv7264;
	u32 rsv7265;
	u32 rsv7266;
	u32 rsv7267;
	u32 rsv7268;
	u32 rsv7269;
	u32 rsv7270;
	u32 rsv7271;
	u32 rsv7272;
	u32 rsv7273;
	u32 rsv7274;
	u32 rsv7275;
	u32 rsv7276;
	u32 rsv7277;
	u32 rsv7278;
	u32 rsv7279;
	u32 rsv7280;
	u32 rsv7281;
	u32 rsv7282;
	u32 rsv7283;
	u32 rsv7284;
	u32 rsv7285;
	u32 rsv7286;
	u32 rsv7287;
	u32 rsv7288;
	u32 rsv7289;
	u32 rsv7290;
	u32 rsv7291;
	u32 rsv7292;
	u32 rsv7293;
	u32 rsv7294;
	u32 rsv7295;
	u32 rsv7296;
	u32 rsv7297;
	u32 rsv7298;
	u32 rsv7299;
	u32 rsv7300;
	u32 rsv7301;
	u32 rsv7302;
	u32 rsv7303;
	u32 rsv7304;
	u32 rsv7305;
	u32 rsv7306;
	u32 rsv7307;
	u32 rsv7308;
	u32 rsv7309;
	u32 rsv7310;
	u32 rsv7311;
	u32 rsv7312;
	u32 rsv7313;
	u32 rsv7314;
	u32 rsv7315;
	u32 rsv7316;
	u32 rsv7317;
	u32 rsv7318;
	u32 rsv7319;
	u32 rsv7320;
	u32 rsv7321;
	u32 rsv7322;
	u32 rsv7323;
	u32 rsv7324;
	u32 rsv7325;
	u32 rsv7326;
	u32 rsv7327;
	u32 rsv7328;
	u32 rsv7329;
	u32 rsv7330;
	u32 rsv7331;
	u32 rsv7332;
	u32 rsv7333;
	u32 rsv7334;
	u32 rsv7335;
	u32 rsv7336;
	u32 rsv7337;
	u32 rsv7338;
	u32 rsv7339;
	u32 rsv7340;
	u32 rsv7341;
	u32 rsv7342;
	u32 rsv7343;
	u32 rsv7344;
	u32 rsv7345;
	u32 rsv7346;
	u32 rsv7347;
	u32 rsv7348;
	u32 rsv7349;
	u32 rsv7350;
	u32 rsv7351;
	u32 rsv7352;
	u32 rsv7353;
	u32 rsv7354;
	u32 rsv7355;
	u32 rsv7356;
	u32 rsv7357;
	u32 rsv7358;
	u32 rsv7359;
	u32 rsv7360;
	u32 rsv7361;
	u32 rsv7362;
	u32 rsv7363;
	u32 rsv7364;
	u32 rsv7365;
	u32 rsv7366;
	u32 rsv7367;
	u32 rsv7368;
	u32 rsv7369;
	u32 rsv7370;
	u32 rsv7371;
	u32 rsv7372;
	u32 rsv7373;
	u32 rsv7374;
	u32 rsv7375;
	u32 rsv7376;
	u32 rsv7377;
	u32 rsv7378;
	u32 rsv7379;
	u32 rsv7380;
	u32 rsv7381;
	u32 rsv7382;
	u32 rsv7383;
	u32 rsv7384;
	u32 rsv7385;
	u32 rsv7386;
	u32 rsv7387;
	u32 rsv7388;
	u32 rsv7389;
	u32 rsv7390;
	u32 rsv7391;
	u32 rsv7392;
	u32 rsv7393;
	u32 rsv7394;
	u32 rsv7395;
	u32 rsv7396;
	u32 rsv7397;
	u32 rsv7398;
	u32 rsv7399;
	u32 rsv7400;
	u32 rsv7401;
	u32 rsv7402;
	u32 rsv7403;
	u32 rsv7404;
	u32 rsv7405;
	u32 rsv7406;
	u32 rsv7407;
	u32 rsv7408;
	u32 rsv7409;
	u32 rsv7410;
	u32 rsv7411;
	u32 rsv7412;
	u32 rsv7413;
	u32 rsv7414;
	u32 rsv7415;
	u32 rsv7416;
	u32 rsv7417;
	u32 rsv7418;
	u32 rsv7419;
	u32 rsv7420;
	u32 rsv7421;
	u32 rsv7422;
	u32 rsv7423;
	u32 rsv7424;
	u32 rsv7425;
	u32 rsv7426;
	u32 rsv7427;
	u32 rsv7428;
	u32 rsv7429;
	u32 rsv7430;
	u32 rsv7431;
	u32 rsv7432;
	u32 rsv7433;
	u32 rsv7434;
	u32 rsv7435;
	u32 rsv7436;
	u32 rsv7437;
	u32 rsv7438;
	u32 rsv7439;
	u32 rsv7440;
	u32 rsv7441;
	u32 rsv7442;
	u32 rsv7443;
	u32 rsv7444;
	u32 rsv7445;
	u32 rsv7446;
	u32 rsv7447;
	u32 rsv7448;
	u32 rsv7449;
	u32 rsv7450;
	u32 rsv7451;
	u32 rsv7452;
	u32 rsv7453;
	u32 rsv7454;
	u32 rsv7455;
	u32 rsv7456;
	u32 rsv7457;
	u32 rsv7458;
	u32 rsv7459;
	u32 rsv7460;
	u32 rsv7461;
	u32 rsv7462;
	u32 rsv7463;
	u32 rsv7464;
	u32 rsv7465;
	u32 rsv7466;
	u32 rsv7467;
	u32 rsv7468;
	u32 rsv7469;
	u32 rsv7470;
	u32 rsv7471;
	u32 rsv7472;
	u32 rsv7473;
	u32 rsv7474;
	u32 rsv7475;
	u32 rsv7476;
	u32 rsv7477;
	u32 rsv7478;
	u32 rsv7479;
	u32 rsv7480;
	u32 rsv7481;
	u32 rsv7482;
	u32 rsv7483;
	u32 rsv7484;
	u32 rsv7485;
	u32 rsv7486;
	u32 rsv7487;
	u32 rsv7488;
	u32 rsv7489;
	u32 rsv7490;
	u32 rsv7491;
	u32 rsv7492;
	u32 rsv7493;
	u32 rsv7494;
	u32 rsv7495;
	u32 rsv7496;
	u32 rsv7497;
	u32 rsv7498;
	u32 rsv7499;
	u32 rsv7500;
	u32 rsv7501;
	u32 rsv7502;
	u32 rsv7503;
	u32 rsv7504;
	u32 rsv7505;
	u32 rsv7506;
	u32 rsv7507;
	u32 rsv7508;
	u32 rsv7509;
	u32 rsv7510;
	u32 rsv7511;
	u32 rsv7512;
	u32 rsv7513;
	u32 rsv7514;
	u32 rsv7515;
	u32 rsv7516;
	u32 rsv7517;
	u32 rsv7518;
	u32 rsv7519;
	u32 rsv7520;
	u32 rsv7521;
	u32 rsv7522;
	u32 rsv7523;
	u32 rsv7524;
	u32 rsv7525;
	u32 rsv7526;
	u32 rsv7527;
	u32 rsv7528;
	u32 rsv7529;
	u32 rsv7530;
	u32 rsv7531;
	u32 rsv7532;
	u32 rsv7533;
	u32 rsv7534;
	u32 rsv7535;
	u32 rsv7536;
	u32 rsv7537;
	u32 rsv7538;
	u32 rsv7539;
	u32 rsv7540;
	u32 rsv7541;
	u32 rsv7542;
	u32 rsv7543;
	u32 rsv7544;
	u32 rsv7545;
	u32 rsv7546;
	u32 rsv7547;
	u32 rsv7548;
	u32 rsv7549;
	u32 rsv7550;
	u32 rsv7551;
	u32 rsv7552;
	u32 rsv7553;
	u32 rsv7554;
	u32 rsv7555;
	u32 rsv7556;
	u32 rsv7557;
	u32 rsv7558;
	u32 rsv7559;
	u32 rsv7560;
	u32 rsv7561;
	u32 rsv7562;
	u32 rsv7563;
	u32 rsv7564;
	u32 rsv7565;
	u32 rsv7566;
	u32 rsv7567;
	u32 rsv7568;
	u32 rsv7569;
	u32 rsv7570;
	u32 rsv7571;
	u32 rsv7572;
	u32 rsv7573;
	u32 rsv7574;
	u32 rsv7575;
	u32 rsv7576;
	u32 rsv7577;
	u32 rsv7578;
	u32 rsv7579;
	u32 rsv7580;
	u32 rsv7581;
	u32 rsv7582;
	u32 rsv7583;
	u32 rsv7584;
	u32 rsv7585;
	u32 rsv7586;
	u32 rsv7587;
	u32 rsv7588;
	u32 rsv7589;
	u32 rsv7590;
	u32 rsv7591;
	u32 rsv7592;
	u32 rsv7593;
	u32 rsv7594;
	u32 rsv7595;
	u32 rsv7596;
	u32 rsv7597;
	u32 rsv7598;
	u32 rsv7599;
	u32 rsv7600;
	u32 rsv7601;
	u32 rsv7602;
	u32 rsv7603;
	u32 rsv7604;
	u32 rsv7605;
	u32 rsv7606;
	u32 rsv7607;
	u32 rsv7608;
	u32 rsv7609;
	u32 rsv7610;
	u32 rsv7611;
	u32 rsv7612;
	u32 rsv7613;
	u32 rsv7614;
	u32 rsv7615;
	u32 rsv7616;
	u32 rsv7617;
	u32 rsv7618;
	u32 rsv7619;
	u32 rsv7620;
	u32 rsv7621;
	u32 rsv7622;
	u32 rsv7623;
	u32 rsv7624;
	u32 rsv7625;
	u32 rsv7626;
	u32 rsv7627;
	u32 rsv7628;
	u32 rsv7629;
	u32 rsv7630;
	u32 rsv7631;
	u32 rsv7632;
	u32 rsv7633;
	u32 rsv7634;
	u32 rsv7635;
	u32 rsv7636;
	u32 rsv7637;
	u32 rsv7638;
	u32 rsv7639;
	u32 rsv7640;
	u32 rsv7641;
	u32 rsv7642;
	u32 rsv7643;
	u32 rsv7644;
	u32 rsv7645;
	u32 rsv7646;
	u32 rsv7647;
	u32 rsv7648;
	u32 rsv7649;
	u32 rsv7650;
	u32 rsv7651;
	u32 rsv7652;
	u32 rsv7653;
	u32 rsv7654;
	u32 rsv7655;
	u32 rsv7656;
	u32 rsv7657;
	u32 rsv7658;
	u32 rsv7659;
	u32 rsv7660;
	u32 rsv7661;
	u32 rsv7662;
	u32 rsv7663;
	u32 rsv7664;
	u32 rsv7665;
	u32 rsv7666;
	u32 rsv7667;
	u32 rsv7668;
	u32 rsv7669;
	u32 rsv7670;
	u32 rsv7671;
	u32 rsv7672;
	u32 rsv7673;
	u32 rsv7674;
	u32 rsv7675;
	u32 rsv7676;
	u32 rsv7677;
	u32 rsv7678;
	u32 rsv7679;
	u32 rsv7680;
	u32 rsv7681;
	u32 rsv7682;
	u32 rsv7683;
	u32 rsv7684;
	u32 rsv7685;
	u32 rsv7686;
	u32 rsv7687;
	u32 rsv7688;
	u32 rsv7689;
	u32 rsv7690;
	u32 rsv7691;
	u32 rsv7692;
	u32 rsv7693;
	u32 rsv7694;
	u32 rsv7695;
	u32 rsv7696;
	u32 rsv7697;
	u32 rsv7698;
	u32 rsv7699;
	u32 rsv7700;
	u32 rsv7701;
	u32 rsv7702;
	u32 rsv7703;
	u32 rsv7704;
	u32 rsv7705;
	u32 rsv7706;
	u32 rsv7707;
	u32 rsv7708;
	u32 rsv7709;
	u32 rsv7710;
	u32 rsv7711;
	u32 rsv7712;
	u32 rsv7713;
	u32 rsv7714;
	u32 rsv7715;
	u32 rsv7716;
	u32 rsv7717;
	u32 rsv7718;
	u32 rsv7719;
	u32 rsv7720;
	u32 rsv7721;
	u32 rsv7722;
	u32 rsv7723;
	u32 rsv7724;
	u32 rsv7725;
	u32 rsv7726;
	u32 rsv7727;
	u32 rsv7728;
	u32 rsv7729;
	u32 rsv7730;
	u32 rsv7731;
	u32 rsv7732;
	u32 rsv7733;
	u32 rsv7734;
	u32 rsv7735;
	u32 rsv7736;
	u32 rsv7737;
	u32 rsv7738;
	u32 rsv7739;
	u32 rsv7740;
	u32 rsv7741;
	u32 rsv7742;
	u32 rsv7743;
	u32 rsv7744;
	u32 rsv7745;
	u32 rsv7746;
	u32 rsv7747;
	u32 rsv7748;
	u32 rsv7749;
	u32 rsv7750;
	u32 rsv7751;
	u32 rsv7752;
	u32 rsv7753;
	u32 rsv7754;
	u32 rsv7755;
	u32 rsv7756;
	u32 rsv7757;
	u32 rsv7758;
	u32 rsv7759;
	u32 rsv7760;
	u32 rsv7761;
	u32 rsv7762;
	u32 rsv7763;
	u32 rsv7764;
	u32 rsv7765;
	u32 rsv7766;
	u32 rsv7767;
	u32 rsv7768;
	u32 rsv7769;
	u32 rsv7770;
	u32 rsv7771;
	u32 rsv7772;
	u32 rsv7773;
	u32 rsv7774;
	u32 rsv7775;
	u32 rsv7776;
	u32 rsv7777;
	u32 rsv7778;
	u32 rsv7779;
	u32 rsv7780;
	u32 rsv7781;
	u32 rsv7782;
	u32 rsv7783;
	u32 rsv7784;
	u32 rsv7785;
	u32 rsv7786;
	u32 rsv7787;
	u32 rsv7788;
	u32 rsv7789;
	u32 rsv7790;
	u32 rsv7791;
	u32 rsv7792;
	u32 rsv7793;
	u32 rsv7794;
	u32 rsv7795;
	u32 rsv7796;
	u32 rsv7797;
	u32 rsv7798;
	u32 rsv7799;
	u32 rsv7800;
	u32 rsv7801;
	u32 rsv7802;
	u32 rsv7803;
	u32 rsv7804;
	u32 rsv7805;
	u32 rsv7806;
	u32 rsv7807;
	u32 rsv7808;
	u32 rsv7809;
	u32 rsv7810;
	u32 rsv7811;
	u32 rsv7812;
	u32 rsv7813;
	u32 rsv7814;
	u32 rsv7815;
	u32 rsv7816;
	u32 rsv7817;
	u32 rsv7818;
	u32 rsv7819;
	u32 rsv7820;
	u32 rsv7821;
	u32 rsv7822;
	u32 rsv7823;
	u32 rsv7824;
	u32 rsv7825;
	u32 rsv7826;
	u32 rsv7827;
	u32 rsv7828;
	u32 rsv7829;
	u32 rsv7830;
	u32 rsv7831;
	u32 rsv7832;
	u32 rsv7833;
	u32 rsv7834;
	u32 rsv7835;
	u32 rsv7836;
	u32 rsv7837;
	u32 rsv7838;
	u32 rsv7839;
	u32 rsv7840;
	u32 rsv7841;
	u32 rsv7842;
	u32 rsv7843;
	u32 rsv7844;
	u32 rsv7845;
	u32 rsv7846;
	u32 rsv7847;
	u32 rsv7848;
	u32 rsv7849;
	u32 rsv7850;
	u32 rsv7851;
	u32 rsv7852;
	u32 rsv7853;
	u32 rsv7854;
	u32 rsv7855;
	u32 rsv7856;
	u32 rsv7857;
	u32 rsv7858;
	u32 rsv7859;
	u32 rsv7860;
	u32 rsv7861;
	u32 rsv7862;
	u32 rsv7863;
	u32 rsv7864;
	u32 rsv7865;
	u32 rsv7866;
	u32 rsv7867;
	u32 rsv7868;
	u32 rsv7869;
	u32 rsv7870;
	u32 rsv7871;
	u32 rsv7872;
	u32 rsv7873;
	u32 rsv7874;
	u32 rsv7875;
	u32 rsv7876;
	u32 rsv7877;
	u32 rsv7878;
	u32 rsv7879;
	u32 rsv7880;
	u32 rsv7881;
	u32 rsv7882;
	u32 rsv7883;
	u32 rsv7884;
	u32 rsv7885;
	u32 rsv7886;
	u32 rsv7887;
	u32 rsv7888;
	u32 rsv7889;
	u32 rsv7890;
	u32 rsv7891;
	u32 rsv7892;
	u32 rsv7893;
	u32 rsv7894;
	u32 rsv7895;
	u32 rsv7896;
	u32 rsv7897;
	u32 rsv7898;
	u32 rsv7899;
	u32 rsv7900;
	u32 rsv7901;
	u32 rsv7902;
	u32 rsv7903;
	u32 rsv7904;
	u32 rsv7905;
	u32 rsv7906;
	u32 rsv7907;
	u32 rsv7908;
	u32 rsv7909;
	u32 rsv7910;
	u32 rsv7911;
	u32 rsv7912;
	u32 rsv7913;
	u32 rsv7914;
	u32 rsv7915;
	u32 rsv7916;
	u32 rsv7917;
	u32 rsv7918;
	u32 rsv7919;
	u32 rsv7920;
	u32 rsv7921;
	u32 rsv7922;
	u32 rsv7923;
	u32 rsv7924;
	u32 rsv7925;
	u32 rsv7926;
	u32 rsv7927;
	u32 rsv7928;
	u32 rsv7929;
	u32 rsv7930;
	u32 rsv7931;
	u32 rsv7932;
	u32 rsv7933;
	u32 rsv7934;
	u32 rsv7935;
	u32 rsv7936;
	u32 rsv7937;
	u32 rsv7938;
	u32 rsv7939;
	u32 rsv7940;
	u32 rsv7941;
	u32 rsv7942;
	u32 rsv7943;
	u32 rsv7944;
	u32 rsv7945;
	u32 rsv7946;
	u32 rsv7947;
	u32 rsv7948;
	u32 rsv7949;
	u32 rsv7950;
	u32 rsv7951;
	u32 rsv7952;
	u32 rsv7953;
	u32 rsv7954;
	u32 rsv7955;
	u32 rsv7956;
	u32 rsv7957;
	u32 rsv7958;
	u32 rsv7959;
	u32 rsv7960;
	u32 rsv7961;
	u32 rsv7962;
	u32 rsv7963;
	u32 rsv7964;
	u32 rsv7965;
	u32 rsv7966;
	u32 rsv7967;
	u32 rsv7968;
	u32 rsv7969;
	u32 rsv7970;
	u32 rsv7971;
	u32 rsv7972;
	u32 rsv7973;
	u32 rsv7974;
	u32 rsv7975;
	u32 rsv7976;
	u32 rsv7977;
	u32 rsv7978;
	u32 rsv7979;
	u32 rsv7980;
	u32 rsv7981;
	u32 rsv7982;
	u32 rsv7983;
	u32 rsv7984;
	u32 rsv7985;
	u32 rsv7986;
	u32 rsv7987;
	u32 rsv7988;
	u32 rsv7989;
	u32 rsv7990;
	u32 rsv7991;
	u32 rsv7992;
	u32 rsv7993;
	u32 rsv7994;
	u32 rsv7995;
	u32 rsv7996;
	u32 rsv7997;
	u32 rsv7998;
	u32 rsv7999;
	u32 rsv8000;
	u32 rsv8001;
	u32 rsv8002;
	u32 rsv8003;
	u32 rsv8004;
	u32 rsv8005;
	u32 rsv8006;
	u32 rsv8007;
	u32 rsv8008;
	u32 rsv8009;
	u32 rsv8010;
	u32 rsv8011;
	u32 rsv8012;
	u32 rsv8013;
	u32 rsv8014;
	u32 rsv8015;
	u32 rsv8016;
	u32 rsv8017;
	u32 rsv8018;
	u32 rsv8019;
	u32 rsv8020;
	u32 rsv8021;
	u32 rsv8022;
	u32 rsv8023;
	u32 rsv8024;
	u32 rsv8025;
	u32 rsv8026;
	u32 rsv8027;
	u32 rsv8028;
	u32 rsv8029;
	u32 rsv8030;
	u32 rsv8031;
	u32 rsv8032;
	u32 rsv8033;
	u32 rsv8034;
	u32 rsv8035;
	u32 rsv8036;
	u32 rsv8037;
	u32 rsv8038;
	u32 rsv8039;
	u32 rsv8040;
	u32 rsv8041;
	u32 rsv8042;
	u32 rsv8043;
	u32 rsv8044;
	u32 rsv8045;
	u32 rsv8046;
	u32 rsv8047;
	u32 rsv8048;
	u32 rsv8049;
	u32 rsv8050;
	u32 rsv8051;
	u32 rsv8052;
	u32 rsv8053;
	u32 rsv8054;
	u32 rsv8055;
	u32 rsv8056;
	u32 rsv8057;
	u32 rsv8058;
	u32 rsv8059;
	u32 rsv8060;
	u32 rsv8061;
	u32 rsv8062;
	u32 rsv8063;
	u32 rsv8064;
	u32 rsv8065;
	u32 rsv8066;
	u32 rsv8067;
	u32 rsv8068;
	u32 rsv8069;
	u32 rsv8070;
	u32 rsv8071;
	u32 rsv8072;
	u32 rsv8073;
	u32 rsv8074;
	u32 rsv8075;
	u32 rsv8076;
	u32 rsv8077;
	u32 rsv8078;
	u32 rsv8079;
	u32 rsv8080;
	u32 rsv8081;
	u32 rsv8082;
	u32 rsv8083;
	u32 rsv8084;
	u32 rsv8085;
	u32 rsv8086;
	u32 rsv8087;
	u32 rsv8088;
	u32 rsv8089;
	u32 rsv8090;
	u32 rsv8091;
	u32 rsv8092;
	u32 rsv8093;
	u32 rsv8094;
	u32 rsv8095;
	u32 rsv8096;
	u32 rsv8097;
	u32 rsv8098;
	u32 rsv8099;
	u32 rsv8100;
	u32 rsv8101;
	u32 rsv8102;
	u32 rsv8103;
	u32 rsv8104;
	u32 rsv8105;
	u32 rsv8106;
	u32 rsv8107;
	u32 rsv8108;
	u32 rsv8109;
	u32 rsv8110;
	u32 rsv8111;
	u32 rsv8112;
	u32 rsv8113;
	u32 rsv8114;
	u32 rsv8115;
	u32 rsv8116;
	u32 rsv8117;
	u32 rsv8118;
	u32 rsv8119;
	u32 rsv8120;
	u32 rsv8121;
	u32 rsv8122;
	u32 rsv8123;
	u32 rsv8124;
	u32 rsv8125;
	u32 rsv8126;
	u32 rsv8127;
	u32 rsv8128;
	u32 rsv8129;
	u32 rsv8130;
	u32 rsv8131;
	u32 rsv8132;
	u32 rsv8133;
	u32 rsv8134;
	u32 rsv8135;
	u32 rsv8136;
	u32 rsv8137;
	u32 rsv8138;
	u32 rsv8139;
	u32 rsv8140;
	u32 rsv8141;
	u32 rsv8142;
	u32 rsv8143;
	u32 rsv8144;
	u32 rsv8145;
	u32 rsv8146;
	u32 rsv8147;
	u32 rsv8148;
	u32 rsv8149;
	u32 rsv8150;
	u32 rsv8151;
	u32 rsv8152;
	u32 rsv8153;
	u32 rsv8154;
	u32 rsv8155;
	u32 rsv8156;
	u32 rsv8157;
	u32 rsv8158;
	u32 rsv8159;
	u32 rsv8160;
	u32 rsv8161;
	u32 rsv8162;
	u32 rsv8163;
	u32 rsv8164;
	u32 rsv8165;
	u32 rsv8166;
	u32 rsv8167;
	u32 rsv8168;
	u32 rsv8169;
	u32 rsv8170;
	u32 rsv8171;
	u32 rsv8172;
	u32 rsv8173;
	u32 rsv8174;
	u32 rsv8175;
	u32 rsv8176;
	u32 rsv8177;
	u32 rsv8178;
	u32 rsv8179;
	u32 rsv8180;
	u32 rsv8181;
	u32 rsv8182;
	u32 rsv8183;
	u32 rsv8184;
	u32 rsv8185;
	u32 rsv8186;
	u32 rsv8187;
	u32 rsv8188;
	u32 rsv8189;
	u32 rsv8190;
	u32 rsv8191;
	u32 RxPktMsgFifo0[1];	/* 0x00008000 */
	u32 RxPktMsgFifo1[1];	/* 0x00008004 */
	u32 RxPktMsgFifo2[1];	/* 0x00008008 */
	u32 RxPktMsgFifo3[1];	/* 0x0000800c */
	u32 RxPktMsgFifo4[1];	/* 0x00008010 */
	u32 RxPktMsgFifo5[1];	/* 0x00008014 */
	u32 RxPktMsgFifo6[1];	/* 0x00008018 */
	u32 RxPktMsgFifo7[1];	/* 0x0000801c */
	u32 RxPktMsgFifo8[1];	/* 0x00008020 */
	u32 RxPktMsgFifo9[1];	/* 0x00008024 */
	u32 RxPktMsgFifo10[1];	/* 0x00008028 */
	u32 RxPktMsgFifo11[1];	/* 0x0000802c */
	u32 RxPktMsgFifo12[1];	/* 0x00008030 */
	u32 RxPktMsgFifo13[1];	/* 0x00008034 */
	u32 RxPktMsgFifo14[1];	/* 0x00008038 */
	u32 RxPktMsgFifo15[1];	/* 0x0000803c */
	u32 RxPktMsgFifo16[1];	/* 0x00008040 */
	u32 RxPktMsgFifo17[1];	/* 0x00008044 */
	u32 RxPktMsgFifo18[1];	/* 0x00008048 */
	u32 RxPktMsgFifo19[1];	/* 0x0000804c */
	u32 RxPktMsgFifo20[1];	/* 0x00008050 */
	u32 RxPktMsgFifo21[1];	/* 0x00008054 */
	u32 RxPktMsgFifo22[1];	/* 0x00008058 */
	u32 RxPktMsgFifo23[1];	/* 0x0000805c */
	u32 RxPktMsgFifo24[1];	/* 0x00008060 */
	u32 RxPktMsgFifo25[1];	/* 0x00008064 */
	u32 RxPktMsgFifo26[1];	/* 0x00008068 */
	u32 RxPktMsgFifo27[1];	/* 0x0000806c */
	u32 RxPktMsgFifo28[1];	/* 0x00008070 */
	u32 RxPktMsgFifo29[1];	/* 0x00008074 */
	u32 RxPktMsgFifo30[1];	/* 0x00008078 */
	u32 RxPktMsgFifo31[1];	/* 0x0000807c */
	u32 RxPktMsgFifo32[1];	/* 0x00008080 */
	u32 RxPktMsgFifo33[1];	/* 0x00008084 */
	u32 RxPktMsgFifo34[1];	/* 0x00008088 */
	u32 RxPktMsgFifo35[1];	/* 0x0000808c */
	u32 RxPktMsgFifo36[1];	/* 0x00008090 */
	u32 RxPktMsgFifo37[1];	/* 0x00008094 */
	u32 RxPktMsgFifo38[1];	/* 0x00008098 */
	u32 RxPktMsgFifo39[1];	/* 0x0000809c */
	u32 RxPktMsgFifo40[1];	/* 0x000080a0 */
	u32 RxPktMsgFifo41[1];	/* 0x000080a4 */
	u32 RxPktMsgFifo42[1];	/* 0x000080a8 */
	u32 RxPktMsgFifo43[1];	/* 0x000080ac */
	u32 RxPktMsgFifo44[1];	/* 0x000080b0 */
	u32 RxPktMsgFifo45[1];	/* 0x000080b4 */
	u32 RxPktMsgFifo46[1];	/* 0x000080b8 */
	u32 RxPktMsgFifo47[1];	/* 0x000080bc */
	u32 RxPktMsgFifo48[1];	/* 0x000080c0 */
	u32 RxPktMsgFifo49[1];	/* 0x000080c4 */
	u32 RxPktMsgFifo50[1];	/* 0x000080c8 */
	u32 RxPktMsgFifo51[1];	/* 0x000080cc */
	u32 RxPktMsgFifo52[1];	/* 0x000080d0 */
	u32 RxPktMsgFifo53[1];	/* 0x000080d4 */
	u32 RxPktMsgFifo54[1];	/* 0x000080d8 */
	u32 RxPktMsgFifo55[1];	/* 0x000080dc */
	u32 RxPktMsgFifo56[1];	/* 0x000080e0 */
	u32 RxPktMsgFifo57[1];	/* 0x000080e4 */
	u32 RxPktMsgFifo58[1];	/* 0x000080e8 */
	u32 RxPktMsgFifo59[1];	/* 0x000080ec */
	u32 RxPktMsgFifo60[1];	/* 0x000080f0 */
	u32 RxPktMsgFifo61[1];	/* 0x000080f4 */
	u32 RxPktMsgFifo62[1];	/* 0x000080f8 */
	u32 RxPktMsgFifo63[1];	/* 0x000080fc */
	u32 RxPktMsgFifo64[1];	/* 0x00008100 */
	u32 RxPktMsgFifo65[1];	/* 0x00008104 */
	u32 RxPktMsgFifo66[1];	/* 0x00008108 */
	u32 RxPktMsgFifo67[1];	/* 0x0000810c */
	u32 RxPktMsgFifo68[1];	/* 0x00008110 */
	u32 RxPktMsgFifo69[1];	/* 0x00008114 */
	u32 RxPktMsgFifo70[1];	/* 0x00008118 */
	u32 RxPktMsgFifo71[1];	/* 0x0000811c */
	u32 RxPktMsgFifo72[1];	/* 0x00008120 */
	u32 RxPktMsgFifo73[1];	/* 0x00008124 */
	u32 RxPktMsgFifo74[1];	/* 0x00008128 */
	u32 RxPktMsgFifo75[1];	/* 0x0000812c */
	u32 RxPktMsgFifo76[1];	/* 0x00008130 */
	u32 RxPktMsgFifo77[1];	/* 0x00008134 */
	u32 RxPktMsgFifo78[1];	/* 0x00008138 */
	u32 RxPktMsgFifo79[1];	/* 0x0000813c */
	u32 RxPktMsgFifo80[1];	/* 0x00008140 */
	u32 RxPktMsgFifo81[1];	/* 0x00008144 */
	u32 RxPktMsgFifo82[1];	/* 0x00008148 */
	u32 RxPktMsgFifo83[1];	/* 0x0000814c */
	u32 RxPktMsgFifo84[1];	/* 0x00008150 */
	u32 RxPktMsgFifo85[1];	/* 0x00008154 */
	u32 RxPktMsgFifo86[1];	/* 0x00008158 */
	u32 RxPktMsgFifo87[1];	/* 0x0000815c */
	u32 RxPktMsgFifo88[1];	/* 0x00008160 */
	u32 RxPktMsgFifo89[1];	/* 0x00008164 */
	u32 RxPktMsgFifo90[1];	/* 0x00008168 */
	u32 RxPktMsgFifo91[1];	/* 0x0000816c */
	u32 RxPktMsgFifo92[1];	/* 0x00008170 */
	u32 RxPktMsgFifo93[1];	/* 0x00008174 */
	u32 RxPktMsgFifo94[1];	/* 0x00008178 */
	u32 RxPktMsgFifo95[1];	/* 0x0000817c */
	u32 RxPktMsgFifo96[1];	/* 0x00008180 */
	u32 RxPktMsgFifo97[1];	/* 0x00008184 */
	u32 RxPktMsgFifo98[1];	/* 0x00008188 */
	u32 RxPktMsgFifo99[1];	/* 0x0000818c */
	u32 RxPktMsgFifo100[1];	/* 0x00008190 */
	u32 RxPktMsgFifo101[1];	/* 0x00008194 */
	u32 RxPktMsgFifo102[1];	/* 0x00008198 */
	u32 RxPktMsgFifo103[1];	/* 0x0000819c */
	u32 RxPktMsgFifo104[1];	/* 0x000081a0 */
	u32 RxPktMsgFifo105[1];	/* 0x000081a4 */
	u32 RxPktMsgFifo106[1];	/* 0x000081a8 */
	u32 RxPktMsgFifo107[1];	/* 0x000081ac */
	u32 RxPktMsgFifo108[1];	/* 0x000081b0 */
	u32 RxPktMsgFifo109[1];	/* 0x000081b4 */
	u32 RxPktMsgFifo110[1];	/* 0x000081b8 */
	u32 RxPktMsgFifo111[1];	/* 0x000081bc */
	u32 RxPktMsgFifo112[1];	/* 0x000081c0 */
	u32 RxPktMsgFifo113[1];	/* 0x000081c4 */
	u32 RxPktMsgFifo114[1];	/* 0x000081c8 */
	u32 RxPktMsgFifo115[1];	/* 0x000081cc */
	u32 RxPktMsgFifo116[1];	/* 0x000081d0 */
	u32 RxPktMsgFifo117[1];	/* 0x000081d4 */
	u32 RxPktMsgFifo118[1];	/* 0x000081d8 */
	u32 RxPktMsgFifo119[1];	/* 0x000081dc */
	u32 RxPktMsgFifo120[1];	/* 0x000081e0 */
	u32 RxPktMsgFifo121[1];	/* 0x000081e4 */
	u32 RxPktMsgFifo122[1];	/* 0x000081e8 */
	u32 RxPktMsgFifo123[1];	/* 0x000081ec */
	u32 RxPktMsgFifo124[1];	/* 0x000081f0 */
	u32 RxPktMsgFifo125[1];	/* 0x000081f4 */
	u32 RxPktMsgFifo126[1];	/* 0x000081f8 */
	u32 RxPktMsgFifo127[1];	/* 0x000081fc */
	u32 RxPktMsgFifo128[1];	/* 0x00008200 */
	u32 RxPktMsgFifo129[1];	/* 0x00008204 */
	u32 RxPktMsgFifo130[1];	/* 0x00008208 */
	u32 RxPktMsgFifo131[1];	/* 0x0000820c */
	u32 RxPktMsgFifo132[1];	/* 0x00008210 */
	u32 RxPktMsgFifo133[1];	/* 0x00008214 */
	u32 RxPktMsgFifo134[1];	/* 0x00008218 */
	u32 RxPktMsgFifo135[1];	/* 0x0000821c */
	u32 RxPktMsgFifo136[1];	/* 0x00008220 */
	u32 RxPktMsgFifo137[1];	/* 0x00008224 */
	u32 RxPktMsgFifo138[1];	/* 0x00008228 */
	u32 RxPktMsgFifo139[1];	/* 0x0000822c */
	u32 RxPktMsgFifo140[1];	/* 0x00008230 */
	u32 RxPktMsgFifo141[1];	/* 0x00008234 */
	u32 RxPktMsgFifo142[1];	/* 0x00008238 */
	u32 RxPktMsgFifo143[1];	/* 0x0000823c */
	u32 RxPktMsgFifo144[1];	/* 0x00008240 */
	u32 RxPktMsgFifo145[1];	/* 0x00008244 */
	u32 RxPktMsgFifo146[1];	/* 0x00008248 */
	u32 RxPktMsgFifo147[1];	/* 0x0000824c */
	u32 RxPktMsgFifo148[1];	/* 0x00008250 */
	u32 RxPktMsgFifo149[1];	/* 0x00008254 */
	u32 RxPktMsgFifo150[1];	/* 0x00008258 */
	u32 RxPktMsgFifo151[1];	/* 0x0000825c */
	u32 RxPktMsgFifo152[1];	/* 0x00008260 */
	u32 RxPktMsgFifo153[1];	/* 0x00008264 */
	u32 RxPktMsgFifo154[1];	/* 0x00008268 */
	u32 RxPktMsgFifo155[1];	/* 0x0000826c */
	u32 RxPktMsgFifo156[1];	/* 0x00008270 */
	u32 RxPktMsgFifo157[1];	/* 0x00008274 */
	u32 RxPktMsgFifo158[1];	/* 0x00008278 */
	u32 RxPktMsgFifo159[1];	/* 0x0000827c */
	u32 RxPktMsgFifo160[1];	/* 0x00008280 */
	u32 RxPktMsgFifo161[1];	/* 0x00008284 */
	u32 RxPktMsgFifo162[1];	/* 0x00008288 */
	u32 RxPktMsgFifo163[1];	/* 0x0000828c */
	u32 RxPktMsgFifo164[1];	/* 0x00008290 */
	u32 RxPktMsgFifo165[1];	/* 0x00008294 */
	u32 RxPktMsgFifo166[1];	/* 0x00008298 */
	u32 RxPktMsgFifo167[1];	/* 0x0000829c */
	u32 RxPktMsgFifo168[1];	/* 0x000082a0 */
	u32 RxPktMsgFifo169[1];	/* 0x000082a4 */
	u32 RxPktMsgFifo170[1];	/* 0x000082a8 */
	u32 RxPktMsgFifo171[1];	/* 0x000082ac */
	u32 RxPktMsgFifo172[1];	/* 0x000082b0 */
	u32 RxPktMsgFifo173[1];	/* 0x000082b4 */
	u32 RxPktMsgFifo174[1];	/* 0x000082b8 */
	u32 RxPktMsgFifo175[1];	/* 0x000082bc */
	u32 RxPktMsgFifo176[1];	/* 0x000082c0 */
	u32 RxPktMsgFifo177[1];	/* 0x000082c4 */
	u32 RxPktMsgFifo178[1];	/* 0x000082c8 */
	u32 RxPktMsgFifo179[1];	/* 0x000082cc */
	u32 RxPktMsgFifo180[1];	/* 0x000082d0 */
	u32 RxPktMsgFifo181[1];	/* 0x000082d4 */
	u32 RxPktMsgFifo182[1];	/* 0x000082d8 */
	u32 RxPktMsgFifo183[1];	/* 0x000082dc */
	u32 RxPktMsgFifo184[1];	/* 0x000082e0 */
	u32 RxPktMsgFifo185[1];	/* 0x000082e4 */
	u32 RxPktMsgFifo186[1];	/* 0x000082e8 */
	u32 RxPktMsgFifo187[1];	/* 0x000082ec */
	u32 RxPktMsgFifo188[1];	/* 0x000082f0 */
	u32 RxPktMsgFifo189[1];	/* 0x000082f4 */
	u32 RxPktMsgFifo190[1];	/* 0x000082f8 */
	u32 RxPktMsgFifo191[1];	/* 0x000082fc */
	u32 RxPktMsgFifo192[1];	/* 0x00008300 */
	u32 RxPktMsgFifo193[1];	/* 0x00008304 */
	u32 RxPktMsgFifo194[1];	/* 0x00008308 */
	u32 RxPktMsgFifo195[1];	/* 0x0000830c */
	u32 RxPktMsgFifo196[1];	/* 0x00008310 */
	u32 RxPktMsgFifo197[1];	/* 0x00008314 */
	u32 RxPktMsgFifo198[1];	/* 0x00008318 */
	u32 RxPktMsgFifo199[1];	/* 0x0000831c */
	u32 RxPktMsgFifo200[1];	/* 0x00008320 */
	u32 RxPktMsgFifo201[1];	/* 0x00008324 */
	u32 RxPktMsgFifo202[1];	/* 0x00008328 */
	u32 RxPktMsgFifo203[1];	/* 0x0000832c */
	u32 RxPktMsgFifo204[1];	/* 0x00008330 */
	u32 RxPktMsgFifo205[1];	/* 0x00008334 */
	u32 RxPktMsgFifo206[1];	/* 0x00008338 */
	u32 RxPktMsgFifo207[1];	/* 0x0000833c */
	u32 RxPktMsgFifo208[1];	/* 0x00008340 */
	u32 RxPktMsgFifo209[1];	/* 0x00008344 */
	u32 RxPktMsgFifo210[1];	/* 0x00008348 */
	u32 RxPktMsgFifo211[1];	/* 0x0000834c */
	u32 RxPktMsgFifo212[1];	/* 0x00008350 */
	u32 RxPktMsgFifo213[1];	/* 0x00008354 */
	u32 RxPktMsgFifo214[1];	/* 0x00008358 */
	u32 RxPktMsgFifo215[1];	/* 0x0000835c */
	u32 RxPktMsgFifo216[1];	/* 0x00008360 */
	u32 RxPktMsgFifo217[1];	/* 0x00008364 */
	u32 RxPktMsgFifo218[1];	/* 0x00008368 */
	u32 RxPktMsgFifo219[1];	/* 0x0000836c */
	u32 RxPktMsgFifo220[1];	/* 0x00008370 */
	u32 RxPktMsgFifo221[1];	/* 0x00008374 */
	u32 RxPktMsgFifo222[1];	/* 0x00008378 */
	u32 RxPktMsgFifo223[1];	/* 0x0000837c */
	u32 RxPktMsgFifo224[1];	/* 0x00008380 */
	u32 RxPktMsgFifo225[1];	/* 0x00008384 */
	u32 RxPktMsgFifo226[1];	/* 0x00008388 */
	u32 RxPktMsgFifo227[1];	/* 0x0000838c */
	u32 RxPktMsgFifo228[1];	/* 0x00008390 */
	u32 RxPktMsgFifo229[1];	/* 0x00008394 */
	u32 RxPktMsgFifo230[1];	/* 0x00008398 */
	u32 RxPktMsgFifo231[1];	/* 0x0000839c */
	u32 RxPktMsgFifo232[1];	/* 0x000083a0 */
	u32 RxPktMsgFifo233[1];	/* 0x000083a4 */
	u32 RxPktMsgFifo234[1];	/* 0x000083a8 */
	u32 RxPktMsgFifo235[1];	/* 0x000083ac */
	u32 RxPktMsgFifo236[1];	/* 0x000083b0 */
	u32 RxPktMsgFifo237[1];	/* 0x000083b4 */
	u32 RxPktMsgFifo238[1];	/* 0x000083b8 */
	u32 RxPktMsgFifo239[1];	/* 0x000083bc */
	u32 RxPktMsgFifo240[1];	/* 0x000083c0 */
	u32 RxPktMsgFifo241[1];	/* 0x000083c4 */
	u32 RxPktMsgFifo242[1];	/* 0x000083c8 */
	u32 RxPktMsgFifo243[1];	/* 0x000083cc */
	u32 RxPktMsgFifo244[1];	/* 0x000083d0 */
	u32 RxPktMsgFifo245[1];	/* 0x000083d4 */
	u32 RxPktMsgFifo246[1];	/* 0x000083d8 */
	u32 RxPktMsgFifo247[1];	/* 0x000083dc */
	u32 RxPktMsgFifo248[1];	/* 0x000083e0 */
	u32 RxPktMsgFifo249[1];	/* 0x000083e4 */
	u32 RxPktMsgFifo250[1];	/* 0x000083e8 */
	u32 RxPktMsgFifo251[1];	/* 0x000083ec */
	u32 RxPktMsgFifo252[1];	/* 0x000083f0 */
	u32 RxPktMsgFifo253[1];	/* 0x000083f4 */
	u32 RxPktMsgFifo254[1];	/* 0x000083f8 */
	u32 RxPktMsgFifo255[1];	/* 0x000083fc */
	u32 RxPktMsgFifo256[1];	/* 0x00008400 */
	u32 RxPktMsgFifo257[1];	/* 0x00008404 */
	u32 RxPktMsgFifo258[1];	/* 0x00008408 */
	u32 RxPktMsgFifo259[1];	/* 0x0000840c */
	u32 RxPktMsgFifo260[1];	/* 0x00008410 */
	u32 RxPktMsgFifo261[1];	/* 0x00008414 */
	u32 RxPktMsgFifo262[1];	/* 0x00008418 */
	u32 RxPktMsgFifo263[1];	/* 0x0000841c */
	u32 RxPktMsgFifo264[1];	/* 0x00008420 */
	u32 RxPktMsgFifo265[1];	/* 0x00008424 */
	u32 RxPktMsgFifo266[1];	/* 0x00008428 */
	u32 RxPktMsgFifo267[1];	/* 0x0000842c */
	u32 RxPktMsgFifo268[1];	/* 0x00008430 */
	u32 RxPktMsgFifo269[1];	/* 0x00008434 */
	u32 RxPktMsgFifo270[1];	/* 0x00008438 */
	u32 RxPktMsgFifo271[1];	/* 0x0000843c */
	u32 RxPktMsgFifo272[1];	/* 0x00008440 */
	u32 RxPktMsgFifo273[1];	/* 0x00008444 */
	u32 RxPktMsgFifo274[1];	/* 0x00008448 */
	u32 RxPktMsgFifo275[1];	/* 0x0000844c */
	u32 RxPktMsgFifo276[1];	/* 0x00008450 */
	u32 RxPktMsgFifo277[1];	/* 0x00008454 */
	u32 RxPktMsgFifo278[1];	/* 0x00008458 */
	u32 RxPktMsgFifo279[1];	/* 0x0000845c */
	u32 RxPktMsgFifo280[1];	/* 0x00008460 */
	u32 RxPktMsgFifo281[1];	/* 0x00008464 */
	u32 RxPktMsgFifo282[1];	/* 0x00008468 */
	u32 RxPktMsgFifo283[1];	/* 0x0000846c */
	u32 RxPktMsgFifo284[1];	/* 0x00008470 */
	u32 RxPktMsgFifo285[1];	/* 0x00008474 */
	u32 RxPktMsgFifo286[1];	/* 0x00008478 */
	u32 RxPktMsgFifo287[1];	/* 0x0000847c */
	u32 RxPktMsgFifo288[1];	/* 0x00008480 */
	u32 RxPktMsgFifo289[1];	/* 0x00008484 */
	u32 RxPktMsgFifo290[1];	/* 0x00008488 */
	u32 RxPktMsgFifo291[1];	/* 0x0000848c */
	u32 RxPktMsgFifo292[1];	/* 0x00008490 */
	u32 RxPktMsgFifo293[1];	/* 0x00008494 */
	u32 RxPktMsgFifo294[1];	/* 0x00008498 */
	u32 RxPktMsgFifo295[1];	/* 0x0000849c */
	u32 RxPktMsgFifo296[1];	/* 0x000084a0 */
	u32 RxPktMsgFifo297[1];	/* 0x000084a4 */
	u32 RxPktMsgFifo298[1];	/* 0x000084a8 */
	u32 RxPktMsgFifo299[1];	/* 0x000084ac */
	u32 RxPktMsgFifo300[1];	/* 0x000084b0 */
	u32 RxPktMsgFifo301[1];	/* 0x000084b4 */
	u32 RxPktMsgFifo302[1];	/* 0x000084b8 */
	u32 RxPktMsgFifo303[1];	/* 0x000084bc */
	u32 RxPktMsgFifo304[1];	/* 0x000084c0 */
	u32 RxPktMsgFifo305[1];	/* 0x000084c4 */
	u32 RxPktMsgFifo306[1];	/* 0x000084c8 */
	u32 RxPktMsgFifo307[1];	/* 0x000084cc */
	u32 RxPktMsgFifo308[1];	/* 0x000084d0 */
	u32 RxPktMsgFifo309[1];	/* 0x000084d4 */
	u32 RxPktMsgFifo310[1];	/* 0x000084d8 */
	u32 RxPktMsgFifo311[1];	/* 0x000084dc */
	u32 RxPktMsgFifo312[1];	/* 0x000084e0 */
	u32 RxPktMsgFifo313[1];	/* 0x000084e4 */
	u32 RxPktMsgFifo314[1];	/* 0x000084e8 */
	u32 RxPktMsgFifo315[1];	/* 0x000084ec */
	u32 RxPktMsgFifo316[1];	/* 0x000084f0 */
	u32 RxPktMsgFifo317[1];	/* 0x000084f4 */
	u32 RxPktMsgFifo318[1];	/* 0x000084f8 */
	u32 RxPktMsgFifo319[1];	/* 0x000084fc */
	u32 RxPktMsgFifo320[1];	/* 0x00008500 */
	u32 RxPktMsgFifo321[1];	/* 0x00008504 */
	u32 RxPktMsgFifo322[1];	/* 0x00008508 */
	u32 RxPktMsgFifo323[1];	/* 0x0000850c */
	u32 RxPktMsgFifo324[1];	/* 0x00008510 */
	u32 rsv8517;
	u32 rsv8518;
	u32 rsv8519;
	u32 rsv8520;
	u32 rsv8521;
	u32 rsv8522;
	u32 rsv8523;
	u32 rsv8524;
	u32 rsv8525;
	u32 rsv8526;
	u32 rsv8527;
	u32 rsv8528;
	u32 rsv8529;
	u32 rsv8530;
	u32 rsv8531;
	u32 rsv8532;
	u32 rsv8533;
	u32 rsv8534;
	u32 rsv8535;
	u32 rsv8536;
	u32 rsv8537;
	u32 rsv8538;
	u32 rsv8539;
	u32 rsv8540;
	u32 rsv8541;
	u32 rsv8542;
	u32 rsv8543;
	u32 rsv8544;
	u32 rsv8545;
	u32 rsv8546;
	u32 rsv8547;
	u32 rsv8548;
	u32 rsv8549;
	u32 rsv8550;
	u32 rsv8551;
	u32 rsv8552;
	u32 rsv8553;
	u32 rsv8554;
	u32 rsv8555;
	u32 rsv8556;
	u32 rsv8557;
	u32 rsv8558;
	u32 rsv8559;
	u32 rsv8560;
	u32 rsv8561;
	u32 rsv8562;
	u32 rsv8563;
	u32 rsv8564;
	u32 rsv8565;
	u32 rsv8566;
	u32 rsv8567;
	u32 rsv8568;
	u32 rsv8569;
	u32 rsv8570;
	u32 rsv8571;
	u32 rsv8572;
	u32 rsv8573;
	u32 rsv8574;
	u32 rsv8575;
	u32 rsv8576;
	u32 rsv8577;
	u32 rsv8578;
	u32 rsv8579;
	u32 rsv8580;
	u32 rsv8581;
	u32 rsv8582;
	u32 rsv8583;
	u32 rsv8584;
	u32 rsv8585;
	u32 rsv8586;
	u32 rsv8587;
	u32 rsv8588;
	u32 rsv8589;
	u32 rsv8590;
	u32 rsv8591;
	u32 rsv8592;
	u32 rsv8593;
	u32 rsv8594;
	u32 rsv8595;
	u32 rsv8596;
	u32 rsv8597;
	u32 rsv8598;
	u32 rsv8599;
	u32 rsv8600;
	u32 rsv8601;
	u32 rsv8602;
	u32 rsv8603;
	u32 rsv8604;
	u32 rsv8605;
	u32 rsv8606;
	u32 rsv8607;
	u32 rsv8608;
	u32 rsv8609;
	u32 rsv8610;
	u32 rsv8611;
	u32 rsv8612;
	u32 rsv8613;
	u32 rsv8614;
	u32 rsv8615;
	u32 rsv8616;
	u32 rsv8617;
	u32 rsv8618;
	u32 rsv8619;
	u32 rsv8620;
	u32 rsv8621;
	u32 rsv8622;
	u32 rsv8623;
	u32 rsv8624;
	u32 rsv8625;
	u32 rsv8626;
	u32 rsv8627;
	u32 rsv8628;
	u32 rsv8629;
	u32 rsv8630;
	u32 rsv8631;
	u32 rsv8632;
	u32 rsv8633;
	u32 rsv8634;
	u32 rsv8635;
	u32 rsv8636;
	u32 rsv8637;
	u32 rsv8638;
	u32 rsv8639;
	u32 rsv8640;
	u32 rsv8641;
	u32 rsv8642;
	u32 rsv8643;
	u32 rsv8644;
	u32 rsv8645;
	u32 rsv8646;
	u32 rsv8647;
	u32 rsv8648;
	u32 rsv8649;
	u32 rsv8650;
	u32 rsv8651;
	u32 rsv8652;
	u32 rsv8653;
	u32 rsv8654;
	u32 rsv8655;
	u32 rsv8656;
	u32 rsv8657;
	u32 rsv8658;
	u32 rsv8659;
	u32 rsv8660;
	u32 rsv8661;
	u32 rsv8662;
	u32 rsv8663;
	u32 rsv8664;
	u32 rsv8665;
	u32 rsv8666;
	u32 rsv8667;
	u32 rsv8668;
	u32 rsv8669;
	u32 rsv8670;
	u32 rsv8671;
	u32 rsv8672;
	u32 rsv8673;
	u32 rsv8674;
	u32 rsv8675;
	u32 rsv8676;
	u32 rsv8677;
	u32 rsv8678;
	u32 rsv8679;
	u32 rsv8680;
	u32 rsv8681;
	u32 rsv8682;
	u32 rsv8683;
	u32 rsv8684;
	u32 rsv8685;
	u32 rsv8686;
	u32 rsv8687;
	u32 rsv8688;
	u32 rsv8689;
	u32 rsv8690;
	u32 rsv8691;
	u32 rsv8692;
	u32 rsv8693;
	u32 rsv8694;
	u32 rsv8695;
	u32 rsv8696;
	u32 rsv8697;
	u32 rsv8698;
	u32 rsv8699;
	u32 rsv8700;
	u32 rsv8701;
	u32 rsv8702;
	u32 rsv8703;
	u32 CpuMacStatsRam0[4];	/* 0x00008800 */
	u32 CpuMacStatsRam1[4];	/* 0x00008810 */
	u32 CpuMacStatsRam2[4];	/* 0x00008820 */
	u32 CpuMacStatsRam3[4];	/* 0x00008830 */
	u32 CpuMacStatsRam4[4];	/* 0x00008840 */
	u32 CpuMacStatsRam5[4];	/* 0x00008850 */
	u32 CpuMacStatsRam6[4];	/* 0x00008860 */
	u32 CpuMacStatsRam7[4];	/* 0x00008870 */
	u32 CpuMacStatsRam8[4];	/* 0x00008880 */
	u32 CpuMacStatsRam9[4];	/* 0x00008890 */
	u32 CpuMacStatsRam10[4];	/* 0x000088a0 */
	u32 CpuMacStatsRam11[4];	/* 0x000088b0 */
	u32 CpuMacStatsRam12[4];	/* 0x000088c0 */
	u32 CpuMacStatsRam13[4];	/* 0x000088d0 */
	u32 CpuMacStatsRam14[4];	/* 0x000088e0 */
	u32 CpuMacStatsRam15[4];	/* 0x000088f0 */
	u32 CpuMacStatsRam16[4];	/* 0x00008900 */
	u32 CpuMacStatsRam17[4];	/* 0x00008910 */
	u32 CpuMacStatsRam18[4];	/* 0x00008920 */
	u32 CpuMacStatsRam19[4];	/* 0x00008930 */
	u32 CpuMacStatsRam20[4];	/* 0x00008940 */
	u32 CpuMacStatsRam21[4];	/* 0x00008950 */
	u32 CpuMacStatsRam22[4];	/* 0x00008960 */
	u32 CpuMacStatsRam23[4];	/* 0x00008970 */
	u32 CpuMacStatsRam24[4];	/* 0x00008980 */
	u32 CpuMacStatsRam25[4];	/* 0x00008990 */
	u32 CpuMacStatsRam26[4];	/* 0x000089a0 */
	u32 CpuMacStatsRam27[4];	/* 0x000089b0 */
	u32 CpuMacStatsRam28[4];	/* 0x000089c0 */
	u32 CpuMacStatsRam29[4];	/* 0x000089d0 */
	u32 CpuMacStatsRam30[4];	/* 0x000089e0 */
	u32 CpuMacStatsRam31[4];	/* 0x000089f0 */
	u32 CpuMacStatsRam32[4];	/* 0x00008a00 */
	u32 CpuMacStatsRam33[4];	/* 0x00008a10 */
	u32 CpuMacStatsRam34[4];	/* 0x00008a20 */
	u32 CpuMacStatsRam35[4];	/* 0x00008a30 */
	u32 CpuMacStatsRam36[4];	/* 0x00008a40 */
	u32 CpuMacStatsRam37[4];	/* 0x00008a50 */
	u32 CpuMacStatsRam38[4];	/* 0x00008a60 */
	u32 CpuMacStatsRam39[4];	/* 0x00008a70 */
	u32 rsv8864;
	u32 rsv8865;
	u32 rsv8866;
	u32 rsv8867;
	u32 rsv8868;
	u32 rsv8869;
	u32 rsv8870;
	u32 rsv8871;
	u32 rsv8872;
	u32 rsv8873;
	u32 rsv8874;
	u32 rsv8875;
	u32 rsv8876;
	u32 rsv8877;
	u32 rsv8878;
	u32 rsv8879;
	u32 rsv8880;
	u32 rsv8881;
	u32 rsv8882;
	u32 rsv8883;
	u32 rsv8884;
	u32 rsv8885;
	u32 rsv8886;
	u32 rsv8887;
	u32 rsv8888;
	u32 rsv8889;
	u32 rsv8890;
	u32 rsv8891;
	u32 rsv8892;
	u32 rsv8893;
	u32 rsv8894;
	u32 rsv8895;
	u32 rsv8896;
	u32 rsv8897;
	u32 rsv8898;
	u32 rsv8899;
	u32 rsv8900;
	u32 rsv8901;
	u32 rsv8902;
	u32 rsv8903;
	u32 rsv8904;
	u32 rsv8905;
	u32 rsv8906;
	u32 rsv8907;
	u32 rsv8908;
	u32 rsv8909;
	u32 rsv8910;
	u32 rsv8911;
	u32 rsv8912;
	u32 rsv8913;
	u32 rsv8914;
	u32 rsv8915;
	u32 rsv8916;
	u32 rsv8917;
	u32 rsv8918;
	u32 rsv8919;
	u32 rsv8920;
	u32 rsv8921;
	u32 rsv8922;
	u32 rsv8923;
	u32 rsv8924;
	u32 rsv8925;
	u32 rsv8926;
	u32 rsv8927;
	u32 rsv8928;
	u32 rsv8929;
	u32 rsv8930;
	u32 rsv8931;
	u32 rsv8932;
	u32 rsv8933;
	u32 rsv8934;
	u32 rsv8935;
	u32 rsv8936;
	u32 rsv8937;
	u32 rsv8938;
	u32 rsv8939;
	u32 rsv8940;
	u32 rsv8941;
	u32 rsv8942;
	u32 rsv8943;
	u32 rsv8944;
	u32 rsv8945;
	u32 rsv8946;
	u32 rsv8947;
	u32 rsv8948;
	u32 rsv8949;
	u32 rsv8950;
	u32 rsv8951;
	u32 rsv8952;
	u32 rsv8953;
	u32 rsv8954;
	u32 rsv8955;
	u32 rsv8956;
	u32 rsv8957;
	u32 rsv8958;
	u32 rsv8959;
	u32 TxDescCfgFifo0[2];	/* 0x00008c00 */
	u32 TxDescCfgFifo1[2];	/* 0x00008c08 */
	u32 TxDescCfgFifo2[2];	/* 0x00008c10 */
	u32 TxDescCfgFifo3[2];	/* 0x00008c18 */
	u32 TxDescCfgFifo4[2];	/* 0x00008c20 */
	u32 TxDescCfgFifo5[2];	/* 0x00008c28 */
	u32 TxDescCfgFifo6[2];	/* 0x00008c30 */
	u32 TxDescCfgFifo7[2];	/* 0x00008c38 */
	u32 TxDescCfgFifo8[2];	/* 0x00008c40 */
	u32 TxDescCfgFifo9[2];	/* 0x00008c48 */
	u32 TxDescCfgFifo10[2];	/* 0x00008c50 */
	u32 TxDescCfgFifo11[2];	/* 0x00008c58 */
	u32 TxDescCfgFifo12[2];	/* 0x00008c60 */
	u32 TxDescCfgFifo13[2];	/* 0x00008c68 */
	u32 TxDescCfgFifo14[2];	/* 0x00008c70 */
	u32 TxDescCfgFifo15[2];	/* 0x00008c78 */
	u32 TxDescCfgFifo16[2];	/* 0x00008c80 */
	u32 TxDescCfgFifo17[2];	/* 0x00008c88 */
	u32 TxDescCfgFifo18[2];	/* 0x00008c90 */
	u32 TxDescCfgFifo19[2];	/* 0x00008c98 */
	u32 TxDescCfgFifo20[2];	/* 0x00008ca0 */
	u32 TxDescCfgFifo21[2];	/* 0x00008ca8 */
	u32 TxDescCfgFifo22[2];	/* 0x00008cb0 */
	u32 TxDescCfgFifo23[2];	/* 0x00008cb8 */
	u32 TxDescCfgFifo24[2];	/* 0x00008cc0 */
	u32 TxDescCfgFifo25[2];	/* 0x00008cc8 */
	u32 TxDescCfgFifo26[2];	/* 0x00008cd0 */
	u32 TxDescCfgFifo27[2];	/* 0x00008cd8 */
	u32 TxDescCfgFifo28[2];	/* 0x00008ce0 */
	u32 TxDescCfgFifo29[2];	/* 0x00008ce8 */
	u32 TxDescCfgFifo30[2];	/* 0x00008cf0 */
	u32 TxDescCfgFifo31[2];	/* 0x00008cf8 */
	u32 TxDescCfgFifo32[2];	/* 0x00008d00 */
	u32 TxDescCfgFifo33[2];	/* 0x00008d08 */
	u32 TxDescCfgFifo34[2];	/* 0x00008d10 */
	u32 TxDescCfgFifo35[2];	/* 0x00008d18 */
	u32 TxDescCfgFifo36[2];	/* 0x00008d20 */
	u32 TxDescCfgFifo37[2];	/* 0x00008d28 */
	u32 TxDescCfgFifo38[2];	/* 0x00008d30 */
	u32 TxDescCfgFifo39[2];	/* 0x00008d38 */
	u32 TxDescCfgFifo40[2];	/* 0x00008d40 */
	u32 TxDescCfgFifo41[2];	/* 0x00008d48 */
	u32 TxDescCfgFifo42[2];	/* 0x00008d50 */
	u32 TxDescCfgFifo43[2];	/* 0x00008d58 */
	u32 TxDescCfgFifo44[2];	/* 0x00008d60 */
	u32 TxDescCfgFifo45[2];	/* 0x00008d68 */
	u32 TxDescCfgFifo46[2];	/* 0x00008d70 */
	u32 TxDescCfgFifo47[2];	/* 0x00008d78 */
	u32 TxDescCfgFifo48[2];	/* 0x00008d80 */
	u32 TxDescCfgFifo49[2];	/* 0x00008d88 */
	u32 TxDescCfgFifo50[2];	/* 0x00008d90 */
	u32 TxDescCfgFifo51[2];	/* 0x00008d98 */
	u32 TxDescCfgFifo52[2];	/* 0x00008da0 */
	u32 TxDescCfgFifo53[2];	/* 0x00008da8 */
	u32 TxDescCfgFifo54[2];	/* 0x00008db0 */
	u32 TxDescCfgFifo55[2];	/* 0x00008db8 */
	u32 TxDescCfgFifo56[2];	/* 0x00008dc0 */
	u32 TxDescCfgFifo57[2];	/* 0x00008dc8 */
	u32 TxDescCfgFifo58[2];	/* 0x00008dd0 */
	u32 TxDescCfgFifo59[2];	/* 0x00008dd8 */
	u32 TxDescCfgFifo60[2];	/* 0x00008de0 */
	u32 TxDescCfgFifo61[2];	/* 0x00008de8 */
	u32 TxDescCfgFifo62[2];	/* 0x00008df0 */
	u32 TxDescCfgFifo63[2];	/* 0x00008df8 */
	u32 RxDesc0CfgFifo0[2];	/* 0x00008e00 */
	u32 RxDesc0CfgFifo1[2];	/* 0x00008e08 */
	u32 RxDesc0CfgFifo2[2];	/* 0x00008e10 */
	u32 RxDesc0CfgFifo3[2];	/* 0x00008e18 */
	u32 RxDesc0CfgFifo4[2];	/* 0x00008e20 */
	u32 RxDesc0CfgFifo5[2];	/* 0x00008e28 */
	u32 RxDesc0CfgFifo6[2];	/* 0x00008e30 */
	u32 RxDesc0CfgFifo7[2];	/* 0x00008e38 */
	u32 RxDesc0CfgFifo8[2];	/* 0x00008e40 */
	u32 RxDesc0CfgFifo9[2];	/* 0x00008e48 */
	u32 RxDesc0CfgFifo10[2];	/* 0x00008e50 */
	u32 RxDesc0CfgFifo11[2];	/* 0x00008e58 */
	u32 RxDesc0CfgFifo12[2];	/* 0x00008e60 */
	u32 RxDesc0CfgFifo13[2];	/* 0x00008e68 */
	u32 RxDesc0CfgFifo14[2];	/* 0x00008e70 */
	u32 RxDesc0CfgFifo15[2];	/* 0x00008e78 */
	u32 RxDesc0CfgFifo16[2];	/* 0x00008e80 */
	u32 RxDesc0CfgFifo17[2];	/* 0x00008e88 */
	u32 RxDesc0CfgFifo18[2];	/* 0x00008e90 */
	u32 RxDesc0CfgFifo19[2];	/* 0x00008e98 */
	u32 RxDesc0CfgFifo20[2];	/* 0x00008ea0 */
	u32 RxDesc0CfgFifo21[2];	/* 0x00008ea8 */
	u32 RxDesc0CfgFifo22[2];	/* 0x00008eb0 */
	u32 RxDesc0CfgFifo23[2];	/* 0x00008eb8 */
	u32 RxDesc0CfgFifo24[2];	/* 0x00008ec0 */
	u32 RxDesc0CfgFifo25[2];	/* 0x00008ec8 */
	u32 RxDesc0CfgFifo26[2];	/* 0x00008ed0 */
	u32 RxDesc0CfgFifo27[2];	/* 0x00008ed8 */
	u32 RxDesc0CfgFifo28[2];	/* 0x00008ee0 */
	u32 RxDesc0CfgFifo29[2];	/* 0x00008ee8 */
	u32 RxDesc0CfgFifo30[2];	/* 0x00008ef0 */
	u32 RxDesc0CfgFifo31[2];	/* 0x00008ef8 */
	u32 RxDesc0CfgFifo32[2];	/* 0x00008f00 */
	u32 RxDesc0CfgFifo33[2];	/* 0x00008f08 */
	u32 RxDesc0CfgFifo34[2];	/* 0x00008f10 */
	u32 RxDesc0CfgFifo35[2];	/* 0x00008f18 */
	u32 RxDesc0CfgFifo36[2];	/* 0x00008f20 */
	u32 RxDesc0CfgFifo37[2];	/* 0x00008f28 */
	u32 RxDesc0CfgFifo38[2];	/* 0x00008f30 */
	u32 RxDesc0CfgFifo39[2];	/* 0x00008f38 */
	u32 RxDesc0CfgFifo40[2];	/* 0x00008f40 */
	u32 RxDesc0CfgFifo41[2];	/* 0x00008f48 */
	u32 RxDesc0CfgFifo42[2];	/* 0x00008f50 */
	u32 RxDesc0CfgFifo43[2];	/* 0x00008f58 */
	u32 RxDesc0CfgFifo44[2];	/* 0x00008f60 */
	u32 RxDesc0CfgFifo45[2];	/* 0x00008f68 */
	u32 RxDesc0CfgFifo46[2];	/* 0x00008f70 */
	u32 RxDesc0CfgFifo47[2];	/* 0x00008f78 */
	u32 RxDesc0CfgFifo48[2];	/* 0x00008f80 */
	u32 RxDesc0CfgFifo49[2];	/* 0x00008f88 */
	u32 RxDesc0CfgFifo50[2];	/* 0x00008f90 */
	u32 RxDesc0CfgFifo51[2];	/* 0x00008f98 */
	u32 RxDesc0CfgFifo52[2];	/* 0x00008fa0 */
	u32 RxDesc0CfgFifo53[2];	/* 0x00008fa8 */
	u32 RxDesc0CfgFifo54[2];	/* 0x00008fb0 */
	u32 RxDesc0CfgFifo55[2];	/* 0x00008fb8 */
	u32 RxDesc0CfgFifo56[2];	/* 0x00008fc0 */
	u32 RxDesc0CfgFifo57[2];	/* 0x00008fc8 */
	u32 RxDesc0CfgFifo58[2];	/* 0x00008fd0 */
	u32 RxDesc0CfgFifo59[2];	/* 0x00008fd8 */
	u32 RxDesc0CfgFifo60[2];	/* 0x00008fe0 */
	u32 RxDesc0CfgFifo61[2];	/* 0x00008fe8 */
	u32 RxDesc0CfgFifo62[2];	/* 0x00008ff0 */
	u32 RxDesc0CfgFifo63[2];	/* 0x00008ff8 */
	u32 RxDesc1CfgFifo0[2];	/* 0x00009000 */
	u32 RxDesc1CfgFifo1[2];	/* 0x00009008 */
	u32 RxDesc1CfgFifo2[2];	/* 0x00009010 */
	u32 RxDesc1CfgFifo3[2];	/* 0x00009018 */
	u32 RxDesc1CfgFifo4[2];	/* 0x00009020 */
	u32 RxDesc1CfgFifo5[2];	/* 0x00009028 */
	u32 RxDesc1CfgFifo6[2];	/* 0x00009030 */
	u32 RxDesc1CfgFifo7[2];	/* 0x00009038 */
	u32 RxDesc1CfgFifo8[2];	/* 0x00009040 */
	u32 RxDesc1CfgFifo9[2];	/* 0x00009048 */
	u32 RxDesc1CfgFifo10[2];	/* 0x00009050 */
	u32 RxDesc1CfgFifo11[2];	/* 0x00009058 */
	u32 RxDesc1CfgFifo12[2];	/* 0x00009060 */
	u32 RxDesc1CfgFifo13[2];	/* 0x00009068 */
	u32 RxDesc1CfgFifo14[2];	/* 0x00009070 */
	u32 RxDesc1CfgFifo15[2];	/* 0x00009078 */
	u32 RxDesc1CfgFifo16[2];	/* 0x00009080 */
	u32 RxDesc1CfgFifo17[2];	/* 0x00009088 */
	u32 RxDesc1CfgFifo18[2];	/* 0x00009090 */
	u32 RxDesc1CfgFifo19[2];	/* 0x00009098 */
	u32 RxDesc1CfgFifo20[2];	/* 0x000090a0 */
	u32 RxDesc1CfgFifo21[2];	/* 0x000090a8 */
	u32 RxDesc1CfgFifo22[2];	/* 0x000090b0 */
	u32 RxDesc1CfgFifo23[2];	/* 0x000090b8 */
	u32 RxDesc1CfgFifo24[2];	/* 0x000090c0 */
	u32 RxDesc1CfgFifo25[2];	/* 0x000090c8 */
	u32 RxDesc1CfgFifo26[2];	/* 0x000090d0 */
	u32 RxDesc1CfgFifo27[2];	/* 0x000090d8 */
	u32 RxDesc1CfgFifo28[2];	/* 0x000090e0 */
	u32 RxDesc1CfgFifo29[2];	/* 0x000090e8 */
	u32 RxDesc1CfgFifo30[2];	/* 0x000090f0 */
	u32 RxDesc1CfgFifo31[2];	/* 0x000090f8 */
	u32 RxDesc1CfgFifo32[2];	/* 0x00009100 */
	u32 RxDesc1CfgFifo33[2];	/* 0x00009108 */
	u32 RxDesc1CfgFifo34[2];	/* 0x00009110 */
	u32 RxDesc1CfgFifo35[2];	/* 0x00009118 */
	u32 RxDesc1CfgFifo36[2];	/* 0x00009120 */
	u32 RxDesc1CfgFifo37[2];	/* 0x00009128 */
	u32 RxDesc1CfgFifo38[2];	/* 0x00009130 */
	u32 RxDesc1CfgFifo39[2];	/* 0x00009138 */
	u32 RxDesc1CfgFifo40[2];	/* 0x00009140 */
	u32 RxDesc1CfgFifo41[2];	/* 0x00009148 */
	u32 RxDesc1CfgFifo42[2];	/* 0x00009150 */
	u32 RxDesc1CfgFifo43[2];	/* 0x00009158 */
	u32 RxDesc1CfgFifo44[2];	/* 0x00009160 */
	u32 RxDesc1CfgFifo45[2];	/* 0x00009168 */
	u32 RxDesc1CfgFifo46[2];	/* 0x00009170 */
	u32 RxDesc1CfgFifo47[2];	/* 0x00009178 */
	u32 RxDesc1CfgFifo48[2];	/* 0x00009180 */
	u32 RxDesc1CfgFifo49[2];	/* 0x00009188 */
	u32 RxDesc1CfgFifo50[2];	/* 0x00009190 */
	u32 RxDesc1CfgFifo51[2];	/* 0x00009198 */
	u32 RxDesc1CfgFifo52[2];	/* 0x000091a0 */
	u32 RxDesc1CfgFifo53[2];	/* 0x000091a8 */
	u32 RxDesc1CfgFifo54[2];	/* 0x000091b0 */
	u32 RxDesc1CfgFifo55[2];	/* 0x000091b8 */
	u32 RxDesc1CfgFifo56[2];	/* 0x000091c0 */
	u32 RxDesc1CfgFifo57[2];	/* 0x000091c8 */
	u32 RxDesc1CfgFifo58[2];	/* 0x000091d0 */
	u32 RxDesc1CfgFifo59[2];	/* 0x000091d8 */
	u32 RxDesc1CfgFifo60[2];	/* 0x000091e0 */
	u32 RxDesc1CfgFifo61[2];	/* 0x000091e8 */
	u32 RxDesc1CfgFifo62[2];	/* 0x000091f0 */
	u32 RxDesc1CfgFifo63[2];	/* 0x000091f8 */
	u32 RxDesc0AckFifo0[1];	/* 0x00009200 */
	u32 RxDesc0AckFifo1[1];	/* 0x00009204 */
	u32 RxDesc0AckFifo2[1];	/* 0x00009208 */
	u32 RxDesc0AckFifo3[1];	/* 0x0000920c */
	u32 RxDesc0AckFifo4[1];	/* 0x00009210 */
	u32 RxDesc0AckFifo5[1];	/* 0x00009214 */
	u32 RxDesc0AckFifo6[1];	/* 0x00009218 */
	u32 RxDesc0AckFifo7[1];	/* 0x0000921c */
	u32 RxDesc0AckFifo8[1];	/* 0x00009220 */
	u32 RxDesc0AckFifo9[1];	/* 0x00009224 */
	u32 RxDesc0AckFifo10[1];	/* 0x00009228 */
	u32 RxDesc0AckFifo11[1];	/* 0x0000922c */
	u32 RxDesc0AckFifo12[1];	/* 0x00009230 */
	u32 RxDesc0AckFifo13[1];	/* 0x00009234 */
	u32 RxDesc0AckFifo14[1];	/* 0x00009238 */
	u32 RxDesc0AckFifo15[1];	/* 0x0000923c */
	u32 RxDesc0AckFifo16[1];	/* 0x00009240 */
	u32 RxDesc0AckFifo17[1];	/* 0x00009244 */
	u32 RxDesc0AckFifo18[1];	/* 0x00009248 */
	u32 RxDesc0AckFifo19[1];	/* 0x0000924c */
	u32 RxDesc0AckFifo20[1];	/* 0x00009250 */
	u32 RxDesc0AckFifo21[1];	/* 0x00009254 */
	u32 RxDesc0AckFifo22[1];	/* 0x00009258 */
	u32 RxDesc0AckFifo23[1];	/* 0x0000925c */
	u32 RxDesc0AckFifo24[1];	/* 0x00009260 */
	u32 RxDesc0AckFifo25[1];	/* 0x00009264 */
	u32 RxDesc0AckFifo26[1];	/* 0x00009268 */
	u32 RxDesc0AckFifo27[1];	/* 0x0000926c */
	u32 RxDesc0AckFifo28[1];	/* 0x00009270 */
	u32 RxDesc0AckFifo29[1];	/* 0x00009274 */
	u32 RxDesc0AckFifo30[1];	/* 0x00009278 */
	u32 RxDesc0AckFifo31[1];	/* 0x0000927c */
	u32 RxDesc0AckFifo32[1];	/* 0x00009280 */
	u32 RxDesc0AckFifo33[1];	/* 0x00009284 */
	u32 RxDesc0AckFifo34[1];	/* 0x00009288 */
	u32 RxDesc0AckFifo35[1];	/* 0x0000928c */
	u32 RxDesc0AckFifo36[1];	/* 0x00009290 */
	u32 RxDesc0AckFifo37[1];	/* 0x00009294 */
	u32 RxDesc0AckFifo38[1];	/* 0x00009298 */
	u32 RxDesc0AckFifo39[1];	/* 0x0000929c */
	u32 RxDesc0AckFifo40[1];	/* 0x000092a0 */
	u32 RxDesc0AckFifo41[1];	/* 0x000092a4 */
	u32 RxDesc0AckFifo42[1];	/* 0x000092a8 */
	u32 RxDesc0AckFifo43[1];	/* 0x000092ac */
	u32 RxDesc0AckFifo44[1];	/* 0x000092b0 */
	u32 RxDesc0AckFifo45[1];	/* 0x000092b4 */
	u32 RxDesc0AckFifo46[1];	/* 0x000092b8 */
	u32 RxDesc0AckFifo47[1];	/* 0x000092bc */
	u32 RxDesc0AckFifo48[1];	/* 0x000092c0 */
	u32 RxDesc0AckFifo49[1];	/* 0x000092c4 */
	u32 RxDesc0AckFifo50[1];	/* 0x000092c8 */
	u32 RxDesc0AckFifo51[1];	/* 0x000092cc */
	u32 RxDesc0AckFifo52[1];	/* 0x000092d0 */
	u32 RxDesc0AckFifo53[1];	/* 0x000092d4 */
	u32 RxDesc0AckFifo54[1];	/* 0x000092d8 */
	u32 RxDesc0AckFifo55[1];	/* 0x000092dc */
	u32 RxDesc0AckFifo56[1];	/* 0x000092e0 */
	u32 RxDesc0AckFifo57[1];	/* 0x000092e4 */
	u32 RxDesc0AckFifo58[1];	/* 0x000092e8 */
	u32 RxDesc0AckFifo59[1];	/* 0x000092ec */
	u32 RxDesc0AckFifo60[1];	/* 0x000092f0 */
	u32 RxDesc0AckFifo61[1];	/* 0x000092f4 */
	u32 RxDesc0AckFifo62[1];	/* 0x000092f8 */
	u32 RxDesc0AckFifo63[1];	/* 0x000092fc */
	u32 RxDesc1AckFifo0[1];	/* 0x00009300 */
	u32 RxDesc1AckFifo1[1];	/* 0x00009304 */
	u32 RxDesc1AckFifo2[1];	/* 0x00009308 */
	u32 RxDesc1AckFifo3[1];	/* 0x0000930c */
	u32 RxDesc1AckFifo4[1];	/* 0x00009310 */
	u32 RxDesc1AckFifo5[1];	/* 0x00009314 */
	u32 RxDesc1AckFifo6[1];	/* 0x00009318 */
	u32 RxDesc1AckFifo7[1];	/* 0x0000931c */
	u32 RxDesc1AckFifo8[1];	/* 0x00009320 */
	u32 RxDesc1AckFifo9[1];	/* 0x00009324 */
	u32 RxDesc1AckFifo10[1];	/* 0x00009328 */
	u32 RxDesc1AckFifo11[1];	/* 0x0000932c */
	u32 RxDesc1AckFifo12[1];	/* 0x00009330 */
	u32 RxDesc1AckFifo13[1];	/* 0x00009334 */
	u32 RxDesc1AckFifo14[1];	/* 0x00009338 */
	u32 RxDesc1AckFifo15[1];	/* 0x0000933c */
	u32 RxDesc1AckFifo16[1];	/* 0x00009340 */
	u32 RxDesc1AckFifo17[1];	/* 0x00009344 */
	u32 RxDesc1AckFifo18[1];	/* 0x00009348 */
	u32 RxDesc1AckFifo19[1];	/* 0x0000934c */
	u32 RxDesc1AckFifo20[1];	/* 0x00009350 */
	u32 RxDesc1AckFifo21[1];	/* 0x00009354 */
	u32 RxDesc1AckFifo22[1];	/* 0x00009358 */
	u32 RxDesc1AckFifo23[1];	/* 0x0000935c */
	u32 RxDesc1AckFifo24[1];	/* 0x00009360 */
	u32 RxDesc1AckFifo25[1];	/* 0x00009364 */
	u32 RxDesc1AckFifo26[1];	/* 0x00009368 */
	u32 RxDesc1AckFifo27[1];	/* 0x0000936c */
	u32 RxDesc1AckFifo28[1];	/* 0x00009370 */
	u32 RxDesc1AckFifo29[1];	/* 0x00009374 */
	u32 RxDesc1AckFifo30[1];	/* 0x00009378 */
	u32 RxDesc1AckFifo31[1];	/* 0x0000937c */
	u32 RxDesc1AckFifo32[1];	/* 0x00009380 */
	u32 RxDesc1AckFifo33[1];	/* 0x00009384 */
	u32 RxDesc1AckFifo34[1];	/* 0x00009388 */
	u32 RxDesc1AckFifo35[1];	/* 0x0000938c */
	u32 RxDesc1AckFifo36[1];	/* 0x00009390 */
	u32 RxDesc1AckFifo37[1];	/* 0x00009394 */
	u32 RxDesc1AckFifo38[1];	/* 0x00009398 */
	u32 RxDesc1AckFifo39[1];	/* 0x0000939c */
	u32 RxDesc1AckFifo40[1];	/* 0x000093a0 */
	u32 RxDesc1AckFifo41[1];	/* 0x000093a4 */
	u32 RxDesc1AckFifo42[1];	/* 0x000093a8 */
	u32 RxDesc1AckFifo43[1];	/* 0x000093ac */
	u32 RxDesc1AckFifo44[1];	/* 0x000093b0 */
	u32 RxDesc1AckFifo45[1];	/* 0x000093b4 */
	u32 RxDesc1AckFifo46[1];	/* 0x000093b8 */
	u32 RxDesc1AckFifo47[1];	/* 0x000093bc */
	u32 RxDesc1AckFifo48[1];	/* 0x000093c0 */
	u32 RxDesc1AckFifo49[1];	/* 0x000093c4 */
	u32 RxDesc1AckFifo50[1];	/* 0x000093c8 */
	u32 RxDesc1AckFifo51[1];	/* 0x000093cc */
	u32 RxDesc1AckFifo52[1];	/* 0x000093d0 */
	u32 RxDesc1AckFifo53[1];	/* 0x000093d4 */
	u32 RxDesc1AckFifo54[1];	/* 0x000093d8 */
	u32 RxDesc1AckFifo55[1];	/* 0x000093dc */
	u32 RxDesc1AckFifo56[1];	/* 0x000093e0 */
	u32 RxDesc1AckFifo57[1];	/* 0x000093e4 */
	u32 RxDesc1AckFifo58[1];	/* 0x000093e8 */
	u32 RxDesc1AckFifo59[1];	/* 0x000093ec */
	u32 RxDesc1AckFifo60[1];	/* 0x000093f0 */
	u32 RxDesc1AckFifo61[1];	/* 0x000093f4 */
	u32 RxDesc1AckFifo62[1];	/* 0x000093f8 */
	u32 RxDesc1AckFifo63[1];	/* 0x000093fc */
	u32 TxDescAckFifo0[1];	/* 0x00009400 */
	u32 TxDescAckFifo1[1];	/* 0x00009404 */
	u32 TxDescAckFifo2[1];	/* 0x00009408 */
	u32 TxDescAckFifo3[1];	/* 0x0000940c */
	u32 TxDescAckFifo4[1];	/* 0x00009410 */
	u32 TxDescAckFifo5[1];	/* 0x00009414 */
	u32 TxDescAckFifo6[1];	/* 0x00009418 */
	u32 TxDescAckFifo7[1];	/* 0x0000941c */
	u32 TxDescAckFifo8[1];	/* 0x00009420 */
	u32 TxDescAckFifo9[1];	/* 0x00009424 */
	u32 TxDescAckFifo10[1];	/* 0x00009428 */
	u32 TxDescAckFifo11[1];	/* 0x0000942c */
	u32 TxDescAckFifo12[1];	/* 0x00009430 */
	u32 TxDescAckFifo13[1];	/* 0x00009434 */
	u32 TxDescAckFifo14[1];	/* 0x00009438 */
	u32 TxDescAckFifo15[1];	/* 0x0000943c */
	u32 TxDescAckFifo16[1];	/* 0x00009440 */
	u32 TxDescAckFifo17[1];	/* 0x00009444 */
	u32 TxDescAckFifo18[1];	/* 0x00009448 */
	u32 TxDescAckFifo19[1];	/* 0x0000944c */
	u32 TxDescAckFifo20[1];	/* 0x00009450 */
	u32 TxDescAckFifo21[1];	/* 0x00009454 */
	u32 TxDescAckFifo22[1];	/* 0x00009458 */
	u32 TxDescAckFifo23[1];	/* 0x0000945c */
	u32 TxDescAckFifo24[1];	/* 0x00009460 */
	u32 TxDescAckFifo25[1];	/* 0x00009464 */
	u32 TxDescAckFifo26[1];	/* 0x00009468 */
	u32 TxDescAckFifo27[1];	/* 0x0000946c */
	u32 TxDescAckFifo28[1];	/* 0x00009470 */
	u32 TxDescAckFifo29[1];	/* 0x00009474 */
	u32 TxDescAckFifo30[1];	/* 0x00009478 */
	u32 TxDescAckFifo31[1];	/* 0x0000947c */
	u32 TxDescAckFifo32[1];	/* 0x00009480 */
	u32 TxDescAckFifo33[1];	/* 0x00009484 */
	u32 TxDescAckFifo34[1];	/* 0x00009488 */
	u32 TxDescAckFifo35[1];	/* 0x0000948c */
	u32 TxDescAckFifo36[1];	/* 0x00009490 */
	u32 TxDescAckFifo37[1];	/* 0x00009494 */
	u32 TxDescAckFifo38[1];	/* 0x00009498 */
	u32 TxDescAckFifo39[1];	/* 0x0000949c */
	u32 TxDescAckFifo40[1];	/* 0x000094a0 */
	u32 TxDescAckFifo41[1];	/* 0x000094a4 */
	u32 TxDescAckFifo42[1];	/* 0x000094a8 */
	u32 TxDescAckFifo43[1];	/* 0x000094ac */
	u32 TxDescAckFifo44[1];	/* 0x000094b0 */
	u32 TxDescAckFifo45[1];	/* 0x000094b4 */
	u32 TxDescAckFifo46[1];	/* 0x000094b8 */
	u32 TxDescAckFifo47[1];	/* 0x000094bc */
	u32 TxDescAckFifo48[1];	/* 0x000094c0 */
	u32 TxDescAckFifo49[1];	/* 0x000094c4 */
	u32 TxDescAckFifo50[1];	/* 0x000094c8 */
	u32 TxDescAckFifo51[1];	/* 0x000094cc */
	u32 TxDescAckFifo52[1];	/* 0x000094d0 */
	u32 TxDescAckFifo53[1];	/* 0x000094d4 */
	u32 TxDescAckFifo54[1];	/* 0x000094d8 */
	u32 TxDescAckFifo55[1];	/* 0x000094dc */
	u32 TxDescAckFifo56[1];	/* 0x000094e0 */
	u32 TxDescAckFifo57[1];	/* 0x000094e4 */
	u32 TxDescAckFifo58[1];	/* 0x000094e8 */
	u32 TxDescAckFifo59[1];	/* 0x000094ec */
	u32 TxDescAckFifo60[1];	/* 0x000094f0 */
	u32 TxDescAckFifo61[1];	/* 0x000094f4 */
	u32 TxDescAckFifo62[1];	/* 0x000094f8 */
	u32 TxDescAckFifo63[1];	/* 0x000094fc */
	u32 CpuMacDescIntf0[2];	/* 0x00009500 */
	u32 CpuMacDescIntf1[2];	/* 0x00009508 */
	u32 CpuMacDescIntf2[2];	/* 0x00009510 */
};

/* ################################################################################
 * # TxPktFifo Definition */
#define TX_PKT_FIFO_W0_TX_PKT_FIFO_FIELD0_BIT                        0
#define TX_PKT_FIFO_W1_TX_PKT_FIFO_FIELD1_BIT                        0
#define TX_PKT_FIFO_W2_TX_PKT_FIFO_FIELD2_BIT                        0

#define TX_PKT_FIFO_W0_TX_PKT_FIFO_FIELD0_MASK                       0xffffffff
#define TX_PKT_FIFO_W1_TX_PKT_FIFO_FIELD1_MASK                       0xffffffff
#define TX_PKT_FIFO_W2_TX_PKT_FIFO_FIELD2_MASK                       0x0000000f

/* ################################################################################
 * # RxPktMsgFifo Definition */
#define RX_PKT_MSG_FIFO_W0_RX_PKT_MSG_FIFO_FIELD_BIT                 0

#define RX_PKT_MSG_FIFO_W0_RX_PKT_MSG_FIFO_FIELD_MASK                0x0000ffff

/* ################################################################################
 * # CpuMacStatsRam Definition */
#define CPU_MAC_STATS_RAM_W0_BYTE_CNT_31_0_BIT                       0
#define CPU_MAC_STATS_RAM_W1_BYTE_CNT_39_32_BIT                      0
#define CPU_MAC_STATS_RAM_W2_FRAME_CNT_31_0_BIT                      0
#define CPU_MAC_STATS_RAM_W3_FRAME_CNT_33_32_BIT                     0

#define CPU_MAC_STATS_RAM_W0_BYTE_CNT_31_0_MASK                      0xffffffff
#define CPU_MAC_STATS_RAM_W1_BYTE_CNT_39_32_MASK                     0x000000ff
#define CPU_MAC_STATS_RAM_W2_FRAME_CNT_31_0_MASK                     0xffffffff
#define CPU_MAC_STATS_RAM_W3_FRAME_CNT_33_32_MASK                    0x00000003

/* ################################################################################
 * # TxDescCfgFifo Definition */
#define TX_DESC_CFG_FIFO_W0_TX_DESC_CFG_FIFO_FIELD0_BIT              0
#define TX_DESC_CFG_FIFO_W1_TX_DESC_CFG_FIFO_FIELD1_BIT              0

#define TX_DESC_CFG_FIFO_W0_TX_DESC_CFG_FIFO_FIELD0_MASK             0xffffffff
#define TX_DESC_CFG_FIFO_W1_TX_DESC_CFG_FIFO_FIELD1_MASK             0x01ffffff

/* ################################################################################
 * # RxDesc0CfgFifo Definition */
#define RX_DESC0_CFG_FIFO_W0_RX_DESC0_CFG_FIFO_FIELD0_BIT            0
#define RX_DESC0_CFG_FIFO_W1_RX_DESC0_CFG_FIFO_FIELD1_BIT            0

#define RX_DESC0_CFG_FIFO_W0_RX_DESC0_CFG_FIFO_FIELD0_MASK           0xffffffff
#define RX_DESC0_CFG_FIFO_W1_RX_DESC0_CFG_FIFO_FIELD1_MASK           0x01ffffff

/* ################################################################################
 * # RxDesc1CfgFifo Definition */
#define RX_DESC1_CFG_FIFO_W0_RX_DESC1_CFG_FIFO_FIELD0_BIT            0
#define RX_DESC1_CFG_FIFO_W1_RX_DESC1_CFG_FIFO_FIELD1_BIT            0

#define RX_DESC1_CFG_FIFO_W0_RX_DESC1_CFG_FIFO_FIELD0_MASK           0xffffffff
#define RX_DESC1_CFG_FIFO_W1_RX_DESC1_CFG_FIFO_FIELD1_MASK           0x01ffffff

/* ################################################################################
 * # RxDesc0AckFifo Definition */
#define RX_DESC0_ACK_FIFO_W0_RX_DESC0_ACK_FIFO_FIELD_BIT             0

#define RX_DESC0_ACK_FIFO_W0_RX_DESC0_ACK_FIFO_FIELD_MASK            0x0007ffff

/* ################################################################################
 * # RxDesc1AckFifo Definition */
#define RX_DESC1_ACK_FIFO_W0_RX_DESC1_ACK_FIFO_FIELD_BIT             0

#define RX_DESC1_ACK_FIFO_W0_RX_DESC1_ACK_FIFO_FIELD_MASK            0x0007ffff

/* ################################################################################
 * # TxDescAckFifo Definition */
#define TX_DESC_ACK_FIFO_W0_TX_DESC_ACK_FIFO_FIELD_BIT               0

#define TX_DESC_ACK_FIFO_W0_TX_DESC_ACK_FIFO_FIELD_MASK              0x00000001

/* ################################################################################
 * # CpuMacDescIntf Definition */
#define CPU_MAC_DESC_INTF_W0_DESC_ADDR_31_0_BIT                      0
#define CPU_MAC_DESC_INTF_W1_DESC_ADDR_39_32_BIT                     0
#define CPU_MAC_DESC_INTF_W1_DESC_EOP_BIT                            23
#define CPU_MAC_DESC_INTF_W1_DESC_ERR_BIT                            24
#define CPU_MAC_DESC_INTF_W1_DESC_ERR_TYPE_BIT                       25
#define CPU_MAC_DESC_INTF_W1_DESC_SIZE_BIT                           8
#define CPU_MAC_DESC_INTF_W1_DESC_SOP_BIT                            22

#define CPU_MAC_DESC_INTF_W0_DESC_ADDR_31_0_MASK                     0xffffffff
#define CPU_MAC_DESC_INTF_W1_DESC_ADDR_39_32_MASK                    0x000000ff
#define CPU_MAC_DESC_INTF_W1_DESC_EOP_MASK                           0x00800000
#define CPU_MAC_DESC_INTF_W1_DESC_ERR_MASK                           0x01000000
#define CPU_MAC_DESC_INTF_W1_DESC_ERR_TYPE_MASK                      0x0e000000
#define CPU_MAC_DESC_INTF_W1_DESC_SIZE_MASK                          0x003fff00
#define CPU_MAC_DESC_INTF_W1_DESC_SOP_MASK                           0x00400000

#define CPUMACUNIT_MEM_BASE   0x00000400
#define CPUMACUNIT_REG_BASE   0x00000040

struct CpuMacUnit_regs {
	u32 CpuMacUnitHssMon[7];	/* 0x00000040 */
	u32 rsv23;
	u32 CpuMacHssRegAccTimingCfg[2];	/* 0x00000060 */
	u32 CpuMacUnitResetCtl;	/* 0x00000068 */
	u32 CpuMacUnitHssRegAccCtl;	/* 0x0000006c */
	u32 CpuMacUnitHssRegAccResult;	/* 0x00000070 */
	u32 CpuMacUnitAxiCfg;	/* 0x00000074 */
	u32 CpuMacUnitTsCfg;	/* 0x00000078 */
	u32 CpuMacUnitFifoStatus;	/* 0x0000007c */
	u32 CpuMacUnitTsMon[3];	/* 0x00000080 */
	u32 rsv35;
	u32 CpuMacUnitRefPulseCfg[4];	/* 0x00000090 */
	u32 CpuMacUnitInterruptFunc[4];	/* 0x000000a0 */
	u32 rsv44;
	u32 rsv45;
	u32 rsv46;
	u32 rsv47;
	u32 CpuMacUnitHssCfg[14];	/* 0x000000c0 */
	u32 rsv62;
	u32 rsv63;
	u32 CpuMacUnitIpCamCfg[32];	/* 0x00000100 */
	u32 CpuMacUnitMacCamCfg[32];	/* 0x00000180 */
	u32 CpuMacUnitFilterCfg[6];	/* 0x00000200 */
	u32 rsv134;
	u32 rsv135;
	u32 CpuMacUnitFilterCfg1[4];	/* 0x00000220 */
};

/* ################################################################################
 * # CpuMacUnitHssMon Definition */
#define CPU_MAC_UNIT_HSS_MON_W0_MON_HSS_CMU0_DBG_OBS_BIT             0
#define CPU_MAC_UNIT_HSS_MON_W0_MON_HSS_CMU0_LOL_BIT                 16
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_DBG_OBS_BIT               0
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_DFE_RST_DONE_BIT          16
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_I_SCAN_DONE_BIT           21
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_LOL_BIT                   19
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_LOL_UDL_BIT               20
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA2_PCS_TX_DET_RX_N_BIT  22
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA2_PCS_TX_DET_RX_P_BIT  23
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA_RST_DONE_BIT          17
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_RX_EI_BIT                 18
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_RX_EI_FILTERED_BIT        24
#define CPU_MAC_UNIT_HSS_MON_W2_MON_HSS_L0_I_SCAN_RESULTS_31_0_BIT   0
#define CPU_MAC_UNIT_HSS_MON_W3_MON_HSS_L0_I_SCAN_RESULTS_63_32_BIT  0
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_DBG_OBS_BIT               0
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_DFE_RST_DONE_BIT          16
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_I_SCAN_DONE_BIT           21
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_LOL_BIT                   19
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_LOL_UDL_BIT               20
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA2_PCS_TX_DET_RX_N_BIT  22
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA2_PCS_TX_DET_RX_P_BIT  23
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA_RST_DONE_BIT          17
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_RX_EI_BIT                 18
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_RX_EI_FILTERED_BIT        24
#define CPU_MAC_UNIT_HSS_MON_W5_MON_HSS_L1_I_SCAN_RESULTS_31_0_BIT   0
#define CPU_MAC_UNIT_HSS_MON_W6_MON_HSS_L1_I_SCAN_RESULTS_63_32_BIT  0

#define CPU_MAC_UNIT_HSS_MON_W0_MON_HSS_CMU0_DBG_OBS_MASK            0x0000ffff
#define CPU_MAC_UNIT_HSS_MON_W0_MON_HSS_CMU0_LOL_MASK                0x00010000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_DBG_OBS_MASK              0x0000ffff
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_DFE_RST_DONE_MASK         0x00010000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_I_SCAN_DONE_MASK          0x00200000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_LOL_MASK                  0x00080000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_LOL_UDL_MASK              0x00100000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA2_PCS_TX_DET_RX_N_MASK 0x00400000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA2_PCS_TX_DET_RX_P_MASK 0x00800000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_PMA_RST_DONE_MASK         0x00020000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_RX_EI_MASK                0x00040000
#define CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_RX_EI_FILTERED_MASK       0x01000000
#define CPU_MAC_UNIT_HSS_MON_W2_MON_HSS_L0_I_SCAN_RESULTS_31_0_MASK  0xffffffff
#define CPU_MAC_UNIT_HSS_MON_W3_MON_HSS_L0_I_SCAN_RESULTS_63_32_MASK 0xffffffff
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_DBG_OBS_MASK              0x0000ffff
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_DFE_RST_DONE_MASK         0x00010000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_I_SCAN_DONE_MASK          0x00200000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_LOL_MASK                  0x00080000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_LOL_UDL_MASK              0x00100000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA2_PCS_TX_DET_RX_N_MASK 0x00400000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA2_PCS_TX_DET_RX_P_MASK 0x00800000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_PMA_RST_DONE_MASK         0x00020000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_RX_EI_MASK                0x00040000
#define CPU_MAC_UNIT_HSS_MON_W4_MON_HSS_L1_RX_EI_FILTERED_MASK       0x01000000
#define CPU_MAC_UNIT_HSS_MON_W5_MON_HSS_L1_I_SCAN_RESULTS_31_0_MASK  0xffffffff
#define CPU_MAC_UNIT_HSS_MON_W6_MON_HSS_L1_I_SCAN_RESULTS_63_32_MASK 0xffffffff

/* ################################################################################
 * # CpuMacHssRegAccTimingCfg Definition */
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W0_CFG_ACTIVE_CYCLES_BIT      0
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W0_CFG_HOLD_CYCLES_BIT        8
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W1_CFG_RD_OUT_VALID_CYCLES_BIT 8
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W1_CFG_SETUP_CYCLES_BIT       0

#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W0_CFG_ACTIVE_CYCLES_MASK     0x000000ff
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W0_CFG_HOLD_CYCLES_MASK       0x0000ff00
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W1_CFG_RD_OUT_VALID_CYCLES_MASK 0x0000ff00
#define CPU_MAC_HSS_REG_ACC_TIMING_CFG_W1_CFG_SETUP_CYCLES_MASK      0x000000ff

/* ################################################################################
 * # CpuMacUnitResetCtl Definition */
#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_BASE_BIT                0
#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC0_BIT            2
#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC1_BIT            1

#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_BASE_MASK               0x00000001
#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC0_MASK           0x00000004
#define CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC1_MASK           0x00000002

/* ################################################################################
 * # CpuMacUnitHssRegAccCtl Definition */
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_ADDR_BIT             0
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_ID_BIT               24
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_IS_READ_BIT          16
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_VALID_BIT            31
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_WDATA_BIT            8

#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_ADDR_MASK            0x000000ff
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_ID_MASK              0x0f000000
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_IS_READ_MASK         0x00010000
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_VALID_MASK           0x80000000
#define CPU_MAC_UNIT_HSS_REG_ACC_CTL_W0_HSS_ACC_WDATA_MASK           0x0000ff00

/* ################################################################################
 * # CpuMacUnitHssRegAccResult Definition */
#define CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_BIT           31
#define CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_DATA_BIT      0

#define CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_MASK          0x80000000
#define CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_DATA_MASK     0x000000ff

/* ################################################################################
 * # CpuMacUnitAxiCfg Definition */
#define CPU_MAC_UNIT_AXI_CFG_W0_CFG_AXI0_ID_BIT                      0
#define CPU_MAC_UNIT_AXI_CFG_W0_CFG_AXI1_ID_BIT                      4

#define CPU_MAC_UNIT_AXI_CFG_W0_CFG_AXI0_ID_MASK                     0x0000000f
#define CPU_MAC_UNIT_AXI_CFG_W0_CFG_AXI1_ID_MASK                     0x000000f0

/* ################################################################################
 * # CpuMacUnitTsCfg Definition */
#define CPU_MAC_UNIT_TS_CFG_W0_CFG_FORCE_S_AND_NS_EN_BIT             31
#define CPU_MAC_UNIT_TS_CFG_W0_CFG_TX_CAPTURE_FIFO_INTR_THRD_BIT     0

#define CPU_MAC_UNIT_TS_CFG_W0_CFG_FORCE_S_AND_NS_EN_MASK            0x80000000
#define CPU_MAC_UNIT_TS_CFG_W0_CFG_TX_CAPTURE_FIFO_INTR_THRD_MASK    0x0000000f

/* ################################################################################
 * # CpuMacUnitFifoStatus Definition */
#define CPU_MAC_UNIT_FIFO_STATUS_W0_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIFO_DEPTH_BIT 0

#define CPU_MAC_UNIT_FIFO_STATUS_W0_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIFO_DEPTH_MASK 0x0000000f

/* ################################################################################
 * # CpuMacUnitTsMon Definition */
#define CPU_MAC_UNIT_TS_MON_W0_MON_ADJ_NS_BIT                        0
#define CPU_MAC_UNIT_TS_MON_W1_MON_ADJ_SECOND_BIT                    0
#define CPU_MAC_UNIT_TS_MON_W2_MON_TX_CAPTURE_FIFO_DROP_CNT_BIT      0

#define CPU_MAC_UNIT_TS_MON_W0_MON_ADJ_NS_MASK                       0x3fffffff
#define CPU_MAC_UNIT_TS_MON_W1_MON_ADJ_SECOND_MASK                   0xffffffff
#define CPU_MAC_UNIT_TS_MON_W2_MON_TX_CAPTURE_FIFO_DROP_CNT_MASK     0x0000000f

/* ################################################################################
 * # CpuMacUnitRefPulseCfg Definition */
#define CPU_MAC_UNIT_REF_PULSE_CFG_W0_REF_PAUSE_TIMER_PULSE_DIV_BIT  0
#define CPU_MAC_UNIT_REF_PULSE_CFG_W0_REF_PAUSE_TIMER_PULSE_RST_BIT  31
#define CPU_MAC_UNIT_REF_PULSE_CFG_W1_REF_LINK_PULSE_DIV_BIT         0
#define CPU_MAC_UNIT_REF_PULSE_CFG_W1_REF_LINK_PULSE_RST_BIT         31
#define CPU_MAC_UNIT_REF_PULSE_CFG_W2_REF_LINK_FILTER_PULSE_DIV_BIT  0
#define CPU_MAC_UNIT_REF_PULSE_CFG_W2_REF_LINK_FILTER_PULSE_RST_BIT  31
#define CPU_MAC_UNIT_REF_PULSE_CFG_W3_REF_EEE_PULSE_DIV_BIT          0
#define CPU_MAC_UNIT_REF_PULSE_CFG_W3_REF_EEE_PULSE_RST_BIT          31

#define CPU_MAC_UNIT_REF_PULSE_CFG_W0_REF_PAUSE_TIMER_PULSE_DIV_MASK 0x7fffffff
#define CPU_MAC_UNIT_REF_PULSE_CFG_W0_REF_PAUSE_TIMER_PULSE_RST_MASK 0x80000000
#define CPU_MAC_UNIT_REF_PULSE_CFG_W1_REF_LINK_PULSE_DIV_MASK        0x7fffffff
#define CPU_MAC_UNIT_REF_PULSE_CFG_W1_REF_LINK_PULSE_RST_MASK        0x80000000
#define CPU_MAC_UNIT_REF_PULSE_CFG_W2_REF_LINK_FILTER_PULSE_DIV_MASK 0x7fffffff
#define CPU_MAC_UNIT_REF_PULSE_CFG_W2_REF_LINK_FILTER_PULSE_RST_MASK 0x80000000
#define CPU_MAC_UNIT_REF_PULSE_CFG_W3_REF_EEE_PULSE_DIV_MASK         0x7fffffff
#define CPU_MAC_UNIT_REF_PULSE_CFG_W3_REF_EEE_PULSE_RST_MASK         0x80000000

/* ################################################################################
 * # CpuMacUnitInterruptFunc Definition */
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W0_VALUE_SET0_CPU_MAC_UNIT_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W1_VALUE_RESET0_CPU_MAC_UNIT_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W2_MASK_SET0_CPU_MAC_UNIT_INTERRUPT_FUNC_BIT 0
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W3_MASK_RESET0_CPU_MAC_UNIT_INTERRUPT_FUNC_BIT 0

#define CPU_MAC_UNIT_INTERRUPT_FUNC_W0_VALUE_SET0_CPU_MAC_UNIT_INTERRUPT_FUNC_MASK 0x00000001
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W1_VALUE_RESET0_CPU_MAC_UNIT_INTERRUPT_FUNC_MASK 0x00000001
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W2_MASK_SET0_CPU_MAC_UNIT_INTERRUPT_FUNC_MASK 0x00000001
#define CPU_MAC_UNIT_INTERRUPT_FUNC_W3_MASK_RESET0_CPU_MAC_UNIT_INTERRUPT_FUNC_MASK 0x00000001

/* ################################################################################
 * # CpuMacUnitHssCfg Definition */
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_BCLK_RST_N_BIT               1
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_BIAS_DN_EN_BIT      13
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_BIAS_UP_EN_BIT      14
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_CFG_DIV_SEL_BIT     5
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_CK_TREE_PD_BIT      29
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_EN_TX_CK_DN_BIT     24
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_EN_TX_CK_UP_BIT     23
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_IBIAS_PD_BIT        26
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_LINK_BUF_EN_BIT     16
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_MULTI_LANE_MODE_BIT 12
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_PMA_TX_CK_PD_BIT    31
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_REFCK_PD_BIT        30
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_REFCK_TERM_EN_BIT   25
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_RST_TREE_PD_MA_BIT  27
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_VCO_DIV_SEL_BIT     2
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_VCO_PD_BIT          28
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_PCLK_GATING_BIT         15
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_PCS2_PMA_PHY_MODE_BIT   18
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_REXT10_K_BIT            11
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_RST_N_BIT               17
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_GLB_RST_N_BIT                0
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_CTRL_LOGIC_PD_BIT   28
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_EN_DUMMY_BIT        29
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_PD_DIV64_BIT        26
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_PD_DIV66_BIT        27
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_CENTER_SPREAD_BIT 25
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_MCNT_MAX_VAL_BIT 9
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_NCN_MAX_VAL_BIT 14
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_RES_EN_BIT   2
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SRC_SEL_BIT  3
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_PI_BW_BIT 6
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_PI_STEP_BIT 4
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_RESET_N_BIT 1
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_RTL_CLK_SEL_BIT 0
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_AUX_RX_CK_SEL_BIT         2
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_AUX_TX_CK_SEL_BIT         1
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_DATA_WIDTH_SEL_BIT        7
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_HWT_MULTI_LANE_MODE_BIT   3
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_LANE_ID_BIT               4
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_PHY_MODE_BIT     22
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_TX_SWING_BIT     31
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_VGA_CTRL_BIT     27
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PMA_RX_DIV_SEL_BIT        19
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PMA_TX_CK_SEL_BIT         13
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RST_N_BIT                 0
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RX_RATE_SEL_BIT           17
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RX_RST_N_BIT              16
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_TX_RATE_SEL_BIT           11
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_TX_RST_N_BIT              10
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_HWT_FOM_SEL_BIT           19
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_PI_EXT_DAC_BIT20_TO14_BIT 20
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_PI_EXT_DAC_BIT23_TO21_BIT 27
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_TX_EI_BIT        18
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_ADV_BIT            9
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_DLY_BIT            8
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_MAIN_BIT           7
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_I_SCAN_EN_BIT         6
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_BIT           1
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_BIT           10
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_MAIN_BIT          0
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_ITX_PREEMP_BASE_BIT 30
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_MCNT_MAX_VAL_BIT 25
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_PI_FLOOP_STEPS_BIT 23
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_RX_SSC_LH_BIT    22
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_EN_BIT       21
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_PI_BW_BIT    17
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_PI_STEP_BIT  15
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_RESETB_BIT   14
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_RTL_CLK_SEL_BIT 13
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_TX_DET_RX_EN_BIT 12
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_TX_DET_RX_STR_BIT 11
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMANCNT_MAX_VAL_BIT  0
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EN_PRE_EMPH_BIT  31
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EQC_FORCE_BIT    27
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EQR_BYP_BIT      26
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_ISCAN_EXT_DAC_BIT7_BIT 25
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_POWER_CTRL_BIT   0
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_FORCE_SIGNAL_DETECT_BIT   31
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCLK_GATING_BIT           7
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_CENTER_SPREADING_BIT 6
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_CTLE_RSTN_BIT    17
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_DIS2ND_ORDER_BIT 5
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EID_LP_BIT       18
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EN_DFE_DIG_BIT   4
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EQ_RES_BIT       0
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_HOLD_DFE_BIT     22
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_ISCAN_SEL_BIT    19
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_PI_HOLD_BIT      20
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_R50_EN_BIT       21
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_BIT    8
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_DLEV_BIT         23
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_DLEV_BYP_BIT     31
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H1_BIT           0
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H2_BIT           6
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H3_BIT           11
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H4_BIT           15
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H5_BIT           19
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H_BYP_BIT        30
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_AUX_RX_CK_SEL_BIT         2
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_AUX_TX_CK_SEL_BIT         1
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_DATA_WIDTH_SEL_BIT        7
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_HWT_MULTI_LANE_MODE_BIT   3
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_LANE_ID_BIT               4
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_PHY_MODE_BIT     22
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_TX_SWING_BIT     31
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_VGA_CTRL_BIT     27
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PMA_RX_DIV_SEL_BIT        19
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PMA_TX_CK_SEL_BIT         13
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RST_N_BIT                 0
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RX_RATE_SEL_BIT           17
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RX_RST_N_BIT              16
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_TX_RATE_SEL_BIT           11
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_TX_RST_N_BIT              10
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_HWT_FOM_SEL_BIT           19
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_PI_EXT_DAC_BIT20_TO14_BIT 20
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_PI_EXT_DAC_BIT23_TO21_BIT 27
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_TX_EI_BIT        18
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_ADV_BIT            9
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_DLY_BIT            8
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_MAIN_BIT           7
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_I_SCAN_EN_BIT         6
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_BIT           1
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_BIT           10
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_MAIN_BIT          0
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_ITX_PREEMP_BASE_BIT 30
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_MCNT_MAX_VAL_BIT 25
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_PI_FLOOP_STEPS_BIT 23
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_RX_SSC_LH_BIT   22
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_EN_BIT      21
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_PI_BW_BIT   17
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_PI_STEP_BIT 15
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_RESETB_BIT  14
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_RTL_CLK_SEL_BIT 13
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_TX_DET_RX_EN_BIT 12
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_TX_DET_RX_STR_BIT 11
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMANCNT_MAX_VAL_BIT 0
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EN_PRE_EMPH_BIT 31
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EQC_FORCE_BIT   27
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EQR_BYP_BIT     26
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_ISCAN_EXT_DAC_BIT7_BIT 25
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_POWER_CTRL_BIT  0
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_FORCE_SIGNAL_DETECT_BIT  31
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCLK_GATING_BIT          7
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_CENTER_SPREADING_BIT 6
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_CTLE_RSTN_BIT   17
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_DIS2ND_ORDER_BIT 5
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EID_LP_BIT      18
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EN_DFE_DIG_BIT  4
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EQ_RES_BIT      0
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_HOLD_DFE_BIT    22
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_ISCAN_SEL_BIT   19
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_PI_HOLD_BIT     20
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_R50_EN_BIT      21
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_BIT   8
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_DLEV_BIT        23
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_DLEV_BYP_BIT    31
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H1_BIT          0
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H2_BIT          6
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H3_BIT          11
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H4_BIT          15
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H5_BIT          19
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H_BYP_BIT       30

#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_BCLK_RST_N_MASK              0x00000002
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_BIAS_DN_EN_MASK     0x00002000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_BIAS_UP_EN_MASK     0x00004000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_CFG_DIV_SEL_MASK    0x000007e0
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_CK_TREE_PD_MASK     0x20000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_EN_TX_CK_DN_MASK    0x01000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_EN_TX_CK_UP_MASK    0x00800000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_IBIAS_PD_MASK       0x04000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_LINK_BUF_EN_MASK    0x00010000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_MULTI_LANE_MODE_MASK 0x00001000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_PMA_TX_CK_PD_MASK   0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_REFCK_PD_MASK       0x40000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_REFCK_TERM_EN_MASK  0x02000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_RST_TREE_PD_MA_MASK 0x08000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_VCO_DIV_SEL_MASK    0x0000001c
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_HWT_VCO_PD_MASK         0x10000000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_PCLK_GATING_MASK        0x00008000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_PCS2_PMA_PHY_MODE_MASK  0x007c0000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_REXT10_K_MASK           0x00000800
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_CMU0_RST_N_MASK              0x00020000
#define CPU_MAC_UNIT_HSS_CFG_W0_CFG_HSS_GLB_RST_N_MASK               0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_CTRL_LOGIC_PD_MASK  0x10000000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_EN_DUMMY_MASK       0x20000000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_PD_DIV64_MASK       0x04000000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_PD_DIV66_MASK       0x08000000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_CENTER_SPREAD_MASK 0x02000000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_MCNT_MAX_VAL_MASK 0x00003e00
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_NCN_MAX_VAL_MASK 0x01ffc000
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_RES_EN_MASK  0x00000004
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SRC_SEL_MASK 0x00000008
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_PI_BW_MASK 0x000001c0
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_PI_STEP_MASK 0x00000030
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_RESET_N_MASK 0x00000002
#define CPU_MAC_UNIT_HSS_CFG_W1_CFG_HSS_CMU0_HWT_REF_CK_SSC_RTL_CLK_SEL_MASK 0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_AUX_RX_CK_SEL_MASK        0x00000004
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_AUX_TX_CK_SEL_MASK        0x00000002
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_DATA_WIDTH_SEL_MASK       0x00000380
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_HWT_MULTI_LANE_MODE_MASK  0x00000008
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_LANE_ID_MASK              0x00000070
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_PHY_MODE_MASK    0x07c00000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_TX_SWING_MASK    0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PCS2_PMA_VGA_CTRL_MASK    0x78000000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PMA_RX_DIV_SEL_MASK       0x00380000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_PMA_TX_CK_SEL_MASK        0x0000e000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RST_N_MASK                0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RX_RATE_SEL_MASK          0x00060000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_RX_RST_N_MASK             0x00010000
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_TX_RATE_SEL_MASK          0x00001800
#define CPU_MAC_UNIT_HSS_CFG_W2_CFG_HSS_L0_TX_RST_N_MASK             0x00000400
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_HWT_FOM_SEL_MASK          0x00080000
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_PI_EXT_DAC_BIT20_TO14_MASK 0x07f00000
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_PI_EXT_DAC_BIT23_TO21_MASK 0x38000000
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS2_PMA_TX_EI_MASK       0x00040000
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_ADV_MASK           0x00000200
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_DLY_MASK           0x00000100
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_MAIN_MASK          0x00000080
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_I_SCAN_EN_MASK        0x00000040
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_MASK          0x0000003e
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_MASK          0x00007c00
#define CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_MAIN_MASK         0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_ITX_PREEMP_BASE_MASK 0xc0000000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_MCNT_MAX_VAL_MASK 0x3e000000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_PI_FLOOP_STEPS_MASK 0x01800000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_RX_SSC_LH_MASK   0x00400000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_EN_MASK      0x00200000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_PI_BW_MASK   0x001e0000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_PI_STEP_MASK 0x00018000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_RESETB_MASK  0x00004000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_SSC_RTL_CLK_SEL_MASK 0x00002000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_TX_DET_RX_EN_MASK 0x00001000
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMA_TX_DET_RX_STR_MASK 0x00000800
#define CPU_MAC_UNIT_HSS_CFG_W4_CFG_HSS_L0_PCS2_PMANCNT_MAX_VAL_MASK 0x000007ff
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EN_PRE_EMPH_MASK 0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EQC_FORCE_MASK   0x78000000
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_EQR_BYP_MASK     0x04000000
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_ISCAN_EXT_DAC_BIT7_MASK 0x02000000
#define CPU_MAC_UNIT_HSS_CFG_W5_CFG_HSS_L0_PCS2_PMA_POWER_CTRL_MASK  0x01ffffff
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_FORCE_SIGNAL_DETECT_MASK  0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCLK_GATING_MASK          0x00000080
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_CENTER_SPREADING_MASK 0x00000040
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_CTLE_RSTN_MASK   0x00020000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_DIS2ND_ORDER_MASK 0x00000020
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EID_LP_MASK      0x00040000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EN_DFE_DIG_MASK  0x00000010
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_EQ_RES_MASK      0x0000000f
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_HOLD_DFE_MASK    0x00400000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_ISCAN_SEL_MASK   0x00080000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_PI_HOLD_MASK     0x00100000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_R50_EN_MASK      0x00200000
#define CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_MASK   0x0001ff00
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_DLEV_MASK        0x3f800000
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_DLEV_BYP_MASK    0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H1_MASK          0x0000003f
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H2_MASK          0x000007c0
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H3_MASK          0x00007800
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H4_MASK          0x00078000
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H5_MASK          0x00780000
#define CPU_MAC_UNIT_HSS_CFG_W7_CFG_HSS_L0_PCS2_PMA_H_BYP_MASK       0x40000000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_AUX_RX_CK_SEL_MASK        0x00000004
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_AUX_TX_CK_SEL_MASK        0x00000002
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_DATA_WIDTH_SEL_MASK       0x00000380
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_HWT_MULTI_LANE_MODE_MASK  0x00000008
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_LANE_ID_MASK              0x00000070
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_PHY_MODE_MASK    0x07c00000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_TX_SWING_MASK    0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PCS2_PMA_VGA_CTRL_MASK    0x78000000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PMA_RX_DIV_SEL_MASK       0x00380000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_PMA_TX_CK_SEL_MASK        0x0000e000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RST_N_MASK                0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RX_RATE_SEL_MASK          0x00060000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_RX_RST_N_MASK             0x00010000
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_TX_RATE_SEL_MASK          0x00001800
#define CPU_MAC_UNIT_HSS_CFG_W8_CFG_HSS_L1_TX_RST_N_MASK             0x00000400
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_HWT_FOM_SEL_MASK          0x00080000
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_PI_EXT_DAC_BIT20_TO14_MASK 0x07f00000
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_PI_EXT_DAC_BIT23_TO21_MASK 0x38000000
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS2_PMA_TX_EI_MASK       0x00040000
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_ADV_MASK           0x00000200
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_DLY_MASK           0x00000100
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_MAIN_MASK          0x00000080
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_I_SCAN_EN_MASK        0x00000040
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_MASK          0x0000003e
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_MASK          0x00007c00
#define CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_MAIN_MASK         0x00000001
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_ITX_PREEMP_BASE_MASK 0xc0000000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_MCNT_MAX_VAL_MASK 0x3e000000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_PI_FLOOP_STEPS_MASK 0x01800000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_RX_SSC_LH_MASK  0x00400000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_EN_MASK     0x00200000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_PI_BW_MASK  0x001e0000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_PI_STEP_MASK 0x00018000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_RESETB_MASK 0x00004000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_SSC_RTL_CLK_SEL_MASK 0x00002000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_TX_DET_RX_EN_MASK 0x00001000
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMA_TX_DET_RX_STR_MASK 0x00000800
#define CPU_MAC_UNIT_HSS_CFG_W10_CFG_HSS_L1_PCS2_PMANCNT_MAX_VAL_MASK 0x000007ff
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EN_PRE_EMPH_MASK 0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EQC_FORCE_MASK  0x78000000
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_EQR_BYP_MASK    0x04000000
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_ISCAN_EXT_DAC_BIT7_MASK 0x02000000
#define CPU_MAC_UNIT_HSS_CFG_W11_CFG_HSS_L1_PCS2_PMA_POWER_CTRL_MASK 0x01ffffff
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_FORCE_SIGNAL_DETECT_MASK 0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCLK_GATING_MASK         0x00000080
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_CENTER_SPREADING_MASK 0x00000040
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_CTLE_RSTN_MASK  0x00020000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_DIS2ND_ORDER_MASK 0x00000020
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EID_LP_MASK     0x00040000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EN_DFE_DIG_MASK 0x00000010
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_EQ_RES_MASK     0x0000000f
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_HOLD_DFE_MASK   0x00400000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_ISCAN_SEL_MASK  0x00080000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_PI_HOLD_MASK    0x00100000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_R50_EN_MASK     0x00200000
#define CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_MASK  0x0001ff00
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_DLEV_MASK       0x3f800000
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_DLEV_BYP_MASK   0x80000000
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H1_MASK         0x0000003f
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H2_MASK         0x000007c0
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H3_MASK         0x00007800
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H4_MASK         0x00078000
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H5_MASK         0x00780000
#define CPU_MAC_UNIT_HSS_CFG_W13_CFG_HSS_L1_PCS2_PMA_H_BYP_MASK      0x40000000

/* ################################################################################
 * # CpuMacUnitIpCamCfg Definition */
#define CPU_MAC_UNIT_IP_CAM_CFG_W0_CFG_IP_CAM_VALUE0_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W1_CFG_IP_CAM_VALUE0_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W2_CFG_IP_CAM_VALUE0_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W3_CFG_IP_CAM_VALUE0_127_96_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W4_CFG_IP_CAM_VALUE1_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W5_CFG_IP_CAM_VALUE1_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W6_CFG_IP_CAM_VALUE1_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W7_CFG_IP_CAM_VALUE1_127_96_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W8_CFG_IP_CAM_VALUE2_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W9_CFG_IP_CAM_VALUE2_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W10_CFG_IP_CAM_VALUE2_95_64_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W11_CFG_IP_CAM_VALUE2_127_96_BIT     0
#define CPU_MAC_UNIT_IP_CAM_CFG_W12_CFG_IP_CAM_VALUE3_31_0_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W13_CFG_IP_CAM_VALUE3_63_32_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W14_CFG_IP_CAM_VALUE3_95_64_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W15_CFG_IP_CAM_VALUE3_127_96_BIT     0
#define CPU_MAC_UNIT_IP_CAM_CFG_W16_CFG_IP_CAM_MASK0_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W17_CFG_IP_CAM_MASK0_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W18_CFG_IP_CAM_MASK0_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W19_CFG_IP_CAM_MASK0_127_96_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W20_CFG_IP_CAM_MASK1_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W21_CFG_IP_CAM_MASK1_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W22_CFG_IP_CAM_MASK1_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W23_CFG_IP_CAM_MASK1_127_96_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W24_CFG_IP_CAM_MASK2_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W25_CFG_IP_CAM_MASK2_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W26_CFG_IP_CAM_MASK2_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W27_CFG_IP_CAM_MASK2_127_96_BIT      0
#define CPU_MAC_UNIT_IP_CAM_CFG_W28_CFG_IP_CAM_MASK3_31_0_BIT        0
#define CPU_MAC_UNIT_IP_CAM_CFG_W29_CFG_IP_CAM_MASK3_63_32_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W30_CFG_IP_CAM_MASK3_95_64_BIT       0
#define CPU_MAC_UNIT_IP_CAM_CFG_W31_CFG_IP_CAM_MASK3_127_96_BIT      0

#define CPU_MAC_UNIT_IP_CAM_CFG_W0_CFG_IP_CAM_VALUE0_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W1_CFG_IP_CAM_VALUE0_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W2_CFG_IP_CAM_VALUE0_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W3_CFG_IP_CAM_VALUE0_127_96_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W4_CFG_IP_CAM_VALUE1_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W5_CFG_IP_CAM_VALUE1_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W6_CFG_IP_CAM_VALUE1_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W7_CFG_IP_CAM_VALUE1_127_96_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W8_CFG_IP_CAM_VALUE2_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W9_CFG_IP_CAM_VALUE2_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W10_CFG_IP_CAM_VALUE2_95_64_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W11_CFG_IP_CAM_VALUE2_127_96_MASK    0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W12_CFG_IP_CAM_VALUE3_31_0_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W13_CFG_IP_CAM_VALUE3_63_32_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W14_CFG_IP_CAM_VALUE3_95_64_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W15_CFG_IP_CAM_VALUE3_127_96_MASK    0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W16_CFG_IP_CAM_MASK0_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W17_CFG_IP_CAM_MASK0_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W18_CFG_IP_CAM_MASK0_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W19_CFG_IP_CAM_MASK0_127_96_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W20_CFG_IP_CAM_MASK1_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W21_CFG_IP_CAM_MASK1_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W22_CFG_IP_CAM_MASK1_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W23_CFG_IP_CAM_MASK1_127_96_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W24_CFG_IP_CAM_MASK2_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W25_CFG_IP_CAM_MASK2_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W26_CFG_IP_CAM_MASK2_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W27_CFG_IP_CAM_MASK2_127_96_MASK     0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W28_CFG_IP_CAM_MASK3_31_0_MASK       0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W29_CFG_IP_CAM_MASK3_63_32_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W30_CFG_IP_CAM_MASK3_95_64_MASK      0xffffffff
#define CPU_MAC_UNIT_IP_CAM_CFG_W31_CFG_IP_CAM_MASK3_127_96_MASK     0xffffffff

/* ################################################################################
 * # CpuMacUnitMacCamCfg Definition */
#define CPU_MAC_UNIT_MAC_CAM_CFG_W0_CFG_MAC_CAM_VALUE0_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W1_CFG_MAC_CAM_VALUE0_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W2_CFG_MAC_CAM_VALUE1_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W3_CFG_MAC_CAM_VALUE1_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W4_CFG_MAC_CAM_VALUE2_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W5_CFG_MAC_CAM_VALUE2_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W6_CFG_MAC_CAM_VALUE3_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W7_CFG_MAC_CAM_VALUE3_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W8_CFG_MAC_CAM_VALUE4_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W9_CFG_MAC_CAM_VALUE4_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W10_CFG_MAC_CAM_VALUE5_31_0_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W11_CFG_MAC_CAM_VALUE5_47_32_BIT    0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W12_CFG_MAC_CAM_VALUE6_31_0_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W13_CFG_MAC_CAM_VALUE6_47_32_BIT    0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W14_CFG_MAC_CAM_VALUE7_31_0_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W15_CFG_MAC_CAM_VALUE7_47_32_BIT    0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W16_CFG_MAC_CAM_MASK0_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W17_CFG_MAC_CAM_MASK0_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W18_CFG_MAC_CAM_MASK1_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W19_CFG_MAC_CAM_MASK1_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W20_CFG_MAC_CAM_MASK2_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W21_CFG_MAC_CAM_MASK2_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W22_CFG_MAC_CAM_MASK3_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W23_CFG_MAC_CAM_MASK3_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W24_CFG_MAC_CAM_MASK4_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W25_CFG_MAC_CAM_MASK4_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W26_CFG_MAC_CAM_MASK5_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W27_CFG_MAC_CAM_MASK5_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W28_CFG_MAC_CAM_MASK6_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W29_CFG_MAC_CAM_MASK6_47_32_BIT     0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W30_CFG_MAC_CAM_MASK7_31_0_BIT      0
#define CPU_MAC_UNIT_MAC_CAM_CFG_W31_CFG_MAC_CAM_MASK7_47_32_BIT     0

#define CPU_MAC_UNIT_MAC_CAM_CFG_W0_CFG_MAC_CAM_VALUE0_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W1_CFG_MAC_CAM_VALUE0_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W2_CFG_MAC_CAM_VALUE1_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W3_CFG_MAC_CAM_VALUE1_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W4_CFG_MAC_CAM_VALUE2_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W5_CFG_MAC_CAM_VALUE2_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W6_CFG_MAC_CAM_VALUE3_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W7_CFG_MAC_CAM_VALUE3_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W8_CFG_MAC_CAM_VALUE4_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W9_CFG_MAC_CAM_VALUE4_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W10_CFG_MAC_CAM_VALUE5_31_0_MASK    0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W11_CFG_MAC_CAM_VALUE5_47_32_MASK   0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W12_CFG_MAC_CAM_VALUE6_31_0_MASK    0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W13_CFG_MAC_CAM_VALUE6_47_32_MASK   0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W14_CFG_MAC_CAM_VALUE7_31_0_MASK    0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W15_CFG_MAC_CAM_VALUE7_47_32_MASK   0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W16_CFG_MAC_CAM_MASK0_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W17_CFG_MAC_CAM_MASK0_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W18_CFG_MAC_CAM_MASK1_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W19_CFG_MAC_CAM_MASK1_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W20_CFG_MAC_CAM_MASK2_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W21_CFG_MAC_CAM_MASK2_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W22_CFG_MAC_CAM_MASK3_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W23_CFG_MAC_CAM_MASK3_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W24_CFG_MAC_CAM_MASK4_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W25_CFG_MAC_CAM_MASK4_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W26_CFG_MAC_CAM_MASK5_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W27_CFG_MAC_CAM_MASK5_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W28_CFG_MAC_CAM_MASK6_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W29_CFG_MAC_CAM_MASK6_47_32_MASK    0x0000ffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W30_CFG_MAC_CAM_MASK7_31_0_MASK     0xffffffff
#define CPU_MAC_UNIT_MAC_CAM_CFG_W31_CFG_MAC_CAM_MASK7_47_32_MASK    0x0000ffff

/* ################################################################################
 * # CpuMacUnitFilterCfg Definition */
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_BLOCK_SUPPRESSION_TRAFFIC_BIT 6
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_ETHER_TYPE_FILTER_EN_BIT      1
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_FILTER_IS_LOOSE_BIT           5
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_FILTER_EN_BIT         2
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA0_BIT            17
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA1_BIT            18
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA2_BIT            19
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA3_BIT            20
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM0_IS_V6_BIT             21
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM1_IS_V6_BIT             22
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM2_IS_V6_BIT             23
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM3_IS_V6_BIT             24
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM_IS_BLACK_LIST_BIT      4
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA0_BIT           9
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA1_BIT           10
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA2_BIT           11
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA3_BIT           12
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA4_BIT           13
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA5_BIT           14
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA6_BIT           15
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA7_BIT           16
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_CAM_IS_BLACK_LIST_BIT     3
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_FILTER_EN_BIT             0
#define CPU_MAC_UNIT_FILTER_CFG_W1_CFG_SUPPRESSION_ETHER_TYPE0_BIT   16
#define CPU_MAC_UNIT_FILTER_CFG_W1_CFG_VLAN_TPID_BIT                 0
#define CPU_MAC_UNIT_FILTER_CFG_W2_CFG_SUPPRESSION_ETHER_TYPE1_BIT   0
#define CPU_MAC_UNIT_FILTER_CFG_W2_CFG_SUPPRESSION_ETHER_TYPE2_BIT   16
#define CPU_MAC_UNIT_FILTER_CFG_W3_CFG_CONFIRM_ETHER_TYPE0_BIT       0
#define CPU_MAC_UNIT_FILTER_CFG_W3_CFG_CONFIRM_ETHER_TYPE1_BIT       16
#define CPU_MAC_UNIT_FILTER_CFG_W4_CFG_CONFIRM_ETHER_TYPE2_BIT       0
#define CPU_MAC_UNIT_FILTER_CFG_W5_CFG_METER_TOKEN_UPD_INTERVAL_BIT  0
#define CPU_MAC_UNIT_FILTER_CFG_W5_CFG_METER_TOKEN_UPD_VALUE_BIT     16

#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_BLOCK_SUPPRESSION_TRAFFIC_MASK 0x000001c0
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_ETHER_TYPE_FILTER_EN_MASK     0x00000002
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_FILTER_IS_LOOSE_MASK          0x00000020
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_FILTER_EN_MASK        0x00000004
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA0_MASK           0x00020000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA1_MASK           0x00040000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA2_MASK           0x00080000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_ADDR_IS_SA3_MASK           0x00100000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM0_IS_V6_MASK            0x00200000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM1_IS_V6_MASK            0x00400000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM2_IS_V6_MASK            0x00800000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM3_IS_V6_MASK            0x01000000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_IP_CAM_IS_BLACK_LIST_MASK     0x00000010
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA0_MASK          0x00000200
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA1_MASK          0x00000400
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA2_MASK          0x00000800
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA3_MASK          0x00001000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA4_MASK          0x00002000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA5_MASK          0x00004000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA6_MASK          0x00008000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA7_MASK          0x00010000
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_CAM_IS_BLACK_LIST_MASK    0x00000008
#define CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_FILTER_EN_MASK            0x00000001
#define CPU_MAC_UNIT_FILTER_CFG_W1_CFG_SUPPRESSION_ETHER_TYPE0_MASK  0xffff0000
#define CPU_MAC_UNIT_FILTER_CFG_W1_CFG_VLAN_TPID_MASK                0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG_W2_CFG_SUPPRESSION_ETHER_TYPE1_MASK  0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG_W2_CFG_SUPPRESSION_ETHER_TYPE2_MASK  0xffff0000
#define CPU_MAC_UNIT_FILTER_CFG_W3_CFG_CONFIRM_ETHER_TYPE0_MASK      0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG_W3_CFG_CONFIRM_ETHER_TYPE1_MASK      0xffff0000
#define CPU_MAC_UNIT_FILTER_CFG_W4_CFG_CONFIRM_ETHER_TYPE2_MASK      0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG_W5_CFG_METER_TOKEN_UPD_INTERVAL_MASK 0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG_W5_CFG_METER_TOKEN_UPD_VALUE_MASK    0xffff0000

/* ################################################################################
 * # CpuMacUnitFilterCfg1 Definition */
#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_MAC_CAM_ENABLE_BMP0_BIT      16
#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_MAC_CAM_ENABLE_BMP1_BIT      24
#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_METER_TOKEN_UPD_VALUE1_BIT   0
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE0_BIT 16
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE1_BIT 17
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT0_BIT         18
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT1_BIT         19
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_ETHER_TYPE_ENABLE_BMP0_BIT   8
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_ETHER_TYPE_ENABLE_BMP1_BIT   12
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_IP_CAM_ENABLE_BMP0_BIT       0
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_IP_CAM_ENABLE_BMP1_BIT       4
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_METER_TOKEN_UPD_VALUE_SEL0_BIT 22
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_METER_TOKEN_UPD_VALUE_SEL1_BIT 23
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_VLAN_FILTER_ENABLE0_BIT      21
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_VLAN_FILTER_ENABLE1_BIT      20
#define CPU_MAC_UNIT_FILTER_CFG1_W2_CFG_METER_TOKEN_MAX_VALUE0_BIT   0
#define CPU_MAC_UNIT_FILTER_CFG1_W3_CFG_METER_TOKEN_MAX_VALUE1_BIT   0

#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_MAC_CAM_ENABLE_BMP0_MASK     0x00ff0000
#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_MAC_CAM_ENABLE_BMP1_MASK     0xff000000
#define CPU_MAC_UNIT_FILTER_CFG1_W0_CFG_METER_TOKEN_UPD_VALUE1_MASK  0x0000ffff
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE0_MASK 0x00010000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE1_MASK 0x00020000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT0_MASK        0x00040000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT1_MASK        0x00080000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_ETHER_TYPE_ENABLE_BMP0_MASK  0x00000f00
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_ETHER_TYPE_ENABLE_BMP1_MASK  0x0000f000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_IP_CAM_ENABLE_BMP0_MASK      0x0000000f
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_IP_CAM_ENABLE_BMP1_MASK      0x000000f0
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_METER_TOKEN_UPD_VALUE_SEL0_MASK 0x00400000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_METER_TOKEN_UPD_VALUE_SEL1_MASK 0x00800000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_VLAN_FILTER_ENABLE0_MASK     0x00200000
#define CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_VLAN_FILTER_ENABLE1_MASK     0x00100000
#define CPU_MAC_UNIT_FILTER_CFG1_W2_CFG_METER_TOKEN_MAX_VALUE0_MASK  0x00ffffff
#define CPU_MAC_UNIT_FILTER_CFG1_W3_CFG_METER_TOKEN_MAX_VALUE1_MASK  0x00ffffff

struct CpuMacUnit_mems {
	u32 CpuMacUnitTxTsCaptureFifo0[3];	/* 0x00000400 */
	u32 CpuMacUnitTxTsCaptureFifo0_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo1[3];	/* 0x00000410 */
	u32 CpuMacUnitTxTsCaptureFifo1_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo2[3];	/* 0x00000420 */
	u32 CpuMacUnitTxTsCaptureFifo2_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo3[3];	/* 0x00000430 */
	u32 CpuMacUnitTxTsCaptureFifo3_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo4[3];	/* 0x00000440 */
	u32 CpuMacUnitTxTsCaptureFifo4_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo5[3];	/* 0x00000450 */
	u32 CpuMacUnitTxTsCaptureFifo5_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo6[3];	/* 0x00000460 */
	u32 CpuMacUnitTxTsCaptureFifo6_rsv3;
	u32 CpuMacUnitTxTsCaptureFifo7[3];	/* 0x00000470 */
	u32 CpuMacUnitTxTsCaptureFifo7_rsv3;
	u32 CpuMacUnitTxCaptureTs0[3];	/* 0x00000480 */
	u32 CpuMacUnitTxCaptureTs0_rsv3;
};

/* ################################################################################
 * # CpuMacUnitTxTsCaptureFifo Definition */
#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W0_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD0_BIT 0
#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W1_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD1_BIT 0
#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W2_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD2_BIT 0

#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W0_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD0_MASK 0xffffffff
#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W1_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD1_MASK 0xffffffff
#define CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_W2_CPU_MAC_UNIT_TX_TS_CAPTURE_FIFO_FIELD2_MASK 0x0000000f

/* ################################################################################
 * # CpuMacUnitTxCaptureTs Definition */
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC0_TX_SEQ_ID_BIT         1
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC0_TX_SFD_BIT            0
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC1_TX_SEQ_ID_BIT         5
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC1_TX_SFD_BIT            4
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W1_ADJ_RC_SECOND_BIT              0
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W2_ADJ_RC_NS_BIT                  0

#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC0_TX_SEQ_ID_MASK        0x00000006
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC0_TX_SFD_MASK           0x00000001
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC1_TX_SEQ_ID_MASK        0x00000060
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W0_CPU_MAC1_TX_SFD_MASK           0x00000010
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W1_ADJ_RC_SECOND_MASK             0xffffffff
#define CPU_MAC_UNIT_TX_CAPTURE_TS_W2_ADJ_RC_NS_MASK                 0x3fffffff

/* defing MDIOSOC_REG_BASE   0x00000000 */

struct MdioSoc_regs {
	u32 MdioSocCmd0[2];	/* 0x00000000 */
	u32 MdioSocCmd1[2];	/* 0x00000008 */
	u32 MdioSocStatus1;	/* 0x00000010 */
	u32 MdioSocStatus0;	/* 0x00000014 */
	u32 MdioSocReserved;	/* 0x00000018 */
	u32 MdioSocCfg0;	/* 0x0000001c */
	u32 MdioSocCfg1;	/* 0x00000020 */
};

/* ################################################################################
 * # MdioSocCmd0 Definition
 */
#define MDIO_SOC_CMD0_W0_DATA_CMD_LANE0                              0
#define MDIO_SOC_CMD0_W0_OP_CODE_CMD_LANE0                           26
#define MDIO_SOC_CMD0_W0_PHY_ADD_CMD_LANE0                           21
#define MDIO_SOC_CMD0_W0_REG_ADD_CMD_LANE0                           16
#define MDIO_SOC_CMD0_W1_START_CMD_LANE0                             0

#define MDIO_SOC_CMD0_W0_DATA_CMD_LANE0_MASK                         0x0000ffff
#define MDIO_SOC_CMD0_W0_OP_CODE_CMD_LANE0_MASK                      0x0c000000
#define MDIO_SOC_CMD0_W0_PHY_ADD_CMD_LANE0_MASK                      0x03e00000
#define MDIO_SOC_CMD0_W0_REG_ADD_CMD_LANE0_MASK                      0x001f0000
#define MDIO_SOC_CMD0_W1_START_CMD_LANE0_MASK                        0x00000003

/* ################################################################################
 * # MdioSocCmd1 Definition
 */
#define MDIO_SOC_CMD1_W0_DATA_CMD_LANE1                              0
#define MDIO_SOC_CMD1_W0_OP_CODE_CMD_LANE1                           26
#define MDIO_SOC_CMD1_W0_PHY_ADD_CMD_LANE1                           21
#define MDIO_SOC_CMD1_W0_REG_ADD_CMD_LANE1                           16
#define MDIO_SOC_CMD1_W1_START_CMD_LANE1                             0

#define MDIO_SOC_CMD1_W0_DATA_CMD_LANE1_MASK                         0x0000ffff
#define MDIO_SOC_CMD1_W0_OP_CODE_CMD_LANE1_MASK                      0x0c000000
#define MDIO_SOC_CMD1_W0_PHY_ADD_CMD_LANE1_MASK                      0x03e00000
#define MDIO_SOC_CMD1_W0_REG_ADD_CMD_LANE1_MASK                      0x001f0000
#define MDIO_SOC_CMD1_W1_START_CMD_LANE1_MASK                        0x00000003

/* ################################################################################
 * # MdioSocStatus1 Definition
 */
#define MDIO_SOC_STATUS1_W0_MDIO_CMD_DONE_LANE1                      16
#define MDIO_SOC_STATUS1_W0_MDIO_READ_DATA_LANE1                     0

#define MDIO_SOC_STATUS1_W0_MDIO_CMD_DONE_LANE1_MASK                 0x00010000
#define MDIO_SOC_STATUS1_W0_MDIO_READ_DATA_LANE1_MASK                0x0000ffff

/* ################################################################################
 * # MdioSocStatus0 Definition
 */
#define MDIO_SOC_STATUS0_W0_MDIO_CMD_DONE_LANE0                      16
#define MDIO_SOC_STATUS0_W0_MDIO_READ_DATA_LANE0                     0

#define MDIO_SOC_STATUS0_W0_MDIO_CMD_DONE_LANE0_MASK                 0x00010000
#define MDIO_SOC_STATUS0_W0_MDIO_READ_DATA_LANE0_MASK                0x0000ffff

/* ################################################################################
 * # MdioSocReserved Definition
 */
#define MDIO_SOC_RESERVED_W0_RESERVED                                0

#define MDIO_SOC_RESERVED_W0_RESERVED_MASK                           0x0000ffff

/* ################################################################################
 * # MdioSocCfg0 Definition
 */
#define MDIO_SOC_CFG0_W0_MDIO_IN_DLY_LANE0                           8
#define MDIO_SOC_CFG0_W0_MDIO_MAC_PRE_LANE0                          0

#define MDIO_SOC_CFG0_W0_MDIO_IN_DLY_LANE0_MASK                      0x00000f00
#define MDIO_SOC_CFG0_W0_MDIO_MAC_PRE_LANE0_MASK                     0x0000003f

/* ################################################################################
 * # MdioSocCfg1 Definition
 */
#define MDIO_SOC_CFG1_W0_MDIO_IN_DLY_LANE1                           8
#define MDIO_SOC_CFG1_W0_MDIO_MAC_PRE_LANE1                          0

#define MDIO_SOC_CFG1_W0_MDIO_IN_DLY_LANE1_MASK                      0x00000f00
#define MDIO_SOC_CFG1_W0_MDIO_MAC_PRE_LANE1_MASK                     0x0000003f

#define CTCMAC_MDIO_CMD_STCODE(V) (V<<0)
#define CTCMAC_MDIO_CMD_OPCODE(V) (V<<26)
#define CTCMAC_MDIO_CMD_PHYAD(V)  (V<<21)
#define CTCMAC_MDIO_CMD_REGAD(V)  (V<<16)
#define CTCMAC_MDIO_CMD_DATA(V)   (V<<0)

#define CTCMAC_MDIO_STAT(V)       (V<<16)

#endif
