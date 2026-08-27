/*
 * FPM dplane NHG object model: 3-level Merkle-hashed nexthop group
 * objects with plugin-allocated uint32 dplane ids.
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

#ifndef _FPM_NHG_H
#define _FPM_NHG_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>

#include "lib/prefix.h"
#include "lib/nexthop.h"
#include "lib/hash.h"


enum fpm_nhg_level { FPM_NHG_L_C = 0, FPM_NHG_L_B, FPM_NHG_L_A };

/*
 * nhg_flags bits carried in the NHGFIB JSON. RECURSIVE (1 << 3) mirrors
 * zebra_nhg.h NEXTHOP_GROUP_RECURSIVE; RECEIVED (1 << 12) mirrors the
 * fpmsyncd wire contract (nhgmgr.h NEXTHOP_GROUP_RECEIVED, checked as
 * (1 << 12)), so these must never change even if zebra_nhg.h flag
 * values move.
 */
#define FPM_NHG_FLAG_RECURSIVE (1 << 3)
#define FPM_NHG_FLAG_RECEIVED (1 << 12)

struct fpm_dplane_nhg;

struct fpm_nhg_child {
	struct fpm_dplane_nhg *obj;
	uint16_t weight;
};

struct fpm_dplane_nhg {
	uint64_t hash;        /* internal Merkle dedupe key */
	uint32_t dplane_id;   /* wire id: NHGFIB id / depends / RTA_NH_ID */
	uint32_t refcount;    /* parents + routes */
	uint8_t level;        /* enum fpm_nhg_level */
	uint32_t nhg_flags;   /* RECEIVED / RECURSIVE subset for JSON */
	struct nexthop *nh;   /* defining nexthop (dup'd) */
	struct prefix resolved_prefix; /* resolving prefix, when known */
	vrf_id_t vrf_id;
	/*
	 * Zebra NHG id this object resolved through (PR #19252 resolved_via).
	 * Not part of identity — it is derivable from the resolution, which is
	 * already reflected in the children (L-B) or in the gate (SRv6 leaf).
	 * Kept because by_rib_id maps it straight onto the dplane object of the
	 * resolving group.
	 */
	uint32_t resolved_via;
	/*
	 * Distinct zebra NHG ids that referenced this object, mirrored by the
	 * tables' by_rib_id reverse index (fpm_nhg_record_rib_id()). Grown one
	 * id at a time and never evicted: the index is only trustworthy if the
	 * object remembers every id pointing at it, both to answer a reverse
	 * lookup and to release exactly its own entries when it dies. Bounded
	 * by the number of zebra NHEs that map here, 4 bytes each.
	 */
	uint32_t *rib_nhg_ids;
	uint16_t rib_nhg_id_count, rib_nhg_id_cap;
	uint16_t num_children;
	struct fpm_nhg_child *children; /* sorted by obj->hash */
};

/*
 * Route key: identifies a route the plugin has seen. The (table_id, afi,
 * prefix, src_p) tuple is the route_nhg_map key — the only memory of "old"
 * across route events.
 *
 * src_p carries dplane_ctx_get_src() for srcdest routes and is all-zero
 * (family AF_UNSPEC) otherwise: two srcdest routes sharing one destination
 * are distinct routes in the kernel/FIB, so they must not share one map
 * entry — sharing would make the second install unref the first one's
 * object and desync the peer.
 */
struct fpm_nhg_route_key {
	uint32_t table_id;
	uint8_t afi;
	struct prefix p;
	struct prefix src_p;
};

void fpm_nhg_route_key_init(struct fpm_nhg_route_key *key, uint32_t table_id,
			    const struct prefix *dest, const struct prefix *src);

struct fpm_nhg_tables {
	struct hash *by_hash;
	struct hash *by_id;
	struct hash *route_map; /* fpm_nhg_route_key -> top (L-A) object */
	struct hash *by_rib_id; /* zebra NHG id -> object (reverse index) */
	/* (vrf, resolving prefix) -> a private bucket of resolved objects. */
	struct hash *by_resolved;
	uint32_t next_id;
	uint32_t *free_ids;
	uint32_t free_id_count, free_id_cap;
	/*
	 * counters (lifetime totals, never reset by a reconnect flush).
	 * dedupe_hits is cumulative lookups incl. rolled-back builds: a
	 * build that fails and rolls back keeps the hits it scored, so the
	 * counter tracks lookup behaviour, not surviving objects.
	 */
	uint64_t obj_created, obj_deleted, nhgfib_sent, dedupe_hits;
};

