/** @file api_secy.h
 *
 * @brief MACsec SecY API
 *
 * This API covers the SecY functionality specified in the IEEE 802.1AE MACsec
 * standard and supported by the MACsec hardware acceleration device.
 *
 * This API does not include packet classification functionality which is
 * covered by the CfyE API. Both the SecY API and CfyE API can be used
 * together.
 *
 * This API implementation can be instantiated for one or several MACsec
 * hardware acceleration devices (further SecY device).
 *
 * Refer to the API Implementation Notes for implementation-specific details,
 * see Security-IP_SecY-API_Implementation-Notes.txt file.
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

#ifndef API_SECY_H_
#define API_SECY_H_

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

/*----------------------------------------------------------------------------
 * Device global events to use with the notifications
 */
/** Packet drop event from classification logic */
#define SECY_EVENT_DROP_CLASS           BIT_0
/** Packet drop event from post-processor logic */
#define SECY_EVENT_DROP_PP              BIT_1
/** Packet drop event from MTU checking logic (egress only) */
#define SECY_EVENT_DROP_MTU             BIT_2
/** Interrupt event from EIP-66 MACsec crypto-engine (PE) core.*/
#define SECY_EVENT_ENG_IRQ              BIT_3

/** Statistics threshold event from SA Statistics Module */
#define SECY_EVENT_STAT_SA_THR          BIT_4
/** Statistics threshold event from SecY Statistics Module */
#define SECY_EVENT_STAT_SECY_THR        BIT_5
/** Statistics threshold event from IFC Statistics Module */
#define SECY_EVENT_STAT_IFC_THR         BIT_6
/** Statistics threshold event from IFC1 Statistics Module */
#define SECY_EVENT_STAT_IFC1_THR        BIT_7
/** Statistics threshold event from IFC1 Statistics Module */
#define SECY_EVENT_STAT_RXCAM_THR       BIT_8
/** Packet number threshold event from Post Processor */
#define SECY_EVENT_SA_PN_THR            BIT_10
/** SA expired event from Classifier */
#define SECY_EVENT_SA_EXPIRED           BIT_11
/** ECC error threshold event. */
#define SECY_EVENT_ECC_ERR              BIT_12

/** Packet Engine interrupt sources */
/** Length error interrupt */
#define SECY_EVENT_PE_LENGTH_ERR0         BIT_16
/** Length error interrupt */
#define SECY_EVENT_PE_LENGTH_ERR1         BIT_17
/** Length error interrupt */
#define SECY_EVENT_PE_LENGTH_ERR2         BIT_18
/** AES counter error interrupt */
#define SECY_EVENT_PE_CTR_ERR             BIT_19
/** Token error interrupt */
#define SECY_EVENT_PE_TOKEN_ERR           BIT_20
/** Context error interrupt */
#define SECY_EVENT_PE_CTX_ERR             BIT_21
/** Context ECC error interrupt */
#define SECY_EVENT_PE_ECC_ERR             BIT_22
/** Inbound zero packet number */
#define SECY_EVENT_PE_IB_ZERO_PKTNUM      BIT_23
/** Outbound sequence number threshold interrupt */
#define SECY_EVENT_PE_OB_SEQNR_THR        BIT_24
/** Inbound authentication failure */
#define SECY_EVENT_PE_IB_AUTH_FAIL        BIT_25
/** Outbound sequence number roll-over interrupt */
#define SECY_EVENT_PE_OB_SEQNR_RO         BIT_26
/** Inbound short length interrupt */
#define SECY_EVENT_PE_IB_SL               BIT_27
/** Outbound MTU check interrupt */
#define SECY_EVENT_PE_OB_MTU_CHECK        BIT_28
/** Outbound packet expansion error */
#define SECY_EVENT_PE_OB_EXPANSION_ERR    BIT_29


/*----------------------------------------------------------------------------
 * Device channel-specific events to use with the notifications
 */
/* Input interface events: */
/** Start-Of-Packet (SOP) is received for a busy channel event */
#define SECY_EVENT_CHAN_ERR_SOP_WO_EOP          BIT_0
/** End-Of-Packet (EOP) is received for an idle channel event */
#define SECY_EVENT_CHAN_ERR_EOP_WO_SOP          BIT_1
/** Data is received for an idle channel event */
#define SECY_EVENT_CHAN_ERR_XFER_WO_SOP         BIT_2
/** SOP is received in the 2nd cycle of the slot event */
#define SECY_EVENT_CHAN_ERR_SLOT_SOP            BIT_3
/** Channel ID is changed in the 2nd cycle of the slot event */
#define SECY_EVENT_CHAN_ERR_SLOT_CHID           BIT_4
/** First 2 packet words are not received back-to-back in the first slot event */
#define SECY_EVENT_CHAN_ERR_NOT_B2B             BIT_5

/* Other channel-specific events: */
/** Multiple RxCAM rule hit for a packet event (ingress only) */
#define SECY_EVENT_CHAN_RXCAM_HIT_MULT          BIT_6
/** RxCAM rule not hit for a packet event (ingress only) */
#define SECY_EVENT_CHAN_RXCAM_MISS              BIT_7
/** Packet is dropped due to overrun in data pipeline event */
#define SECY_EVENT_CHAN_PKT_DATA_OVERRUN        BIT_8
/** Packet is dropped due to overrun in classification pipeline event */
#define SECY_EVENT_CHAN_PKT_CFY_OVERRUN         BIT_9

/** SecY device Operation mode */
typedef enum
{
    SECY_MODE_MACSEC,  /**< \n */
    SECY_MODE_IPSEC,   /**< \n */
    SECY_MODE_INVALID  /**< \n */
} SecY_Channel_Mode_t;

