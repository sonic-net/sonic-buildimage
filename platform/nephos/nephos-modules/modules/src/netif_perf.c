/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:  netif_perf.c
 * PURPOSE:
 *      It provide customer performance test API.
 * NOTES:
 */
#include <nps_error.h>
#include <nps_types.h>

#include <netif_osal.h>
#include <netif_perf.h>

#if defined (NPS_EN_TAURUS)
#include <hal_tau_pkt_knl.h>
#endif

/* -------------------------------------------------------------- switch */
#if defined (NPS_EN_ARIES)
#define PERF_TX_CHANNEL_NUM_MAX     (HAL_ARI_PKT_TX_CHANNEL_LAST)
#define PERF_RX_CHANNEL_NUM_MAX     (HAL_ARI_PKT_RX_CHANNEL_LAST)
typedef HAL_ARI_PKT_TX_SW_GPD_T     PERF_TX_SW_GPD;
typedef HAL_ARI_PKT_RX_SW_GPD_T     PERF_RX_SW_GPD;
#endif

#if defined (NPS_EN_TAURUS)
#define PERF_TX_CHANNEL_NUM_MAX     (HAL_TAU_PKT_TX_CHANNEL_LAST)
#define PERF_RX_CHANNEL_NUM_MAX     (HAL_TAU_PKT_RX_CHANNEL_LAST)
typedef HAL_TAU_PKT_TX_SW_GPD_T     PERF_TX_SW_GPD;
typedef HAL_TAU_PKT_RX_SW_GPD_T     PERF_RX_SW_GPD;
#endif

/* -------------------------------------------------------------- common */
#define PERF_TX_PERF_NUM            (1000000) /* max: 4294967 */
#define PERF_TX_PERF_MESG           (50000)
#define PERF_TX_PERF_FAIL           (10000)
#define PERF_RX_PERF_NUM            (1000000) /* max: 4294967 */
#define PERF_RX_PERF_MESG           (50000)
#define PERF_RX_PERF_FAIL           (10000)

/* -------------------------------------------------------------- callbacks for chip dependency */
/* Tx */
typedef NPS_ERROR_NO_T
(*PERF_TX_GET_INTR_T)(
    const UI32_T                unit,
    const UI32_T                channel,
    UI32_T                     *ptr_intr_cnt);

typedef NPS_ERROR_NO_T
(*PERF_TX_GET_NETDEV_T)(
    const UI32_T                unit,
    const UI32_T                port,
    struct net_device           **pptr_net_dev);

typedef NPS_ERROR_NO_T
(*PERF_TX_PREPARE_GPD_T)(
    const UI32_T                unit,
    const NPS_ADDR_T            phy_addr,
    const UI32_T                len,
    const UI32_T                port,
    PERF_TX_SW_GPD              *ptr_sw_gpd);

typedef NPS_ERROR_NO_T
(*PERF_TX_SEND_GPD_T)(
    const UI32_T                unit,
    const UI32_T                channel,
    PERF_TX_SW_GPD              *ptr_sw_gpd);

/* Rx */
typedef NPS_ERROR_NO_T
(*PERF_RX_GET_INTR_T)(
    const UI32_T                unit,
    const UI32_T                channel,
    UI32_T                     *ptr_intr_cnt);

/* -------------------------------------------------------------- structs */
typedef enum
{
    PERF_DIR_TX = 0,
    PERF_DIR_RX,
    PERF_DIR_LAST,

} PERF_DIR_T;

typedef struct
{
    UI32_T                      unit;
    UI32_T                      channel;
    UI32_T                      len;
    UI32_T                      num;
    UI32_T                      port;
    BOOL_T                      test_skb;

} PERF_COOKIE_T;

