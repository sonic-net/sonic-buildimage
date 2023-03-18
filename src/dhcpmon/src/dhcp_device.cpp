/**
 * @file dhcp_device.c
 *
 *  device (interface) module
 */

#include <err.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <syslog.h>
#include <libexplain/ioctl.h>
#include <linux/filter.h>
#include <netpacket/packet.h>
#include "subscriberstatetable.h"
#include "select.h"

#include "dhcp_devman.h"
#include "dhcp_device.h"

/** Counter print width */
#define DHCP_COUNTER_WIDTH  9

/** Start of Ether header of a captured frame */
#define ETHER_START_OFFSET  0
/** Start of IP header of a captured frame */
#define IP_START_OFFSET (ETHER_START_OFFSET + ETHER_HDR_LEN)
/** Start of UDP header of a captured frame */
#define UDP_START_OFFSET (IP_START_OFFSET + sizeof(struct ip))
/** Start of DHCP header of a captured frame */
#define DHCP_START_OFFSET (UDP_START_OFFSET + sizeof(struct udphdr))
/** Start of DHCP Options segment of a captured frame */
#define DHCP_OPTIONS_HEADER_SIZE 240
/** Offset of DHCP GIADDR */
#define DHCP_GIADDR_OFFSET 24
#define CLIENT_IF_PREFIX "Ethernet"

#define OP_LDHA     (BPF_LD  | BPF_H   | BPF_ABS)   /** bpf ldh Abs */
#define OP_LDHI     (BPF_LD  | BPF_H   | BPF_IND)   /** bpf ldh Ind */
#define OP_LDB      (BPF_LD  | BPF_B   | BPF_ABS)   /** bpf ldb Abs*/
#define OP_JEQ      (BPF_JMP | BPF_JEQ | BPF_K)     /** bpf jeq */
#define OP_JGT      (BPF_JMP | BPF_JGT | BPF_K)     /** bpf jgt */
#define OP_RET      (BPF_RET | BPF_K)               /** bpf ret */
#define OP_JSET     (BPF_JMP | BPF_JSET | BPF_K)    /** bpf jset */
#define OP_LDXB     (BPF_LDX | BPF_B    | BPF_MSH)  /** bpf ldxb */

std::shared_ptr<swss::DBConnector> mConfigDbPtr = std::make_shared<swss::DBConnector> ("CONFIG_DB", 0);
std::shared_ptr<swss::DBConnector> mStateDbPtr = std::make_shared<swss::DBConnector> ("STATE_DB", 0);
std::shared_ptr<swss::Table> mStateDbMuxTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(), "HW_MUX_CABLE_TABLE"
        );

/* interface to vlan mapping */
std::unordered_map<std::string, std::string> vlan_map;

/* interface to port-channel mapping */
std::unordered_map<std::string, std::string> portchan_map;

/* interface to mgmt port mapping */
std::unordered_map<std::string, std::string> mgmt_map;

/** Berkeley Packet Filter program for "udp and (port 67 or port 68)".
 * This program is obtained using the following command tcpdump:
 * `tcpdump -dd "outbound and udp and (port 67 or port 68)"`
 */
static struct sock_filter dhcp_outbound_bpf_code[] = {
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0xfffff004}, // (000) ldh      #fffff004
    {.code = OP_JEQ,  .jt = 0,  .jf = 22, .k = 0x00000004}, // (001) jeq      #0x04            jt 0 jf 22
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x0000000c}, // (002) ldh      [12]
    {.code = OP_JEQ,  .jt = 0,  .jf = 7,  .k = 0x000086dd}, // (003) jeq      #0x86dd          jt 2	jf 9
    {.code = OP_LDB,  .jt = 0,  .jf = 0,  .k = 0x00000014}, // (004) ldb      [20]
    {.code = OP_JEQ,  .jt = 0,  .jf = 18, .k = 0x00000011}, // (005) jeq      #0x11            jt 4	jf 22
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000036}, // (006) ldh      [54]
    {.code = OP_JEQ,  .jt = 15, .jf = 0,  .k = 0x00000043}, // (007) jeq      #0x43            jt 21	jf 6
    {.code = OP_JEQ,  .jt = 14, .jf = 0,  .k = 0x00000044}, // (008) jeq      #0x44            jt 21	jf 7
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000038}, // (009) ldh      [56]
    {.code = OP_JEQ,  .jt = 12, .jf = 11, .k = 0x00000043}, // (010) jeq      #0x43            jt 21	jf 20
    {.code = OP_JEQ,  .jt = 0,  .jf = 12, .k = 0x00000800}, // (011) jeq      #0x800           jt 10	jf 22
    {.code = OP_LDB,  .jt = 0,  .jf = 0,  .k = 0x00000017}, // (012) ldb      [23]
    {.code = OP_JEQ,  .jt = 0,  .jf = 10, .k = 0x00000011}, // (013) jeq      #0x11            jt 12	jf 22
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000014}, // (014) ldh      [20]
    {.code = OP_JSET, .jt = 8,  .jf = 0,  .k = 0x00001fff}, // (015) jset     #0x1fff          jt 22	jf 14
    {.code = OP_LDXB, .jt = 0,  .jf = 0,  .k = 0x0000000e}, // (016) ldxb     4*([14]&0xf)
    {.code = OP_LDHI, .jt = 0,  .jf = 0,  .k = 0x0000000e}, // (017) ldh      [x + 14]
    {.code = OP_JEQ,  .jt = 4,  .jf = 0,  .k = 0x00000043}, // (018) jeq      #0x43            jt 21	jf 17
    {.code = OP_JEQ,  .jt = 3,  .jf = 0,  .k = 0x00000044}, // (019) jeq      #0x44            jt 21	jf 18
    {.code = OP_LDHI, .jt = 0,  .jf = 0,  .k = 0x00000010}, // (020) ldh      [x + 16]
    {.code = OP_JEQ,  .jt = 1,  .jf = 0,  .k = 0x00000043}, // (021) jeq      #0x43            jt 21	jf 20
    {.code = OP_JEQ,  .jt = 0,  .jf = 1,  .k = 0x00000044}, // (022) jeq      #0x44            jt 21	jf 22
    {.code = OP_RET,  .jt = 0,  .jf = 0,  .k = 0x00040000}, // (023) ret      #262144
    {.code = OP_RET,  .jt = 0,  .jf = 0,  .k = 0x00000000}, // (024) ret      #0
};

