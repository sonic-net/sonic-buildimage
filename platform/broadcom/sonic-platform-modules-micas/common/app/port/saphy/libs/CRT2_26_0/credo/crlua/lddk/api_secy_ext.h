/** @file api_secy_ext.h
 *
 * @brief MACsec Extended SecY API
 *
 * This API provides functions to read the parameters of the installed
 * SAs.
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

#ifndef API_SECY_EXT_H_
#define API_SECY_EXT_H_

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

/* Driver Framework Basic Definitions API */
#include "basic_defs.h"             /* uint32_t */

/* SecY API types */
#include "api_secy_types.h"


/*----------------------------------------------------------------------------
 * Definitions and macros
 */


/**---------------------------------------------------------------------------
 * @fn SecY_Device_Role_Get(
 *        const unsigned int DeviceId,
 *        SecY_Role_t * const Role_p);
 *
 * Read the role of the device (ingress or egress).
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] Role_p
 *      The device role.
 *
 * This function is re-entrant for the same or different Devices.
 * This function can be called concurrently with any other function for
 * the same or different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Role_Get(
        const unsigned int DeviceId,
        SecY_Role_t * const Role_p);


/**---------------------------------------------------------------------------
 * @fn SecY_vPort_Params_Read(
 *       const unsigned int DeviceId,
 *       const unsigned int vPortId,
 *       SecY_SA_t * const SA_p);
 *
 * Read the SA flow parameters for a given vPort. Unlike  SecY_SA_Params_Read,
 * this function does not depend on any valid installed SAs for this vPort
 * and it will not return the SCI or AN for an ingress SA.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPortId
 *      vPort ID of the parameters to be read.
 *
 * @param [out] SA_p
 *      Pointer to a memory location where the parameters of the SA are
 *      stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use
 * order is followed.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_vPort_Params_Read(
        const unsigned int DeviceId,
        const unsigned int vPortId,
        SecY_SA_t * const SA_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_Params_Read(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t SAHandle,
 *       SecY_SA_t * const SA_p,
 *       uint8_t * const SCI_p);
 *
 * Read the SA flow parameters for an already added SA for a SecY device
 * instance identified by DeviceId parameter. This function does not read
 * the contents of the SA record, use the SecY_SA_Read() function for this.
 *
 * API use order:
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be read.
 *
 * @param [out] SA_p
 *      Pointer to a memory location where the parameters of the SA are
 *      stored.
 *
 * @param [out] SCI_p
 *      Pointer to a memory location where the SCI of an inbound SA will be
 *      stored. This is only used for inbound MACsec SAs.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use
 * order is followed. In particular this function can only be called
 * for a valid SA handle, returned by SecY_SA_Add() or
 * SecY_SA_Chain(), which is not yet removed with SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Params_Read(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        SecY_SA_t * const SA_p,
        uint8_t * const SCI_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_vPortIndex_Get(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t SAHandle,
 *       unsigned int * const vPort_p);
 *
 * Read the vPort index of a given SA handle.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA.
 *
 * @param [out] vPort_p
 *      The vPort associated with the SA.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use
 * order is followed. In particular this function can only be called
 * for a valid SA handle, returned by SecY_SA_Add() or
 * SecY_SA_Chain(), which is not yet removed with SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 */
SecY_Status_t
SecY_SA_vPortIndex_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        unsigned int * const vPort_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_Next_Get(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t CurrentSAHandle,
 *       SecY_SAHandle_t * const NextSAHandle_p);
 *
 * Return the next SA Handle that was allocated after a given SA
 * Handle for a given device.  Use this function to produce a list of
 * all allocated SAs.  Return SecY_SAHandle_NULL if the given SA
 * handle is the last one allocated.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] CurrentSAHandle
 *      SA handle returned by the last call to SecY_SAHandle_Next_Get. Use
 *      SecY_SAHAndle_NULL to for the first call.
 *
 * @param [out] NextSAHandle_p
 *      SA handle of the next allocated  SA.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * Note: a sequence of calls to this function is often performed to
 *       obtain the list of all allocated SAs. During this sequence
 *       of calls, no calls to SecY_SA_Add(), SecY_SA_Chain() and
 *       SecY_SA_Remove() shall be made for the same device,
 *       otherwise the list is inconsistent or calls may fail.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 */
