/*! \file bcmcnet_cmicr_acc.h
 *
 * CMICr PDMA registers and descriptors access macros extracted from:
 *    bcmbd/include/bcmbd/bcmbd_cmicr_acc.h
 *    bcmbd/include/bcmbd/bcmbd_cmicr_intr.h
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef BCMCNET_CMICR_ACC_H
#define BCMCNET_CMICR_ACC_H

#define IPROC_IRQ_BASE5                 (5 * 32)
#define IPROC_IRQ_BASE6                 (6 * 32)
#define CMICR_IRQ_CMC0_PKTDMA_CH0_INTR          (IPROC_IRQ_BASE5 + 8)
#define CMICR_IRQ_CMC0_PKTDMA_CH1_INTR          (IPROC_IRQ_BASE5 + 9)
#define CMICR_IRQ_CMC0_PKTDMA_CH2_INTR          (IPROC_IRQ_BASE5 + 10)
#define CMICR_IRQ_CMC0_PKTDMA_CH3_INTR          (IPROC_IRQ_BASE5 + 11)
#define CMICR_IRQ_CMC0_PKTDMA_CH4_INTR          (IPROC_IRQ_BASE5 + 12)
#define CMICR_IRQ_CMC0_PKTDMA_CH5_INTR          (IPROC_IRQ_BASE5 + 13)
#define CMICR_IRQ_CMC0_PKTDMA_CH6_INTR          (IPROC_IRQ_BASE5 + 14)
#define CMICR_IRQ_CMC0_PKTDMA_CH7_INTR          (IPROC_IRQ_BASE5 + 15)
#define CMICR_IRQ_CMC0_PKTDMA_CH8_INTR          (IPROC_IRQ_BASE5 + 16)
#define CMICR_IRQ_CMC0_PKTDMA_CH9_INTR          (IPROC_IRQ_BASE5 + 17)
#define CMICR_IRQ_CMC0_PKTDMA_CH10_INTR         (IPROC_IRQ_BASE5 + 18)
#define CMICR_IRQ_CMC0_PKTDMA_CH11_INTR         (IPROC_IRQ_BASE5 + 19)
#define CMICR_IRQ_CMC0_PKTDMA_CH12_INTR         (IPROC_IRQ_BASE5 + 20)
#define CMICR_IRQ_CMC0_PKTDMA_CH13_INTR         (IPROC_IRQ_BASE5 + 21)
#define CMICR_IRQ_CMC0_PKTDMA_CH14_INTR         (IPROC_IRQ_BASE5 + 22)
#define CMICR_IRQ_CMC0_PKTDMA_CH15_INTR         (IPROC_IRQ_BASE5 + 23)
#define CMICR_IRQ_CMC1_PKTDMA_CH0_INTR          (IPROC_IRQ_BASE5 + 24)
#define CMICR_IRQ_CMC1_PKTDMA_CH1_INTR          (IPROC_IRQ_BASE5 + 25)
#define CMICR_IRQ_CMC1_PKTDMA_CH2_INTR          (IPROC_IRQ_BASE5 + 26)
#define CMICR_IRQ_CMC1_PKTDMA_CH3_INTR          (IPROC_IRQ_BASE5 + 27)
#define CMICR_IRQ_CMC1_PKTDMA_CH4_INTR          (IPROC_IRQ_BASE5 + 28)
#define CMICR_IRQ_CMC1_PKTDMA_CH5_INTR          (IPROC_IRQ_BASE5 + 29)
#define CMICR_IRQ_CMC1_PKTDMA_CH6_INTR          (IPROC_IRQ_BASE5 + 30)
#define CMICR_IRQ_CMC1_PKTDMA_CH7_INTR          (IPROC_IRQ_BASE5 + 31)
#define CMICR_IRQ_CMC1_PKTDMA_CH8_INTR          (IPROC_IRQ_BASE6 + 0)
#define CMICR_IRQ_CMC1_PKTDMA_CH9_INTR          (IPROC_IRQ_BASE6 + 1)
#define CMICR_IRQ_CMC1_PKTDMA_CH10_INTR         (IPROC_IRQ_BASE6 + 2)
#define CMICR_IRQ_CMC1_PKTDMA_CH11_INTR         (IPROC_IRQ_BASE6 + 3)
#define CMICR_IRQ_CMC1_PKTDMA_CH12_INTR         (IPROC_IRQ_BASE6 + 4)
#define CMICR_IRQ_CMC1_PKTDMA_CH13_INTR         (IPROC_IRQ_BASE6 + 5)
#define CMICR_IRQ_CMC1_PKTDMA_CH14_INTR         (IPROC_IRQ_BASE6 + 6)
#define CMICR_IRQ_CMC1_PKTDMA_CH15_INTR         (IPROC_IRQ_BASE6 + 7)

#define PAXB_0_INTC_SET_INTR_ENABLE_REG5r_OFFSET 0x0292d114
#define PAXB_0_INTC_SET_INTR_ENABLE_REG6r_OFFSET 0x0292d118
#define PAXB_0_INTC_CLEAR_INTR_ENABLE_REG5r_OFFSET 0x0292d13c
#define PAXB_0_INTC_CLEAR_INTR_ENABLE_REG6r_OFFSET 0x0292d140

/*
 * This structure should be used to declare and program CMIC_TOP_CONFIG.
 *
 */
typedef union CMIC_TOP_CONFIGr_s {
    uint32_t v[1];
    uint32_t cmic_top_config[1];
    uint32_t _cmic_top_config;
} CMIC_TOP_CONFIGr_t;

