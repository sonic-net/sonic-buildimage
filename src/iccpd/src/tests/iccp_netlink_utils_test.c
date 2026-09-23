#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>

#include "../../include/iccp_netlink_utils.h"

struct test_attr
{
    struct rtattr attr;
    uint8_t data[32];
};

static void init_attr(struct test_attr *attr, unsigned short type,
                      size_t payload_size, uint8_t value)
{
    memset(attr, 0, sizeof(*attr));
    attr->attr.rta_type = type;
    attr->attr.rta_len = RTA_LENGTH(payload_size);
    memset(attr->data, value, payload_size);
}

static void test_attr_payload_size(void)
{
    struct test_attr attr;

    init_attr(&attr, NDA_DST, 4, 0x11);
    assert(iccp_netlink_attr_payload_is(&attr.attr, 4));
    assert(!iccp_netlink_attr_payload_is(&attr.attr, 3));
    assert(!iccp_netlink_attr_payload_is(&attr.attr, 5));

    attr.attr.rta_len = sizeof(struct rtattr) - 1;
    assert(!iccp_netlink_attr_payload_is(&attr.attr, 0));
    assert(!iccp_netlink_attr_payload_is(NULL, 4));
}

static void test_parse_rtattrs(void)
{
    uint8_t buffer[RTA_SPACE(4) + RTA_SPACE(6)] = {0};
    struct rtattr *tb[NDA_MAX + 1];
    struct rtattr *dst = (struct rtattr *)buffer;
    struct rtattr *lladdr =
        (struct rtattr *)(buffer + RTA_SPACE(4));

    dst->rta_type = NDA_DST;
    dst->rta_len = RTA_LENGTH(4);
    memset(RTA_DATA(dst), 0x22, 4);
    lladdr->rta_type = NDA_LLADDR;
    lladdr->rta_len = RTA_LENGTH(6);
    memset(RTA_DATA(lladdr), 0x33, 6);

    assert(iccp_netlink_parse_rtattrs(tb, NDA_MAX, dst,
                                     sizeof(buffer)) == 0);
    assert(tb[NDA_DST] == dst);
    assert(tb[NDA_LLADDR] == lladdr);

    dst->rta_len = sizeof(buffer) + 1;
    assert(iccp_netlink_parse_rtattrs(tb, NDA_MAX, dst,
                                     sizeof(buffer)) == -EINVAL);
    /* On failure no entry may reference an out-of-bounds attribute. */
    assert(tb[NDA_DST] == NULL);
    assert(tb[NDA_LLADDR] == NULL);

    dst->rta_len = RTA_LENGTH(4);
    assert(iccp_netlink_parse_rtattrs(tb, NDA_MAX, dst,
                                     sizeof(buffer) - 1) == -EINVAL);

    /* A final attribute may be unpadded, but only if it consumes the
     * remaining length exactly. */
    lladdr->rta_type = NDA_LLADDR;
    lladdr->rta_len = RTA_LENGTH(6);
    assert(RTA_ALIGN(RTA_LENGTH(6)) > RTA_LENGTH(6));
    assert(iccp_netlink_parse_rtattrs(tb, NDA_MAX, dst,
                                     RTA_SPACE(4) + RTA_LENGTH(6)) == 0);
    assert(tb[NDA_DST] == dst);
    assert(tb[NDA_LLADDR] == lladdr);

    assert(iccp_netlink_parse_rtattrs(tb, NDA_MAX, dst,
                                     RTA_SPACE(4) + RTA_LENGTH(6) - 1) == -EINVAL);
}

