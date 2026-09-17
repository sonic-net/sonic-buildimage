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
 * @file sonic_frr_redis_interface.c
 * @brief SONiC EVPN-MH dataplane provider for FRR zebra.
 *
 * This loadable FRR module registers a zebra dataplane postprocess provider
 * that observes bridge-port dataplane updates and publishes SONiC EVPN-MH
 * state to orchagent through the swss-common ZMQ table contract when enabled.
 * The ZMQ path uses DB persistence for APP_DB audit/replay. When ZMQ is not
 * enabled or cannot be initialized, the provider falls back to the direct async
 * Redis APP_DB publisher.
 *
 * The module is intentionally separate from the FPM/netlink plugin. It consumes
 * zebra dataplane contexts, emits EVPN-MH updates for the SONiC control-plane
 * state it owns, and then passes each dataplane context to the next provider in
 * the zebra dataplane pipeline.
 */

#ifdef HAVE_CONFIG_H
#include "config.h" /* Include this explicitly */
#endif

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#if defined(SONIC_FRR_REDIS_UNIT_TEST)
#include <stdatomic.h>
#else
#include <hiredis/async.h>
#include <hiredis/hiredis.h>
#include <swss/c-api/dbconnector.h>
#include <swss/c-api/zmqclient.h>
#include <swss/c-api/zmqproducerstatetable.h>
#include <swss/schema.h>
#endif

#include "sonic_frr_redis_interface.h"

#if !defined(SONIC_FRR_REDIS_UNIT_TEST)
#include "lib/frratomic.h"
#include "lib/json.h"
#include "lib/libfrr.h"
#include "lib/zebra.h"
#include "zebra/debug.h"
#include "zebra/interface.h"
#include "zebra/zebra_dplane.h"
#include "zebra/zebra_vxlan_private.h"
#endif

#define SONIC_FRR_REDIS_TIMEOUT_USEC 50000
#define SONIC_FRR_REDIS_MAX_PENDING 8192
#define SONIC_FRR_REDIS_MAX_SEPARATOR_STR 16
#define SONIC_FRR_REDIS_MAX_VTEPS_STR 4096
#define SONIC_FRR_REDIS_MAX_KEY_STR 256
#define SONIC_FRR_ZMQ_DEFAULT_PORT 8100
#define SONIC_FRR_ZMQ_ENDPOINT_STR 64
#define SONIC_FRR_CONFIG_DB_NAME "CONFIG_DB"
#define SONIC_FRR_DEVICE_METADATA_KEY "DEVICE_METADATA|localhost"
#define SONIC_FRR_EVPN_MH_ZMQ_ENABLED "orch_northbond_evpn_mh_zmq_enabled"

enum sonic_frr_redis_cmd_kind {
	SONIC_FRR_REDIS_CMD_SELECT,
	SONIC_FRR_REDIS_CMD_SET,
	SONIC_FRR_REDIS_CMD_DEL,
};

struct sonic_frr_redis_ctx {
	struct zebra_dplane_provider *provider;
	struct event_loop *master;
	redisAsyncContext *redis;
	SWSSDBConnector zmq_db;
	SWSSZmqClient zmq_client;
	SWSSZmqProducerStateTable zmq_shl_table;
	SWSSZmqProducerStateTable zmq_df_table;
	bool enabled;
	bool connecting;
	bool connected;
	bool zmq_ready;
	bool zmq_feature_checked;
	bool zmq_feature_enabled;
	bool closing;
	bool reading;
	bool writing;
	size_t pending;
	struct event *t_read;
	struct event *t_write;
	struct event *t_timer;
	struct event *t_disconnect;

	struct {
		_Atomic uint32_t connects;
		_Atomic uint32_t sets;
		_Atomic uint32_t dels;
		_Atomic uint32_t errors;
	} counters;
};

static const char *sonic_frr_redis_provider_name =
	"sonic_frr_redis_interface";
static struct sonic_frr_redis_ctx *g_redis_ctx;

static void sonic_frr_redis_read(struct event *t);
static void sonic_frr_redis_write(struct event *t);
static void sonic_frr_redis_timeout(struct event *t);
static void sonic_frr_redis_disconnect_event(struct event *t);

/**
 * Return a C string from a swss-common string handle.
 *
 * @param str swss-common string handle. NULL is treated as an empty string.
 * @return String data owned by @p str, or an empty string for NULL.
 */
static const char *sonic_frr_redis_string_c_str(SWSSString str)
{
	return str ? SWSSStrRef_c_str((SWSSStrRef)str) : "";
}

/**
 * Release strings embedded in a swss-common result object.
 *
 * @param result swss-common result whose message/location fields are released.
 */
static void sonic_frr_redis_result_free(SWSSResult *result)
{
	SWSSString_free(result->message);
	SWSSString_free(result->location);
	result->message = NULL;
	result->location = NULL;
}

/**
 * Check a swss-common C API result and log an error on failure.
 *
 * @param result swss-common C API result.
 * @param action Human-readable action being attempted.
 * @return True when @p result is successful.
 */
static bool sonic_frr_redis_result_ok(SWSSResult result, const char *action)
{
	if (result.exception == SWSSException_None)
		return true;

	zlog_warn("%s: %s failed%s%s%s%s", __func__, action,
		  result.location ? " at " : "",
		  sonic_frr_redis_string_c_str(result.location),
		  result.message ? ": " : "",
		  sonic_frr_redis_string_c_str(result.message));
	sonic_frr_redis_result_free(&result);
	return false;
}

/**
 * Build the local orchagent ZMQ endpoint for this namespace.
 *
 * @param endpoint Output buffer for the endpoint string.
 * @param endpoint_len Size of @p endpoint in bytes.
 * @return True when the endpoint was written without truncation.
 */
static bool sonic_frr_redis_zmq_endpoint(char *endpoint, size_t endpoint_len)
{
	const char *nsid = getenv("NAMESPACE_ID");
	char *endptr = NULL;
	long port = SONIC_FRR_ZMQ_DEFAULT_PORT;
	int rc;

	if (nsid && nsid[0] != '\0') {
		long namespace_id = strtol(nsid, &endptr, 10);

		if (endptr && *endptr == '\0' && namespace_id >= 0)
			port += namespace_id + 1;
		else
			zlog_warn("%s: invalid NAMESPACE_ID %s, using default ZMQ port",
				  __func__, nsid);
	}

	rc = snprintf(endpoint, endpoint_len, "tcp://localhost:%ld", port);
	return rc >= 0 && (size_t)rc < endpoint_len;
}