/** Berkeley Packet Filter program for "udp and (port 67 or port 68)".
 * This program is obtained using the following command tcpdump:
 * `tcpdump -dd "inbound and udp and (port 67 or port 68)"`
 */
static struct sock_filter dhcp_inbound_bpf_code[] = {
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0xfffff004}, // (000) ldh      #fffff004
    {.code = OP_JEQ,  .jt = 22, .jf = 0, .k = 0x00000004},  // (001) jeq      #0x04            jt 22 jf 0
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x0000000c}, // (002) ldh      [12]
    {.code = OP_JEQ,  .jt = 0,  .jf = 7,  .k = 0x000086dd}, // (003) jeq      #0x86dd          jt 2	jf 9
    {.code = OP_LDB,  .jt = 0,  .jf = 0,  .k = 0x00000014}, // (004) ldb      [20]
    {.code = OP_JEQ,  .jt = 0,  .jf = 18, .k = 0x00000011}, // (005) jeq      #0x11            jt 4	jf 22
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000036}, // (006) ldh      [54]
    {.code = OP_JEQ,  .jt = 15, .jf = 0,  .k = 0x00000043}, // (007) jeq      #0x43            jt 21	jf 6
    {.code = OP_JEQ,  .jt = 14, .jf = 0,  .k = 0x00000044}, // (008) jeq      #0x44            jt 21	jf 7
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000038}, // (009) ldh      [56]
    {.code = OP_JEQ,  .jt = 12, .jf = 11, .k = 0x00000043}, // (010) jeq      #0x43            jt 21	jf 20
    {.code = OP_JEQ,  .jt = 0,  .jf = 12, .k = 0x00000800}, // (011) jeq      #0x800           jt 10	jf 22
    {.code = OP_LDB,  .jt = 0,  .jf = 0,  .k = 0x00000017}, // (012) ldb      [23]
    {.code = OP_JEQ,  .jt = 0,  .jf = 10, .k = 0x00000011}, // (013) jeq      #0x11            jt 12	jf 22
    {.code = OP_LDHA, .jt = 0,  .jf = 0,  .k = 0x00000014}, // (014) ldh      [20]
    {.code = OP_JSET, .jt = 8,  .jf = 0,  .k = 0x00001fff}, // (015) jset     #0x1fff          jt 22	jf 14
    {.code = OP_LDXB, .jt = 0,  .jf = 0,  .k = 0x0000000e}, // (016) ldxb     4*([14]&0xf)
    {.code = OP_LDHI, .jt = 0,  .jf = 0,  .k = 0x0000000e}, // (017) ldh      [x + 14]
    {.code = OP_JEQ,  .jt = 4,  .jf = 0,  .k = 0x00000043}, // (018) jeq      #0x43            jt 21	jf 17
    {.code = OP_JEQ,  .jt = 3,  .jf = 0,  .k = 0x00000044}, // (019) jeq      #0x44            jt 21	jf 18
    {.code = OP_LDHI, .jt = 0,  .jf = 0,  .k = 0x00000010}, // (020) ldh      [x + 16]
    {.code = OP_JEQ,  .jt = 1,  .jf = 0,  .k = 0x00000043}, // (021) jeq      #0x43            jt 21	jf 20
    {.code = OP_JEQ,  .jt = 0,  .jf = 1,  .k = 0x00000044}, // (022) jeq      #0x44            jt 21	jf 22
    {.code = OP_RET,  .jt = 0,  .jf = 0,  .k = 0x00040000}, // (023) ret      #262144
    {.code = OP_RET,  .jt = 0,  .jf = 0,  .k = 0x00000000}, // (024) ret      #0
};

