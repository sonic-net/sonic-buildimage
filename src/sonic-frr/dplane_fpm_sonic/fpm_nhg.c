/*
 * FPM dplane NHG object model implementation: Merkle hashing, id
 * allocation, index maintenance, and raw NHGFIB message construction.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; see the file COPYING; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h" /* Include this explicitly */
#endif

#include <linux/nexthop.h>
#include <linux/rtnetlink.h>

#include <string.h>

#include "lib/zebra.h"
#include "lib/memory.h"
#include "lib/sha256.h"
#include "lib/jhash.h"
#include "lib/mpls.h"
#include "lib/srv6.h"
#include "lib/command.h"
#include "lib/json.h"
#include "lib/vty.h"
#include "lib/vrf.h"
#include "lib/frr_pthread.h" /* frr_with_mutex */
#include "zebra/rib.h" /* DECLARE_MGROUP(ZEBRA) */
#include "zebra/debug.h" /* IS_ZEBRA_DEBUG_FPM */
#include "zebra/zebra_srv6.h" /* zebra_srv6_get_default() */
#include "zebra/zebra_vrf.h" /* zebra_vrf_lookup_by_* */
#include "zebra/rt_netlink.h"

#include "zebra/fpm_nhg.h"
#include <nexthopgroup/c_nexthopgroupfull.h>
#include <nexthopgroup/c-api/nexthopgroup_capi.h>

DEFINE_MTYPE_STATIC(ZEBRA, FPM_NHG, "FPM dplane nexthop group");
DEFINE_MTYPE_STATIC(ZEBRA, FPM_NHG_FRAME, "FPM pending output frame");

/* Help string for the show command (kept local, as in dplane_fpm_nl.c). */
#define FPM_STR "Forwarding Plane Manager configuration\n"

/* Scratch size for encoding one NHGFIB message before the framing guard. */
#define FPM_NHG_MSG_BUF_SIZE 65536

/* NHGFIB-only extension messages and JSON payload attribute. */
enum fpm_nhg_nlmsg_type {
	RTM_NEWNHGFIB = 5000,
	RTM_DELNHGFIB = 5001,
};

enum fpm_nhg_nlattr_type {
	FPM_NHA_JSON_STR = 2,
};

/*
 * All object lifecycle tracing goes through here so a single `debug zebra fpm`
 * turns the whole dplane NHG layer verbose, and so no call site pays for
 * argument formatting while the flag is off.
 */
#define FPM_NHG_DEBUG(...)                                                     \
	do {                                                                   \
		if (IS_ZEBRA_DEBUG_FPM)                                        \
			zlog_debug("nhg-fib: " __VA_ARGS__);                    \
	} while (0)

static void fpm_nhg_ref(struct fpm_dplane_nhg *obj);
static void fpm_nhg_unref(struct fpm_nhg_tables *t,
			  struct fpm_dplane_nhg *obj,
			  struct fpm_nhg_del_queue *delq);
static void fpm_nhg_record_rib_id(struct fpm_nhg_tables *t,
				  struct fpm_dplane_nhg *obj, uint32_t rib_id);
static void fpm_nhg_release_rib_id(struct fpm_nhg_tables *t,
				   struct fpm_dplane_nhg *obj, uint32_t rib_id);

static const char *fpm_nhg_level_str(uint8_t level)
{
	switch (level) {
	case FPM_NHG_L_C:
		return "L-C";
	case FPM_NHG_L_B:
		return "L-B";
	case FPM_NHG_L_A:
		return "L-A";
	default:
		return "L-?";
	}
}

void fpm_nhg_route_key_init(struct fpm_nhg_route_key *key, uint32_t table_id,
			    const struct prefix *dest, const struct prefix *src)
{
	memset(key, 0, sizeof(*key));
	key->table_id = table_id;
	key->afi = dest->family == AF_INET6 ? AFI_IP6 : AFI_IP;
	prefix_copy(&key->p, dest);
	if (src)
		prefix_copy(&key->src_p, src);
}

/* Bytes per encoded group member: child hash u64 + weight u16 BE */
#define FPM_NHG_CHILD_ENC_LEN 10
/* Group header: level u8 + masked nhg_flags u32 */
#define FPM_NHG_HDR_ENC_LEN 5
/* Resolved prefix: family u8 + prefixlen u8 + 16 addr bytes zero-padded */
#define FPM_NHG_PFX_ENC_LEN 18

static uint64_t fpm_nhg_digest(const void *buf, size_t len)
{
	unsigned char d[32];
	SHA256_CTX ctx;
	uint64_t out = 0;
	int i;

	SHA256_Init(&ctx);
	SHA256_Update(&ctx, buf, len);
	SHA256_Final(d, &ctx);
	for (i = 0; i < 8; i++)
		out = (out << 8) | d[i];
	return out;
}

/*
 * Canonical leaf identity key: the exact bytes fpm_nhg_hash_leaf()
 * digests. Kept as a named struct so a hash-table hit can be verified
 * bit-for-bit against the candidate nexthop (collision handling) —
 * correctness never depends on hash uniqueness.
 */
struct fpm_nhg_leaf_key {
	vrf_id_t vrf;
	uint8_t type, bh;
	int32_t ifindex;
	union g_addr gate;
	/* src/rmap_src are emitted in the NHGFIB JSON, so they are part of
	 * leaf identity: leaves differing only there must not dedupe.
	 */
	union g_addr src;
	union g_addr rmap_src;
	uint32_t flags_subset; /* NEXTHOP_FLAGS_HASHED bits only */
	uint8_t label_type, nlabels;
	mpls_label_t labels[MPLS_MAX_LABELS];
	uint32_t seg6local_action;
	/* seg6local_context copied memberwise: the source struct
	 * carries interior padding (inside flv) whose content is
	 * unspecified wire data and must not reach the digest.
	 */
	struct in_addr s6l_nh4;
	struct in6_addr s6l_nh6;
	uint32_t s6l_table;
	int32_t s6l_ifindex;
	uint32_t s6l_flv_ops;
	uint8_t s6l_lcblock_len, s6l_lcnode_func_len;
	uint8_t s6l_block_len, s6l_node_len;
	uint8_t s6l_function_len, s6l_argument_len;
	uint8_t encap_behavior;
	uint8_t nseg;
	struct in6_addr segs[SRV6_MAX_SIDS];
};

static void fpm_nhg_leaf_key_fill(struct fpm_nhg_leaf_key *k,
				  const struct nexthop *nh)
{
	memset(k, 0, sizeof(*k)); /* deterministic padding */
	k->vrf = nh->vrf_id;
	k->type = nh->type;
	k->ifindex = nh->ifindex;
	if (nh->type == NEXTHOP_TYPE_BLACKHOLE)
		k->bh = nh->bh_type;
	else
		k->gate = nh->gate;
	k->src = nh->src;
	k->rmap_src = nh->rmap_src;
	k->flags_subset = nh->flags & NEXTHOP_FLAGS_HASHED;
	k->label_type = nh->nh_label_type;
	if (nh->nh_label) {
		k->nlabels = MIN(nh->nh_label->num_labels, MPLS_MAX_LABELS);
		memcpy(k->labels, nh->nh_label->label,
		       k->nlabels * sizeof(mpls_label_t));
	}
	if (nh->nh_srv6) {
		const struct seg6local_context *ctx =
			&nh->nh_srv6->seg6local_ctx;

		k->seg6local_action = nh->nh_srv6->seg6local_action;
		k->s6l_nh4 = ctx->nh4;
		k->s6l_nh6 = ctx->nh6;
		k->s6l_table = ctx->table;
		k->s6l_ifindex = ctx->ifindex;
		k->s6l_flv_ops = ctx->flv.flv_ops;
		k->s6l_lcblock_len = ctx->flv.lcblock_len;
		k->s6l_lcnode_func_len = ctx->flv.lcnode_func_len;
		k->s6l_block_len = ctx->block_len;
		k->s6l_node_len = ctx->node_len;
		k->s6l_function_len = ctx->function_len;
		k->s6l_argument_len = ctx->argument_len;
		if (nh->nh_srv6->seg6_segs) {
			k->encap_behavior =
				(uint8_t)nh->nh_srv6->seg6_segs->encap_behavior;
			k->nseg = MIN(nh->nh_srv6->seg6_segs->num_segs,
				      SRV6_MAX_SIDS);
			memcpy(k->segs, nh->nh_srv6->seg6_segs->seg,
			       k->nseg * sizeof(struct in6_addr));
		}
	}
}

uint64_t fpm_nhg_hash_leaf(const struct nexthop *nh)
{
	struct fpm_nhg_leaf_key k;

	fpm_nhg_leaf_key_fill(&k, nh);
	return fpm_nhg_digest(&k, sizeof(k));
}

/* Resolving vrf: vrf_id u32. Only meaningful for L-B objects, but always
 * encoded so the layout stays fixed; callers pass 0 for non-L-B levels. */
#define FPM_NHG_VRF_ENC_LEN 4

uint64_t fpm_nhg_hash_group(uint8_t level, uint32_t nhg_flags,
			    const struct fpm_nhg_child *children,
			    uint16_t count, const struct prefix *resolved,
			    vrf_id_t vrf_id)
{
	uint32_t flags = nhg_flags &
			 (FPM_NHG_FLAG_RECURSIVE | FPM_NHG_FLAG_RECEIVED);
	size_t len = FPM_NHG_HDR_ENC_LEN +
		     (size_t)count * FPM_NHG_CHILD_ENC_LEN +
		     FPM_NHG_PFX_ENC_LEN + FPM_NHG_VRF_ENC_LEN;
	uint8_t *buf, *p;
	uint64_t h;
	uint16_t i;
	int j, blen;

	/* byte-packed big-endian encode, zero-filled: no padding leaks */
	buf = XCALLOC(MTYPE_FPM_NHG, len);
	p = buf;
	*p++ = level;
	for (j = 3; j >= 0; j--)
		*p++ = (flags >> (8 * j)) & 0xff;
	for (i = 0; i < count; i++) {
		for (j = 7; j >= 0; j--)
			*p++ = (children[i].obj->hash >> (8 * j)) & 0xff;
		*p++ = (uint8_t)(children[i].weight >> 8);
		*p++ = (uint8_t)(children[i].weight & 0xff);
	}
	if (level == FPM_NHG_L_B && resolved) {
		*p++ = resolved->family;
		*p++ = (uint8_t)resolved->prefixlen;
		blen = MIN(prefix_blen(resolved), 16);
		memcpy(p, &resolved->u.prefix, blen);
	}
	/* vrf_id sits right after the 18-byte prefix block */
	p = buf + FPM_NHG_HDR_ENC_LEN +
	    (size_t)count * FPM_NHG_CHILD_ENC_LEN + FPM_NHG_PFX_ENC_LEN;
	for (j = 3; j >= 0; j--)
		*p++ = ((uint32_t)vrf_id >> (8 * j)) & 0xff;
	h = fpm_nhg_digest(buf, len);
	XFREE(MTYPE_FPM_NHG, buf);
	return h;
}

uint32_t fpm_nhg_id_alloc(struct fpm_nhg_tables *t)
{
	uint32_t id;

	if (t->free_id_count)
		return t->free_ids[--t->free_id_count];
	id = t->next_id++; /* starts at 1; 0 is the invalid sentinel */
	if (t->next_id == 0)
		t->next_id = 1; /* wrap: never hand out the 0 sentinel */
	return id;
}

void fpm_nhg_id_free(struct fpm_nhg_tables *t, uint32_t id)
{
	if (t->free_id_count == t->free_id_cap) {
		t->free_id_cap = t->free_id_cap ? t->free_id_cap * 2 : 64;
		t->free_ids = XREALLOC(MTYPE_FPM_NHG, t->free_ids,
				       t->free_id_cap * sizeof(uint32_t));
	}
	t->free_ids[t->free_id_count++] = id;
}

static unsigned int fpm_nhg_hash_key(const void *data)
{
	const struct fpm_dplane_nhg *obj = data;

	/* xor-fold the u64 Merkle hash down to the u32 bucket key */
	return (unsigned int)(obj->hash ^ (obj->hash >> 32));
}

static bool fpm_nhg_hash_cmp(const void *data1, const void *data2)
{
	return ((const struct fpm_dplane_nhg *)data1)->hash ==
	       ((const struct fpm_dplane_nhg *)data2)->hash;
}

static unsigned int fpm_nhg_id_key(const void *data)
{
	return ((const struct fpm_dplane_nhg *)data)->dplane_id;
}

static bool fpm_nhg_id_cmp(const void *data1, const void *data2)
{
	return ((const struct fpm_dplane_nhg *)data1)->dplane_id ==
	       ((const struct fpm_dplane_nhg *)data2)->dplane_id;
}

/*
 * route_nhg_map: (table_id, afi, prefix, src_p) -> top (L-A) object. Entries
 * are their own small allocation; the value is a borrowed pointer (the map's
 * refcount on the object is taken/released by the caller, not here).
 */
struct fpm_nhg_route_entry {
	struct fpm_nhg_route_key key;
	struct fpm_dplane_nhg *obj;
	/*
	 * The zebra NHG id this route carried when it was installed. Kept so
	 * the (rib id -> object) association can be released when the route
	 * goes away or re-points, which is the only way that mapping can be
	 * known to be dead: the object itself may still serve other routes.
	 */
	uint32_t rib_id;
};

static unsigned int fpm_nhg_route_key_hash(const void *data)
{
	const struct fpm_nhg_route_entry *e = data;

	/* prefix_hash_key() (lib/prefix.h) zero-normalizes the prefix bytes;
	 * it handles AF_UNSPEC (the "no source prefix" case) as well.
	 */
	return jhash_2words(e->key.table_id, e->key.afi,
			    jhash_1word(prefix_hash_key(&e->key.src_p),
					prefix_hash_key(&e->key.p)));
}

