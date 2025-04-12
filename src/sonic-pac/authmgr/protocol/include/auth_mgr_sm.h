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

#ifndef INCLUDE_AUTHMGR_SM_H
#define INCLUDE_AUTHMGR_SM_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

typedef enum authmgrEvents_s
{
  /***************************************************************/
  /* Events just for Authenticator PAE Machine (APM)             */
  /***************************************************************/
  /*0*/authmgrInitialize = 0,                               /*E1.*/
  /*1*/authmgrStartAuthenticate,                            /*E2.*/
  /*2*/authmgrAuthSuccess,                                  /*E3*/
  /*3*/authmgrNotAuthSuccessNoNextMethod,                   /*E4*/
  /*4*/authmgrNotAuthSuccessNextMethod,                     /*E5*/
  /*5*/authmgrHeldTimerEqualsZero,                          /*E6*/
  /*6*/authmgrStopAuthenticate,                             /*E7*/
  /*7*/authmgrHigherAuthMethodAdded,                        /*E8*/
  /*8*/authmgrReauthenticate,                               /*E9*/
  /*9*/authmgrAuthenticatedRxEapolStart,                    /*E10*/
  /*10*/authmgrAuthFail,                                    /*E11*/
  /*11*/authmgrAbortAndRestartAuth,                         /*E12*/
  /*12*/authmgrSmEvents/*keep this last in sub group*/
 
}authmgrSmEvents_t;

struct authmgrLogicalPortInfo_s;
extern RC_t authmgrStateMachineClassifier(authmgrSmEvents_t authmgrEvent, uint32 lIntIfNum); 

extern RC_t authmgrAuthenticationInitiate(uint32 lIntIfNum);

extern void authmgrAbortAuth(struct authmgrLogicalPortInfo_s *logicalPortInfo);

RC_t authmgrDisableAuthenticatorPorts(uint32 intIfNum);
RC_t authmgrEnableAuthenticatorPorts(uint32 intIfNum);

/*********************************************************************
 * @purpose  Events that the APM needs to generate and propagate
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 * @param   *msg     @b{(input)) message containing event, bufHandle
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrGenerateEvents(uint32 lIntIfNum);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_SM_H */
