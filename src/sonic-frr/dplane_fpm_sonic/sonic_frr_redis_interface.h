/*
 *  Copyright 2026 (c) Microsoft Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/**
 * @file sonic_frr_redis_interface.h
 * @brief Public interface for the SONiC FRR EVPN-MH dataplane provider.
 *
 * The provider is implemented as a standalone FRR zebra module. It publishes
 * SONiC EVPN-MH dataplane state through the orchagent ZMQ contract when enabled
 * and keeps APP_DB state through ZMQ persistence or direct Redis fallback while
 * allowing dataplane contexts to continue through the provider chain.
 */

#ifndef SONIC_FRR_REDIS_INTERFACE_H
#define SONIC_FRR_REDIS_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

struct event_loop;
struct zebra_dplane_ctx;

/** Opaque state for the SONiC FRR EVPN-MH dataplane provider. */
struct sonic_frr_redis_ctx;

/** Runtime counters exported by the EVPN-MH dataplane provider. */
struct sonic_frr_redis_counters {
	/** APP_DB Redis connection attempts. */
	uint32_t connects;
	/** APP_DB/ZMQ SET publish requests accepted successfully. */
	uint32_t sets;
	/** APP_DB/ZMQ DEL publish requests accepted successfully. */
	uint32_t dels;
	/** APP_DB/ZMQ connection, formatting, or publish errors. */
	uint32_t errors;
};

/**
 * Allocate Redis provider state.
 *
 * @param master FRR event loop used for Redis async socket integration.
 * @return Provider state, or NULL on allocation failure.
 */
struct sonic_frr_redis_ctx *sonic_frr_redis_new(struct event_loop *master);

/**
 * Cancel Redis provider events during zebra dataplane early shutdown.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_finish_early(struct sonic_frr_redis_ctx *ctx);

/**
 * Disconnect Redis and free provider state.
 *
 * @param ctx Redis provider state to free. NULL is ignored.
 */
void sonic_frr_redis_free(struct sonic_frr_redis_ctx *ctx);

/**
 * Enable or disable EVPN-MH publishing.
 *
 * @param ctx Redis provider state. NULL is ignored.
 * @param enabled True to publish EVPN-MH updates, false to suppress them.
 */
void sonic_frr_redis_set_enabled(struct sonic_frr_redis_ctx *ctx, bool enabled);

/**
 * Return whether EVPN-MH publishing is enabled.
 *
 * @param ctx Redis provider state. NULL is treated as disabled.
 * @return True when EVPN-MH publishing is enabled.
 */
bool sonic_frr_redis_is_enabled(const struct sonic_frr_redis_ctx *ctx);

/**
 * Return whether the async APP_DB Redis connection is established.
 *
 * @param ctx Redis provider state. NULL is treated as disconnected.
 * @return True when the APP_DB Redis connection is established.
 */
bool sonic_frr_redis_is_connected(const struct sonic_frr_redis_ctx *ctx);

/**
 * Disconnect the APP_DB Redis async context immediately.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_disconnect(struct sonic_frr_redis_ctx *ctx);

/**
 * Schedule Redis disconnect on the provider event loop.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_schedule_disconnect(struct sonic_frr_redis_ctx *ctx);

/**
 * Copy provider counters into @p counters.
 *
 * @param ctx Redis provider state. NULL produces zeroed counters.
 * @param counters Output counter snapshot. NULL is ignored.
 */
void sonic_frr_redis_get_counters(const struct sonic_frr_redis_ctx *ctx,
				  struct sonic_frr_redis_counters *counters);

/**
 * Reset provider counters to zero.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_reset_counters(struct sonic_frr_redis_ctx *ctx);

/**
 * Publish a bridge-port dataplane update when relevant.
 *
 * @param ctx Redis provider state. NULL or disabled state suppresses publish.
 * @param dplane_ctx Zebra dataplane context describing the bridge-port update.
 */
void sonic_frr_redis_publish_br_port_update(struct sonic_frr_redis_ctx *ctx,
					    const struct zebra_dplane_ctx *dplane_ctx);

#endif /* SONIC_FRR_REDIS_INTERFACE_H */