SecY_Status_t
SecY_SA_Next_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t CurrentSAHandle,
        SecY_SAHandle_t * const NextSAHandle_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_Chained_Get(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t CurrentSAHandle,
 *       SecY_SAHandle_t * const NextSAHandle_p);
 *
 * Return the SA handle of the SA that was chained to the given SA.
 * Return SecY_SAHandle_NULL if no SA handle is chained to the given
 * SA handle.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] CurrentSAHandle
 *      SA handle of the SA for which to find the chained SA.
 *
 * @param [out] NextSAHandle_p
 *      SA handle of the chained SA.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use
 * order is followed. In particular this function can only be called
 * for a valid SA handle, returned by SecY_SA_Add() or
 * SecY_SA_Chain(), which is not yet removed with SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Chained_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t CurrentSAHandle,
        SecY_SAHandle_t * const NextSAHandle_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_WindowSize_Get(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t SAHandle,
 *       uint32_t * const  WindowSize_p);
 *
 * Reads the Window Size field for an inbound SA.
 *
 * API use order:
 *      This function may be called for the SAHandle obtained via the
 *      SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the XtSecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be updated.
 *
 * @param [out] WindowSize_p)
 *      32-bit integer representing the anti-replay window size.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function cannot be called concurrently with the XtSecY_SA_Remove()
 * function for the same DeviceId.
 * This function can be called concurrently with any other XtSecY API
 * function for the same or different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_WindowSize_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        uint32_t * const  WindowSize_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_NextPN_Get(
 *        const unsigned int DeviceId,
 *       const SecY_SAHandle_t SAHandle,
 *       uint32_t * const NextPN_Lo_p,
 *       uint32_t * const NextPN_Hi_p,
 *       bool * const fExtPN_p);
 *
 * Reads the NextPN field for an inbound SA.
 *
 * API use order:
 *      This function may be called for the SAHandle obtained via the
 *      SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be updated.
 *
 * @param [out] NextPN_Lo_p
 *      Least significant 32 bits of new NextPN. For 32-bit sequence numbers
 *      this is the entire sequence number.
 *
 * @param [out] NextPN_Hi_p
 *      Most significant 32 bits of new NextPN. Only applicable for 64-bit
 *      sequence numbers, will not be output for 32-bit sequence numbers.
 *
 * @param [out] fExtPN_p
 *      Flag to indicate that sequence number is 64-bit.
 *
 * This function is re-entrant for the same or  different DeviceId's.
 *
 * This function cannot be called concurrently with the SecY_SA_Remove()
 * function for the same DeviceId.
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_NextPN_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        uint32_t * const NextPN_Lo_p,
        uint32_t * const NextPN_Hi_p,
        bool * const fExtPN_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SCI_Next_Get(
 *       const unsigned int DeviceId,
 *       const unsigned int vPort,
 *       const void ** TmpHandle_p,
 *       uint8_t * const NextSCI_p,
 *       uint32_t * const SCIndex_p);
 *
 * Obtain the next ingress SCI that is associated with a given vPort.
 * Use this function to produce a list of all
 * SCIs associated with a given vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort on which to find the SCI.
 *
 * @param [inout] TmpHandle_p
 *      Pointer that represents the last SCI returned by this function.
 *      For the first call, pass the address of a pointer variable containing
 *      NULL. For subsequent calls, pass the address of the same variable,
 *      The function updates the variable after each call. The variable will
 *      contain non-NULL if a valid SCI is returned, NULL if no more valid
 *      SCIs are available.
 *
 * @param [out] NextSCI_p
 *      Pointer to a buffer of 8 bytes where the SCI must be stored.
 *
 * @param [out] SCIndex_p
 *      Pointer to a location where the SC index will be stored. Will not
 *      be stored if parameter is NULL.
 *
 * Note: a sequence of calls to this function is often performed to
 *       obtain the list of all allocated SAs. During this sequence
 *       of calls, no calls to SecY_SA_Add(), SecY_SA_Chain() and
 *       SecY_SA_Remove() shall be made for the same device,
 *       otherwise the list is inconsistent or calls may fail.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 */