/**
 * Return whether EVPN-MH ZMQ publishing is enabled in CONFIG_DB.
 *
 * @param ctx Redis/ZMQ provider state.
 * @return True unless the feature flag is explicitly set to false.
 */
static bool sonic_frr_redis_zmq_feature_enabled(struct sonic_frr_redis_ctx *ctx)
{
	SWSSDBConnector config_db = NULL;
	SWSSString enabled = NULL;
	SWSSResult result;
	const char *enabled_str;

	if (!ctx)
		return false;
	if (ctx->zmq_feature_checked)
		return ctx->zmq_feature_enabled;

	ctx->zmq_feature_checked = true;
	ctx->zmq_feature_enabled = true;
	result = SWSSDBConnector_new_named(SONIC_FRR_CONFIG_DB_NAME, 0, 0,
			&config_db);
	if (!sonic_frr_redis_result_ok(result, "create CONFIG_DB connector"))
		return ctx->zmq_feature_enabled;

	result = SWSSDBConnector_hget(config_db, SONIC_FRR_DEVICE_METADATA_KEY,
			SONIC_FRR_EVPN_MH_ZMQ_ENABLED, &enabled);
	if (!sonic_frr_redis_result_ok(result, "read EVPN-MH ZMQ feature flag"))
		goto done;

	if (enabled) {
		enabled_str = sonic_frr_redis_string_c_str(enabled);
		ctx->zmq_feature_enabled = strcmp(enabled_str, "false") != 0;
		SWSSString_free(enabled);
	}

done:
	SWSSDBConnector_free(config_db);
	return ctx->zmq_feature_enabled;
}

/**
 * Resolve the APP_DB Redis Unix socket path from swss-common DB config.
 *
 * @param path Output buffer for the socket path.
 * @param path_len Size of @p path in bytes.
 * @return True when a non-empty socket path was written to @p path.
 */
static bool sonic_frr_redis_get_socket(char *path, size_t path_len)
{
	SWSSString sock = NULL;
	SWSSResult result;
	const char *sock_path;
	int rc;

	result = SWSSSonicDBConfig_getDbSock(APPL_DB_NAME, &sock);
	if (result.exception != SWSSException_None) {
		zlog_warn("%s: failed to resolve APP_DB Redis socket%s%s%s%s",
			  __func__, result.location ? " at " : "",
			  sonic_frr_redis_string_c_str(result.location),
			  result.message ? ": " : "",
			  sonic_frr_redis_string_c_str(result.message));
		sonic_frr_redis_result_free(&result);
		return false;
	}

	sock_path = sonic_frr_redis_string_c_str(sock);
	rc = snprintf(path, path_len, "%s", sock_path);
	SWSSString_free(sock);
	if (rc < 0 || (size_t)rc >= path_len) {
		zlog_warn("%s: APP_DB Redis socket path is too long", __func__);
		return false;
	}
	if (path[0] == '\0') {
		zlog_warn("%s: APP_DB Redis socket path is empty", __func__);
		return false;
	}

	return true;
}

/**
 * Resolve the APP_DB table separator from swss-common DB config.
 *
 * @param separator Output buffer for the separator string.
 * @param separator_len Size of @p separator in bytes.
 * @return True when a non-empty separator was written to @p separator.
 */
static bool sonic_frr_redis_get_separator(char *separator, size_t separator_len)
{
	SWSSString sep = NULL;
	SWSSResult result;
	const char *sep_value;
	int rc;

	result = SWSSSonicDBConfig_getSeparator(APPL_DB_NAME, &sep);
	if (result.exception != SWSSException_None) {
		zlog_warn("%s: failed to resolve APP_DB table separator%s%s%s%s",
			  __func__, result.location ? " at " : "",
			  sonic_frr_redis_string_c_str(result.location),
			  result.message ? ": " : "",
			  sonic_frr_redis_string_c_str(result.message));
		sonic_frr_redis_result_free(&result);
		return false;
	}

	sep_value = sonic_frr_redis_string_c_str(sep);
	rc = snprintf(separator, separator_len, "%s", sep_value);
	SWSSString_free(sep);
	if (rc < 0 || (size_t)rc >= separator_len) {
		zlog_warn("%s: APP_DB table separator is too long", __func__);
		return false;
	}
	if (separator[0] == '\0') {
		zlog_warn("%s: APP_DB table separator is empty", __func__);
		return false;
	}

	return true;
}

/**
 * Schedule a hiredis read event on the FRR event loop.
 *
 * @param ctx Redis provider state.
 */
static void sonic_frr_redis_schedule_read(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx || !ctx->redis || !ctx->reading || ctx->t_read)
		return;

	event_add_read(ctx->master, sonic_frr_redis_read, ctx, ctx->redis->c.fd,
		       &ctx->t_read);
}

/**
 * Schedule a hiredis write event on the FRR event loop.
 *
 * @param ctx Redis provider state.
 */
static void sonic_frr_redis_schedule_write(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx || !ctx->redis || !ctx->writing || ctx->t_write)
		return;

	event_add_write(ctx->master, sonic_frr_redis_write, ctx, ctx->redis->c.fd,
			&ctx->t_write);
}

/**
 * Hiredis adapter callback enabling read notifications.
 *
 * @param privdata Redis provider state passed to hiredis.
 */
static void sonic_frr_redis_add_read(void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	ctx->reading = true;
	sonic_frr_redis_schedule_read(ctx);
}

/**
 * Hiredis adapter callback disabling read notifications.
 *
 * @param privdata Redis provider state passed to hiredis.
 */
static void sonic_frr_redis_del_read(void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	ctx->reading = false;
	event_cancel(&ctx->t_read);
}

/**
 * Hiredis adapter callback enabling write notifications.
 *
 * @param privdata Redis provider state passed to hiredis.
 */
static void sonic_frr_redis_add_write(void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	ctx->writing = true;
	sonic_frr_redis_schedule_write(ctx);
}

