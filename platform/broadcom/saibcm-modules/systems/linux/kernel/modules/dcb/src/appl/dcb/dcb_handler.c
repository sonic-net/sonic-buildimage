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
 * File:        dcb.c
 * Purpose:     DCB Library
 *
 */

#include <sal/types.h>
#include <soc/drv.h>
#include <soc/dcb.h>
#include <soc/devids.h>
#include <appl/dcb/dcb_int.h>
#include <appl/dcb/dcb_handler.h>

soc_control_t       soc_control;

#define DEVICE_FAMILY(_dev)    ((_dev) & 0xfff0)

#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
extern dcb_op_t dcb23_op;
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
extern dcb_op_t dcb26_op;
#endif
#if defined(BCM_KATANA2_SUPPORT)
extern dcb_op_t dcb29_op;
#endif
#if defined(BCM_HURRICANE2_SUPPORT)
extern dcb_op_t dcb30_op;
#endif
#if defined(BCM_GREYHOUND_SUPPORT)
extern dcb_op_t dcb31_op;
#endif
#if defined(BCM_TOMAHAWK_SUPPORT) || defined(BCM_TOMAHAWK2_SUPPORT)
extern dcb_op_t dcb32_op;
#endif
#if defined(BCM_TRIDENT2PLUS_SUPPORT)
extern dcb_op_t dcb33_op;
#endif
#if defined(BCM_HURRICANE3_SUPPORT)
extern dcb_op_t dcb34_op;
#endif
#if defined(BCM_APACHE_SUPPORT)
extern dcb_op_t dcb35_op;
#endif
#if defined(BCM_TRIDENT3_SUPPORT)
extern dcb_op_t dcb36_op;
#endif
#if defined(BCM_GREYHOUND2_SUPPORT)
extern dcb_op_t dcb37_op;
#endif
#if defined(BCM_TOMAHAWK3_SUPPORT)
extern dcb_op_t dcb38_op;
#endif

/*
 * Function:
 *      _dcb_op_get
 * Description:
 *      Get DCB operation structure for a given device type and revision ID.
 * Parameters:
 *      dev_id  - (IN) Device ID
 *      rev_id  - (IN) Revision ID
 * Return:
 *      !NULL    Pointer to DCB operations object
 *       NULL    Failure
 */