/** Filter program socket struct */
static struct sock_fprog dhcp_outbound_sock_bfp = {
    .len = sizeof(dhcp_outbound_bpf_code) / sizeof(*dhcp_outbound_bpf_code), .filter = dhcp_outbound_bpf_code
};
static struct sock_fprog dhcp_inbound_sock_bfp = {
    .len = sizeof(dhcp_inbound_bpf_code) / sizeof(*dhcp_inbound_bpf_code), .filter = dhcp_inbound_bpf_code
};

static uint8_t *rx_recv_buffer = NULL;
static uint8_t *tx_recv_buffer = NULL;
static uint32_t snap_length;

/** Aggregate device of DHCP interfaces. It contains aggregate counters from
    all interfaces
 */
static dhcp_device_context_t aggregate_dev = {0};

/** Monitored DHCP message type */
static dhcp_message_type_t monitored_msgs[] = {
    DHCP_MESSAGE_TYPE_DISCOVER,
    DHCP_MESSAGE_TYPE_OFFER,
    DHCP_MESSAGE_TYPE_REQUEST,
    DHCP_MESSAGE_TYPE_ACK
};

/** update ethernet interface to vlan map
 *  VLAN_MEMBER|Vlan1000|Ethernet48
 */
void update_vlan_mapping(std::shared_ptr<swss::DBConnector> db_conn) {
    auto match_pattern = std::string("VLAN_MEMBER|*");
    auto keys = db_conn->keys(match_pattern);
    for (auto &itr : keys) {
        auto first = itr.find_first_of('|');
        auto second = itr.find_last_of('|');
        auto vlan = itr.substr(first + 1, second - first - 1);
        auto interface = itr.substr(second + 1);
        vlan_map[interface] = vlan;
        syslog(LOG_INFO, "add <%s, %s> into interface vlan map\n", interface.c_str(), vlan.c_str());
    }
}

/** update ethernet interface to port-channel map
 *  PORTCHANNEL_MEMBER|PortChannel101|Ethernet112
 */
void update_portchannel_mapping(std::shared_ptr<swss::DBConnector> db_conn) {
    auto match_pattern = std::string("PORTCHANNEL_MEMBER|*");
    auto keys = db_conn->keys(match_pattern);
    for (auto &itr : keys) {
        auto first = itr.find_first_of('|');
        auto second = itr.find_last_of('|');
        auto portchannel = itr.substr(first + 1, second - first - 1);
        auto interface = itr.substr(second + 1);
        portchan_map[interface] = portchannel;
        syslog(LOG_INFO, "add <%s, %s> into interface port-channel map\n", interface.c_str(), portchannel.c_str());
    }
}

/** update interface to mgmt map
 */
void update_mgmt_mapping() {
    auto mgmt = dhcp_devman_get_mgmt_dev();
    if (mgmt) {
        auto name = std::string(mgmt->intf);
        mgmt_map[name] = name;
    }
}

dhcp_device_context_t *find_device_context(std::unordered_map<std::string, struct intf*> *intfs, std::string if_name) {
    auto intf = intfs->find(if_name);
    if (intf == intfs->end()) {
        return NULL;
    }
    return intf->second->dev_context;
}

/** Number of monitored DHCP message type */
static uint8_t monitored_msg_sz = sizeof(monitored_msgs) / sizeof(*monitored_msgs);

/**
 * @code handle_dhcp_option_53(context, dhcp_option, dir, iphdr, dhcphdr);
 *
 * @brief handle the logic related to DHCP option 53
 *
 * @param context       Device (interface) context
 * @param dhcp_option   pointer to DHCP option buffer space
 * @param dir           packet direction
 * @param iphdr         pointer to packet IP header
 * @param dhcphdr       pointer to DHCP header
 *
 * @return none
 */
static void handle_dhcp_option_53(dhcp_device_context_t *context,
                                  const u_char *dhcp_option,
                                  dhcp_packet_direction_t dir,
                                  struct ip *iphdr,
                                  uint8_t *dhcphdr)
{
    in_addr_t giaddr;
    switch (dhcp_option[2])
    {
    // DHCP messages send by client
    case DHCP_MESSAGE_TYPE_DISCOVER:
    case DHCP_MESSAGE_TYPE_REQUEST:
    case DHCP_MESSAGE_TYPE_DECLINE:
    case DHCP_MESSAGE_TYPE_RELEASE:
    case DHCP_MESSAGE_TYPE_INFORM:
        giaddr = ntohl(dhcphdr[DHCP_GIADDR_OFFSET] << 24 | dhcphdr[DHCP_GIADDR_OFFSET + 1] << 16 |
                       dhcphdr[DHCP_GIADDR_OFFSET + 2] << 8 | dhcphdr[DHCP_GIADDR_OFFSET + 3]);
        if ((context->giaddr_ip == giaddr && context->is_uplink && dir == DHCP_TX) ||
            (!context->is_uplink && dir == DHCP_RX && iphdr->ip_dst.s_addr == INADDR_BROADCAST)) {
            context->counters[DHCP_COUNTERS_CURRENT][dir][dhcp_option[2]]++;
            aggregate_dev.counters[DHCP_COUNTERS_CURRENT][dir][dhcp_option[2]]++;
        }
        break;
    // DHCP messages send by server
    case DHCP_MESSAGE_TYPE_OFFER:
    case DHCP_MESSAGE_TYPE_ACK:
    case DHCP_MESSAGE_TYPE_NAK:
        if ((context->giaddr_ip == iphdr->ip_dst.s_addr && context->is_uplink && dir == DHCP_RX) ||
            (!context->is_uplink && dir == DHCP_TX)) {
            context->counters[DHCP_COUNTERS_CURRENT][dir][dhcp_option[2]]++;
            aggregate_dev.counters[DHCP_COUNTERS_CURRENT][dir][dhcp_option[2]]++;
        }
        break;
    default:
        syslog(LOG_WARNING, "handle_dhcp_option_53(%s): Unknown DHCP option 53 type %d", context->intf, dhcp_option[2]);
        break;
    }
}