/**
 * Hiredis adapter callback disabling write notifications.
 *
 * @param privdata Redis provider state passed to hiredis.
 */
static void sonic_frr_redis_del_write(void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	ctx->writing = false;
	event_cancel(&ctx->t_write);
}

/**
 * Hiredis adapter callback scheduling command/connect timeout handling.
 *
 * @param privdata Redis provider state passed to hiredis.
 * @param tv Timeout requested by hiredis.
 */
static void sonic_frr_redis_schedule_timer(void *privdata, struct timeval tv)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	event_cancel(&ctx->t_timer);
	if (!ctx->redis)
		return;

	event_add_timer_tv(ctx->master, sonic_frr_redis_timeout, ctx, &tv,
			   &ctx->t_timer);
}

/**
 * Hiredis adapter callback cleaning up registered FRR events.
 *
 * @param privdata Redis provider state passed to hiredis.
 */
static void sonic_frr_redis_cleanup(void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = privdata;

	if (!ctx)
		return;

	ctx->reading = false;
	ctx->writing = false;
	event_cancel(&ctx->t_read);
	event_cancel(&ctx->t_write);
	event_cancel(&ctx->t_timer);
}

/**
 * Attach hiredis async callbacks to the FRR event loop adapter.
 *
 * @param ctx Redis provider state.
 * @param redis Hiredis async context to attach.
 * @return REDIS_OK on success, REDIS_ERR when the context is already attached.
 */
static int sonic_frr_redis_attach(struct sonic_frr_redis_ctx *ctx,
			      redisAsyncContext *redis)
{
	if (redis->ev.data)
		return REDIS_ERR;

	redis->ev.addRead = sonic_frr_redis_add_read;
	redis->ev.delRead = sonic_frr_redis_del_read;
	redis->ev.addWrite = sonic_frr_redis_add_write;
	redis->ev.delWrite = sonic_frr_redis_del_write;
	redis->ev.cleanup = sonic_frr_redis_cleanup;
	redis->ev.scheduleTimer = sonic_frr_redis_schedule_timer;
	redis->ev.data = ctx;
	return REDIS_OK;
}

/**
 * Release ZMQ producer resources.
 *
 * @param ctx Redis/ZMQ provider state. NULL is ignored.
 */
static void sonic_frr_redis_zmq_disconnect(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx)
		return;

	if (ctx->zmq_shl_table)
		SWSSZmqProducerStateTable_free(ctx->zmq_shl_table);
	if (ctx->zmq_df_table)
		SWSSZmqProducerStateTable_free(ctx->zmq_df_table);
	if (ctx->zmq_client)
		SWSSZmqClient_free(ctx->zmq_client);
	if (ctx->zmq_db)
		SWSSDBConnector_free(ctx->zmq_db);

	ctx->zmq_shl_table = NULL;
	ctx->zmq_df_table = NULL;
	ctx->zmq_client = NULL;
	ctx->zmq_db = NULL;
	ctx->zmq_ready = false;
}

/**
 * Return the ZMQ producer table for an APP_DB table name.
 *
 * @param ctx Redis/ZMQ provider state.
 * @param table APP_DB table name.
 * @return Matching ZMQ producer table, or NULL when unsupported.
 */
static SWSSZmqProducerStateTable
sonic_frr_redis_zmq_table(struct sonic_frr_redis_ctx *ctx, const char *table)
{
	if (strcmp(table, APP_EVPN_SPLIT_HORIZON_TABLE_NAME) == 0)
		return ctx->zmq_shl_table;
	if (strcmp(table, APP_EVPN_DF_TABLE_NAME) == 0)
		return ctx->zmq_df_table;

	return NULL;
}

/**
 * Initialize ZMQ producers for EVPN-MH APP_DB tables.
 *
 * Database persistence is enabled so the ZMQ apply path also maintains APP_DB
 * state asynchronously for audit and replay.
 *
 * @param ctx Redis/ZMQ provider state.
 * @return True when the ZMQ producers are ready.
 */
static bool sonic_frr_redis_zmq_connect(struct sonic_frr_redis_ctx *ctx)
{
	SWSSResult result;
	char endpoint[SONIC_FRR_ZMQ_ENDPOINT_STR];

	if (!ctx || ctx->zmq_ready)
		return ctx && ctx->zmq_ready;
	if (!sonic_frr_redis_zmq_feature_enabled(ctx))
		return false;

	if (!sonic_frr_redis_zmq_endpoint(endpoint, sizeof(endpoint))) {
		zlog_warn("%s: orchagent ZMQ endpoint is too long", __func__);
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return false;
	}

	result = SWSSDBConnector_new_named(APPL_DB_NAME, 0, 0, &ctx->zmq_db);
	if (!sonic_frr_redis_result_ok(result, "create APP_DB connector"))
		goto error;

	result = SWSSZmqClient_new(endpoint, &ctx->zmq_client);
	if (!sonic_frr_redis_result_ok(result, "create orchagent ZMQ client"))
		goto error;

	result = SWSSZmqClient_connect(ctx->zmq_client);
	if (!sonic_frr_redis_result_ok(result, "connect orchagent ZMQ client"))
		goto error;

	result = SWSSZmqProducerStateTable_new(ctx->zmq_db,
			APP_EVPN_SPLIT_HORIZON_TABLE_NAME, ctx->zmq_client, 1,
			&ctx->zmq_shl_table);
	if (!sonic_frr_redis_result_ok(result, "create split-horizon ZMQ producer"))
		goto error;

	result = SWSSZmqProducerStateTable_new(ctx->zmq_db, APP_EVPN_DF_TABLE_NAME,
			ctx->zmq_client, 1, &ctx->zmq_df_table);
	if (!sonic_frr_redis_result_ok(result, "create DF ZMQ producer"))
		goto error;

	ctx->zmq_ready = true;
	return true;

error:
	atomic_fetch_add_explicit(&ctx->counters.errors, 1, memory_order_relaxed);
	sonic_frr_redis_zmq_disconnect(ctx);
	return false;
}

