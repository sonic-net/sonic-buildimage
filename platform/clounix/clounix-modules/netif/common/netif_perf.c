/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:  netif_perf.c
 * PURPOSE:
 *      It provide customer performance test API.
 * NOTES:
 */
#include <clx_error.h>
#include <clx_types.h>

#include <clx_dev_knl.h>
#include <netif_osal.h>
#include <netif_perf.h>
#include <netif_pkt_knl.h>

#include <light/dawn/hal_lt_dawn_pkt_knl.h>
#include <light/lightning/hal_lt_lightning_pkt_knl.h>
#include <mountain/namchabarwa/hal_mt_namchabarwa_pkt_knl.h>
#include <osal/osal_mdc.h>

/* -------------------------------------------------------------- switch */
#define PERF_TX_CHANNEL_NUM_MAX (HAL_LT_LIGHTNING_PKT_TX_CHANNEL_LAST)
#define PERF_RX_CHANNEL_NUM_MAX (HAL_LT_LIGHTNING_PKT_RX_CHANNEL_LAST)

extern struct pci_dev *_ptr_ext_pci_dev;
static UI16_T dev_id = 0;

/* -------------------------------------------------------------- common */
#define PERF_TX_PERF_NUM  (1000000) /* max: 4294967 */
#define PERF_TX_PERF_MESG (50000)
#define PERF_TX_PERF_FAIL (10000)
#define PERF_RX_PERF_NUM  (1000000) /* max: 4294967 */
#define PERF_RX_PERF_MESG (50000)
#define PERF_RX_PERF_FAIL (10000)

/* -------------------------------------------------------------- callbacks for chip dependency */
/* Tx */
typedef CLX_ERROR_NO_T (*PERF_TX_GET_INTR_T)(const UI32_T unit,
                                             const UI32_T channel,
                                             UI32_T *ptr_intr_cnt);
#if defined(CLX_EN_NETIF)
typedef CLX_ERROR_NO_T (*PERF_TX_GET_NETDEV_T)(const UI32_T unit,
                                               const UI32_T port,
                                               struct net_device **pptr_net_dev);
#endif
typedef CLX_ERROR_NO_T (*PERF_TX_PREPARE_GPD_T)(const UI32_T unit,
                                                const CLX_ADDR_T phy_addr,
                                                const UI32_T len,
                                                const UI32_T port,
                                                void *ptr_sw_gpd);

typedef CLX_ERROR_NO_T (*PERF_TX_SEND_GPD_T)(const UI32_T unit,
                                             const UI32_T channel,
                                             void *ptr_sw_gpd);

/* Rx */
typedef CLX_ERROR_NO_T (*PERF_RX_GET_INTR_T)(const UI32_T unit,
                                             const UI32_T channel,
                                             UI32_T *ptr_intr_cnt);

/* -------------------------------------------------------------- structs */
typedef enum {
    PERF_DIR_TX = 0,
    PERF_DIR_RX,
    PERF_DIR_LAST,

} PERF_DIR_T;

typedef struct {
    UI32_T unit;
    UI32_T channel;
    UI32_T len;
    UI32_T num;
    UI32_T port;
    BOOL_T test_skb;

} PERF_COOKIE_T;

typedef struct {
    /* netif-only */
    PERF_COOKIE_T tx_cookie[PERF_TX_CHANNEL_NUM_MAX];

    UI32_T tx_channel_bitmap;
    UI32_T target_num;
    UI32_T target_len;
    CLX_THREAD_ID_T tx_task[PERF_TX_CHANNEL_NUM_MAX];
    CLX_SEMAPHORE_ID_T start_sync[PERF_TX_CHANNEL_NUM_MAX];
    CLX_SEMAPHORE_ID_T end_sync[PERF_TX_CHANNEL_NUM_MAX];
    UI32_T send_ok[PERF_TX_CHANNEL_NUM_MAX];
    UI32_T send_fail[PERF_TX_CHANNEL_NUM_MAX];
    BOOL_T test_skb;

    /* chip dependent callbacks */
    PERF_TX_GET_INTR_T get_intr_cnt;
#if defined(CLX_EN_NETIF)
    PERF_TX_GET_NETDEV_T get_netdev;
#endif
    PERF_TX_PREPARE_GPD_T prepare_gpd;
    PERF_TX_SEND_GPD_T send_gpd;

} PERF_TX_PERF_CB_T;

