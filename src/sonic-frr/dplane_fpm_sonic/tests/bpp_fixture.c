/* SPDX-License-Identifier: GPL-2.0-or-later */
/* Getter-backed unit fixture; this is not a running zebra provider. */
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>
#include <stdint.h>
#include <netinet/in.h>
#define EVPN_MH_NHG_MAX_MEMBERS 510
#define IS_IPADDR_V4(a) ((a)->type == AF_INET)
#define IS_IPADDR_V6(a) ((a)->type == AF_INET6)
struct ipaddr {
	int type;
	union {
		struct in_addr ipaddr_v4;
		struct in6_addr ipaddr_v6;
	};
};
struct ethaddr {
	uint8_t octet[6];
};
typedef struct {
	uint8_t val[10];
} esi_t;
enum dplane_op_e {
	DPLANE_OP_EVPN_MH_NH_UPDATE = 1,
	DPLANE_OP_EVPN_MH_NHG_UPDATE = 2,
	DPLANE_OP_EVPN_MH_REPLAY_DONE = 3
};
struct dplane_evpn_mh_member {
	uint32_t nh_id;
	uint8_t type;
	struct ipaddr vtep;
	struct ethaddr rmac;
};
struct zebra_dplane_ctx {
	int op;
	bool deleting;
	uint32_t id;
	uint8_t type;
	struct ipaddr vtep;
	struct ethaddr rmac;
	uint32_t count;
	struct dplane_evpn_mh_member *members;
	esi_t esi;
	unsigned nulls;
};
static enum dplane_op_e dplane_ctx_get_op(const struct zebra_dplane_ctx *c)
{
	return c->op;
}
static bool dplane_ctx_get_evpn_mh_is_del(const struct zebra_dplane_ctx *c)
{
	return c->deleting;
}
static uint32_t dplane_ctx_get_evpn_mh_nh_id(const struct zebra_dplane_ctx *c)
{
	return c->id;
}
static uint32_t dplane_ctx_get_evpn_mh_nhg_id(const struct zebra_dplane_ctx *c)
{
	return c->id;
}
static uint8_t dplane_ctx_get_evpn_mh_nh_type(const struct zebra_dplane_ctx *c)
{
	return c->type;
}
static const struct ipaddr *
dplane_ctx_get_evpn_mh_vtep(const struct zebra_dplane_ctx *c)
{
	return c->nulls & 1 ? NULL : &c->vtep;
}
static const struct ethaddr *
dplane_ctx_get_evpn_mh_rmac(const struct zebra_dplane_ctx *c)
{
	return c->nulls & 2 ? NULL : &c->rmac;
}
static uint32_t
dplane_ctx_get_evpn_mh_member_count(const struct zebra_dplane_ctx *c)
{
	return c->count;
}
static const struct dplane_evpn_mh_member *
dplane_ctx_get_evpn_mh_members(const struct zebra_dplane_ctx *c)
{
	return c->members;
}
static const esi_t *dplane_ctx_get_evpn_mh_esi(const struct zebra_dplane_ctx *c)
{
	return c->nulls & 4 ? NULL : &c->esi;
}
#include "../bridge_port_protection.h"
ssize_t encode(const struct zebra_dplane_ctx *ctx, uint8_t *buffer,
	       size_t capacity)
{
	return sonic_bpp_encode(ctx, buffer, capacity);
}

ssize_t binding(uint32_t ifindex, uint32_t group_id, bool deleting,
		uint8_t *buffer, size_t capacity)
{
	return sonic_bpp_binding(ifindex, group_id, deleting, buffer, capacity);
}