/**
 * Disconnect the APP_DB Redis async context immediately.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_disconnect(struct sonic_frr_redis_ctx *ctx)
{
	redisAsyncContext *redis;

	if (!ctx || !ctx->redis)
		return;

	redis = ctx->redis;
	ctx->closing = true;
	ctx->redis = NULL;
	ctx->connected = false;
	ctx->connecting = false;
	ctx->reading = false;
	ctx->writing = false;
	ctx->pending = 0;
	redisAsyncFree(redis);
	ctx->closing = false;
}

/**
 * Schedule Redis disconnect on the provider event loop.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_schedule_disconnect(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx)
		return;

	event_add_event(ctx->master, sonic_frr_redis_disconnect_event, ctx, 0,
			&ctx->t_disconnect);
}

/**
 * FRR event callback that disconnects the Redis async context.
 *
 * @param t FRR event carrying Redis provider state.
 */
static void sonic_frr_redis_disconnect_event(struct event *t)
{
	struct sonic_frr_redis_ctx *ctx = EVENT_ARG(t);

	sonic_frr_redis_disconnect(ctx);
}

/**
 * Hiredis connection-complete callback.
 *
 * @param redis Hiredis async context that completed connection handling.
 * @param status REDIS_OK on successful connection, otherwise an error status.
 */
static void sonic_frr_redis_connect_cb(const redisAsyncContext *redis, int status)
{
	struct sonic_frr_redis_ctx *ctx = redis->data;

	if (!ctx)
		return;

	ctx->connecting = false;
	if (status != REDIS_OK) {
		if (!ctx->closing) {
			zlog_warn("%s: failed to connect to APP_DB Redis socket: %s",
				  __func__, redis->errstr);
			atomic_fetch_add_explicit(&ctx->counters.errors, 1,
						      memory_order_relaxed);
		}
		if (ctx->redis == redis)
			ctx->redis = NULL;
		ctx->connected = false;
		return;
	}

	ctx->connected = true;
}

/**
 * Hiredis disconnect callback.
 *
 * @param redis Hiredis async context that disconnected.
 * @param status REDIS_OK for expected disconnects, otherwise an error status.
 */
static void sonic_frr_redis_disconnect_cb(const redisAsyncContext *redis, int status)
{
	struct sonic_frr_redis_ctx *ctx = redis->data;

	if (!ctx)
		return;

	if (status != REDIS_OK && !ctx->closing) {
		zlog_warn("%s: APP_DB Redis connection closed: %s", __func__,
			  redis->errstr);
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
	}

	if (ctx->redis == redis)
		ctx->redis = NULL;
	ctx->connected = false;
	ctx->connecting = false;
	ctx->reading = false;
	ctx->writing = false;
}

/**
 * Hiredis command completion callback.
 *
 * @param redis Hiredis async context that completed the command.
 * @param reply Redis reply object provided by hiredis. May be NULL on failure.
 * @param privdata Command kind encoded as enum sonic_frr_redis_cmd_kind.
 */
static void sonic_frr_redis_command_cb(redisAsyncContext *redis, void *reply,
				   void *privdata)
{
	struct sonic_frr_redis_ctx *ctx = redis->data;
	redisReply *redis_reply = reply;
	enum sonic_frr_redis_cmd_kind kind = (uintptr_t)privdata;

	if (!ctx)
		return;

	if (ctx->pending > 0)
		ctx->pending--;

	if (ctx->closing)
		return;

	if (!redis_reply || redis_reply->type == REDIS_REPLY_ERROR) {
		zlog_warn("%s: APP_DB async publish failed: %s", __func__,
			  redis_reply && redis_reply->str ? redis_reply->str :
			  (redis->errstr ? redis->errstr : "no reply"));
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		if (ctx->redis == redis)
			redisAsyncDisconnect(redis);
		return;
	}

	if (kind == SONIC_FRR_REDIS_CMD_SET)
		atomic_fetch_add_explicit(&ctx->counters.sets, 1,
					      memory_order_relaxed);
	else if (kind == SONIC_FRR_REDIS_CMD_DEL)
		atomic_fetch_add_explicit(&ctx->counters.dels, 1,
					      memory_order_relaxed);
}

/**
 * FRR read event callback for the hiredis socket.
 *
 * @param t FRR event carrying Redis provider state.
 */
static void sonic_frr_redis_read(struct event *t)
{
	struct sonic_frr_redis_ctx *ctx = EVENT_ARG(t);
	redisAsyncContext *redis = ctx->redis;

	if (!redis)
		return;

	redisAsyncHandleRead(redis);
	if (ctx->redis == redis)
		sonic_frr_redis_schedule_read(ctx);
}

/**
 * FRR write event callback for the hiredis socket.
 *
 * @param t FRR event carrying Redis provider state.
 */
static void sonic_frr_redis_write(struct event *t)
{
	struct sonic_frr_redis_ctx *ctx = EVENT_ARG(t);
	redisAsyncContext *redis = ctx->redis;

	if (!redis)
		return;

	redisAsyncHandleWrite(redis);
	if (ctx->redis == redis)
		sonic_frr_redis_schedule_write(ctx);
}

/**
 * FRR timer callback for hiredis timeout handling.
 *
 * @param t FRR event carrying Redis provider state.
 */
static void sonic_frr_redis_timeout(struct event *t)
{
	struct sonic_frr_redis_ctx *ctx = EVENT_ARG(t);
	redisAsyncContext *redis = ctx->redis;

	if (!redis)
		return;

	redisAsyncHandleTimeout(redis);
}

/**
 * Queue a Redis command on the async APP_DB connection.
 *
 * @param ctx Redis provider state.
 * @param kind Command kind used for provider counters.
 * @param argc Number of Redis command arguments.
 * @param argv Redis command argument array.
 * @param argvlen Redis command argument lengths.
 * @return True when the command was queued successfully.
 */
static bool sonic_frr_redis_queue_command(struct sonic_frr_redis_ctx *ctx,
				       enum sonic_frr_redis_cmd_kind kind, int argc,
				       const char **argv, const size_t *argvlen)
{
	if (!ctx->redis || ctx->redis->err)
		return false;

	if (ctx->pending >= SONIC_FRR_REDIS_MAX_PENDING) {
		zlog_warn("%s: APP_DB async queue full, dropping publish request",
			  __func__);
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return false;
	}

	if (redisAsyncCommandArgv(ctx->redis, sonic_frr_redis_command_cb,
				  (void *)(uintptr_t)kind, argc, argv,
				  argvlen) != REDIS_OK) {
		zlog_warn("%s: failed to queue APP_DB Redis command: %s", __func__,
			  ctx->redis->errstr);
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		redisAsyncDisconnect(ctx->redis);
		return false;
	}

	ctx->pending++;
	return true;
}

