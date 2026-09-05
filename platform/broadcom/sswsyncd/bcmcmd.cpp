#include <unistd.h>
#include <limits.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <system_error>
#include "dsserve.h"
using namespace std;

class timeout_error : public std::runtime_error
{
public:
    explicit timeout_error(const string& what_arg) : std::runtime_error(what_arg) { }
    explicit timeout_error(const char* what_arg) : std::runtime_error(what_arg) { }
};

class socketio_error : public std::runtime_error
{
public:
    explicit socketio_error(const string& what_arg) : std::runtime_error(what_arg) { }
    explicit socketio_error(const char* what_arg) : std::runtime_error(what_arg) { }
};

// Default timeout for connecting to brcm sdk unix domain socket
// User may change it by command line argument
const int DEFAULT_TIMEOUT_SEC = 30;

const int MILLISECONDS_IN_SEC = 1000;

typedef vector<string>::iterator vsi;

ssize_t write(int fd, const string& s) {
    return write(fd, s.c_str(), s.size());
}

/*  returns true iff str ends with suffix  */
bool str_ends_with(const char * str, const char * suffix) {
    if( str == NULL || suffix == NULL ) return 0;

    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if(suffix_len > str_len) return 0;

    return 0 == strncmp(str + str_len - suffix_len, suffix, suffix_len);
}

