/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  netif_perf.h
 * PURPOSE:
 *      It provide customer performance test API.
 * NOTES:
 */

#ifndef NETIF_PERF_H
#define NETIF_PERF_H

/* #define PERF_EN_TEST */

/* FUNCTION NAME: perf_rxCallback
 * PURPOSE:
 *      To count the Rx-gpd for Rx-test.
 * INPUT:
 *      len         -- To check if the Rx-gpd length equals to test length.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_rxCallback(
    const UI32_T                len);

/* FUNCTION NAME: perf_rxTest
 * PURPOSE:
 *      To check if Rx-test is going.
 * INPUT:
 *      None
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_rxTest(
    void);

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
 *      CLX_E_OK    -- Successful operation.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
perf_test(
    UI32_T                      len,
    UI32_T                      tx_channel,
    UI32_T                      rx_channel,
    BOOL_T                      test_skb);

#endif /* end of NETIF_PERF_H */