/*
 * prefix_same() has no AF_UNSPEC case, so it reports two all-zero prefixes
 * as different. A non-srcdest route carries exactly such a zeroed src_p, so
 * that case is handled explicitly here.
 */
static bool fpm_nhg_src_same(const struct prefix *a, const struct prefix *b)
{
	if (a->family != b->family || a->prefixlen != b->prefixlen)
		return false;
	if (a->family == AF_UNSPEC)
		return true;
	return prefix_same(a, b) != 0;
}

static bool fpm_nhg_route_key_cmp(const void *data1, const void *data2)
{
	const struct fpm_nhg_route_entry *a = data1, *b = data2;

	return a->key.table_id == b->key.table_id &&
	       a->key.afi == b->key.afi && prefix_same(&a->key.p, &b->key.p) &&
	       fpm_nhg_src_same(&a->key.src_p, &b->key.src_p);
}

/*
 * by_rib_id: zebra NHG id -> object. Like route_map, entries are their own
 * small allocation holding a borrowed object pointer — the index takes no
 * refcount, and every entry is released before its object is freed
 * (fpm_nhg_rib_ids_purge()).
 */
struct fpm_nhg_rib_entry {
	uint32_t rib_id;
	struct fpm_dplane_nhg *obj;
	/* Routes currently mapping this rib id onto obj. */
	uint32_t refcount;
};

static unsigned int fpm_nhg_rib_id_key(const void *data)
{
	return ((const struct fpm_nhg_rib_entry *)data)->rib_id;
}

static bool fpm_nhg_rib_id_cmp(const void *data1, const void *data2)
{
	return ((const struct fpm_nhg_rib_entry *)data1)->rib_id ==
	       ((const struct fpm_nhg_rib_entry *)data2)->rib_id;
}


/*
 * Resolving-prefix index: (vrf, resolving prefix) -> every live dplane NHG
 * object that resolved through that prefix. Buckets borrow object pointers;
 * each object is unindexed before it is freed.
 */
struct fpm_nhg_resolved_bucket {
	vrf_id_t vrf_id;
	struct prefix p;
	struct fpm_dplane_nhg **objs;
	uint32_t count, cap;
};

/*
 * by_resolved: (vrf, resolving prefix) -> bucket of objects resolving through
 * it.
 */
static unsigned int fpm_nhg_resolved_key(const void *data)
{
	const struct fpm_nhg_resolved_bucket *b = data;

	return jhash_1word(b->vrf_id, prefix_hash_key(&b->p));
}

static bool fpm_nhg_resolved_cmp(const void *data1, const void *data2)
{
	const struct fpm_nhg_resolved_bucket *a = data1, *b = data2;

	return a->vrf_id == b->vrf_id && a->p.family == b->p.family &&
	       a->p.prefixlen == b->p.prefixlen && prefix_same(&a->p, &b->p);
}

static void *fpm_nhg_resolved_alloc(void *arg)
{
	const struct fpm_nhg_resolved_bucket *ref = arg;
	struct fpm_nhg_resolved_bucket *b;

	b = XCALLOC(MTYPE_FPM_NHG, sizeof(*b));
	b->vrf_id = ref->vrf_id;
	b->p = ref->p;
	return b;
}

static void fpm_nhg_resolved_bucket_free(void *data)
{
	struct fpm_nhg_resolved_bucket *b = data;

	XFREE(MTYPE_FPM_NHG, b->objs);
	XFREE(MTYPE_FPM_NHG, b);
}

/*
 * Index `obj` under its current resolving prefix. A no-op for an object
 * without one, and idempotent: the same object is never listed twice, so a
 * refresh that does not actually move the resolution costs nothing.
 */
static void fpm_nhg_resolved_add(struct fpm_nhg_tables *t,
				 struct fpm_dplane_nhg *obj)
{
	struct fpm_nhg_resolved_bucket ref = {}, *b;
	uint32_t i;

	if (obj->resolved_prefix.family == AF_UNSPEC)
		return;

	ref.vrf_id = obj->vrf_id;
	ref.p = obj->resolved_prefix;
	b = hash_get(t->by_resolved, &ref, fpm_nhg_resolved_alloc);

	for (i = 0; i < b->count; i++)
		if (b->objs[i] == obj)
			return;

	if (b->count == b->cap) {
		b->cap = b->cap ? b->cap * 2 : 8;
		b->objs = XREALLOC(MTYPE_FPM_NHG, b->objs,
				   b->cap * sizeof(*b->objs));
	}
	b->objs[b->count++] = obj;

	FPM_NHG_DEBUG("resolved-index add %pFX vrf %u -> dplane NHG %u (%s), bucket now %u",
		      &obj->resolved_prefix, obj->vrf_id, obj->dplane_id,
		      fpm_nhg_level_str(obj->level), b->count);
}

/*
 * Drop `obj` from the bucket of `p` in `vrf_id`, releasing the bucket once it
 * holds nothing. Order inside a bucket carries no meaning, so the vacated slot
 * is filled from the tail.
 */
static void fpm_nhg_resolved_del_key(struct fpm_nhg_tables *t, vrf_id_t vrf_id,
				     const struct prefix *p,
				     struct fpm_dplane_nhg *obj)
{
	struct fpm_nhg_resolved_bucket ref = {}, *b;
	uint32_t i;

	if (p->family == AF_UNSPEC)
		return;

	ref.vrf_id = vrf_id;
	ref.p = *p;
	b = hash_lookup(t->by_resolved, &ref);
	if (!b)
		return;

	for (i = 0; i < b->count; i++) {
		if (b->objs[i] != obj)
			continue;
		b->objs[i] = b->objs[--b->count];
		FPM_NHG_DEBUG("resolved-index del %pFX vrf %u -> dplane NHG %u (%s), bucket now %u",
			      p, vrf_id, obj->dplane_id,
			      fpm_nhg_level_str(obj->level), b->count);
		break;
	}

	if (b->count == 0) {
		hash_release(t->by_resolved, b);
		fpm_nhg_resolved_bucket_free(b);
	}
}

static const struct fpm_nhg_resolved_bucket *
fpm_nhg_resolved_lookup(struct fpm_nhg_tables *t, vrf_id_t vrf_id,
			const struct prefix *p)
{
	struct fpm_nhg_resolved_bucket ref = {};

	if (p == NULL || p->family == AF_UNSPEC)
		return NULL;

	ref.vrf_id = vrf_id;
	ref.p = *p;
	return hash_lookup(t->by_resolved, &ref);
}


/*
 * Debug probe for a resolving-prefix backwalk. The caller holds the table lock
 * and supplies the route-event details so this module need not depend on a
 * dplane context.
 */
void fpm_nhg_debug_resolved_route(struct fpm_nhg_tables *t,
				  vrf_id_t vrf_id, const struct prefix *p,
				  const char *op_name)
{
	const struct fpm_nhg_resolved_bucket *b;
	char nhbuf[NEXTHOP_STRLEN];
	uint32_t i;

	b = fpm_nhg_resolved_lookup(t, vrf_id, p);
	if (b == NULL)
		return;

	if (IS_ZEBRA_DEBUG_FPM)
		zlog_debug("nhg-fib backwalk: %s %pFX vrf %u is a resolving prefix for %u dplane NHG(s) (logging only)",
			   op_name, p, vrf_id, b->count);

	for (i = 0; i < b->count; i++) {
		const struct fpm_dplane_nhg *obj = b->objs[i];

		if (!IS_ZEBRA_DEBUG_FPM)
			continue;
		if (obj->nh)
			nexthop2str(obj->nh, nhbuf, sizeof(nhbuf));
		else
			strlcpy(nhbuf, "(none)", sizeof(nhbuf));
		zlog_debug("nhg-fib backwalk:   dplane NHG %u (rib nhg %u), nexthop %s",
			   obj->dplane_id, obj->resolved_via, nhbuf);
	}
}

void fpm_nhg_tables_init(struct fpm_nhg_tables *t)
{
	memset(t, 0, sizeof(*t));
	t->by_hash = hash_create(fpm_nhg_hash_key, fpm_nhg_hash_cmp,
				 "FPM dplane NHG by hash");
	t->by_id = hash_create(fpm_nhg_id_key, fpm_nhg_id_cmp,
			       "FPM dplane NHG by id");
	t->route_map = hash_create(fpm_nhg_route_key_hash,
				   fpm_nhg_route_key_cmp,
				   "FPM dplane NHG route map");
	t->by_rib_id = hash_create(fpm_nhg_rib_id_key, fpm_nhg_rib_id_cmp,
				   "FPM dplane NHG by rib id");
	t->by_resolved = hash_create(fpm_nhg_resolved_key, fpm_nhg_resolved_cmp,
				     "FPM dplane NHG by resolving prefix");
	t->next_id = 1;
}

static struct fpm_dplane_nhg *fpm_nhg_lookup_hash(struct fpm_nhg_tables *t,
						  uint64_t hash)
{
	struct fpm_dplane_nhg dummy = { .hash = hash };

	return hash_lookup(t->by_hash, &dummy);
}

static struct fpm_dplane_nhg *fpm_nhg_lookup_id(struct fpm_nhg_tables *t,
						uint32_t id)
{
	struct fpm_dplane_nhg dummy = { .dplane_id = id };

	return hash_lookup(t->by_id, &dummy);
}

void fpm_nhg_insert(struct fpm_nhg_tables *t, struct fpm_dplane_nhg *obj)
{
	struct fpm_dplane_nhg *ret;

	ret = hash_get(t->by_hash, obj, hash_alloc_intern);
	assert(ret == obj);
	ret = hash_get(t->by_id, obj, hash_alloc_intern);
	assert(ret == obj);
}

void fpm_nhg_remove(struct fpm_nhg_tables *t, struct fpm_dplane_nhg *obj)
{
	fpm_nhg_resolved_del_key(t, obj->vrf_id, &obj->resolved_prefix, obj);
	hash_release(t->by_hash, obj);
	hash_release(t->by_id, obj);
}

static void fpm_nhg_obj_free(void *data)
{
	struct fpm_dplane_nhg *obj = data;

	/*
	 * obj->nh is one nexthop_dup() copy, never chained via ->next.
	 * nexthop_free() releases labels, srv6 data and the ->resolved
	 * chain the dup may have recursed into (lib/nexthop.c), so
	 * nexthops_free() is not needed.
	 */
	if (obj->nh)
		nexthop_free(obj->nh);
	XFREE(MTYPE_FPM_NHG, obj->rib_nhg_ids);
	XFREE(MTYPE_FPM_NHG, obj->children);
	XFREE(MTYPE_FPM_NHG, obj);
}

static void fpm_nhg_rib_entry_free(void *data)
{
	XFREE(MTYPE_FPM_NHG, data);
}

/*
 * Drop the by_rib_id entries this object still owns, before it is freed.
 *
 * An id in obj's list does not imply the index still points here: a later
 * object may have claimed that id (see fpm_nhg_record_rib_id()). Releasing
 * such an entry would erase a live mapping, so each candidate is checked
 * against the object first.
 */
static void fpm_nhg_rib_ids_purge(struct fpm_nhg_tables *t,
				  struct fpm_dplane_nhg *obj)
{
	struct fpm_nhg_rib_entry ref, *e;
	uint16_t i;

	for (i = 0; i < obj->rib_nhg_id_count; i++) {
		ref.rib_id = obj->rib_nhg_ids[i];
		e = hash_lookup(t->by_rib_id, &ref);
		if (!e || e->obj != obj)
			continue;
		hash_release(t->by_rib_id, &ref);
		XFREE(MTYPE_FPM_NHG, e);
	}
}

static void fpm_nhg_route_entry_free(void *data)
{
	XFREE(MTYPE_FPM_NHG, data);
}

void fpm_nhg_tables_flush(struct fpm_nhg_tables *t)
{
	FPM_NHG_DEBUG("flush: discarding %u dplane NHG object(s) and every binding",
		      fpm_nhg_count(t));
	/*
	 * Objects live in both tables: empty by_id without freeing, then
	 * free each object once via by_hash. The tables themselves are
	 * kept (reused after reconnect). route_map entries only hold
	 * borrowed object pointers, so they go first.
	 */
	hash_clean(t->route_map, fpm_nhg_route_entry_free);
	hash_clean(t->by_rib_id, fpm_nhg_rib_entry_free);
	hash_clean(t->by_resolved, fpm_nhg_resolved_bucket_free);
	hash_clean(t->by_id, NULL);
	hash_clean(t->by_hash, fpm_nhg_obj_free);
	XFREE(MTYPE_FPM_NHG, t->free_ids);
	t->free_id_count = 0;
	t->free_id_cap = 0;
	t->next_id = 1;
	/* Counters are lifetime totals; reconnect flush does not reset them. */
}

/*
 * Shutdown counterpart of flush: empty the tables, then destroy the table
 * structures themselves. Only for plugin teardown — after this the tables
 * must be re-inited before any further use.
 *
 * lib's hash_free() is static, so hash_clean_and_free() is the public
 * equivalent; the flush above already emptied every table, hence the NULL
 * free_func (nothing left to free) and the NULLed pointers it leaves behind.
 */
void fpm_nhg_tables_fini(struct fpm_nhg_tables *t)
{
	fpm_nhg_tables_flush(t);
	hash_clean_and_free(&t->route_map, NULL);
	hash_clean_and_free(&t->by_rib_id, NULL);
	hash_clean_and_free(&t->by_resolved, NULL);
	hash_clean_and_free(&t->by_id, NULL);
	hash_clean_and_free(&t->by_hash, NULL);
}

