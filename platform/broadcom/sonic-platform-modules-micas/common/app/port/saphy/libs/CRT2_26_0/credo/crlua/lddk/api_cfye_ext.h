/** @file api_cfye_ext.h
 *
 * @brief Classification Engine (CfyE) API. extended functions.
 *
 * This API can be used to read the installed vPorts and Rules.
 */

/* -------------------------------------------------------------------------- */
/*                                                                            */
/*   Module        : ddk164                                                   */
/*   Version       : 3.1.1                                                    */
/*   Configuration : DDK-164-IPSEC-CREDO-BSD                                  */
/*                                                                            */
/*   Date          : 2021-Aug-12                                              */
/*                                                                            */
/* Copyright (c) 2008-2021 by Rambus, Inc. and/or its subsidiaries.           */
/*                                                                            */
/* Redistribution and use in source and binary forms, with or without         */
/* modification, are permitted provided that the following conditions are     */
/* met:                                                                       */
/*                                                                            */
/* 1. Redistributions of source code must retain the above copyright          */
/* notice, this list of conditions and the following disclaimer.              */
/*                                                                            */
/* 2. Redistributions in binary form must reproduce the above copyright       */
/* notice, this list of conditions and the following disclaimer in the        */
/* documentation and/or other materials provided with the distribution.       */
/*                                                                            */
/* 3. Neither the name of the copyright holder nor the names of its           */
/* contributors may be used to endorse or promote products derived from       */
/* this software without specific prior written permission.                   */
/*                                                                            */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS        */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR      */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT       */
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT           */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,      */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      */
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE      */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.       */
/* -------------------------------------------------------------------------- */

#ifndef API_CFYE_EXT_H_
#define API_CFYE_EXT_H_

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

/* Driver Framework Basic Definitions API */
#include "basic_defs.h"             /* uint32_t */

/* Basic CfyE API. */
#include "api_cfye.h"


