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

/* FILE NAME:  netif_pkt_knl.h
 * PURPOSE:
 *
 * NOTES:
 *
 */
#ifndef NETIF_PKT_NL_H
#define NETIF_PKT_NL_H

/* INCLUDE FILE DECLARTIONS
 */
#include <linux/fs.h>
#include <linux/netdevice.h>

#include <clx_types.h>
#include <clx_error.h>
#include <clx_pkt.h>

CLX_ERROR_NO_T
hal_lt_dawn_pkt_getNetDev(const UI32_T unit, const UI32_T port, struct net_device **pptr_net_dev);

CLX_ERROR_NO_T
hal_lt_dawn_pkt_dev_tx(const UI32_T unit, void *ptr_data);

long
hal_lt_dawn_pkt_dev_ioctl(const UI32_T unit);

CLX_ERROR_NO_T
hal_lt_lightning_pkt_getNetDev(const UI32_T unit,
                               const UI32_T port,
                               struct net_device **pptr_net_dev);

CLX_ERROR_NO_T
hal_lt_lightning_pkt_dev_tx(const UI32_T unit, void *ptr_data);

long
hal_lt_lightning_pkt_dev_ioctl(const UI32_T unit);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getNetDev(const UI32_T unit,
                                 const UI32_T port,
                                 struct net_device **pptr_net_dev);

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_dev_tx(const UI32_T unit, void *ptr_data);

long
hal_mt_namchabarwa_pkt_dev_ioctl(const UI32_T unit);

#endif /* End of NETIF_PKT_KNL_H */