/**
 * @code client_packet_handler(dhcp_device_context_t *context, ssize_t buffer_sz);
 *
 * @brief packet handler to process received rx and tx packets
 *
 * @param context       pointer to device (interface) context
 * @param buffer_sz     buffer that stores received packet data
 *
 * @return none
 */
static void client_packet_handler(dhcp_device_context_t *context, uint8_t *buffer,
                                  ssize_t buffer_sz, dhcp_packet_direction_t dir)
{
    struct ip *iphdr = (struct ip*) (buffer + IP_START_OFFSET);
    struct udphdr *udp = (struct udphdr*) (buffer + UDP_START_OFFSET);
    uint8_t *dhcphdr = buffer + DHCP_START_OFFSET;
    int dhcp_option_offset = DHCP_START_OFFSET + DHCP_OPTIONS_HEADER_SIZE;

    if (((unsigned)buffer_sz > UDP_START_OFFSET + sizeof(struct udphdr) + DHCP_OPTIONS_HEADER_SIZE) &&
        (ntohs(udp->len) > DHCP_OPTIONS_HEADER_SIZE))
    {
        int dhcp_sz = ntohs(udp->len) < buffer_sz - UDP_START_OFFSET - sizeof(struct udphdr) ?
                    ntohs(udp->len) : buffer_sz - UDP_START_OFFSET - sizeof(struct udphdr);
        int dhcp_option_sz = dhcp_sz - DHCP_OPTIONS_HEADER_SIZE;
        const u_char *dhcp_option = buffer + dhcp_option_offset;
        
        int offset = 0;
        while ((offset < (dhcp_option_sz + 1)) && dhcp_option[offset] != 255) {
            if (dhcp_option[offset] == OPTION_DHCP_MESSAGE_TYPE) {
                if (offset < (dhcp_option_sz + 2)) {
                    handle_dhcp_option_53(context, &dhcp_option[offset], dir, iphdr, dhcphdr);
                }
                break; // break while loop since we are only interested in Option 53
            }

            if (dhcp_option[offset] == 0) { // DHCP Option Padding
                offset++;
            } else {
                offset += dhcp_option[offset + 1] + 2;
            }
        }
    } else {
        syslog(LOG_WARNING, "read_callback(%s %s): read length (%ld) is too small to capture DHCP options",
               context->intf, dir == DHCP_TX ? "TX" : "RX", buffer_sz);
    }
}

static dhcp_device_context_t *interface_to_dev_context(std::unordered_map<std::string, struct intf*> *devices,
                                                       std::string ifname)
{
    auto vlan = vlan_map.find(ifname);
    if (vlan != vlan_map.end()) {
        if (dual_tor_sock) {
            std::string state;
            mStateDbMuxTablePtr->hget(ifname, "state", state);
            if (state == "standby") {
                return NULL;
            }
        }
        return find_device_context(devices, vlan->second);
    } else {
        auto port_channel = portchan_map.find(ifname);
        if (port_channel != portchan_map.end()) {
            return find_device_context(devices, port_channel->second);
        }
        else {
            // mgmt interface check
            auto mgmt = mgmt_map.find(ifname);
            if (mgmt != mgmt_map.end()) {
                return find_device_context(devices, mgmt->second);
            }
        }
    }
    return NULL;
}


/**
 * @code read_tx_callback(fd, event, arg);
 *
 * @brief callback for libevent which is called every time out in order to read queued outgoing packet capture
 *
 * @param fd            socket to read from
 * @param event         libevent triggered event
 * @param arg           user provided argument for callback (interface context)
 *
 * @return none
 */