static dcb_op_t *
_dcb_op_get(uint16 dev_id, uint8 rev_id)
{
    switch(dev_id) {
#if defined(BCM_TRIUMPH3_SUPPORT) || defined(BCM_KATANA_SUPPORT) || \
    defined(BCM_TRIDENT2_SUPPORT)
    case DEVICE_FAMILY(BCM56640_DEVICE_ID):    /* Triumph 3 */
    case DEVICE_FAMILY(BCM56340_DEVICE_ID):    /* Helix 4 */
        return &dcb23_op;
        break;
#endif
#if defined(BCM_TRIDENT2_SUPPORT)
    case DEVICE_FAMILY(BCM56850_DEVICE_ID):    /* Trident 2 */
        return &dcb26_op;
        break;
#endif
#if defined(BCM_KATANA2_SUPPORT)  
    case DEVICE_FAMILY(BCM56450_DEVICE_ID):    /* Katana 2 */
        return &dcb29_op;
        break;
#endif
#if defined(BCM_HURRICANE2_SUPPORT)  
    case DEVICE_FAMILY(BCM56150_DEVICE_ID):    /* Hurricane 2 */
        return &dcb30_op;
        break;
#endif
#if defined(BCM_GREYHOUND_SUPPORT)  
    case DEVICE_FAMILY(BCM53400_DEVICE_ID):    /* Greyhound */
        return &dcb31_op;
        break;
#endif
#if defined(BCM_TOMAHAWK_SUPPORT) || defined(BCM_TOMAHAWK2_SUPPORT)
    case DEVICE_FAMILY(BCM56960_DEVICE_ID):    /* Tomahawk */
    case DEVICE_FAMILY(BCM56970_DEVICE_ID):    /* Tomahawk 2 */
        return &dcb32_op;
        break;
#endif
#if defined(BCM_TRIDENT2PLUS_SUPPORT)
    case DEVICE_FAMILY(BCM56860_DEVICE_ID):    /* Trident 2 plus*/
        return &dcb33_op;
        break;
#endif
#if defined(BCM_HURRICANE3_SUPPORT)  
    case DEVICE_FAMILY(BCM56160_DEVICE_ID):    /* Hurricane 3 */
        return &dcb34_op;
        break;
#endif
#if defined(BCM_APACHE_SUPPORT)  
    case DEVICE_FAMILY(BCM56560_DEVICE_ID):    /* Apache */
    case DEVICE_FAMILY(BCM56760_DEVICE_ID):    /* Maverick */
        return &dcb35_op;
        break;
#endif
#if defined(BCM_TRIDENT3_SUPPORT)
    case DEVICE_FAMILY(BCM56870_DEVICE_ID):    /* Trident 3*/
    case DEVICE_FAMILY(BCM56770_DEVICE_ID):    /* Maverick 2*/
    case DEVICE_FAMILY(BCM56370_DEVICE_ID):    /* Helix 5*/
    case DEVICE_FAMILY(BCM56470_DEVICE_ID):    /* Firebolt 6*/
    case DEVICE_FAMILY(BCM56275_DEVICE_ID):    /* Hurricane 4*/
    case DEVICE_FAMILY(BCM56175_DEVICE_ID):    /* Trident3 X1*/
        return &dcb36_op;
        break;
#endif
#if defined(BCM_GREYHOUND2_SUPPORT)  
    case DEVICE_FAMILY(BCM53570_DEVICE_ID):    /* Greyhound 2 */
        return &dcb37_op;
        break;
#endif
#if defined(BCM_TOMAHAWK3_SUPPORT)
    case DEVICE_FAMILY(BCM56980_DEVICE_ID):    /* Tomahawk 3 */
        return &dcb38_op;
        break;
#endif
    default:
        return NULL;
        break;
    }

    return NULL;
}

/*
 * Function:
 *      _dcb_htonl
 * Description:
 *      Returns the value in network byte order.
 * Parameters:
 *      value - (IN) Value
 * Return:
 *      Value in network byte order
 */
uint32
_dcb_htonl(uint32 value)
{
    uint32 new_value;
    uint8  *buf =(uint8 *)&new_value;

    buf[0] = (value >> 24) & 0xff;
    buf[1] = (value >> 16) & 0xff;
    buf[2] = (value >> 8) & 0xff;
    buf[3] = value & 0xff;

    return new_value;
}

/*
 * Function:
 *      dcb_handle_init
 * Description:
 *      Initialize a DCB library handle for a given device type and revision.
 *      It must be called before calling any of the macros.
 * Parameters:
 *      handle  - (IN/OUT) Pointer to DCB library handle for given device
 *      dev_id  - (IN) Device ID
 *      rev_id  - (IN) Revision ID
 * Return:
 *      DCB_OK     Success
 *      DCB_ERROR  Failure
 * Notes:
 *      Each device type and revision (in some) needs to have a unique
 *      DCB library handle associated with it.
 */
int
dcb_handle_init(dcb_handle_t *handle, uint16 dev_id, uint8 rev_id)
{
    dcb_op_t  *dcb_op;

    if (handle == NULL) {
        return DCB_ERROR;
    }

    handle->dcb_op = NULL;

    /* Get DCB operations */
    /* First use given specific device ID */
    if ((dcb_op = _dcb_op_get(dev_id, rev_id)) == NULL) {
        /* Try family device */
        if ((dcb_op = _dcb_op_get(DEVICE_FAMILY(dev_id), rev_id)) == NULL) {
            return DCB_ERROR;
        }
    }

    handle->dcb_op = dcb_op;

    return DCB_OK;
}