SecY_Status_t
SecY_SCI_Next_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        const void ** TmpHandle_p,
        uint8_t * const NextSCI_p,
        uint32_t * const SCIndex_p);


/**---------------------------------------------------------------------------
 * @fn SecY_Diag_Device_Dump(
 *       const unsigned int DeviceId);
 *
 * Provide a diagnostics dump of all global device configuration and status
 * information. This includes:
 * - Device limits (number of vPorts, SCs, SAs, channels).
 * - Any information that can be obtained with SecY_Device_Config_Get() except
 *   per-channel information.
 * - Device ECC status.
 * - Global packets in-flight status.
 * - Various header parser state registers.
 * - Statistics summary status registers.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with SecY_Device_Update()
 * for the same device.
 * This function can be called concurrently with any SecY API function
 * for a different device.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Diag_Device_Dump(
        const unsigned int DeviceId);


/**---------------------------------------------------------------------------
 * @fn SecY_Diag_Channel_Dump(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       const bool fAllChannels);
 *
 * Provide a diagnostics dump of all per-channel configuration and status
 * information. This includes:
 * - Packet in-flight status of this channel.
 * - All information that can be obtained with SecY_Channel_Config_Get().
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] ChannelId
 *      Channel identifier for which diagnostics information is desired.
 *
 * @param [in] fAllChannels
 *      Dump the information of all channels instead of the one selected with
 *      ChannelId.
 *
 * This function is re-entrant for the same or different DeviceId.
 *
 * This function cannot be called concurrently with SecY_Device_Update()
 * for the same device.
 * This function can be called concurrently with any SecY API function
 * for a different device.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Diag_Channel_Dump(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const bool fAllChannels);


/**---------------------------------------------------------------------------
 * @fn SecY_Diag_vPort_Dump(
 *        const unsigned int DeviceId,
 *        const unsigned int vPortId,
 *        const bool fAllvPorts,
 *        const bool fIncludeSA);
 *
 * Provide a diagnostics dump of all per-vPort configuration and status
 * information. This includes:
 * - Per-vPort and per-interface statistics.
 * - Parameters related to the SA which are not in the SA record
 *   (stored in SAMFlowCtrl registers),
 * - For ingress configuration: all RxCAM entries associated with the vPort
 *   and up to four SA indexes associated with each SCI. RxCAM statistics
 *   for the entries involved.
 * - For egress configuration: the SA the index of the egress SA plus the index
 *   of  any chained SA records.
 * - Optionally all SA-related information for each of the SAs associated
 *   with the vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPortId
 *      vPort identifier for which diagnostics information is desired.
 *
 * @param [in] fAllvPorts
 *      Dump the information of all vPorts instead of the one selected with
 *      vPortId.
 *
 * @param [in] fIncludeSA
 *      Dump the information (SecY_Diag_SA_Dump) for all SAs associated
 *      with this particular vPort.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with
 * SecY_SA_Add, SecY_SA_Remove or SecY_SA_Chain the same device.
 * This function can be called concurrently with any SecY API function
 * for a different device.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Diag_vPort_Dump(
        const unsigned int DeviceId,
        const unsigned int vPortId,
        const bool fAllvPorts,
        const bool fIncludeSA);


/**---------------------------------------------------------------------------
 * @fn SecY_Diag_SA_Dump(
 *       const unsigned int DeviceId,
 *       const SecY_SAHandle_t SAHandle,
 *       const bool fAllSAs);
 *
 * Provide a diagnostics dump of all per-SA configuration and status
 * information. This includes:
 * - The contents of the SA record.
 * - Per-SA statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in]  SAHandle
 *      SA handle for which diagnostics information is desired.
 *
 * @param [in] fAllSAs
 *      Dump the information of all SAs instead of the one selected with
 *      SAHandle. SAHandle may be a null handle if this parameter is true.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use
 * order is followed. In particular this function can only be called
 * for a valid SA handle, returned by SecY_SA_Add() or
 * SecY_SA_Chain(), which is not yet removed with SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Diag_SA_Dump(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        const bool fAllSAs);


#endif /* API_SECY_EXT_H_ */


/* end of file api_secy_ext.h */