static void read_tx_callback(int fd, short event, void *arg)
{
    auto devices = (std::unordered_map<std::string, struct intf*> *)arg;
    ssize_t buffer_sz;
    struct sockaddr_ll sll;
    socklen_t slen = sizeof sll;
    dhcp_device_context_t *context = NULL;

    while ((buffer_sz = recvfrom(fd, tx_recv_buffer, snap_length, MSG_DONTWAIT, (struct sockaddr *)&sll, &slen)) > 0) 
    {
        char interfaceName[IF_NAMESIZE];
        if (if_indextoname(sll.sll_ifindex, interfaceName) == NULL) {
            syslog(LOG_WARNING, "invalid output interface index %d\n", sll.sll_ifindex);
            continue;
        }
        std::string intf(interfaceName);
        context = find_device_context(devices, intf);
        if (context) {
            client_packet_handler(context, tx_recv_buffer, buffer_sz, DHCP_TX);
        }
    }
}

/**
 * @code read_rx_callback(fd, event, arg);
 *
 * @brief callback for libevent which is called every time out in order to read queued incoming packet capture
 *
 * @param fd            socket to read from
 * @param event         libevent triggered event
 * @param arg           user provided argument for callback (interface context)
 *
 * @return none
 */
static void read_rx_callback(int fd, short event, void *arg)
{
    auto devices = (std::unordered_map<std::string, struct intf*> *)arg;
    ssize_t buffer_sz;
    struct sockaddr_ll sll;
    socklen_t slen = sizeof(sll);
    dhcp_device_context_t *context = NULL;

    while ((buffer_sz = recvfrom(fd, rx_recv_buffer, snap_length, MSG_DONTWAIT, (struct sockaddr *)&sll, &slen)) > 0) 
    {
        char interfaceName[IF_NAMESIZE];
        if (if_indextoname(sll.sll_ifindex, interfaceName) == NULL) {
            syslog(LOG_WARNING, "invalid input interface index %d\n", sll.sll_ifindex);
            continue;
        }
        std::string intf(interfaceName);
        context = interface_to_dev_context(devices, intf);
        if (context) {
            client_packet_handler(context, rx_recv_buffer, buffer_sz, DHCP_RX);
        }
    }
}

/**
 * @code dhcp_device_is_dhcp_inactive(counters);
 *
 * @brief Check if there were no DHCP activity
 *
 * @param counters  current/snapshot counter
 *
 * @return true if there were no DHCP activity, false otherwise
 */
static bool dhcp_device_is_dhcp_inactive(uint64_t counters[][DHCP_DIR_COUNT][DHCP_MESSAGE_TYPE_COUNT])
{
    uint64_t *rx_counters = counters[DHCP_COUNTERS_CURRENT][DHCP_RX];
    uint64_t *rx_counter_snapshot = counters[DHCP_COUNTERS_SNAPSHOT][DHCP_RX];

    bool rv = true;
    for (uint8_t i = 0; (i < monitored_msg_sz) && rv; i++) {
        rv = rx_counters[monitored_msgs[i]] == rx_counter_snapshot[monitored_msgs[i]];
    }

    return rv;
}

/**
 * @code dhcp_device_is_dhcp_msg_unhealthy(type, counters);
 *
 * @brief Check if DHCP relay is functioning properly for message of type 'type'.
 *        For every rx of message 'type', there should be increment of the same message type.
 *
 * @param type      DHCP message type
 * @param counters  current/snapshot counter
 *
 * @return true if DHCP message 'type' is transmitted,false otherwise
 */
static bool dhcp_device_is_dhcp_msg_unhealthy(dhcp_message_type_t type,
                                              uint64_t counters[][DHCP_DIR_COUNT][DHCP_MESSAGE_TYPE_COUNT])
{
    // check if DHCP message 'type' is being relayed
    return ((counters[DHCP_COUNTERS_CURRENT][DHCP_RX][type] >  counters[DHCP_COUNTERS_SNAPSHOT][DHCP_RX][type]) &&
            (counters[DHCP_COUNTERS_CURRENT][DHCP_TX][type] <= counters[DHCP_COUNTERS_SNAPSHOT][DHCP_TX][type])    );
}

/**
 * @code dhcp_device_check_positive_health(counters, counters_snapshot);
 *
 * @brief Check if DHCP relay is functioning properly for monitored messages (Discover, Offer, Request, ACK.)
 *        For every rx of monitored messages, there should be increment of the same message type.
 *
 * @param counters  current/snapshot counter
 *
 * @return DHCP_MON_STATUS_HEALTHY, DHCP_MON_STATUS_UNHEALTHY, or DHCP_MON_STATUS_INDETERMINATE
 */
static dhcp_mon_status_t dhcp_device_check_positive_health(uint64_t counters[][DHCP_DIR_COUNT][DHCP_MESSAGE_TYPE_COUNT])
{
    dhcp_mon_status_t rv = DHCP_MON_STATUS_HEALTHY;

    bool is_dhcp_unhealthy = false;
    for (uint8_t i = 0; (i < monitored_msg_sz) && !is_dhcp_unhealthy; i++) {
        is_dhcp_unhealthy = dhcp_device_is_dhcp_msg_unhealthy(monitored_msgs[i], counters);
    }

    // if we have rx DORA then we should have corresponding tx DORA (DORA being relayed)
    if (is_dhcp_unhealthy) {
        rv = DHCP_MON_STATUS_UNHEALTHY;
    }

    return rv;
}