typedef struct
{
    /* netif-only */
    PERF_COOKIE_T               tx_cookie  [PERF_TX_CHANNEL_NUM_MAX];

    NPS_THREAD_ID_T             tx_task    [PERF_TX_CHANNEL_NUM_MAX];
    NPS_SEMAPHORE_ID_T          start_sync [PERF_TX_CHANNEL_NUM_MAX];
    NPS_SEMAPHORE_ID_T          end_sync   [PERF_TX_CHANNEL_NUM_MAX];
    UI32_T                      send_ok    [PERF_TX_CHANNEL_NUM_MAX];
    UI32_T                      send_fail  [PERF_TX_CHANNEL_NUM_MAX];

    /* chip dependent callbacks */
    PERF_TX_GET_INTR_T          get_intr_cnt;
    PERF_TX_GET_NETDEV_T        get_netdev;
    PERF_TX_PREPARE_GPD_T       prepare_gpd;
    PERF_TX_SEND_GPD_T          send_gpd;

} PERF_TX_PERF_CB_T;

typedef struct
{
    /* netif-only */
    BOOL_T                      rx_test;

    NPS_SEMAPHORE_ID_T          start_sync;
    NPS_SEMAPHORE_ID_T          end_sync;
    UI32_T                      target_num;
    UI32_T                      target_len;
    UI32_T                      recv_pass;
    UI32_T                      recv_fail;

    /* duplicate packets */
    UI32_T                      rch_qid_map_lo [PERF_RX_CHANNEL_NUM_MAX];
    UI32_T                      rch_qid_map_hi [PERF_RX_CHANNEL_NUM_MAX];

    /* chip dependent callbacks */
    PERF_RX_GET_INTR_T          get_intr_cnt;

} PERF_RX_PERF_CB_T;

/* -------------------------------------------------------------- statics */
static PERF_TX_PERF_CB_T        _perf_tx_perf_cb =
{
#if defined (NPS_EN_ARIES)
    .get_intr_cnt               = hal_ari_pkt_getTxIntrCnt,
    .get_netdev                 = hal_ari_pkt_getNetDev,    /* test_skb = TRUE  */
    .prepare_gpd                = hal_ari_pkt_prepareGpd,   /* test_skb = FALSE */
    .send_gpd                   = hal_ari_pkt_sendGpd,      /* test_skb = FALSE */
#endif
#if defined (NPS_EN_TAURUS)
    .get_intr_cnt               = hal_tau_pkt_getTxIntrCnt,
    .get_netdev                 = hal_tau_pkt_getNetDev,    /* test_skb = TRUE  */
    .prepare_gpd                = hal_tau_pkt_prepareGpd,   /* test_skb = FALSE */
    .send_gpd                   = hal_tau_pkt_sendGpd,      /* test_skb = FALSE */
#endif
};

static PERF_RX_PERF_CB_T        _perf_rx_perf_cb =
{
#if defined (NPS_EN_ARIES)
    .get_intr_cnt               = hal_ari_pkt_getRxIntrCnt,
#endif
#if defined (NPS_EN_TAURUS)
    .get_intr_cnt               = hal_tau_pkt_getRxIntrCnt,
#endif
};

/* -------------------------------------------------------------- functions */
static void
_perf_duplicateRxPacket(
    const UI32_T                unit,
    const UI32_T                rx_channel,
    const BOOL_T                enable)
{
    ;
}

