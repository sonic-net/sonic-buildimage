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


#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/statfs.h>

#include "log.h"
#include "osapi_file.h"

/**************************************************************************
*
* @purpose  Opens a file
*
* @param    filename @b{(input)} File to Open
* @param    fd       @b{(input)}  Pointer to file descriptor
*
* @returns   ERROR if file does not exist
*
* @comments    none.
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiFsOpen( char8 *filename,  int32 *fd)
{
  if ( NULLPTR == filename)
  {
    return  FAILURE;
  }
  if ((*fd = open(filename, O_RDWR | O_SYNC, 0644)) == -1)
  {
    return( ERROR);
  }

  return( SUCCESS);
}

/**************************************************************************
*
* @purpose  Opens a file in read-only mode
*
* @param    filename @b{(input)} File to Open
* @param    fd       @b{(input)}  Pointer to file descriptor
*
* @returns   ERROR if file does not exist
*
* @comments    none.
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiFsOpenRDonly( char8 *filename,  int32 *fd)
{
  if ( NULLPTR == filename)
  {
    return  FAILURE;
  }
  if ((*fd = open(filename, O_RDONLY)) == -1)
  {
    return( ERROR);
  }

  return( SUCCESS);
}

/**************************************************************************
*
* @purpose  closes a file opened by osapiFsOpen
*
* @param    filedesc @b{(input)} descriptor of file to close
*
* @returns   SUCCESS
* @returns   ERROR if file already closed
*
* @comments    none.
*
* @end
*
*************************************************************************/
EXT_API RC_t osapiFsClose( int32 filedesc)
{
   int32 ret;
  if ((filedesc < 3) || (filedesc > FD_SETSIZE))
  {
     LOGF( LOG_SEVERITY_ERROR,
            "Out of range file descriptor argument %d", filedesc);
    return  ERROR;
  }

  while (((ret = close(filedesc)) == -1) && (errno == EINTR))
  /* Nothing*/;
  if (ret < 0)
  {
     LOGF( LOG_SEVERITY_ERROR,
            "File close for descriptor %d asserted errno %d", filedesc, errno);
    return  ERROR;
  }

  sync();

  return( SUCCESS);
}

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
EXT_API RC_t osapiFsFileSizeGet ( char8 *filename, uint32 *filesize)
{
   int32 filedesc = -1;
  struct stat fileStat;

  if (( NULLPTR == filename) || ( NULLPTR == filesize))
  {
    return  ERROR;
  }

  /* file size is 0 if  ERROR or the file does not exist */
  *filesize =  0;
  if ((filedesc = open (filename, O_RDWR, 0)) <  0)
  {
    return  ERROR;
  }

  if (fstat (filedesc, &fileStat) < 0)
  {
    close (filedesc);
    return  ERROR;
  }

  if (close (filedesc) < 0)
  {
    return  ERROR;
  }

  *filesize = (uint32) (fileStat.st_size);
  return  SUCCESS;
}