/**
 * Ensure the async APP_DB Redis connection is available.
 *
 * @param ctx Redis provider state.
 * @return True when Redis is connected or a connection attempt was queued.
 */
static bool sonic_frr_redis_connect(struct sonic_frr_redis_ctx *ctx)
{
	redisOptions options = {0};
	struct timeval timeout = {0, SONIC_FRR_REDIS_TIMEOUT_USEC};
	redisAsyncContext *redis;
	char app_db_sock[PATH_MAX];
	char app_db_id[16];
	const char *argv[] = {"SELECT", app_db_id};
	size_t argvlen[] = {strlen("SELECT"), 0};

	if (!sonic_frr_redis_is_enabled(ctx))
		return false;

	if (ctx->redis && !ctx->redis->err)
		return true;

	sonic_frr_redis_disconnect(ctx);
	atomic_fetch_add_explicit(&ctx->counters.connects, 1,
				      memory_order_relaxed);
	if (!sonic_frr_redis_get_socket(app_db_sock, sizeof(app_db_sock))) {
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return false;
	}
	snprintf(app_db_id, sizeof(app_db_id), "%d", APPL_DB);
	argvlen[1] = strlen(app_db_id);

	REDIS_OPTIONS_SET_UNIX(&options, app_db_sock);
	options.connect_timeout = &timeout;
	options.command_timeout = &timeout;
	redis = redisAsyncConnectWithOptions(&options);
	if (!redis || redis->err) {
		zlog_warn("%s: failed to connect to APP_DB Redis socket %s: %s",
			  __func__, app_db_sock,
			  redis ? redis->errstr : "no context");
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		if (redis)
			redisAsyncFree(redis);
		return false;
	}

	ctx->redis = redis;
	ctx->connecting = true;
	ctx->connected = false;
	redis->data = ctx;

	if (sonic_frr_redis_attach(ctx, redis) != REDIS_OK ||
	    redisAsyncSetConnectCallback(redis, sonic_frr_redis_connect_cb) != REDIS_OK ||
	    redisAsyncSetDisconnectCallback(redis, sonic_frr_redis_disconnect_cb) !=
		    REDIS_OK) {
		zlog_warn("%s: failed to attach APP_DB Redis async context", __func__);
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		sonic_frr_redis_disconnect(ctx);
		return false;
	}

	return sonic_frr_redis_queue_command(ctx, SONIC_FRR_REDIS_CMD_SELECT, 2, argv,
				       argvlen);
}

/**
 * Queue a Redis EVAL command against APP_DB.
 *
 * @param ctx Redis provider state.
 * @param kind Command kind used for provider counters.
 * @param script Lua script to execute.
 * @param key_count Number of Redis KEYS entries.
 * @param keys Redis KEYS argument array.
 * @param arg_count Number of Redis ARGV entries.
 * @param args Redis ARGV argument array.
 * @return True when the EVAL command was queued successfully.
 */
static bool sonic_frr_redis_eval(struct sonic_frr_redis_ctx *ctx,
			     enum sonic_frr_redis_cmd_kind kind, const char *script,
			     int key_count, const char * const *keys,
			     int arg_count, const char * const *args)
{
	const int max_items = 1 + 1 + 1 + 8 + 8;
	const char *argv[max_items];
	size_t argvlen[max_items];
	char key_count_str[16];
	int argc = 0;

	if (key_count > 8 || arg_count > 8)
		return false;

	if (!sonic_frr_redis_connect(ctx))
		return false;

	snprintf(key_count_str, sizeof(key_count_str), "%d", key_count);
	argv[argc] = "EVAL";
	argvlen[argc++] = strlen("EVAL");
	argv[argc] = script;
	argvlen[argc++] = strlen(script);
	argv[argc] = key_count_str;
	argvlen[argc++] = strlen(key_count_str);

	for (int i = 0; i < key_count; i++) {
		argv[argc] = keys[i];
		argvlen[argc++] = strlen(keys[i]);
	}
	for (int i = 0; i < arg_count; i++) {
		argv[argc] = args[i];
		argvlen[argc++] = strlen(args[i]);
	}

	return sonic_frr_redis_queue_command(ctx, kind, argc, argv, argvlen);
}

/**
 * Build ProducerStateTable-compatible Redis key names for a table/key pair.
 *
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 * @param channel Output buffer for the table channel name.
 * @param channel_len Size of @p channel in bytes.
 * @param key_set Output buffer for the key-set name.
 * @param key_set_len Size of @p key_set in bytes.
 * @param state_key Output buffer for the state hash key.
 * @param state_key_len Size of @p state_key in bytes.
 * @param del_set Output buffer for the delete-set name.
 * @param del_set_len Size of @p del_set in bytes.
 * @return True when all names were written without truncation.
 */
static bool sonic_frr_redis_key_names(const char *table, const char *key,
				  char *channel, size_t channel_len,
				  char *key_set, size_t key_set_len,
				  char *state_key, size_t state_key_len,
				  char *del_set, size_t del_set_len)
{
	char separator[SONIC_FRR_REDIS_MAX_SEPARATOR_STR];
	int rc;

	if (!sonic_frr_redis_get_separator(separator, sizeof(separator)))
		return false;

	rc = snprintf(channel, channel_len, "%s%s%s%d", table,
		      SWSS_TABLE_CHANNEL_SUFFIX, SWSS_TABLE_CHANNEL_DB_SEPARATOR,
		      APPL_DB);
	if (rc < 0 || (size_t)rc >= channel_len)
		return false;
	rc = snprintf(key_set, key_set_len, "%s%s", table,
		      SWSS_TABLE_KEY_SET_SUFFIX);
	if (rc < 0 || (size_t)rc >= key_set_len)
		return false;
	rc = snprintf(state_key, state_key_len, "%s%s%s%s",
		      SWSS_TABLE_STATE_HASH_PREFIX, table, separator, key);
	if (rc < 0 || (size_t)rc >= state_key_len)
		return false;
	rc = snprintf(del_set, del_set_len, "%s%s", table,
		      SWSS_TABLE_DEL_SET_SUFFIX);
	if (rc < 0 || (size_t)rc >= del_set_len)
		return false;

	return true;
}

