#ifndef __NET_GENL_PACKET_H
#define __NET_GENL_PACKET_H

#include <uapi/linux/genl-packet.h>

extern void genl_packet_send_packet(struct net *net, struct sk_buff *skb,
							 int in_ifindex, int out_ifindex, unsigned int context);

#endif /* __NET_GENL_PACKET_H */
