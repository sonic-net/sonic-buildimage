#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "../include/iccp_netlink_utils.h"

int iccp_netlink_parse_rtattrs(struct rtattr *tb[], int max,
                               struct rtattr *rta, int len)
{
    int aligned_len;
    unsigned short type;

    if (!tb || max < 0 || len < 0)
        return -EINVAL;

    memset(tb, 0, sizeof(*tb) * (max + 1));

    while (len > 0)
    {
        if (len < (int)sizeof(*rta) ||
            rta->rta_len < sizeof(*rta) ||
            rta->rta_len > len)
            return -EINVAL;

        type = rta->rta_type;
        if (type <= max && !tb[type])
            tb[type] = rta;

        aligned_len = RTA_ALIGN(rta->rta_len);
        if (aligned_len > len)
        {
            if (rta->rta_len != len)
                return -EINVAL;
            len = 0;
        }
        else
        {
            len -= aligned_len;
            rta = (struct rtattr *)((char *)rta + aligned_len);
        }
    }

    return 0;
}

int iccp_netlink_attr_payload_is(const struct rtattr *attr, size_t size)
{
    if (!attr || attr->rta_len < RTA_LENGTH(0))
        return 0;

    return (size_t)RTA_PAYLOAD(attr) == size;
}

int iccp_netlink_parse_neighbor_attrs(int family, int is_delete,
                                      struct rtattr *tb[], void *address,
                                      size_t address_size,
                                      uint8_t mac_addr[ETHER_ADDR_LEN])
{
    size_t expected_address_size;

    if (!tb || !address || !mac_addr)
        return -EINVAL;

    if (family == AF_INET)
        expected_address_size = sizeof(uint32_t);
    else if (family == AF_INET6)
        expected_address_size = sizeof(uint32_t) * 4;
    else
        return -EAFNOSUPPORT;

    if (address_size != expected_address_size ||
        !iccp_netlink_attr_payload_is(tb[NDA_DST], expected_address_size))
        return -EINVAL;

    memcpy(address, RTA_DATA(tb[NDA_DST]), expected_address_size);

    /*
     * NDA_LLADDR is mandatory when learning a neighbor and optional on
     * delete, where the kernel omits it for entries that never resolved.
     * When the kernel does supply it on a delete we keep it, so the delete
     * synced to the MCLAG peer carries the real MAC rather than zeroes.
     * Either way the payload must be exactly ETHER_ADDR_LEN.
     */
    if (!tb[NDA_LLADDR])
        return is_delete ? 0 : -EINVAL;

    if (!iccp_netlink_attr_payload_is(tb[NDA_LLADDR], ETHER_ADDR_LEN))
        return -EINVAL;

    memcpy(mac_addr, RTA_DATA(tb[NDA_LLADDR]), ETHER_ADDR_LEN);
    return 0;
}
