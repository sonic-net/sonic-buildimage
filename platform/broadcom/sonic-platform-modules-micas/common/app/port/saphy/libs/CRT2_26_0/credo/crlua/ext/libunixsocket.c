#ifndef _GNU_SOURCE
#define _GNU_SOURCE  // accept4()
#endif

// #include <conf.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>  // UNIX domain sockets
#include <unistd.h>  // read()/write()

/**
 * @addtogroup libunixsocket
 * @{
 * @file libunixsocket.c
 *
 * Contains all C functions to handle UNIX domain sockets.

   The committers of the libsocket project, all rights reserved
   (c) 2012, dermesser <lbo@spheniscida.de>

   Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice,
 this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS “AS IS” AND ANY
 EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

/*
 * Structure of the functions defined here:
 *
 * <Declarations>
 * <Checks on passed arguments>
 * <actual code>
 *
 */

// # define VERBOSE

// Macro definitions

#define LIBSOCKET_BACKLOG 128

#define LIBSOCKET_STREAM 1
#define LIBSOCKET_DGRAM  2

#define LIBSOCKET_READ  1
#define LIBSOCKET_WRITE 2

#define debug_write(str)                \
    {                                   \
        int verbose_errno_save = errno; \
        write(2, str, strlen(str));     \
        errno = verbose_errno_save;     \
    }

/**
 * @brief Checks return value for error.
 *
 * Every value returned by a syscall is passed to this function. It returns 0
 * if the return value is ok or -1 if there was an error.
 * If the macro `VERBOSE` is defined, an appropriate message is printed to
 * STDERR.
 *
 * @param return_value A return value from a syscall.
 *
 * @retval 0 The syscall was successful.
 * @retval -1 There was an error.
 */
static inline signed int check_error(int return_value) {
#ifdef VERBOSE
    const char* errbuf;
#endif
    if (return_value < 0) {
#ifdef VERBOSE
        errbuf = strerror(errno);
        debug_write(errbuf);
#endif
        return -1;
    }

    return 0;
}

/**
 * @brief Create and connect a new UNIX STREAM socket.
 *
 * Creates and connects a new STREAM socket with the socket given in `path`.
 *
 * @retval >0 Success; return value is a socket file descriptor
 * @retval <0 Error.
 */
int create_unix_stream_socket(const char* path, int flags) {
    struct sockaddr_un saddr;
    int sfd;

    if (path == NULL) return -1;

    if (-1 == check_error(sfd = socket(AF_UNIX, SOCK_STREAM | flags, 0))) return -1;

    memset(&saddr, 0, sizeof(struct sockaddr_un));

    if (strlen(path) > (sizeof(saddr.sun_path) - 1)) {
#ifdef VERBOSE
        debug_write(
            "create_unix_stream_socket: UNIX socket destination path too "
            "long\n");
#endif
        return -1;
    }

    saddr.sun_family = AF_UNIX;
    snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);

    if (-1 == check_error(connect(sfd, (struct sockaddr*)&saddr, sizeof(saddr.sun_family) + strlen(saddr.sun_path)))) {
        close(sfd);
        return -1;
    }

    return sfd;
}

/**
 * @brief Create a UNIX DGRAM socket
 *
 * @param bind_path If not `NULL`, bind to `bind_path`.
 * @param flags Flags to pass to `socket(2)` (varies from OS to OS; look in the
 * man pages)
 *
 * @retval >0 Success. Value is socket.
 * @retval <0 Error.
 */
int create_unix_dgram_socket(const char* bind_path, int flags) {
    int sfd, retval;
    struct sockaddr_un saddr;

    if (-1 == check_error(sfd = socket(AF_UNIX, SOCK_DGRAM | flags, 0))) return -1;

    memset(&saddr, 0, sizeof(struct sockaddr_un));

    if (bind_path != NULL) {
        if ((retval = unlink(bind_path)) == -1 && errno != ENOENT)  // If there's another error than "doesn't exist"
        {
            check_error(retval);
            return -1;
        }

        if (strlen(bind_path) > (sizeof(saddr.sun_path) - 1)) {
#ifdef VERBOSE
            debug_write("create_unix_dgram_socket: UNIX socket path too long\n");
#endif
            return -1;
        }

        saddr.sun_family = AF_UNIX;
        snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", bind_path);

        bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr.sun_family) + strlen(saddr.sun_path));
    }

    return sfd;
}