/*
 * Build engine: post-order DFS decomposition of a route ctx nexthop
 * tree into deduplicated dplane NHG objects. Pure
 * state — no message emission; every newly created object is pushed
 * onto the caller's staging queue in child-first order (a child is
 * always created — and thus queued — before any parent referencing
 * it, and cache hits queue nothing), which is exactly the
 * RTM_NEWNHGFIB define-before-reference order and, reversed, the
 * rollback order.
 */

static void fpm_nhg_staging_push(struct fpm_nhg_staging *s,
				 struct fpm_dplane_nhg *obj)
{
	if (s->count == s->cap) {
		/*
		 * cap doubles only when count reaches it, so cap is the
		 * smallest power of two >= count; count is bounded by the
		 * object count of one route's nexthop tree (see the struct
		 * comment), so cap never approaches 2^31.
		 */
		s->cap = s->cap ? s->cap * 2 : 16;
		s->objs = XREALLOC(MTYPE_FPM_NHG, s->objs,
				   s->cap * sizeof(*s->objs));
	}
	s->objs[s->count++] = obj;
}

void fpm_nhg_staging_free(struct fpm_nhg_staging *s)
{
	XFREE(MTYPE_FPM_NHG, s->objs);
	s->count = 0;
	s->cap = 0;
}

/*
 * Collision handling: a by_hash hit is only reused after
 * a full content comparison. On mismatch (true 64-bit collision) the
 * key is deterministically re-derived by digesting the previous key
 * plus an incrementing salt byte, and the probe repeats — both the
 * original creation and every later lookup walk the same salt
 * sequence from the same base hash, so colliding objects chain onto
 * distinct perturbed keys and are still found. Bounded: more than
 * FPM_NHG_MAX_PROBES chained collisions is beyond astronomically
 * unlikely and asserts.
 */
#define FPM_NHG_MAX_PROBES 16

static uint64_t fpm_nhg_hash_perturb(uint64_t hash, uint8_t salt)
{
	uint8_t buf[9];
	int j;

	for (j = 0; j < 8; j++)
		buf[j] = (hash >> (8 * (7 - j))) & 0xff;
	buf[8] = salt;
	return fpm_nhg_digest(buf, sizeof(buf));
}

/*
 * Probe the by_hash table starting at *hash. Returns the matching
 * object (dedupe hit), or NULL with *hash updated to the first free
 * slot on the probe sequence (creation key).
 */
static struct fpm_dplane_nhg *
fpm_nhg_probe(struct fpm_nhg_tables *t, uint64_t *hash,
	      bool (*match)(const struct fpm_dplane_nhg *, const void *),
	      const void *arg)
{
	struct fpm_dplane_nhg *obj;
	int attempt;

	for (attempt = 0; attempt < FPM_NHG_MAX_PROBES; attempt++) {
		obj = fpm_nhg_lookup_hash(t, *hash);
		if (!obj)
			return NULL;
		if (match(obj, arg))
			return obj;
		*hash = fpm_nhg_hash_perturb(*hash, (uint8_t)(attempt + 1));
	}
	assert(!"fpm_nhg: hash probe sequence exhausted");
	return NULL;
}

static bool fpm_nhg_leaf_match(const struct fpm_dplane_nhg *obj,
			       const void *arg)
{
	struct fpm_nhg_leaf_key ko, kn;

	if (obj->level != FPM_NHG_L_C || obj->num_children != 0 || !obj->nh)
		return false;
	/* compare the exact canonical bytes the leaf hash digests */
	fpm_nhg_leaf_key_fill(&ko, obj->nh);
	fpm_nhg_leaf_key_fill(&kn, arg);
	return memcmp(&ko, &kn, sizeof(ko)) == 0;
}

static struct fpm_dplane_nhg *fpm_nhg_obj_new(struct fpm_nhg_tables *t,
					      uint64_t hash, uint8_t level,
					      struct fpm_nhg_staging *newq)
{
	struct fpm_dplane_nhg *obj;

	obj = XCALLOC(MTYPE_FPM_NHG, sizeof(*obj));
	obj->hash = hash;
	obj->dplane_id = fpm_nhg_id_alloc(t);
	obj->level = level;
	fpm_nhg_insert(t, obj);
	fpm_nhg_staging_push(newq, obj);
	t->obj_created++;
	FPM_NHG_DEBUG("create dplane NHG %u (%s) hash %" PRIx64
		      ", staged, %" PRIu64 " object(s) live",
		      obj->dplane_id, fpm_nhg_level_str(level), hash,
		      t->obj_created - t->obj_deleted);
	return obj;
}

/*
 * A nexthop carrying SRv6 information the peer can actually consume.
 *
 * The guard mirrors exactly what nexthop_copy_no_recurse() (lib/nexthop.c)
 * preserves into the leaf's dup'd nexthop — a segment list only when num_segs
 * is non zero and the SIDs are not all zeroes — so the RECEIVED flag derived
 * from this predicate can never disagree with the SRv6 content the NHGFIB
 * message ends up carrying. An SRv6-less nexthop, and one whose nh_srv6 holds
 * neither a usable segment list nor a seg6local action, keep the normal
 * recursive treatment. sid_zero() asserts on NULL and dereferences seg[0],
 * hence the two checks ahead of it.
 */
static bool fpm_nhg_nh_has_srv6(const struct nexthop *nh)
{
	if (!nh->nh_srv6)
		return false;
	if (nh->nh_srv6->seg6local_action != ZEBRA_SEG6_LOCAL_ACTION_UNSPEC)
		return true;
	return nh->nh_srv6->seg6_segs && nh->nh_srv6->seg6_segs->num_segs &&
	       !sid_zero(nh->nh_srv6->seg6_segs);
}

static struct fpm_dplane_nhg *fpm_nhg_get_leaf(struct fpm_nhg_tables *t,
					       const struct nexthop *nh,
					       const struct prefix *resolved,
					       vrf_id_t resolved_vrf,
					       uint32_t resolved_via,
					       struct fpm_nhg_staging *newq)
{
	struct fpm_dplane_nhg *obj;
	uint64_t hash;

	hash = fpm_nhg_hash_leaf(nh);
	obj = fpm_nhg_probe(t, &hash, fpm_nhg_leaf_match, nh);
	if (obj) {
		t->dedupe_hits++;
		FPM_NHG_DEBUG("reuse leaf dplane NHG %u hash %" PRIx64
			      ", refcount %u",
			      obj->dplane_id, hash, obj->refcount);
		/*
		 * Refresh the resolving info on reuse. An equal leaf key means
		 * the same gate, so the caller's values describe this object's
		 * *current* resolution while the stored ones may predate a
		 * re-resolution (e.g. a /128 endpoint route withdrawn and the
		 * gate now covered by a less specific locator prefix). Identity
		 * is untouched — neither field is in the leaf key — and neither
		 * reaches the wire, so no message follows from the update.
		 */
		if (resolved && resolved->family != AF_UNSPEC) {
			if (obj->resolved_prefix.family != AF_UNSPEC &&
			    (obj->vrf_id != resolved_vrf ||
			     !prefix_same(&obj->resolved_prefix, resolved)))
				FPM_NHG_DEBUG("leaf dplane NHG %u re-resolved: %pFX vrf %u -> %pFX vrf %u",
					      obj->dplane_id,
					      &obj->resolved_prefix, obj->vrf_id,
					      resolved, resolved_vrf);
			/*
			 * The refresh can move this leaf to a different
			 * resolving prefix, so the index entry moves with it.
			 * Both calls are no-ops when nothing actually changed.
			 */
			fpm_nhg_resolved_del_key(t, obj->vrf_id,
						 &obj->resolved_prefix, obj);
			obj->resolved_prefix = *resolved;
			obj->vrf_id = resolved_vrf;
			obj->resolved_via = resolved_via;
			fpm_nhg_resolved_add(t, obj);
		}
		return obj;
	}
	obj = fpm_nhg_obj_new(t, hash, FPM_NHG_L_C, newq);
	/* leaves have no ->resolved subtree; no_recurse dup == plain dup */
	obj->nh = nexthop_dup_no_recurse(nh, NULL);
	/*
	 * An SRv6 leaf is the "received" object fpmsyncd turns into an SRv6 PIC
	 * context object (nhgmgr.cpp checkNeedCreateSonicPICObj(): SRv6 info
	 * plus RECEIVED). The flag is a pure function of the SRv6 fields the
	 * leaf key already digests, so a dedupe hit above necessarily carries
	 * the same value and needs no fixup here.
	 */
	if (fpm_nhg_nh_has_srv6(nh))
		obj->nhg_flags = FPM_NHG_FLAG_RECEIVED;
	/*
	 * Resolving info for a leaf that stands in for a recursive nexthop
	 * (the SRv6 top-level-only path): that nexthop still resolved through a
	 * prefix, and the resolved-via view records which one. Set only on
	 * creation, never patched onto a dedupe hit: the resolving prefix
	 * follows from the gate, which the leaf key already digests, so every
	 * sharer of this object resolved the same way.
	 */
	if (resolved && resolved->family != AF_UNSPEC) {
		obj->resolved_prefix = *resolved;
		obj->vrf_id = resolved_vrf;
		obj->resolved_via = resolved_via;
		fpm_nhg_resolved_add(t, obj);
	}
	return obj;
}

/* Everything the group hash covers; used for post-hit verification. */
struct fpm_nhg_group_key {
	uint8_t level;
	uint32_t nhg_flags;
	const struct fpm_nhg_child *children; /* sorted by obj->hash */
	uint16_t count;
	const struct prefix *resolved; /* non-NULL iff level == L_B */
	vrf_id_t vrf_id;
};

static bool fpm_nhg_group_match(const struct fpm_dplane_nhg *obj,
				const void *arg)
{
	const struct fpm_nhg_group_key *key = arg;
	uint16_t i;

	if (obj->level != key->level || obj->nhg_flags != key->nhg_flags ||
	    obj->num_children != key->count)
		return false;
	for (i = 0; i < key->count; i++) {
		/*
		 * Child pointer equality is the correct deep comparison:
		 * children are themselves content-deduped, so an equal
		 * subtree is the same object.
		 */
		if (obj->children[i].obj != key->children[i].obj ||
		    obj->children[i].weight != key->children[i].weight)
			return false;
	}
	if (key->level == FPM_NHG_L_B) {
		if (obj->resolved_prefix.family != key->resolved->family ||
		    obj->resolved_prefix.prefixlen != key->resolved->prefixlen)
			return false;
		if (key->resolved->family != AF_UNSPEC &&
		    !prefix_same(&obj->resolved_prefix, key->resolved))
			return false;
		if (obj->vrf_id != key->vrf_id)
			return false;
	}
	return true;
}

static int fpm_nhg_child_cmp(const void *a, const void *b)
{
	const struct fpm_nhg_child *ca = a, *cb = b;

	if (ca->obj->hash < cb->obj->hash)
		return -1;
	if (ca->obj->hash > cb->obj->hash)
		return 1;
	/* Same deduped child may appear twice with different weights;
	 * tie-break so the encoded sequence is deterministic.
	 */
	if (ca->weight < cb->weight)
		return -1;
	if (ca->weight > cb->weight)
		return 1;
	return 0;
}

/*
 * The L-B resolving info (PR #19252 struct nh_res_info) participates in the L-B
 * group hash, so it is derived BEFORE recursing and passed down — never patched
 * onto the child object after hashing.
 *
 * zebra stamps this info on the *recursive parent*: set_resolving_info()
 * (zebra/zebra_nhg.c) is called as set_resolving_info(nexthop, rn, match) right
 * after nexthop_set_resolved() allocated the child, i.e. with the parent, and
 * "show nexthop rib" prints "res via" on the recursive line for that reason.
 * The resolved child may also carry it, so the child is checked as a fallback;
 * both placements agree on the value, since one (rn, match) pair produces the
 * whole child list.
 *
 * res_info is a heap pointer and is NULL whenever zebra recorded no resolution;
 * that yields an AF_UNSPEC prefix, which the group hash treats as "unset".
 */
static const struct nh_res_info *fpm_nhg_res_info(const struct nexthop *nh)
{
	if (nh->res_info)
		return nh->res_info;
	if (nh->resolved && nh->resolved->res_info)
		return nh->resolved->res_info;
	return NULL;
}

static void fpm_nhg_resolved_prefix(const struct nexthop *nh,
				    struct prefix *p)
{
	const struct nh_res_info *info = fpm_nhg_res_info(nh);

	/* lib ipaddr2prefix() is static in prefix.c: convert manually */
	memset(p, 0, sizeof(*p));
	if (info == NULL) {
		p->family = AF_UNSPEC;
		return;
	}
	switch (info->addr.ipa_type) {
	case IPADDR_V4:
		p->family = AF_INET;
		p->u.prefix4 = info->addr.ipaddr_v4;
		break;
	case IPADDR_V6:
		p->family = AF_INET6;
		p->u.prefix6 = info->addr.ipaddr_v6;
		break;
	case IPADDR_NONE:
		p->family = AF_UNSPEC;
		break;
	}
	p->prefixlen = info->pfxlen;
}

static struct fpm_dplane_nhg *
fpm_nhg_build_group(struct fpm_nhg_tables *t, const struct nexthop *chain,
		    enum fpm_nhg_level level,
		    const struct nexthop *defining_nh,
		    const struct prefix *resolved, vrf_id_t vrf_id,
		    struct fpm_nhg_staging *newq);

