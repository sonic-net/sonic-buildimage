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


#ifndef OSAPISUPPORT_HEADER
#define OSAPISUPPORT_HEADER

#include <stddef.h>
#include "commdefs.h"
#include "datatypes.h"
#include "osapi_sockdefs.h"
#include "l3_addrdefs.h"

#include <sys/select.h>

/*
 ****************************************************************************
 *                         setsockopt/getsockopt
 ****************************************************************************
 */    

#if defined (PC_LINUX_HOST) && !defined (NDA_RTA)
#define NDA_RTA(r)  ((struct rtattr*)(((char*)(r)) + NLMSG_ALIGN(sizeof(struct ndmsg))))
#endif

/**************************************************************************
*
* @purpose  Create a socket
*
* @param    domain      @b{(input)} address family (for example, AF_INET)
* @param    type        @b{(input)} SOCK_STREAM, SOCK_DGRAM, or SOCK_RAW
* @param    protocol    @b{(input)} protocol to be used
* @param    descriptor  @b{(output)}ptr to socket descriptor
*
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
extern uint32 osapiSocketCreate(uint32 domain,
                                   uint32 type,
                                   uint32 protocol,
                                    int32 *descriptor);

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
extern RC_t osapiSocketBind(uint32 socketDescriptor,
                                sockaddr_t *saddr,uint32 s_len);

/**************************************************************************
*
* @purpose      Initiate a connection on a socket
*
* @param        sockFd          @b{(input)} socket descriptor
* @param        servAddress     @b{(input)} pointer to connector's address info.
* @param        addrLength      @b{(input)} length of connector's address info.
*
* @returns       SUCCESS      if the connection or binding succeeds
*                FAILURE      on error
*
* @end
*
*************************************************************************/

extern RC_t osapiConnect (uint32 sockFd,  sockaddr_t *servAddress, uint32 addrLength);

/**************************************************************************
*
* @purpose      Accept a connection on a socket
*
* @param        sockFd          @b{(input)} socket fd
* @param        servAddress     @b{(input)} pointer to connector's address info.
* @param        addrLength      @b{(input)} length of connector's address info.
* @param        descriptor      @b{(output)} socket descriptor 
*
* @returns       SUCCESS      if the connection is accepted
*                FAILURE      on error
*
* @end
*
*************************************************************************/

extern RC_t osapiAccept (uint32 sockFd,  sockaddr_t *servAddress,
                            uint32 *addrLength,  int32 *descriptor);

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
extern  int32 osapiListen( int32 listen_sock,  int32 listen_queue);

/**************************************************************************
*
* @purpose      Wait on a set of file descriptors
*
* @param    width             @b{(input)} number of bits to examine from 0
* @param    rFds              @b{(input)} read fds
* @param    wFds              @b{(input)} write fds
* @param    eFds              @b{(input)} exception fds
* @param    timeOutSec    @b{(input)} max time in sec to wait,
*                                             NULL = forever
* @param    timeOutMicroSec   @b{(input)} time in microsec
*
* @returns      The number of file descriptors with activity, 0 if timed out
*               or ERROR
*
* @end
*
*************************************************************************/
extern  int32 osapiSelect( uint32 width,
                              fd_set*   rFds,
                              fd_set*   wFds,
                              fd_set*   eFds,
                               int32 timeOutSec,
                               int32 timeOutMicroSec);

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
extern RC_t osapiSocketRecvfrom(uint32 socketDescriptor,
                                      uchar8 *buf,
                                     uint32 length,
                                     uint32 flag,
                                      sockaddr_t *from,
                                     uint32   *from_len,
                                     uint32 *bytesRcvd);

/**************************************************************************
*
* @purpose  Receive data from a socket
*
* @param      fd                @b{(input)} socket to receive from
* @param      ptr               @b{(input)} pointer to data buffer
* @param      nbytes            @b{(input/output)} length of buffer
* @param      flag              @b{(input)} flags to underlying protocols
* @param      from              @b{(output)} source address
* @param      from_len          @b{(output)} source address length
* @param      to                @b{(output)} source address
* @param      to_len            @b{(output)} source address length
* @param      ifIndex           @b{(output)} intIfNum
* @param      hops              @b{(output)} hop count
* @param      rtrAlert          @b{(output)} did packet have Router-Alert
*                               option set?
*
* @returns   SUCCESS
* @returns   ERROR
*
* @end
*
*************************************************************************/
RC_t osapiPktInfoRecv(int fd , void *ptr, int *nbytes,int *flagsp, sockaddr_t *from,
                   int *from_len,  sockaddr_t *to, int *to_len, uint32 *ifIndex,uint32 *hops, BOOL *rtrAlert);