/**----------------------------------------------------------------------------
 * @typedef SecY_NotifyFunction_t
 *
 * This type specifies the callback function prototype for the function
 * SecY_Notify_Request(). The notification will occur only once.
 *
 * @note The exact context in which the callback function is invoked and the
 *       allowed actions in that callback are implementation specific. The
 *       intention is that all API functions can be used, except
 *       SecY_Device_Init(), SecY_Device_Uninit(), SecY API functions with
 *       fSync = true.
 *
 * @param [in] Events
 *     Mask containing the SECY_EVENT_* flags indicating which Classification
 *     Device events cause the callback.
 *
 * @return value
 *     None
 */
typedef void (* SecY_NotifyFunction_t)(unsigned int Events);

/** Asynchronous SecY device notification */
typedef struct
{
    /** Pointer to the callback function */
    SecY_NotifyFunction_t CBFunc_p;

    /** The requested events,
     * bitwise OR of SECY_EVENT_* global events or
     * SECY_EVENT_CHAN_* channel-specific events
     */
    unsigned int EventMask;

    /** True if notification is for the device Global AIC,
     *  otherwise the notification is for the device Channel AIC
     */
    bool fGlobal;

    /** If fGlobal is true then this parameter indicates the Channel Id,
     *  otherwise not used
     */
    unsigned int ChannelId;

} SecY_Notify_t;


/**----------------------------------------------------------------------------
 * @fn SecY_Notify_Request(
 *     const unsigned int DeviceId,
 *     const SecY_Notify_t * const Notify_p);
 *
 * This routine can be used to request a one-time notification of available
 * Statistics or Classification device events. It is typically used when the
 * caller does not want to poll the device for classification events.
 *
 * Once the requested events become signaled, the implementation will invoke
 * the callback one-time to notify this fact.
 * The notification is then immediately disabled. It is possible that the
 * notification callback is invoked when not all the requested events (mask)
 * have been signaled (or even zero). In this case the application must
 * invoke SecY_Notify_Request() again to be notified about the remaining
 * expected events.
 *
 * Once the notification callback is invoked, the application must handle
 * the signaled events and then call SecY_Notify_Request() again, else the
 * notification callback may not be called again.
 *
 * @param [in] DeviceId
 *      Device identifier of the Classification device.
 *
 * @param [in] Notify_p
 *     Pointer to the notification data structure.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Notify_Request(
        const unsigned int DeviceId,
        const SecY_Notify_t * const Notify_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Init(
 *     const unsigned int DeviceId,
 *     const SecY_Role_t Role,
 *     const SecY_Settings_t * const Settings_p)
 *
 * Initializes a SecY device instance identified by IntefaceId parameter.
 *
 * @pre  API use order:\n
 *       This function must be executed before any other of the SecY_*()
 *       or CfyE_*() functions may be called for this DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] Role
 *      Device role: Egress, Ingress or combined.
 *
 * @param [in] Settings_p
 *      Pointer to a memory location where the device settings are stored.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * @note When this function returns an error, all driver resources
 *       are freed and no device entry will be created.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Init(
        const unsigned int DeviceId,
        const SecY_Role_t Role,
        const SecY_Settings_t * const Settings_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Uninit(
 *     const unsigned int DeviceId)
 *
 * Uninitializes a SecY device instance identified by DeviceId parameter.
 *
 * @pre  API use order:\n
 *       This function must be called when the SecY device for this DeviceId
 *       is no longer needed. After this function is called no other CfyE_*()
 *       or SecY_*() functions may be called for this DeviceId except the
 *       SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with any other CfyE or SecY
 * API function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Uninit(
        const unsigned int DeviceId);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Limits(
 *     const unsigned int DeviceId,
 *     unsigned int * const MaxChannelCount_p,
 *     unsigned int * const MaxvPortCount_p,
 *     unsigned int * const MaxSACount_p,
 *     unsigned int * const MaxSCCount_p)
 *
 * Returns the maximum number of channels, vPorts, Security Associations (SAs)
 * and/or Secure CHannels (SCs) of the SecY device instance identified by
 * DeviceId parameter.
 *
 * @pre  API use order:\n
 *       This function can be called after the function SecY_Device_Init() and
 *       before the function SecY_Device_Uninit() is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] MaxChannelCount_p
 *      Pointer to a memory location where the maximum number of Channels must
 *      be stored. Skipped if the pointer is NULL.
 *
 * @param [out] MaxvPortCount_p
 *      Pointer to a memory location where the maximum number of vPorts must
 *      be stored. Skipped if the pointer is NULL.
 *
 * @param [out] MaxSACount_p
 *      Pointer to a memory location where the maximum number of SAs must be
 *      stored. Skipped if the pointer is NULL.
 *
 * @param [out] MaxSCCount_p
 *      Pointer to a memory location where the maximum number of SCs must be
 *      stored. Skipped if the pointer is NULL.
 *
 * This function is re-entrant and called always as long as the API use order
 * is respected.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 */
