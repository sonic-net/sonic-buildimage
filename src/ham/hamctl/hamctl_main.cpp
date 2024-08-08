// Host Account Management
#include <dbus-c++/dbus.h>

#include <string.h>         // strcmp()
#include <getopt.h>         // getopt_long()
#include <stdlib.h>         // EXIT_SUCCESS, EXIT_FAILURE
#include <errno.h>          // program_invocation_short_name
#include <unistd.h>         /* geteuid() */

#include "hamctl_subsys.h"
#include "hamctl.h"

#define N_ELEMENTS(arr)  (sizeof (arr) / sizeof ((arr)[0]))

static const struct subsys_c * subsystems[] =
{
    &subsys_accounts,
    &subsys_debug,
    &subsys_sac,
};

/**
 * @brief Print the help text
 *
 * @param argc
 * @param argv
 *
 * @return int
 */
static int help(int argc, char *argv[])
{
    unsigned int i;

    fprintf(stderr,
            CTL_NAME " [--help|-h|-?] [--version|-V] [SUBSYS] [OPTIONS] [COMMAND] [ARGS]\n"
            "\n"
            "Query or send control commands to the Host Account Management Daemon (hamd).\n"
            "\n"
            "SUBSYS:\n"
           );
    for (i = 0; i < N_ELEMENTS(subsystems); i++)
    {
        if ((subsystems[i]->help_pm != NULL) && !subsystems[i]->hidden_m)
            printf("  %-12s %s\n", subsystems[i]->name_pm, subsystems[i]->help_pm);
    }
    fprintf(stderr,
            "\n"
            "To get help for a particular subsystem use:\n"
            CTL_NAME " [SUBSYS] --help\n"
           );
    return 0;
}

/**
 * @brief Print the version ID
 *
 * @param argc
 * @param argv
 *
 * @return int
 */
static int version(int argc, char *argv[])
{
    printf("%s\n", VERSION);
    return 0;
}

/**
 * @brief Run one of the commands
 *
 * @param command_r
 * @param argc
 * @param argv
 *
 * @return int
 */
static int run_command(const subsys_c & command_r, int argc, char *argv[])
{
    return command_r.cmd_pm(argc, argv);
}

/**
 * @brief Main entry point
 *
 * @param argc
 * @param argv
 *
 * @return int
 */
int main(int argc, char *argv[])
{
    if (geteuid() != 0)
    {
        fprintf(stderr, CTL_NAME ": Permission denied. Root privileges required.\n");
        exit(EXIT_FAILURE);
    }

    setvbuf(stdout, NULL, _IONBF, 0); // Make sure stdout is unbuffered

    static const struct option options[] =
    {
        { "help",    no_argument, NULL, 'h' },
        { "help",    no_argument, NULL, '?' },
        { "version", no_argument, NULL, 'V' },
        {}
    };
    const char   * command;
    unsigned int   i;
    int            c;
    int            rc = 1;

    while ((c = getopt_long(argc, argv, "+h?V", options, NULL)) >= 0)
    {
        switch (c)
        {
        case '?':
        case 'h': return help(argc, argv);
        case 'V': return version(argc, argv);
        default:  return 1;
        }
    }

    command = argv[optind];

    if (command != NULL)
    {
        for (i = 0; i < N_ELEMENTS(subsystems); i++)
        {
            if (0 == strcmp(subsystems[i]->name_pm, command))
            {
                argc -= optind;
                argv += optind;
                /* we need '0' here to reset the internal state */
                optind = 0;
                rc = run_command(*subsystems[i], argc, argv);
                goto out;
            }
        }
    }

    fprintf(stderr, CTL_NAME ": missing or unknown command\n");
    rc = 2;

out:
    exit((0 == rc) ? EXIT_SUCCESS : EXIT_FAILURE);
}