/**************************************************************************
*
* @purpose  receive with packet info with incoming stack if index
*
* @param    fd                @b{(input)} socket descriptor 
* @param    ptr               @b{(input)} data destination
* @param    nbytes            @b{(in/out)} rx data length
* @param    from              @b{(in/out)} sender address
* @param    from_len          @b{(in/out)} sender address legnth
* @param    to                @b{(in/out)} destination address
* @param    to_len            @b{(in/out)} destination address legnth
* @param    stackIfIndex      @b{(in/out)} incoming stack interface index
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
RC_t osapiPktIntfRecv(int fd , void *ptr, int *nbytes,int *flagsp, sockaddr_t *from,
                         int *from_len,  sockaddr_t *to, int *to_len, uint32 *stackIfIndex);

/**************************************************************************
*
* @purpose  send with packet info
*
* @param    fd                @b{(input)} socket descriptor 
* @param    buf               @b{(input)} data destination
* @param    bufLen            @b{(input)} tx data length
* @param    flags             @b{(input)} tx flags
* @param    from              @b{(in/out)} sender address
* @param    from_len          @b{(in/out)} sender address legnth
* @param    to                @b{(in/out)} destination address
* @param    to_len            @b{(in/out)} destination address legnth
* @param    intIfNum          @b{(in/out)} transmitting interface number
* @param    hops              @b{(in/out)} received hop count
* @param    bytesSent         @b{(in/out)} tx bytes transmitted
* @param    trafficClass      @b{(input)} Traffic Class in the tx'ed header
* @param    hopLimit          @b{(input)} Hop Limit in the tx'ed header
* @param    nextHdr           @b{(input)} Next Header value
* @param    tcFlag            @b{(input)} If the traffic-class is supplied
* @param    hlFlag            @b{(input)} If the hop-limit is supplied
* @param    nhFlag            @b{(input)} If the next-header is supplied
* @param    rtrAlert          @b{(input)} Send with Router Alert hdr option
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/

RC_t osapiPktInfoSend(uint32 fd, uchar8 *buf,uint32 bufLen,uint32 flags,
                          sockaddr_t *from, uint32 from_len,
                          sockaddr_t *to, uint32 to_len, uint32 intIfNum,
                         uint32 *bytesSent, uint32 trafficClass,
                         uint32 hopLimit, uint32 nextHdr,
                          BOOL tcFlag,  BOOL hlFlag,  BOOL nhFlag,  BOOL rtrAlert);

/**************************************************************************
*
* @purpose  Set TCP MD5 socket option. 
*
* @param    s               @b{(input)} socket descriptor
* @param    remoteAddress   @b{(input)} IP address of remote end of TCP connection
*                                       (host byte order if IPv4)
* @param    keyLen          @b{(input)} password length in bytes
* @param    password        @b{(input)} MD5 password
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
extern RC_t osapiTcpMd5PasswordSet(uint32 s,  l3_inet_addr_t *remoteAddress,
                                      uint32 keyLen,  uchar8 *password);

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
extern RC_t osapiSetsockopt(uint32 targetSocket,
                               uint32 level,
                               uint32 optname,
                                uchar8 *optval,
                               uint32 optlen);

/**************************************************************************
*
* @purpose  Get socket options
*
* @param    targetSocket @b{(input)} socket to receive from
* @param    level        @b{(input)} protocol level of option
* @param    optname      @b{(input)} option name
* @param    optval       @b{(input)} pointer to option value
* @param    optlen       @b{(input/output)} option length
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
extern RC_t osapiGetsockopt(uint32 targetSocket,
                               uint32 level,
                               uint32 optname,
                                uchar8 *optval,
                               uint32 *optlen);

/**************************************************************************
*
* @purpose  Set Broadcast Address on the interface
*
* @param    fd   @b{(input)} socket descriptor
* @param    intf @b{(input)} Ptr to interface name
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
extern RC_t osapiSetBroadAddr( int32 fd,
                                 const  uchar8 *intf);

/**************************************************************************
*
* @purpose  Shutdown read and/or write on a socket
*
* @param    fd   @b{(input)} socket descriptor
* @param    how  @b{(input)} read/write flags
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
extern RC_t osapiShutdown(uint32 fd, uint32 how);

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
extern RC_t osapiSocketSendto(uint32 s,
                                    uchar8 *buf,
                                   uint32 bufLen,
                                   uint32 flags,
                                    sockaddr_t *to,
                                   uint32   to_len,
                                   uint32 *bytesSent);

/**************************************************************************
*
* @purpose  Send a message to a socket on a specific interface
*
* @param    s         @b{(input)} socket to send  
* @param    buf       @b{(input)} pointer to data buffer
* @param    bufLen    @b{(input)} length of buffer
* @param    flags     @b{(input)} flags to underlying protocols
* @param    family    @b{(input)} Address Family  
* @param  port      @b{(input)} Port 
* @param  ipAddr    @b{(input)} IP Address
* @param  intIfNum  @b{(input)} internal interface
* @param  bytesSent @b{(output)} Ptr to number of bytes sent
*
* @returns   SUCCESS
* @returns   ERROR
*
* @notes    Needed due to link local addresses where unicast addr does
*           not identify interface.
* @end
*
*************************************************************************/

