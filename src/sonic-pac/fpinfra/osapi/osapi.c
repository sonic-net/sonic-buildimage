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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <linux/rtc.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <dlfcn.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>

#include "datatypes.h"
#include "commdefs.h"

#include "log.h"

#include "osapi.h"
#include "osapi_priv.h"
#include "osapi_sem.h"


/* taken from VxWorks OSAPI implementation */
static void *syncSemaArray[ TASK_SYNC_LAST] = { NULL};

static pthread_mutex_t task_list_lock = PTHREAD_MUTEX_INITIALIZER;
static osapi_task_t *task_list_head = NULL;

static pthread_mutex_t zombie_tasks_lock = PTHREAD_MUTEX_INITIALIZER;
static osapi_task_t *zombie_tasks_list = NULL;
static pthread_cond_t zombie_tasks_cond;

static uint32 system_page_size = 0;

int osapi_printf(const char *fmt, ...)
{
  int rc;
  va_list args;

  va_start(args, fmt);
  rc = sysapiVaPrintf(fmt, args);
  va_end(args);

  return(rc);
}

/**************************************************************************
 *
 * @purpose  Retrieve number of milliseconds since last reset
 *
 * @param    void
 *
 * @returns  rawtime in milliseconds
 *
 * @comments
 *
 * @end
 *
 *************************************************************************/
uint32 osapiUpTimeMillisecondsGet ( void )
{
  struct timespec tp;

  (void) clock_gettime(CLOCK_MONOTONIC, &tp);

  return(( tp.tv_sec * 1000 ) + ( tp.tv_nsec / 1000000 ));
}


/**************************************************************************
 *
 * @purpose  Retrieve number of milliseconds
 *          AVOID THIS FUNCTION
 *          Use the osapiUpTimeMillisecondsGet()
 *
 * @param    void
 *
 * @returns  milliseconds since last reset
 *
 * @notes
 *
 * @end
 *
 *************************************************************************/
uint32 osapiTimeMillisecondsGet( void )
{
  return osapiUpTimeMillisecondsGet ();
}


/*********************************************************************
 * @purpose  Returns the function containing a given address
 *
 * @param    addr, uint32
 *           funcName,  char8* buffer allocated by caller
 *           funcNameLen, uint32 length of name buffer
 *           offset, uint32* pointer to uint32 allocated by caller.
 *                   Address's offset from beginning of function stored there.
 *
 * @returns   SUCCESS if function found, name printed into funcName buffer,
 *                      offset set into *offset.
 *            FAILURE if function not found, buffers untouched.
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t osapiFunctionLookup(void * addr,  char8 *funcName,
                            uint32 funcNameLen, uint32 * offset)
{
  /* Not required */
  return  FAILURE;
}


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
void utilsFilenameStrip( char8 **fullPath)
{
  uint32 i;
   char8 *fileP;

  if ((fullPath ==  NULLPTR) || (*fullPath ==  NULLPTR))
    return;

  i = strlen(*fullPath);
  fileP = &((*fullPath)[i]);

  /* look for the first slash from the end of string */
  for (; i > 0; i--)
  {
    fileP--;

    /* check for forward or backward slash (need two '\' since first one's an escape char) */
    if (*fileP == '/' || *fileP == '\\')
    {
      fileP++;    /* went too far, move ahead one char */
      break;
    }
  }

  *fullPath = fileP;
}

/**************************************************************************
 **
 ** @purpose  Copy a string to a buffer with a bounded length (with safeguards)
 **
 ** @param    *dest       @b{(input)}  destination location
 ** @param    *src        @b{(input)}  source string ptr
 ** @param    n           @b{(input)}  maximum number of characters to copy
 **
 ** @returns  Pointer to destination location (always)
 **
 ** @comments The dest and src strings must not overlap.  The dest location
 **           must have enough space for n additional characters.
 **
 ** @comments No more than n characters are copied from src to dest.  If there
 **           is no '\0' character within the first n > 0 characters of src, the
 **           n-th byte of dest string will be written with the '\0'
 **           string termination character (e.g., if n=10, *(dest+9)='\0').
 **
 ** @comments If the src string length is less than n, the remainder of
 **           dest is padded with nulls.
 **
 ** @comments To maintain consistency with the POSIX functions, the des
 **           pointer is returned, even if no characters were actually copied
 **           (e.g., src string has zero length, src ptr is null, n is 0,
 * *           etc.)
 * *
 * * @end
 * *
 * *************************************************************************/
 char8 *osapiStrncpySafe( char8 *dest, const  char8 *src, uint32 n)
{
   char8      *retptr = dest;

  /* don't copy anything if there is no dest location */
  if (dest ==  NULLPTR)
    return retptr;

  if (src ==  NULLPTR)
  {
    if (n > 0)
    {
      /* null src ptr, but n > 0:  pad dest to ensure termination */
      memset(dest,  EOS, n);
    }
    return retptr;
  }
 
  if (n > 0)
  {
    memcpy(dest, src, n);
    *(dest+n-1) =  EOS;
  }

  return retptr;
}

