/**
 * Most basic configuration that loads firmware and performs register access. It then spawns the crsh to let you play
 * with the slice. This crsh is spawned for all examples.
 */
#include "libunixsocket.h"
#include "shell.h"

#include "crintl/shell.h"
#include "credo.h"  // credo sdk api (only 1 needed to import)
#include "sdk.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#define CONN_BUFFER_SIZE 2048

typedef struct {
    int listenfd;
    int listenoutfd;
    int connfd;
    int outfd;
    char socket_path[1024];
    char socket_path2[1030];
    pthread_mutex_t lock;
} ServerState_t;

static ServerState_t server_state = {.lock = PTHREAD_MUTEX_INITIALIZER};

#define SERVER_LOCK()   pthread_mutex_lock(&server_state.lock)
#define SERVER_UNLOCK() pthread_mutex_unlock(&server_state.lock)

static const char* log_level_to_str(CredoLogLevel_t level) {
    switch (level) {
        case CR_LOG_TRACE:
            return "TRACE";
        case CR_LOG_DEBUG:
            return "DEBUG";
        case CR_LOG_INFO:
            return "INFO";
        case CR_LOG_WARN:
            return "WARNING";
        case CR_LOG_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

inline static int recieve_data(int fd, char* dataBuffer, int len) {
    // iterate multiple times in case the packets are broken up
    ssize_t n = recv(fd, dataBuffer, len - 1, MSG_NOSIGNAL);
    if (n <= 0) {
        return -1;
    }
    dataBuffer[n] = '\0';
    return (int)n;
}
inline static int send_data(int fd, const char* data) {
    ssize_t n = send(fd, data, strlen(data), MSG_NOSIGNAL);
    if (n <= 0 || (size_t)n < strlen(data)) {
        return -1;
    }
    return (int)n;
}

// customize the logger to store it in log capture
void shell_log(void* slice_context, void* user_data, CredoLogLevel_t level, const char* scope, const char* message) {
    (void)slice_context;
    (void)user_data;
    // store log output

    if (server_state.outfd == 0) {
        return;
    }
    char buffer[256];
    const char separator = strchr(message, '\n') != NULL ? '\n' : ' ';
    const char* level_str = log_level_to_str(level);
    snprintf(buffer, sizeof(buffer), "[%7s %8s]%c", level_str, scope, separator);
    send_data(server_state.outfd, buffer);
    send_data(server_state.outfd, message);
    send_data(server_state.outfd, "\n");
}

void shell_writer(void* ud, CredoShellStream_t stream, const char* message) {
    (void)(ud);
    (void)(stream);
    send_data(server_state.outfd, message);
}

CredoError_t socket_readline(const char* prompt, char input[CR_CMDLINE_SIZE]) {
    if (server_state.outfd != 0) {
        shutdown(server_state.outfd, SHUT_RDWR);
        close(server_state.outfd);
        server_state.outfd = 0;
    }
    char databuffer[CR_CMDLINE_SIZE * 2];
    char prompt_command[64] = {0};
    int n;
    snprintf(prompt_command, sizeof(prompt_command) - 1, "@prompt:%s", prompt);
    n = send_data(server_state.connfd, prompt_command);
    if (n < 0) goto send_quit;

    while (true) {
        n = recieve_data(server_state.connfd, databuffer, sizeof(databuffer));
        if (n < 0) goto send_quit;
        if (strncmp(databuffer, "@auto:", 6) != 0) {
            break;
        }
        n = send_data(server_state.connfd, "ack");
        if (n < 0) goto send_quit;

        char posbuf[CR_CMDLINE_SIZE];
        n = recieve_data(server_state.connfd, posbuf, CR_CMDLINE_SIZE);
        if (n < 0) goto send_quit;
        size_t pos = strtoul(posbuf, NULL, 10);
        cr_lua_get_completions(databuffer + 6, NULL, pos);
        n = send_data(server_state.connfd, "@auto:done");
        if (n < 0) goto send_quit;
    }
    if (strncmp(databuffer, "@input:", 7) != 0) {  // invalid packet sent
        goto send_quit;
    }
    snprintf(input, CR_CMDLINE_SIZE, "%s", databuffer + 7);
    input[CR_CMDLINE_SIZE - 1] = '\0';

    int outfd = accept_unix_stream_socket(server_state.listenoutfd, 0);
    if (outfd < 0) {
        fprintf(stderr, "error making output socket connection?");
        goto send_quit;
    }
    server_state.outfd = outfd;

    return CR_OK;
send_quit:  // here we are sending the quit command to the shell, its a bit funky
    return CR_FAIL;
}

static void socket_send_completion(const char* text, void* lc, size_t pos) {
    (void)(lc);
    char buff[CR_CMDLINE_SIZE];
    int n = send_data(server_state.connfd, text);
    if (n < 0) return;
    n = recieve_data(server_state.connfd, buff, sizeof(buff));
    if (n < 0) return;
    snprintf(buff, sizeof(buff), "%lu", pos);
    n = send_data(server_state.connfd, buff);
    if (n < 0) return;
    n = recieve_data(server_state.connfd, buff, sizeof(buff));
    if (n < 0) return;
}

CredoError_t run_server() {
    char recvBuff[CONN_BUFFER_SIZE];
    remove(server_state.socket_path);
    remove(server_state.socket_path2);
    int listenfd = create_unix_server_socket(server_state.socket_path, LIBSOCKET_STREAM, 0);
    if (listenfd < 0) {
        return CR_FAIL;
    }
    server_state.listenfd = listenfd;

    int listenoutfd = create_unix_server_socket(server_state.socket_path2, LIBSOCKET_STREAM, 0);
    if (listenoutfd < 0) {
        destroy_unix_socket(server_state.listenfd);
        return CR_FAIL;
    }
    server_state.listenoutfd = listenoutfd;

    while (1) {
        int connfd = accept_unix_stream_socket(listenfd, 0);
        if (connfd < 0) continue;

        int n = recieve_data(connfd, recvBuff, CONN_BUFFER_SIZE);
        if (n <= 0) continue;
        server_state.connfd = connfd;
        // kill the server
        if (strcmp(recvBuff, "@kill") == 0) {
            destroy_unix_socket(connfd);
            goto full_exit;
        } else if (strcmp(recvBuff, "@start") == 0) {
            pid_t server_pid = getpid();

            snprintf(recvBuff, sizeof(recvBuff), "%i", server_pid);
            n = send_data(connfd, recvBuff);
            if (n <= 0) goto cleanup_conn;
            recieve_data(connfd, recvBuff, sizeof(recvBuff));  // ack
            if (n <= 0) goto cleanup_conn;

            cr_shell_set_readline(socket_readline);
            cr_shell_set_logger(shell_log);
            cr_shell_set_writer(shell_writer, NULL);
            cr_lua_set_completion_adder(socket_send_completion);
            crsh.in_server = true;
            cr_shell_spawn(NULL, 0);
            crsh.in_server = false;
            cr_lua_set_completion_adder(NULL);
            cr_shell_set_readline(NULL);
            cr_shell_set_logger(NULL);
            cr_shell_set_writer(NULL, NULL);
        } else {
            fprintf(stderr, "Invalid command %s", recvBuff);
        }
        send_data(connfd, "@quit");
    cleanup_conn:
        shutdown_unix_stream_socket(connfd, LIBSOCKET_READ | LIBSOCKET_WRITE);
        destroy_unix_socket(connfd);
        if (server_state.outfd != 0) {  // exiting, make sure output stream is shut
            shutdown_unix_stream_socket(server_state.outfd, LIBSOCKET_READ | LIBSOCKET_WRITE);
            destroy_unix_socket(server_state.outfd);
            server_state.outfd = 0;
        }
    }
full_exit:
    shutdown_unix_stream_socket(listenfd, LIBSOCKET_READ | LIBSOCKET_WRITE);
    destroy_unix_socket(listenfd);
    destroy_unix_socket(listenoutfd);

    return CR_OK;
}

CredoError_t cr_shell_spawn_server2(const char* socket_path) {
    SERVER_LOCK();
    if (socket_path == NULL) {
        socket_path = "/tmp/credo-server.sock";
    }
    snprintf(server_state.socket_path, sizeof(server_state.socket_path), "%s", socket_path);
    snprintf(server_state.socket_path2, sizeof(server_state.socket_path2), "%s-out", socket_path);
    CredoError_t err = run_server();
    cr_shell_set_logger(NULL);
    cr_shell_set_readline(NULL);
    SERVER_UNLOCK();
    return err;
}

CredoError_t cr_shell_spawn_server() {
    return cr_shell_spawn_server2(NULL);
}