RC_t osapiSocketIntfSendto(uint32 s, uchar8 *buf,uint32 bufLen,uint32 flags,
                           sockaddr_t *to, uint32 to_len, uint32 intIfNum,
                          uint32 *bytesSent);

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
extern void osapiSocketClose(uint32 fd);

/**************************************************************************
*
* @purpose  Convert a dot notation Internet address to a long integer
*
* @param    address @b{(input)} IP address in dotted notation
*
* @returns  IP address in long integer
*
* @end
*
*************************************************************************/
extern uint32 osapiInet_addr( uchar8 *address);

/**************************************************************************
*
* @purpose  Convert a network address to dotted decimal notation
*
* @param    inetAddress @b{(input)} network address
*
* @returns  char pointer to the string version of an Internet address
*
* @end
*
*************************************************************************/
extern  char8* osapiInet_ntoa(uint32 inetAddress);

/**************************************************************************
*
* @purpose  Convert host byte order to network byte order in 64-bit integer
*
* @param    hostlong @b{(input)} host byte order
*
* @returns  network byte order
*
* @end
*
*************************************************************************/
extern  uint64  osapiHtonll( uint64 hostlong);

/**************************************************************************
*
* @purpose  Convert network byte order to host byte order in 64-bit integer
*
* @param    netlong @b{(input)} network byte order
*
* @returns  host byte order
*
* @end
*
*************************************************************************/
extern  uint64  osapiNtohll( uint64 netlong);

/**************************************************************************
*
* @purpose  Convert host byte order to network byte order in long integer
*
* @param    hostlong @b{(input)} host byte order
*
* @returns  network byte order
*
* @end
*
*************************************************************************/
extern uint32  osapiHtonl(uint32 hostlong);


/**************************************************************************
*
* @purpose  Convert host byte order to network byte order in short integer
*
* @param    hostshort @b{(input)} host byte order
*
* @returns  network byte order
*
* @end
*
*************************************************************************/
extern  ushort16  osapiHtons( ushort16 hostshort);


/**************************************************************************
*
* @purpose  Convert network byte order to host byte order in long integer
*
* @param    netlong @b{(input)} network byte order
*
* @returns  host byte order
*
* @end
*
*************************************************************************/
extern uint32  osapiNtohl(uint32 netlong);


/**************************************************************************
*
* @purpose  Convert network byte order to host byte order in short integer
*
* @param    netshort @b{(input)} network byte order
*
* @returns  host byte order
*
* @end
*
*************************************************************************/
extern  ushort16  osapiNtohs( ushort16 netshort);

/**************************************************************************
*
* @purpose  to convert host byte order to little endian byte order in long integer
*
* @param    hostlong  @b{(input)} host byte order
*
* @returns  little endian byte order
*
* @end
*
*************************************************************************/
extern uint32  osapiHtole32(uint32 hostlong);

