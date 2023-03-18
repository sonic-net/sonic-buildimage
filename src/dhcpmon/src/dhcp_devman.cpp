/**
 * @file dhcp_devman.c
 *
 *  Device (interface) manager
 */
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <stdlib.h>

#include "dhcp_devman.h"

/** Prefix appended to Aggregation device */
#define AGG_DEV_PREFIX  "Agg-"

/** intfs map of interfaces */
std::unordered_map<std::string, struct intf*> intfs;
/** dhcp_num_south_intf number of south interfaces */
static uint32_t dhcp_num_south_intf = 0;
/** dhcp_num_north_intf number of north interfaces */
static uint32_t dhcp_num_north_intf = 0;
/** dhcp_num_mgmt_intf number of mgmt interfaces */
static uint32_t dhcp_num_mgmt_intf = 0;

/** On Device  vlan interface IP address corresponding vlan downlink IP
 *  This IP is used to filter Offer/Ack packet coming from DHCP server */
static in_addr_t vlan_ip = 0;

/* Device loopback interface ip, which will be used as the giaddr in dual tor setup. */
static in_addr_t loopback_ip = 0;

/* Whether the device is in dual tor mode, 0 as default for single tor mode. */
static int dual_tor_mode = 0;

/** mgmt interface */
static struct intf *mgmt_intf = NULL;

/**
 * @code dhcp_devman_get_vlan_intf();
 *
 * Accessor method
 */
dhcp_device_context_t* dhcp_devman_get_agg_dev()
{
    return dhcp_device_get_aggregate_context();
}

/**
 * @code dhcp_devman_get_mgmt_dev();
 *
 * Accessor method
 */
dhcp_device_context_t* dhcp_devman_get_mgmt_dev()
{
    return mgmt_intf ? mgmt_intf->dev_context : NULL;
}

/**
 * @code dhcp_devman_shutdown();
 *
 * shuts down device (interface) manager. Also, stops packet capture on interface and cleans up any allocated
 * memory
 */
void dhcp_devman_shutdown()
{
    for (auto it = intfs.begin(); it != intfs.end();) {
        auto inf = it->second;
        dhcp_device_shutdown(inf->dev_context);
        it = intfs.erase(it);
        free(inf);
    }
}

/**
 * @code dhcp_devman_add_intf(name, is_uplink);
 *
 * @brief adds interface to the device manager.
 */
int dhcp_devman_add_intf(const char *name, char intf_type)
{
    int rv = -1;
    struct intf *dev = (struct intf*) malloc(sizeof(struct intf));

    if (dev != NULL) {
        dev->name = name;
        dev->is_uplink = intf_type != 'd';

        switch (intf_type)
        {
        case 'u':
            dhcp_num_north_intf++;
            break;
        case 'd':
            dhcp_num_south_intf++;
            assert(dhcp_num_south_intf <= 1);
            break;
        case 'm':
            dhcp_num_mgmt_intf++;
            assert(dhcp_num_mgmt_intf <= 1);
            mgmt_intf = dev;
            break;
        default:
            break;
        }

        rv = dhcp_device_init(&dev->dev_context, dev->name, dev->is_uplink);
        if (rv == 0 && intf_type == 'd') {
            rv = dhcp_device_get_ip(dev->dev_context, &vlan_ip);

            dhcp_device_context_t *agg_dev = dhcp_device_get_aggregate_context();

            strncpy(agg_dev->intf, AGG_DEV_PREFIX, strlen(AGG_DEV_PREFIX) + 1);
            strncpy(agg_dev->intf + strlen(AGG_DEV_PREFIX), name, sizeof(agg_dev->intf) - strlen(AGG_DEV_PREFIX) - 1);
            agg_dev->intf[sizeof(agg_dev->intf) - 1] = '\0';
            syslog(LOG_INFO, "dhcpmon add aggregate interface '%s'\n", agg_dev->intf);
        }
        std::string if_name;
        if_name.assign(dev->name);
        intfs[if_name] = dev;
    }
    else {
        syslog(LOG_ALERT, "malloc: failed to allocate memory for intf '%s'\n", name);
    }

    return rv;
}

/**
 * @code dhcp_devman_setup_dual_tor_mode(name);
 *
 * @brief set up dual tor mode: 1) set dual_tor_mode flag and 2) retrieve loopback_ip.
 */
int dhcp_devman_setup_dual_tor_mode(const char *name)
{
    int rv = -1;

    dhcp_device_context_t loopback_intf_context;

    if (strlen(name) < sizeof(loopback_intf_context.intf)) {
        strncpy(loopback_intf_context.intf, name, sizeof(loopback_intf_context.intf) - 1);
        loopback_intf_context.intf[sizeof(loopback_intf_context.intf) - 1] = '\0';
    } else {
        syslog(LOG_ALERT, "loopback interface name (%s) is too long", name);
        return rv;
    }

    if (initialize_intf_mac_and_ip_addr(&loopback_intf_context) == 0 &&
        dhcp_device_get_ip(&loopback_intf_context, &loopback_ip) == 0) {
            dual_tor_mode = 1;
    } else {
        syslog(LOG_ALERT, "failed to retrieve ip addr for loopback interface (%s)", name);
        return rv;
    }

    rv = 0;
    return rv;
}

/**
 * @code dhcp_devman_start_capture(snaplen, base);
 *
 * @brief start packet capture on the devman interface list
 */
int dhcp_devman_start_capture(size_t snaplen, struct event_base *base)
{
    int rv = -1;

    if ((dhcp_num_south_intf == 1) && (dhcp_num_north_intf >= 1)) {
        rv = dhcp_device_start_capture(snaplen, base, dual_tor_mode ? loopback_ip : vlan_ip);
        if (rv != 0) {
            syslog(LOG_ALERT, "Capturing DHCP packets on interface failed");
            exit(1);
        }
    }
    else {
        syslog(LOG_ERR, "Invalid number of interfaces, downlink/south %d, uplink/north %d\n",
               dhcp_num_south_intf, dhcp_num_north_intf);
    }

    return rv;
}

/**
 * @code dhcp_devman_get_status(check_type, context);
 *
 * @brief collects DHCP relay status info.
 */
dhcp_mon_status_t dhcp_devman_get_status(dhcp_mon_check_t check_type, dhcp_device_context_t *context)
{
    return dhcp_device_get_status(check_type, context);
}

/**
 * @code dhcp_devman_update_snapshot(context);
 *
 * @brief Update device/interface counters snapshot
 */
void dhcp_devman_update_snapshot(dhcp_device_context_t *context)
{
    if (context == NULL) {
        for (auto &itr : intfs) {
            dhcp_device_update_snapshot(itr.second->dev_context);
        }
        dhcp_device_update_snapshot(dhcp_devman_get_agg_dev());
    } else {
        dhcp_device_update_snapshot(context);
    }
}

/**
 * @code dhcp_devman_print_status(context, type);
 *
 * @brief prints status counters to syslog, if context is null, it prints status counters for all interfaces
 */
void dhcp_devman_print_status(dhcp_device_context_t *context, dhcp_counters_type_t type)
{
    if (context == NULL) {
        for (auto &itr : intfs) {
            dhcp_device_print_status(itr.second->dev_context, type);
        }
        dhcp_device_print_status(dhcp_devman_get_agg_dev(), type);
    } else {
        dhcp_device_print_status(context, type);
    }
}