/**
 * Publish or update one field in an APP_DB table object.
 *
 * @param ctx Redis provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 * @param field Field name to set.
 * @param value Field value to set.
 * @return True when the Redis update command was queued successfully.
 */
static bool sonic_frr_redis_set_field(struct sonic_frr_redis_ctx *ctx, const char *table,
				  const char *key, const char *field,
				  const char *value)
{
	static const char lua_set[] =
		"local added = redis.call('SADD', KEYS[2], ARGV[2])\n"
		"redis.call('HSET', KEYS[3], ARGV[3], ARGV[4])\n"
		"if added > 0 then redis.call('PUBLISH', KEYS[1], ARGV[1]) end\n";
	char channel[SONIC_FRR_REDIS_MAX_KEY_STR];
	char key_set[SONIC_FRR_REDIS_MAX_KEY_STR];
	char state_key[SONIC_FRR_REDIS_MAX_KEY_STR];
	char del_set[SONIC_FRR_REDIS_MAX_KEY_STR];
	const char *keys[] = {channel, key_set, state_key};
	const char *args[] = {SWSS_TABLE_UPDATE_NOTIFICATION, key, field, value};

	if (!sonic_frr_redis_key_names(table, key, channel, sizeof(channel), key_set,
				  sizeof(key_set), state_key, sizeof(state_key),
				  del_set, sizeof(del_set))) {
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return false;
	}

	return sonic_frr_redis_eval(ctx, SONIC_FRR_REDIS_CMD_SET, lua_set, 3, keys, 4,
				 args);
}

/**
 * Delete one object from an APP_DB table.
 *
 * @param ctx Redis provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key to delete.
 * @return True when the Redis delete command was queued successfully.
 */
static bool sonic_frr_redis_del_key(struct sonic_frr_redis_ctx *ctx, const char *table,
				const char *key)
{
	static const char lua_del[] =
		"local added = redis.call('SADD', KEYS[2], ARGV[2])\n"
		"redis.call('SADD', KEYS[4], ARGV[2])\n"
		"redis.call('DEL', KEYS[3])\n"
		"if added > 0 then redis.call('PUBLISH', KEYS[1], ARGV[1]) end\n";
	char channel[SONIC_FRR_REDIS_MAX_KEY_STR];
	char key_set[SONIC_FRR_REDIS_MAX_KEY_STR];
	char state_key[SONIC_FRR_REDIS_MAX_KEY_STR];
	char del_set[SONIC_FRR_REDIS_MAX_KEY_STR];
	const char *keys[] = {channel, key_set, state_key, del_set};
	const char *args[] = {SWSS_TABLE_UPDATE_NOTIFICATION, key, "''", "''"};

	if (!sonic_frr_redis_key_names(table, key, channel, sizeof(channel), key_set,
				  sizeof(key_set), state_key, sizeof(state_key),
				  del_set, sizeof(del_set))) {
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return false;
	}

	return sonic_frr_redis_eval(ctx, SONIC_FRR_REDIS_CMD_DEL, lua_del, 4, keys, 4,
				 args);
}

/**
 * Publish one APP_DB field update over ZMQ.
 *
 * @param ctx Redis/ZMQ provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 * @param field Field name to set.
 * @param value Field value to set.
 * @return True when the ZMQ update was accepted.
 */
static bool sonic_frr_redis_zmq_set_field(struct sonic_frr_redis_ctx *ctx,
					  const char *table, const char *key,
					  const char *field, const char *value)
{
	SWSSZmqProducerStateTable zmq_table;
	SWSSFieldValueTuple tuple;
	SWSSFieldValueArray values;
	SWSSResult result;
	bool ok;

	if (!sonic_frr_redis_zmq_connect(ctx))
		return false;

	zmq_table = sonic_frr_redis_zmq_table(ctx, table);
	if (!zmq_table)
		return false;

	tuple.field = field;
	tuple.value = SWSSString_new_c_str(value);
	values.len = 1;
	values.data = &tuple;

	result = SWSSZmqProducerStateTable_set(zmq_table, key, values);
	ok = sonic_frr_redis_result_ok(result, "publish ZMQ SET");
	SWSSString_free(tuple.value);

	if (ok)
		atomic_fetch_add_explicit(&ctx->counters.sets, 1,
					      memory_order_relaxed);
	else
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);

	return ok;
}

/**
 * Publish one APP_DB delete over ZMQ.
 *
 * @param ctx Redis/ZMQ provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 * @return True when the ZMQ delete was accepted.
 */
static bool sonic_frr_redis_zmq_del_key(struct sonic_frr_redis_ctx *ctx,
					const char *table, const char *key)
{
	SWSSZmqProducerStateTable zmq_table;
	SWSSResult result;
	bool ok;

	if (!sonic_frr_redis_zmq_connect(ctx))
		return false;

	zmq_table = sonic_frr_redis_zmq_table(ctx, table);
	if (!zmq_table)
		return false;

	result = SWSSZmqProducerStateTable_del(zmq_table, key);
	ok = sonic_frr_redis_result_ok(result, "publish ZMQ DEL");
	if (ok)
		atomic_fetch_add_explicit(&ctx->counters.dels, 1,
					      memory_order_relaxed);
	else
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);

	return ok;
}

/**
 * Publish or update one field, preferring ZMQ and falling back to Redis.
 *
 * @param ctx Redis/ZMQ provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 * @param field Field name to set.
 * @param value Field value to set.
 */
static void sonic_frr_redis_publish_set_field(struct sonic_frr_redis_ctx *ctx,
					      const char *table, const char *key,
					      const char *field, const char *value)
{
	if (sonic_frr_redis_zmq_set_field(ctx, table, key, field, value))
		return;

	sonic_frr_redis_set_field(ctx, table, key, field, value);
}

/**
 * Delete one object, preferring ZMQ and falling back to Redis.
 *
 * @param ctx Redis/ZMQ provider state.
 * @param table APP_DB table name.
 * @param key APP_DB object key.
 */
