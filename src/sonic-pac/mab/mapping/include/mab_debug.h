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

#ifndef INCLUDE_MAB_DEBUG_H
#define INCLUDE_MAB_DEBUG_H

  /* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif

#define MAB_ERROR_SEVERE(format,args...)                  \
{                                                           \
     LOGF(LOG_SEVERITY_ERROR,format,##args); \
}

#define MAB_EVENT_TRACE(__fmt__, __args__...)         \
  {  \
     char8  __buf1__[256] = {0};    \
    (void)osapiSnprintf(__buf1__, 256, __fmt__, ## __args__);  \
     LOGF(LOG_SEVERITY_DEBUG, \
          "[%s:%d]%s",__FUNCTION__, __LINE__, (char *)__buf1__);  \
  }

char *mabHostModeStringGet( AUTHMGR_HOST_CONTROL_t hostMode);
char *mabNodeTypeStringGet(authmgrNodeType_t type);
char *mabTimerTypeStringGet(mabTimerType_t type);
char *mabVlanTypeStringGet(authmgrVlanType_t type);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_DEBUG_H*/