#define CMIC_TOP_CONFIGr_CLR(r) (r).cmic_top_config[0] = 0
#define CMIC_TOP_CONFIGr_SET(r,d) (r).cmic_top_config[0] = d
#define CMIC_TOP_CONFIGr_GET(r) (r).cmic_top_config[0]
#define CMIC_TOP_CONFIGr_OFFSET 0x0000000c
#define CMIC_TOP_CONFIGr_IP_2_EP_LOOPBACK_ENABLEf_GET(r) ((((r).cmic_top_config[0]) >> 1) & 0x1)
#define CMIC_TOP_CONFIGr_IP_2_EP_LOOPBACK_ENABLEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_TOP_CONFIGr_CMC0_CLK_ENf_GET(r) ((((r).cmic_top_config[0]) >> 2) & 0x1)
#define CMIC_TOP_CONFIGr_CMC0_CLK_ENf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_TOP_CONFIGr_CMC1_CLK_ENf_GET(r) ((((r).cmic_top_config[0]) >> 3) & 0x1)
#define CMIC_TOP_CONFIGr_CMC1_CLK_ENf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define CMIC_TOP_CONFIGr_COMMON_POOL_CLK_ENf_GET(r) ((((r).cmic_top_config[0]) >> 4) & 0x1)
#define CMIC_TOP_CONFIGr_COMMON_POOL_CLK_ENf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4))
#define CMIC_TOP_CONFIGr_RPE_CLK_ENf_GET(r) ((((r).cmic_top_config[0]) >> 5) & 0x1)
#define CMIC_TOP_CONFIGr_RPE_CLK_ENf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5))
#define CMIC_TOP_CONFIGr_IP_INTERFACE_PAYLOAD_ENDIANESSf_GET(r) ((((r).cmic_top_config[0]) >> 6) & 0x1)
#define CMIC_TOP_CONFIGr_IP_INTERFACE_PAYLOAD_ENDIANESSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6))
#define CMIC_TOP_CONFIGr_IP_INTERFACE_HEADER_ENDIANESSf_GET(r) ((((r).cmic_top_config[0]) >> 7) & 0x1)
#define CMIC_TOP_CONFIGr_IP_INTERFACE_HEADER_ENDIANESSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define CMIC_TOP_CONFIGr_EP_INTERFACE_PAYLOAD_ENDIANESSf_GET(r) ((((r).cmic_top_config[0]) >> 8) & 0x1)
#define CMIC_TOP_CONFIGr_EP_INTERFACE_PAYLOAD_ENDIANESSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8))
#define CMIC_TOP_CONFIGr_EP_INTERFACE_HEADER_ENDIANESSf_GET(r) ((((r).cmic_top_config[0]) >> 9) & 0x1)
#define CMIC_TOP_CONFIGr_EP_INTERFACE_HEADER_ENDIANESSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9))
#define CMIC_TOP_CONFIGr_CLEAR_ON_READ_ENABLEf_GET(r) ((((r).cmic_top_config[0]) >> 10) & 0x1)
#define CMIC_TOP_CONFIGr_CLEAR_ON_READ_ENABLEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10))
#define CMIC_TOP_CONFIGr_SATURATE_ENABLEf_GET(r) ((((r).cmic_top_config[0]) >> 11) & 0x1)
#define CMIC_TOP_CONFIGr_SATURATE_ENABLEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11))
#define CMIC_TOP_CONFIGr_CLEAR_CMC0_COUNTERSf_GET(r) ((((r).cmic_top_config[0]) >> 12) & 0x1)
#define CMIC_TOP_CONFIGr_CLEAR_CMC0_COUNTERSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12))
#define CMIC_TOP_CONFIGr_CLEAR_CMC1_COUNTERSf_GET(r) ((((r).cmic_top_config[0]) >> 13) & 0x1)
#define CMIC_TOP_CONFIGr_CLEAR_CMC1_COUNTERSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13))
#define CMIC_TOP_CONFIGr_CLEAR_RPE_COUNTERSf_GET(r) ((((r).cmic_top_config[0]) >> 14) & 0x1)
#define CMIC_TOP_CONFIGr_CLEAR_RPE_COUNTERSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14))
#define CMIC_TOP_CONFIGr_CLEAR_TOP_COUNTERSf_GET(r) ((((r).cmic_top_config[0]) >> 15) & 0x1)
#define CMIC_TOP_CONFIGr_CLEAR_TOP_COUNTERSf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15))
#define CMIC_TOP_CONFIGr_RPE_PIPE_MAPf_GET(r) ((((r).cmic_top_config[0]) >> 16) & 0x1)
#define CMIC_TOP_CONFIGr_RPE_PIPE_MAPf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 16)) | ((((uint32_t)f) & 0x1) << 16))
#define CMIC_TOP_CONFIGr_SBUS_RING_ARB_CUT_THROUGH_MODE_ENABLEf_GET(r) ((((r).cmic_top_config[0]) >> 17) & 0x1)
#define CMIC_TOP_CONFIGr_SBUS_RING_ARB_CUT_THROUGH_MODE_ENABLEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 17)) | ((((uint32_t)f) & 0x1) << 17))
#define CMIC_TOP_CONFIGr_IP_ARB_QUANTA_SELf_GET(r) ((((r).cmic_top_config[0]) >> 18) & 0x3)
#define CMIC_TOP_CONFIGr_IP_ARB_QUANTA_SELf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x3 << 18)) | ((((uint32_t)f) & 0x3) << 18))
#define CMIC_TOP_CONFIGr_ENABLE_CMIC_RST_AFTER_SW_IF_PURGEf_GET(r) ((((r).cmic_top_config[0]) >> 20) & 0x1)
#define CMIC_TOP_CONFIGr_ENABLE_CMIC_RST_AFTER_SW_IF_PURGEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 20)) | ((((uint32_t)f) & 0x1) << 20))
#define CMIC_TOP_CONFIGr_COS_MASK_OVERRIDEf_GET(r) ((((r).cmic_top_config[0]) >> 21) & 0x1)
#define CMIC_TOP_CONFIGr_COS_MASK_OVERRIDEf_SET(r,f) (r).cmic_top_config[0]=(((r).cmic_top_config[0] & ~((uint32_t)0x1 << 21)) | ((((uint32_t)f) & 0x1) << 21))
#define READ_CMIC_TOP_CONFIGr(u,r) BCMDRD_DEV_READ32(u,CMIC_TOP_CONFIGr_OFFSET,r._cmic_top_config)
#define WRITE_CMIC_TOP_CONFIGr(u,r) BCMDRD_DEV_WRITE32(u,CMIC_TOP_CONFIGr_OFFSET,r._cmic_top_config)

/*
 * This structure should be used to declare and program CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZE.
 *
 */
typedef union CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_s {
    uint32_t v[1];
    uint32_t cmic_top_status_ep_to_cpu_header_size[1];
    uint32_t _cmic_top_status_ep_to_cpu_header_size;
} CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_t;

#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_CLR(r) (r).cmic_top_status_ep_to_cpu_header_size[0] = 0
#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_SET(r,d) (r).cmic_top_status_ep_to_cpu_header_size[0] = d
#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_GET(r) (r).cmic_top_status_ep_to_cpu_header_size[0]
#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_OFFSET 0x00000004
#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_EP_TO_CPU_HEADER_SIZEf_GET(r) (((r).cmic_top_status_ep_to_cpu_header_size[0]) & 0xf)
#define CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_EP_TO_CPU_HEADER_SIZEf_SET(r,f) (r).cmic_top_status_ep_to_cpu_header_size[0]=(((r).cmic_top_status_ep_to_cpu_header_size[0] & ~((uint32_t)0xf)) | (((uint32_t)f) & 0xf))
#define READ_CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr(u,r) BCMDRD_DEV_READ32(u,CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_OFFSET,r._cmic_top_status_ep_to_cpu_header_size)
#define WRITE_CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr(u,r) BCMDRD_DEV_WRITE32(u,CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_OFFSET,r._cmic_top_status_ep_to_cpu_header_size)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_COS_CTRL_RX_0.
 *
 */
typedef union CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_cos_ctrl_rx_0[1];
    uint32_t _cmic_cmc_pktdma_cos_ctrl_rx_0;
} CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_t;

#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_CLR(r) (r).cmic_cmc_pktdma_cos_ctrl_rx_0[0] = 0
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_SET(r,d) (r).cmic_cmc_pktdma_cos_ctrl_rx_0[0] = d
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_GET(r) (r).cmic_cmc_pktdma_cos_ctrl_rx_0[0]
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_OFFSET 0x00003124
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_COS_BMPf_GET(r) ((r).cmic_cmc_pktdma_cos_ctrl_rx_0[0])
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_COS_BMPf_SET(r,f) (r).cmic_cmc_pktdma_cos_ctrl_rx_0[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_COS_CTRL_RX_0r(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_cos_ctrl_rx_0)
#define WRITE_CMIC_CMC_PKTDMA_COS_CTRL_RX_0r(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_cos_ctrl_rx_0)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_COS_CTRL_RX_1.
 *
 */
typedef union CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_cos_ctrl_rx_1[1];
    uint32_t _cmic_cmc_pktdma_cos_ctrl_rx_1;
} CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_t;

