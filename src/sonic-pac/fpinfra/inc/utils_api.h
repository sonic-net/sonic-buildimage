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

#ifndef _UTILS_API_H_
#define _UTILS_API_H_

#include "pacinfra_common.h"
//#include "l3_commdefs.h"

/*********************************************************************
* @purpose  Given a pointer to full path of a file, change the pointer
*           to point to the beginning of the file name
*
* @param    **fullPath    @{(input)}  Address of pointer to full path to file
*
* @returns  none
*
* @end
*********************************************************************/
void utilsFilenameStrip( char8 **fullPath);

#endif //_UTILS_API_H_