typedef struct {
    /* netif-only */
    BOOL_T rx_test;

    CLX_SEMAPHORE_ID_T start_sync;
    CLX_SEMAPHORE_ID_T end_sync;
    UI32_T rx_channel;
    UI32_T target_num;
    UI32_T target_len;
    UI32_T recv_pass;
    UI32_T recv_fail;

    /* duplicate packets */
    UI32_T rch_qid_map_lo[PERF_RX_CHANNEL_NUM_MAX];
    UI32_T rch_qid_map_hi[PERF_RX_CHANNEL_NUM_MAX];

    /* chip dependent callbacks */
    PERF_RX_GET_INTR_T get_intr_cnt;

} PERF_RX_PERF_CB_T;

CLX_ERROR_NO_T
_hal_perf_pkt_getTxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt)
{
    CLX_ERROR_NO_T ret;
#if defined(CLX_EN_NETIF)
    if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
        ret = hal_lt_dawn_pkt_getTxIntrCnt(unit, channel, ptr_intr_cnt);
    } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
        ret = hal_lt_lightning_pkt_getTxIntrCnt(unit, channel, ptr_intr_cnt);
    } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#else
    if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#endif
        ret = hal_mt_namchabarwa_pkt_getTxIntrCnt(unit, channel, ptr_intr_cnt);
    } else {
        OSAL_PRINT(OSAL_DBG_ERR, "unknown chip family, dev_id=0x%x\n", dev_id);
        ret = CLX_E_OTHERS;
    }
    return ret;
}

#if defined(CLX_EN_NETIF)
CLX_ERROR_NO_T
_hal_perf_pkt_getNetDev(const UI32_T unit, const UI32_T port, struct net_device **pptr_net_dev)
{
    CLX_ERROR_NO_T ret;

    if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
        ret = hal_lt_dawn_pkt_getNetDev(unit, port, pptr_net_dev);
    } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
        ret = hal_lt_lightning_pkt_getNetDev(unit, port, pptr_net_dev);
    } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
        ret = hal_mt_namchabarwa_pkt_getNetDev(unit, port, pptr_net_dev);
    } else {
        OSAL_PRINT(OSAL_DBG_ERR, "unknown chip family, dev_id=0x%x\n", dev_id);
        ret = CLX_E_OTHERS;
    }
    return ret;
}
#endif

