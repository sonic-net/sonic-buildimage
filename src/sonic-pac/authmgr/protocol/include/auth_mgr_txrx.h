/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef INCLUDE_AUTHMGR_TXRX_H
#define INCLUDE_AUTHMGR_TXRX_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "auth_mgr_common.h"

extern RC_t authmgrTxRxInit(void);
extern RC_t authmgrPduReceive( netBufHandle bufHandle, sysnet_pdu_info_t *pduInfo);
extern void authmgrTxRxHeaderTagRemove( netBufHandle bufHandle);

extern void authmgrTxReqId(uint32 intIfNum, authmgrPortType_t portType);
extern void authmgrTxReq(authmgrLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle);
extern void authmgrTxCannedSuccess(uint32 intIfNum, authmgrPortType_t portType);
extern void authmgrTxCannedFail(uint32 intIfNum, authmgrPortType_t portType);
extern void authmgrFrameTransmit(uint32 intIfNum,  netBufHandle bufHandle,authmgrPortType_t portType);
extern RC_t authmgrTxRxHostConvert( netBufHandle bufHandle);
extern RC_t authmgrSendRespToServer(uint32 intIfNum,  netBufHandle bufHandle);
extern RC_t authmgrRadiusSendNotification(uint32 intIfNum,
                                            uchar8 *radiusPayload,
                                           uint32 payloadLen);
extern RC_t authmgrTxReqIdCheck(uint32 intIfNum);
extern void authmgrTxStart(uint32 intIfNum);
extern void authmgrTxSuppRsp(uint32 intIfNum);
extern RC_t authmgrCtlPacketInterceptRegister ( BOOL reg);
extern RC_t authmgrPacketTransmit(uint32 intIfNum,  ushort16 vlanId,
                                      uchar8 *frame,  ushort16 frameLen);

extern RC_t authmgrPacketFlood(uint32 intIfNum,  ushort16 vlanId,
                                   uchar8 *frame,  ushort16 frameLen);

RC_t auth_mgr_eap_socket_create( int32 *fd);
/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_TXRX_H */