/*
 * Resolve every member of chain to a child object. Same-level
 * traversal uses ->next only; ->resolved subtrees are entered
 * exclusively through the L-B recursion (never ALL_NEXTHOPS, which
 * would flatten the tree).
 *
 * A member flagged RECURSIVE but with no ->resolved chain is skipped
 * entirely: it is an unresolved recursive nexthop, i.e. nothing that can
 * be installed. Treating it as an L-C leaf would publish its (unresolved)
 * gate as a real forwarding entry. A recursive member whose whole resolved
 * chain turns out to be unresolvable (fpm_nhg_build_group() returns NULL
 * without having created anything) is skipped for exactly the same reason:
 * failing the whole context there would drop a route that may still have
 * perfectly usable other members. *count therefore reports how many
 * members were actually collected, which may be less than the member
 * count of the chain — and may be 0, which the caller turns into a failed
 * group build.
 *
 * Returns 0, or -1 if any child failed for a reason other than "nothing
 * installable" (partial creations stay queued; the caller rolls back).
 */
static int fpm_nhg_collect_children(struct fpm_nhg_tables *t,
				    const struct nexthop *chain,
				    struct fpm_nhg_child *children,
				    uint16_t *count, bool *any_recursive,
				    struct fpm_nhg_staging *newq)
{
	const struct nexthop *nh;
	struct fpm_dplane_nhg *child;
	struct prefix rp;
	uint32_t staged, rvia;
	uint16_t i = 0;

	for (nh = chain; nh; nh = nh->next) {
		if (fpm_nhg_nh_has_srv6(nh)) {
			/*
			 * SRv6: derive the top level object only. The SID list
			 * lives on this nexthop, and fpmsyncd builds its PIC
			 * context object from SRv6 info plus RECEIVED, which
			 * fpm_nhg_get_leaf() stamps here. Recursing would emit
			 * objects the consumer discards anyway: the resolved
			 * children inherit the SID list from this parent
			 * (nexthop_set_resolved(), zebra_nhg.c) but carry no
			 * RECEIVED, and nhgmgr.cpp checkNeedCreateSonicNHGObj()
			 * skips a NHG that has SRv6 info without RECEIVED.
			 *
			 * The subtree is skipped but the resolving prefix is
			 * not: a recursive SRv6 nexthop still resolved through
			 * one, and the resolved-via view records it, so it is
			 * handed to the leaf.
			 */
			memset(&rp, 0, sizeof(rp));
			rvia = 0;
			if (CHECK_FLAG(nh->flags, NEXTHOP_FLAG_RECURSIVE) &&
			    nh->resolved) {
				const struct nh_res_info *info;

				fpm_nhg_resolved_prefix(nh, &rp);
				info = fpm_nhg_res_info(nh);
				if (info)
					rvia = info->id;
			}
			child = fpm_nhg_get_leaf(t, nh, &rp, nh->vrf_id, rvia,
						 newq);
			if (!child)
				return -1;
		} else if (CHECK_FLAG(nh->flags, NEXTHOP_FLAG_RECURSIVE)) {
			if (!nh->resolved)
				continue; /* unresolved: not installable */
			fpm_nhg_resolved_prefix(nh, &rp);
			staged = newq->count;
			child = fpm_nhg_build_group(t, nh->resolved,
						    FPM_NHG_L_B, nh, &rp,
						    nh->vrf_id, newq);
			if (!child) {
				/*
				 * Objects already queued means the nested build
				 * hit a real error mid-way: propagate so the
				 * caller rolls them back. Otherwise nothing was
				 * created and this member simply has nothing
				 * installable behind it — skip it.
				 */
				if (newq->count != staged)
					return -1;
				continue;
			}
			/* only a member that contributed makes the group recursive */
			*any_recursive = true;
		} else {
			const struct nh_res_info *info;

			/*
			 * A leaf resolves too: zebra stamps the connected
			 * prefix it was reached through (e.g. "res via
			 * fc06::/120"), so record it rather than dropping it.
			 * Identity is unaffected — resolved_prefix is not in
			 * the leaf key — and nothing here reaches the wire.
			 */
			memset(&rp, 0, sizeof(rp));
			rvia = 0;
			fpm_nhg_resolved_prefix(nh, &rp);
			info = fpm_nhg_res_info(nh);
			if (info)
				rvia = info->id;
			child = fpm_nhg_get_leaf(t, nh, &rp, nh->vrf_id, rvia,
						 newq);
			if (!child)
				return -1;
		}
		children[i].obj = child;
		children[i].weight = nh->weight;
		i++;
	}
	*count = i;
	return 0;
}

static struct fpm_dplane_nhg *
fpm_nhg_group_new(struct fpm_nhg_tables *t,
		  const struct fpm_nhg_group_key *key, uint64_t hash,
		  const struct nexthop *defining_nh,
		  struct fpm_nhg_child *children,
		  struct fpm_nhg_staging *newq)
{
	struct fpm_dplane_nhg *obj;
	uint16_t i;

	obj = fpm_nhg_obj_new(t, hash, key->level, newq);
	obj->nhg_flags = key->nhg_flags;
	/*
	 * Defining nexthop: L-B = the recursive parent (JSON needs its
	 * gate/type), L-A = none (JSON multi-builder walks children).
	 * nexthop_dup_no_recurse() (lib/nexthop.h) copies scalar fields and
	 * deep-copies res_info, but not the ->resolved subtree — that
	 * subtree is already modeled as obj->children, so a recursing
	 * dup would only waste memory. NEXTHOP_FLAG_RECURSIVE stays set
	 * in the copy (JSON consumers use gate/type/flags only).
	 * nexthop_free() releases the copied res_info.
	 */
	if (defining_nh)
		obj->nh = nexthop_dup_no_recurse(defining_nh, NULL);
	if (key->level == FPM_NHG_L_B) {
		obj->resolved_prefix = *key->resolved;
		obj->vrf_id = key->vrf_id;
		/* resolving NHG id lives on the recursive parent (see
		 * fpm_nhg_res_info(), which also tolerates the child)
		 */
		if (defining_nh) {
			const struct nh_res_info *info =
				fpm_nhg_res_info(defining_nh);

			if (info)
				obj->resolved_via = info->id;
		}
		/*
		 * Creation is the only place an L-B needs indexing: the
		 * resolving prefix is part of the group hash, so a dedupe hit
		 * necessarily carries the same one.
		 */
		fpm_nhg_resolved_add(t, obj);
	}
	obj->num_children = key->count;
	obj->children = children; /* ownership transferred, sorted */
	for (i = 0; i < key->count; i++)
		fpm_nhg_ref(children[i].obj);

	if (IS_ZEBRA_DEBUG_FPM) {
		for (i = 0; i < key->count; i++)
			zlog_debug("nhg-fib: group dplane NHG %u (%s) member %u/%u: dplane NHG %u weight %u",
				   obj->dplane_id,
				   fpm_nhg_level_str(obj->level), i + 1,
				   key->count, children[i].obj->dplane_id,
				   children[i].weight);
		if (key->level == FPM_NHG_L_B)
			zlog_debug("nhg-fib: group dplane NHG %u resolves via %pFX vrf %u (rib nhg %u)",
				   obj->dplane_id, &obj->resolved_prefix,
				   obj->vrf_id, obj->resolved_via);
	}
	return obj;
}

static struct fpm_dplane_nhg *
fpm_nhg_build_group(struct fpm_nhg_tables *t, const struct nexthop *chain,
		    enum fpm_nhg_level level,
		    const struct nexthop *defining_nh,
		    const struct prefix *resolved, vrf_id_t vrf_id,
		    struct fpm_nhg_staging *newq)
{
	struct fpm_nhg_child *children;
	struct fpm_nhg_group_key key;
	struct fpm_dplane_nhg *obj;
	const struct nexthop *nh;
	bool any_recursive = false;
	uint64_t hash;
	uint16_t nmembers = 0, n = 0;

	for (nh = chain; nh; nh = nh->next)
		nmembers++;
	if (nmembers == 0)
		return NULL;
	children = XCALLOC(MTYPE_FPM_NHG, nmembers * sizeof(*children));
	if (fpm_nhg_collect_children(t, chain, children, &n, &any_recursive,
				     newq) < 0) {
		XFREE(MTYPE_FPM_NHG, children);
		return NULL;
	}
	/*
	 * Every member was skipped as not installable (unresolved recursive
	 * nexthop, or a recursive nexthop with nothing installable behind it):
	 * NULL here consistently means "nothing to install", never "error".
	 */
	if (n == 0) {
		XFREE(MTYPE_FPM_NHG, children);
		return NULL;
	}
	/* single non-recursive member at top level: L-A == L-C, no wrapper */
	if (level == FPM_NHG_L_A && n == 1 && !any_recursive) {
		obj = children[0].obj;
		XFREE(MTYPE_FPM_NHG, children);
		return obj;
	}
	qsort(children, n, sizeof(*children), fpm_nhg_child_cmp);
	key.level = level;
	/*
	 * nhg_flags feeds the group hash below, so it has to be decided here.
	 *
	 * Only L-B carries RECURSIVE. RECEIVED is deliberately NOT set on a
	 * plain-IP L-A group: to fpmsyncd RECEIVED means "pre-resolution group,
	 * do not program". nhgmgr.cpp RIBNHGEntry::checkNeedCreateSonicNHGObj()
	 * bails out for a NORMAL (non-SRv6) entry that carries it —
	 *   "NextHop %d is a received NHG without SRv6 info, skip create sonic
	 *    object." -> m_create_sonic_nhg_obj = false
	 * so the group would never get a SONiC NHG object id and every route
	 * pointing at it would fail its RTA_NH_ID lookup.
	 *
	 * The one case where the consumer wants RECEIVED is SRv6:
	 * checkNeedCreateSonicPICObj() turns SRv6 + RECEIVED into
	 * SONIC_NHG_OBJ_TYPE_NHG_WITH_SRV6_PIC_CONTEXT. That flag is carried by
	 * the SRv6 leaf itself (fpm_nhg_get_leaf()), which is the object holding
	 * the SID list the PIC context is built from; a group over several SRv6
	 * leaves inherits the PIC object from its members instead
	 * (checkNeedCreateSonicPICObj() scans them first). No group level object
	 * ever sets RECEIVED.
	 */
	key.nhg_flags = (level == FPM_NHG_L_B ? FPM_NHG_FLAG_RECURSIVE : 0);
	/*
	 * A group over SRv6 members is itself a "received" group: fpmsyncd
	 * needs RECEIVED next to the SRv6 info to build the PIC context object
	 * for it (checkNeedCreateSonicPICObj()). Only SRv6 leaves ever carry
	 * RECEIVED, so a plain-IP group still comes out flagless.
	 * Set before hashing — nhg_flags is part of group identity.
	 */
	if (level == FPM_NHG_L_A) {
		uint16_t c;

		for (c = 0; c < n; c++) {
			if (children[c].obj->nhg_flags &
			    FPM_NHG_FLAG_RECEIVED) {
				key.nhg_flags |= FPM_NHG_FLAG_RECEIVED;
				break;
			}
		}
	}
	key.children = children;
	key.count = n;
	key.resolved = (level == FPM_NHG_L_B) ? resolved : NULL;
	key.vrf_id = (level == FPM_NHG_L_B) ? vrf_id : 0;
	hash = fpm_nhg_hash_group(level, key.nhg_flags, children, n,
				  key.resolved, key.vrf_id);
	obj = fpm_nhg_probe(t, &hash, fpm_nhg_group_match, &key);
	if (obj) {
		/* cache hit stops recursion: no message, no new refs */
		t->dedupe_hits++;
		FPM_NHG_DEBUG("reuse group dplane NHG %u (%s) hash %" PRIx64
			      ", %u member(s), refcount %u",
			      obj->dplane_id, fpm_nhg_level_str(level), hash, n,
			      obj->refcount);
		XFREE(MTYPE_FPM_NHG, children);
		return obj;
	}
	return fpm_nhg_group_new(t, &key, hash, defining_nh, children, newq);
}

/*
 * Public entry: always the top-level (L-A) chain of a route ctx; the
 * L-B recursion is internal. No resolving info at the top level.
 */
struct fpm_dplane_nhg *fpm_nhg_build(struct fpm_nhg_tables *t,
				     const struct nexthop *chain,
				     struct fpm_nhg_staging *newq)
{
	return fpm_nhg_build_group(t, chain, FPM_NHG_L_A, NULL, NULL, 0,
				   newq);
}

static void fpm_nhg_ref(struct fpm_dplane_nhg *obj)
{
	obj->refcount++;
	FPM_NHG_DEBUG("ref dplane NHG %u (%s), refcount %u", obj->dplane_id,
		      fpm_nhg_level_str(obj->level), obj->refcount);
}

/*
 * Fully undo one fpm_nhg_build(): destroy every object it created, in
 * reverse (parent-first) staging order. Objects in newq hold no route
 * refs yet, so at each step the object's remaining refcount stems
 * only from later-queued (already removed) new parents — it must be 0
 * (asserted; this also verifies the child-first queue order).
 * Pre-existing children that gained +1 from a new parent are simply
 * decremented back to their original live count, never to 0.
 */
