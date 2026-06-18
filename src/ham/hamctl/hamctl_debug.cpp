// Host Account Management
#include <getopt.h>
#include <string.h>
#include <limits.h>
#include <dbus/dbus-shared.h>           // DBUS_BUS_SYSTEM

#include "hamctl.h"                     // get_dbusconn()
#include "hamctl_subsys.h"
#include "../shared/dbus-address.h"     // DBUS_BUS_NAME_BASE

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
static int debug(int argc, char *argv[])
{
    static const struct option options[] =
    {
        { "help",  no_argument,       NULL, 'h' },
        { "help",  no_argument,       NULL, '?' },
        {}
    };

    static const char * usage_p =
        CTL_NAME " debug [OPTIONS] [COMMANDS]\n"
        "\n"
        "This is used by software developers for debugging purposes\n"
        "\n"
        "OPTIONS:\n"
        " -?,-h,--help          print this message\n"
        "\n"
        "COMMANDS:\n"
        " tron                  Trace on\n"
        " troff                 Trace off\n"
        " show                  Print debug information\n";

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
            debug_proxy_c  debug(get_dbusconn(), DBUS_BUS_NAME_BASE, DBUS_OBJ_PATH_BASE);

            if (0 == strcmp("tron", command_p))
            {
                std::cout << debug.tron() << '\n';
            }
            else if (0 == strcmp("troff", command_p))
            {
                std::cout << debug.troff() << '\n';
            }
            else if (0 == strcmp("show", command_p))
            {
                std::cout << debug.show() << '\n';
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
            fprintf(stderr, "hamctl debug - %s\n", ex.what());
        }
    }

    return rc;
}

const subsys_c subsys_debug("debug", "Debug commands", debug, false);

