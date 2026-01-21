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

#ifndef OSAPI_FILE_H
#define OSAPI_FILE_H

#include "datatypes.h"
#include "commdefs.h"
#include <dirent.h>

/* enumeration of different file types */

#define OSAPI_REGULAR_FILE    "regular"
#define OSAPI_DIRECTORY       "directory"
#define OSAPI_CONFIGSCR       "config-script"
#define OSAPI_IMAGE           "code-image"
#define OSAPI_CONFIGURATION   "configuration"
#define OSAPI_SYSTEM_FILE     "system"
#define OSAPI_SPECIAL_FILE    "special"
#define OSAPI_UNKNOWN         "unknown"

typedef enum
{
  OSAPI_TYPE_REGULAR_FILE = 1,
  OSAPI_TYPE_DIRECTORY,
  OSAPI_TYPE_CONFIGSCR,
  OSAPI_TYPE_IMAGE,
  OSAPI_TYPE_CONFIGURATION,
  OSAPI_TYPE_SYSTEM_FILE,
  OSAPI_TYPE_SPECIAL_FILE,
  OSAPI_TYPE_UNKNOWN,
  OSAPI_TYPE_MAX
} osapiFileType_t;

/* enumeration of different permission modes */

#define OSAPI_RO              "ro"
#define OSAPI_RW              "rw"
#define OSAPI_WO              "wo"
//#include "transfer_exports.h"

/* Buffer size for command "cp -f >/dev/null 2>&1" which is used in osapiFsCopyFile function */
#define  TRANSFER_CP_CMD_SIZE  32
/* Buffer size for full content of the cp command with all arguments */
#define  FULL_CP_CMD_BUFF_SIZE ( MAX_FILENAME_BUF_SIZE +  MAX_FILEPATH +  TRANSFER_CP_CMD_SIZE +  TRANSFER_SRC_PATH_BUF_SIZE)

typedef enum
{
  OSAPI_PERM_RO = 1,
  OSAPI_PERM_RW,
  OSAPI_PERM_WO,
  OSAPI_PERM_XO,
  OSAPI_PERM_RWX,
  OSAPI_PERM_MAX
} osapiFilePermission_t;

typedef enum
{
  OSAPI_FILE_CREATE = 0,
  OSAPI_FILE_OPEN,
  OSAPI_FILE_READ,
  OSAPI_FILE_WRITE,
  OSAPI_FILE_CLOSE,
  OSAPI_FILE_DELETE,
  OSAPI_FILE_CP_CREATE, /* Create file for Captive Portal Download */
  OSAPI_FILE_NV_CREATE /* Create file for Network Visualization Download */
} osapiFileOperation;


/**************************************************************************
 *
 * @purpose  Opens a file
 *
 * @param    filename  @b{(input)}  File to Open
 * @param    filedesc  @b{(input)}  Pointer to file descriptor
 *
 * @returns   ERROR if file does not exist
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiFsOpen ( char8 * filename,  int32 *filedesc);

/**************************************************************************
 *
 * @purpose  Opens a file in read-only mode
 *
 * @param    filename  @b{(input)}  File to Open
 * @param    filedesc  @b{(input)}  Pointer to file descriptor
 *
 * @returns   ERROR if file does not exist
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiFsOpenRDonly ( char8 * filename,  int32 *filedesc);

/**************************************************************************
 *
 * @purpose  closes a file opened by osapiFsOpen
 *
 * @param    filedesc  @b{(input)}  descriptor of file to close
 *
 * @returns   SUCCESS
 * @returns   ERROR if file already closed
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiFsClose ( int32 filedesc);

/**************************************************************************
 *
 * @purpose  Retrieve file size
 *
 * @param    filename  @b{(input)}  string of file to retrieve size
 * @param    filesize  @b{(output)} integer of file size
 *
 * @returns   SUCCESS
 * @returns   ERROR
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
EXT_API RC_t osapiFsFileSizeGet ( char8 * filename, uint32 * filesize);

#endif /* OSAPI_FILE_H */