/**
 * @code dhcp_device_check_negative_health(counters);
 *
 * @brief Check that DHCP relayed messages are not being transmitted out of this interface/dev
 *        using its counters. The interface is negatively healthy if there are not DHCP message
 *        travelling through it.
 *
 * @param counters              recent interface counter
 * @param counters_snapshot     snapshot counters
 *
 * @return DHCP_MON_STATUS_HEALTHY, DHCP_MON_STATUS_UNHEALTHY, or DHCP_MON_STATUS_INDETERMINATE
 */
static dhcp_mon_status_t dhcp_device_check_negative_health(uint64_t counters[][DHCP_DIR_COUNT][DHCP_MESSAGE_TYPE_COUNT])
{
    dhcp_mon_status_t rv = DHCP_MON_STATUS_HEALTHY;

    uint64_t *tx_counters = counters[DHCP_COUNTERS_CURRENT][DHCP_TX];
    uint64_t *tx_counter_snapshot = counters[DHCP_COUNTERS_SNAPSHOT][DHCP_TX];

    bool is_dhcp_unhealthy = false;
    for (uint8_t i = 0; (i < monitored_msg_sz) && !is_dhcp_unhealthy; i++) {
        is_dhcp_unhealthy = tx_counters[monitored_msgs[i]] > tx_counter_snapshot[monitored_msgs[i]];
    }

    // for negative validation, return unhealthy if DHCP packet are being
    // transmitted out of the device/interface
    if (is_dhcp_unhealthy) {
        rv = DHCP_MON_STATUS_UNHEALTHY;
    }

    return rv;
}

/**
 * @code dhcp_device_check_health(check_type, counters, counters_snapshot);
 *
 * @brief Check that DHCP relay is functioning properly given a check type. Positive check
 *        indicates for every rx of DHCP message of type 'type', there would increment of
 *        the corresponding TX of the same message type. While negative check indicates the
 *        device should not be actively transmitting any DHCP messages. If it does, it is
 *        considered unhealthy.
 *
 * @param check_type    type of health check
 * @param counters      current/snapshot counter
 *
 * @return DHCP_MON_STATUS_HEALTHY, DHCP_MON_STATUS_UNHEALTHY, or DHCP_MON_STATUS_INDETERMINATE
 */
static dhcp_mon_status_t dhcp_device_check_health(dhcp_mon_check_t check_type,
                                                  uint64_t counters[][DHCP_DIR_COUNT][DHCP_MESSAGE_TYPE_COUNT])
{
    dhcp_mon_status_t rv = DHCP_MON_STATUS_HEALTHY;

    if (dhcp_device_is_dhcp_inactive(aggregate_dev.counters)) {
        rv = DHCP_MON_STATUS_INDETERMINATE;
    } else if (check_type == DHCP_MON_CHECK_POSITIVE) {
        rv = dhcp_device_check_positive_health(counters);
    } else if (check_type == DHCP_MON_CHECK_NEGATIVE) {
        rv = dhcp_device_check_negative_health(counters);
    }

    return rv;
}

/**
 * @code dhcp_print_counters(vlan_intf, type, counters);
 *
 * @brief prints DHCP counters to sylsog.
 *
 * @param vlan_intf vlan interface name
 * @param type      counter type
 * @param counters  interface counter
 *
 * @return none
 */
static void dhcp_print_counters(const char *vlan_intf,
                                dhcp_counters_type_t type,
                                uint64_t counters[][DHCP_MESSAGE_TYPE_COUNT])
{
    static const char *counter_desc[DHCP_COUNTERS_COUNT] = {
        [DHCP_COUNTERS_CURRENT] = " Current",
        [DHCP_COUNTERS_SNAPSHOT] = "Snapshot"
    };

    syslog(
        LOG_NOTICE,
        "[%*s-%*s rx/tx] Discover: %*lu/%*lu, Offer: %*lu/%*lu, Request: %*lu/%*lu, ACK: %*lu/%*lu\n",
        IF_NAMESIZE, vlan_intf,
        (int) strlen(counter_desc[type]), counter_desc[type],
        DHCP_COUNTER_WIDTH, counters[DHCP_RX][DHCP_MESSAGE_TYPE_DISCOVER],
        DHCP_COUNTER_WIDTH, counters[DHCP_TX][DHCP_MESSAGE_TYPE_DISCOVER],
        DHCP_COUNTER_WIDTH, counters[DHCP_RX][DHCP_MESSAGE_TYPE_OFFER],
        DHCP_COUNTER_WIDTH, counters[DHCP_TX][DHCP_MESSAGE_TYPE_OFFER],
        DHCP_COUNTER_WIDTH, counters[DHCP_RX][DHCP_MESSAGE_TYPE_REQUEST],
        DHCP_COUNTER_WIDTH, counters[DHCP_TX][DHCP_MESSAGE_TYPE_REQUEST],
        DHCP_COUNTER_WIDTH, counters[DHCP_RX][DHCP_MESSAGE_TYPE_ACK],
        DHCP_COUNTER_WIDTH, counters[DHCP_TX][DHCP_MESSAGE_TYPE_ACK]
    );
}