#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_CLR(r) (r).cmic_cmc_pktdma_cos_ctrl_rx_1[0] = 0
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_SET(r,d) (r).cmic_cmc_pktdma_cos_ctrl_rx_1[0] = d
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_GET(r) (r).cmic_cmc_pktdma_cos_ctrl_rx_1[0]
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_OFFSET 0x00003128
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_COS_BMPf_GET(r) ((r).cmic_cmc_pktdma_cos_ctrl_rx_1[0])
#define CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_COS_BMPf_SET(r,f) (r).cmic_cmc_pktdma_cos_ctrl_rx_1[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_COS_CTRL_RX_1r(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_cos_ctrl_rx_1)
#define WRITE_CMIC_CMC_PKTDMA_COS_CTRL_RX_1r(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_cos_ctrl_rx_1)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_CTRL.
 *
 */
typedef union CMIC_CMC_PKTDMA_CTRLr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_ctrl[1];
    uint32_t _cmic_cmc_pktdma_ctrl;
} CMIC_CMC_PKTDMA_CTRLr_t;

#define CMIC_CMC_PKTDMA_CTRLr_CLR(r) (r).cmic_cmc_pktdma_ctrl[0] = 0
#define CMIC_CMC_PKTDMA_CTRLr_SET(r,d) (r).cmic_cmc_pktdma_ctrl[0] = d
#define CMIC_CMC_PKTDMA_CTRLr_GET(r) (r).cmic_cmc_pktdma_ctrl[0]
#define CMIC_CMC_PKTDMA_CTRLr_OFFSET 0x00003100
#define CMIC_CMC_PKTDMA_CTRLr_DIRECTIONf_GET(r) (((r).cmic_cmc_pktdma_ctrl[0]) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DIRECTIONf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))
#define CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 1) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_CMC_PKTDMA_CTRLr_ABORT_DMAf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 2) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_ABORT_DMAf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_CMC_PKTDMA_CTRLr_PKTDMA_ENDIANESSf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 3) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_PKTDMA_ENDIANESSf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define CMIC_CMC_PKTDMA_CTRLr_DESC_ENDIANESSf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 4) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DESC_ENDIANESSf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4))
#define CMIC_CMC_PKTDMA_CTRLr_DROP_RX_PKT_ON_CHAIN_ENDf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 5) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DROP_RX_PKT_ON_CHAIN_ENDf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5))
#define CMIC_CMC_PKTDMA_CTRLr_RLD_STS_UPD_DISf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 6) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_RLD_STS_UPD_DISf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6))
#define CMIC_CMC_PKTDMA_CTRLr_DESC_DONE_INTR_MODEf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 7) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DESC_DONE_INTR_MODEf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define CMIC_CMC_PKTDMA_CTRLr_ENABLE_CONTINUOUS_DMAf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 8) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_ENABLE_CONTINUOUS_DMAf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8))
#define CMIC_CMC_PKTDMA_CTRLr_CONTIGUOUS_DESCRIPTORSf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 9) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_CONTIGUOUS_DESCRIPTORSf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9))
#define CMIC_CMC_PKTDMA_CTRLr_HEADER_ENDIANESSf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 12) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_HEADER_ENDIANESSf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12))
#define CMIC_CMC_PKTDMA_CTRLr_DISABLE_ABORT_ON_ERRORf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 13) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DISABLE_ABORT_ON_ERRORf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13))
#define CMIC_CMC_PKTDMA_CTRLr_PIPE_MAPf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 14) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_PIPE_MAPf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14))
#define CMIC_CMC_PKTDMA_CTRLr_DISABLE_DESC_OTDSTD_READf_GET(r) ((((r).cmic_cmc_pktdma_ctrl[0]) >> 15) & 0x1)
#define CMIC_CMC_PKTDMA_CTRLr_DISABLE_DESC_OTDSTD_READf_SET(r,f) (r).cmic_cmc_pktdma_ctrl[0]=(((r).cmic_cmc_pktdma_ctrl[0] & ~((uint32_t)0x1 << 15)) | ((((uint32_t)f) & 0x1) << 15))
#define READ_CMIC_CMC_PKTDMA_CTRLr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_CTRLr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_ctrl)
#define WRITE_CMIC_CMC_PKTDMA_CTRLr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_CTRLr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_ctrl)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_CURR_DESC_HI.
 *
 */
typedef union CMIC_CMC_PKTDMA_CURR_DESC_HIr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_curr_desc_hi[1];
    uint32_t _cmic_cmc_pktdma_curr_desc_hi;
} CMIC_CMC_PKTDMA_CURR_DESC_HIr_t;

#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_CLR(r) (r).cmic_cmc_pktdma_curr_desc_hi[0] = 0
#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_SET(r,d) (r).cmic_cmc_pktdma_curr_desc_hi[0] = d
#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_GET(r) (r).cmic_cmc_pktdma_curr_desc_hi[0]
#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_OFFSET 0x00003134
#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_curr_desc_hi[0])
#define CMIC_CMC_PKTDMA_CURR_DESC_HIr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_curr_desc_hi[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_CURR_DESC_HIr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_CURR_DESC_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_curr_desc_hi)
#define WRITE_CMIC_CMC_PKTDMA_CURR_DESC_HIr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_CURR_DESC_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_curr_desc_hi)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_CURR_DESC_LO.
 *
 */
typedef union CMIC_CMC_PKTDMA_CURR_DESC_LOr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_curr_desc_lo[1];
    uint32_t _cmic_cmc_pktdma_curr_desc_lo;
} CMIC_CMC_PKTDMA_CURR_DESC_LOr_t;

#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_CLR(r) (r).cmic_cmc_pktdma_curr_desc_lo[0] = 0
#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_SET(r,d) (r).cmic_cmc_pktdma_curr_desc_lo[0] = d
#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_GET(r) (r).cmic_cmc_pktdma_curr_desc_lo[0]
#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_OFFSET 0x00003130
#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_curr_desc_lo[0])
#define CMIC_CMC_PKTDMA_CURR_DESC_LOr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_curr_desc_lo[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_CURR_DESC_LOr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_CURR_DESC_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_curr_desc_lo)
#define WRITE_CMIC_CMC_PKTDMA_CURR_DESC_LOr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_CURR_DESC_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_curr_desc_lo)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DEBUG_CONTROL.
 *
 */
typedef union CMIC_CMC_PKTDMA_DEBUG_CONTROLr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_debug_control[1];
    uint32_t _cmic_cmc_pktdma_debug_control;
} CMIC_CMC_PKTDMA_DEBUG_CONTROLr_t;

#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_CLR(r) (r).cmic_cmc_pktdma_debug_control[0] = 0
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_SET(r,d) (r).cmic_cmc_pktdma_debug_control[0] = d
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_GET(r) (r).cmic_cmc_pktdma_debug_control[0]
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_OFFSET 0x0000313c
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_DESC_PENDING_SER_Q_SIZEf_GET(r) (((r).cmic_cmc_pktdma_debug_control[0]) & 0x7)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_DESC_PENDING_SER_Q_SIZEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x7)) | (((uint32_t)f) & 0x7))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_DESC_PENDING_WRITE_Q_SIZEf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 3) & 0x7)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_DESC_PENDING_WRITE_Q_SIZEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x7 << 3)) | ((((uint32_t)f) & 0x7) << 3))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_FLUSH_DESC_Qf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 6) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_FLUSH_DESC_Qf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_OUTSTANDING_CNTf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 7) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_OUTSTANDING_CNTf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_SM_SELECTf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 8) & 0x3)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_SM_SELECTf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x3 << 8)) | ((((uint32_t)f) & 0x3) << 8))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DESC_READ_SM_IDLEf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 10) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DESC_READ_SM_IDLEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_MEM_RDWR_SM_IDLEf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 11) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_MEM_RDWR_SM_IDLEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DESC_STATUS_WR_SM_IDLEf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 12) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DESC_STATUS_WR_SM_IDLEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12))
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DYN_RCNFG_SM_IDLEf_GET(r) ((((r).cmic_cmc_pktdma_debug_control[0]) >> 13) & 0x1)
#define CMIC_CMC_PKTDMA_DEBUG_CONTROLr_RESET_DYN_RCNFG_SM_IDLEf_SET(r,f) (r).cmic_cmc_pktdma_debug_control[0]=(((r).cmic_cmc_pktdma_debug_control[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13))
#define READ_CMIC_CMC_PKTDMA_DEBUG_CONTROLr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DEBUG_CONTROLr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_control)
#define WRITE_CMIC_CMC_PKTDMA_DEBUG_CONTROLr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DEBUG_CONTROLr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_control)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DEBUG_SM_STATUS.
 *
 */