static void
_perf_showPerf(
    PERF_DIR_T                  dir,
    UI32_T                      channel,
    UI32_T                      len,
    UI32_T                      num,
    UI32_T                      intr,
    UI32_T                      duration)
{
    UI32_T                      tx_channel = 0;
    UI32_T                      tx_fail = 0;

    if (duration < 1000)
    {
        osal_printf("***Error***, %d packets cost < 1000 us.\n", num);
        return ;
    }

    osal_printf("\n");

    if (PERF_DIR_TX == dir)
    {
        osal_printf("Tx-perf\n");
    }
    else
    {
        osal_printf("Rx-perf\n");
    }

    osal_printf("------------------------------------\n");
    osal_printf("channel number          : %d\n", channel);
    osal_printf("packet length    (bytes): %d\n", len);
    osal_printf("packet number           : %d\n", num);
    osal_printf("time duration    (us)   : %d\n", duration);
    osal_printf("------------------------------------\n");
    osal_printf("avg. packet rate (pps)  : %d\n", (num * 1000) / (duration / 1000));
    osal_printf("avg. throughput  (Mbps) : %d\n", ((num / 1000) * len * 8) / (duration / 1000));
    osal_printf("interrupt number        : %d\n", intr);

    if (PERF_DIR_TX == dir)
    {
        for (tx_channel = 0; tx_channel < channel; tx_channel++)
        {
            tx_fail += _perf_tx_perf_cb.send_fail[tx_channel];
        }
        osal_printf("Tx fail                 : %d\n", tx_fail);
    }

    osal_printf("------------------------------------\n");
}

static void
_perf_getIntrCnt(
    UI32_T                      unit,
    PERF_DIR_T                  dir,
    UI32_T                      *ptr_intr_cnt)
{
    UI32_T                      intr_cnt = 0;
    UI32_T                      channel = 0;

    if (PERF_DIR_TX == dir)
    {
        for (channel = 0; channel < PERF_TX_CHANNEL_NUM_MAX; channel++)
        {
            _perf_tx_perf_cb.get_intr_cnt(unit, channel, &intr_cnt);
            *ptr_intr_cnt += intr_cnt;
        }
    }
    else
    {
        for (channel = 0; channel < PERF_RX_CHANNEL_NUM_MAX; channel++)
        {
            _perf_rx_perf_cb.get_intr_cnt(unit, channel, &intr_cnt);
            *ptr_intr_cnt += intr_cnt;
        }
    }
}

static void
_perf_txCallback(
    const UI32_T                unit,
    PERF_TX_SW_GPD              *ptr_sw_gpd,
    void                        *ptr_virt_addr)
{
    /* free dma */
    osal_dma_free(ptr_virt_addr);

    /* free gpd */
    osal_free(ptr_sw_gpd);
}