/**
 * @code init_socket();
 *
 * @brief initializes rx/tx sockets, bind it to interface and bpf program
 *
 * @return 0 on success, otherwise for failure
 */
static int init_socket()
{
    int rv = -1;

    do {
        auto rx_sock = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
        auto tx_sock = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, htons(ETH_P_ALL));
        if (rx_sock < 0 || tx_sock < 0) {
            syslog(LOG_ALERT, "socket: failed to open socket with '%s'\n", strerror(errno));
            exit(1);
        }

        struct sockaddr_ll rx_addr;
        memset(&rx_addr, 0, sizeof(rx_addr));
        rx_addr.sll_ifindex = 0; // any interface
        rx_addr.sll_family = AF_PACKET;
        rx_addr.sll_protocol = htons(ETH_P_ALL);
        if (bind(rx_sock, (struct sockaddr *) &rx_addr, sizeof(rx_addr))) {
            syslog(LOG_ALERT, "bind: failed to bind to all interface with '%s'\n", strerror(errno));
            break;
        }

        struct sockaddr_ll tx_addr;
        memset(&tx_addr, 0, sizeof(tx_addr));
        tx_addr.sll_ifindex = 0; // any interface
        tx_addr.sll_family = AF_PACKET;
        tx_addr.sll_protocol = htons(ETH_P_ALL);
        if (bind(tx_sock, (struct sockaddr *) &tx_addr, sizeof(tx_addr))) {
            syslog(LOG_ALERT, "bind: failed to bind to interface with '%s'\n", strerror(errno));
            exit(1);
        }

        for (auto &itr : intfs) {
            itr.second->dev_context->rx_sock = rx_sock;
            itr.second->dev_context->tx_sock = tx_sock;
        }
        rv = 0;
    } while (0);

    return rv;
}

static void init_recv_buffers(int snaplen)
{
    snap_length = snaplen;
    rx_recv_buffer = (uint8_t *) malloc(snaplen);
    if (rx_recv_buffer == NULL) {
        syslog(LOG_ALERT, "malloc: failed to allocate memory for socket rx buffer '%s'\n", strerror(errno));
        exit(1);
    }

    tx_recv_buffer = (uint8_t *) malloc(snaplen);
    if (tx_recv_buffer == NULL) {
        syslog(LOG_ALERT, "malloc: failed to allocate memory for socket tx buffer '%s'\n", strerror(errno));
        exit(1);
    }
}

/**
 * @code initialize_intf_mac_and_ip_addr(context);
 *
 * @brief initializes device (interface) mac/ip addresses
 *
 * @param context           pointer to device (interface) context
 *
 * @return 0 on success, otherwise for failure
 */
int initialize_intf_mac_and_ip_addr(dhcp_device_context_t *context)
{
    int rv = -1;

    do {
        int fd;
        struct ifreq ifr;
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
            syslog(LOG_ALERT, "socket: %s", strerror(errno));
            break;
        }

        ifr.ifr_addr.sa_family = AF_INET;
        strncpy(ifr.ifr_name, context->intf, sizeof(ifr.ifr_name) - 1);
        ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

        // Get network address
        if (ioctl(fd, SIOCGIFADDR, &ifr) == -1) {
            syslog(LOG_ALERT, "ioctl: %s", explain_ioctl(fd, SIOCGIFADDR, &ifr));
            break;
        }
        context->ip = ((struct sockaddr_in*) &ifr.ifr_addr)->sin_addr.s_addr;

        // Get mac address
        if (ioctl(fd, SIOCGIFHWADDR, &ifr) == -1) {
            syslog(LOG_ALERT, "ioctl: %s", explain_ioctl(fd, SIOCGIFHWADDR, &ifr));
            break;
        }
        memcpy(context->mac, ifr.ifr_hwaddr.sa_data, sizeof(context->mac));

        close(fd);

        rv = 0;
    } while (0);

    return rv;
}

/**
 * @code dhcp_device_get_ip(context);
 *
 * @brief Accessor method
 *
 * @param context       pointer to device (interface) context
 *
 * @return interface IP
 */
int dhcp_device_get_ip(dhcp_device_context_t *context, in_addr_t *ip)
{
    int rv = -1;

    if (context != NULL && ip != NULL) {
        *ip = context->ip;
        rv = 0;
    }

    return rv;
}

/**
 * @code dhcp_device_get_aggregate_context();
 *
 * @brief Accessor method
 *
 * @return pointer to aggregate device (interface) context
 */
dhcp_device_context_t* dhcp_device_get_aggregate_context()
{
    return &aggregate_dev;
}

/**
 * @code dhcp_device_init(context, intf, is_uplink);
 *
 * @brief initializes device (interface) that handles packet capture per interface.
 */