typedef union CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_debug_sm_status[1];
    uint32_t _cmic_cmc_pktdma_debug_sm_status;
} CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_t;

#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_CLR(r) (r).cmic_cmc_pktdma_debug_sm_status[0] = 0
#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_SET(r,d) (r).cmic_cmc_pktdma_debug_sm_status[0] = d
#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_GET(r) (r).cmic_cmc_pktdma_debug_sm_status[0]
#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_OFFSET 0x00003140
#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_STATEf_GET(r) ((r).cmic_cmc_pktdma_debug_sm_status[0])
#define CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_STATEf_SET(r,f) (r).cmic_cmc_pktdma_debug_sm_status[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_sm_status)
#define WRITE_CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_sm_status)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DEBUG_STATUS.
 *
 */
typedef union CMIC_CMC_PKTDMA_DEBUG_STATUSr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_debug_status[1];
    uint32_t _cmic_cmc_pktdma_debug_status;
} CMIC_CMC_PKTDMA_DEBUG_STATUSr_t;

#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_CLR(r) (r).cmic_cmc_pktdma_debug_status[0] = 0
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_SET(r,d) (r).cmic_cmc_pktdma_debug_status[0] = d
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_GET(r) (r).cmic_cmc_pktdma_debug_status[0]
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_OFFSET 0x00003144
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_PENDING_SER_Q_SIZE_NUM_ENTRIESf_GET(r) (((r).cmic_cmc_pktdma_debug_status[0]) & 0x3f)
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_PENDING_SER_Q_SIZE_NUM_ENTRIESf_SET(r,f) (r).cmic_cmc_pktdma_debug_status[0]=(((r).cmic_cmc_pktdma_debug_status[0] & ~((uint32_t)0x3f)) | (((uint32_t)f) & 0x3f))
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_PENDING_WRITE_Q_NUM_ENTRIESf_GET(r) ((((r).cmic_cmc_pktdma_debug_status[0]) >> 6) & 0x7f)
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_PENDING_WRITE_Q_NUM_ENTRIESf_SET(r,f) (r).cmic_cmc_pktdma_debug_status[0]=(((r).cmic_cmc_pktdma_debug_status[0] & ~((uint32_t)0x7f << 6)) | ((((uint32_t)f) & 0x7f) << 6))
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_NUM_TXPKTBUF_CELL_USEDf_GET(r) ((((r).cmic_cmc_pktdma_debug_status[0]) >> 13) & 0x7f)
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_NUM_TXPKTBUF_CELL_USEDf_SET(r,f) (r).cmic_cmc_pktdma_debug_status[0]=(((r).cmic_cmc_pktdma_debug_status[0] & ~((uint32_t)0x7f << 13)) | ((((uint32_t)f) & 0x7f) << 13))
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_READ_OUTSTD_CNTf_GET(r) ((((r).cmic_cmc_pktdma_debug_status[0]) >> 20) & 0x1f)
#define CMIC_CMC_PKTDMA_DEBUG_STATUSr_DESC_READ_OUTSTD_CNTf_SET(r,f) (r).cmic_cmc_pktdma_debug_status[0]=(((r).cmic_cmc_pktdma_debug_status[0] & ~((uint32_t)0x1f << 20)) | ((((uint32_t)f) & 0x1f) << 20))
#define READ_CMIC_CMC_PKTDMA_DEBUG_STATUSr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DEBUG_STATUSr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_status)
#define WRITE_CMIC_CMC_PKTDMA_DEBUG_STATUSr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DEBUG_STATUSr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_debug_status)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_ADDR_HI.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_ADDR_HIr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_addr_hi[1];
    uint32_t _cmic_cmc_pktdma_desc_addr_hi;
} CMIC_CMC_PKTDMA_DESC_ADDR_HIr_t;

#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_CLR(r) (r).cmic_cmc_pktdma_desc_addr_hi[0] = 0
#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_SET(r,d) (r).cmic_cmc_pktdma_desc_addr_hi[0] = d
#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_GET(r) (r).cmic_cmc_pktdma_desc_addr_hi[0]
#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_OFFSET 0x00003108
#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_desc_addr_hi[0])
#define CMIC_CMC_PKTDMA_DESC_ADDR_HIr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_desc_addr_hi[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_ADDR_HIr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_ADDR_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_addr_hi)
#define WRITE_CMIC_CMC_PKTDMA_DESC_ADDR_HIr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_ADDR_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_addr_hi)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_ADDR_LO.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_ADDR_LOr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_addr_lo[1];
    uint32_t _cmic_cmc_pktdma_desc_addr_lo;
} CMIC_CMC_PKTDMA_DESC_ADDR_LOr_t;

#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_CLR(r) (r).cmic_cmc_pktdma_desc_addr_lo[0] = 0
#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_SET(r,d) (r).cmic_cmc_pktdma_desc_addr_lo[0] = d
#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_GET(r) (r).cmic_cmc_pktdma_desc_addr_lo[0]
#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_OFFSET 0x00003104
#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_desc_addr_lo[0])
#define CMIC_CMC_PKTDMA_DESC_ADDR_LOr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_desc_addr_lo[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_ADDR_LOr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_ADDR_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_addr_lo)
#define WRITE_CMIC_CMC_PKTDMA_DESC_ADDR_LOr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_ADDR_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_addr_lo)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_COUNT_REQ.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_COUNT_REQr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_count_req[1];
    uint32_t _cmic_cmc_pktdma_desc_count_req;
} CMIC_CMC_PKTDMA_DESC_COUNT_REQr_t;

#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_CLR(r) (r).cmic_cmc_pktdma_desc_count_req[0] = 0
#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_SET(r,d) (r).cmic_cmc_pktdma_desc_count_req[0] = d
#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_GET(r) (r).cmic_cmc_pktdma_desc_count_req[0]
#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_OFFSET 0x00003154
#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_desc_count_req[0])
#define CMIC_CMC_PKTDMA_DESC_COUNT_REQr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_desc_count_req[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_COUNT_REQr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_COUNT_REQr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_req)
#define WRITE_CMIC_CMC_PKTDMA_DESC_COUNT_REQr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_COUNT_REQr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_req)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_COUNT_RX.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_COUNT_RXr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_count_rx[1];
    uint32_t _cmic_cmc_pktdma_desc_count_rx;
} CMIC_CMC_PKTDMA_DESC_COUNT_RXr_t;