void fpm_nhg_rollback(struct fpm_nhg_tables *t, struct fpm_nhg_staging *newq)
{
	struct fpm_dplane_nhg *obj;
	uint32_t i;
	uint16_t c;

	FPM_NHG_DEBUG("rollback: destroying %u newly created object(s)",
		      newq->count);

	for (i = newq->count; i > 0; i--) {
		obj = newq->objs[i - 1];
		assert(obj->refcount == 0);
		FPM_NHG_DEBUG("rollback: destroy dplane NHG %u (%s)",
			      obj->dplane_id, fpm_nhg_level_str(obj->level));
		for (c = 0; c < obj->num_children; c++) {
			assert(obj->children[c].obj->refcount > 0);
			obj->children[c].obj->refcount--;
		}
		fpm_nhg_rib_ids_purge(t, obj);
		fpm_nhg_remove(t, obj);
		fpm_nhg_id_free(t, obj->dplane_id);
		fpm_nhg_obj_free(obj);
		t->obj_created--;
	}
	newq->count = 0;
}

/*
 * Deletion. Objects die only when their refcount reaches 0 — no GC pass, no
 * tree diffing.
 *
 * The DEL of a parent is staged BEFORE recursing into its children, so
 * the staged sequence is parent-before-child: fpmsyncd never sees a live
 * object whose depends have already been deleted.
 *
 * Staging is by value (dplane id only): the object is freed before this
 * function returns, so staging the pointer would hand the emitter freed
 * memory.
 *
 * id_free() vs DEL flush: a freed id must not be reused before its DEL has
 * been flushed. That holds even though the caller now DEFERS the staged DELs
 * to the head of its next batch, because
 *   (a) within one ctx the whole fpm_nhg_build() runs before any unref,
 *       so ids this ctx frees cannot be popped by this ctx's allocations;
 *   (b) the deferred DELs are emitted at the HEAD of the next batch, i.e.
 *       ahead of every RTM_NEWNHGFIB in it, so a later ctx that pops a
 *       recycled id from the free list necessarily emits its NEW after
 *       that DEL in the byte stream.
 */
static void fpm_nhg_del_queue_push(struct fpm_nhg_del_queue *q, uint32_t id)
{
	if (q->count == q->cap) {
		/*
		 * cap doubles only when count reaches it, so cap is the
		 * smallest power of two >= count; count is bounded by the
		 * number of live objects (see the struct comment), so cap
		 * never approaches 2^31.
		 */
		q->cap = q->cap ? q->cap * 2 : 16;
		q->ids = XREALLOC(MTYPE_FPM_NHG, q->ids,
				  q->cap * sizeof(*q->ids));
	}
	q->ids[q->count++].dplane_id = id;
}

void fpm_nhg_del_queue_free(struct fpm_nhg_del_queue *q)
{
	XFREE(MTYPE_FPM_NHG, q->ids);
	q->count = 0;
	q->cap = 0;
}

void fpm_nhg_del_queue_reset(struct fpm_nhg_del_queue *q)
{
	q->count = 0;
}

uint32_t fpm_nhg_del_queue_count(const struct fpm_nhg_del_queue *q)
{
	return q->count;
}

static void fpm_nhg_unref(struct fpm_nhg_tables *t,
			  struct fpm_dplane_nhg *obj,
			  struct fpm_nhg_del_queue *delq)
{
	uint16_t i;

	assert(obj->refcount > 0);
	if (--obj->refcount > 0) {
		FPM_NHG_DEBUG("unref dplane NHG %u (%s), refcount %u",
			      obj->dplane_id, fpm_nhg_level_str(obj->level),
			      obj->refcount);
		return;
	}

	FPM_NHG_DEBUG("unref dplane NHG %u (%s), last reference: staging DEL and destroying, %u child(ren) to follow",
		      obj->dplane_id, fpm_nhg_level_str(obj->level),
		      obj->num_children);

	fpm_nhg_del_queue_push(delq, obj->dplane_id);
	for (i = 0; i < obj->num_children; i++)
		fpm_nhg_unref(t, obj->children[i].obj, delq);
	fpm_nhg_rib_ids_purge(t, obj);
	fpm_nhg_remove(t, obj);
	fpm_nhg_id_free(t, obj->dplane_id);
	fpm_nhg_obj_free(obj);
	t->obj_deleted++;
}

/*
 * route_nhg_map accessors. The upsert flow is get-then-set: the caller
 * reads the old object first, installs the new one, then unrefs the old.
 */
static void *fpm_nhg_route_entry_alloc(void *arg)
{
	const struct fpm_nhg_route_entry *ref = arg;
	struct fpm_nhg_route_entry *e;

	e = XCALLOC(MTYPE_FPM_NHG, sizeof(*e));
	e->key = ref->key;
	return e;
}

struct fpm_dplane_nhg *fpm_nhg_route_get(struct fpm_nhg_tables *t,
					 const struct fpm_nhg_route_key *k)
{
	struct fpm_nhg_route_entry ref = { .key = *k }, *e;

	e = hash_lookup(t->route_map, &ref);
	return e ? e->obj : NULL;
}

static void fpm_nhg_route_set(struct fpm_nhg_tables *t,
			      const struct fpm_nhg_route_key *k,
			      struct fpm_dplane_nhg *obj, uint32_t rib_id)
{
	struct fpm_nhg_route_entry ref = { .key = *k }, *e;

	e = hash_get(t->route_map, &ref, fpm_nhg_route_entry_alloc);
	/*
	 * Release whatever this route mapped before overwriting it, otherwise
	 * a re-pointed or re-numbered route would leak its old association and
	 * the object would keep advertising a stale zebra NHG id.
	 */
	if (e->obj && (e->obj != obj || e->rib_id != rib_id))
		fpm_nhg_release_rib_id(t, e->obj, e->rib_id);

	if (e->obj == NULL)
		FPM_NHG_DEBUG("bind route %pFX table %u -> dplane NHG %u (%s), rib nhg %u",
			      &k->p, k->table_id, obj->dplane_id,
			      fpm_nhg_level_str(obj->level), rib_id);
	else if (e->obj != obj || e->rib_id != rib_id)
		FPM_NHG_DEBUG("rebind route %pFX table %u: dplane NHG %u (rib nhg %u) -> dplane NHG %u (rib nhg %u)",
			      &k->p, k->table_id, e->obj->dplane_id, e->rib_id,
			      obj->dplane_id, rib_id);

	e->obj = obj; /* replaces any previous object: caller unrefs the old */
	e->rib_id = rib_id;
	fpm_nhg_record_rib_id(t, obj, rib_id);
}

static struct fpm_dplane_nhg *
fpm_nhg_route_pop(struct fpm_nhg_tables *t, const struct fpm_nhg_route_key *k)
{
	struct fpm_nhg_route_entry ref = { .key = *k }, *e;
	struct fpm_dplane_nhg *obj;

	e = hash_release(t->route_map, &ref);
	if (!e)
		return NULL;
	obj = e->obj;
	FPM_NHG_DEBUG("unbind route %pFX table %u from dplane NHG %u, rib nhg %u",
		      &k->p, k->table_id, obj ? obj->dplane_id : 0, e->rib_id);
	fpm_nhg_release_rib_id(t, obj, e->rib_id);
	XFREE(MTYPE_FPM_NHG, e);
	return obj;
}

void fpm_nhg_route_commit(struct fpm_nhg_tables *t,
			  const struct fpm_nhg_route_key *k,
			  struct fpm_dplane_nhg *new_top, uint32_t rib_id,
			  struct fpm_nhg_del_queue *delq)
{
	struct fpm_dplane_nhg *old_top = fpm_nhg_route_get(t, k);

	fpm_nhg_route_set(t, k, new_top, rib_id);
	fpm_nhg_ref(new_top);
	if (old_top)
		fpm_nhg_unref(t, old_top, delq);
}

void fpm_nhg_route_forget(struct fpm_nhg_tables *t,
			  const struct fpm_nhg_route_key *k,
			  struct fpm_nhg_del_queue *delq)
{
	struct fpm_dplane_nhg *old_top = fpm_nhg_route_pop(t, k);

	if (old_top)
		fpm_nhg_unref(t, old_top, delq);
}

/*
 * zebra (rib) NHG id <-> object mapping. Every id seen referencing an object
 * is remembered on the object and indexed in by_rib_id, so the relation can be
 * walked in both directions: object -> ids for `show fpm nhg-fib`, id -> object
 * for a reverse lookup. Id 0 means "no zebra NHG" and is never recorded.
 *
 * Dedupe makes this many-to-one: several zebra NHG ids commonly share one
 * dplane object.
 */
static void *fpm_nhg_rib_entry_alloc(void *arg)
{
	const struct fpm_nhg_rib_entry *ref = arg;
	struct fpm_nhg_rib_entry *e;

	e = XCALLOC(MTYPE_FPM_NHG, sizeof(*e));
	e->rib_id = ref->rib_id;
	return e;
}

static void fpm_nhg_record_rib_id(struct fpm_nhg_tables *t,
				  struct fpm_dplane_nhg *obj, uint32_t rib_id)
{
	struct fpm_nhg_rib_entry ref = { .rib_id = rib_id }, *e;
	bool known = false;
	uint16_t i;

	if (rib_id == 0)
		return;

	for (i = 0; i < obj->rib_nhg_id_count; i++) {
		if (obj->rib_nhg_ids[i] == rib_id) {
			known = true;
			break;
		}
	}
	if (!known) {
		if (obj->rib_nhg_id_count == obj->rib_nhg_id_cap) {
			/*
			 * cap doubles only when count reaches it, so it stays
			 * the smallest power of two >= count. count is bounded
			 * by the number of distinct zebra NHG ids mapping to
			 * this object, far below the uint16 range.
			 */
			obj->rib_nhg_id_cap = obj->rib_nhg_id_cap
						      ? obj->rib_nhg_id_cap * 2
						      : 4;
			obj->rib_nhg_ids =
				XREALLOC(MTYPE_FPM_NHG, obj->rib_nhg_ids,
					 obj->rib_nhg_id_cap *
						 sizeof(*obj->rib_nhg_ids));
		}
		obj->rib_nhg_ids[obj->rib_nhg_id_count++] = rib_id;
		FPM_NHG_DEBUG("record rib nhg %u on dplane NHG %u (%s), %u id(s) mapped",
			      rib_id, obj->dplane_id,
			      fpm_nhg_level_str(obj->level),
			      obj->rib_nhg_id_count);
	}

	/*
	 * Repoint unconditionally, including when the id was already known:
	 * another object may have claimed it in between, and the recorder of
	 * the route event just processed is the current mapping.
	 */
	e = hash_get(t->by_rib_id, &ref, fpm_nhg_rib_entry_alloc);
	if (e->obj != obj) {
		if (e->obj)
			FPM_NHG_DEBUG("rib nhg %u re-pointed: dplane NHG %u -> dplane NHG %u",
				      rib_id, e->obj->dplane_id, obj->dplane_id);
		/* re-pointed at a different object: the old one keeps the id in
		 * its own list until its last route releases it below.
		 */
		e->obj = obj;
		e->refcount = 0;
	}
	e->refcount++;
}

/*
 * Undo one fpm_nhg_record_rib_id(): the route that established this
 * (rib id -> obj) mapping is gone or has re-pointed. On the last release the
 * index entry goes away and the id is dropped from the object's own list, so
 * `show fpm nhg-fib` never reports a zebra NHG that no longer refers here.
 */
static void fpm_nhg_release_rib_id(struct fpm_nhg_tables *t,
				   struct fpm_dplane_nhg *obj, uint32_t rib_id)
{
	struct fpm_nhg_rib_entry ref = { .rib_id = rib_id }, *e;
	uint16_t i;

	if (rib_id == 0 || obj == NULL)
		return;

	e = hash_lookup(t->by_rib_id, &ref);
	if (e && e->obj == obj) {
		if (e->refcount > 0)
			e->refcount--;
		if (e->refcount == 0) {
			hash_release(t->by_rib_id, &ref);
			XFREE(MTYPE_FPM_NHG, e);
		} else {
			/* still referenced by another route: keep the id */
			return;
		}
	} else if (e) {
		/* another object owns the mapping now; only drop our own list */
	}

	for (i = 0; i < obj->rib_nhg_id_count; i++) {
		if (obj->rib_nhg_ids[i] != rib_id)
			continue;
		memmove(&obj->rib_nhg_ids[i], &obj->rib_nhg_ids[i + 1],
			(obj->rib_nhg_id_count - i - 1) *
				sizeof(*obj->rib_nhg_ids));
		obj->rib_nhg_id_count--;
		FPM_NHG_DEBUG("release rib nhg %u from dplane NHG %u (%s), %u id(s) left",
			      rib_id, obj->dplane_id,
			      fpm_nhg_level_str(obj->level),
			      obj->rib_nhg_id_count);
		break;
	}
}

/* Reverse lookup: zebra NHG id -> dplane object, NULL when unmapped. */
static struct fpm_dplane_nhg *fpm_nhg_lookup_rib_id(struct fpm_nhg_tables *t,
						    uint32_t rib_id)
{
	struct fpm_nhg_rib_entry ref = { .rib_id = rib_id }, *e;

	if (rib_id == 0)
		return NULL;
	e = hash_lookup(t->by_rib_id, &ref);
	return e ? e->obj : NULL;
}

/*
 * Show helpers. The caller MUST hold the lock that serializes the tables
 * for the complete call.
 */
uint32_t fpm_nhg_count(struct fpm_nhg_tables *t)
{
	return (uint32_t)hashcount(t->by_id);
}

struct fpm_nhg_walk_state {
	const struct fpm_dplane_nhg **objs;
	uint32_t count, cap;
};

static void fpm_nhg_walk_collect(struct hash_bucket *bucket, void *arg)
{
	struct fpm_nhg_walk_state *w = arg;

	/*
	 * cap comes from hashcount() taken under the same lock as this walk,
	 * so it can never be exceeded; the guard is pure belt and braces.
	 */
	if (w->count >= w->cap)
		return;
	w->objs[w->count++] = bucket->data;
}