int dhcp_device_init(dhcp_device_context_t **context, const char *intf, uint8_t is_uplink)
{
    int rv = -1;
    dhcp_device_context_t *dev_context = NULL;

    if ((context != NULL) && (strlen(intf) < sizeof(dev_context->intf))) {
        dev_context = (dhcp_device_context_t *) malloc(sizeof(dhcp_device_context_t));
        if (dev_context != NULL) {
            // set device name
            strncpy(dev_context->intf, intf, sizeof(dev_context->intf) - 1);
            dev_context->intf[sizeof(dev_context->intf) - 1] = '\0';
            // set device meta data
            if (initialize_intf_mac_and_ip_addr(dev_context) == 0) {
                dev_context->is_uplink = is_uplink;
                memset(dev_context->counters, 0, sizeof(dev_context->counters));
                *context = dev_context;
                rv = 0;
            }
        }
        else {
            syslog(LOG_ALERT, "malloc: failed to allocated device context memory for '%s'", dev_context->intf);
        }
    }

    return rv;
}

/**
 * @code dhcp_device_start_capture(snaplen, base, giaddr_ip);
 *
 * @brief starts packet capture on this interface
 */
int dhcp_device_start_capture(size_t snaplen, struct event_base *base, in_addr_t giaddr_ip)
{
    int rv = -1;
    struct event *rx_ev;
    struct event *tx_ev;
    int rx_sock = -1, tx_sock = -1;

    do {
        if (snaplen < UDP_START_OFFSET + sizeof(struct udphdr) + DHCP_OPTIONS_HEADER_SIZE) {
            syslog(LOG_ALERT, "dhcp_device_start_capture: snap length is too low to capture DHCP options");
            exit(1);
        }

        init_socket();

        init_recv_buffers(snaplen);

        update_vlan_mapping(mConfigDbPtr);
        update_portchannel_mapping(mConfigDbPtr);
        update_mgmt_mapping();

        for (auto &itr : intfs) {
            itr.second->dev_context->snaplen = snaplen;
            itr.second->dev_context->giaddr_ip = giaddr_ip;
            // all interface dev context has same rx/tx socket
            rx_sock = itr.second->dev_context->rx_sock;
            tx_sock = itr.second->dev_context->tx_sock;
        }

        if (rx_sock == -1 || tx_sock == -1) {
            syslog(LOG_ALERT, "dhcp_device_start_capture: invalid rx_sock or tx_sock");
            exit(1);
        }
        if (setsockopt(rx_sock, SOL_SOCKET, SO_ATTACH_FILTER, &dhcp_inbound_sock_bfp, sizeof(dhcp_inbound_sock_bfp)) != 0) {
            syslog(LOG_ALERT, "setsockopt: failed to attach filter with '%s'\n", strerror(errno));
            exit(1);
        }

        if (setsockopt(tx_sock, SOL_SOCKET, SO_ATTACH_FILTER, &dhcp_outbound_sock_bfp, sizeof(dhcp_outbound_sock_bfp)) != 0) {
            syslog(LOG_ALERT, "setsockopt: failed to attach filter with '%s'\n", strerror(errno));
            exit(1);
        }

        rx_ev = event_new(base, rx_sock, EV_READ | EV_PERSIST, read_rx_callback, &intfs);
        tx_ev = event_new(base, tx_sock, EV_READ | EV_PERSIST, read_tx_callback, &intfs);

        if (rx_ev == NULL || tx_ev == NULL) {
            syslog(LOG_ALERT, "event_new: failed to allocate memory for libevent event '%s'\n", strerror(errno));
            exit(1);
        }
        event_add(rx_ev, NULL);
        event_add(tx_ev, NULL);

        rv = 0;
    } while (0);

    return rv;
}

/**
 * @code dhcp_device_shutdown(context);
 *
 * @brief shuts down device (interface). Also, stops packet capture on interface and cleans up any allocated memory
 */
void dhcp_device_shutdown(dhcp_device_context_t *context)
{
    free(context);
}

/**
 * @code dhcp_device_get_status(check_type, context);
 *
 * @brief collects DHCP relay status info for a given interface. If context is null, it will report aggregate
 *        status
 */
dhcp_mon_status_t dhcp_device_get_status(dhcp_mon_check_t check_type, dhcp_device_context_t *context)
{
    dhcp_mon_status_t rv = DHCP_MON_STATUS_HEALTHY;

    if (context != NULL) {
        rv = dhcp_device_check_health(check_type, context->counters);
    }

    return rv;
}

/**
 * @code dhcp_device_update_snapshot(context);
 *
 * @brief Update device/interface counters snapshot
 */
void dhcp_device_update_snapshot(dhcp_device_context_t *context)
{
    if (context != NULL) {
        memcpy(context->counters[DHCP_COUNTERS_SNAPSHOT],
               context->counters[DHCP_COUNTERS_CURRENT],
               sizeof(context->counters[DHCP_COUNTERS_SNAPSHOT]));
    }
}

/**
 * @code dhcp_device_print_status(context, type);
 *
 * @brief prints status counters to syslog.
 */
void dhcp_device_print_status(dhcp_device_context_t *context, dhcp_counters_type_t type)
{
    if (context != NULL) {
        dhcp_print_counters(context->intf, type, context->counters[type]);
    }
}