SecY_Status_t
SecY_Device_Limits(
        const unsigned int DeviceId,
        unsigned int * const MaxChannelCount_p,
        unsigned int * const MaxvPortCount_p,
        unsigned int * const MaxSACount_p,
        unsigned int * const MaxSCCount_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Update(
 *       const unsigned int DeviceId,
 *       const SecY_Device_Params_t * const Control_p)
 *
 * This function updates the SecY device control settings
 * for the specified channel.
 *
 * The SECY_EVENT_STAT_SA_THR, SECY_EVENT_STAT_SECY_THR,
 * SECY_EVENT_STAT_IFC_THR, SECY_EVENT_STAT_IFC1_THR, SECY_EVENT_STAT_RXCAM_THR
 * and SECY_EVENT_STAT_CHAN_THR events can be configured via the
 * SecY_Device_Params_t:StatControl_p data structure by specifying the packet
 * counter threshold to be reached in order for all events to be generated by
 * the SecY Engine.\n
 * The SecY_Notify_Request() can be used to request the asynchronous
 * notifications for these events to be signaled via the SecY_NotifyFunction_t
 * callback.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] Control_p
 *      Pointer to the memory location where the control parameters
 *      are stored, see SecY_Device_Params_t.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for different DeviceId's.
 *
 * This function cannot be called concurrently with SecY_Device_Config_Get()
 * or SecY_Device_Status_Get() for the same DeviceId.
 * It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Update(
        const unsigned int DeviceId,
        const SecY_Device_Params_t * const Control_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Config_Get(
 *        const unsigned int DeviceId,
 *        SecY_Device_Params_t * const Control_p);
 *
 * This function reads the SecY device control settings that can be controlled.
 * by SecY_Device_Update.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] Control_p
 *      Pointer to the memory location where the control parameters
 *      are stored, see SecY_Device_Params_t. The caller must initialize
 *      pointers within this structure to point to memory locations where the
 *      appropriate data structures can be stored.
 *      Alternatively the application can set some pointers within
 *      this structure to NULL and then this function will not read the
 *      corresponding configuration information.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function cannot be called concurrently with SecY_Device_Update()
 * or the same DeviceId.
 * It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Config_Get(
        const unsigned int DeviceId,
        SecY_Device_Params_t * const Control_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_Status_Get(
 *       const unsigned int DeviceId,
 *       SecY_DeviceStatus_t * const DeviceStatus_p);
 *
 * This function reads the ECC error status counters end resets the error
 * counters to zero.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] DeviceStatus_p
 *      Data structure in which to return the device status.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function cannot be called concurrently with SecY_Device_Update()
 * or the same DeviceId.
 * It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_Status_Get(
        const unsigned int DeviceId,
        SecY_DeviceStatus_t * const DeviceStatus_p);


/**---------------------------------------------------------------------------
 * @fn SecY_Device_InsertSOP(
 *        const unsigned int DeviceId,
 *        const SecY_Ch_Mask_t * const ChannelMask_p);
 *
 * This function inserts an SOP signal into the processing pipeline of the SecY
 * device. This signal can be used with SecY_Device_InsertEOP() to
 * clear the processing pipe in case of a fault condition.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] ChannelMask_p
 *      Bit mask to specify on which channels the SOP signal must be inserted.
 *      A '1' bit in a bit position specifies the insertion of the signal on
 *      the corresponding channel.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_InsertSOP(
        const unsigned int DeviceId,
        const SecY_Ch_Mask_t * const ChannelMask_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_InsertEOP(
 *        const unsigned int DeviceId,
 *        const SecY_Ch_Mask_t * const ChannelMask_p);
 *
 * This function inserts an EOP signal into the processing pipeline of the SecY
 * device. This signal can be used with SecY_Device_InsertSOP() to
 * clear the processing pipe in case of a fault condition.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] ChannelMask_p
 *      Bit mask to specify on which channels the EOP signal must be inserted.
 *      A '1' bit in a bit position specifies the insertion of the signal on
 *      the corresponding channel.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_InsertEOP(
        const unsigned int DeviceId,
        const SecY_Ch_Mask_t * const ChannelMask_p);



/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_PSecY_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const SecYIndexes_pp,
 *     unsigned int * const NumSecYIndexes_p)
 *
 * Reads the values for the Per-SecY count summary registers and clear the
 * bits that are set by hardware. Each bit represents a SecY that has a
 * statistics counter that crossed the threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] SecYIndexes_pp
 *      Pointer to a memory location where a set of SecY indexes that have
 *      their statistics crossed the threshold will be stored.
 *
 * @param [out] NumSecYIndexes_p
 *      Pointer to a memory location where to save number of SecY indexes
 *      whose statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_PSecY_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const SecYIndexes_pp,
        unsigned int * const NumSecYIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_SecY_CheckAndClear(
 *     const unsigned int DeviceId,
 *     const unsigned int SecYIndex,
 *     uint32_t * const CountSummarySecY_p)
 *
 * Reads the value of a SecY counter summary register and clear the bits that
 * are set by hardware. Each bit represents a SecY statistics counter. If the
 * bit is set it indicates that the corresponding counter has crossed the set
 * threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SecYIndex
 *      SecY index of the SecY to be checked.
 *
 * @param [out] CountSummarySecY_p
 *      Pointer to a memory location in which a set of bits is returned,
 *      each bit represents a SecY statistics counter.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_SecY_CheckAndClear(
        const unsigned int DeviceId,
        const unsigned int SecYIndex,
        uint32_t * const CountSummarySecY_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_PIfc_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const IfcIndexes_pp,
 *     unsigned int * const NumIfcIndexes_p)
 *
 * Reads the values for Per-IFC count summary registers and clear the bits
 * that are set by hardware. Each bit represents an IFC (interface) that has
 * a statistics counter that crossed the threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for IFC.
 *
 * @param [out] IfcIndexes_pp
 *      Pointer to a memory location where a set of IFC indexes that have
 *      their statistics crossed the threshold will be stored.
 *
 * @param [out] NumIfcIndexes_p
 *      Pointer to a memory location where to save number of IFC indexes
 *      whose statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_PIfc_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const IfcIndexes_pp,
        unsigned int * const NumIfcIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_Ifc_CheckAndClear(
 *     const unsigned int DeviceId,
 *     const unsigned int IfcIndex,
 *     uint32_t * const CountSummaryIfc_p)
 *
 * Reads the value of a IFC counter summary register and clear the bits that
 * are set by hardware. Each bit represents an IFC statistics counter. If the
 * bit is set it indicates that the corresponding counter has crossed the set
 * threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for IFC.
 *
 * @param [in] IfcIndex
 *      IFC index of the IFC to be checked.
 *
 * @param [out] CountSummaryIfc_p
 *      Pointer to a memory location in which a set of bits is returned,
 *      each bit represents one IFC statistics counter.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_Ifc_CheckAndClear(
        const unsigned int DeviceId,
        const unsigned int IfcIndex,
        uint32_t * const CountSummaryIfc_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_PIfc1_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const IfcIndexes_pp,
 *     unsigned int * const NumIfcIndexes_p)
 *
 * Reads the values for Per-IFC1 count summary registers and clear the bits
 * that are set by hardware. Each bit represents an IFC1 (interface1) that has
 * a statistics counter that crossed the threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for IFC1.
 *
 * @param [out] IfcIndexes_pp
 *      Pointer to a memory location where a set of IFC1 indexes that have
 *      their statistics crossed the threshold will be stored.
 *
 * @param [out] NumIfcIndexes_p
 *      Pointer to a memory location where to save number of IFC1 indexes
 *      whose statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_PIfc1_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const IfcIndexes_pp,
        unsigned int * const NumIfcIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_Ifc1_CheckAndClear(
 *     const unsigned int DeviceId,
 *     const unsigned int IfcIndex,
 *     uint32_t * const CountSummaryIfc_p)
 *
 * Reads the value of a IFC1 counter summary register and clear the bits that
 * are set by hardware. Each bit represents an IFC1 statistics counter. If the
 * bit is set it indicates that the corresponding counter has crossed the set
 * threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for IFC1.
 *
 * @param [in] IfcIndex
 *      IFC1 index of the IFC1 to be checked.
 *
 * @param [out] CountSummaryIfc_p
 *      Pointer to a memory location in which a set of bits is returned,
 *      each bit represents one IFC1 statistics counter.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_Ifc1_CheckAndClear(
        const unsigned int DeviceId,
        const unsigned int IfcIndex,
        uint32_t * const CountSummaryIfc_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_PRxCAM_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const RxCAMIndexes_pp,
 *     unsigned int * const NumRxCAMIndexes_p)
 *
 * Reads the values for Per-RxCAM count summary registers and clear the bits
 * that are set by hardware. Each bit represents an RxCAM hit counter that has
 * crossed the threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for RxCAM.
 *
 * @param [out] RxCAMIndexes_pp
 *      Pointer to a memory location where a set of RxCAM hit counter indexes
 *      that have their statistics crossed the threshold will be stored.
 *
 * @param [out] NumRxCAMIndexes_p
 *      Pointer to a memory location where to save number of RxCAM hit counter
 *      indexes whose statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_PRxCAM_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const RxCAMIndexes_pp,
        unsigned int * const NumRxCAMIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_PSA_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const SAIndexes_pp,
 *     unsigned int * const NumSAIndexes_p)
 *
 * Reads the values for Per-SA expired summary registers and clear the bits
 * that are set by hardware. Each bit represents a SA that has a statistics
 * counter that crossed the threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] SAIndexes_pp
 *      Pointer to a memory location where a set of SA indexes that have
 *      their statistics crossed the threshold will be stored.
 *
 * @param [out] NumSAIndexes_p
 *      Pointer to a memory location where to save number of SA indexes whose
 *      statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * This function cannot be called concurrently with SecY_SA_Remove()
 * for the same DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_PSA_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const SAIndexes_pp,
        unsigned int * const NumSAIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Device_CountSummary_SA_CheckAndClear(
 *     const unsigned int DeviceId,
 *     const unsigned int SAIndex,
 *     uint32_t * const CountSummarySA_p)
 *
 * Reads the value of a SA statistics summary register and clear the bits that
 * are set by hardware. Each bit represents a SA statistics counter. If the
 * bit is set it indicates that the corresponding counter has crossed the set
 * threshold.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAIndex
 *      SA index for the SA to be checked.
 *
 * @param [out] CountSummarySA_p
 *      Pointer to a memory location where saves a set of bits, each of them
 *      represents one SA statistics counters.
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.\n
 *
 * This function cannot be called concurrently with SecY_SA_Remove()
 * for the same DeviceId.
 *
 * @note This function will remove the driver resources of the SA in all
 *       cases where it is possible, even if the function returns an error.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Device_CountSummary_SA_CheckAndClear(
        const unsigned int DeviceId,
        const unsigned int SAIndex,
        uint32_t * const CountSummarySA_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SAHandle_IsSame(
 *     const SecY_SAHandle_t * const Handle1_p,
 *     const SecY_SAHandle_t * const Handle2_p)
 *
 * Check whether the provided Handle1 is equal to provided Handle2.
 *
 * @param Handle1_p
 *      First handle
 *
 * @param Handle2_p
 *      Second handle
 *
 * This function is re-entrant for the same or different DeviceId's.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.
 *
 * @return true  : provided handles are equal
 * @return false : provided handles are not equal
 */
