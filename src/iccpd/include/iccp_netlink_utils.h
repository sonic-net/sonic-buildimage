#ifndef ICCP_NETLINK_UTILS_H
#define ICCP_NETLINK_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <linux/rtnetlink.h>

#ifndef ETHER_ADDR_LEN
#define ETHER_ADDR_LEN 6
#endif

int iccp_netlink_parse_rtattrs(struct rtattr *tb[], int max,
                               struct rtattr *rta, int len);
int iccp_netlink_attr_payload_is(const struct rtattr *attr, size_t size);
int iccp_netlink_parse_neighbor_attrs(int family, int is_delete,
                                      struct rtattr *tb[], void *address,
                                      size_t address_size,
                                      uint8_t mac_addr[ETHER_ADDR_LEN]);

#endif