static void sonic_frr_redis_publish_del_key(struct sonic_frr_redis_ctx *ctx,
					    const char *table, const char *key)
{
	if (sonic_frr_redis_zmq_del_key(ctx, table, key))
		return;

	sonic_frr_redis_del_key(ctx, table, key);
}

/**
 * Build the APP_DB key for a bridge-port dataplane context.
 *
 * @param dplane_ctx Zebra dataplane context containing bridge-port metadata.
 * @param key Output buffer for the APP_DB object key.
 * @param key_len Size of @p key in bytes.
 * @return True when the key was written without truncation.
 */
static bool sonic_frr_redis_br_port_key(const struct zebra_dplane_ctx *dplane_ctx,
				    char *key, size_t key_len)
{
	struct interface *ifp;
	const char *ifname = "unknown";
	int rc;

	ifp = if_lookup_by_index(dplane_ctx_get_ifindex(dplane_ctx), VRF_DEFAULT);
	if (ifp)
		ifname = ifp->name;

	rc = snprintf(key, key_len, "Vlan%u:%s",
		      dplane_ctx_get_br_port_vlan_id(dplane_ctx), ifname);
	return rc >= 0 && (size_t)rc < key_len;
}

/**
 * Build the comma-separated split-horizon VTEP list for APP_DB.
 *
 * @param dplane_ctx Zebra dataplane context containing split-horizon filters.
 * @param vteps Output buffer for the comma-separated VTEP list.
 * @param vteps_len Size of @p vteps in bytes.
 * @return True when the VTEP list was written without truncation.
 */
static bool sonic_frr_redis_build_shl_vteps(const struct zebra_dplane_ctx *dplane_ctx,
					char *vteps, size_t vteps_len)
{
	const struct ipaddr *sph_filters;
	char ip_addr_str[INET6_ADDRSTRLEN];
	size_t i;

	vteps[0] = '\0';
	sph_filters = dplane_ctx_get_br_port_sph_filters(dplane_ctx);
	for (i = 0; i < dplane_ctx_get_br_port_sph_filter_cnt(dplane_ctx); i++) {
		if (IS_IPADDR_V4(&sph_filters[i]))
			inet_ntop(AF_INET, &sph_filters[i].ipaddr_v4, ip_addr_str,
				  sizeof(ip_addr_str));
		else if (IS_IPADDR_V6(&sph_filters[i]))
			inet_ntop(AF_INET6, &sph_filters[i].ipaddr_v6, ip_addr_str,
				  sizeof(ip_addr_str));
		else
			continue;

		if (vteps[0] != '\0')
			strlcat(vteps, ",", vteps_len);
		strlcat(vteps, ip_addr_str, vteps_len);
		if (strlen(vteps) >= vteps_len - 1)
			return false;
	}

	return true;
}

/**
 * Publish a bridge-port dataplane update to APP_DB when relevant.
 *
 * @param ctx Redis provider state. NULL or disabled state suppresses publish.
 * @param dplane_ctx Zebra dataplane context describing the bridge-port update.
 */
void sonic_frr_redis_publish_br_port_update(struct sonic_frr_redis_ctx *ctx,
					const struct zebra_dplane_ctx *dplane_ctx)
{
	enum dplane_op_e op = dplane_ctx_get_op(dplane_ctx);
	uint16_t vlan_id = dplane_ctx_get_br_port_vlan_id(dplane_ctx);
	char key[SONIC_FRR_REDIS_MAX_KEY_STR];
	char vteps[SONIC_FRR_REDIS_MAX_VTEPS_STR];
	const char *df_state;

	if (!sonic_frr_redis_is_enabled(ctx))
		return;

	if (!sonic_frr_redis_br_port_key(dplane_ctx, key, sizeof(key))) {
		atomic_fetch_add_explicit(&ctx->counters.errors, 1,
					      memory_order_relaxed);
		return;
	}

	/* fpmsyncd ignores VLAN 4095 port-specific SHL/DF records today. */
	if (vlan_id != 0xFFF) {
		if (op == DPLANE_OP_BR_PORT_UPDATE) {
			if (!sonic_frr_redis_build_shl_vteps(dplane_ctx, vteps,
						       sizeof(vteps))) {
				zlog_warn("%s: SHL VTEP list too large for %s",
					  __func__, key);
				atomic_fetch_add_explicit(&ctx->counters.errors, 1,
						      memory_order_relaxed);
			} else {
				sonic_frr_redis_publish_set_field(ctx,
						      APP_EVPN_SPLIT_HORIZON_TABLE_NAME,
						      key, "vteps", vteps);
			}

			df_state = (dplane_ctx_get_br_port_flags(dplane_ctx) &
				    DPLANE_BR_PORT_NON_DF) ? "false" : "true";
			sonic_frr_redis_publish_set_field(ctx, APP_EVPN_DF_TABLE_NAME, key, "df",
					      df_state);
		} else if (op == DPLANE_OP_BR_PORT_DELETE) {
			sonic_frr_redis_publish_del_key(ctx, APP_EVPN_SPLIT_HORIZON_TABLE_NAME,
					    key);
			sonic_frr_redis_publish_del_key(ctx, APP_EVPN_DF_TABLE_NAME, key);
		}
	}

}

/**
 * Allocate Redis provider state.
 *
 * @param master FRR event loop used for Redis async socket integration.
 * @return Provider state, or NULL on allocation failure.
 */
struct sonic_frr_redis_ctx *sonic_frr_redis_new(struct event_loop *master)
{
	struct sonic_frr_redis_ctx *ctx;

	ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return NULL;

	ctx->master = master;
	return ctx;
}

/**
 * Cancel Redis provider events during zebra dataplane early shutdown.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_finish_early(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx || !ctx->master)
		return;

	event_cancel_async(ctx->master, &ctx->t_read, NULL);
	event_cancel_async(ctx->master, &ctx->t_write, NULL);
	event_cancel_async(ctx->master, &ctx->t_timer, NULL);
	event_cancel_async(ctx->master, &ctx->t_disconnect, NULL);
}

/**
 * Disconnect Redis and free provider state.
 *
 * @param ctx Redis provider state to free. NULL is ignored.
 */
void sonic_frr_redis_free(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx)
		return;

	sonic_frr_redis_zmq_disconnect(ctx);
	sonic_frr_redis_disconnect(ctx);
	free(ctx);
}