static int fpm_nhg_walk_id_cmp(const void *a, const void *b)
{
	const struct fpm_dplane_nhg *oa = *(const struct fpm_dplane_nhg *const *)a;
	const struct fpm_dplane_nhg *ob = *(const struct fpm_dplane_nhg *const *)b;

	if (oa->dplane_id < ob->dplane_id)
		return -1;
	if (oa->dplane_id > ob->dplane_id)
		return 1;
	return 0;
}

/*
 * hash_iterate() yields buckets in table order, i.e. arbitrary as far as the
 * dplane id is concerned. Show output has to be stable and comparable across
 * invocations, so collect the pointers first, sort them by id, and only then
 * hand them to the callback. The temporary array borrows the pointers; the
 * objects themselves stay owned by the tables.
 */
/* Callback invoked once per object, ascending dplane id. */
typedef void (*fpm_nhg_walk_cb)(const struct fpm_dplane_nhg *obj, void *arg);

static void fpm_nhg_walk(struct fpm_nhg_tables *t, fpm_nhg_walk_cb cb,
			 void *arg)
{
	struct fpm_nhg_walk_state w = {};
	uint32_t i;

	w.cap = (uint32_t)hashcount(t->by_id);
	if (w.cap == 0)
		return;

	w.objs = XCALLOC(MTYPE_FPM_NHG, w.cap * sizeof(*w.objs));
	hash_iterate(t->by_id, fpm_nhg_walk_collect, &w);
	qsort(w.objs, w.count, sizeof(*w.objs), fpm_nhg_walk_id_cmp);
	for (i = 0; i < w.count; i++)
		cb(w.objs[i], arg);
	XFREE(MTYPE_FPM_NHG, w.objs);
}

/*
 * NHGFIB C object construction. Builds the
 * C_NextHopGroupFull that libnexthopgroup serializes to the NHGFIB JSON, from
 * a derived dplane NHG object. Lives here with the object model rather than in
 * dplane_fpm_sonic.c so the wire encoding stays close to the objects it emits.
 */

/**
 * Depth first flatten of one object's member subtree into an NHGFIB
 * nh_grp_full_list, replicating zebra_nhg_nhe2grp_full_internal()
 * (zebra/zebra_nhg.c): the node itself is never written, only its members —
 * and a member that is itself a group is written with its direct member count
 * and then immediately followed by its own (recursively flattened) members.
 *
 * fpmsyncd depends on exactly this "all depths" shape: nhgmgr.cpp
 * RIBNHGEntry::getResolvedGroupFromNHGFull() builds a NORMAL group's resolved
 * set from `if (nhg.num_direct == 0)` entries of nh_grp_full_list, so an
 * L-A group listing only its immediate L-B members would resolve to an empty
 * set and never reach the leaves that carry the actual forwarding info.
 *
 * Unlike zebra, which has no edge weight available for an inner group node
 * and writes 0 there, we write the real edge weight: the consumer only reads
 * `weight` on num_direct == 0 entries, so for inner nodes it is informational.
 *
 * \param[in] obj object whose member subtree is flattened.
 * \param[out] list nh_grp_full array to fill.
 * \param[in] max capacity of list.
 * \param[in,out] index next free slot, advanced by every entry written.
 *
 * \returns false when the subtree does not fit into max entries.
 */
static bool flatten_nhgfull_members(const struct fpm_dplane_nhg *obj,
				    struct C_nh_grp_full *list, size_t max,
				    uint32_t *index)
{
	uint16_t i;

	for (i = 0; i < obj->num_children; i++) {
		const struct fpm_dplane_nhg *child = obj->children[i].obj;

		if (*index >= max)
			return false;

		list[*index].id = child->dplane_id;
		list[*index].weight = obj->children[i].weight;
		list[*index].num_direct = child->num_children;
		(*index)++;

		/* a group member is followed by its own members */
		if (child->num_children &&
		    !flatten_nhgfull_members(child, list, max, index))
			return false;
	}

	return true;
}

/**
 * Construct a C_NextHopGroupFull object from a derived dplane NHG object.
 *
 * One builder covers all three levels of the derived hierarchy: the C object
 * shape only distinguishes "has members" (L-A / L-B, encoded by
 * libnexthopgroup's multi builder) from "is a leaf" (L-C, its singleton
 * builder), and the caller picks the encoder from obj->num_children.
 *
 * Every id on the wire is a plugin allocated uint32 dplane id:
 * the object's own id and every depends / nh_grp_full_list entry.
 *
 * depends[] holds the IMMEDIATE members only (what the ctx driven builder fed
 * from dplane_ctx_get_nhe_depends()), while nh_grp_full_list holds the members
 * of ALL depths, flattened depth first — see flatten_nhgfull_members().
 *
 * @param c_nhg object to fill (fully overwritten).
 * @param obj derived dplane NHG object.
 * @param grp_full_count set to the number of nh_grp_full_list entries written
 *		(the flattened member count, >= obj->num_children).
 * @return false when obj's members do not fit into the C arrays.
 */
static bool build_c_nhgfull_from_obj(struct C_NextHopGroupFull *c_nhg,
				     const struct fpm_dplane_nhg *obj,
				     uint32_t *grp_full_count)
{
	const struct nexthop *nh = obj->nh;
	uint16_t i;

	memset(c_nhg, 0, sizeof(*c_nhg));
	*grp_full_count = 0;

	/*
	 * Failing here (instead of clamping) keeps a truncated group off the
	 * wire: fpmsyncd would install it as if it were complete.
	 */
	if (obj->num_children > array_size(c_nhg->depends)) {
		zlog_err("%s: dplane NHG %u has %u members, depends[] holds %zu",
			 __func__, obj->dplane_id, obj->num_children,
			 array_size(c_nhg->depends));
		return false;
	}

	c_nhg->id = obj->dplane_id;
	/*
	 * The JSON `key` is 32 bit and purely informational (it used to carry
	 * zebra's NHG hash): the low half of the Merkle hash is the closest
	 * equivalent. Nothing keys off the wire value — the plugin dedupes on
	 * the full 64 bit hash, fpmsyncd on the id.
	 */
	c_nhg->key = (uint32_t)obj->hash;
	c_nhg->nhg_flags = obj->nhg_flags;

	/* depends[]: immediate members only */
	for (i = 0; i < obj->num_children; i++)
		c_nhg->depends[i] = obj->children[i].obj->dplane_id;

	/* nh_grp_full_list: members of all depths, flattened depth first */
	if (!flatten_nhgfull_members(obj, c_nhg->nh_grp_full_list,
				     array_size(c_nhg->nh_grp_full_list),
				     grp_full_count)) {
		zlog_err("%s: dplane NHG %u member tree overflows the %zu entry nh_grp_full_list",
			 __func__, obj->dplane_id,
			 array_size(c_nhg->nh_grp_full_list));
		return false;
	}

	/*
	 * dependents[] stays empty (the memset above): fpmsyncd derives the
	 * reverse edges from the depends lists it registers.
	 */

	/*
	 * Nexthop detail fields, same set the ctx driven singleton builder
	 * used to emit. A leaf (L-C) has its own nexthop, a resolved group
	 * (L-B) has its recursive parent — which is what the old multi builder
	 * took gate/type from — and an L-A group has no defining nexthop.
	 */
	if (nh) {
		c_nhg->type = nh->type;
		c_nhg->vrf_id = nh->vrf_id;
		c_nhg->ifindex = nh->ifindex;
		c_nhg->nh_label_type = nh->nh_label_type;

		if (nh->type == NEXTHOP_TYPE_BLACKHOLE)
			c_nhg->bh_type = nh->bh_type;
		else
			memcpy(&c_nhg->gate, &nh->gate, sizeof(union g_addr));

		memcpy(&c_nhg->src, &nh->src, sizeof(union g_addr));
		memcpy(&c_nhg->rmap_src, &nh->rmap_src, sizeof(union g_addr));
		c_nhg->weight = nh->weight;
		c_nhg->flags = nh->flags;

		/*
		 * set nexthop srv6 information if present. A nexthop_srv6 that
		 * carries neither a segment list nor a seg6local action holds
		 * nothing the peer can use (nhgmgr.cpp getNextHopFields() only
		 * reads nh_srv6 when nh_srv6->seg6_segs is non null), so do not
		 * allocate one for it.
		 */
		if (nh->nh_srv6 != NULL &&
		    (nh->nh_srv6->seg6_segs != NULL ||
		     nh->nh_srv6->seg6local_action !=
			     ZEBRA_SEG6_LOCAL_ACTION_UNSPEC)) {
			/* Get the default SRv6 context which contains seg6's src IP */
			struct zebra_srv6 *srv6 = zebra_srv6_get_default();

			c_nhg->nh_srv6 = malloc(sizeof(struct C_nexthop_srv6));
			if (c_nhg->nh_srv6) {
				/* field copy from nexthop_srv6 to C_nexthop_srv6 */
				/* seg6local_action */
				c_nhg->nh_srv6->seg6local_action =
					(enum C_seg6local_action_t)nh->nh_srv6->seg6local_action;

				/* seg6local_ctx */
				memcpy(&c_nhg->nh_srv6->seg6local_ctx, &nh->nh_srv6->seg6local_ctx,
					sizeof(struct C_seg6local_context));

				/* seg6_src */
				c_nhg->nh_srv6->seg6_src = srv6->encap_src_addr;

				/* seg6_segs */
				c_nhg->nh_srv6->seg6_segs = NULL;  // clear the pointer to avoid pointing to old address
				/* set nexthop_srv6 seg6_segs if present */
				if (nh->nh_srv6->seg6_segs != NULL) {
					size_t total_size = sizeof(struct C_seg6_seg_stack) +
								nh->nh_srv6->seg6_segs->num_segs * sizeof(struct in6_addr);
					c_nhg->nh_srv6->seg6_segs = malloc(total_size);
					if (c_nhg->nh_srv6->seg6_segs) {
						memcpy(c_nhg->nh_srv6->seg6_segs, nh->nh_srv6->seg6_segs, total_size);
					}
				}
			}
		}
	}

	/*
	 * obj->resolved_prefix / obj->vrf_id (L-B resolving info) are
	 * deliberately NOT emitted: they are plugin internal state feeding the
	 * dedupe hash. The NHGFIB schema has no field for them and fpmsyncd
	 * never reads them.
	 */

	return true;
}

/**
 * Free C_NextHopGroupFull Object.
 *
 * \param[in] c_nhg pointer of C_NextHopGroupFull Object.
 */
static void free_c_nexthopgroupfull(struct C_NextHopGroupFull *c_nhg)
{
	if (!c_nhg) {
		return;
	}

	/* Free srv6 related memory if present */
	if (c_nhg->nh_srv6) {
		/* Free seg6_segs if present */
		if (c_nhg->nh_srv6->seg6_segs) {
			free(c_nhg->nh_srv6->seg6_segs);
			c_nhg->nh_srv6->seg6_segs = NULL;
		}

		/* Free nh_srv6 itself */
		free(c_nhg->nh_srv6);
		c_nhg->nh_srv6 = NULL;
	}
}


/**
 * Encode one RTM_NEWNHGFIB message for a derived dplane NHG object.
 * The payload is independent of FPM framing; the caller validates the frame
 * length before it appends this message to an FPM output batch.
 */
ssize_t fpm_nhg_new_msg_encode(const struct fpm_dplane_nhg *obj, void *buf,
			       size_t buflen)
{
	struct {
		struct nlmsghdr n;
		struct nhmsg nhm;
		char buf[];
	} *req = buf;
	struct C_NextHopGroupFull c_nhg;
	uint32_t grp_full_count = 0;
	char *json_str = NULL;
	ssize_t ret = -1;

	if (buflen < sizeof(*req))
		return 0;

	if (!build_c_nhgfull_from_obj(&c_nhg, obj, &grp_full_count)) {
		free_c_nexthopgroupfull(&c_nhg);
		return -1;
	}

	memset(req, 0, sizeof(*req));
	req->n.nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg));
	req->n.nlmsg_flags = NLM_F_CREATE | NLM_F_REQUEST | NLM_F_REPLACE;
	req->n.nlmsg_type = RTM_NEWNHGFIB;
	req->nhm.nh_family = AF_UNSPEC;

	if (!nl_attr_put32(&req->n, buflen, NHA_ID, obj->dplane_id)) {
		free_c_nexthopgroupfull(&c_nhg);
		return 0;
	}

	if (obj->num_children) {
		json_str = nexthopgroupfull_json_from_c_nhg_multi(
			&c_nhg, grp_full_count, obj->num_children, 0,
			CHECK_FLAG(obj->nhg_flags, FPM_NHG_FLAG_RECURSIVE));
	} else {
		switch (c_nhg.type) {
		case C_NEXTHOP_TYPE_IPV4:
		case C_NEXTHOP_TYPE_IPV4_IFINDEX:
			req->nhm.nh_family = AF_INET;
			break;
		case C_NEXTHOP_TYPE_IPV6:
		case C_NEXTHOP_TYPE_IPV6_IFINDEX:
			req->nhm.nh_family = AF_INET6;
			break;
		default:
			break;
		}
		json_str = nexthopgroupfull_json_from_c_nhg_singleton(&c_nhg, 0, 0);
	}

	if (!json_str) {
		zlog_err("%s: failed to convert dplane NHG %u to a JSON string",
			 __func__, obj->dplane_id);
		free_c_nexthopgroupfull(&c_nhg);
		return -1;
	}

	if (!nl_attr_put(&req->n, buflen, FPM_NHA_JSON_STR, json_str,
			 strlen(json_str) + 1)) {
		zlog_err("%s: dplane NHG %u JSON (%zu bytes) does not fit the message",
			 __func__, obj->dplane_id, strlen(json_str) + 1);
		goto cleanup;
	}

	ret = NLMSG_ALIGN(req->n.nlmsg_len);

