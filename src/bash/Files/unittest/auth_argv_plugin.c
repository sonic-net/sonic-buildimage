#include <string.h>

int plugin_init(void) { return 0; }
int plugin_uninit(void) { return 0; }

int on_shell_execve(char *user, int shell_level, char *cmd, char **argv)
{
    int i;

    (void)user;
    (void)shell_level;
    (void)cmd;
    for (i = 1; argv[i] != 0; ++i)
        if (strcmp(argv[i], "blocked") == 0)
            return 1;
    return 0;
}