void fpm_nhg_tables_init(struct fpm_nhg_tables *t);
void fpm_nhg_tables_flush(struct fpm_nhg_tables *t);
void fpm_nhg_tables_fini(struct fpm_nhg_tables *t);
uint32_t fpm_nhg_id_alloc(struct fpm_nhg_tables *t);
void fpm_nhg_id_free(struct fpm_nhg_tables *t, uint32_t id);
uint64_t fpm_nhg_hash_leaf(const struct nexthop *nh);
uint64_t fpm_nhg_hash_group(uint8_t level, uint32_t nhg_flags,
			    const struct fpm_nhg_child *children,
			    uint16_t count, const struct prefix *resolved,
			    vrf_id_t vrf_id);
void fpm_nhg_insert(struct fpm_nhg_tables *t, struct fpm_dplane_nhg *obj);
void fpm_nhg_remove(struct fpm_nhg_tables *t, struct fpm_dplane_nhg *obj);

/**
 * FPM header:
 * {
 *   version: 1 byte (always 1),
 *   type: 1 byte (1 for netlink, 2 protobuf),
 *   len: 2 bytes (network order),
 * }
 *
 * This header is used with any format to tell the users how many bytes to
 * expect. Defined in dplane_fpm_sonic.c; guarded here so fpm_nhg.c can do
 * frame accounting without depending on include order.
 */
#ifndef FPM_HEADER_SIZE
#define FPM_HEADER_SIZE 4
#endif

/* One FPM frame pending write: fully encoded netlink message bytes. */
struct fpm_frame {
	uint8_t *buf;
	size_t len;
};

/*
 * Every frame one dplane context produces, in wire order. The batch is the
 * unit of atomicity: its total byte count is known exactly before anything is
 * written, so it is admitted by a single room check and then written to obuf
 * in one critical section — the peer never observes a partially emitted
 * context.
 *
 * count/cap are uint32 so an unusually large batch degrades into a room
 * failure (which the caller can roll back and retry) instead of tripping an
 * assert. The per-frame framing bound is enforced in
 * fpm_nhg_frame_batch_add().
 */
struct fpm_frame_batch {
	struct fpm_frame *frames;
	uint32_t count, cap;
	size_t total_len;   /* sum of len + FPM_HEADER_SIZE per frame */
	/*
	 * How many of those frames are NHGFIB messages. The nhgfib_sent
	 * counter is bumped only once the batch reaches obuf, so it counts
	 * frames actually written instead of frames assembled — an assembled
	 * batch can still be rolled back.
	 */
	uint32_t nhgfib_count;
};

struct fpm_nhg_staging {
	struct fpm_dplane_nhg **objs;  /* objects needing RTM_NEWNHGFIB, child-first order */
	/*
	 * count is bumped one object at a time and cap only doubles (from 16)
	 * when count reaches it, so cap stays the smallest power of two >=
	 * count. count itself is bounded by the number of objects one build
	 * creates, i.e. by the size of one route's nexthop tree (thousands at
	 * most) — five orders of magnitude below the 2^31 where the doubling
	 * would leave the uint32.
	 */
	uint32_t count, cap;
};

void fpm_nhg_staging_free(struct fpm_nhg_staging *s);
struct fpm_dplane_nhg *fpm_nhg_build(struct fpm_nhg_tables *t,
				     const struct nexthop *chain,
				     struct fpm_nhg_staging *newq);
void fpm_nhg_rollback(struct fpm_nhg_tables *t, struct fpm_nhg_staging *newq);

/*
 * DEL staging carries the payload BY VALUE, not the object pointer: the model
 * queues the id before freeing the object, so a pointer queue would hand the
 * emitter a dangling object. RTM_DELNHGFIB needs nothing but the id.
 */
struct fpm_nhg_del_entry {
	uint32_t dplane_id;
};