cleanup:
	free(json_str);
	free_c_nexthopgroupfull(&c_nhg);

	if (IS_ZEBRA_DEBUG_FPM)
		zlog_debug("%s: RTM_NEWNHGFIB, id=%u, len=%zd", __func__,
			   obj->dplane_id, ret);

	return ret;
}

/** Encode one RTM_DELNHGFIB message; the id is its entire payload. */
ssize_t fpm_nhg_del_msg_encode(uint32_t dplane_id, void *buf, size_t buflen)
{
	struct {
		struct nlmsghdr n;
		struct nhmsg nhm;
		char buf[];
	} *req = buf;

	if (buflen < sizeof(*req))
		return 0;

	memset(req, 0, sizeof(*req));
	req->n.nlmsg_len = NLMSG_LENGTH(sizeof(struct nhmsg));
	req->n.nlmsg_flags = NLM_F_CREATE | NLM_F_REQUEST;
	req->n.nlmsg_type = RTM_DELNHGFIB;
	req->nhm.nh_family = AF_UNSPEC;

	if (!nl_attr_put32(&req->n, buflen, NHA_ID, dplane_id))
		return 0;

	if (IS_ZEBRA_DEBUG_FPM)
		zlog_debug("%s: RTM_DELNHGFIB, id=%u", __func__, dplane_id);

	return NLMSG_ALIGN(req->n.nlmsg_len);
}

/**
 * Append one fully encoded netlink message to the batch. The bytes are
 * copied, so callers may encode every frame into the same scratch buffer.
 *
 * The assert is the per-frame wire bound: the FPM header carries the frame
 * length as a u16 (stream_putw()), so no single frame may exceed UINT16_MAX
 * bytes including that header. Every producer is bounded by it — netlink
 * messages are encoded into a 64KiB scratch buffer, and the NHGFIB emitters
 * below refuse anything the framing cannot carry.
 */
void fpm_nhg_frame_batch_add(struct fpm_frame_batch *batch, const uint8_t *buf,
			     size_t len)
{
	assert(len > 0 && (len + FPM_HEADER_SIZE) <= UINT16_MAX);

	if (batch->count == batch->cap) {
		batch->cap = batch->cap ? batch->cap * 2 : 16;
		batch->frames = XREALLOC(MTYPE_FPM_NHG_FRAME, batch->frames,
					 (size_t)batch->cap *
						 sizeof(*batch->frames));
	}

	batch->frames[batch->count].buf = XCALLOC(MTYPE_FPM_NHG_FRAME, len);
	memcpy(batch->frames[batch->count].buf, buf, len);
	batch->frames[batch->count].len = len;
	batch->count++;
	batch->total_len += len + FPM_HEADER_SIZE;
}

void fpm_nhg_frame_batch_free(struct fpm_frame_batch *batch)
{
	uint32_t i;

	for (i = 0; i < batch->count; i++)
		XFREE(MTYPE_FPM_NHG_FRAME, batch->frames[i].buf);
	XFREE(MTYPE_FPM_NHG_FRAME, batch->frames);
	batch->count = 0;
	batch->cap = 0;
	batch->total_len = 0;
	batch->nhgfib_count = 0;
}

/*
 * NHGFIB message emitters.
 *
 * Emitters only APPEND to the caller's frame batch, they never touch obuf:
 * the batch is written in one piece afterwards, which is what makes the
 * whole context atomic on the wire — and what makes its size exactly known
 * before any byte is written.
 *
 * Each emitter encodes into its own scratch buffer: the caller's buffer
 * already holds the route message of this context, which must survive until
 * the batch is assembled. fpm_nhg_frame_batch_add() copies the bytes, so one
 * buffer per call is enough. The encoders refuse anything the FPM framing
 * cannot carry (len + FPM_HEADER_SIZE > UINT16_MAX), which is what keeps the
 * assert in fpm_nhg_frame_batch_add() unreachable.
 *
 * @return false on encode failure; the caller then emits nothing at all and
 *         reports the failure to zebra, exactly like a route encode failure.
 */
static bool fpm_nhg_emit_new(const struct fpm_dplane_nhg *obj,
			     struct fpm_frame_batch *batch)
{
	uint8_t buf[FPM_NHG_MSG_BUF_SIZE];
	ssize_t rv;

	rv = fpm_nhg_new_msg_encode(obj, buf, sizeof(buf));
	if (rv > 0 && (size_t)rv + FPM_HEADER_SIZE > UINT16_MAX) {
		zlog_err("%s: NEWNHGFIB for dplane NHG %u is too large to frame",
			 __func__, obj->dplane_id);
		rv = -1;
	}
	if (rv <= 0) {
		zlog_err("%s: NEWNHGFIB encode failed for dplane NHG %u",
			 __func__, obj->dplane_id);
		return false;
	}

	fpm_nhg_frame_batch_add(batch, buf, (size_t)rv);
	batch->nhgfib_count++;

	if (IS_ZEBRA_DEBUG_FPM)
		zlog_debug("%s: NEWNHGFIB id %u level %u flags 0x%x children %u refcount %u",
			   __func__, obj->dplane_id, obj->level, obj->nhg_flags,
			   obj->num_children, obj->refcount);

	return true;
}

static bool fpm_nhg_emit_del(uint32_t dplane_id, struct fpm_frame_batch *batch)
{
	/* A DEL carries nothing but the id: a small buffer covers it. */
	uint8_t buf[128];
	ssize_t rv;

	rv = fpm_nhg_del_msg_encode(dplane_id, buf, sizeof(buf));
	if (rv > 0 && (size_t)rv + FPM_HEADER_SIZE > UINT16_MAX) {
		zlog_err("%s: DELNHGFIB for dplane NHG %u is too large to frame",
			 __func__, dplane_id);
		rv = -1;
	}
	if (rv <= 0) {
		zlog_err("%s: DELNHGFIB encode failed for dplane NHG %u",
			 __func__, dplane_id);
		return false;
	}

	fpm_nhg_frame_batch_add(batch, buf, (size_t)rv);
	batch->nhgfib_count++;

	if (IS_ZEBRA_DEBUG_FPM)
		zlog_debug("%s: DELNHGFIB id %u", __func__, dplane_id);

	return true;
}

bool fpm_nhg_batch_add_pending_dels(const struct fpm_nhg_del_queue *q,
				    struct fpm_frame_batch *batch)
{
	uint32_t i;

	for (i = 0; i < q->count; i++)
		if (!fpm_nhg_emit_del(q->ids[i].dplane_id, batch))
			return false;

	return true;
}

bool fpm_nhg_batch_add_staging(const struct fpm_nhg_staging *s,
			       struct fpm_frame_batch *batch)
{
	uint32_t i;

	for (i = 0; i < s->count; i++)
		if (!fpm_nhg_emit_new(s->objs[i], batch))
			return false;

	return true;
}

/*
 * The command reads the tables through these pointers, bound once by
 * fpm_nhg_vty_init() to the dplane provider's own state. The pointed-to
 * objects live as long as the daemon, so no lifetime handshake is needed.
 */
static struct fpm_nhg_tables *show_tables;
static pthread_mutex_t *show_lock;
static const bool *show_enabled;

/*
 * `show fpm nhg-fib` printer. One of the two output sinks is
 * active: `jarray` set means JSON, otherwise plain text goes to `vty`.
 */
struct fpm_nhg_show_args {
	struct vty *vty;
	struct json_object *jarray;
};

/* Max zebra NHG ids printed per object in the text form of `show fpm nhg-fib`. */
#define FPM_NHG_SHOW_RIB_IDS 8

static void fpm_nhg_show_obj(const struct fpm_dplane_nhg *obj, void *arg)
{
	struct fpm_nhg_show_args *sa = arg;
	struct vty *vty = sa->vty;
	char hashbuf[32];
	uint16_t i;

	snprintfrr(hashbuf, sizeof(hashbuf), "0x%016" PRIx64, obj->hash);

	if (sa->jarray) {
		struct json_object *jo = json_object_new_object();
		struct json_object *jchildren, *jrib;

		json_object_int_add(jo, "dplaneId", obj->dplane_id);
		json_object_string_add(jo, "level",
				       fpm_nhg_level_str(obj->level));
		json_object_int_add(jo, "flags", obj->nhg_flags);
		json_object_int_add(jo, "refcount", obj->refcount);
		json_object_string_add(jo, "hash", hashbuf);

		jchildren = json_object_new_array();
		for (i = 0; i < obj->num_children; i++) {
			struct json_object *jc = json_object_new_object();

			json_object_int_add(jc, "id",
					    obj->children[i].obj->dplane_id);
			json_object_int_add(jc, "weight",
					    obj->children[i].weight);
			json_object_array_add(jchildren, jc);
		}
		json_object_object_add(jo, "children", jchildren);

		/* L-B only: omitted entirely when there is no resolving info. */
		if (obj->resolved_prefix.family != AF_UNSPEC) {
			char pbuf[PREFIX_STRLEN];

			snprintfrr(pbuf, sizeof(pbuf), "%pFX",
				   &obj->resolved_prefix);
			json_object_string_add(jo, "resolvedVia", pbuf);
			json_object_int_add(jo, "resolvingRibNhgId",
					    obj->resolved_via);
		}
		json_object_int_add(jo, "vrfId", obj->vrf_id);

		jrib = json_object_new_array();
		for (i = 0; i < obj->rib_nhg_id_count; i++)
			json_object_array_add(
				jrib,
				json_object_new_int64(obj->rib_nhg_ids[i]));
		json_object_object_add(jo, "ribNhgIds", jrib);

		json_object_array_add(sa->jarray, jo);
		return;
	}

	vty_out(vty, "Dplane NHG %u (%s) flags 0x%04x refcount %u hash %s\n",
		obj->dplane_id, fpm_nhg_level_str(obj->level), obj->nhg_flags,
		obj->refcount, hashbuf);

	if (obj->resolved_prefix.family != AF_UNSPEC)
		vty_out(vty, "  resolved via %pFX vrf %u rib nhg %u\n",
			&obj->resolved_prefix, obj->vrf_id, obj->resolved_via);

	if (obj->num_children) {
		vty_out(vty, "  children:");
		for (i = 0; i < obj->num_children; i++)
			vty_out(vty, " %u(w%u)",
				obj->children[i].obj->dplane_id,
				obj->children[i].weight);
		vty_out(vty, "\n");
	}

	if (obj->rib_nhg_id_count) {
		/*
		 * The id list is uncapped (a widely shared object can collect
		 * many), so bound the text form to keep one object on one
		 * line. The json form above stays complete.
		 */
		uint16_t shown = MIN(obj->rib_nhg_id_count,
				     FPM_NHG_SHOW_RIB_IDS);

		vty_out(vty, "  rib nhg ids:");
		for (i = 0; i < shown; i++)
			vty_out(vty, " %u", obj->rib_nhg_ids[i]);
		if (shown < obj->rib_nhg_id_count)
			vty_out(vty, " +%u more",
				obj->rib_nhg_id_count - shown);
		vty_out(vty, "\n");
	}
}

/*
 * `show fpm nhg-fib resolved-via`: the inverted view of the resolving info.
 *
 * Only L-B objects carry a resolving prefix, and several of them can share one
 * (they differ in label stack or in resolved members), so the mapping is
 * prefix -> set of dplane NHG ids. That set is exactly what depends on the
 * prefix.
 *
 * Derived by walking the object table rather than from a dedicated index: the
 * view is diagnostic, so a scan cannot go stale against the objects it reports.
 */
struct fpm_nhg_rv_entry {
	struct prefix p;
	vrf_id_t vrf_id;
	uint32_t resolved_via; /* zebra NHG id of the resolution, 0 if unknown */
	uint32_t dplane_id;
};

struct fpm_nhg_rv_collect {
	struct fpm_nhg_rv_entry *ents;
	uint32_t count, cap;
};

static void fpm_nhg_rv_collect_cb(const struct fpm_dplane_nhg *obj, void *arg)
{
	struct fpm_nhg_rv_collect *c = arg;

	if (obj->resolved_prefix.family == AF_UNSPEC)
		return;

	if (c->count == c->cap) {
		c->cap = c->cap ? c->cap * 2 : 16;
		c->ents = XREALLOC(MTYPE_TMP, c->ents,
				   c->cap * sizeof(*c->ents));
	}
	c->ents[c->count].p = obj->resolved_prefix;
	c->ents[c->count].vrf_id = obj->vrf_id;
	c->ents[c->count].resolved_via = obj->resolved_via;
	c->ents[c->count].dplane_id = obj->dplane_id;
	c->count++;
}

/*
 * Group key is (vrf, prefix, resolving NHG id); dplane id only orders within a
 * group. The resolving id is part of the key so a prefix that is reached
 * through two different resolutions is reported as two rows rather than one
 * misleading merged row.
 */
static int fpm_nhg_rv_cmp(const void *a, const void *b)
{
	const struct fpm_nhg_rv_entry *ea = a, *eb = b;
	int rv;

	if (ea->vrf_id != eb->vrf_id)
		return ea->vrf_id < eb->vrf_id ? -1 : 1;
	rv = prefix_cmp(&ea->p, &eb->p);
	if (rv != 0)
		return rv;
	if (ea->resolved_via != eb->resolved_via)
		return ea->resolved_via < eb->resolved_via ? -1 : 1;
	if (ea->dplane_id != eb->dplane_id)
		return ea->dplane_id < eb->dplane_id ? -1 : 1;
	return 0;
}