#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_CLR(r) (r).cmic_cmc_pktdma_desc_count_rx[0] = 0
#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_SET(r,d) (r).cmic_cmc_pktdma_desc_count_rx[0] = d
#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_GET(r) (r).cmic_cmc_pktdma_desc_count_rx[0]
#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_OFFSET 0x00003158
#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_desc_count_rx[0])
#define CMIC_CMC_PKTDMA_DESC_COUNT_RXr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_desc_count_rx[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_COUNT_RXr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_COUNT_RXr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_rx)
#define WRITE_CMIC_CMC_PKTDMA_DESC_COUNT_RXr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_COUNT_RXr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_rx)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WR.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_count_status_wr[1];
    uint32_t _cmic_cmc_pktdma_desc_count_status_wr;
} CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_t;

#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_CLR(r) (r).cmic_cmc_pktdma_desc_count_status_wr[0] = 0
#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_SET(r,d) (r).cmic_cmc_pktdma_desc_count_status_wr[0] = d
#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_GET(r) (r).cmic_cmc_pktdma_desc_count_status_wr[0]
#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_OFFSET 0x0000315c
#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_desc_count_status_wr[0])
#define CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_desc_count_status_wr[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_status_wr)
#define WRITE_CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_count_status_wr)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HI.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_halt_addr_hi[1];
    uint32_t _cmic_cmc_pktdma_desc_halt_addr_hi;
} CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_t;

#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_CLR(r) (r).cmic_cmc_pktdma_desc_halt_addr_hi[0] = 0
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_SET(r,d) (r).cmic_cmc_pktdma_desc_halt_addr_hi[0] = d
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_GET(r) (r).cmic_cmc_pktdma_desc_halt_addr_hi[0]
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_OFFSET 0x00003110
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_desc_halt_addr_hi[0])
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_desc_halt_addr_hi[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_halt_addr_hi)
#define WRITE_CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_halt_addr_hi)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LO.
 *
 */
typedef union CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_desc_halt_addr_lo[1];
    uint32_t _cmic_cmc_pktdma_desc_halt_addr_lo;
} CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_t;

#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_CLR(r) (r).cmic_cmc_pktdma_desc_halt_addr_lo[0] = 0
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_SET(r,d) (r).cmic_cmc_pktdma_desc_halt_addr_lo[0] = d
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_GET(r) (r).cmic_cmc_pktdma_desc_halt_addr_lo[0]
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_OFFSET 0x0000310c
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_ADDRf_GET(r) ((r).cmic_cmc_pktdma_desc_halt_addr_lo[0])
#define CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_ADDRf_SET(r,f) (r).cmic_cmc_pktdma_desc_halt_addr_lo[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_halt_addr_lo)
#define WRITE_CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_desc_halt_addr_lo)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_INTR.
 *
 */
typedef union CMIC_CMC_PKTDMA_INTRr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_intr[1];
    uint32_t _cmic_cmc_pktdma_intr;
} CMIC_CMC_PKTDMA_INTRr_t;

#define CMIC_CMC_PKTDMA_INTRr_CLR(r) (r).cmic_cmc_pktdma_intr[0] = 0
#define CMIC_CMC_PKTDMA_INTRr_SET(r,d) (r).cmic_cmc_pktdma_intr[0] = d
#define CMIC_CMC_PKTDMA_INTRr_GET(r) (r).cmic_cmc_pktdma_intr[0]
#define CMIC_CMC_PKTDMA_INTRr_OFFSET 0x00003118
#define CMIC_CMC_PKTDMA_INTRr_CHAIN_DONE_INTRf_GET(r) (((r).cmic_cmc_pktdma_intr[0]) & 0x1)
#define CMIC_CMC_PKTDMA_INTRr_CHAIN_DONE_INTRf_SET(r,f) (r).cmic_cmc_pktdma_intr[0]=(((r).cmic_cmc_pktdma_intr[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))
#define CMIC_CMC_PKTDMA_INTRr_DESC_DONE_INTRf_GET(r) ((((r).cmic_cmc_pktdma_intr[0]) >> 1) & 0x1)
#define CMIC_CMC_PKTDMA_INTRr_DESC_DONE_INTRf_SET(r,f) (r).cmic_cmc_pktdma_intr[0]=(((r).cmic_cmc_pktdma_intr[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_CMC_PKTDMA_INTRr_DESC_CONTROLLED_INTRf_GET(r) ((((r).cmic_cmc_pktdma_intr[0]) >> 2) & 0x1)
#define CMIC_CMC_PKTDMA_INTRr_DESC_CONTROLLED_INTRf_SET(r,f) (r).cmic_cmc_pktdma_intr[0]=(((r).cmic_cmc_pktdma_intr[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_CMC_PKTDMA_INTRr_INTR_COALESCING_INTRf_GET(r) ((((r).cmic_cmc_pktdma_intr[0]) >> 3) & 0x1)
#define CMIC_CMC_PKTDMA_INTRr_INTR_COALESCING_INTRf_SET(r,f) (r).cmic_cmc_pktdma_intr[0]=(((r).cmic_cmc_pktdma_intr[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define READ_CMIC_CMC_PKTDMA_INTRr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_INTRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr)
#define WRITE_CMIC_CMC_PKTDMA_INTRr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_INTRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_INTR_CLR.
 *
 */
typedef union CMIC_CMC_PKTDMA_INTR_CLRr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_intr_clr[1];
    uint32_t _cmic_cmc_pktdma_intr_clr;
} CMIC_CMC_PKTDMA_INTR_CLRr_t;

#define CMIC_CMC_PKTDMA_INTR_CLRr_CLR(r) (r).cmic_cmc_pktdma_intr_clr[0] = 0
#define CMIC_CMC_PKTDMA_INTR_CLRr_SET(r,d) (r).cmic_cmc_pktdma_intr_clr[0] = d
#define CMIC_CMC_PKTDMA_INTR_CLRr_GET(r) (r).cmic_cmc_pktdma_intr_clr[0]
#define CMIC_CMC_PKTDMA_INTR_CLRr_OFFSET 0x00003120
#define CMIC_CMC_PKTDMA_INTR_CLRr_DESC_DONE_INTR_CLRf_GET(r) ((((r).cmic_cmc_pktdma_intr_clr[0]) >> 1) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_CLRr_DESC_DONE_INTR_CLRf_SET(r,f) (r).cmic_cmc_pktdma_intr_clr[0]=(((r).cmic_cmc_pktdma_intr_clr[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_CMC_PKTDMA_INTR_CLRr_DESC_CONTROLLED_INTR_CLRf_GET(r) ((((r).cmic_cmc_pktdma_intr_clr[0]) >> 2) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_CLRr_DESC_CONTROLLED_INTR_CLRf_SET(r,f) (r).cmic_cmc_pktdma_intr_clr[0]=(((r).cmic_cmc_pktdma_intr_clr[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_CMC_PKTDMA_INTR_CLRr_INTR_COALESCING_INTR_CLRf_GET(r) ((((r).cmic_cmc_pktdma_intr_clr[0]) >> 3) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_CLRr_INTR_COALESCING_INTR_CLRf_SET(r,f) (r).cmic_cmc_pktdma_intr_clr[0]=(((r).cmic_cmc_pktdma_intr_clr[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define CMIC_CMC_PKTDMA_INTR_CLRr_DYN_RCNFG_ERR_CLRf_GET(r) ((((r).cmic_cmc_pktdma_intr_clr[0]) >> 4) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_CLRr_DYN_RCNFG_ERR_CLRf_SET(r,f) (r).cmic_cmc_pktdma_intr_clr[0]=(((r).cmic_cmc_pktdma_intr_clr[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4))
#define READ_CMIC_CMC_PKTDMA_INTR_CLRr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_INTR_CLRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_clr)
#define WRITE_CMIC_CMC_PKTDMA_INTR_CLRr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_INTR_CLRr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_clr)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_INTR_COAL.
 *
 */
