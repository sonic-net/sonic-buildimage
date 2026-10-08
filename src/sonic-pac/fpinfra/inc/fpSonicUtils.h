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

#ifndef FPSONICUTILS_H_H

#define FPSONICUTILS_H_H

#ifdef __cplusplus

#define PAC_GET_STD_IF_FORMAT(key)({                         \
    string __in(key);                                        \
    string __out("");                                        \
    if(__in.length() > 8)                                     \
   {                                                         \
      /* Native format: Ethernetx, nothing to do    */       \
      __out = __in;                                           \
   }                                                         \
   else                                                      \
   {                                                         \
     __out = "Eth" + __in.substr(1,1) + '/' + __in.substr(3);  \
   }                                                         \
   __out.c_str();                                            \
})

extern "C" {

#endif

extern int fpGetIntIfNumFromHostIfName(const char *ifName, uint32 *outIntfNum);

extern int fpGetHostIntfName(uint32 physPort,  uchar8 *ifName);



#ifdef __cplusplus

}

#endif

#endif /* FPSONICUTILS_H_H */