bool
SecY_SAHandle_IsSame(
        const SecY_SAHandle_t * const Handle1_p,
        const SecY_SAHandle_t * const Handle2_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SAHandle_SAIndex_IsSame(
 *     const SecY_SAHandle_t SAHandle,
 *     const unsigned int SAIndex)
 *
 * Check whether the provided Handle points to the same SA as the provided
 * SA index.
 *
 * @param SAHandle
 *      SA handle
 *
 * @param SAIndex
 *      SA index
 *
 * This function is re-entrant for the same or different DeviceId's.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.
 *
 * @return true  : the provided handle points the same SA as the provide index
 * @return false : the provided handle points the different SA as the provide index
 */
bool
SecY_SAHandle_SAIndex_IsSame(
        const SecY_SAHandle_t SAHandle,
        const unsigned int SAIndex);


/**----------------------------------------------------------------------------
 * @fn SecY_CryptAuth_BypassLen_Update(
 *     const unsigned int DeviceId,
 *     const unsigned int BypassLength)
 *
 * Updates the length of the bypass data during crypt-authenticate operation
 * for a SecY device identified by DeviceId. The bypass data should contain
 * MAC header to be able to classify the packet into the corresponding
 * crypto-authenticate flow control register and additionally it might include
 * the IV used in the transformation.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] BypassLength
 *      The length in bytes of the bypass data.
 *
 * This function is re-entrant for the same or different DeviceId's.\n
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_CryptAuth_BypassLen_Update(
        const unsigned int DeviceId,
        const unsigned int BypassLength);


/**---------------------------------------------------------------------------
 * @fn SecY_CryptAuth_BypassLen_Get(
 *        const unsigned int DeviceId,
 *        unsigned int * const BypassLength_p);
 *
 * Read the length of the bypass data during crypt-authenticate operation
 * for a SecY device identified by DeviceId. The bypass data should contain
 * MAC header to be able to classify the packet into the corresponding
 * crypto-authenticate flow control register and additionally it might include
 * the IV used in the transformation.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] BypassLength_p
 *      The length in bytes of the bypass data.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_CryptAuth_BypassLen_Get(
        const unsigned int DeviceId,
        unsigned int * const BypassLength_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Add(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     SecY_SAHandle_t * const SAHandle_p,
 *     const SecY_SA_t * const SA_p)
 *
 * Adds a new SA for a SecY device instance identified by DeviceId parameter.
 *
 * @pre  API use order:\n
 *       The flow associated with this SA if any can be enabled via the
 *       CfyE_Flow_Enable() function after this function is called for the
 *       same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used for the SecY.
 *
 * @param [in] vPort
 *      vPort number to which the SA applies.
 *
 * @param [out] SAHandle_p
 *      Place holder where the SA handle will be stored that can be used
 *      in the SecY_SA_*() function calls for this DeviceId.
 *
 * @param [in] SA_p
 *      Pointer to a memory location where the data for the new SA is stored.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Remove()
 * function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @note When this function returns an error other than
 *    SECY_ERROR_BAD_PARAMETER and the returned SA handle is not a null
 *    handle, an SA entry has been allocated, which must then be removed
 *    by SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Add(
        const unsigned int DeviceId,
        const unsigned int vPort,
        SecY_SAHandle_t * const SAHandle_p,
        const SecY_SA_t * const SA_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Update(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     const SecY_SA_t * const SA_p)
 *
 * Updates SA flow parameters for an already added SA for a SecY device
 * instance identified by DeviceId parameter.
 *
 * @note This function does not update the SA transform record data and it
 *       cannot be used to update the key in the transform record.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be updated.
 *
 * @param [in] SA_p
 *      Pointer to a memory location where the new parameters for the SA are
 *      stored.
 *
 * This function is re-entrant for the same of different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Remove()
 * function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Update(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        const SecY_SA_t * const SA_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_NextPN_Update(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     const uint32_t NextPN_Lo,
 *     const uint32_t NextPN_Hi,
 *     bool * const fNextPNWritten_p)
 *
 * Updates the NextPN field for an inbound SA.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via the
 *      SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be updated.
 *
 * @param [in] NextPN_Lo
 *      Least significant 32 bits of new NextPN. For 32-bit sequence numbers
 *      this is the entire sequence number.
 *
 * @param [in] NextPN_Hi
 *      Most significant 32 bits of new NextPN. Only applicable for 64-bit
 *      sequence numbers, will be ignored for 32-bit sequence numbers.
 *
 * @param [out] fNextPNWritten_p
 *      flag to indicate that NextPN is actually written. Output not provided
 *      if pointer is NULL.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Remove()
 * function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_NextPN_Update(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        const uint32_t NextPN_Lo,
        const uint32_t NextPN_Hi,
        bool * const fNextPNWritten_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_WindowSize_Update(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     const uint32_t WindowSize);
 *
 * Updates the Window Size field for an inbound SA.
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
 * @param [in] WindowSize
 *      32-bit integer representing the anti-replay window size.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for different DeviceId's.
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
SecY_SA_WindowSize_Update(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        const uint32_t WindowSize);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Read(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     const unsigned int WordOffset,
 *     const unsigned int WordCount,
 *     uint32_t * Transform_p)
 *
 * Reads (a part of) a transform record of an already added SA from a SecY
 * device instance identified by DeviceId parameter.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via the
 *      SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be read.
 *
 * @param [in] WordOffset
 *      Word offset in transform record where the data should be read from.
 *
 * @param [in] WordCount
 *      Number of words which must be read from the SA transform record.
 *
 * @param [out] Transform_p
 *      Pointer to the memory location where the WordCount 32-bit words
 *      of the SA transform record will be stored.
 *
 * This function is re-entrant.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different SAHandle of the same or different
 * DeviceId provided the user order of the API is followed.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Read(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        const unsigned int WordOffset,
        const unsigned int WordCount,
        uint32_t * Transform_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Remove(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle)
 *
 * Removes a previously added SA from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @pre  API use order:\n
 *       The flow associated with this SA if any must be disabled via the
 *       CfyE_Flow_Disable() function and removed via the CfyE_Flow_Remove()
 *       function before this function can be called for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to be removed. After this function returns this
 *      handle should not be used anymore with the SecY_SA_*() functions.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Add()
 * function for the same DeviceId.\n
 * This function cannot be called concurrently with the SecY_SA_Read()
 * function for the same SAHandle.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Remove(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Chain(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t ActiveSAHandle,
 *     SecY_SAHandle_t * const NewSAHandle_p,
 *     const SecY_SA_t * const NewSA_p)
 *
 * Adds a new SA transform record and chain it to the current active SA.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] ActiveSAHandle
 *      SA handle of the current active SA.
 *
 * @param [out] NewSAHandle_p
 *      Place holder where the SA handle will be stored that can be used
 *      in the SecY_SA_*() function calls for this DeviceId.
 *
 * @param [in] NewSA_p
 *      Pointer to a memory location where the data for the new SA is stored.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Add() and
 * SecY_SA_Remove() function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @note When this function returns an error other than
 *    SECY_ERROR_BAD_PARAMETER and the new SA handle is not a null
 *    handle, an SA entry has been allocated, which must then be removed
 *    by SecY_SA_Remove().
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Chain(
        const unsigned int DeviceId,
        const SecY_SAHandle_t ActiveSAHandle,
        SecY_SAHandle_t * const NewSAHandle_p,
        const SecY_SA_t * const NewSA_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Switch(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t ActiveSAHandle,
 *     const SecY_SAHandle_t NewSAHandle,
 *     const SecY_SA_t * const NewSA_p)
 *
 * Manually switch to the new SA from the current active SA.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] ActiveSAHandle
 *      SA handle of the current active SA.
 *
 * @param [in] NewSAHandle
 *      SA handle of the SA to be switched to.
 *
 * @param [in] NewSA_p
 *      Pointer to a memory location where the data for the new SA is stored.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function cannot be called concurrently with the SecY_SA_Remove()
 * function for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Switch(
        const unsigned int DeviceId,
        const SecY_SAHandle_t ActiveSAHandle,
        const SecY_SAHandle_t NewSAHandle,
        const SecY_SA_t * const NewSA_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_Active_E_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     SecY_SAHandle_t * const ActiveSAHandle_p)
 *
 * Return the handle of the currently active egress SA for a given vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort number to which the SA applies.
 *
 * @param [out] ActiveSAHandle_p
 *      Currently active egress SA for the given vPort.
 *
 * This function is re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Active_E_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        SecY_SAHandle_t * const ActiveSAHandle_p);


/**---------------------------------------------------------------------------
 * @fn SecY_SA_Active_I_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     const uint8_t * const SCI_p,
 *     SecY_SAHandle_t * const ActiveSAHandle_p)
 *
 * Return the handles of the currently active ingress SA's for a given vPort
 * and SCI.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort number to which the SA applies.
 *
 * @param [in] SCI_p
 *      SCI for which the SA is desired.
 *
 * @param [out] ActiveSAHandle_p
 *      Array of four currently active egress SA for the given vPort, one
 *      for each AN from 0 to 3. Any SA that is not active is represented
 *      by SecY_SAHandle_NULL.
 *
 * This function is re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceId's.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId.\n
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Active_I_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        const uint8_t * const SCI_p,
        SecY_SAHandle_t * const ActiveSAHandle_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Statistics_E_Get(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     SecY_SA_Stat_E_t * const Stat_p,
 *     const bool fSync)
 *
 * Reads egress SA statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the egress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA for which the statistics must be read.
 *
 * @param [out] Stat_p
 *      Pointer to a memory location where the egress SA statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same SAHandle no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Statistics_E_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        SecY_SA_Stat_E_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Statistics_I_Get(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     SecY_SA_Stat_I_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads ingress SA statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA for which the statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the ingress SA statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same SAHandle no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Statistics_I_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        SecY_SA_Stat_I_t * const Stats_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Statistics_IPsec_I_Get(
 *     const unsigned int DeviceId,
 *     const SecY_SAHandle_t SAHandle,
 *     SecY_SA_Stat_IPsec_I_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads ingress SA statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA for which the statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the IPsec ingress SA statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same SAHandle no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Statistics_IPsec_I_Get(
        const unsigned int DeviceId,
        const SecY_SAHandle_t SAHandle,
        SecY_SA_Stat_IPsec_I_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_SecY_Statistics_E_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     SecY_SecY_Stat_E_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads egress SecY statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the egress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the SecY statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the egress SecY statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SecY_Statistics_E_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        SecY_SecY_Stat_E_t * const Stats_p,
        const bool fSync);

/**----------------------------------------------------------------------------
 * @fn SecY_SecY_Statistics_IPsec_E_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     SecY_SecY_Stat_IPsec_E_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads egress SecY statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the egress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the SecY statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the egress SecY statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SecY_Statistics_IPsec_E_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        SecY_SecY_Stat_IPsec_E_t * const Stats_p,
        const bool fSync);

/**----------------------------------------------------------------------------
 * @fn SecY_SecY_Statistics_IPsec_I_Get(
 *     const unsigned int DeviceId,
 *     unsigned int vPort,
 *     SecY_SecY_Stat_IPsec_I_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads ingress SecY statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the SecY statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the ingress SecY statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SecY_Statistics_IPsec_I_Get(
        const unsigned int DeviceId,
        unsigned int vPort,
        SecY_SecY_Stat_IPsec_I_t * const Stats_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_SecY_Statistics_I_Get(
 *     const unsigned int DeviceId,
 *     unsigned int vPort,
 *     SecY_SecY_Stat_I_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads ingress SecY statistics from a SecY device instance identified by
 * DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the SecY statistics must be read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the ingress SecY statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SecY_Statistics_I_Get(
        const unsigned int DeviceId,
        unsigned int vPort,
        SecY_SecY_Stat_I_t * const Stats_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_Ifc_Statistics_E_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPort,
 *     SecY_Ifc_Stat_E_t * const Stat_p,
 *     const bool fSync)
 *
 * Reads egress interface statistics from a SecY device instance identified
 * by DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the egress or combined egress/ingress role.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the interface statistics must be read.
 *
 * @param [out] Stat_p
 *      Pointer to a memory location where the egress interface statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Ifc_Statistics_E_Get(
        const unsigned int DeviceId,
        const unsigned int vPort,
        SecY_Ifc_Stat_E_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_Ifc_Statistics_I_Get(
 *     const unsigned int DeviceId,
 *     unsigned int vPort,
 *     SecY_Ifc_Stat_I_t * const Stat_p,
 *     const bool fSync)
 *
 * Reads ingress interface statistics from a SecY device instance identified
 * by DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the interface statistics must be read.
 *
 * @param [out] Stat_p
 *      Pointer to a memory location where the ingress interface statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same vPort no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Ifc_Statistics_I_Get(
        const unsigned int DeviceId,
        unsigned int vPort,
        SecY_Ifc_Stat_I_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn SecY_RxCAM_Statistics_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int SCIndex,
 *     SecY_RxCAM_Stat_t * const Stats_p,
 *     const bool fSync)
 *
 * Reads RxCAM statistics (ingress only) from a SecY device instance
 * identified by DeviceId parameter.
 *
 * @note This function can be called for the device that has been initialized
 *       for the ingress or combined egress/ingress role.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SCIndex
 *      Index of the Secure Channel for which the RxCAM statistics must be
 *      read.
 *
 * @param [out] Stats_p
 *      Pointer to a memory location where the ingress RxCAM statistics
 *      will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same SCIndex no matter what fSync
 * value is.\n
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.\n
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the SecY API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_RxCAM_Statistics_Get(
        const unsigned int DeviceId,
        const unsigned int SCIndex,
        SecY_RxCAM_Stat_t * const Stats_p,
        const bool fSync);


/**---------------------------------------------------------------------------
 * @fn SecY_vPort_Statistics_Clear(
 *       unsigned int DeviceId,
 *       unsigned int vPort);
 *
 * Clear the SecY and IFC statistics counters belonging to a given vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort number for which to clear the statistics.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_vPort_Statistics_Clear(
        unsigned int DeviceId,
        unsigned int vPort);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_Statistics_Clear(
 *     unsigned int DeviceId,
 *     SecY_SAHandle_t SAHandle)
 *
 * Clear the SA statistics counters belonging to a given SA.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] SAHandle
 *      SA handle of the SA for which the statistics must be cleared.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_Statistics_Clear(
        unsigned int DeviceId,
        SecY_SAHandle_t SAHandle);

/**----------------------------------------------------------------------------
 * @fn SecY_SecY_Statistics_Clear(
 *     unsigned int DeviceId,
 *     unsigned int vPort)
 *
 * Clear the SecY statistics counters belonging to a given vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the SecY statistics must be cleared.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SecY_Statistics_Clear(
        unsigned int DeviceId,
        unsigned int vPort);

/**----------------------------------------------------------------------------
 * @fn SecY_Ifc_Statistics_Clear(
 *     unsigned int DeviceId,
 *     unsigned int vPort)
 *
 * Clear the Ifc statistics counters belonging to a given vPort.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [in] vPort
 *      vPort for which the Ifc statistics must be cleared.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Ifc_Statistics_Clear(
        unsigned int DeviceId,
        unsigned int vPort);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_PnThrSummary_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const SAIndexes_pp,
 *     unsigned int * const NumSAIndexes_p)
 *
 * Reads the values for SA packet number threshold summary registers and clear
 * the bits that are set by hardware. Each bit tell if one SA processed more
 * packets than the threshold set.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] SAIndexes_pp
 *      Pointer to a memory location where the a set of SA indexes whose SA have
 *      processed more than the threshold.
 *
 * @param [out] NumSAIndexes_p
 *      Pointer to a memory location where to save number of SA indexes whose
 *      statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_PnThrSummary_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const SAIndexes_pp,
        unsigned int * const NumSAIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SA_ExpiredSummary_CheckAndClear(
 *     const unsigned int DeviceId,
 *     unsigned int ** const SAIndexes_pp,
 *     unsigned int * const NumSAIndexes_p)
 *
 * Reads the values for SA expired summary registers and clear the bits that are
 * set by hardware. Each bit tell if one SA expired.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device to be used.
 *
 * @param [out] SAIndexes_pp
 *      Pointer to a memory location where the a set of SA indexes whose SA have
 *      processed more than the threshold.
 *
 * @param [out] NumSAIndexes_p
 *      Pointer to a memory location where to save number of SA indexes whose
 *      statistics counters crossed the threshold.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return SECY_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_SA_ExpiredSummary_CheckAndClear(
        const unsigned int DeviceId,
        unsigned int ** const SAIndexes_pp,
        unsigned int * const NumSAIndexes_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Rules_MTUCheck_Update(
 *     const unsigned int DeviceId,
 *     const unsigned int SCIndex,
 *     const SecY_SC_Rule_MTUCheck_t * const MTUCheck_Rule_p)
 *
 * Updates rule for outbound MTU checks.
 *
 * @pre  API use order
 *       Rules can be updated for a SecY device instance only after
 *       this device has been initialized via the SecY_Device_Init()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] SCIndex
 *      Secure Channel index for which the MTU check rule must be updated.
 *
 * @param [in] MTUCheck_Rule_p
 *     Pointer to a memory location where the MTU compare rule
 *     for this Secure Channel are stored.
 *
 * This function is re-entrant for the same or different Devices.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Rules_MTUCheck_Update(
        const unsigned int DeviceId,
        const unsigned int SCIndex,
        const SecY_SC_Rule_MTUCheck_t * const MTUCheck_Rule_p);


/**---------------------------------------------------------------------------
 * @fn SecY_Rules_MTUCheck_Get(
 *       const unsigned int DeviceId,
 *       const unsigned int SCIndex,
 *       SecY_SC_Rule_MTUCheck_t * const MTUCheck_Rule_p);
 *
 * Reads the rule for outbound MTU checks.
 *
 * API use order:
 *       Rules can be read for a SecY device instance only after
 *       this device has been initialized via the SecY_Device_Init()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] SCIndex
 *      Secure Channel index for which the MTU check rule must be updated.
 *
 * @param [out] MTUCheck_Rule_p
 *     Pointer to a memory location where the MTU compare rule
 *     for this Secure Channel are stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Rules_MTUCheck_Get(
        const unsigned int DeviceId,
        const unsigned int SCIndex,
        SecY_SC_Rule_MTUCheck_t * const MTUCheck_Rule_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Rules_SecTag_Update(
 *     const unsigned int DeviceId,
 *     const unsigned int ChannelId,
 *     const SecY_Channel_Rule_SecTAG_t * const SecTag_Rules_p)
 *
 * Updates rules for SecTag parsing.
 *
 * @pre  API use order:\n
 *       Rules can be updated for a SecY device instance only after
 *       this device has been initialized via the SecY_Device_Init()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] ChannelId
 *      The channel number/index for which the SecTag rules need to be updated.
 *
 * @param [in] SecTag_Rules_p
 *     Pointer to a memory location where the SecTAG parsing and compare rules
 *     for each packet are specified.
 *
 * This function is re-entrant for the same or different Devices.\n
 *
 * This function can be called concurrently with any other CfyE or SecY API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Rules_SecTag_Update(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const SecY_Channel_Rule_SecTAG_t * const SecTag_Rules_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_Bypass_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int ChannelId,
 *     bool * const fBypass_p)
 *
 * Read the current channel low-latency bypass setting.
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] ChannelId
 *      The channel number/index for which the bypass setting is read.
 *
 * @param [out] fBypass_p
 *      Pointer to a bool indication in which the setting must be returned
 *      true if channel is set to low-latency bypass, false if channel is
 *      set for normal operation.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_Bypass_Get(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        bool * const fBypass_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_Bypass_Set(
 *     const unsigned int DeviceId,
 *     const unsigned int ChannelId,
 *     const bool fBypass)
 *
 * Configure channel for low-latency bypass.
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] ChannelId
 *      The channel number/index for which the bypass setting is set.
 *
 * @param [in] fBypass
 *      true if channel is set to low-latency bypass, false if channel is
 *      set for normal operation.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_Bypass_Set(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const bool fBypass);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_PacketsInflight_Get(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       bool * const fInFlight_p,
 *       SecY_Ch_Mask_t * const Mask_p);
 *
 * Reads which channels are processing packets (have packets in flight).
 *
 * @note This function is intended for channel switching
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] ChannelId
 *      The channel number/index for which to read the in-flight information.
 *
 * @param [out] fInFlight_p
 *      Pointer to the location to return the in-flight information for the
 *      requested channel. If this set to NULL, this output is not returned.
 *
 * @param [out] Mask_p
 *      Bit mask to specify on which channels have packets in flight
 *      A '1' bit in a bit position signifies that the corresponding channel
 *      has packets in flight. If this us set to NULL, the output is not
 *      returned.
'*
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function can be called concurrently with any other function for the
 * same or different DeviceId's.
 *
 * @return    SECY_STATUS_OK : success
 * @return    SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_PacketsInflight_Get(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        bool * const fInFlight_p,
        SecY_Ch_Mask_t * const Mask_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_Config_Get(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       SecY_Channel_t * const ChannelConfig_p);
 *
 * Read the channel configuration mode of the device.
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in]  ChannelId
 *      The channel number/index for which the configuration is read.
 *
 * @param [out] ChannelConfig_p
 *      Pointer to the variable in which configuration must be stored.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function cannot be called concurrently with
 * SecY_Channel_Config_Set or SecY_Channel_Config_Update for the same
 * DeiveId. It can be called concurrently with other functions for the
 * same DeviceId or any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_Config_Get(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        SecY_Channel_t * const ChannelConfig_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_Config_Update(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       const SecY_Channel_t * const ChannelConfig_p);
 *
 * Update the channel configuration mode of the device.
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device.
 *
 * @param [in]  ChannelId
 *      The channel number/index for which the configuration is updated.
 *
 * @param [out] ChannelConfig_p
 *      Pointer to the channel configuration.
 *
 * This function is re-entrant for the same or different DeviceId's.
 *
 * This function cannot be called concurrently with
 * SecY_Channel_Config_Set or SecY_Channel_Config_Get for the same
 * DeiveId. It can be called concurrently with other functions for the
 * same DeviceId or any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_Config_Update(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const SecY_Channel_t * const ChannelConfig_p);


/**----------------------------------------------------------------------------
 * @fn SecY_Channel_Config_Set(
 *     const unsigned int DeviceId,
 *     const SecY_ChannelConf_t * const ChannelConfig_p)
 *
 * Set the channel configuration of the device.
 *
 * @pre  API use order:\n
 *       This function can be called for a SecY device only after this
 *       device has been initialized via the SecY_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device
 *
 * @param [in] ChannelConfig_p
 *      Configuration parameters describing the channel configuration.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for a different DeviceId.
 *
 * This function cannot be called concurrently with
 * SecY_Channel_Config_Get or SecY_Channel_Config_Update for the same
 * DeiveId. It can be called concurrently with other functions for the
 * same DeviceId or any function for a different DeviceId.
 *
 * @return SECY_STATUS_OK : success
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
SecY_Status_t
SecY_Channel_Config_Set(
        const unsigned int DeviceId,
        const SecY_ChannelConf_t * const ChannelConfig_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SAIndex_Get(
 *     const SecY_SAHandle_t SAHandle,
 *     unsigned int * const SAIndex_p,
 *     unsigned int * const SCIndex_p)
 *
 * Get the SA index from an SA, using the SA handle.
 *
 * @pre API use order:\n
 *      This function may be called for the SAHandle obtained via
 *      the SecY_SA_Add() function for the same DeviceId.
 *
 * @param [in] SAHandle
 *      SA handle of the SA to get the index from.
 *
 * @param [out] SAIndex_p
 *      Pointer to a memory location where the SA index will be stored.
 *
 * @param [out] SCIndex_p
 *      Pointer to a memory location where the SC index will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 */
SecY_Status_t
SecY_SAIndex_Get(
        const SecY_SAHandle_t SAHandle,
        unsigned int * const SAIndex_p,
        unsigned int * const SCIndex_p);


/**----------------------------------------------------------------------------
 * @fn SecY_SAHandle_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int SAIndex,
 *     SecY_SAHandle_t * const SAHandle_p)
 *
 * Get the SA handle from an SA, using the SA index.
 *
 * @pre API use order:\n
 *      This function can be called after the function SecY_Device_Init()
 *      is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the SecY device.
 *
 * @param [in] SAIndex
 *      Secure Association index for which the SA handle must be returned.
 *
 * @param [out] SAHandle_p
 *     Pointer to a memory location where the SA handle will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other SecY API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return SECY_STATUS_OK : success\n
 * @return SECY_ERROR_BAD_PARAMETER : incorrect input parameter
 */
SecY_Status_t
SecY_SAHandle_Get(
        const unsigned int DeviceId,
        const unsigned int SAIndex,
        SecY_SAHandle_t * const SAHandle_p);



#endif /* API_SECY_H_ */


/* end of file api_secy.h */