typedef union CMIC_CMC_PKTDMA_INTR_COALr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_intr_coal[1];
    uint32_t _cmic_cmc_pktdma_intr_coal;
} CMIC_CMC_PKTDMA_INTR_COALr_t;

#define CMIC_CMC_PKTDMA_INTR_COALr_CLR(r) (r).cmic_cmc_pktdma_intr_coal[0] = 0
#define CMIC_CMC_PKTDMA_INTR_COALr_SET(r,d) (r).cmic_cmc_pktdma_intr_coal[0] = d
#define CMIC_CMC_PKTDMA_INTR_COALr_GET(r) (r).cmic_cmc_pktdma_intr_coal[0]
#define CMIC_CMC_PKTDMA_INTR_COALr_OFFSET 0x0000312c
#define CMIC_CMC_PKTDMA_INTR_COALr_TIMERf_GET(r) (((r).cmic_cmc_pktdma_intr_coal[0]) & 0xffff)
#define CMIC_CMC_PKTDMA_INTR_COALr_TIMERf_SET(r,f) (r).cmic_cmc_pktdma_intr_coal[0]=(((r).cmic_cmc_pktdma_intr_coal[0] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff))
#define CMIC_CMC_PKTDMA_INTR_COALr_COUNTf_GET(r) ((((r).cmic_cmc_pktdma_intr_coal[0]) >> 16) & 0x7fff)
#define CMIC_CMC_PKTDMA_INTR_COALr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_intr_coal[0]=(((r).cmic_cmc_pktdma_intr_coal[0] & ~((uint32_t)0x7fff << 16)) | ((((uint32_t)f) & 0x7fff) << 16))
#define CMIC_CMC_PKTDMA_INTR_COALr_ENABLEf_GET(r) ((((r).cmic_cmc_pktdma_intr_coal[0]) >> 31) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_COALr_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_intr_coal[0]=(((r).cmic_cmc_pktdma_intr_coal[0] & ~((uint32_t)0x1 << 31)) | ((((uint32_t)f) & 0x1) << 31))
#define READ_CMIC_CMC_PKTDMA_INTR_COALr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_INTR_COALr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_coal)
#define WRITE_CMIC_CMC_PKTDMA_INTR_COALr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_INTR_COALr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_coal)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_INTR_ENABLE.
 *
 */
typedef union CMIC_CMC_PKTDMA_INTR_ENABLEr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_intr_enable[1];
    uint32_t _cmic_cmc_pktdma_intr_enable;
} CMIC_CMC_PKTDMA_INTR_ENABLEr_t;