/**
 * @brief Connect a datagram socket
 *
 * Connects a datagram socket to the specified socket so the
 * stream i/o operations may be used (`read(2)/write(2)`)
 *
 * @param sfd The socket
 * @param path The path to connect to
 *
 * @retval 0 Fine
 * @retval <0 Not fine
 */
int connect_unix_dgram_socket(int sfd, const char* path) {
    struct sockaddr_un new_addr;
    struct sockaddr deconnect;

    if (sfd < 0) return -1;

    if (path == NULL) {
        memset(&deconnect, 0, sizeof(struct sockaddr));

        deconnect.sa_family = AF_UNSPEC;

        if (check_error(connect(sfd, &deconnect, sizeof(struct sockaddr)))) return -1;

        return 0;
    }

    memset(&new_addr, 0, sizeof(struct sockaddr_un));

    new_addr.sun_family = AF_UNIX;

    if (strlen(path) > sizeof(new_addr.sun_path) - 1) {
#ifdef VERBOSE
        debug_write("connect_unix_dgram_socket: Path too long\n");
#endif
        return -1;
    }

    snprintf(new_addr.sun_path, sizeof(new_addr.sun_path), "%s", path);

    if (-1 ==
        check_error(connect(sfd, (struct sockaddr*)&new_addr, sizeof(new_addr.sun_family) + strlen(new_addr.sun_path))))
        return -1;

    return 0;
}

/**
 * @brief Close a socket
 *
 * Actually, it's the same as `close(2)`.
 *
 * @param sfd The socket to close.
 *
 * @retval 0 Socket could be closed
 * @retval <0 Socket was already closed.
 */
int destroy_unix_socket(int sfd) {
    if (sfd < 0) return -1;

    if (-1 == check_error(close(sfd))) return -1;

    return 0;
}

/**
 * @brief Shut a socket down
 *
 * Shut a socket down for reading or writing. If shut down for
 * reading, the program can't read anymore. If shut down for writing
 * no data can be sent anymore and the peer receives EOF.
 *
 * @param sfd The socket
 * @param method `LIBSOCKET_READ`, `LIBSOCKET_WRITE` or
 * `LIBSOCKET_READ|LIBSOCKET_WRITE`
 *
 * @retval 0 Success
 * @retval <0 Error
 */
int shutdown_unix_stream_socket(int sfd, int method) {
    if (sfd < 0) return -1;

    if ((method != LIBSOCKET_READ) && (method != LIBSOCKET_WRITE) && (method != (LIBSOCKET_READ | LIBSOCKET_WRITE)))
        return -1;

    if (method & LIBSOCKET_READ)  // READ is set (0001 && 0001 => 0001)
    {
        if (-1 == check_error(shutdown(sfd, SHUT_RD))) return -1;
    }

    if (method & LIBSOCKET_WRITE)  // WRITE is set (0010 && 0010 => 0010)
    {
        if (-1 == check_error(shutdown(sfd, SHUT_WR))) return -1;
    }

    return 0;
}

/**
 * @brief Create a passive UNIX socket
 *
 * Creating a DGRAM server socket is the same as creating one
 * using `create_unix_dgram_socket()` but with latter you may
 * also not bind to anywhere.
 *
 * @param path Path to bind the socket to
 * @param socktype `LIBSOCKET_STREAM` or `LIBSOCKET_DGRAM`
 * @param flags Flags for `socket(2)`.
 *
 * @retval >0 Success; returned value is a file descriptor for the socket
 * @retval <0 An error occurred.
 */