CLX_ERROR_NO_T
_hal_perf_pkt_prepareGpd(const UI32_T unit,
                         const CLX_ADDR_T phy_addr,
                         const UI32_T len,
                         const UI32_T port,
                         void *ptr_sw_gpd)
{
    CLX_ERROR_NO_T ret;
    void *ptr_cookie = NULL;
#if defined(CLX_EN_NETIF)
    if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
        ret = hal_lt_dawn_pkt_prepareGpd(unit, phy_addr, len, port,
                                         (HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
        ret = hal_lt_lightning_pkt_prepareGpd(unit, phy_addr, len, port,
                                              (HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#else
    if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#endif
        ptr_cookie = ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_cookie;
        ret = hal_mt_namchabarwa_pkt_preparePPh(
            unit, port,
            (HAL_MT_NAMCHABARWA_PKT_PPH_L2_T *)((void *)ptr_cookie +
                                                HAL_MT_NAMCHABARWA_PKT_EMAC_SZ));
        ret = hal_mt_namchabarwa_pkt_prepareGpd(unit, phy_addr, len,
                                                (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else {
        OSAL_PRINT(OSAL_DBG_ERR, "unknown chip family, dev_id=0x%x\n", dev_id);
        ret = CLX_E_OTHERS;
    }
    return ret;
}
CLX_ERROR_NO_T
_hal_perf_pkt_sendGpd(const UI32_T unit, const UI32_T channel, void *ptr_sw_gpd)
{
    CLX_ERROR_NO_T ret;
#if defined(CLX_EN_NETIF)
    if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
        ret = hal_lt_dawn_pkt_sendGpd(unit, channel, (HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
        ret = hal_lt_lightning_pkt_sendGpd(unit, channel,
                                           (HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#else
    if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#endif
        ret = hal_mt_namchabarwa_pkt_sendGpd(unit, channel,
                                             (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd);
    } else {
        OSAL_PRINT(OSAL_DBG_ERR, "unknown chip family, dev_id=0x%x\n", dev_id);
        ret = CLX_E_OTHERS;
    }
    return ret;
}
CLX_ERROR_NO_T
_hal_perf_pkt_getRxIntrCnt(const UI32_T unit, const UI32_T channel, UI32_T *ptr_intr_cnt)
{
    CLX_ERROR_NO_T ret;
#if defined(CLX_EN_NETIF)
    if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
        ret = hal_lt_dawn_pkt_getRxIntrCnt(unit, channel, ptr_intr_cnt);
    } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
        ret = hal_lt_lightning_pkt_getRxIntrCnt(unit, channel, ptr_intr_cnt);
    } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#else
    if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
#endif
        ret = hal_mt_namchabarwa_pkt_getRxIntrCnt(unit, channel, ptr_intr_cnt);
    } else {
        OSAL_PRINT(OSAL_DBG_ERR, "unknown chip family, dev_id=0x%x\n", dev_id);
        ret = CLX_E_OTHERS;
    }
    return ret;
}

/* -------------------------------------------------------------- statics */
static PERF_TX_PERF_CB_T _perf_tx_perf_cb = {
    .get_intr_cnt = _hal_perf_pkt_getTxIntrCnt,
#if defined(CLX_EN_NETIF)
    .get_netdev = _hal_perf_pkt_getNetDev,   /* test_skb = TRUE  */
#endif
    .prepare_gpd = _hal_perf_pkt_prepareGpd, /* test_skb = FALSE */
    .send_gpd = _hal_perf_pkt_sendGpd,       /* test_skb = FALSE */
};

static PERF_RX_PERF_CB_T _perf_rx_perf_cb = {
    .get_intr_cnt = _hal_perf_pkt_getRxIntrCnt,
};

/* -------------------------------------------------------------- functions */
static void
_perf_duplicateRxPacket(const UI32_T unit, const UI32_T rx_channel, const BOOL_T enable)
{
    ;
}

static UI32_T
_perf_showPerf(char *buf,
               PERF_DIR_T dir,
               UI32_T channel,
               UI32_T len,
               UI32_T num,
               UI32_T intr,
               CLX_TIME_T duration)
{
    UI32_T tx_channel = 0;
    UI32_T tx_fail = 0;
    UI32_T count = 0;

    if (duration < 1000) {
        count += sprintf(buf + count, "***Error***, %d packets cost < 1000 us.\n", num);
        return count;
    }

    count += sprintf(buf + count, "\n");

    if (PERF_DIR_TX == dir) {
        sprintf(buf + count, "Tx-perf\n");
    } else {
        sprintf(buf + count, "Rx-perf\n");
    }

    count += sprintf(buf + count, "------------------------------------\n");
    count += sprintf(buf + count, "channel number          : %d\n", channel);
    count += sprintf(buf + count, "packet length    (bytes): %d\n", len);
    count += sprintf(buf + count, "packet number           : %d\n", num);
    count += sprintf(buf + count, "time duration    (us)   : %llu\n", duration);
    count += sprintf(buf + count, "------------------------------------\n");
    count +=
        sprintf(buf + count, "avg. packet rate (pps)  : %llu\n", (num * 1000) / (duration / 1000));
    count += sprintf(buf + count, "avg. throughput  (Mbps) : %llu\n",
                     ((num / 1000) * len * 8) / (duration / 1000));
    count += sprintf(buf + count, "interrupt number        : %d\n", intr);

    if (PERF_DIR_TX == dir) {
        for (tx_channel = 0; tx_channel < PERF_TX_CHANNEL_NUM_MAX; tx_channel++) {
            if (!((1 << tx_channel) & _perf_tx_perf_cb.tx_channel_bitmap)) {
                continue;
            }
            tx_fail += _perf_tx_perf_cb.send_fail[tx_channel];
        }
        count += sprintf(buf + count, "Tx fail                 : %d\n", tx_fail);
    }

    count += sprintf(buf + count, "------------------------------------\n");
    printk(buf);
    return count;
}

static void
_perf_getIntrCnt(UI32_T unit, PERF_DIR_T dir, UI32_T *ptr_intr_cnt)
{
    UI32_T intr_cnt = 0;
    UI32_T channel = 0;

    if (PERF_DIR_TX == dir) {
        for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++) {
            _perf_tx_perf_cb.get_intr_cnt(unit, channel, &intr_cnt);
            *ptr_intr_cnt += intr_cnt;
        }
    } else {
        for (channel = 0; channel < PERF_RX_CHANNEL_NUM_MAX; channel++) {
            _perf_rx_perf_cb.get_intr_cnt(unit, channel, &intr_cnt);
            *ptr_intr_cnt += intr_cnt;
        }
    }
}

static void
_perf_txCallback(const UI32_T unit, void *ptr_sw_gpd, void *ptr_virt_addr)
{
    /* free dma */
    osal_dma_free(ptr_virt_addr);

    /* free gpd */
    osal_free(ptr_sw_gpd);
}

static void
_perf_txTask(void *ptr_argv)
{
    CLX_ERROR_NO_T rc = CLX_E_OK;
    UI32_T unit = ((PERF_COOKIE_T *)ptr_argv)->unit;
    UI32_T channel = ((PERF_COOKIE_T *)ptr_argv)->channel;
    UI32_T len = ((PERF_COOKIE_T *)ptr_argv)->len;
    UI32_T num = ((PERF_COOKIE_T *)ptr_argv)->num;
    UI32_T port = ((PERF_COOKIE_T *)ptr_argv)->port;
    BOOL_T test_skb = ((PERF_COOKIE_T *)ptr_argv)->test_skb;

    /* test targets */
    void *ptr_sw_gpd = NULL;
#if defined(CLX_EN_NETIF)
    struct sk_buff *ptr_skb = NULL;
#endif

    /* temp variables */
    UI32_T send_fail = 0;
    void *ptr_virt_addr = NULL;
    CLX_ADDR_T phy_addr = 0x0;

    osal_initRunThread();
    do {
        rc = osal_waitEvent(&_perf_tx_perf_cb.start_sync[channel]);
        if (CLX_E_OK != osal_isRunThread()) {
            break; /* deinit-thread */
        }

        while (_perf_tx_perf_cb.send_ok[channel] < num) {
            if (0 == (_perf_tx_perf_cb.send_ok[channel] % PERF_TX_PERF_MESG)) {
                printk("T");
            }

            if (TRUE == test_skb) {
#if defined(CLX_EN_NETIF)
                ptr_skb = osal_skb_alloc(len);
                ptr_skb->len = len;
                _perf_tx_perf_cb.get_netdev(unit, port, &ptr_skb->dev);

                /* send skb */
                osal_skb_send(ptr_skb);
#else
                OSAL_PRINT(OSAL_DBG_ERR, "NOT SUPPORT TEST SKB\n");
#endif
            } else {
                /* prepare buf */
                ptr_virt_addr = osal_dma_alloc(len);
                phy_addr = osal_dma_convertVirtToPhy(ptr_virt_addr);

                if (NETIF_KNL_DEVICE_IS_DAWN(dev_id)) {
                    ptr_sw_gpd = (HAL_LT_DAWN_PKT_TX_SW_GPD_T *)osal_alloc(
                        sizeof(HAL_LT_DAWN_PKT_TX_SW_GPD_T));
                    if (NULL == ptr_sw_gpd) {
                        OSAL_PRINT(OSAL_DBG_ERR, "***Error***, alloc sw-gpd fail.\n");
                        break;
                    }
                    osal_memset(ptr_sw_gpd, 0x0, sizeof(HAL_LT_DAWN_PKT_TX_SW_GPD_T));

                    /* trans skb to gpd */
                    ((HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->callback =
                        (void *)_perf_txCallback;
                    ((HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_cookie = (void *)ptr_virt_addr;
                    ((HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->gpd_num = 1;
                    ((HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_next = NULL;
                    ((HAL_LT_DAWN_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->channel = channel;

                } else if (NETIF_KNL_DEVICE_IS_LIGHTNING(dev_id)) {
                    ptr_sw_gpd = (void *)osal_alloc(sizeof(HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T));
                    if (NULL == ptr_sw_gpd) {
                        OSAL_PRINT(OSAL_DBG_ERR, "***Error***, alloc sw-gpd fail.\n");
                        break;
                    }
                    osal_memset(ptr_sw_gpd, 0x0, sizeof(HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T));

                    /* trans skb to gpd */
                    ((HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->callback =
                        (void *)_perf_txCallback;
                    ((HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_cookie =
                        (void *)ptr_virt_addr;
                    ((HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->gpd_num = 1;
                    ((HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_next = NULL;
                    ((HAL_LT_LIGHTNING_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->channel = channel;
                } else if (NETIF_KNL_DEVICE_IS_NAMCHABARWA(dev_id)) {
                    ptr_sw_gpd = (void *)osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
                    if (NULL == ptr_sw_gpd) {
                        OSAL_PRINT(OSAL_DBG_ERR, "***Error***, alloc sw-gpd fail.\n");
                        break;
                    }
                    osal_memset(ptr_sw_gpd, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));

                    /* trans skb to gpd */
                    ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->callback =
                        (void *)_perf_txCallback;
                    ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_cookie =
                        (void *)ptr_virt_addr;
                    ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->gpd_num = 1;
                    ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->ptr_next = NULL;
                    ((HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)ptr_sw_gpd)->channel = channel;
                }

                /* prepare gpd */
                rc = _perf_tx_perf_cb.prepare_gpd(unit, phy_addr, len, port, ptr_sw_gpd);

                /* send gpd */
                rc = _perf_tx_perf_cb.send_gpd(unit, channel, ptr_sw_gpd);
                if (CLX_E_OK == rc) {
                    _perf_tx_perf_cb.send_ok[channel]++;
                    send_fail = 0;
                } else {
                    _perf_tx_perf_cb.send_fail[channel]++;
                    if (send_fail++ >= PERF_TX_PERF_FAIL) {
                        OSAL_PRINT(OSAL_DBG_ERR,
                                   "***Error***, Tch-%d send fail over %d packet(s). (rc: %d)\n",
                                   channel, PERF_TX_PERF_FAIL, rc);
                        break;
                    }

                    _perf_txCallback(unit, ptr_sw_gpd, ptr_virt_addr);
                    osal_sleepThread(1000);
                }
            }
        }

        osal_triggerEvent(&_perf_tx_perf_cb.end_sync[channel]);
    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

static void
_perf_txDeinit(const UI32_T unit)
{
    UI32_T channel = 0;

    for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++) {
        if (!((1 << channel) & _perf_tx_perf_cb.tx_channel_bitmap)) {
            continue;
        }
        /* destroy Tx resources */
        osal_stopThread(&_perf_tx_perf_cb.tx_task[channel]);
        osal_triggerEvent(&_perf_tx_perf_cb.start_sync[channel]);
        osal_destroyThread(&_perf_tx_perf_cb.tx_task[channel]);
        osal_destroyEvent(&_perf_tx_perf_cb.end_sync[channel]);
        osal_destroyEvent(&_perf_tx_perf_cb.start_sync[channel]);
    }
}

static void
_perf_txInit(const UI32_T unit, const UI32_T len, BOOL_T test_skb)
{
    UI32_T channel = 0;
    UI32_T channel_num = 0;

    for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++) {
        if (!((1 << channel) & _perf_tx_perf_cb.tx_channel_bitmap)) {
            continue;
        }
        channel_num++;
        _perf_tx_perf_cb.send_ok[channel] = 0;
        _perf_tx_perf_cb.send_fail[channel] = 0;

        /* create Tx resources */
        osal_createEvent("TX_START", &_perf_tx_perf_cb.start_sync[channel]);
        osal_createEvent("TX_END", &_perf_tx_perf_cb.end_sync[channel]);

        _perf_tx_perf_cb.tx_cookie[channel].unit = unit;
        _perf_tx_perf_cb.tx_cookie[channel].channel = channel;
        _perf_tx_perf_cb.tx_cookie[channel].len = len;
        _perf_tx_perf_cb.tx_cookie[channel].num = _perf_tx_perf_cb.target_num / channel_num;
        _perf_tx_perf_cb.tx_cookie[channel].port = 0;
        _perf_tx_perf_cb.tx_cookie[channel].test_skb = test_skb;

        osal_createThread("TX_PERF", 64 * 1024, 90, _perf_txTask,
                          (void *)&_perf_tx_perf_cb.tx_cookie[channel],
                          &_perf_tx_perf_cb.tx_task[channel]);
    }
}

static void
_perf_rxDeinit(const UI32_T unit, const UI32_T rx_channel)
{
    /* turn-off Rx test */
    _perf_rx_perf_cb.rx_test = FALSE;
    osal_sleepThread(10000);

    /* destroy Rx resources */
    osal_destroyEvent(&_perf_rx_perf_cb.end_sync);
    osal_destroyEvent(&_perf_rx_perf_cb.start_sync);

    /* disable duplicate Rx packets to channels */
    _perf_duplicateRxPacket(unit, rx_channel, FALSE);
}

static void
_perf_rxInit(const UI32_T unit, const UI32_T rx_channel, const UI32_T len)
{
    /* enable duplicate Rx packets to channels */
    _perf_duplicateRxPacket(unit, rx_channel, TRUE);

    /* create Rx callback resources */
    _perf_rx_perf_cb.target_num = _perf_rx_perf_cb.target_num;
    _perf_rx_perf_cb.target_len = len;
    _perf_rx_perf_cb.recv_pass = 0;

    osal_createEvent("RX_START", &_perf_rx_perf_cb.start_sync);
    osal_createEvent("RX_END", &_perf_rx_perf_cb.end_sync);

    /* turn-on Rx test */
    _perf_rx_perf_cb.rx_test = TRUE;
}

/**
 * @brief To count the Rx-gpd for Rx-test.
 *
 * @param [in]     len    - To check if the Rx-gpd length equals to test length.
 * @return         CLX_E_OK    - Successful operation.
 */
CLX_ERROR_NO_T
perf_rxCallback(const UI32_T len)
{
    /* check length */
    if (len == _perf_rx_perf_cb.target_len) {
        _perf_rx_perf_cb.recv_pass++;
    } else {
        if (_perf_rx_perf_cb.recv_fail++ >= PERF_RX_PERF_FAIL) {
            _perf_rx_perf_cb.recv_fail = 0;
        }
    }

    /* send signals */
    if (0 == _perf_rx_perf_cb.recv_pass) {
        ; /* do nothing */
    } else if (1 == (_perf_rx_perf_cb.recv_pass % _perf_rx_perf_cb.target_num)) {
        osal_triggerEvent(&_perf_rx_perf_cb.start_sync);
    } else if ((_perf_rx_perf_cb.recv_pass % _perf_rx_perf_cb.target_num) ==
               _perf_rx_perf_cb.target_num - 1) {
        osal_triggerEvent(&_perf_rx_perf_cb.end_sync);
    } else if (0 == (_perf_rx_perf_cb.recv_pass % PERF_RX_PERF_MESG)) {
        printk("R");
    }

    return (CLX_E_OK);
}

/**
 * @brief To check if Rx-test is going.
 *
 * @return         CLX_E_OK    - Successful operation.
 */
CLX_ERROR_NO_T
perf_rxTest(void)
{
    if (FALSE == _perf_rx_perf_cb.rx_test) {
        return (CLX_E_OTHERS);
    }

    return (CLX_E_OK);
}

static ssize_t
perf_tx_test(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    CLX_TIME_T start_time;
    CLX_TIME_T end_time;
    UI32_T unit = 0, channel = 0;
    UI32_T tx_pkt_cnt = 0, tx_start_intr = 0, tx_end_intr = 0;

    _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_start_intr);
    _perf_txInit(unit, _perf_tx_perf_cb.target_len, _perf_tx_perf_cb.test_skb);

    /* ------------- in-time ------------- */
    osal_getTime(&start_time);
    for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++) {
        if (!((1 << channel) & _perf_tx_perf_cb.tx_channel_bitmap)) {
            continue;
        }
        osal_triggerEvent(&_perf_tx_perf_cb.start_sync[channel]);
    }
    for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++) {
        if (!((1 << channel) & _perf_tx_perf_cb.tx_channel_bitmap)) {
            continue;
        }
        osal_waitEvent(&_perf_tx_perf_cb.end_sync[channel]);
        tx_pkt_cnt += _perf_tx_perf_cb.send_ok[channel];
    }
    osal_getTime(&end_time);
    /* ------------- in-time ------------- */

    _perf_txDeinit(unit);
    _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_end_intr);

    return _perf_showPerf(buf, PERF_DIR_TX, _perf_tx_perf_cb.tx_channel_bitmap,
                          _perf_tx_perf_cb.target_len, tx_pkt_cnt, tx_end_intr - tx_start_intr,
                          end_time - start_time);
}

static ssize_t
perf_rx_test(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    CLX_TIME_T start_time;
    CLX_TIME_T end_time;
    UI32_T unit = 0;
    UI32_T rx_start_intr = 0, rx_end_intr = 0;

    _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_start_intr);
    _perf_rxInit(unit, _perf_rx_perf_cb.rx_channel, _perf_rx_perf_cb.target_len);

    /* wait 1st Rx GPD done */
    osal_waitEvent(&_perf_rx_perf_cb.start_sync);

    /* ------------- in-time ------------- */
    osal_getTime(&start_time);
    osal_waitEvent(&_perf_rx_perf_cb.end_sync);
    osal_getTime(&end_time);
    /* ------------- in-time ------------- */

    _perf_rxDeinit(unit, _perf_rx_perf_cb.rx_channel);
    _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_end_intr);

    return _perf_showPerf(buf, PERF_DIR_RX, _perf_rx_perf_cb.rx_channel,
                          _perf_rx_perf_cb.target_len, _perf_rx_perf_cb.target_num,
                          rx_end_intr - rx_start_intr, end_time - start_time);
}

static ssize_t
get_rx_target_num(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", _perf_rx_perf_cb.target_num);
}

static ssize_t
set_rx_target_num(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;

    ret = kstrtoint(buf, 10, &_perf_rx_perf_cb.target_num);
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t
get_rx_target_len(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", _perf_rx_perf_cb.target_len);
}

static ssize_t
set_rx_target_len(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;

    ret = kstrtoint(buf, 10, &_perf_rx_perf_cb.target_len);
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t
get_tx_target_num(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", _perf_tx_perf_cb.target_num);
}

static ssize_t
set_tx_target_num(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;

    ret = kstrtoint(buf, 10, &_perf_tx_perf_cb.target_num);
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t
get_tx_target_len(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", _perf_tx_perf_cb.target_len);
}

static ssize_t
set_tx_target_len(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;

    ret = kstrtoint(buf, 10, &_perf_tx_perf_cb.target_len);
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t
get_tx_channel_bmp(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", _perf_tx_perf_cb.tx_channel_bitmap);
}

static ssize_t
set_tx_channel_bmp(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int ret;

    ret = kstrtoint(buf, 10, &_perf_tx_perf_cb.tx_channel_bitmap);
    if (ret < 0)
        return ret;

    return count;
}

static ssize_t
perf_get_help(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    char *help =
        "1. set rx packet num:     echo 100000 > /sys/module/clx_dev/perf_test/rx_target_num \n"
        "2. set rx packet len:     echo 256> /sys/module/clx_dev/perf_test/rx_target_len \n"
        "3. set tx packet num:     echo 100000 > /sys/module/clx_dev/perf_test/tx_target_num \n"
        "4. set tx packet len:     echo 256> /sys/module/clx_dev/perf_test/tx_target_len \n"
        "5. tx channel bitmap:     echo 8 > /sys/module/clx_dev/perf_test/tx_channel_bmp \n"
        "6. start rx test:         cat /sys/module/clx_dev/perf_test/rx_test \n"
        "7. start tx test:         cat /sys/module/clx_dev/perf_test/tx_test \n";

    return sprintf(buf, "%s\n", help);
}

static struct kobj_attribute perf_test_help_attribute = __ATTR(help, 0660, perf_get_help, NULL);

static struct kobj_attribute perf_rx_test_attribute = __ATTR(rx_test, 0660, perf_rx_test, NULL);

static struct kobj_attribute perf_tx_test_attribute = __ATTR(tx_test, 0660, perf_tx_test, NULL);

static struct kobj_attribute perf_test_rx_target_num_attribute =
    __ATTR(rx_target_num, 0664, get_rx_target_num, set_rx_target_num);

static struct kobj_attribute perf_test_rx_target_len_attribute =
    __ATTR(rx_target_len, 0664, get_rx_target_len, set_rx_target_len);

static struct kobj_attribute perf_test_tx_target_num_attribute =
    __ATTR(tx_target_num, 0664, get_tx_target_num, set_tx_target_num);

static struct kobj_attribute perf_test_tx_target_len_attribute =
    __ATTR(tx_target_len, 0664, get_tx_target_len, set_tx_target_len);

static struct kobj_attribute perf_test_tx_channel_bmp_attribute =
    __ATTR(tx_channel_bmp, 0664, get_tx_channel_bmp, set_tx_channel_bmp);

static struct attribute *attrs[] = {
    &perf_test_help_attribute.attr,
    &perf_rx_test_attribute.attr,
    &perf_tx_test_attribute.attr,
    &perf_test_rx_target_num_attribute.attr,
    &perf_test_rx_target_len_attribute.attr,
    &perf_test_tx_target_num_attribute.attr,
    &perf_test_tx_target_len_attribute.attr,
    &perf_test_tx_channel_bmp_attribute.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

static struct kobject *perf_test_kobj;

int
perf_test_init(void)
{
    int retval;
    pci_read_config_word(_ptr_ext_pci_dev, PCI_DEVICE_ID, &dev_id);

    _perf_rx_perf_cb.target_num = PERF_RX_PERF_NUM;
    _perf_rx_perf_cb.target_len = 256;

    _perf_tx_perf_cb.target_num = PERF_TX_PERF_NUM;
    _perf_tx_perf_cb.tx_channel_bitmap = 0x1;
    _perf_tx_perf_cb.target_len = 256;
    _perf_tx_perf_cb.test_skb = FALSE;

    perf_test_kobj = kobject_create_and_add("perf_test", &__this_module.mkobj.kobj);
    if (!perf_test_kobj)
        return -ENOMEM;

    retval = sysfs_create_group(perf_test_kobj, &attr_group);
    if (retval)
        kobject_put(perf_test_kobj);

    return retval;
}
void
perf_test_exit(void)
{
    sysfs_remove_group(perf_test_kobj, &attr_group);
    kobject_put(perf_test_kobj);
}
