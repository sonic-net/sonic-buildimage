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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <arpa/inet.h>
#include "defaultconfig.h"
#include "pacinfra_common.h"
#include "osapi.h"
#include "log.h"
#include "osapi_support.h"

#ifdef  RLIM_PACKAGE
#include "rlim_api.h"
#endif

#ifdef PC_LINUX_HOST
#include <linux/types.h>
#endif
#include <linux/icmpv6.h>  /* after socket.h */

#define ERROR (-1)

#ifdef PC_LINUX_HOST
#define  SETSOCKOPT_ERROR  SUCCESS
#else
#define  SETSOCKOPT_ERROR  FAILURE
#endif

extern int osapi_proc_set(const char *path, const char *value);

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
* @purpose  map AF to L7AF
*
* @param    af         AF type
*
* @returns   AF
*
* @comments static use only
*
* @end
*
*************************************************************************/
static uint32 osapiToL7AF(uint32 af)
{
  switch(af)
  {
  case AF_INET:
      return  AF_INET;
  case AF_INET6:
      return  AF_INET6;
  default:
      break;
  }
  return 0;
}

/**************************************************************************
*
* @purpose  map  L7 socket type to stack socket type
*
* @param    l7_stype     L7 socket type
*
* @returns  stack socket type
*
* @comments static use only
*
* @end
*************************************************************************/
static uint32 osapiFromL7SockType(uint32 l7_stype)
{
  switch(l7_stype)
  {
  case  SOCK_RAW:
      return SOCK_RAW;
  case  SOCK_DGRAM:
      return SOCK_DGRAM;
  case  SOCK_STREAM:
      return SOCK_STREAM;
  default:
      break;
  }
  return 0;
}

/**************************************************************************
*
* @purpose  Create a socket
*
* @param    domain   @b{(input)}   address family (for example, AF_INET)
* @param    type     @b{(input)}   SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW
* @param    protocol @b{(input)}   protocol to be used
* @param    descriptor      @b{(input)}   ptr to socket descriptor
*
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
uint32 osapiSocketCreate(uint32 domain,uint32 type,uint32 protocol, int32 *descriptor)
{
   int32 fd = -1;

  domain = osapiFromL7AF(domain);
  type = osapiFromL7SockType(type);

  fd = socket(domain,type,protocol);
  if (fd == ERROR)
  {
    return  ERROR;
  }
  *descriptor = fd;
  return  SUCCESS;
}


/**************************************************************************
*
* @purpose  Bind a name to a socket
*
* @param    socketDescriptor  @b{(input)} socket descriptor
* @param    family            @b{(input)} Address Family (for example, AF_INET)
* @param    bindIP            @b{(input)} IP Address to bind with socket
* @param    port              @b{(input)} Port
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/

RC_t osapiSocketBind(uint32 socketDescriptor,  sockaddr_t *saddr, uint32 s_len)
{
  /* os sockaddr has (ushort sa_family), l7_sockaddr has (u8)sa_len/(u8)sa_family */
   sockaddr_union_t c_saddr;
  struct sockaddr *os_saddr = (struct sockaddr *)&c_saddr;


  if(s_len > sizeof(c_saddr)) return  FAILURE;
  memcpy(&c_saddr, saddr, s_len);
  os_saddr->sa_family = ( ushort16)osapiFromL7AF(saddr->sa_family);

  if ( bind(socketDescriptor,os_saddr, s_len) == ERROR)
    return  FAILURE;
  else
    return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Get peer name