/* return the index of matched prompt, otherwise -1 */
/* output parameter: bytes_read - the total bytes read till the prompt (inclusive) */
int read_to_prompts(int sock, vsi prompt_begin, vsi prompt_end, bool enable_out, int ms_timeout, size_t& bytes_read) {
    const size_t BUF_SIZE = 1024;
    static char buf[BUF_SIZE];
    static size_t leftover = 0;
    bytes_read = 0;

    struct pollfd fd;
    fd.fd = sock;
    fd.events = POLLIN;

    for (;;) {
        // Discard complete lines left in the buffer, and keep the last partial line
        // The line delimiters could be \n, or \r\n
        // Note: if buffer ends with \r, it may follow by \n later
        char *p;
        for (p = buf + leftover - 1; p >= buf; p--)
        {
            if (*p == '\n') break;
        }
        if (p < buf)
        {
            if (leftover >= BUF_SIZE - 1)
            {
                // There is large amount of char in the buffer without line separators,
                // which is unexpected. Just flush the left over chars.
                leftover = 0;
            }
        }
        else
        {
            char *partial = p + 1;
            leftover = buf + leftover - partial;
            memmove(buf, partial, leftover);
        }

        // Poll the sock to detect timeout or other errors
        int res = poll(&fd, 1, ms_timeout);
        switch (res) {
        case -1:
            throw socketio_error("polling socket error");
        case 0:
            throw timeout_error("polling socket timeout");
        default:
            // will get your data later
            break;
        }

        // Read a batch from the socket
        ssize_t rval = read(sock, buf + leftover, (BUF_SIZE - 1 - leftover));

        if (rval < 0) throw socketio_error("reading stream message");
        if (rval == 0) throw socketio_error("ending connection");
        bytes_read += (size_t)rval;
        buf[leftover + rval] = '\0'; // Terminate read string with NUL char
        if (enable_out) {
            printf("%s", buf + leftover);
            fflush(stdout);
        }
        leftover += rval;

        int index = 0;
        for (auto i = prompt_begin; i != prompt_end; i++, index++) {
            if (str_ends_with(buf, i->c_str()))
                return index;
        }
    }
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_un server;

    auto usage = [=]() {
        printf("USAGE: %s [-f <sun_path>] -v <cmd>\n", argv[0]);
        printf("  -v                         verbose mode\n");
        printf("  -f                         domain socket filename, default %s\n", DEFAULT_SUN_PATH);
        printf("  -t                         timeout in seconds, default %d\n", DEFAULT_TIMEOUT_SEC);
        printf("RETURN VALUE:\n"
               "    0                        success\n");
        printf("  %3d                        socket io error\n", EIO);
        printf("  %3d                        invalid argument\n", EINVAL);
        printf("  %3d                        timeout\n", ETIME);
        printf("\n");
        return EINVAL;
    };

    // Parse command line
    const char *sun_path = DEFAULT_SUN_PATH;
    const char *cmd = NULL;
    bool verbose = false;
    int timeout_sec = DEFAULT_TIMEOUT_SEC;
    if (argc < 2) {
        return usage();
    }
    for (argc--, argv++; argc > 0 && *argv; argc--, argv++) {
        if (!strcmp(*argv, "--help") || !strcmp(*argv, "-h")) {
            return usage();
        }
        else if (!strcmp(*argv, "-v")) {
            verbose = true;
        }
        else if (!strcmp(*argv, "-t")) {
            argc--, argv++;
            if (argc > 1 && *argv && isdigit(argv[0][0])) {
                timeout_sec = atoi(*argv);
            }
            else {
                fprintf(stderr, "[ERROR] bad timeout\n");
                return usage();
            }
        }
        else if (!strcmp(*argv, "-f")) {
            argc--, argv++;
            if (argc > 1 && *argv) {
                sun_path = *argv;
            }
            else {
                fprintf(stderr, "[ERROR] bad domain socket filename\n");
                return usage();
            }
            if (verbose) printf("[INFO] domain socket filename: %s\n", sun_path);
        }
        else {
            cmd = *argv;
            if (verbose) printf("[INFO] cmd: %s\n", cmd);
        }
    }
    if (cmd == NULL || *cmd == '\0' || *sun_path == '\0' || timeout_sec < 0) {
        return usage();
    }
    int timeout_ms;
    if (timeout_sec >= INT_MAX / MILLISECONDS_IN_SEC) {
        timeout_ms = INT_MAX;
    }
    else {
        timeout_ms = timeout_sec * MILLISECONDS_IN_SEC;
    }

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening stream socket");
        exit(1);
    }
    server.sun_family = AF_UNIX;
    strcpy(server.sun_path, sun_path);

    if (connect(sock, (struct sockaddr *) &server, sizeof(struct sockaddr_un)) < 0) {
        close(sock);
        perror("connecting stream socket");
        exit(1);
    }

    ssize_t written;
    written = write(sock, "\n");
    if (written <= 0) {
        perror("writing on stream socket");
        exit(1);
    }

    // Interactive with shell
    // Wait for the first shell prompt
    vector<string> prompt = { "Hit enter to get drivshell prompt..\r\n", "drivshell>" };

    try
    {
        int index;
        size_t bytesread;
        index = read_to_prompts(sock, prompt.begin(), prompt.end(), false, timeout_ms, bytesread);
        if (index < 0) {
            perror("failed to wait the prompt");
            exit(index);
        }

        if (index == 0) {
            // Write enter char to socket
            written = write(sock, "\n");
            if (written <= 0) {
                perror("failed to write enter");
                exit(1);
            }

            // Wait next prompt
            index = read_to_prompts(sock, prompt.begin() + 1, prompt.begin() + 2, true, timeout_ms, bytesread);
            if (index < 0) {
                perror("failed to wait the prompt");
                exit(index);
            }
        }

        // Write the command to the socket
        written = write(sock, cmd + string("\n"));
        if (written <= 0) {
            perror("failed to write command");
            exit(1);
        }

        // Wait for the prompt after the command output, may ignore empty prompt lines
        do {
            index = read_to_prompts(sock, prompt.begin() + 1, prompt.begin() + 2, true, timeout_ms, bytesread);
        } while (bytesread == prompt[1].size() && index >= 0);

        if (index < 0) {
            perror("failed to wait the prompt");
            exit(index);
        }

        printf("\n"); // Print enter after the final prompt
        close(sock);
        return 0;
    }
    catch(timeout_error& ex)
    {
        perror(ex.what());
        exit(ETIME);
    }
    catch(socketio_error& ex)
    {
        perror(ex.what());
        exit(EIO);
    }
}
