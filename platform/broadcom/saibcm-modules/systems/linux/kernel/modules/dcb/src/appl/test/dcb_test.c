/* 
 * $Id:$
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
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
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * File:        test.c
 * Purpose:     DCB Library Example
 *
 */

#include <appl/dcb/dcb_int.h>
#include <appl/dcb/dcb_handler.h>

int dcb_example()
{
    int rv;
    dcb_handle_t dcb_handle;
    int device_id = 0xb820;
    int rev_id = 0;
    int dcb_type, dcb_size;
    uint8 dcb_buffer[256];
    dcb_t *dcb = (dcb_t *)dcb_buffer;
    int value;
    int dt_mode;
    int ingport_is_hg;
    soc_rx_reasons_t reasons;
    soc_rx_reason_t reason;
    dcb_handle_t  *dcb_null = NULL;
    
    /* Error condition */
    rv = dcb_handle_init(dcb_null, device_id, rev_id);
    printf("Error check: null handle. dcb_handle_init = %d\n", rv);

    rv = dcb_handle_init(&dcb_handle, 0xffff, 0xff);
    printf("Error check: invalid device. dcb_handle_init = %d\n", rv);

    /* Init DCB library */
    printf("\n\nEnter device ID in hex (e.g. b860 for Trident 2+): ");
    scanf("%x", &device_id);
    rv = dcb_handle_init(&dcb_handle, device_id, rev_id);
    printf("For device ID 0x%04X, dcb_handle_init = %d\n", device_id, rv);

    if (rv != DCB_OK) {
        return 1;
    }

    /* Get DCB type */
    dcb_type = DCB_TYPE(&dcb_handle);
    printf("DCB type %d\n", dcb_type);

    /* Get DCB size */
    dcb_size = DCB_SIZE(&dcb_handle);
    printf("DCB size %d\n", dcb_size);

    DCB_INIT(&dcb_handle, dcb);

    printf("\n");
    DCB_REQCOUNT_SET(&dcb_handle, dcb, 5);
    value = DCB_REQCOUNT_GET(&dcb_handle, dcb);
    printf("DCB_REQCOUNT_GET %d\n", value);

    value = DCB_XFERCOUNT_GET(&dcb_handle, dcb);
    printf("DCB_XFERCOUNT_GET %d\n", value);

    DCB_DONE_SET(&dcb_handle, dcb, 1);
    printf("DCB_DONE_GET %d\n", DCB_DONE_GET(&dcb_handle, dcb));

    DCB_SG_SET(&dcb_handle, dcb, 1);
    printf("DCB_SG_GET %d\n", DCB_SG_GET(&dcb_handle, dcb));

    DCB_CHAIN_SET(&dcb_handle, dcb, 1);
    printf("DCB_CHAIN_GET %d\n", DCB_CHAIN_GET(&dcb_handle, dcb));

    DCB_RELOAD_SET(&dcb_handle, dcb, 1);
    printf("DCB_RELOAD_GET %d\n", DCB_RELOAD_GET(&dcb_handle, dcb));

    printf("\n");
    DCB_TX_CRC_SET(&dcb_handle, dcb, 1);
    DCB_TX_COS_SET(&dcb_handle, dcb, 4);
    DCB_TX_DESTMOD_SET(&dcb_handle, dcb, 2);
    DCB_TX_DESTPORT_SET(&dcb_handle, dcb, 8);
    DCB_TX_OPCODE_SET(&dcb_handle, dcb, 3);
    DCB_TX_SRCMOD_SET(&dcb_handle, dcb, 1);
    DCB_TX_SRCPORT_SET(&dcb_handle, dcb, 7);
    DCB_TX_PRIO_SET(&dcb_handle, dcb, 4);
    DCB_TX_PFM_SET(&dcb_handle, dcb, 1);

    printf("\n");
    dt_mode = 0;
    ingport_is_hg = 0;
    value = DCB_RX_UNTAGGED_GET(&dcb_handle, dcb, dt_mode, ingport_is_hg);
    printf("DCB_RX_UNTAGGED_GET 0x%x (%d)\n", value, value);
          
    dt_mode = 0;
    ingport_is_hg = 1;
    value = DCB_RX_UNTAGGED_GET(&dcb_handle, dcb, dt_mode, ingport_is_hg);
    printf("DCB_RX_UNTAGGED_GET 0x%x (%d)\n", value, value);

    printf("\n");
    printf("DCB_RX_CRC_GET %d\n", DCB_RX_CRC_GET(&dcb_handle, dcb));
    printf("DCB_RX_COS_GET %d\n", DCB_RX_COS_GET(&dcb_handle, dcb));
    printf("DCB_RX_DESTMOD_GET %d\n", DCB_RX_DESTMOD_GET(&dcb_handle, dcb));
    printf("DCB_RX_DESTPORT_GET %d\n", DCB_RX_DESTPORT_GET(&dcb_handle, dcb));
    printf("DCB_RX_OPCODE_GET %d\n", DCB_RX_OPCODE_GET(&dcb_handle, dcb));

    printf("\n");
    printf("DCB_RX_CLASSTAG_GET %d\n", DCB_RX_CLASSTAG_GET(&dcb_handle, dcb));
    printf("DCB_RX_MATCHRULE_GET %d\n", DCB_RX_MATCHRULE_GET(&dcb_handle, dcb));
    printf("DCB_RX_START_GET %d\n", DCB_RX_START_GET(&dcb_handle, dcb));
    printf("DCB_RX_END_GET %d\n", DCB_RX_END_GET(&dcb_handle, dcb));
    printf("DCB_RX_ERROR_GET %d\n", DCB_RX_ERROR_GET(&dcb_handle, dcb));
    printf("DCB_RX_PRIO_GET %d\n", DCB_RX_PRIO_GET(&dcb_handle, dcb));

    printf("\n");
    printf("DCB_RX_REASON_GET 0x%x\n", DCB_RX_REASON_GET(&dcb_handle, dcb));
    printf("DCB_RX_REASON_HI_GET 0x%x\n",
           DCB_RX_REASON_HI_GET(&dcb_handle, dcb));
    printf("DCB_RX_REASONS_GET\n");
    DCB_RX_REASONS_GET(&dcb_handle, dcb, &reasons);
    _SHR_RX_REASON_ITER(reasons, reason) {
        printf("    %d\n", reason);
    }
    printf("\n");
    printf("DCB_RX_INGPORT_GET %d\n", DCB_RX_INGPORT_GET(&dcb_handle, dcb));
    printf("DCB_RX_SRCPORT_GET %d\n", DCB_RX_SRCPORT_GET(&dcb_handle, dcb));
    printf("DCB_RX_SRCMOD_GET %d\n", DCB_RX_SRCMOD_GET(&dcb_handle, dcb));

    printf("\n");
    printf("DCB_RX_MCAST_GET %d\n", DCB_RX_MCAST_GET(&dcb_handle, dcb));
    printf("DCB_RX_VCLABEL_GET %d\n", DCB_RX_VCLABEL_GET(&dcb_handle, dcb));
    printf("DCB_RX_MIRROR_GET %d\n", DCB_RX_MIRROR_GET(&dcb_handle, dcb));
    printf("DCB_RX_MATCHRULE_GET %d\n", DCB_RX_MATCHRULE_GET(&dcb_handle, dcb));
    printf("DCB_RX_TIMESTAMP_GET %d\n", DCB_RX_TIMESTAMP_GET(&dcb_handle, dcb));
    printf("DCB_RX_TIMESTAMP_UPPER_GET %d\n",
           DCB_RX_TIMESTAMP_UPPER_GET(&dcb_handle, dcb));

    printf("\n");
    DCB_HG_SET(&dcb_handle, dcb, 1);
    printf("DCB_HG_GET %d\n", DCB_HG_GET(&dcb_handle, dcb));
    DCB_STAT_SET(&dcb_handle, dcb, 1);
    printf("DCB_STAT_GET %d\n", DCB_STAT_GET(&dcb_handle, dcb));
    DCB_PURGE_SET(&dcb_handle, dcb, 1);
    printf("DCB_PURGE_GET %d\n", DCB_PURGE_GET(&dcb_handle, dcb));
         
    printf("\n");
    printf("DCB_MHP_GET 0x%lx\n",
           (((uintptr_t)(DCB_MHP_GET(&dcb_handle, dcb))) & 0xffffffff));
    printf("DCB_RX_OUTER_VID_GET %d\n",
           DCB_RX_OUTER_VID_GET(&dcb_handle, dcb));
    printf("DCB_RX_OUTER_PRI_GET %d\n",
           DCB_RX_OUTER_PRI_GET(&dcb_handle, dcb));
    printf("DCB_RX_OUTER_CFI_GET %d\n",
           DCB_RX_OUTER_CFI_GET(&dcb_handle, dcb));
    printf("DCB_RX_OUTER_TAG_ACTION_GET %d\n",
           DCB_RX_OUTER_TAG_ACTION_GET(&dcb_handle, dcb));
    printf("DCB_RX_INNER_VID_GET %d\n",
           DCB_RX_INNER_VID_GET(&dcb_handle, dcb));
    printf("DCB_RX_INNER_PRI_GET %d\n",
           DCB_RX_INNER_PRI_GET(&dcb_handle, dcb));
    printf("DCB_RX_INNER_CFI_GET %d\n",
           DCB_RX_INNER_CFI_GET(&dcb_handle, dcb));
    printf("DCB_RX_INNER_TAG_ACTION_GET %d\n",
           DCB_RX_INNER_TAG_ACTION_GET(&dcb_handle, dcb));
    printf("DCB_RX_BPDU_GET %d\n",
           DCB_RX_BPDU_GET(&dcb_handle, dcb));
    printf("DCB_RX_L3_INTF_GET %d\n",\
           DCB_RX_L3_INTF_GET(&dcb_handle, dcb));

    return rv;
}

int main()
{
    dcb_example();

    return 0;
}