/**************************************************************************
*
* @purpose  to convert host byte order to little endian byte order in short integer
*
* @param    hostshort  @b{(input)} host byte order
*
* @returns  little endian byte order
*
* @end
*
*************************************************************************/
extern  ushort16  osapiHtole16( ushort16 hostshort);

/**************************************************************************
*
* @purpose  to convert little endian byte order to host byte order in long integer
*
* @param    netlong  @b{(input)} host byte order
*
* @returns  little endian byte order
*
* @end
*
*************************************************************************/
extern uint32  osapiLe32toh(uint32 netlong);

/**************************************************************************
*
* @purpose  to convert little endian byte order to host byte order in short integer
*
* @param    nettshort  @b{(input)} host byte order
*
* @returns  little endian byte order
*
* @end
*
*************************************************************************/
extern  ushort16  osapiLe16toh( ushort16 netshort);

/**************************************************************************
* @purpose  Read bytes from a file
*
* @param    fd       @b{(input)} file descriptor
* @param    buffer   @b{(input)} ptr to buffer to read data
* @param    maxbytes @b{(input)} number of bytes to read
*
* @returns  The number of bytes read from file or -1 on error.
*
* @end
*
*************************************************************************/
extern  int32 osapiRead(uint32 fd,
                            char8* buffer,
                           size_t maxbytes);


/**************************************************************************
* @purpose  Write bytes to a file
*
* @param    fd        @b{(input)} file descriptor
* @param    buffer    @b{(input)} ptr to buffer to write data
* @param    maxbytes  @b{(input)} number of bytes to write to file
*
* @returns  The number of bytes written to file or -1 on error.
*
* @end
*
*************************************************************************/
extern  int32 osapiWrite(uint32 fd,
                             char8* buffer,
                            size_t maxbytes);
/**************************************************************************
* @purpose  Get Operating System Error Number
*
* @returns  ERRNO set by the system.
*
* @end
*
*************************************************************************/
extern uint32 osapiErrnoGet(void);

/**************************************************************************
* @purpose  Get Operating System Error String
*
* @returns  string corresponding to ERRNO set by the system.
*
* @end
*
*************************************************************************/
extern  char8 *osapiErrStrGet(void);

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

RC_t osapiSocketNonBlockingModeSet(uint32 fd,  BOOL nbio);

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
void osapiSocketNonBlockingModeGet(uint32 fd,  BOOL* non_blocking_mode);

/**************************************************************************
*
* @purpose      Get the file descriptor for task standard input/output/error.
*               Retun  FAILURE if stdFd is not 0, 1, or 2,
*               or the routine is called at interrupt level. 
*               
* @params       taskId          ID of desired task (0 = self)
*               stdFd           std input (0), output (1), or error (2)
*               newFd           the underlying file descriptor. 
*
* @returns       SUCCESS or  FAILURE
*
*
* @end
*
*************************************************************************/

RC_t osapiTaskStdGet(uint32 taskId, uint32 stdFd,  int32 *fd);

/**************************************************************************
*
* @purpose      Set the file descriptor for task standard input/output/error.
*               If stdFd is not 0, 1, or 2, this routine has no effect. 
*
* @params       taskId          task whose std fd is to be set (0 = self)
*               stdFd           std input (0), output (1), or error (2)
*               newFd           new underlying file descriptor
*
* @returns       SUCCESS or  FAILURE
*
*
* @end
*
*************************************************************************/

RC_t osapiTaskStdSet(uint32 taskId, uint32 stdFd, uint32 newFd);

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
 uchar8 *osapiInetNtop(uint32 family,  uchar8 *addr,  uchar8 *str, uint32 len);

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
RC_t osapiInetPton(uint32 family,  char8 *str,  uchar8 *addr);

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
* @param    s   @b{(input)}ipv6 string format.
*
* @returns   TRUE
* @returns   FALSE
*
* @end
*
*************************************************************************/
 BOOL  osapiIpv6Support( char8 *s);

/**************************************************************************
*
* @purpose  Get peer name
*
* @param    socketDescriptor  @b{(input)} socket descriptor
* @param    saddr             @b{(output)} remote address
* @param    s_len             @b{(output)} remote address length
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*
*************************************************************************/
RC_t osapiGetpeername(uint32 socketDescriptor,
                                sockaddr_t *saddr,uint32 *s_len);
#endif/* OSAPISUPPORT_HEADER*/
