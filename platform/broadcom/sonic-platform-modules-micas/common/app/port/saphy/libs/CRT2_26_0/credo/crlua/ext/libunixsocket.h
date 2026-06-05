#ifndef LIBSOCKET_LIBUNIXSOCKET_H_61CF2FC7034E4AD982DA08144D578572
#define LIBSOCKET_LIBUNIXSOCKET_H_61CF2FC7034E4AD982DA08144D578572
/**
 * @file libunixsocket.h
 *
 * Contains all libunixsocket C functions.
 *
        The committers of the libsocket project, all rights reserved
        (c) 2012, dermesser <lbo@spheniscida.de>

        Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

                1. Redistributions of source code must retain the above
 copyright notice, this list of conditions and the following disclaimer.
                2. Redistributions in binary form must reproduce the above
 copyright notice, this list of conditions and the following disclaimer in the
 documentation and/or other materials provided with the distribution.

        THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/* Headers (e.g. for flags) */
#include <sys/socket.h>
#include <sys/types.h>

/* Macro definitions */

#define LIBSOCKET_STREAM 1
#define LIBSOCKET_DGRAM  2

#define LIBSOCKET_READ  1
#define LIBSOCKET_WRITE 2

#ifdef __cplusplus
#ifdef MIXED
extern "C" {
#endif
#endif

extern int create_unix_stream_socket(const char* path, int flags);
extern int create_unix_dgram_socket(const char* bind_path, int flags);
extern int connect_unix_dgram_socket(int sfd, const char* path);
extern int destroy_unix_socket(int sfd);
extern int shutdown_unix_stream_socket(int sfd, int method);
extern int create_unix_server_socket(const char* path, int socktype, int flags);
extern int accept_unix_stream_socket(int sfd, int flags);
extern ssize_t recvfrom_unix_dgram_socket(int sfd, void* buf, size_t size, char* from, size_t from_size,
                                          int recvfrom_flags);
extern ssize_t sendto_unix_dgram_socket(int sfd, const void* buf, size_t size, const char* path, int sendto_flags);

#ifdef __cplusplus
#ifdef MIXED
}
#endif
#endif

#endif