static void test_ipv4_neighbor_attrs(void)
{
    struct test_attr dst;
    struct test_attr lladdr;
    struct rtattr *tb[NDA_MAX + 1] = {0};
    uint32_t address = 0;
    uint8_t mac[ETHER_ADDR_LEN] = {0};
    uint8_t expected_mac[ETHER_ADDR_LEN];

    init_attr(&dst, NDA_DST, sizeof(address), 0x44);
    init_attr(&lladdr, NDA_LLADDR, sizeof(mac), 0x55);
    tb[NDA_DST] = &dst.attr;
    tb[NDA_LLADDR] = &lladdr.attr;
    memset(expected_mac, 0x55, sizeof(expected_mac));

    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == 0);
    assert(address == UINT32_C(0x44444444));
    assert(memcmp(mac, expected_mac, sizeof(mac)) == 0);

    dst.attr.rta_len = RTA_LENGTH(sizeof(address) - 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == -EINVAL);
    dst.attr.rta_len = RTA_LENGTH(sizeof(address) + 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == -EINVAL);
    dst.attr.rta_len = RTA_LENGTH(sizeof(address));

    lladdr.attr.rta_len = RTA_LENGTH(sizeof(mac) - 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == -EINVAL);
    lladdr.attr.rta_len = RTA_LENGTH(sizeof(mac) + 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == -EINVAL);

    tb[NDA_LLADDR] = NULL;
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 0, tb, &address,
                                             sizeof(address), mac) == -EINVAL);

    /*
     * Delete with no NDA_LLADDR is accepted: the kernel omits the attribute
     * for entries that never resolved. The MAC is left as the caller
     * supplied it.
     */
    memset(mac, 0, sizeof(mac));
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 1, tb, &address,
                                             sizeof(address), mac) == 0);
    assert(mac[0] == 0 && memcmp(mac, mac + 1, sizeof(mac) - 1) == 0);

    /*
     * Delete with a well-formed NDA_LLADDR keeps the MAC, so the delete
     * synced to the peer carries the real address instead of zeroes.
     */
    lladdr.attr.rta_len = RTA_LENGTH(sizeof(mac));
    tb[NDA_LLADDR] = &lladdr.attr;
    memset(mac, 0, sizeof(mac));
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 1, tb, &address,
                                             sizeof(address), mac) == 0);
    assert(memcmp(mac, expected_mac, sizeof(mac)) == 0);

    /* A malformed NDA_LLADDR is still rejected on the delete path. */
    lladdr.attr.rta_len = RTA_LENGTH(sizeof(mac) + 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 1, tb, &address,
                                             sizeof(address), mac) == -EINVAL);
    lladdr.attr.rta_len = RTA_LENGTH(sizeof(mac) - 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET, 1, tb, &address,
                                             sizeof(address), mac) == -EINVAL);
}

static void test_ipv6_neighbor_attrs(void)
{
    struct test_attr dst;
    struct test_attr lladdr;
    struct rtattr *tb[NDA_MAX + 1] = {0};
    uint32_t address[4] = {0};
    uint8_t mac[ETHER_ADDR_LEN] = {0};

    init_attr(&dst, NDA_DST, sizeof(address), 0x66);
    init_attr(&lladdr, NDA_LLADDR, sizeof(mac), 0x77);
    tb[NDA_DST] = &dst.attr;
    tb[NDA_LLADDR] = &lladdr.attr;

    assert(iccp_netlink_parse_neighbor_attrs(AF_INET6, 0, tb, address,
                                             sizeof(address), mac) == 0);

    dst.attr.rta_len = RTA_LENGTH(sizeof(address) - 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET6, 0, tb, address,
                                             sizeof(address), mac) == -EINVAL);
    dst.attr.rta_len = RTA_LENGTH(sizeof(address) + 1);
    assert(iccp_netlink_parse_neighbor_attrs(AF_INET6, 0, tb, address,
                                             sizeof(address), mac) == -EINVAL);

    assert(iccp_netlink_parse_neighbor_attrs(AF_UNSPEC, 0, tb, address,
                                             sizeof(address), mac) ==
           -EAFNOSUPPORT);
}

int main(void)
{
    test_attr_payload_size();
    test_parse_rtattrs();
    test_ipv4_neighbor_attrs();
    test_ipv6_neighbor_attrs();
    return 0;
}
