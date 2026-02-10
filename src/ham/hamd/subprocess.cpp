#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>         // recv(), MSG_DONTWAIT
#include <limits.h>             // LINE_MAX
#include <string>               // std::string
#include <tuple>                // std::tuple

#include "subprocess.h"         // run()

#define SHELL_PATH  "/bin/sh"
#define SHELL_NAME  "sh"

class capture_pipe_c
{
private:
    #define RD_END 0
    #define WR_END 1

    int   stdout_pipe_fds[2] = { -1, -1 };
    int   stderr_pipe_fds[2] = { -1, -1 };
    bool  broken_m           = true;

    void _close(int fd_id)
    {
        (void)close(stdout_pipe_fds[fd_id]);
        stdout_pipe_fds[fd_id] = -1;

        (void)close(stderr_pipe_fds[fd_id]);
        stderr_pipe_fds[fd_id] = -1;
    }

    void _remap(int fd_id)
    {
        (void)dup2(stdout_pipe_fds[fd_id], STDOUT_FILENO);
        _close(stdout_pipe_fds[fd_id]); // Not needed anymore since it's been mapped to STDOUT

        (void)dup2(stderr_pipe_fds[fd_id], STDERR_FILENO);
        _close(stderr_pipe_fds[fd_id]); // Not needed anymore since it's been mapped to STDOUT
    }

public:
    capture_pipe_c()
    {
        broken_m = (0 != pipe(stdout_pipe_fds)) ||
                   (0 != pipe(stderr_pipe_fds));
    }

    ~capture_pipe_c()
    {
        _close(RD_END); // Close the reader end of both pipes
        _close(WR_END); // Close the writer end of both pipes
    }

    bool broken() const { return broken_m; }

    void running_as_child()
    {
        _remap(WR_END); // Remap stdout/stderr to the writer end of the two pipes
        _close(RD_END); // Close the reader end of the pipes when running in the child process
    }

    void running_as_parent()
    {
        _close(WR_END);  // Close the writer end of the pipes when running in the parent process
    }

    std::string stdout()
    {
        char    buf[LINE_MAX];
        ssize_t len = read(stdout_pipe_fds[RD_END], buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }

    std::string stderr()
    {
        char    buf[LINE_MAX];
        ssize_t len = read(stderr_pipe_fds[RD_END], buf, sizeof(buf));
        return (len > 0) ? std::string(buf, len) : "";
    }
};

std::tuple<int/*rc*/, std::string/*stdout*/, std::string/*stderr*/> run(const std::string & cmd_r)
{
    capture_pipe_c  pipe; // CREATE THE PIPE BEFORE FORKING!!!!
    if (pipe.broken())
        return std::make_tuple(-1, "", "failed to create stdout/stderr capture pipe");

    pid_t pid = fork();
    if (pid < (pid_t)0) // Did fork fail?
        return std::make_tuple(-1, "", "failed to fork process");

    if (pid == (pid_t)0) /* Child */
    {
        pipe.running_as_child();

        const char  * new_argv[4];
        new_argv[0] = SHELL_NAME;
        new_argv[1] = "-c";
        new_argv[2] = cmd_r.c_str();
        new_argv[3] = NULL;

        // Execute the shell
        (void)execve(SHELL_PATH, (char *const *)new_argv, environ);

        exit(127); // exit the child
    }

    /* Parent */
    pipe.running_as_parent();

    int exit_status = -1;
    if (TEMP_FAILURE_RETRY(waitpid(pid, &exit_status, 0)) != pid)
        exit_status = -1;

    bool term_normal = WIFEXITED(exit_status);
    if (!term_normal)
        return std::make_tuple(-1, "", "abnormal command termination");

    int    rc = WEXITSTATUS(exit_status);
    return std::make_tuple(rc, pipe.stdout(), pipe.stderr());
}
