#define _GNU_SOURCE
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <hiredis/hiredis.h>

static unsigned int socket_checks;
static unsigned int socket_failures;

/* Check each new descriptor in the parent and in an executed child. */
int socket(int domain, int type, int protocol) {
    int (*real_socket)(int, int, int) = dlsym(RTLD_NEXT, "socket");
    int fd = real_socket(domain, type, protocol);
    if (fd < 0)
        return fd;

    socket_checks++;
    pid_t child = fork();
    if (child == 0) {
        char number[32];
        snprintf(number, sizeof(number), "%d", fd);
        execl("/proc/self/exe", "close-on-exec", "--child", number, NULL);
        _exit(2);
    }
    int status;
    int flags = fcntl(fd, F_GETFD);
    if (child < 0 || waitpid(child, &status, 0) != child ||
        !WIFEXITED(status) || WEXITSTATUS(status) != 0 ||
        flags < 0 || !(flags & FD_CLOEXEC))
        socket_failures++;
    return fd;
}

static void ping(redisContext *context) {
    if (!context || context->err) {
        fprintf(stderr, "Redis connection failed\n");
        exit(1);
    }
    redisReply *reply = redisCommand(context, "PING");
    if (!reply || reply->type != REDIS_REPLY_STATUS ||
        strcmp(reply->str, "PONG") != 0) {
        fprintf(stderr, "Redis PING failed\n");
        exit(1);
    }
    freeReplyObject(reply);
}

int main(int argc, char **argv) {
    if (argc == 3 && strcmp(argv[1], "--child") == 0)
        return !(fcntl(atoi(argv[2]), F_GETFD) == -1 && errno == EBADF);
    if (argc != 3)
        return 2;

    redisContext *contexts[] = {
        redisConnectUnix(argv[1]),
        redisConnect("127.0.0.1", atoi(argv[2]))
    };
    for (size_t i = 0; i < sizeof(contexts) / sizeof(contexts[0]); i++) {
        ping(contexts[i]);
        if (redisReconnect(contexts[i]) != REDIS_OK)
            return 1;
        ping(contexts[i]);
        redisFree(contexts[i]);
    }
    printf("socket-return exec checks: %u; failures: %u\n",
           socket_checks, socket_failures);
    return socket_checks != 4 || socket_failures != 0;
}
