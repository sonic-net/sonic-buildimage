/**
 * Run this file with the shell commands passed in
 */
#include "argparse.h"
#include "libunixsocket.h"
#include "linenoise.h"
#include "sdk_rev.h"

#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// lets just make the recv buffer large to handle all the variability in size
static char dataBuffer[4096];
static void inline recieve_data(int fd) {
    ssize_t n = recv(fd, dataBuffer, sizeof(dataBuffer) - 1, MSG_NOSIGNAL);
    if (n < 0) {
        perror("Unable to recieve message from server");
        exit(-1);
    }
    dataBuffer[n] = '\0';
}

static void inline send_data(int fd, const char *data) {
    ssize_t n = send(fd, data, strlen(data), MSG_NOSIGNAL);
    if (n < strlen(data)) {
        perror("Unable to send message to server");
        exit(-1);
    }
}

static const char *const usages[] = {
    "crcli [options]                  Uses default /tmp/credo-server.sock",
    "crcli [options] <socket_file>    Specify custom socket location",
    NULL,
};

static int sockfd;
FILE *capfp = NULL;

char printbuff[4096];
static void print_output(int outfd) {
    while (true) {
        ssize_t n = recv(outfd, printbuff, sizeof(printbuff), MSG_NOSIGNAL);
        if (n <= 0) {
            return;
        }
        fwrite(printbuff, sizeof(char), n, stdout);
        if (capfp != NULL) {
            fwrite(printbuff, sizeof(char), n, capfp);
        }
    }
}
void get_completions(const char *data, linenoiseCompletions *lc, size_t pos) {
    snprintf(dataBuffer, sizeof(dataBuffer), "@auto:%s", data);
    send_data(sockfd, dataBuffer);
    recieve_data(sockfd);  // ack
    snprintf(dataBuffer, sizeof(dataBuffer), "%lu", (unsigned long)pos);
    send_data(sockfd, dataBuffer);

    while (true) {
        recieve_data(sockfd);
        if (strcmp(dataBuffer, "@auto:done") == 0) {
            break;
        }
        char autobuff[4096];
        snprintf(autobuff, sizeof(autobuff), "%s", dataBuffer);
        send_data(sockfd, "ack");
        recieve_data(sockfd);
        size_t autopos = strtoul(dataBuffer, NULL, 10);
        send_data(sockfd, "ack");
        linenoiseAddCompletion(lc, autobuff, autopos);
    }
}

int main(int argc, const char *argv[]) {
    int kill_server = 0, show_version = 0, server_interrupt = -1, wait_server = 0;
    const char *capfile = NULL;
    struct argparse_option options[] = {
        OPT_GROUP("Actions"),
        OPT_STRING('c', "capture", &capfile,
                   "Capture output simultaneously to stdout and specified file (teeing). Defaults to off.", NULL, 0, 0),
        OPT_BOOLEAN('k', "kill", &kill_server, "kill the server. This does not kill the process", NULL, 0, 0),
        OPT_INTEGER('i', "interrupt", &server_interrupt, "interrupt (SIGINT) current shell in server", NULL, 0, 0),
        OPT_BOOLEAN('w', "wait", &wait_server, "wait for the server to come up", NULL, 0, 0),

        OPT_GROUP("Information"),
        OPT_HELP(),
        OPT_BOOLEAN('v', "version", &show_version, "show the version", NULL, 0, 0),
        OPT_END(),
    };

    struct argparse argparse;
    argparse_init(&argparse, options, usages, 0);
    argparse_describe(&argparse, "\nUtility to Connect to credo shell server",
                      "\nProvides methods of specify where and how to connect.\n"
                      "You can also provide an optional socket_file argument if spawning from a different location.\n");
    argc = argparse_parse(&argparse, argc, argv);

    if (show_version) {
        printf("crcli (version %s)\n", CREDO_SDK_REV);
        return 0;
    }

    if (server_interrupt != -1) {  // user wants to stop shell running something that is taking too long
        printf("Interrupting Server PID (SIGINT): %i\n", server_interrupt);
        kill(server_interrupt, SIGINT);
        return 0;
    }

    if (capfile != NULL) {
        capfp = fopen(capfile, "a");
        if (capfp == NULL) {
            perror("Error opening capture file");
            exit(-1);
        }
    }

    const char *socket_addr = "/tmp/credo-server.sock";  // default value
    if (argc >= 1) {
        socket_addr = argv[0];
    }
    char socket_addr2[1024];
    snprintf(socket_addr2, sizeof(socket_addr2), "%s-out", socket_addr);

    do {
        sockfd = create_unix_stream_socket(socket_addr, 0);
    } while (sockfd < 0 && wait_server);  // keep trying to connect to the socket if wait is enabled
    if (sockfd < 0) {
        perror("Unable to open socket");
        exit(-1);
    }

    if (kill_server) {
        send_data(sockfd, "@kill");
        return 0;
    }

    send_data(sockfd, "@start");

    recieve_data(sockfd);
    long server_pid = strtol(dataBuffer, NULL, 10);
    printf("Server PID: %li\n", server_pid);
    send_data(sockfd, "ack");

    linenoiseHistoryLoad("./.crsh_history");
    linenoiseHistorySetMaxLen(100);
    linenoiseSetCompletionCallback(get_completions);

    while (1) {
        // get the first command
        recieve_data(sockfd);
        if (strncmp(dataBuffer, "@prompt:", 8) == 0) {
            char *line = NULL;
            char *prompt = strdup(dataBuffer + 8);
            while (line == NULL) {
                line = linenoise(prompt);
                // ctrl+D <eof> exit
                if (line == NULL) {
                    free(prompt);
                    return 0;
                }
                size_t line_len = strlen(line);
                // run sys commands locally
                if (line_len > 0 && line[0] == '!') {
                    system(line + 1);  // NOLINT(cert-env33-c)
                    linenoiseFree(line);
                    line = NULL;
                    continue;
                }
                // make sure line is not too long
                if (line_len > 1023) {
                    fprintf(stderr, "Line is too long %zu (>1023)\n", line_len);
                    linenoiseFree(line);
                    line = NULL;
                    continue;
                }
            }
            linenoiseHistoryAdd(line); /* Add to the history. */
            linenoiseHistorySave("./.crsh_history");
            if (capfp != NULL) {
                fprintf(capfp, "%s%s\n", prompt, line);
            }
            snprintf(dataBuffer, sizeof(dataBuffer), "@input:%s", line);
            send_data(sockfd, dataBuffer);
            linenoiseFree(line);
            free(prompt);
            int outfd = create_unix_stream_socket(socket_addr2, 0);
            if (outfd < 0) {
                perror("Unable to open log socket");
                exit(-1);
            }
            print_output(outfd);
            shutdown(outfd, SHUT_RDWR);
            close(outfd);
        } else if (strcmp(dataBuffer, "@quit") == 0) {
            return 0;
        } else {
            fprintf(stderr, "Invalid packet: %s\n", dataBuffer);
            return -1;
        }
    }
    return 0;
}