/**************************************************************************
*
* @purpose     Convert an IP address from a hex value to an ASCII string
*
* @param       ipAddr     @b{(input)}  IP address to be converted (host byte
*                                        order)
* @param       buf        @b{(output)} location to store IP address string
*
* @returns     void
*
* @comments    Caller must provide an output buffer of at least
* @comments    OSAPI_INET_NTOA_BUF_SIZE bytes (as defined in
* @comments    osapi.h).
* @comments    'ipAddr' is supplied in HOST BYTE ORDER
*
* @end
*
*************************************************************************/
void osapiInetNtoa (uint32 ipAddr,  uchar8 *buf)
{
  sprintf( buf, "%d.%d.%d.%d",
           (ipAddr & 0xff000000) >> 24,
           (ipAddr & 0x00ff0000) >> 16,
           (ipAddr & 0x0000ff00) >> 8,
           (ipAddr & 0x000000ff) );
}

/**************************************************************************
*
* @purpose  map  AF to AF
*
* @param    l7af          AF type
*
* @returns  stack AF
*
* @comments static use only. here since osapi callers may not have access
* to AF_ values of stack. Values are NOT the same.
*
* @end
*
*************************************************************************/
static uint32 osapiFromL7AF(uint32 l7af)
{
  switch(l7af)
  {
  case  AF_INET:
      return AF_INET;
  case  AF_INET6:
      return AF_INET6;
  default:
      break;
  }
  return 0;
}

/**************************************************************************
*
* @purpose  convert a dot notation Internet address to a long integer
*
* @param    address @b{(input)} IP address in dotted notation
*
* @returns  IP address in long integer
*
* @end 
*
*************************************************************************/
uint32 osapiInet_addr( uchar8 *address)
{
  uint32 longAddress;
  longAddress=osapiNtohl(inet_addr(address));
  return longAddress;
}

/**************************************************************************
*
* @purpose      create ascii displayable string from IP (4/6) address
*
* @param        family          address faminly
*               addr            pointer to address
*               str             pointer to ouput string
*               len             length of output string
*
* @returns      output string or  NULL
*
*
* @end
*
*************************************************************************/
 uchar8 *osapiInetNtop(uint32 family,  uchar8 *addr,  uchar8 *str, uint32 len)
{
   const  uchar8 *rp;

   family = osapiFromL7AF(family);
   rp =  inet_ntop(family,addr,str,len);
   return ( uchar8 *)rp;
}

/*********************************************************************
* @purpose  Returns the string representation of given address
*
* @param    addr, void *
*           buf,  char8* buffer allocated by caller
*           bufsize, uint32 length of name buffer
*
* @returns  None
*
* @comments if function found, address, name and offset are printed
*           info passed buffer else only address is printed
*
* @comments
*
* @end
*********************************************************************/
void osapiAddressStringify (void *addr,  char8 *buf, uint32 bufsize)
{
  osapiSnprintf(buf, bufsize, "$%p$ ?????", addr);
}

uint32 osapiUpTimeRaw(void)
{
  struct timespec tp;
  int rc;

  rc = clock_gettime(CLOCK_MONOTONIC, &tp);
  if (rc < 0)
  {
      return 0;
  }
  return(tp.tv_sec);
}

/*******************************************************************************
* @purpose      To locate character in string
*
* @param   *s    @b{(input)} input string to search for the character
* @param   len   @b{(input)} len of the input string
* @param   c         @b{(input)} character to search for
* @returns      a pointer to the matched character or NULL if the character is not found
*
* @comments 
*
* @end
*******************************************************************************/
 char8 *osapiStrnchr (const  char8 *s, uint32 len,  char8 c)
{
  uint32 i;

  for (i = 0; i < len; i++)
  {
    if (s[i] == 0)
    {
      return 0;
    }
    if (s[i] == c)
    {
      return (char *)s + i;
    }
  }

  return 0;
}

/**********************************************************************
 * @purpose  Get the current UTC time since the Unix Epoch.
 *
 * @param    ct @b{(output)} UTC time
 *
 * @returns   SUCCESS
 * 
 * @notes    NTP epoch is different from Unix epoch.
 * 
 * @end
 *********************************************************************/
RC_t osapiUTCTimeGet( clocktime * ct)
{
  struct timeval tv;

  memset (&tv, 0, sizeof (tv));
  gettimeofday(&tv,NULL);
  ct->seconds = tv.tv_sec; /* local time as per the configured timezone */
  tzset(); /* Read timezone information in kernel */
#if !defined(PC_LINUX_HOST) || defined( CHIP_LINE)
  ct->seconds += timezone; /* Adjust local time to UTC */
#endif /* defined PC_LINUX_HOST && !defined  CHIP_LINE */
  ct->nanoseconds = tv.tv_usec * 1000; /* usec to nsec conversion */ 
  return  SUCCESS;
}