#define CMIC_CMC_PKTDMA_INTR_ENABLEr_CLR(r) (r).cmic_cmc_pktdma_intr_enable[0] = 0
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_SET(r,d) (r).cmic_cmc_pktdma_intr_enable[0] = d
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_GET(r) (r).cmic_cmc_pktdma_intr_enable[0]
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_OFFSET 0x0000311c
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_CHAIN_DONE_INTR_ENABLEf_GET(r) (((r).cmic_cmc_pktdma_intr_enable[0]) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_CHAIN_DONE_INTR_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_intr_enable[0]=(((r).cmic_cmc_pktdma_intr_enable[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_DESC_DONE_INTR_ENABLEf_GET(r) ((((r).cmic_cmc_pktdma_intr_enable[0]) >> 1) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_DESC_DONE_INTR_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_intr_enable[0]=(((r).cmic_cmc_pktdma_intr_enable[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_DESC_CONTROLLED_INTR_ENABLEf_GET(r) ((((r).cmic_cmc_pktdma_intr_enable[0]) >> 2) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_DESC_CONTROLLED_INTR_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_intr_enable[0]=(((r).cmic_cmc_pktdma_intr_enable[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_INTR_COALESCING_INTR_ENABLEf_GET(r) ((((r).cmic_cmc_pktdma_intr_enable[0]) >> 3) & 0x1)
#define CMIC_CMC_PKTDMA_INTR_ENABLEr_INTR_COALESCING_INTR_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_intr_enable[0]=(((r).cmic_cmc_pktdma_intr_enable[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define READ_CMIC_CMC_PKTDMA_INTR_ENABLEr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_INTR_ENABLEr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_enable)
#define WRITE_CMIC_CMC_PKTDMA_INTR_ENABLEr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_INTR_ENABLEr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_intr_enable)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT.
 *
 */
typedef union CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_pkt_count_rxpkt[1];
    uint32_t _cmic_cmc_pktdma_pkt_count_rxpkt;
} CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_t;

#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_CLR(r) (r).cmic_cmc_pktdma_pkt_count_rxpkt[0] = 0
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_SET(r,d) (r).cmic_cmc_pktdma_pkt_count_rxpkt[0] = d
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_GET(r) (r).cmic_cmc_pktdma_pkt_count_rxpkt[0]
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_OFFSET 0x00003148
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_pkt_count_rxpkt[0])
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_pkt_count_rxpkt[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_rxpkt)
#define WRITE_CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_rxpkt)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROP.
 *
 */
typedef union CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_pkt_count_rxpkt_drop[1];
    uint32_t _cmic_cmc_pktdma_pkt_count_rxpkt_drop;
} CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_t;

#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_CLR(r) (r).cmic_cmc_pktdma_pkt_count_rxpkt_drop[0] = 0
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_SET(r,d) (r).cmic_cmc_pktdma_pkt_count_rxpkt_drop[0] = d
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_GET(r) (r).cmic_cmc_pktdma_pkt_count_rxpkt_drop[0]
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_OFFSET 0x00003150
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_pkt_count_rxpkt_drop[0])
#define CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_pkt_count_rxpkt_drop[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_rxpkt_drop)
#define WRITE_CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_rxpkt_drop)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_PKT_COUNT_TXPKT.
 *
 */
typedef union CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_pkt_count_txpkt[1];
    uint32_t _cmic_cmc_pktdma_pkt_count_txpkt;
} CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_t;

#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_CLR(r) (r).cmic_cmc_pktdma_pkt_count_txpkt[0] = 0
#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_SET(r,d) (r).cmic_cmc_pktdma_pkt_count_txpkt[0] = d
#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_GET(r) (r).cmic_cmc_pktdma_pkt_count_txpkt[0]
#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_OFFSET 0x0000314c
#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_COUNTf_GET(r) ((r).cmic_cmc_pktdma_pkt_count_txpkt[0])
#define CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_COUNTf_SET(r,f) (r).cmic_cmc_pktdma_pkt_count_txpkt[0]=((uint32_t)f)
#define READ_CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_txpkt)
#define WRITE_CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_pkt_count_txpkt)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIG.
 *
 */
typedef union CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_rxbuf_threshold_config[1];
    uint32_t _cmic_cmc_pktdma_rxbuf_threshold_config;
} CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_t;

#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_CLR(r) (r).cmic_cmc_pktdma_rxbuf_threshold_config[0] = 0
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_SET(r,d) (r).cmic_cmc_pktdma_rxbuf_threshold_config[0] = d
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_GET(r) (r).cmic_cmc_pktdma_rxbuf_threshold_config[0]
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_OFFSET 0x00003138
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_RXBUF_THRESHOLDf_GET(r) (((r).cmic_cmc_pktdma_rxbuf_threshold_config[0]) & 0x1ff)
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_RXBUF_THRESHOLDf_SET(r,f) (r).cmic_cmc_pktdma_rxbuf_threshold_config[0]=(((r).cmic_cmc_pktdma_rxbuf_threshold_config[0] & ~((uint32_t)0x1ff)) | (((uint32_t)f) & 0x1ff))
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_ENABLEf_GET(r) ((((r).cmic_cmc_pktdma_rxbuf_threshold_config[0]) >> 9) & 0x1)
#define CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_ENABLEf_SET(r,f) (r).cmic_cmc_pktdma_rxbuf_threshold_config[0]=(((r).cmic_cmc_pktdma_rxbuf_threshold_config[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9))
#define READ_CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_rxbuf_threshold_config)
#define WRITE_CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_rxbuf_threshold_config)

/*
 * This structure should be used to declare and program CMIC_CMC_PKTDMA_STAT.
 *
 */
typedef union CMIC_CMC_PKTDMA_STATr_s {
    uint32_t v[1];
    uint32_t cmic_cmc_pktdma_stat[1];
    uint32_t _cmic_cmc_pktdma_stat;
} CMIC_CMC_PKTDMA_STATr_t;

#define CMIC_CMC_PKTDMA_STATr_CLR(r) (r).cmic_cmc_pktdma_stat[0] = 0
#define CMIC_CMC_PKTDMA_STATr_SET(r,d) (r).cmic_cmc_pktdma_stat[0] = d
#define CMIC_CMC_PKTDMA_STATr_GET(r) (r).cmic_cmc_pktdma_stat[0]
#define CMIC_CMC_PKTDMA_STATr_OFFSET 0x00003114
#define CMIC_CMC_PKTDMA_STATr_CHAIN_DONEf_GET(r) (((r).cmic_cmc_pktdma_stat[0]) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_CHAIN_DONEf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1)) | (((uint32_t)f) & 0x1))
#define CMIC_CMC_PKTDMA_STATr_DMA_ACTIVEf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 1) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DMA_ACTIVEf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 1)) | ((((uint32_t)f) & 0x1) << 1))
#define CMIC_CMC_PKTDMA_STATr_STWT_ADDR_DECODE_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 2) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_STWT_ADDR_DECODE_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 2)) | ((((uint32_t)f) & 0x1) << 2))
#define CMIC_CMC_PKTDMA_STATr_PKTWRRD_ADDR_DECODE_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 3) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_PKTWRRD_ADDR_DECODE_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 3)) | ((((uint32_t)f) & 0x1) << 3))
#define CMIC_CMC_PKTDMA_STATr_DESCRD_ADDR_DECODE_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 4) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DESCRD_ADDR_DECODE_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 4)) | ((((uint32_t)f) & 0x1) << 4))
#define CMIC_CMC_PKTDMA_STATr_PKTWR_ECC_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 5) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_PKTWR_ECC_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 5)) | ((((uint32_t)f) & 0x1) << 5))
#define CMIC_CMC_PKTDMA_STATr_CH_IN_HALTf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 6) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_CH_IN_HALTf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 6)) | ((((uint32_t)f) & 0x1) << 6))
#define CMIC_CMC_PKTDMA_STATr_RELOAD_UNALIGNED_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 7) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_RELOAD_UNALIGNED_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 7)) | ((((uint32_t)f) & 0x1) << 7))
#define CMIC_CMC_PKTDMA_STATr_DESC_DONEf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 8) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DESC_DONEf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 8)) | ((((uint32_t)f) & 0x1) << 8))
#define CMIC_CMC_PKTDMA_STATr_DESC_CONTROLLEDf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 9) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DESC_CONTROLLEDf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 9)) | ((((uint32_t)f) & 0x1) << 9))
#define CMIC_CMC_PKTDMA_STATr_INTR_COALESCINGf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 10) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_INTR_COALESCINGf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 10)) | ((((uint32_t)f) & 0x1) << 10))
#define CMIC_CMC_PKTDMA_STATr_DYN_RCNFG_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 11) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DYN_RCNFG_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 11)) | ((((uint32_t)f) & 0x1) << 11))
#define CMIC_CMC_PKTDMA_STATr_INVALID_AXI_CMD_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 12) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_INVALID_AXI_CMD_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 12)) | ((((uint32_t)f) & 0x1) << 12))
#define CMIC_CMC_PKTDMA_STATr_DMA_IS_ENf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 13) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DMA_IS_ENf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 13)) | ((((uint32_t)f) & 0x1) << 13))
#define CMIC_CMC_PKTDMA_STATr_DESC_MEM_ECC_ERRf_GET(r) ((((r).cmic_cmc_pktdma_stat[0]) >> 14) & 0x1)
#define CMIC_CMC_PKTDMA_STATr_DESC_MEM_ECC_ERRf_SET(r,f) (r).cmic_cmc_pktdma_stat[0]=(((r).cmic_cmc_pktdma_stat[0] & ~((uint32_t)0x1 << 14)) | ((((uint32_t)f) & 0x1) << 14))
#define READ_CMIC_CMC_PKTDMA_STATr(u,_cmc,_ch,r) BCMDRD_DEV_READ32(u,CMIC_CMC_PKTDMA_STATr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_stat)
#define WRITE_CMIC_CMC_PKTDMA_STATr(u,_cmc,_ch,r) BCMDRD_DEV_WRITE32(u,CMIC_CMC_PKTDMA_STATr_OFFSET+(0x80*(_ch))+(0x2000*(_cmc)),r._cmic_cmc_pktdma_stat)

#define RX_DCB_SIZE 16
#define TX_DCB_SIZE 16

/*
 * This structure should be used to declare and program TX_DCB.
 *
 */
typedef union TX_DCB_s {
    uint32_t v[4];
    uint32_t tx_dcb[4];
    uint32_t _tx_dcb;
} TX_DCB_t;

#define TX_DCB_CLR(r) sal_memset(&((r).tx_dcb[0]), 0, sizeof(TX_DCB_t))
#define TX_DCB_SET(r,i,d) (r).tx_dcb[i] = d
#define TX_DCB_GET(r,i) (r).tx_dcb[i]
#define TX_DCB_OFFSET 0x00000000
#define TX_DCB_BYTES_TRANSFERREDf_GET(r) (((r).tx_dcb[3]) & 0xffff)
#define TX_DCB_BYTES_TRANSFERREDf_SET(r,f) (r).tx_dcb[3]=(((r).tx_dcb[3] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff))
#define TX_DCB_DONEf_GET(r) ((((r).tx_dcb[3]) >> 31) & 0x1)
#define TX_DCB_DONEf_SET(r,f) (r).tx_dcb[3]=(((r).tx_dcb[3] & ~((uint32_t)0x1 << 31)) | ((((uint32_t)f) & 0x1) << 31))
#define TX_DCB_BYTE_COUNTf_GET(r) (((r).tx_dcb[2]) & 0xffff)
#define TX_DCB_BYTE_COUNTf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff))
#define TX_DCB_CHAINf_GET(r) ((((r).tx_dcb[2]) >> 16) & 0x1)
#define TX_DCB_CHAINf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 16)) | ((((uint32_t)f) & 0x1) << 16))
#define TX_DCB_SGf_GET(r) ((((r).tx_dcb[2]) >> 17) & 0x1)
#define TX_DCB_SGf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 17)) | ((((uint32_t)f) & 0x1) << 17))
#define TX_DCB_RELOADf_GET(r) ((((r).tx_dcb[2]) >> 18) & 0x1)
#define TX_DCB_RELOADf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 18)) | ((((uint32_t)f) & 0x1) << 18))
#define TX_DCB_HGf_GET(r) ((((r).tx_dcb[2]) >> 19) & 0x1)
#define TX_DCB_HGf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 19)) | ((((uint32_t)f) & 0x1) << 19))
#define TX_DCB_ALIGN_BYTESf_GET(r) ((((r).tx_dcb[2]) >> 20) & 0x3)
#define TX_DCB_ALIGN_BYTESf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x3 << 20)) | ((((uint32_t)f) & 0x3) << 20))
#define TX_DCB_PURGEf_GET(r) ((((r).tx_dcb[2]) >> 22) & 0x1)
#define TX_DCB_PURGEf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 22)) | ((((uint32_t)f) & 0x1) << 22))
#define TX_DCB_DESC_DONE_INTRf_GET(r) ((((r).tx_dcb[2]) >> 23) & 0x1)
#define TX_DCB_DESC_DONE_INTRf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 23)) | ((((uint32_t)f) & 0x1) << 23))
#define TX_DCB_DESC_CTRL_INTRf_GET(r) ((((r).tx_dcb[2]) >> 24) & 0x1)
#define TX_DCB_DESC_CTRL_INTRf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 24)) | ((((uint32_t)f) & 0x1) << 24))
#define TX_DCB_DESC_REMAINf_GET(r) ((((r).tx_dcb[2]) >> 25) & 0x3f)
#define TX_DCB_DESC_REMAINf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x3f << 25)) | ((((uint32_t)f) & 0x3f) << 25))
#define TX_DCB_DESC_STAT_WR_DISABLEf_GET(r) ((((r).tx_dcb[2]) >> 31) & 0x1)
#define TX_DCB_DESC_STAT_WR_DISABLEf_SET(r,f) (r).tx_dcb[2]=(((r).tx_dcb[2] & ~((uint32_t)0x1 << 31)) | ((((uint32_t)f) & 0x1) << 31))
#define TX_DCB_ADDR_HIf_GET(r) ((r).tx_dcb[1])
#define TX_DCB_ADDR_HIf_SET(r,f) (r).tx_dcb[1]=((uint32_t)f)
#define TX_DCB_ADDR_LOf_GET(r) ((r).tx_dcb[0])
#define TX_DCB_ADDR_LOf_SET(r,f) (r).tx_dcb[0]=((uint32_t)f)

/*
 * This structure should be used to declare and program RX_DCB.
 *
 */
typedef union RX_DCB_s {
    uint32_t v[4];
    uint32_t rx_dcb[4];
    uint32_t _rx_dcb;
} RX_DCB_t;

#define RX_DCB_CLR(r) sal_memset(&((r).rx_dcb[0]), 0, sizeof(RX_DCB_t))
#define RX_DCB_SET(r,i,d) (r).rx_dcb[i] = d
#define RX_DCB_GET(r,i) (r).rx_dcb[i]
#define RX_DCB_OFFSET 0x00000000
#define RX_DCB_BYTES_TRANSFERREDf_GET(r) (((r).rx_dcb[3]) & 0xffff)
#define RX_DCB_BYTES_TRANSFERREDf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff))
#define RX_DCB_END_BITf_GET(r) ((((r).rx_dcb[3]) >> 16) & 0x1)
#define RX_DCB_END_BITf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0x1 << 16)) | ((((uint32_t)f) & 0x1) << 16))
#define RX_DCB_START_BITf_GET(r) ((((r).rx_dcb[3]) >> 17) & 0x1)
#define RX_DCB_START_BITf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0x1 << 17)) | ((((uint32_t)f) & 0x1) << 17))
#define RX_DCB_CELL_ERRORf_GET(r) ((((r).rx_dcb[3]) >> 18) & 0x1)
#define RX_DCB_CELL_ERRORf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0x1 << 18)) | ((((uint32_t)f) & 0x1) << 18))
#define RX_DCB_ECC_ERRORf_GET(r) ((((r).rx_dcb[3]) >> 19) & 0x1)
#define RX_DCB_ECC_ERRORf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0x1 << 19)) | ((((uint32_t)f) & 0x1) << 19))
#define RX_DCB_DONEf_GET(r) ((((r).rx_dcb[3]) >> 31) & 0x1)
#define RX_DCB_DONEf_SET(r,f) (r).rx_dcb[3]=(((r).rx_dcb[3] & ~((uint32_t)0x1 << 31)) | ((((uint32_t)f) & 0x1) << 31))
#define RX_DCB_BYTE_COUNTf_GET(r) (((r).rx_dcb[2]) & 0xffff)
#define RX_DCB_BYTE_COUNTf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0xffff)) | (((uint32_t)f) & 0xffff))
#define RX_DCB_CHAINf_GET(r) ((((r).rx_dcb[2]) >> 16) & 0x1)
#define RX_DCB_CHAINf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 16)) | ((((uint32_t)f) & 0x1) << 16))
#define RX_DCB_SGf_GET(r) ((((r).rx_dcb[2]) >> 17) & 0x1)
#define RX_DCB_SGf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 17)) | ((((uint32_t)f) & 0x1) << 17))
#define RX_DCB_RELOADf_GET(r) ((((r).rx_dcb[2]) >> 18) & 0x1)
#define RX_DCB_RELOADf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 18)) | ((((uint32_t)f) & 0x1) << 18))
#define RX_DCB_ALIGN_BYTESf_GET(r) ((((r).rx_dcb[2]) >> 20) & 0x3)
#define RX_DCB_ALIGN_BYTESf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x3 << 20)) | ((((uint32_t)f) & 0x3) << 20))
#define RX_DCB_DESC_DONE_INTRf_GET(r) ((((r).rx_dcb[2]) >> 23) & 0x1)
#define RX_DCB_DESC_DONE_INTRf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 23)) | ((((uint32_t)f) & 0x1) << 23))
#define RX_DCB_DESC_CTRL_INTRf_GET(r) ((((r).rx_dcb[2]) >> 24) & 0x1)
#define RX_DCB_DESC_CTRL_INTRf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 24)) | ((((uint32_t)f) & 0x1) << 24))
#define RX_DCB_DESC_REMAINf_GET(r) ((((r).rx_dcb[2]) >> 25) & 0x3f)
#define RX_DCB_DESC_REMAINf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x3f << 25)) | ((((uint32_t)f) & 0x3f) << 25))
#define RX_DCB_DESC_STAT_WR_DISABLEf_GET(r) ((((r).rx_dcb[2]) >> 31) & 0x1)
#define RX_DCB_DESC_STAT_WR_DISABLEf_SET(r,f) (r).rx_dcb[2]=(((r).rx_dcb[2] & ~((uint32_t)0x1 << 31)) | ((((uint32_t)f) & 0x1) << 31))
#define RX_DCB_ADDR_HIf_GET(r) ((r).rx_dcb[1])
#define RX_DCB_ADDR_HIf_SET(r,f) (r).rx_dcb[1]=((uint32_t)f)
#define RX_DCB_ADDR_LOf_GET(r) ((r).rx_dcb[0])
#define RX_DCB_ADDR_LOf_SET(r,f) (r).rx_dcb[0]=((uint32_t)f)

#endif /* BCMCNET_CMICR_ACC_H */
