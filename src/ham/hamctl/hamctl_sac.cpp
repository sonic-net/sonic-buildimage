// Host Account Management
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>                     // getpid()
#include <dbus/dbus-shared.h>           // DBUS_BUS_SYSTEM

#include "hamctl.h"                     // get_dbusconn()
#include "hamctl_subsys.h"
#include "../shared/dbus-address.h"     // DBUS_BUS_NAME_BASE
#include "../shared/utils.h"            // split_exact()

#define FMT_RED         "\x1b[0;31m"
#define FMT_NORMAL      "\x1b[0m"

/**
 * @brief hamctl's debug command
 *
 * @param argc
 * @param argv
 *
 * @return int
 */
static int sac(int argc, char *argv[])
{
    static const struct option options[] =
    {
        { "help",  no_argument,       NULL, 'h' },
        { "help",  no_argument,       NULL, '?' },
        {}
    };

    static const char * usage_p =
        CTL_NAME " sac [OPTIONS] [COMMANDS]\n"
        "\n"
        "This is used by software developers for debugging purposes\n"
        "\n"
        "OPTIONS:\n"
        " -?,-h,--help         print this message\n"
        "\n"
        "COMMANDS:\n"
        " add_unconfirmed_user [LOGIN]          Add a local unconfirmed user\n"
        " user_confirm         [LOGIN] [ROLES]  Confirm the user\n"
        "\n"
        "ARGUMENTS:\n"
        " [LOGIN]              User login name\n"
        " [ROLES]              Comma-separated roles\n";

    int  c;
    int  rc = 0;

    while ((c = getopt_long(argc, argv, "h?", options, NULL)) >= 0)
    {
        switch (c)
        {
        case '?':
        case 'h': printf("%s\n", usage_p); return 0;
        default:  return 1;
        }
    }

    const char * command_p = argv[optind];

    if (command_p == NULL)
    {
        rc = 1;
        fprintf(stderr, FMT_RED "Error! Missing command" FMT_NORMAL "\n");
    }

    if (rc == 0)
    {
        try
        {
            sac_proxy_c  sac(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

            if (0 == strcmp("add_unconfirmed_user", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char  * login_p = argv[2];
                std::cout << sac.add_unconfirmed_user(login_p, (uint32_t)getpid()) << '\n';
            }
            else if (0 == strcmp("user_confirm", command_p))
            {
                if (argc < 3)
                {
                    fprintf(stderr, "%s: Missing arguments. Try --help.\n", program_invocation_short_name);
                    return -1;
                }

                const char                  * login_p = argv[2];
                std::vector< std::string >    roles;

                if (argc == 4) roles = split_exact(argv[3], ",");

                std::cout << sac.user_confirm(login_p, roles, "hamctl-manual-test") << '\n';
            }
            else
            {
                rc = 1;
                fprintf(stderr, FMT_RED "Error! Unknown command \"%s\"" FMT_NORMAL "\n", command_p);
            }
        }
        catch (DBus::Error &ex)
        {
            rc = 1;
            fprintf(stderr, "hamctl sac - %s\n", ex.what());
        }
    }

    return rc;
}

const subsys_c subsys_sac("sac", "System-assigned credentials commands", sac, false);