static void
_perf_txTask(
    void                        *ptr_argv)
{
    NPS_ERROR_NO_T              rc = NPS_E_OK;
    UI32_T                      unit     = ((PERF_COOKIE_T *)ptr_argv)->unit;
    UI32_T                      channel  = ((PERF_COOKIE_T *)ptr_argv)->channel;
    UI32_T                      len      = ((PERF_COOKIE_T *)ptr_argv)->len;
    UI32_T                      num      = ((PERF_COOKIE_T *)ptr_argv)->num;
    UI32_T                      port     = ((PERF_COOKIE_T *)ptr_argv)->port;
    BOOL_T                      test_skb = ((PERF_COOKIE_T *)ptr_argv)->test_skb;

    /* test targets */
    PERF_TX_SW_GPD              *ptr_sw_gpd = NULL;
    struct sk_buff              *ptr_skb = NULL;

    /* temp variables */
    UI32_T                      send_fail = 0;
    void                        *ptr_virt_addr = NULL;
    NPS_ADDR_T                  phy_addr = 0x0;

    osal_initRunThread();
    do
    {
        rc = osal_waitEvent(&_perf_tx_perf_cb.start_sync[channel]);
        if (NPS_E_OK != osal_isRunThread())
        {
            break; /* deinit-thread */
        }

        while (_perf_tx_perf_cb.send_ok[channel] < num)
        {
            if (0 == (_perf_tx_perf_cb.send_ok[channel] % PERF_TX_PERF_MESG))
            {
                printk("T");
            }

            if (TRUE == test_skb)
            {
                ptr_skb = osal_skb_alloc(len);
                ptr_skb->len = len;
                _perf_tx_perf_cb.get_netdev(unit, port, &ptr_skb->dev);

                /* send skb */
                osal_skb_send(ptr_skb);
            }
            else
            {
                ptr_sw_gpd = osal_alloc(sizeof(PERF_TX_SW_GPD));
                if (NULL == ptr_sw_gpd)
                {
                    osal_printf("***Error***, alloc sw-gpd fail.\n");
                    break;
                }

                /* prepare buf */
                ptr_virt_addr = osal_dma_alloc(len);
                phy_addr = osal_dma_convertVirtToPhy(ptr_virt_addr);

                /* trans skb to gpd */
                osal_memset(ptr_sw_gpd, 0x0, sizeof(PERF_TX_SW_GPD));
                ptr_sw_gpd->callback   = (void *)_perf_txCallback;
                ptr_sw_gpd->ptr_cookie = (void *)ptr_virt_addr;
                ptr_sw_gpd->gpd_num    = 1;
                ptr_sw_gpd->ptr_next   = NULL;
                ptr_sw_gpd->channel    = channel;

                /* prepare gpd */
                rc = _perf_tx_perf_cb.prepare_gpd(unit, phy_addr, len, port, ptr_sw_gpd);

                /* send gpd */
                rc = _perf_tx_perf_cb.send_gpd(unit, channel, ptr_sw_gpd);
                if (NPS_E_OK == rc)
                {
                    _perf_tx_perf_cb.send_ok[channel]++;
                    send_fail = 0;
                }
                else
                {
                    _perf_tx_perf_cb.send_fail[channel]++;
                    if (send_fail++ >= PERF_TX_PERF_FAIL)
                    {
                        osal_printf("***Error***, Tch-%d send fail over %d packet(s). (rc: %d)\n",
                            channel, PERF_TX_PERF_FAIL, rc);
                        break;
                    }

                    _perf_txCallback(unit, ptr_sw_gpd, ptr_virt_addr);
                    osal_sleepThread(1000);
                }
            }
        }

        osal_triggerEvent(&_perf_tx_perf_cb.end_sync[channel]);
    }
    while (NPS_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

static void
_perf_txDeinit(
    const UI32_T                unit,
    const UI32_T                tx_channel)
{
    UI32_T                      channel = 0;

    for (channel = 0; channel < tx_channel; channel++)
    {
        /* destroy Tx resources */
        osal_stopThread   (&_perf_tx_perf_cb.tx_task    [channel]);
        osal_triggerEvent(&_perf_tx_perf_cb.start_sync [channel]);
        osal_destroyThread(&_perf_tx_perf_cb.tx_task    [channel]);
        osal_destroyEvent(&_perf_tx_perf_cb.end_sync   [channel]);
        osal_destroyEvent(&_perf_tx_perf_cb.start_sync [channel]);
    }
}

static void
_perf_txInit(
    const UI32_T                unit,
    const UI32_T                tx_channel,
    const UI32_T                len,
    BOOL_T                      test_skb)
{
    UI32_T                      channel = 0;

    for (channel = 0; channel < tx_channel; channel++)
    {
        _perf_tx_perf_cb.send_ok  [channel] = 0;
        _perf_tx_perf_cb.send_fail[channel] = 0;

        /* create Tx resources */
        osal_createEvent("TX_START", &_perf_tx_perf_cb.start_sync [channel]);
        osal_createEvent("TX_END",   &_perf_tx_perf_cb.end_sync   [channel]);

        _perf_tx_perf_cb.tx_cookie[channel].unit     = unit;
        _perf_tx_perf_cb.tx_cookie[channel].channel  = channel;
        _perf_tx_perf_cb.tx_cookie[channel].len      = len;
        _perf_tx_perf_cb.tx_cookie[channel].num      = PERF_TX_PERF_NUM / tx_channel;
        _perf_tx_perf_cb.tx_cookie[channel].port     = 0;
        _perf_tx_perf_cb.tx_cookie[channel].test_skb = test_skb;

        osal_createThread(
            "TX_PERF", 64 * 1024, 90,
            _perf_txTask,
            (void *)&_perf_tx_perf_cb.tx_cookie[channel],
            &_perf_tx_perf_cb.tx_task[channel]);
     }
}

static void
_perf_rxDeinit(
    const UI32_T                unit,
    const UI32_T                rx_channel)
{
    /* turn-off Rx test */
    _perf_rx_perf_cb.rx_test = FALSE;

    /* destroy Rx resources */
    osal_destroyEvent(&_perf_rx_perf_cb.end_sync);
    osal_destroyEvent(&_perf_rx_perf_cb.start_sync);

    /* disable duplicate Rx packets to channels */
    _perf_duplicateRxPacket(unit, rx_channel, FALSE);
}

static void
_perf_rxInit(
    const UI32_T                unit,
    const UI32_T                rx_channel,
    const UI32_T                len)
{
    /* enable duplicate Rx packets to channels */
    _perf_duplicateRxPacket(unit, rx_channel, TRUE);

    /* create Rx callback resources */
    _perf_rx_perf_cb.target_num = PERF_RX_PERF_NUM;
    _perf_rx_perf_cb.target_len = len;
    _perf_rx_perf_cb.recv_pass = 0;

    osal_createEvent("RX_START", &_perf_rx_perf_cb.start_sync);
    osal_createEvent("RX_END",   &_perf_rx_perf_cb.end_sync);

    /* turn-on Rx test */
    _perf_rx_perf_cb.rx_test = TRUE;
}

/* FUNCTION NAME: perf_rxCallback
 * PURPOSE:
 *      To count the Rx-gpd for Rx-test.
 * INPUT:
 *      len         -- To check if the Rx-gpd length equals to test length.
 * OUTPUT:
 *      None
 * RETURN:
 *      NPS_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
NPS_ERROR_NO_T
perf_rxCallback(
    const UI32_T                len)
{
    /* check length */
    if (len == _perf_rx_perf_cb.target_len)
    {
        _perf_rx_perf_cb.recv_pass++;
    }
    else
    {
        if (_perf_rx_perf_cb.recv_fail++ >= PERF_RX_PERF_FAIL)
        {
            _perf_rx_perf_cb.recv_fail = 0;
        }
    }

    /* send signals */
    if (0 == _perf_rx_perf_cb.recv_pass)
    {
        ; /* do nothing */
    }
    else if (1 == _perf_rx_perf_cb.recv_pass)
    {
        osal_triggerEvent(&_perf_rx_perf_cb.start_sync);
    }
    else if (_perf_rx_perf_cb.recv_pass == _perf_rx_perf_cb.target_num)
    {
        osal_triggerEvent(&_perf_rx_perf_cb.end_sync);
    }
    else if (0 == (_perf_rx_perf_cb.recv_pass % PERF_RX_PERF_MESG))
    {
        printk("R");
    }

    return (NPS_E_OK);
}

/* FUNCTION NAME: perf_rxTest
 * PURPOSE:
 *      To check if Rx-test is going.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      NPS_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
NPS_ERROR_NO_T
perf_rxTest(void)
{
    if (FALSE == _perf_rx_perf_cb.rx_test)
    {
        return (NPS_E_OTHERS);
    }

    return (NPS_E_OK);
}

/* FUNCTION NAME: perf_test
 * PURPOSE:
 *      To do Tx-test or Rx-test.
 * INPUT:
 *      len         -- Test length
 *      tx_channel  -- Test Tx channel numbers
 *      rx_channel  -- Test Rx channel numbers
 *      test_skb    -- Test GPD or SKB
 * OUTPUT:
 *      None
 * RETURN:
 *      NPS_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
NPS_ERROR_NO_T
perf_test(
    UI32_T                      len,
    UI32_T                      tx_channel,
    UI32_T                      rx_channel,
    BOOL_T                      test_skb)
{
    NPS_ERROR_NO_T              rc = NPS_E_OK;
    NPS_TIME_T                  start_time;
    NPS_TIME_T                  end_time;
    UI32_T                      unit = 0, channel = 0;
    UI32_T                      tx_pkt_cnt = 0, tx_start_intr = 0, tx_end_intr = 0;
    UI32_T                      rx_pkt_cnt = 0, rx_start_intr = 0, rx_end_intr = 0;

    if ((0 == tx_channel) && (0 == rx_channel))
    {
        return (NPS_E_NOT_SUPPORT);
    }

    /* start test */
    if ((tx_channel > 0) && (rx_channel > 0))
    {
        _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_start_intr);
        _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_start_intr);
        _perf_txInit(unit, tx_channel, len, test_skb);
        _perf_rxInit(unit, rx_channel, len);

        /* wait 1st Rx GPD done */
        osal_waitEvent(&_perf_rx_perf_cb.start_sync);

        /* ------------- in-time ------------- */
        osal_getTime(&start_time);
        for (channel = 0; channel < tx_channel; channel++)
        {
            osal_triggerEvent(&_perf_tx_perf_cb.start_sync[channel]);
        }
        for (channel = 0; channel < tx_channel; channel++)
        {
            osal_waitEvent(&_perf_tx_perf_cb.end_sync[channel]);
            tx_pkt_cnt += _perf_tx_perf_cb.send_ok[channel];
        }
        rx_pkt_cnt = _perf_rx_perf_cb.recv_pass;
        osal_getTime(&end_time);
        /* ------------- in-time ------------- */

        _perf_txDeinit(unit, tx_channel);
        _perf_rxDeinit(unit, rx_channel);
        _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_end_intr);
        _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_end_intr);

        _perf_showPerf(PERF_DIR_TX,
            tx_channel, len, tx_pkt_cnt, tx_end_intr - tx_start_intr, end_time - start_time);

        _perf_showPerf(PERF_DIR_RX,
            rx_channel, len, rx_pkt_cnt, rx_end_intr - rx_start_intr, end_time - start_time);
    }
    else if (tx_channel > 0)
    {
        _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_start_intr);
        _perf_txInit(unit, tx_channel, len, test_skb);

        /* ------------- in-time ------------- */
        osal_getTime(&start_time);
        for (channel = 0; channel < tx_channel; channel++)
        {
            osal_triggerEvent(&_perf_tx_perf_cb.start_sync[channel]);
        }
        for (channel = 0; channel < tx_channel; channel++)
        {
            osal_waitEvent(&_perf_tx_perf_cb.end_sync[channel]);
            tx_pkt_cnt += _perf_tx_perf_cb.send_ok[channel];
        }
        osal_getTime(&end_time);
        /* ------------- in-time ------------- */

        _perf_txDeinit(unit, tx_channel);
        _perf_getIntrCnt(unit, PERF_DIR_TX, &tx_end_intr);

        _perf_showPerf(PERF_DIR_TX,
            tx_channel, len, tx_pkt_cnt, tx_end_intr - tx_start_intr, end_time - start_time);
    }
    else if (rx_channel > 0)
    {
        _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_start_intr);
        _perf_rxInit(unit, rx_channel, len);

        /* wait 1st Rx GPD done */
        osal_waitEvent(&_perf_rx_perf_cb.start_sync);

        /* ------------- in-time ------------- */
        osal_getTime(&start_time);
        osal_waitEvent(&_perf_rx_perf_cb.end_sync);
        osal_getTime(&end_time);
        /* ------------- in-time ------------- */

        _perf_rxDeinit(unit, rx_channel);
        _perf_getIntrCnt(unit, PERF_DIR_RX, &rx_end_intr);

        _perf_showPerf(PERF_DIR_RX,
            rx_channel, len, PERF_RX_PERF_NUM, rx_end_intr - rx_start_intr, end_time - start_time);
    }

    return (rc);
}

