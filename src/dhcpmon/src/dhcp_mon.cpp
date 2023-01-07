/**
 * @file dhcp_mon.c
 *
 *  @brief dhcp relay monitor module
 */

#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <syslog.h>
#include <assert.h>

#include "dhcp_mon.h"
#include "dhcp_devman.h"
#include "events.h"

/** DHCP device/interface state */
typedef struct
{
    dhcp_mon_check_t check_type;                /** check type */
    dhcp_device_context_t* (*get_context)();    /** functor to a device context accessor function */
    int count;                                  /** count in the number of unhealthy checks */
    const char *msg;                            /** message to be printed if unhealthy state is determined */
} dhcp_mon_state_t;

/** window_interval_sec monitoring window for dhcp relay health checks */
static int window_interval_sec = 18;
/** dhcp_unhealthy_max_count max count of consecutive unhealthy statuses before reporting to syslog */
static int dhcp_unhealthy_max_count = 10;
/** libevent base struct */
static struct event_base *base;
/** libevent timeout event struct */
static struct event *ev_timeout = NULL;
/** libevent SIGINT signal event struct */
static struct event *ev_sigint;
/** libevent SIGTERM signal event struct */
static struct event *ev_sigterm;
/** libevent SIGUSR1 signal event struct */
static struct event *ev_sigusr1;

event_handle_t g_events_handle;

/** DHCP monitor state data for aggregate device for mgmt device */
static dhcp_mon_state_t state_data[] = {
    [0] = {
        .check_type = DHCP_MON_CHECK_POSITIVE,
        .get_context = dhcp_devman_get_agg_dev,
        .count = 0,
        .msg = "dhcpmon detected disparity in DHCP Relay behavior. Duration: %d (sec) for vlan: '%s'\n"
    },
    [1] = {
        .check_type = DHCP_MON_CHECK_NEGATIVE,
        .get_context = dhcp_devman_get_mgmt_dev,
        .count = 0,
        .msg = "dhcpmon detected DHCP packets traveling through mgmt interface (please check BGP routes.)"
               " Duration: %d (sec) for intf: '%s'\n"
    }
};

/**
 * @code signal_callback(fd, event, arg);
 *
 * @brief signal handler for dhcpmon. It will initiate shutdown when signal is caught
 *
 * @param fd        libevent socket
 * @param event     event triggered
 * @param arg       pointer to user provided context (libevent base)
 *
 * @return none
 */
static void signal_callback(evutil_socket_t fd, short event, void *arg)
{
    syslog(LOG_ALERT, "Received signal: '%s'\n", strsignal(fd));
    dhcp_devman_print_status(NULL, DHCP_COUNTERS_CURRENT);
    if ((fd == SIGTERM) || (fd == SIGINT)) {
        dhcp_mon_stop();
    }
}

/**
 * @code check_dhcp_relay_health(state_data);
 *
 * @brief check DHCP relay overall health
 *
 * @param state_data        pointer to dhcpmon state data
 *
 * @return none
 */
static void check_dhcp_relay_health(dhcp_mon_state_t *state_data)
{
    dhcp_device_context_t *context = state_data->get_context();
    dhcp_mon_status_t dhcp_mon_status = dhcp_devman_get_status(state_data->check_type, context);

    switch (dhcp_mon_status)
    {
    case DHCP_MON_STATUS_UNHEALTHY:
        if (++state_data->count > dhcp_unhealthy_max_count) {
            auto duration = state_data->count * window_interval_sec;
            std::string vlan(context->intf);
            syslog(LOG_ALERT, state_data->msg, duration, context->intf);
            if (state_data->check_type == DHCP_MON_CHECK_POSITIVE) {
                event_params_t params = {
                    { "vlan", vlan },
                    { "duration", std::to_string(duration) }};
                event_publish(g_events_handle, "dhcp-relay-disparity", &params);
            }
            dhcp_devman_print_status(context, DHCP_COUNTERS_SNAPSHOT);
            dhcp_devman_print_status(context, DHCP_COUNTERS_CURRENT);
        }
        break;
    case DHCP_MON_STATUS_HEALTHY:
        state_data->count = 0;
        break;
    case DHCP_MON_STATUS_INDETERMINATE:
        if (state_data->count) {
            state_data->count++;
        }
        break;
    default:
        syslog(LOG_ERR, "DHCP Relay returned unknown status %d\n", dhcp_mon_status);
        break;
    }
}