/**---------------------------------------------------------------------------
 * @fn CfyE_Device_Role_Get(
 *       const unsigned int DeviceId,
 *       CfyE_Role_t * const Role_p);
 *
 * Read the role of the device (ingress or egress).
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [out] Role_p
 *      The device role.
 *
 * This function is re-entrant for the same or different Devices.
 * This function can be called concurrently with any other function for
 * the same or different DeviceId.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Device_Role_Get(
        const unsigned int DeviceId,
        CfyE_Role_t * const Role_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPort_Read(
 *       const unsigned int DeviceId,
 *       const CfyE_vPortHandle_t vPortHandle,
 *       CfyE_vPort_t * const vPort_p);
 *
 * Read the vPort policy of the given vPort.
 *
 * API use order:
 *       A vPort can be read from a classification device instance only after
 *       this vPort has been added to the device via the CfyE_vPort_Add()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] vPortHandle
 *      vPort handle for vPort to be updated.
 *
 * @param [out] vPort_p
 *      Pointer to a memory location where the data for the vPort is stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed. In particular this function can only be called for a valid
 * vPort handle, returned by CfyE_vPort_Add(), which is not yet removed with
 * CfyE_vPort_Remove().
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_vPort_Read(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        CfyE_vPort_t * const vPort_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_MTT_Read(
 *       const unsigned int DeviceId,
 *       const unsigned int MTTIndex,
 *       CfyE_MTT_t * const MTT_p,
 *       bool * const fEnabled_p);
 *
 * Read the MTT matching rule at a given index,.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] MTTIndex
 *      MTT index of the MTT rule to be read.
 *
 * @param [out] MTT_p
 *      Pointer to a memory location where the data for the vPort is stored.
 *
 * @param [out] fEnabled_p
 *      Indicate if the MTT entry is enabled.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 * *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_MTT_Read(
        const unsigned int DeviceId,
        const unsigned int MTTIndex,
        CfyE_MTT_t * const MTT_p,
        bool * const fEnabled_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_vPort_Next_Get(
 *       const unsigned int DeviceId,
 *       const CfyE_vPortHandle_t CurrentvPortHandle,
 *       CfyE_vPortHandle_t * const NextvPortHandle_p);
 *
 * Return the next vPort Handle that was allocated after a given vPort
 * Handle for a given device.  Use this together with to produce a
 * list of all allocated vPorts.  Return CfyE_vPortHandle_NULL if the
 * given vPort handle is the last one allocated.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] CurrentvPortHandle
 *      vPort handle returned by the last call to CfyE_vPortHandle_Next_Get or
 *      CfyE_vPortHandle_NULL for the first call to this function.
 *
 * @param [out] NextvPortHandle_p
 *      vPort handle of the next allocated vPort.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * Note: a sequence of calls to this function is often performed to
 *       obtain the list of all allocated vPorts. During this sequence
 *       of calls, no calls to CfyE_vPort_Add() and
 *       CfyE_vPort_Remove() shall be made for the same device,
 *       otherwise the list is inconsistent or calls may fail.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_vPort_Next_Get(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t CurrentvPortHandle,
        CfyE_vPortHandle_t * const NextvPortHandle_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Read(
 *       const unsigned int DeviceId,
 *       const CfyE_RuleHandle_t RuleHandle,
 *       CfyE_Rule_t * const Rule_p,
 *       bool * const fEnabled_p);
 *
 * Read a packet matching rule for one classification device instance
 * identified by DeviceId parameter.
 *
 * API use order:
 *       A rule can be read from a classification device instance only after
 *       it has been added with CfyE_Rule_Add().
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] RuleHandle
 *      Rule handle for rule to be read.
 *
 * @param [out] Rule_p
 *      Pointer to a memory location where the data for the rule is stored.
 *
 * @param [out] fEnabled_p
 *      Pointer to a flag indicating whether the rule was enabled.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed. In particular this function can only be called for a valid
 * Rule handle, returned by CfyE_Rule_Add(), which is not yet removed with
 * CfyE_Rule_Remove().
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Rule_Read(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle,
        CfyE_Rule_t * const Rule_p,
        bool * const fEnabled_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Rule_Next_Get(
 *       const unsigned int DeviceId,
 *       const CfyE_RuleHandle_t CurrentRuleHandle,
 *       CfyE_RuleHandle_t * const NextRuleHandle_p);
 *
 * Return the next Rule Handle that was allocated after a given Rule
 * Handle for a given device.  Use this to produce a list of all
 * allocated Rules.  Return CfyE_RuleHandle_NULL if the given Rule
 * handle is the last one allocated.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] CurrentRuleHandle
 *      Rule handle returned by the last call to CfyE_RuleHandle_Next_Get or
 *      CfyE_RuleHandle_NULL for the first call to this function.
 *
 * @param [out] NextRuleHandle_p
 *      vPort handle of the next allocated Rule.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * Note: a sequence of calls to this function is often performed to
 *       obtain the list of all allocated Rules. During this sequence
 *       of calls, no calls to CfyE_Rule_Add() and CfyE_Rule_Remove()
 *       shall be made for the same device, otherwise the list is
 *       inconsistent or calls may fail.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Rule_Next_Get(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t CurrentRuleHandle,
        CfyE_RuleHandle_t * const NextRuleHandle_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Rule_vPort_Next_Get(
 *       const unsigned int DeviceId,
 *       const CfyE_vPortHandle_t vPortHandle,
 *       const CfyE_RuleHandle_t CurrentRuleHandle,
 *       CfyE_RuleHandle_t * const NextRuleHandle_p);
 *
 * Return the next Rule Handle that was allocated after a given Rule
 * Handle for a given device that are bound to a given vPort.
 *  Use this to produce a list of all
 * allocated Rules.  Return CfyE_RuleHandle_NULL if the given Rule
 * handle is the last one allocated.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] vPortHandle
 *      Handle of the vPort for which rules must be found.
 *
 * @param [in] CurrentRuleHandle
 *      Rule handle returned by the last call to CfyE_RuleHandle_Next_Get or
 *      CfyE_RuleHandle_NULL for the first call to this function.
 *
 * @param [out] NextRuleHandle_p
 *      vPort handle of the next allocated Rule.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * Note: a sequence of calls to this function is often performed to
 *       obtain the list of all allocated Rules. During this sequence
 *       of calls, no calls to CfyE_Rule_Add() and CfyE_Rule_Remove()
 *       shall be made for the same device, otherwise the list is
 *       inconsistent or calls may fail.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Rule_vPort_Next_Get(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        const CfyE_RuleHandle_t CurrentRuleHandle,
        CfyE_RuleHandle_t * const NextRuleHandle_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Diag_Device_Dump(
 *       const unsigned int DeviceId);
 *
 * Provide a diagnostics dump of all global device configuration and status
 * information. This includes:
 * - Device limits (number of vPorts, Rules, channels).
 * - Any information that can be obtained with CfyE_Device_Config_Get() that
 *   is not per-channel.
 * - Device ECC status.
 * - Various header parser state registers.
 * - Summary status registers.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with CfyE_Device_Update for
 * the same device.
 * This function can be called concurrently with any CfyE API function
 * for a different device.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Diag_Device_Dump(
        const unsigned int DeviceId);


/**---------------------------------------------------------------------------
 * @fn CfyE_Diag_Channel_Dump(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       const bool fAllChannels);
 *
 * Provide a diagnostics dump of all per-channel configuration and status
 * information. This includes:
 * - All information that can be obtained with CfyE_Device_Config_Get() that
 *   is per-channel.
 * - Per-channel statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] ChannelId
 *      Channel identifier for which diagnostics information is desired.
 *
 * @param [in] fAllChannels
 *      Dump the information of all channels instead of the one selected with
 *      ChannelId.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with CfyE_Device_Update for
 * the same device.
 * This function can be called concurrently with any CfyE API function
 * for a different device.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Diag_Channel_Dump(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const bool fAllChannels);


/**---------------------------------------------------------------------------
 * @fn CfyE_Diag_vPort_Dump(
 *       const unsigned int DeviceId,
 *       const CfyE_vPortHandle_t vPortHandle,
 *       const bool fAllvPorts,
 *       const bool fIncludeRule);
 *
 * Provide a diagnostics dump of all per-vPort configuration and status
 * information. This includes:
 * - vPort policy.
 * - List of rules associated with this vPort.
 * - Optionally all information releated to those rules.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] vPortHandle
 *      vPort handle for which diagnostics information is desired.
 *
 * @param [in] fAllvPorts
 *      Dump the information of all vPorts instead of the one selected with
 *      vPortHandle.
 *
 * @param [in] fIncludeRule
 *      Dump the information (CfyE_Diag_Rule_Dump) for all Rules associated
 *      with this particular vPort. vPortHandle may be a null handle
 *      if this parameter is true.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with CfyE_vPort_Add(),
 * CfyE_vPort_Remove(), CfyE_Rule_Add() or CfyE_Rule_Remove() for the
 * same device.
 * This function can be called concurrently with any CfyE API function
 * for a different device.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Diag_vPort_Dump(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        const bool fAllvPorts,
        const bool fIncludeRule);


/**---------------------------------------------------------------------------
 * @fn CfyE_Diag_Rule_Dump(
 *       const unsigned int DeviceId,
 *       const CfyE_RuleHandle_t RuleHandle,
 *       const bool fAllRules);
 *
 * Provide a diagnostics dump of all per-Rule configuration and status
 * information. This includes:
 * - The content of the rule (TCAM contents and vPort policy).
 * - flag to indicate whether rule is enabled.
 * - TCAM statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] RuleHandle
 *      Rule handle for which diagnostics information is desired.
 *
 * @param [in] fAllRules
 *      Dump the information of all Rules instead of the one selected with
 *      RuleHandle. RuleHandle may be a null handle if this parameter is true.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with CfyE_Rule_Add()
 * or CfyE_Rule_Remove() for the same device.
 * This function can be called concurrently with any CfyE API function
 * for a different device.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Diag_Rule_Dump(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle,
        const bool fAllRules);


/**---------------------------------------------------------------------------
 * @fn CfyE_Diag_MTT_Dump(
 *       const unsigned int DeviceId,
 *       const unsigned int MTTIndex,
 *       const bool fAllMTT);
 *
 * Provide a diagnostics dump of all per-MTT configuration and status
 * information. This includes:
 * - The content of the rule (TCAM entry)
 * - flag to indicate whether rule is enabled.
 * - MTT statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] MTTIndex
 *      Index of the MTT entry to be read.
 *
 * @param [in] fAllMTT
 *      Dump the information of all MTT enrries instead of the one selected with
 *      MTTIndex..
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function can be called concurrently with any CfyE API function
 * for a different device.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Diag_MTT_Dump(
        const unsigned int DeviceId,
        const unsigned int  MTTIndex,
        const bool fAllMTT);


#endif /* API_CFYE_EXT_H_ */


/* end of file api_cfye_ext.h */