int create_unix_server_socket(const char* path, int socktype, int flags) {
    struct sockaddr_un saddr;
    int sfd, type, retval;

    if (path == NULL) return -1;

    if (strlen(path) > (sizeof(saddr.sun_path) - 1)) {
#ifdef VERBOSE
        debug_write("create_unix_server_socket: Path too long\n");
#endif
        return -1;
    }

    switch (socktype) {
        case LIBSOCKET_STREAM:
            type = SOCK_STREAM;
            break;
        case LIBSOCKET_DGRAM:
            type = SOCK_DGRAM;
            break;
        default:
            return -1;
    }

    if (-1 == check_error(sfd = socket(AF_UNIX, type | flags, 0))) return -1;

    if ((retval = unlink(path)) == -1 && errno != ENOENT)  // If there's another error than "doesn't exist"
    {
        check_error(retval);
        return -1;
    }

    memset(&saddr, 0, sizeof(struct sockaddr_un));

    saddr.sun_family = AF_UNIX;

    snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);

    if (-1 == check_error(bind(sfd, (struct sockaddr*)&saddr, sizeof(saddr.sun_family) + strlen(saddr.sun_path))))
        return -1;

    if (type == SOCK_STREAM) {
        if (-1 == check_error(listen(sfd, LIBSOCKET_BACKLOG))) return -1;
    }

    return sfd;
}

/**
 * @brief Accept connections on a passive UNIX socket
 *
 * @param sfd The server socket
 * @param flags Flags for `accept4(3)` (therefore useless on any other system
 * than Linux)
 *
 * @retval >0 Return value is a socket connected to the client
 * @retval <0 Error at `accept[4]()`
 */
int accept_unix_stream_socket(int sfd, int flags) {
    (void)flags;
    int cfd;

    if (sfd < 0) return -1;
#if LIBSOCKET_LINUX
    if (-1 == check_error(cfd = accept4(sfd, 0, 0, flags))) return -1;
#else
    if (-1 == check_error(cfd = accept(sfd, 0, 0))) return -1;
#endif
    return cfd;
}

/**
 * @brief Receive datagram from another UNIX socket
 *
 * @param sfd The socket descriptor
 * @param buf The buffer to which the data is written
 * @param size its size
 * @param from Place where the path of the sending socket is placed to
 * @param from_size its size
 * @param recvfrom_flags Flags passed to `recvfrom(2)`
 *
 * @retval n *n* bytes were received
 * @retval <0 Error at `recvfrom(2)`
 */
ssize_t recvfrom_unix_dgram_socket(int sfd, void* buf, size_t size, char* from, size_t from_size, int recvfrom_flags) {
    int bytes;
    socklen_t socksize = sizeof(struct sockaddr_un);
    struct sockaddr_un saddr;

    memset(buf, 0, size);
    memset(from, 0, from_size);

    if (-1 == check_error(bytes = recvfrom(sfd, buf, size, recvfrom_flags, (struct sockaddr*)&saddr, &socksize)))
        return -1;

    if (from != NULL && from_size > 0) snprintf(from, from_size, "%s", saddr.sun_path);

    return bytes;
}

/**
 * @brief Send datagram to socket
 *
 * @param sfd Socket
 * @param buf Data to be sent
 * @param size The length of the buffer `buf`
 * @param path Destination socket
 * @param sendto_flags Flags passed to `sendto(2)`
 *
 * @retval n *n* bytes were sent
 * @retval <0 Error at `sendto(2)`.
 */
ssize_t sendto_unix_dgram_socket(int sfd, const void* buf, size_t size, const char* path, int sendto_flags) {
    int bytes;
    struct sockaddr_un saddr;

    if (strlen(path) > sizeof(saddr.sun_path) - 1) {
#ifdef VERBOSE
        debug_write(
            "sendto_unix_dgram_socket: UNIX destination socket path too "
            "long\n");
#endif
        return -1;
    }

    memset(&saddr, 0, sizeof(struct sockaddr_un));

    saddr.sun_family = AF_UNIX;
    snprintf(saddr.sun_path, sizeof(saddr.sun_path), "%s", path);

    if (-1 ==
        check_error(bytes = sendto(sfd, buf, size, sendto_flags, (struct sockaddr*)&saddr, sizeof(struct sockaddr_un))))
        return -1;

    return bytes;
}

/**
 * @}
 */

#undef debug_write
