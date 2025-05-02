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

#ifndef INCLUDE_MAB_IH_H
#define INCLUDE_MAB_IH_H

  /* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif

extern RC_t mabIntfChangeCallback(uint32 intIfNum, uint32 intfEvent,NIM_CORRELATOR_t correlator,
                                   NIM_EVENT_SPECIFIC_DATA_t eventData);
extern RC_t mabIhProcessIntfChange(uint32 intIfNum, uint32 intfEvent, NIM_CORRELATOR_t correlator);
extern RC_t mabIhProcessIntfStartup(NIM_STARTUP_PHASE_t startupPhase);
extern RC_t mabIntfActivateStartup();
extern void mabIntfStartupCallback(NIM_STARTUP_PHASE_t startupPhase);
extern  BOOL mabIntfIsConfigurable(uint32 intIfNum, mabPortCfg_t **pCfg);
extern RC_t mabIntfCreate(uint32 intIfNum);
extern RC_t mabIntfDetach(uint32 intIfNum);
extern RC_t mabIntfDelete(uint32 intIfNum);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /*INCLUDE_MAB_IH_H*/