static void fpm_nhg_show_resolved_via(struct vty *vty,
				      struct json_object *jarray)
{
	struct fpm_nhg_rv_collect c = {};
	char pbuf[PREFIX_STRLEN];
	uint32_t i, j, k;

	fpm_nhg_walk(show_tables, fpm_nhg_rv_collect_cb, &c);
	if (c.count == 0) {
		if (!jarray)
			vty_out(vty, "No resolving prefixes recorded\n");
		return;
	}
	qsort(c.ents, c.count, sizeof(*c.ents), fpm_nhg_rv_cmp);

	if (!jarray)
		vty_out(vty, "%-40s %-4s %-12s %s\n", "Resolving prefix",
			"VRF", "Resolved by", "Dplane NHG ids");

	for (i = 0; i < c.count; i = j) {
		/* collapse the run of entries sharing this (vrf, prefix) */
		for (j = i + 1; j < c.count; j++) {
			if (c.ents[j].vrf_id != c.ents[i].vrf_id ||
			    c.ents[j].resolved_via != c.ents[i].resolved_via ||
			    prefix_cmp(&c.ents[j].p, &c.ents[i].p) != 0)
				break;
		}

		snprintfrr(pbuf, sizeof(pbuf), "%pFX", &c.ents[i].p);

		/*
		 * Only the resolving zebra NHG id is reported. It deliberately
		 * is NOT translated through by_rib_id: that index is keyed by
		 * the id a *route* reported (dplane_ctx_get_nhe_id, i.e. the
		 * resolved NHE), and zebra_nhg_resolve() collapses several
		 * distinct NHEs onto one such id, so more than one dplane
		 * object can claim it and the index would answer with whichever
		 * recorded it last — not with the resolution.
		 */
		if (jarray) {
			struct json_object *jo = json_object_new_object();
			struct json_object *jids = json_object_new_array();

			json_object_string_add(jo, "resolvedVia", pbuf);
			json_object_int_add(jo, "vrfId", c.ents[i].vrf_id);
			json_object_int_add(jo, "resolvingRibNhgId",
					    c.ents[i].resolved_via);
			for (k = i; k < j; k++)
				json_object_array_add(
					jids,
					json_object_new_int64(
						c.ents[k].dplane_id));
			json_object_object_add(jo, "dplaneIds", jids);
			json_object_array_add(jarray, jo);
		} else {
			if (c.ents[i].resolved_via)
				vty_out(vty, "%-40s %-4u rib %-8u", pbuf,
					c.ents[i].vrf_id,
					c.ents[i].resolved_via);
			else
				vty_out(vty, "%-40s %-4u %-12s", pbuf,
					c.ents[i].vrf_id, "-");
			for (k = i; k < j; k++)
				vty_out(vty, " %u", c.ents[k].dplane_id);
			vty_out(vty, "\n");
		}
	}

	XFREE(MTYPE_TMP, c.ents);
}

/*
 * `show fpm nhg-fib route <prefix> [vrf NAME]`: the object a specific route
 * references, straight out of route_nhg_map.
 *
 * This is the only unambiguous route -> dplane id answer. Going through
 * by-rib-id instead is unsound: that index is keyed by the id a route reported
 * (dplane_ctx_get_nhe_id, i.e. the *resolved* NHE), and zebra_nhg_resolve()
 * collapses several distinct NHEs onto one such id, so two routes with
 * different trees — and therefore different dplane objects — can report the
 * same id.
 *
 * The key must be built with fpm_nhg_route_key_init(), hence the table id
 * lookup: a VRF route is keyed on that VRF's table, not on the
 * VRF id.
 */
static void fpm_nhg_show_route(struct vty *vty, struct json_object *jarray,
			       const char *prefix_str, const char *vrf_name)
{
	struct fpm_nhg_show_args sa = { .vty = vty, .jarray = jarray };
	const struct fpm_dplane_nhg *obj;
	struct fpm_nhg_route_key key;
	struct zebra_vrf *zvrf;
	struct prefix p;

	if (str2prefix(prefix_str, &p) == 0) {
		vty_out(vty, "%% Malformed prefix %s\n", prefix_str);
		return;
	}

	if (vrf_name)
		zvrf = zebra_vrf_lookup_by_name(vrf_name);
	else
		zvrf = zebra_vrf_lookup_by_id(VRF_DEFAULT);
	if (zvrf == NULL) {
		vty_out(vty, "%% VRF %s not found\n",
			vrf_name ? vrf_name : VRF_DEFAULT_NAME);
		return;
	}

	fpm_nhg_route_key_init(&key, zvrf->table_id, &p, NULL);
	/* src_p stays AF_UNSPEC: srcdest routes are not addressable here. */

	obj = fpm_nhg_route_get(show_tables, &key);
	if (obj == NULL) {
		if (!jarray)
			vty_out(vty, "%% No dplane NHG for %pFX in vrf %s (table %u)\n",
				&p, vrf_name ? vrf_name : VRF_DEFAULT_NAME,
				key.table_id);
		return;
	}

	fpm_nhg_show_obj(obj, &sa);
}

DEFUN(fpm_show_nhg_fib, fpm_show_nhg_fib_cmd,
      "show fpm nhg-fib [<id (1-4294967295)|by-rib-id (1-4294967295)|resolved-via|route WORD [vrf WORD]>] [json]",
      SHOW_STR
      FPM_STR
      "Dplane next hop groups derived from route events\n"
      "Filter by dplane next hop group id\n"
      "Identifier\n"
      "Filter by zebra (rib) next hop group id\n"
      "Identifier\n"
      "Resolving prefix to dplane next hop group id mapping\n"
      "Look up the object a specific route references\n"
      "IP prefix (A.B.C.D/M or X:X::X:X/M)\n"
      VRF_CMD_HELP_STR
      "VRF name\n"
      JSON_STR)
{
	struct fpm_nhg_show_args sa = {};
	bool json = false, filter = false, rib_filter = false;
	bool resolved_via = false, found = true;
	const char *route_str = NULL, *vrf_str = NULL;
	uint32_t filter_id = 0;
	int idx;

	for (idx = 3; idx < argc; idx++) {
		if (!strcmp(argv[idx]->text, "id") && idx + 1 < argc) {
			filter = true;
			filter_id = strtoul(argv[idx + 1]->arg, NULL, 10);
			idx++;
		} else if (!strcmp(argv[idx]->text, "by-rib-id") &&
			   idx + 1 < argc) {
			rib_filter = true;
			filter_id = strtoul(argv[idx + 1]->arg, NULL, 10);
			idx++;
		} else if (!strcmp(argv[idx]->text, "resolved-via"))
			resolved_via = true;
		else if (!strcmp(argv[idx]->text, "route") && idx + 1 < argc) {
			route_str = argv[idx + 1]->arg;
			idx++;
		} else if (!strcmp(argv[idx]->text, "vrf") && idx + 1 < argc) {
			vrf_str = argv[idx + 1]->arg;
			idx++;
		} else if (!strcmp(argv[idx]->text, "json"))
			json = true;
	}

	if (show_tables == NULL || !*show_enabled) {
		vty_out(vty, "FPM nhg-fib mode is not enabled\n");
		return CMD_SUCCESS;
	}

	sa.vty = vty;
	if (json)
		sa.jarray = json_object_new_array();

	/*
	 * nhg_tables is written under obuf_mutex by two threads — the FPM
	 * pthread processing dplane contexts and the zebra main thread calling
	 * fpm_nl_enqueue() from the fpm_rib_send() resync walk — together with
	 * the messages describing those mutations. A reader that skipped the
	 * mutex could see half-built objects (children array swapped in, count
	 * not yet bumped) or follow a child pointer an unref just freed, so the
	 * whole walk stays inside the same critical section.
	 */
	frr_with_mutex (show_lock) {
		if (route_str) {
			fpm_nhg_show_route(vty, sa.jarray, route_str, vrf_str);
		} else if (resolved_via) {
			fpm_nhg_show_resolved_via(vty, sa.jarray);
		} else if (filter || rib_filter) {
			const struct fpm_dplane_nhg *obj;

			/*
			 * by-rib-id resolves through the reverse index: a zebra
			 * NHG id maps to the one dplane object its routes
			 * currently reference.
			 */
			if (rib_filter)
				obj = fpm_nhg_lookup_rib_id(show_tables,
							    filter_id);
			else
				obj = fpm_nhg_lookup_id(show_tables, filter_id);
			if (obj)
				fpm_nhg_show_obj(obj, &sa);
			else
				found = false;
		} else
			fpm_nhg_walk(show_tables, fpm_nhg_show_obj, &sa);
	}

	if (sa.jarray) {
		vty_json(vty, sa.jarray);
		return CMD_SUCCESS;
	}

	if (!found)
		vty_out(vty, "%% %s NHG %u not found\n",
			rib_filter ? "Rib" : "Dplane", filter_id);

	return CMD_SUCCESS;
}

void fpm_nhg_vty_init(struct fpm_nhg_tables *t, pthread_mutex_t *lock,
		      const bool *enabled)
{
	show_tables = t;
	show_lock = lock;
	show_enabled = enabled;

	install_element(ENABLE_NODE, &fpm_show_nhg_fib_cmd);
}

static enum fib_log_level *fib_log_level_ptr;

const char *fpm_nhg_fib_log_level_str(enum fib_log_level level)
{
	switch (level) {
	case FIB_LOG_LEVEL_DEBUG: return "debug";
	case FIB_LOG_LEVEL_INFO:  return "info";
	case FIB_LOG_LEVEL_WARN:  return "warn";
	case FIB_LOG_LEVEL_ERROR: return "error";
	default:                  return "unknown";
	}
}

DEFUN(fpm_set_fib_log_level, fpm_set_fib_log_level_cmd,
      "fpm fib-log-level <debug|info|warn|error>",
      FPM_STR
      "Set FIB library log level\n"
      "Debug level (most verbose)\n"
      "Info level\n"
      "Warn level\n"
      "Error level (least verbose)\n")
{
	enum fib_log_level level;
	const char *level_str = argv[2]->text;

	if (strcmp(level_str, "debug") == 0)
		level = FIB_LOG_LEVEL_DEBUG;
	else if (strcmp(level_str, "info") == 0)
		level = FIB_LOG_LEVEL_INFO;
	else if (strcmp(level_str, "warn") == 0)
		level = FIB_LOG_LEVEL_WARN;
	else if (strcmp(level_str, "error") == 0)
		level = FIB_LOG_LEVEL_ERROR;
	else {
		vty_out(vty, "%% Invalid log level: %s\n", level_str);
		return CMD_WARNING;
	}

	*fib_log_level_ptr = level;
	fib_frr_set_log_level((int)level);
	zlog_info("%s: FIB log level set to %s (%d)", __func__, level_str, (int)level);
	return CMD_SUCCESS;
}

DEFUN(no_fpm_set_fib_log_level, no_fpm_set_fib_log_level_cmd,
      "no fpm fib-log-level [<debug|info|warn|error>]",
      NO_STR
      FPM_STR
      "Set FIB library log level\n"
      "Log level value\n")
{
	*fib_log_level_ptr = FIB_LOG_LEVEL_INFO; /* restore to default: INFO */
	fib_frr_set_log_level(*fib_log_level_ptr);
	zlog_info("%s: FIB log level reset to default (INFO)", __func__);
	return CMD_SUCCESS;
}

DEFUN(fpm_show_fib_log_level, fpm_show_fib_log_level_cmd,
      "show fpm fib-log-level",
      SHOW_STR
      FPM_STR
      "Show current FIB library log level\n")
{
	vty_out(vty, "FIB log level: %d (%s)\n",
		*fib_log_level_ptr,
		fpm_nhg_fib_log_level_str(*fib_log_level_ptr));
	return CMD_SUCCESS;
}

/*
 * FRR-compatible callback to forward logs from FIB to FRR's logging system.
 */
static void frr_log_forwarder(int level,
                              const char *file,
                              int line,
                              const char *func,
                              const char *fmt,
                              va_list args)
{
    int current_log_level = fib_frr_get_log_level();

    /*
     * We would skip logging message if
     * level is below current log level and debug zebra fpm is not enabled
     */
    if (level < current_log_level &&  !IS_ZEBRA_DEBUG_FPM) {
        return;
    }

    /*
     * Direct stderr — FRR convention for FIB path debugging
     * We can't use FRR's zlog here because no API for forwarding va_list, and
     * we want to preserve the original log level and formatting as much as possible.
     */
    fprintf(stderr, "[ZEBRA:FIB] %s:%d (%s) ", file, line, func);
    va_list args_copy;
    va_copy(args_copy, args);
    vfprintf(stderr, fmt, args_copy);
    va_end(args_copy);
    fprintf(stderr, "\n");
    fflush(stderr);
}

void fpm_nhg_fib_log_register(void)
{
    /* Register callback BEFORE any fib_LOG() calls */
    fib_frr_register_callback(frr_log_forwarder);

    zlog_info("%s : FIB logging callback registered, log level will be applied after configuration load",
			  __func__);
}

void fpm_nhg_fib_log_init(enum fib_log_level *level)
{
	fib_log_level_ptr = level;
	fib_frr_set_log_level(*level);
	zlog_info("%s: FIB log level initialized to %d (INFO)", __func__, *level);

	install_element(ENABLE_NODE, &fpm_show_fib_log_level_cmd);
	install_element(CONFIG_NODE, &fpm_set_fib_log_level_cmd);
	install_element(CONFIG_NODE, &no_fpm_set_fib_log_level_cmd);
}