/**
 * @code timeout_callback(fd, event, arg);
 *
 * @brief periodic timer call back
 *
 * @param fd        libevent socket
 * @param event     event triggered
 * @param arg       pointer user provided context (libevent base)
 *
 * @return none
 */
static void timeout_callback(evutil_socket_t fd, short event, void *arg)
{
    for (uint8_t i = 0; i < sizeof(state_data) / sizeof(*state_data); i++) {
        check_dhcp_relay_health(&state_data[i]);
    }

    dhcp_devman_update_snapshot(NULL);
}

/**
 * @code dhcp_mon_init(window_sec, max_count);
 *
 * initializes event base and periodic timer event that continuously collects dhcp relay health status every window_sec
 * seconds. It also writes to syslog when dhcp relay has been unhealthy for consecutive max_count checks.
 *
 */
int dhcp_mon_init(int window_sec, int max_count)
{
    int rv = -1;

    do {
        window_interval_sec = window_sec;
        dhcp_unhealthy_max_count = max_count;

        base = event_base_new();
        if (base == NULL) {
            syslog(LOG_ERR, "Could not initialize libevent!\n");
            break;
        }

        ev_sigint = evsignal_new(base, SIGINT, signal_callback, base);
        if (ev_sigint == NULL) {
            syslog(LOG_ERR, "Could not create SIGINT libevent signal!\n");
            break;
        }

        ev_sigterm = evsignal_new(base, SIGTERM, signal_callback, base);
        if (ev_sigterm == NULL) {
            syslog(LOG_ERR, "Could not create SIGTERM libevent signal!\n");
            break;
        }

        ev_sigusr1 = evsignal_new(base, SIGUSR1, signal_callback, base);
        if (ev_sigusr1 == NULL) {
            syslog(LOG_ERR, "Could not create SIGUSER1 libevent signal!\n");
            break;
        }

        ev_timeout = event_new(base, -1, EV_PERSIST, timeout_callback, base);
        if (ev_timeout == NULL) {
            syslog(LOG_ERR, "Could not create libevent timer!\n");
            break;
        }

        g_events_handle = events_init_publisher("sonic-events-dhcp-relay");

        rv = 0;
    } while (0);

    return rv;
}

/**
 * @code dhcp_mon_shutdown();
 *
 * @brief shuts down libevent loop
 */
void dhcp_mon_shutdown()
{
    event_del(ev_timeout);
    event_del(ev_sigint);
    event_del(ev_sigterm);
    event_del(ev_sigusr1);

    event_free(ev_timeout);
    event_free(ev_sigint);
    event_free(ev_sigterm);
    event_free(ev_sigusr1);

    event_base_free(base);

    events_deinit_publisher(g_events_handle);
}

/**
 * @code dhcp_mon_start(snaplen);
 *
 * @brief start monitoring DHCP Relay
 */
int dhcp_mon_start(size_t snaplen)
{
    int rv = -1;

    do
    {
        if (dhcp_devman_start_capture(snaplen, base) != 0) {
            break;
        }

        if (evsignal_add(ev_sigint, NULL) != 0) {
            syslog(LOG_ERR, "Could not add SIGINT libevent signal!\n");
            break;
        }

        if (evsignal_add(ev_sigterm, NULL) != 0) {
            syslog(LOG_ERR, "Could not add SIGTERM libevent signal!\n");
            break;
        }

        if (evsignal_add(ev_sigusr1, NULL) != 0) {
            syslog(LOG_ERR, "Could not add SIGUSR1 libevent signal!\n");
            break;
        }

        struct timeval event_time = {.tv_sec = window_interval_sec, .tv_usec = 0};
        if (evtimer_add(ev_timeout, &event_time) != 0) {
            syslog(LOG_ERR, "Could not add event timer to libevent!\n");
            break;
        }

        if (event_base_dispatch(base) != 0) {
            syslog(LOG_ERR, "Could not start libevent dispatching loop!\n");
            break;
        }

        rv = 0;
    } while (0);

    return rv;
}

/**
 * @code dhcp_mon_stop();
 *
 * @brief stop monitoring DHCP Relay
 */
void dhcp_mon_stop()
{
    event_base_loopexit(base, NULL);
}
