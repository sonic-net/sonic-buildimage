/**
 * @file main.c
 *
 *  @brief: Main entry point for dhcpmon utility.
 *
 */

#include <err.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "subscriberstatetable.h"
#include "select.h"

#include "dhcp_mon.h"
#include "dhcp_devman.h"
#include "dhcp_device.h"

/** dhcpmon_default_snaplen: default snap length of packet being captured */
static const size_t dhcpmon_default_snaplen = 65535;
/** dhcpmon_default_health_check_window: default value for a time window, during which DHCP DORA packet counts are being
 *  collected */
static const uint32_t dhcpmon_default_health_check_window = 18;
/** dhcpmon_default_unhealthy_max_count: default max consecutive unhealthy status reported before reporting an issue
 *  with DHCP relay */
static const uint32_t dhcpmon_default_unhealthy_max_count = 10;

bool dual_tor_sock = false;

/**
 * @code usage(prog);
 *
 * @brief prints help message about how to use dhcpmon utility
 *
 * @param prog program name
 *
 * @return none
 */
static void usage(const char *prog)
{
    printf("Usage: %s -id <south interface> {-iu <north interface>}+ -im <mgmt interface> [-u <loopback interface>]"
            "[-w <snapshot window in sec>] [-c <unhealthy status count>] [-s <snap length>] [-D] [-d]\n", prog);
    printf("where\n");
    printf("\tsouth interface: is a vlan interface,\n");
    printf("\tnorth interface: is a TOR-T1 interface,\n");
    printf("\tloopback interface: is the loopback interface for dual tor setup,\n");
    printf("\tsnapshot window: during which DHCP counters are gathered and DHCP status is validated (default %d),\n",
            dhcpmon_default_health_check_window);
    printf("\tunhealthy status count: count of consecutive unhealthy status before writing an alert to syslog "
           "(default %d),\n",
           dhcpmon_default_unhealthy_max_count);
    printf("\tsnap length: snap length of packet capture (default %ld),\n", dhcpmon_default_snaplen);
    printf("\t-D: debug mode: print counter to syslog\n");
    printf("\t-d: daemonize %s.\n", prog);

    exit(EXIT_SUCCESS);
}

/**
 * @code dhcpmon_daemonize();
 *
 * @brief make this utility run as a daemon.
 *
 * @return none
 */
static void dhcpmon_daemonize()
{
    pid_t pid, sid;
    pid = fork();
    if (pid < 0) {
        syslog(LOG_ALERT, "fork: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    // this is the daemon running now
    umask(0);
    // Create a new SID for the child process
    sid = setsid();
    if (sid < 0) {
        syslog(LOG_ALERT, "setsid: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    // Change the current working directory
    if ((chdir("/")) < 0) {
        syslog(LOG_ALERT, "chdir: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

/**
 * @code main(argc, argv);
 *
 * @brief main entry point of dhcpmon utility
 *
 * @return int 0 on success, otherwise on failure
 */
int main(int argc, char **argv)
{
    int rv = EXIT_FAILURE;
    int i;
    int window_interval = dhcpmon_default_health_check_window;
    int max_unhealthy_count = dhcpmon_default_unhealthy_max_count;
    size_t snaplen = dhcpmon_default_snaplen;
    int make_daemon = 0;
    bool debug_mode = false;

    setlogmask(LOG_UPTO(LOG_INFO));
    openlog(basename(argv[0]), LOG_CONS | LOG_PID | LOG_NDELAY, LOG_DAEMON);

    for (i = 1; i < argc;) {
        if ((argv[i] == NULL) || (argv[i][0] != '-')) {
            break;
        }
        switch (argv[i][1])
        {
        case 'h':
            usage(basename(argv[0]));
            break;
        case 'i':
            if (dhcp_devman_add_intf(argv[i + 1], argv[i][2]) != 0) {
                usage(basename(argv[0]));
            }
            i += 2;
            break;
        case 'u':
            dual_tor_sock = true;
            if (dhcp_devman_setup_dual_tor_mode(argv[i + 1]) != 0) {
                usage(basename(argv[0]));
            }
            i += 2;
            break;
        case 'd':
            make_daemon = 1;
            i++;
            break;
        case 's':
            snaplen = atoi(argv[i + 1]);
            i += 2;
            break;
        case 'w':
            window_interval = atoi(argv[i + 1]);
            i += 2;
            break;
        case 'c':
            max_unhealthy_count = atoi(argv[i + 1]);
            i += 2;
            break;
        case 'D':
            debug_mode = true;
            i += 1;
            break;
        default:
            fprintf(stderr, "%s: %c: Unknown option\n", basename(argv[0]), argv[i][1]);
            usage(basename(argv[0]));
        }
    }

    if (make_daemon) {
        dhcpmon_daemonize();
    }

    if ((dhcp_mon_init(window_interval, max_unhealthy_count) == 0) &&
        (dhcp_mon_start(snaplen, debug_mode) == 0)) {

        rv = EXIT_SUCCESS;

        dhcp_mon_shutdown();
    }

    dhcp_devman_shutdown();

    closelog();

    return rv;
}