/**
 * Enable or disable APP_DB publishing.
 *
 * @param ctx Redis provider state. NULL is ignored.
 * @param enabled True to publish APP_DB updates, false to suppress them.
 */
void sonic_frr_redis_set_enabled(struct sonic_frr_redis_ctx *ctx, bool enabled)
{
	if (ctx)
		ctx->enabled = enabled;
}

/**
 * Return whether APP_DB publishing is enabled.
 *
 * @param ctx Redis provider state. NULL is treated as disabled.
 * @return True when APP_DB publishing is enabled.
 */
bool sonic_frr_redis_is_enabled(const struct sonic_frr_redis_ctx *ctx)
{
	return ctx && ctx->enabled;
}

/**
 * Return whether the async APP_DB Redis connection is established.
 *
 * @param ctx Redis provider state. NULL is treated as disconnected.
 * @return True when the APP_DB Redis connection is established.
 */
bool sonic_frr_redis_is_connected(const struct sonic_frr_redis_ctx *ctx)
{
	return ctx && ctx->connected;
}

/**
 * Copy Redis provider counters into @p counters.
 *
 * @param ctx Redis provider state. NULL produces zeroed counters.
 * @param counters Output counter snapshot. NULL is ignored.
 */
void sonic_frr_redis_get_counters(const struct sonic_frr_redis_ctx *ctx,
				      struct sonic_frr_redis_counters *counters)
{
	if (!counters)
		return;

	memset(counters, 0, sizeof(*counters));
	if (!ctx)
		return;

	counters->connects = atomic_load_explicit(&ctx->counters.connects,
						    memory_order_relaxed);
	counters->sets = atomic_load_explicit(&ctx->counters.sets,
						memory_order_relaxed);
	counters->dels = atomic_load_explicit(&ctx->counters.dels,
						memory_order_relaxed);
	counters->errors = atomic_load_explicit(&ctx->counters.errors,
						  memory_order_relaxed);
}

/**
 * Reset Redis provider counters to zero.
 *
 * @param ctx Redis provider state. NULL is ignored.
 */
void sonic_frr_redis_reset_counters(struct sonic_frr_redis_ctx *ctx)
{
	if (!ctx)
		return;

	atomic_store_explicit(&ctx->counters.connects, 0, memory_order_relaxed);
	atomic_store_explicit(&ctx->counters.sets, 0, memory_order_relaxed);
	atomic_store_explicit(&ctx->counters.dels, 0, memory_order_relaxed);
	atomic_store_explicit(&ctx->counters.errors, 0, memory_order_relaxed);
}

/**
 * Start callback for the Redis dataplane provider.
 *
 * @param provider Zebra dataplane provider being started.
 * @return Zero on success.
 */
static int sonic_frr_redis_start(struct zebra_dplane_provider *provider)
{
	struct sonic_frr_redis_ctx *ctx = dplane_provider_get_data(provider);

	ctx->provider = provider;
	ctx->master = dplane_get_thread_master();
	ctx->enabled = true;
	return 0;
}

/**
 * Process dataplane contexts offered to the Redis provider.
 *
 * @param provider Zebra dataplane provider with pending input contexts.
 * @return Zero on success.
 */
static int sonic_frr_redis_process(struct zebra_dplane_provider *provider)
{
	struct sonic_frr_redis_ctx *ctx = dplane_provider_get_data(provider);
	struct zebra_dplane_ctx *dplane_ctx;
	int counter;
	int limit = dplane_provider_get_work_limit(provider);

	for (counter = 0; counter < limit; counter++) {
		dplane_ctx = dplane_provider_dequeue_in_ctx(provider);
		if (!dplane_ctx)
			break;

		if (dplane_ctx_get_op(dplane_ctx) == DPLANE_OP_BR_PORT_UPDATE ||
		    dplane_ctx_get_op(dplane_ctx) == DPLANE_OP_BR_PORT_DELETE)
			sonic_frr_redis_publish_br_port_update(ctx, dplane_ctx);

		dplane_provider_enqueue_out_ctx(provider, dplane_ctx);
	}

	return 0;
}

/**
 * Finish callback for the Redis dataplane provider.
 *
 * @param provider Zebra dataplane provider being stopped.
 * @param early True during early shutdown while dataplane work can drain.
 * @return Zero on success.
 */
static int sonic_frr_redis_finish(struct zebra_dplane_provider *provider,
				  bool early)
{
	struct sonic_frr_redis_ctx *ctx = dplane_provider_get_data(provider);

	if (early) {
		sonic_frr_redis_finish_early(ctx);
		return 0;
	}

	sonic_frr_redis_free(ctx);
	g_redis_ctx = NULL;
	return 0;
}

/**
 * Register the Redis dataplane provider during FRR late init.
 *
 * @param master FRR main event loop. Unused; provider uses dataplane loop.
 * @return Zero on success, or an errno-compatible error value.
 */
static int sonic_frr_redis_provider_new(struct event_loop *master)
{
	struct zebra_dplane_provider *provider = NULL;
	int result;

	(void)master;

	g_redis_ctx = sonic_frr_redis_new(NULL);
	if (!g_redis_ctx)
		return ENOMEM;

	result = dplane_provider_register(sonic_frr_redis_provider_name,
					  DPLANE_PRIO_POSTPROCESS,
					  DPLANE_PROV_FLAGS_DEFAULT,
					  sonic_frr_redis_start,
					  sonic_frr_redis_process,
					  sonic_frr_redis_finish, g_redis_ctx,
					  &provider);
	if (result != 0) {
		sonic_frr_redis_free(g_redis_ctx);
		g_redis_ctx = NULL;
		return result;
	}

	if (IS_ZEBRA_DEBUG_DPLANE)
		zlog_debug("%s register status: %d",
			   sonic_frr_redis_provider_name, result);

	return 0;
}

/**
 * Register FRR initialization hooks for the Redis provider module.
 *
 * @return Zero on success.
 */
static int sonic_frr_redis_init(void)
{
	hook_register(frr_late_init, sonic_frr_redis_provider_new);
	return 0;
}

FRR_MODULE_SETUP(
	.name = "sonic_frr_redis_interface",
	.version = "0.0.1",
	.description = "SONiC FRR Redis interface.",
	.init = sonic_frr_redis_init,
);