*
* @param    socketDescriptor  @b{(input)} socket descriptor
* @param    saddr             @b{(output)} remote address
* @param    s_len             @b{(inout)} remote address length
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
RC_t osapiGetpeername(uint32 socketDescriptor,
                          sockaddr_t *saddr, uint32 *s_len)
{
  /* sa_len/sa_family vs (u16)sa_family */
  struct sockaddr *os_saddr = (struct sockaddr *)saddr;

  if(getpeername(socketDescriptor, (struct sockaddr *)saddr, (socklen_t *)s_len) < 0)
  {
     LOGF( LOG_SEVERITY_DEBUG,
            "Unable to get the peer info for socket descriptor %d. Error = %s.",
            socketDescriptor, strerror(errno));
    return  FAILURE;
  }
  saddr->sa_family = ( uchar8)osapiToL7AF(os_saddr->sa_family);
  saddr->sa_len = ( uchar8)(*s_len);
  return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Receive data from a socket
*
* @param      socketDescriptor  @b{(input)} socket to receive from
* @param      buf               @b{(input)} pointer to data buffer
* @param      length            @b{(input)} length of buffer
* @param      flag              @b{(input)} flags to underlying protocols
* @param      family            @b{(output)} Ptr to Address Family for received data
* @param    port              @b{(output)} Ptr to Port
* @param    ipAddr            @b{(output)} Ptr to IP Address
* @param    bytesRcvd         @b{(output)} Ptr to number of bytes received
*
* @returns   SUCCESS
* @returns   ERROR
*
* @end
*
*************************************************************************/
RC_t osapiSocketRecvfrom(uint32 socketDescriptor,  uchar8 *buf, uint32 length,
                              uint32 flag,  sockaddr_t *from, uint32 *from_len,
                              uint32 *bytesRcvd)
{
   int32 count;
  /* sa_len/sa_family vs (u16)sa_family */
  struct sockaddr *os_saddr = (struct sockaddr *)from;

  do 
  {
    count= recvfrom(socketDescriptor, buf, length,flag, (struct sockaddr *)from,from_len);
  } while ((0 > count) && (EINTR == errno));

  if (count < 0)
  {
    return  ERROR;
  }

  *bytesRcvd=count;
  from->sa_family = ( uchar8)osapiToL7AF(os_saddr->sa_family);
  from->sa_len = *from_len;

  return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Set socket options
*
* @param    targetSocket @b{(input)} socket to receive from
* @param    level        @b{(input)} protocol level of option
* @param    optname      @b{(input)} option name
* @param    optval       @b{(input)} pointer to option value
* @param    optlen       @b{(input)} option length
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
RC_t osapiSetsockopt(uint32 targetSocket,uint32 level,uint32 optname,
                         uchar8 *optval,uint32 optlen)
{
  /* converts to SOL_SOCKET, IPPROT_xxx vals are IETF defined and dont need
     conversion
  */
#if defined( IPV6_PACKAGE) || defined( IPV6_MGMT_PACKAGE)
  uint32 i;
   int32  sd = -1;
  struct  ip_mreq6_s *l7mreq6 = (struct  ip_mreq6_s *)optval;
  struct ipv6_mreq     mreq6;
  struct ifreq         ifr;
  int realval;
#endif

  if(level ==  SOL_SOCKET)
  {
    level = SOL_SOCKET;

     /* convert optname */
     switch(optname)
     {
     case  SO_REUSEADDR:
        optname = SO_REUSEADDR;
        break;
     case  SO_BROADCAST:
        optname = SO_BROADCAST;
        break;
/* tbd jpp
     case  SO_REUSEPORT:
        optname = SO_REUSEPORT;
        break;
*/
     case  SO_DONTROUTE:
        optname = SO_DONTROUTE;
        break;
     case  SO_SNDBUF:
       {
    FILE *max_wmem_fp;
    char buf[11];
    int cur_max_wmem = 0, requested_wmem;

    max_wmem_fp = fopen("/proc/sys/net/core/wmem_max", "r");
    if (max_wmem_fp)
    {
      requested_wmem = *(int *)optval;
      if(0 > fscanf(max_wmem_fp, "%d", &cur_max_wmem)){}
      fclose(max_wmem_fp);
      if (requested_wmem > cur_max_wmem)
      {
        snprintf(buf, 10, "%d", requested_wmem);
        osapi_proc_set("/proc/sys/net/core/wmem_max", buf);
      }
    }
        optname = SO_SNDBUF;

        break;
       }
     case  SO_RCVBUF:
       {
    FILE *max_rmem_fp;
    char buf[11];
    int cur_max_rmem = 0, requested_rmem;

    max_rmem_fp = fopen("/proc/sys/net/core/rmem_max", "r");
    if (max_rmem_fp)
    {
      requested_rmem = *(int *)optval;
      if(0 > fscanf(max_rmem_fp, "%d", &cur_max_rmem)){}
      fclose(max_rmem_fp);
      if (requested_rmem > cur_max_rmem) {
        snprintf(buf, 10, "%d", requested_rmem);
        osapi_proc_set("/proc/sys/net/core/rmem_max", buf);
      }
    }
        optname = SO_RCVBUF;
        break;
       }
     case  SO_RCVTIMEO:
        optname = SO_RCVTIMEO;
        break;
     case  SO_ERROR:
        optname = SO_ERROR;
        break;
     case  SO_TYPE:
        optname = SO_TYPE;
        break;
     case  SO_KEEPALIVE:
        optname = SO_KEEPALIVE;
        break;
     case  SO_BINDTODEVICE:
        optname = SO_BINDTODEVICE;
        break;
     default:
        return  FAILURE;
     }
  }
  else
  {
     return  FAILURE;
  }


  if (setsockopt(targetSocket,level,optname,optval,optlen) == ERROR)
    return  SETSOCKOPT_ERROR;

  return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Send a message to a socket
*
* @param    s         @b{(input)} socket to send
* @param    buf       @b{(input)} pointer to data buffer
* @param    bufLen    @b{(input)} length of buffer
* @param    flags     @b{(input)} flags to underlying protocols
* @param    family    @b{(input)} Address Family
* @param  port      @b{(input)} Port
* @param  ipAddr    @b{(input)} IP Address
* @param  bytesSent @b{(output)} Ptr to number of bytes sent
*
* @returns   SUCCESS
* @returns   ERROR
*
* @end
*
*************************************************************************/

RC_t osapiSocketSendto(uint32 s, uchar8 *buf,uint32 bufLen,uint32 flags,
                           sockaddr_t *to, uint32 to_len,
                          uint32 *bytesSent)
{
   int32 count;
   sockaddr_union_t c_saddr;
  struct sockaddr *os_saddr = (struct sockaddr *)&c_saddr;

  if(to_len > sizeof(c_saddr)) return  FAILURE;
  memcpy(&c_saddr, to, to_len);
  /* AF values abstracted from caller, so dont need to include world */
  os_saddr->sa_family = ( ushort16)osapiFromL7AF(to->sa_family);

  do 
  {
    count = sendto(s,buf,bufLen,flags,os_saddr,to_len);
  } while ((count < 0) && (errno == EINTR));

  if (count < 0)
  {
    return  ERROR;
  }
  *bytesSent = count;
  return  SUCCESS;
}

/**************************************************************************
*
* @purpose  Close a socket
*
* @param    fd @b{(input)} socket descriptor to close
*
* @returns  none
*
* @end
*
*************************************************************************/
void osapiSocketClose(uint32 fd)
{
  int rc = 0;
  if ((fd < 3) || (fd > FD_SETSIZE))
  {
     LOGF( LOG_SEVERITY_NOTICE,
            "Socket close passed invalid descriptor %d. Socket not closed.", fd);
  }
  else
  {
    while (((rc = close(fd)) < 0) && (errno == EINTR))
    /* Nothing */;
    if (rc < 0)
    {
       LOGF( LOG_SEVERITY_NOTICE,
              "Socket close asserted %s for descriptor %d", strerror(errno),fd);
    }
  }
}
/**************************************************************************
*
* @purpose  Shutdown a socket
*
* @param    fd  @b{(input)} socket descriptor to close
* @param    how @b{(input)} how to shutdown
*
* @returns  none
*
* @end
*
*************************************************************************/
RC_t osapiShutdown(uint32 fd, uint32 how)
{
  int rc;

  switch(how)
  {
  case  SHUT_RD:
     how = SHUT_RD;
     break;
  case  SHUT_WR:
     how = SHUT_WR;
     break;
  case  SHUT_RDWR:
     how = SHUT_RDWR;
     break;
  default:
     return  FAILURE;
  }


  rc = shutdown(fd,(int)how) ;
  return (rc < 0)? FAILURE: SUCCESS;
}

/**************************************************************************
*
* @purpose      pend on a set of file descriptors
*
* @param        width           @b{(input)} number of bits to examine from 0
* @param        rFds            @b{(input)} read fds
* @param        wFds            @b{(input)} write fds
* @param        eFds            @b{(input)} exception fds
* @param        timeOutSec      @b{(input)} max time in sec to wait,
*                                           NULL = forever
* @param        timeOutMicroSec @b{(input)} time in microsec
*
* @returns      The number of file descriptors with activity, 0 if timed out
*               or ERROR
*
* @end
*
*************************************************************************/

 int32 osapiSelect(uint32 width, fd_set* rFds, fd_set* wFds,
                      fd_set* eFds,  int32 timeOutSec,
                       int32 timeOutMicroSec)
{
   int32 noFD;
  struct timeval timeout;

  if ((timeOutSec == 0) && (timeOutMicroSec == 0))
  {
    /* must pass null pointer to wait forever */
    noFD = select(width, rFds, wFds, eFds,  NULLPTR);
  }
  else
  {
    timeout.tv_sec=timeOutSec;
    timeout.tv_usec=timeOutMicroSec;

    noFD = select(width, rFds, wFds, eFds, &timeout);
  }
  return noFD;
}

/**************************************************************************
* @purpose  Read bytes from the file
*
* @param    fd       @b{(input)} file descriptor
* @param    buffer   @b{(output)} ptr to buffer to read data
* @param    maxbytes @b{(input)} no. of bytes
*
* @returns  The number of bytes read from file or -1 on error.
*
* @end
*
*************************************************************************/

 int32 osapiRead(uint32 fd, char8* buffer,size_t maxbytes)
{
   int32 num;
  num = read(fd,buffer,maxbytes);
  return num;
}

/**************************************************************************
* @purpose  write bytes to the file
*
* @param    fd        @b{(input)} file descriptor
* @param    buffer    @b{(input)} ptr to buffer to write data
* @param    maxbytes  @b{(input)} no. of bytes
*
* @returns  The number of bytes written to file or -1 on error.
*
* @end
*
*************************************************************************/

 int32 osapiWrite(uint32 fd, char8* buffer,size_t maxbytes)
{
   int32 num;

  do
  {
    num = write(fd,buffer,maxbytes);
  } while ((0 > num) && (EINTR == errno));

  return num;
}



/**************************************************************************
* @purpose  Get Operating System Error Number
*
*
* @returns  ERRNO set by the system.
*
* @end
*
*************************************************************************/
extern uint32 osapiErrnoGet(void)
{
  return(errno);
}

/**************************************************************************
* @purpose  Get Operating System Error String
*
* @returns  string corresponding to ERRNO set by the system.
*
* @end
*
*************************************************************************/
 char8 *osapiErrStrGet(void)
{
  return strerror(errno);
}

/**************************************************************************
*
* @purpose      Initiate a connection on a socket
*
* @param        sockFd          @b{(input)} socket descriptor
* @param        family          @b{(input)} Address Family (for example, AF_INET)
* @param        ipAddr          @b{(input)} IP Address to connect with socket
* @param        port            @b{(input)} Port
*
* @returns       SUCCESS      if the connection or binding succeeds
*                FAILURE      on error
*
* @end
*
*************************************************************************/

RC_t osapiConnect(uint32 sockFd, sockaddr_t *saddr, uint32 s_len)
{
   sockaddr_union_t c_saddr;
  struct sockaddr *os_saddr = (struct sockaddr *)&c_saddr;

  if(s_len > sizeof(c_saddr)) return  FAILURE;
  memcpy(&c_saddr, saddr, s_len);
  os_saddr->sa_family = ( ushort16)osapiFromL7AF(saddr->sa_family);

  if (connect (sockFd, os_saddr, s_len) == ERROR)
    return  FAILURE;
  else
    return  SUCCESS;
}
/**************************************************************************
*
* @purpose      Accept a connection on a socket
*
* @param        sockFd          @b{(input)}  socket fd
* @param        saddr           @b{(input)}  pointer to connector's address info.
* @param        s_len           @b{(input)}  length of connector's address info.
* @param        descriptor      @b{(output)} socket descriptor
*
* @returns       SUCCESS      if the connection is accepted
*                FAILURE      on error
*
* @end
*
*************************************************************************/
RC_t osapiAccept(uint32 sockFd,  sockaddr_t *saddr, uint32 *s_len,
                     int32 *descriptor)
{
   sockaddr_union_t c_saddr;
  struct sockaddr *os_saddr = (struct sockaddr *)&c_saddr;
   int32 fd;

  if(*s_len > sizeof(c_saddr)) return  FAILURE;
  memcpy(&c_saddr, saddr, *s_len);
  os_saddr->sa_family = ( ushort16)osapiFromL7AF(saddr->sa_family);

  fd = accept(sockFd, os_saddr, s_len);
  if (fd == ERROR)
  {
    return  FAILURE;
  }
  else
  {
    *descriptor = fd;
    return  SUCCESS;
  }
}

/**************************************************************************
*
* @purpose      Prepare to accept connections on socket
*
* @param        listen_sock   @b{(input)}  FD of the listening socket
* @param        listen_queue  @b{(input)}  connection requests that will be queued
*                                          before further requests are refused.
*
* @returns       SUCCESS      on success
*                FAILURE      on error
*
* @end
*
*************************************************************************/
 int32 osapiListen( int32 listen_sock,  int32 listen_queue)
{
   int32 fd;

  fd = listen(listen_sock, listen_queue);
  if (fd == ERROR)
  {
    return fd;
  }
  else
  {
    return  SUCCESS;
  }
}

/**************************************************************************
*
* @purpose      Set the non-blocking mode on a file (socket).
*               The FIONBIO method says only for sockets in tornado include,
*               but we apply it to other file types as well.
*
* @param        fd              file descriptor
*               nbio            if  TRUE, non-blocking, else blocking
*
* @returns       SUCCESS or  FAILURE
*
*
* @end
*
*************************************************************************/

RC_t osapiSocketNonBlockingModeSet(uint32 fd,  BOOL nbio)
{
    int mode = (nbio ==  TRUE)?1:0;
    int rc;

    rc = ioctl(fd, FIONBIO, (void *)&mode);

    return (rc< 0)?  FAILURE: SUCCESS;
}

/**************************************************************************
*
* @purpose      Get the non-blocking mode on a file (socket).
*
* @param        fd                @b{(input)}     file descriptor
*               non_blocking_mode @b{(output)}    if  TRUE - non-blocking, else blocking
*
* @returns      none
*
*
* @end
*
*************************************************************************/
void osapiSocketNonBlockingModeGet(uint32 fd,  BOOL* non_blocking_mode)
{
  int flags;

  flags = fcntl(fd, F_GETFL,  NULL);

  if (flags & O_NONBLOCK)
  {
    *non_blocking_mode =  TRUE;
  }
  else
  {
    *non_blocking_mode =  FALSE;
  }
}

/**************************************************************************
*
* @purpose      create IP (4/6) address from ascii string
*
* @param        family          address faminly
*               str             pointer to ouput string
*               addr            pointer to address
*
* @returns       SUCCESS or  FAILURE
*
*
* @end
*
*************************************************************************/
RC_t osapiInetPton(uint32 family,  char8 *str,  uchar8 *addr)
{
   int rc;

   family = osapiFromL7AF(family);

   if(family == AF_INET6)
   {
     /* This condition checks the ipv6 adress having less than or eqult to 4
        characters in each colon part or not */

     if(osapiIpv6Support(str) ==  FALSE)
     return  FAILURE;
   }

   rc = inet_pton(family,str,addr);
   return (rc <= 0)?  FAILURE: SUCCESS;
}

/**************************************************************************
*
* @purpose  It finds wheather all the string part contains 4 characters or more
*           in between colons in IPV6 address.Since This condition is not handled
*           in interpeak stack.(NOTE: This function checks only four characters
*           condition only.Not any other conditions.)
*
*           Return true if string is contains not more than 4 characters
*           xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx
*
*           Return false if string contains more than 4 characters in between colons
*           xxxx:xxxxxx:xxxxxxx:xxxx:xxxx:xxxx:xxxx:xxxx
*
*           Return false if the number of colons in complete string exceeds a count of 7
*
*           Return false if the string ends with a colon (ABCD::)
*
* @param    s   @b{(input)}ipv6 string format.
*
* @returns   TRUE
* @returns   FALSE
*
* @end
*
*************************************************************************/
 BOOL  osapiIpv6Support( char8 *s)
{

   uint32 index =0, count=0, searchLength = 0, final = 0, start, end, colonCount = 0;
    char8  *ref1, *ref2;
    BOOL   isdotForm = 0;

   ref1 = ref2 = s;

   /* This loop finds upto what position search shold be done.
      to handle the dotted decimal and colon (ipv4 on ipv6) notation */

   while(*ref1 != '\0')
   {
      if(*ref1 == ':')
      {
         final = index;
         colonCount++;
      }

      if(*ref1 == '.')
      {
         isdotForm =  TRUE;
         break;
      }

      if(colonCount > 7)
      {
        return  FALSE;
      }


      ref1++;
      index++;
   }

   if(isdotForm ==  TRUE)
   {
      searchLength = final;
   }
   else
   {
      searchLength = index;
   }

   end = 0 ;
   start= 0;

   while( (count <= searchLength) && (*ref2 != '\0') )
   {
      /* It will handle the single colon case
         if it is single quotation it will move one position forward */
      if(*ref2 == ':')
      {
         end = count;

         if((end-start) > 4)
         return  FALSE;

         start = count + 1;

         /* It will handle the double colon case
            if it is double colon then it should move two positions forward */


         if( *(ref2 + 1) == ':')
         {
              ref2++;
              count++;
              start = count + 1;
         }
      }

      ref2++;
      count++;

      /* it will handle the case like colon and dotted quotation mark
         as weill as final 4 characters.since it wont end with colon */

      if(count == searchLength)
      {
        if((count-start) > 4 )
            return  FALSE;
      }
   }
   return  TRUE;
}