struct fpm_nhg_del_queue {
	struct fpm_nhg_del_entry *ids; /* parent-before-child DEL order */
	/*
	 * Same growth argument as fpm_nhg_staging: cap doubles (from 16) only
	 * when count reaches it, so cap is the smallest power of two >= count.
	 * count is bounded by the number of live objects the tables hold, and
	 * every one of those costs more than a hundred heap bytes, so reaching
	 * the 2^31 where the doubling would leave the uint32 would need
	 * hundreds of gigabytes of NHG state.
	 */
	uint32_t count, cap;
};

void fpm_nhg_del_queue_free(struct fpm_nhg_del_queue *q);
/* Empty the queue but keep its allocation (a queue reused across batches). */
void fpm_nhg_del_queue_reset(struct fpm_nhg_del_queue *q);
uint32_t fpm_nhg_del_queue_count(const struct fpm_nhg_del_queue *q);

/*
 * Frame-batch assembly: append copies of fully encoded netlink messages in
 * wire order. The batch is written to the FPM output buffer by the caller,
 * which owns the buffer and its lock; this module never touches either.
 */
void fpm_nhg_frame_batch_add(struct fpm_frame_batch *batch, const uint8_t *buf,
			     size_t len);
void fpm_nhg_frame_batch_free(struct fpm_frame_batch *batch);

/*
 * Encode and append every carried-over DEL (parent-before-child) / every
 * staged NEW (child-before-parent) to the batch. False on encode failure: the
 * caller emits nothing and leaves the queue in place for the next attempt.
 */
bool fpm_nhg_batch_add_pending_dels(const struct fpm_nhg_del_queue *q,
				    struct fpm_frame_batch *batch);
bool fpm_nhg_batch_add_staging(const struct fpm_nhg_staging *s,
			       struct fpm_frame_batch *batch);

struct fpm_dplane_nhg *fpm_nhg_route_get(struct fpm_nhg_tables *t,
					 const struct fpm_nhg_route_key *k);
void fpm_nhg_route_commit(struct fpm_nhg_tables *t,
			  const struct fpm_nhg_route_key *k,
			  struct fpm_dplane_nhg *new_top, uint32_t rib_id,
			  struct fpm_nhg_del_queue *delq);
void fpm_nhg_route_forget(struct fpm_nhg_tables *t,
			  const struct fpm_nhg_route_key *k,
			  struct fpm_nhg_del_queue *delq);

/*
 * Log the objects resolved through this route prefix. The caller must hold the
 * same lock that serializes the tables for the complete call.
 */
void fpm_nhg_debug_resolved_route(struct fpm_nhg_tables *t, vrf_id_t vrf_id,
				  const struct prefix *p, const char *op_name);
uint32_t fpm_nhg_count(struct fpm_nhg_tables *t);

/*
 * Encode raw NHGFIB netlink payloads for one derived dplane object. FPM frame
 * construction and its length limit remain the caller's responsibility.
 */
ssize_t fpm_nhg_new_msg_encode(const struct fpm_dplane_nhg *obj, void *buf,
			       size_t buflen);
ssize_t fpm_nhg_del_msg_encode(uint32_t dplane_id, void *buf, size_t buflen);

/*
 * Install `show fpm nhg-fib`. The pointed-to objects must outlive the daemon:
 * the command reads the tables under the given lock whenever it runs, and the
 * enabled flag gates the output. Called once from the dplane provider init.
 */
void fpm_nhg_vty_init(struct fpm_nhg_tables *t, pthread_mutex_t *lock,
		      const bool *enabled);

/* FIB log level constants, aligned with fib::LogLevel in nexthopgroup_debug.h */
enum fib_log_level {
	FIB_LOG_LEVEL_DEBUG = 0,
	FIB_LOG_LEVEL_INFO  = 1,
	FIB_LOG_LEVEL_WARN  = 2,
	FIB_LOG_LEVEL_ERROR = 3,
};

const char *fpm_nhg_fib_log_level_str(enum fib_log_level level);

/*
 * FIB library logging glue. Register the forwarder at module setup, before
 * any fib_LOG() can fire; fpm_nhg_fib_log_init() runs once the provider's
 * context exists: it binds the level field (written back by the CLI
 * commands), applies the current level, and installs the fib-log-level
 * commands.
 */
void fpm_nhg_fib_log_register(void);
void fpm_nhg_fib_log_init(enum fib_log_level *level);

#endif /* _FPM_NHG_H */
