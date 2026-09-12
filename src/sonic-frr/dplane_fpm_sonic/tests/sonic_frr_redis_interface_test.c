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
 * @file sonic_frr_redis_interface_test.c
 * @brief Unit tests for the standalone SONiC FRR Redis dataplane provider.
 *
 * These CUnit tests compile the production provider source with
 * SONIC_FRR_REDIS_UNIT_TEST and replace FRR, hiredis, and swss-common entry
 * points with small local stubs. The harness captures ZMQ publish commands and
 * fallback Redis commands instead of opening real transport connections, which
 * lets the tests verify APP_DB key naming, field values, counters, and
 * dataplane-provider pass-through behavior without a running zebra instance,
 * orchagent, or Redis server.
 */

#define SONIC_FRR_REDIS_UNIT_TEST 1

#include <arpa/inet.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define REDIS_OK 0
#define REDIS_ERR 1
#define REDIS_REPLY_ERROR 6
#define REDIS_REPLY_STATUS 5

#define APPL_DB_NAME "APPL_DB"
#define APPL_DB 0
#define SWSS_TABLE_CHANNEL_SUFFIX "_CHANNEL"
#define SWSS_TABLE_CHANNEL_DB_SEPARATOR "@"
#define SWSS_TABLE_KEY_SET_SUFFIX "_KEY_SET"
#define SWSS_TABLE_DEL_SET_SUFFIX "_DEL_SET"
#define SWSS_TABLE_STATE_HASH_PREFIX "_"
#define SWSS_TABLE_UPDATE_NOTIFICATION "G"
#define APP_EVPN_SPLIT_HORIZON_TABLE_NAME "EVPN_SPLIT_HORIZON_TABLE"
#define APP_EVPN_DF_TABLE_NAME "EVPN_DF_TABLE"

#define DPLANE_BR_PORT_NON_DF 0x1
#define DPLANE_PRIO_POSTPROCESS 0
#define DPLANE_PROV_FLAGS_DEFAULT 0
#define VRF_DEFAULT 0
#define IS_ZEBRA_DEBUG_DPLANE 0
#define frr_late_init 0
#define EVENT_ARG(event_ptr) ((event_ptr)->arg)
#define FRR_MODULE_SETUP(...) static const int sonic_frr_redis_module_setup = 0

typedef char *SWSSString;
typedef const char *SWSSStrRef;

enum {
    SWSSException_None = 0,
    SWSSException_Runtime = 1,
};

typedef struct {
    int exception;
    SWSSString message;
    SWSSString location;
} SWSSResult;

typedef struct redisAsyncContext redisAsyncContext;
typedef struct redisReply {
    int type;
    char *str;
} redisReply;

typedef void(redisCallbackFn)(redisAsyncContext *redis, void *reply,
                              void *privdata);
typedef void(redisConnectCallback)(const redisAsyncContext *redis, int status);
typedef void(redisDisconnectCallback)(const redisAsyncContext *redis, int status);

typedef struct {
    const char *endpoint;
    struct timeval *connect_timeout;
    struct timeval *command_timeout;
} redisOptions;

typedef struct {
    const char *field;
    SWSSString value;
} SWSSFieldValueTuple;

typedef struct {
    uint64_t len;
    SWSSFieldValueTuple *data;
} SWSSFieldValueArray;

typedef void *SWSSDBConnector;
typedef void *SWSSZmqClient;
typedef struct test_zmq_table *SWSSZmqProducerStateTable;

#define REDIS_OPTIONS_SET_UNIX(options, path) ((options)->endpoint = (path))

struct redisAsyncContext {
    struct {
        int fd;
    } c;
    int err;
    const char *errstr;
    void *data;
    struct {
        void (*addRead)(void *privdata);
        void (*delRead)(void *privdata);
        void (*addWrite)(void *privdata);
        void (*delWrite)(void *privdata);
        void (*cleanup)(void *privdata);
        void (*scheduleTimer)(void *privdata, struct timeval tv);
        void *data;
    } ev;
};

struct event_loop {
    int unused;
};

struct event {
    void *arg;
};

enum dplane_op_e {
    DPLANE_OP_NONE,
    DPLANE_OP_ROUTE_INSTALL,
    DPLANE_OP_BR_PORT_UPDATE,
    DPLANE_OP_BR_PORT_DELETE,
};

enum ipaddr_type {
    IPADDR_NONE,
    IPADDR_V4,
    IPADDR_V6,
};

struct ipaddr {
    enum ipaddr_type ipa_type;
    struct in_addr ipaddr_v4;
    struct in6_addr ipaddr_v6;
};

#define IS_IPADDR_V4(ipaddr_ptr) ((ipaddr_ptr)->ipa_type == IPADDR_V4)
#define IS_IPADDR_V6(ipaddr_ptr) ((ipaddr_ptr)->ipa_type == IPADDR_V6)

struct interface {
    unsigned int ifindex;
    char name[64];
};

struct zebra_dplane_ctx {
    enum dplane_op_e op;
    unsigned int ifindex;
    uint16_t vlan_id;
    uint32_t flags;
    const struct ipaddr *sph_filters;
    size_t sph_filter_count;
};

struct zebra_dplane_provider {
    void *data;
    int work_limit;
    struct zebra_dplane_ctx **in;
    size_t in_count;
    size_t in_pos;
    struct zebra_dplane_ctx **out;
    size_t out_count;
};

/** Captured hiredis command for validating ProducerStateTable updates. */
struct captured_command {
    int argc;
    char *argv[16];
    redisCallbackFn *callback;
    void *privdata;
};

/** Captured ZMQ publish command for validating orchagent messages. */
struct captured_zmq_command {
    bool is_del;
    const char *table;
    char key[128];
    char field[64];
    char value[128];
};

/** Test ZMQ producer table bound to one APP_DB table name. */
struct test_zmq_table {
    const char *table;
};

static struct captured_command captured_commands[32];
static size_t captured_command_count;
static struct captured_zmq_command captured_zmq_commands[32];
static size_t captured_zmq_command_count;
static struct test_zmq_table test_zmq_tables[3];
static size_t test_zmq_table_count;
static char test_appl_db;
static char test_config_db;
static redisAsyncContext test_redis_context;
static struct interface test_interfaces[8];
static size_t test_interface_count;
static const char *test_separator = ":";
static const char *test_socket = "/var/run/redis/redis.sock";
static const char *test_zmq_feature_value = "true";
static struct event_loop test_dplane_master;

/**
 * Return the number of CUnit test failures recorded so far.
 *
 * @return Number of failed CUnit assertions/tests.
 */
static unsigned int test_failure_count(void)
{
    CU_pRunSummary run_summary = CU_get_run_summary();

    return run_summary ? run_summary->nTestsFailed : 1;
}

/**
 * Duplicate a bounded string for captured command arguments.
 *
 * @param value Source string.
 * @param len Number of bytes to copy from @p value.
 * @return Newly allocated NUL-terminated copy.
 */
static char *test_strndup(const char *value, size_t len)
{
    char *copy = malloc(len + 1);

    CU_ASSERT_FATAL(copy != NULL);
    if (!copy)
        return NULL;

    memcpy(copy, value, len);
    copy[len] = '\0';
    return copy;
}

/**
 * Duplicate a NUL-terminated string for test-owned storage.
 *
 * @param value Source string.
 * @return Newly allocated copy.
 */
static char *test_strdup(const char *value)
{
    return test_strndup(value, strlen(value));
}

/** Release Redis command arguments captured by the hiredis stub. */
static void reset_captured_commands(void)
{
    for (size_t i = 0; i < captured_command_count; i++) {
        for (int j = 0; j < captured_commands[i].argc; j++)
            free(captured_commands[i].argv[j]);
    }
    memset(captured_commands, 0, sizeof(captured_commands));
    captured_command_count = 0;
}

/** Reset all mutable stub state before each unit test. */
static void reset_test_state(void)
{
    reset_captured_commands();
    memset(captured_zmq_commands, 0, sizeof(captured_zmq_commands));
    captured_zmq_command_count = 0;
    memset(test_zmq_tables, 0, sizeof(test_zmq_tables));
    test_zmq_table_count = 0;
    memset(&test_redis_context, 0, sizeof(test_redis_context));
    test_redis_context.c.fd = 42;
    test_redis_context.errstr = "unit test error";
    memset(test_interfaces, 0, sizeof(test_interfaces));
    test_interface_count = 0;
    test_separator = ":";
    test_socket = "/var/run/redis/redis.sock";
    test_zmq_feature_value = "true";
}

/**
 * Add an interface visible through the if_lookup_by_index() stub.
 *
 * @param ifindex Interface index to register.
 * @param name Interface name returned for @p ifindex.
 */
static void add_test_interface(unsigned int ifindex, const char *name)
{
    CU_ASSERT_FATAL(test_interface_count <
                    sizeof(test_interfaces) / sizeof(test_interfaces[0]));
    test_interfaces[test_interface_count].ifindex = ifindex;
    snprintf(test_interfaces[test_interface_count].name,
             sizeof(test_interfaces[test_interface_count].name), "%s", name);
    test_interface_count++;
}

/**
 * Assert that two strings match and print a useful failure message.
 *
 * @param actual Actual string.
 * @param expected Expected string.
 */
static void assert_string(const char *actual, const char *expected)
{
    CU_ASSERT_STRING_EQUAL(actual, expected);
}

/**
 * Assert one captured Redis command argument.
 *
 * @param command_index Captured command index.
 * @param arg_index Argument index within the captured command.
 * @param expected Expected argument value.
 */
static void assert_command_arg(size_t command_index, int arg_index,
                               const char *expected)
{
    CU_ASSERT_FATAL(command_index < captured_command_count);
    CU_ASSERT_FATAL(arg_index < captured_commands[command_index].argc);
    assert_string(captured_commands[command_index].argv[arg_index], expected);
}

/**
 * Assert one captured ZMQ publish command.
 *
 * @param command_index Captured ZMQ command index.
 * @param table Expected APP_DB table name.
 * @param key Expected APP_DB object key.
 * @param field Expected field name, or NULL for DEL.
 * @param value Expected field value, or NULL for DEL.
 * @param is_del True when the command should be a DEL.
 */
static void assert_zmq_command(size_t command_index, const char *table,
                               const char *key, const char *field,
                               const char *value, bool is_del)
{
    const struct captured_zmq_command *command;

    CU_ASSERT_FATAL(command_index < captured_zmq_command_count);
    command = &captured_zmq_commands[command_index];
    CU_ASSERT_EQUAL(command->is_del, is_del);
    assert_string(command->table, table);
    assert_string(command->key, key);
    if (!is_del) {
        assert_string(command->field, field);
        assert_string(command->value, value);
    }
}

static const char *SWSSStrRef_c_str(SWSSStrRef str)
{
    return str ? str : "";
}

static SWSSString SWSSString_new_c_str(const char *str)
{
    return test_strdup(str);
}

static void SWSSString_free(SWSSString str)
{
    free(str);
}

static SWSSResult SWSSDBConnector_new_named(const char *db_name,
                                            uint32_t timeout_ms, uint8_t is_tcp,
                                            SWSSDBConnector *db)
{
    CU_ASSERT_EQUAL(timeout_ms, 0);
    CU_ASSERT_EQUAL(is_tcp, 0);
    if (strcmp(db_name, APPL_DB_NAME) == 0)
        *db = (SWSSDBConnector)&test_appl_db;
    else if (strcmp(db_name, "CONFIG_DB") == 0)
        *db = (SWSSDBConnector)&test_config_db;
    else
        CU_FAIL_FATAL("unexpected DB name");
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSDBConnector_free(SWSSDBConnector db)
{
    CU_ASSERT_PTR_NOT_NULL(db);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSDBConnector_hget(SWSSDBConnector db, const char *key,
                                       const char *field, SWSSString *value)
{
    CU_ASSERT_PTR_EQUAL(db, &test_config_db);
    assert_string(key, "DEVICE_METADATA|localhost");
    assert_string(field, "orch_northbond_evpn_mh_zmq_enabled");
    *value = test_strdup(test_zmq_feature_value);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSSonicDBConfig_getDbSock(const char *db_name,
                                              SWSSString *sock)
{
    assert_string(db_name, APPL_DB_NAME);
    *sock = test_strdup(test_socket);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSSonicDBConfig_getSeparator(const char *db_name,
                                                 SWSSString *separator)
{
    assert_string(db_name, APPL_DB_NAME);
    *separator = test_strdup(test_separator);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqClient_new(const char *endpoint, SWSSZmqClient *client)
{
    assert_string(endpoint, "tcp://localhost:8100");
    *client = (SWSSZmqClient)&test_redis_context;
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqClient_free(SWSSZmqClient client)
{
    CU_ASSERT_PTR_NOT_NULL(client);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqClient_connect(SWSSZmqClient client)
{
    CU_ASSERT_PTR_NOT_NULL(client);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqProducerStateTable_new(SWSSDBConnector db,
                                                const char *table_name,
                                                SWSSZmqClient client,
                                                uint8_t db_persistence,
                                                SWSSZmqProducerStateTable *table)
{
    CU_ASSERT_PTR_NOT_NULL(db);
    CU_ASSERT_PTR_NOT_NULL(client);
    CU_ASSERT_EQUAL(db_persistence, 1);
    CU_ASSERT_FATAL(test_zmq_table_count <
                    sizeof(test_zmq_tables) / sizeof(test_zmq_tables[0]));

    test_zmq_tables[test_zmq_table_count].table = table_name;
    *table = &test_zmq_tables[test_zmq_table_count++];
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqProducerStateTable_free(SWSSZmqProducerStateTable table)
{
    CU_ASSERT_PTR_NOT_NULL(table);
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqProducerStateTable_set(SWSSZmqProducerStateTable table,
                                                const char *key,
                                                SWSSFieldValueArray values)
{
    struct captured_zmq_command *command;

    CU_ASSERT_PTR_NOT_NULL(table);
    CU_ASSERT_EQUAL(values.len, 1);
    CU_ASSERT_FATAL(captured_zmq_command_count <
                    sizeof(captured_zmq_commands) /
                    sizeof(captured_zmq_commands[0]));

    command = &captured_zmq_commands[captured_zmq_command_count++];
    command->is_del = false;
    command->table = table->table;
    snprintf(command->key, sizeof(command->key), "%s", key);
    snprintf(command->field, sizeof(command->field), "%s", values.data[0].field);
    snprintf(command->value, sizeof(command->value), "%s",
             SWSSStrRef_c_str((SWSSStrRef)values.data[0].value));
    return (SWSSResult){.exception = SWSSException_None};
}

static SWSSResult SWSSZmqProducerStateTable_del(SWSSZmqProducerStateTable table,
                                                const char *key)
{
    struct captured_zmq_command *command;

    CU_ASSERT_PTR_NOT_NULL(table);
    CU_ASSERT_FATAL(captured_zmq_command_count <
                    sizeof(captured_zmq_commands) /
                    sizeof(captured_zmq_commands[0]));

    command = &captured_zmq_commands[captured_zmq_command_count++];
    command->is_del = true;
    command->table = table->table;
    snprintf(command->key, sizeof(command->key), "%s", key);
    return (SWSSResult){.exception = SWSSException_None};
}

static void zlog_warn(const char *format, ...)
{
    (void)format;
}

static void zlog_debug(const char *format, ...)
{
    (void)format;
}

static redisAsyncContext *redisAsyncConnectWithOptions(const redisOptions *options)
{
    assert_string(options->endpoint, test_socket);
    return &test_redis_context;
}

static void redisAsyncFree(redisAsyncContext *redis)
{
    redis->data = NULL;
}

static int redisAsyncSetConnectCallback(redisAsyncContext *redis,
                                        redisConnectCallback *callback)
{
    (void)redis;
    (void)callback;
    return REDIS_OK;
}

static int redisAsyncSetDisconnectCallback(redisAsyncContext *redis,
                                           redisDisconnectCallback *callback)
{
    (void)redis;
    (void)callback;
    return REDIS_OK;
}

static int redisAsyncCommandArgv(redisAsyncContext *redis, redisCallbackFn *callback,
                                 void *privdata, int argc, const char **argv,
                                 const size_t *argvlen)
{
    struct captured_command *command;

    CU_ASSERT_FATAL(redis == &test_redis_context);
    CU_ASSERT_FATAL(captured_command_count < sizeof(captured_commands) /
                                            sizeof(captured_commands[0]));
    CU_ASSERT_FATAL(argc <= 16);

    command = &captured_commands[captured_command_count++];
    command->argc = argc;
    command->callback = callback;
    command->privdata = privdata;
    for (int i = 0; i < argc; i++)
        command->argv[i] = test_strndup(argv[i], argvlen[i]);

    return REDIS_OK;
}

static void redisAsyncDisconnect(redisAsyncContext *redis)
{
    (void)redis;
}

static void redisAsyncHandleRead(redisAsyncContext *redis)
{
    (void)redis;
}

static void redisAsyncHandleWrite(redisAsyncContext *redis)
{
    (void)redis;
}

static void redisAsyncHandleTimeout(redisAsyncContext *redis)
{
    (void)redis;
}

static struct event *test_event_new(void *arg)
{
    struct event *event = calloc(1, sizeof(*event));

    CU_ASSERT_FATAL(event != NULL);
    if (!event)
        return NULL;

    event->arg = arg;
    return event;
}

static void event_cancel(struct event **event)
{
    if (!event || !*event)
        return;

    free(*event);
    *event = NULL;
}

static void event_cancel_async(struct event_loop *master, struct event **event,
                               void *unused)
{
    (void)master;
    (void)unused;
    event_cancel(event);
}

static void event_add_read(struct event_loop *master,
                           void (*callback)(struct event *), void *arg, int fd,
                           struct event **event)
{
    (void)master;
    (void)callback;
    (void)fd;
    *event = test_event_new(arg);
}

static void event_add_write(struct event_loop *master,
                            void (*callback)(struct event *), void *arg, int fd,
                            struct event **event)
{
    (void)master;
    (void)callback;
    (void)fd;
    *event = test_event_new(arg);
}

static void event_add_timer_tv(struct event_loop *master,
                               void (*callback)(struct event *), void *arg,
                               struct timeval *timeout, struct event **event)
{
    (void)master;
    (void)callback;
    (void)timeout;
    *event = test_event_new(arg);
}

static void event_add_event(struct event_loop *master,
                            void (*callback)(struct event *), void *arg, int value,
                            struct event **event)
{
    (void)master;
    (void)callback;
    (void)value;
    *event = test_event_new(arg);
}

static enum dplane_op_e dplane_ctx_get_op(const struct zebra_dplane_ctx *ctx)
{
    return ctx->op;
}

static unsigned int dplane_ctx_get_ifindex(const struct zebra_dplane_ctx *ctx)
{
    return ctx->ifindex;
}

static uint16_t dplane_ctx_get_br_port_vlan_id(const struct zebra_dplane_ctx *ctx)
{
    return ctx->vlan_id;
}

static uint32_t dplane_ctx_get_br_port_flags(const struct zebra_dplane_ctx *ctx)
{
    return ctx->flags;
}

static const struct ipaddr *
dplane_ctx_get_br_port_sph_filters(const struct zebra_dplane_ctx *ctx)
{
    return ctx->sph_filters;
}

static size_t dplane_ctx_get_br_port_sph_filter_cnt(const struct zebra_dplane_ctx *ctx)
{
    return ctx->sph_filter_count;
}

static struct interface *if_lookup_by_index(unsigned int ifindex, int vrf_id)
{
    (void)vrf_id;

    for (size_t i = 0; i < test_interface_count; i++) {
        if (test_interfaces[i].ifindex == ifindex)
            return &test_interfaces[i];
    }

    return NULL;
}

static void *dplane_provider_get_data(struct zebra_dplane_provider *provider)
{
    return provider->data;
}

static int dplane_provider_get_work_limit(struct zebra_dplane_provider *provider)
{
    return provider->work_limit;
}

static struct zebra_dplane_ctx *
dplane_provider_dequeue_in_ctx(struct zebra_dplane_provider *provider)
{
    if (provider->in_pos >= provider->in_count)
        return NULL;

    return provider->in[provider->in_pos++];
}

static void dplane_provider_enqueue_out_ctx(struct zebra_dplane_provider *provider,
                                            struct zebra_dplane_ctx *ctx)
{
    provider->out[provider->out_count++] = ctx;
}

static struct event_loop *dplane_get_thread_master(void)
{
    return &test_dplane_master;
}

static int dplane_provider_register(
        const char *name, int priority, int flags,
        int (*start)(struct zebra_dplane_provider *),
        int (*process)(struct zebra_dplane_provider *),
        int (*finish)(struct zebra_dplane_provider *, bool), void *data,
        struct zebra_dplane_provider **provider)
{
    static struct zebra_dplane_provider registered_provider;

    assert_string(name, "sonic_frr_redis_interface");
    CU_ASSERT_EQUAL(priority, DPLANE_PRIO_POSTPROCESS);
    CU_ASSERT_EQUAL(flags, DPLANE_PROV_FLAGS_DEFAULT);
    CU_ASSERT_PTR_NOT_NULL(start);
    CU_ASSERT_PTR_NOT_NULL(process);
    CU_ASSERT_PTR_NOT_NULL(finish);
    registered_provider.data = data;
    *provider = &registered_provider;
    return 0;
}

static int hook_register(int hook, int (*callback)(struct event_loop *))
{
    CU_ASSERT_EQUAL(hook, frr_late_init);
    CU_ASSERT_PTR_NOT_NULL(callback);
    return 0;
}

#if defined(SONIC_FRR_REDIS_FRR_TEST)
#include "../../zebra/sonic_frr_redis_interface.c"
#else
#include "../sonic_frr_redis_interface.c"
#endif

/**
 * Complete a captured Redis command through the provider callback.
 *
 * @param index Captured command index.
 * @param reply_type Redis reply type to report to the provider.
 */
static void complete_command(size_t index, int reply_type)
{
    redisReply reply = {.type = reply_type};

    CU_ASSERT_FATAL(index < captured_command_count);
    captured_commands[index].callback(&test_redis_context, &reply,
                                      captured_commands[index].privdata);
}

/**
 * Build an IPv4 split-horizon filter address.
 *
 * @param address Text-form IPv4 address.
 * @return Test ipaddr value with IPv4 type set.
 */
static struct ipaddr ipv4_addr(const char *address)
{
    struct ipaddr ipaddr = {.ipa_type = IPADDR_V4};

    CU_ASSERT_EQUAL(inet_pton(AF_INET, address, &ipaddr.ipaddr_v4), 1);
    return ipaddr;
}

/**
 * Build an IPv6 split-horizon filter address.
 *
 * @param address Text-form IPv6 address.
 * @return Test ipaddr value with IPv6 type set.
 */
static struct ipaddr ipv6_addr(const char *address)
{
    struct ipaddr ipaddr = {.ipa_type = IPADDR_V6};

    CU_ASSERT_EQUAL(inet_pton(AF_INET6, address, &ipaddr.ipaddr_v6), 1);
    return ipaddr;
}

/**
 * Verify basic provider state APIs.
 *
 * This test covers allocation, enable/disable state, connection state, counter
 * snapshots, counter reset, and cleanup without touching Redis publish paths.
 */
static void test_enable_disable_and_counters(void)
{
    struct sonic_frr_redis_ctx *ctx;
    struct sonic_frr_redis_counters counters;

    reset_test_state();
    ctx = sonic_frr_redis_new(NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);
    CU_ASSERT_FALSE(sonic_frr_redis_is_enabled(ctx));
    CU_ASSERT_FALSE(sonic_frr_redis_is_connected(ctx));

    sonic_frr_redis_set_enabled(ctx, true);
    CU_ASSERT_TRUE(sonic_frr_redis_is_enabled(ctx));

    atomic_store_explicit(&ctx->counters.connects, 3, memory_order_relaxed);
    atomic_store_explicit(&ctx->counters.sets, 2, memory_order_relaxed);
    atomic_store_explicit(&ctx->counters.dels, 1, memory_order_relaxed);
    atomic_store_explicit(&ctx->counters.errors, 4, memory_order_relaxed);
    sonic_frr_redis_get_counters(ctx, &counters);
    CU_ASSERT_EQUAL(counters.connects, 3);
    CU_ASSERT_EQUAL(counters.sets, 2);
    CU_ASSERT_EQUAL(counters.dels, 1);
    CU_ASSERT_EQUAL(counters.errors, 4);

    sonic_frr_redis_reset_counters(ctx);
    sonic_frr_redis_get_counters(ctx, &counters);
    CU_ASSERT_EQUAL(counters.connects, 0);
    CU_ASSERT_EQUAL(counters.sets, 0);
    CU_ASSERT_EQUAL(counters.dels, 0);
    CU_ASSERT_EQUAL(counters.errors, 0);

    sonic_frr_redis_free(ctx);
}

/**
 * Verify APP_DB Redis key names match ProducerStateTable conventions.
 *
 * The provider must use swss-common table suffixes, channel DB separators, and
 * the APP_DB separator when constructing channel, key-set, state-hash, and
 * delete-set names.
 */
static void test_key_names_use_swss_common_naming(void)
{
    char channel[128];
    char key_set[128];
    char state_key[128];
    char del_set[128];

    reset_test_state();
    CU_ASSERT_FATAL(sonic_frr_redis_key_names(APP_EVPN_DF_TABLE_NAME,
                                              "Vlan100:Ethernet8", channel,
                                              sizeof(channel), key_set,
                                              sizeof(key_set), state_key,
                                              sizeof(state_key), del_set,
                                              sizeof(del_set)));

    assert_string(channel, "EVPN_DF_TABLE_CHANNEL@0");
    assert_string(key_set, "EVPN_DF_TABLE_KEY_SET");
    assert_string(state_key, "_EVPN_DF_TABLE:Vlan100:Ethernet8");
    assert_string(del_set, "EVPN_DF_TABLE_DEL_SET");
}

/**
 * Verify bridge-port update publishing for split-horizon and DF state.
 *
 * The test validates captured ZMQ command shape, APP_DB object key, VTEP list
 * formatting for IPv4/IPv6 filters, DF field value, and successful SET
 * counters.
 */
static void test_publish_update_queues_split_horizon_and_df(void)
{
    struct ipaddr filters[] = {ipv4_addr("10.0.0.1"), ipv6_addr("2001:db8::1")};
    struct zebra_dplane_ctx dplane_ctx = {
        .op = DPLANE_OP_BR_PORT_UPDATE,
        .ifindex = 7,
        .vlan_id = 100,
        .flags = DPLANE_BR_PORT_NON_DF,
        .sph_filters = filters,
        .sph_filter_count = sizeof(filters) / sizeof(filters[0]),
    };
    struct sonic_frr_redis_ctx *ctx;
    struct sonic_frr_redis_counters counters;

    reset_test_state();
    test_zmq_feature_value = "enabled";
    add_test_interface(7, "Ethernet8");
    ctx = sonic_frr_redis_new(NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);
    sonic_frr_redis_set_enabled(ctx, true);

    sonic_frr_redis_publish_br_port_update(ctx, &dplane_ctx);

    CU_ASSERT_EQUAL(captured_command_count, 0);
    CU_ASSERT_EQUAL(captured_zmq_command_count, 2);
    assert_zmq_command(0, APP_EVPN_SPLIT_HORIZON_TABLE_NAME,
                       "Vlan100:Ethernet8", "vteps",
                       "10.0.0.1,2001:db8::1", false);
    assert_zmq_command(1, APP_EVPN_DF_TABLE_NAME, "Vlan100:Ethernet8", "df",
                       "false", false);

    sonic_frr_redis_get_counters(ctx, &counters);
    CU_ASSERT_EQUAL(counters.sets, 2);
    CU_ASSERT_EQUAL(counters.dels, 0);
    CU_ASSERT_EQUAL(counters.errors, 0);

    sonic_frr_redis_free(ctx);
}

/**
 * Verify bridge-port delete publishing for split-horizon and DF state.
 *
 * The provider should queue delete-style ProducerStateTable updates for both
 * APP_DB tables and increment DEL counters when the async completions succeed.
 */
static void test_publish_delete_queues_split_horizon_and_df_deletes(void)
{
    struct zebra_dplane_ctx dplane_ctx = {
        .op = DPLANE_OP_BR_PORT_DELETE,
        .ifindex = 7,
        .vlan_id = 100,
    };
    struct sonic_frr_redis_ctx *ctx;
    struct sonic_frr_redis_counters counters;

    reset_test_state();
    add_test_interface(7, "Ethernet8");
    ctx = sonic_frr_redis_new(NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);
    sonic_frr_redis_set_enabled(ctx, true);

    sonic_frr_redis_publish_br_port_update(ctx, &dplane_ctx);

    CU_ASSERT_EQUAL(captured_command_count, 0);
    CU_ASSERT_EQUAL(captured_zmq_command_count, 2);
    assert_zmq_command(0, APP_EVPN_SPLIT_HORIZON_TABLE_NAME,
                       "Vlan100:Ethernet8", NULL, NULL, true);
    assert_zmq_command(1, APP_EVPN_DF_TABLE_NAME, "Vlan100:Ethernet8", NULL,
                       NULL, true);

    sonic_frr_redis_get_counters(ctx, &counters);
    CU_ASSERT_EQUAL(counters.sets, 0);
    CU_ASSERT_EQUAL(counters.dels, 2);
    CU_ASSERT_EQUAL(counters.errors, 0);

    sonic_frr_redis_free(ctx);
}

/**
 * Verify dataplane provider pass-through behavior.
 *
 * The Redis provider observes bridge-port contexts but must enqueue every input
 * context to the next dataplane provider, including unrelated operations.
 */
static void test_provider_process_passes_contexts_to_next_provider(void)
{
    struct sonic_frr_redis_ctx *ctx;
    struct zebra_dplane_ctx update_ctx = {.op = DPLANE_OP_BR_PORT_UPDATE};
    struct zebra_dplane_ctx route_ctx = {.op = DPLANE_OP_ROUTE_INSTALL};
    struct zebra_dplane_ctx *input[] = {&update_ctx, &route_ctx};
    struct zebra_dplane_ctx *output[2] = {NULL, NULL};
    struct zebra_dplane_provider provider = {
        .work_limit = 2,
        .in = input,
        .in_count = sizeof(input) / sizeof(input[0]),
        .out = output,
    };

    reset_test_state();
    ctx = sonic_frr_redis_new(NULL);
    CU_ASSERT_PTR_NOT_NULL_FATAL(ctx);
    provider.data = ctx;

    CU_ASSERT_EQUAL(sonic_frr_redis_process(&provider), 0);
    CU_ASSERT_EQUAL(provider.out_count, 2);
    CU_ASSERT_PTR_EQUAL(output[0], &update_ctx);
    CU_ASSERT_PTR_EQUAL(output[1], &route_ctx);
    CU_ASSERT_EQUAL(captured_command_count, 0);
    CU_ASSERT_EQUAL(captured_zmq_command_count, 0);

    sonic_frr_redis_free(ctx);
}

/**
 * Run all Redis provider unit tests.
 *
 * @return Zero when every assertion passes.
 */
int main(void)
{
    CU_pSuite suite;

    if (CU_initialize_registry() != CUE_SUCCESS)
        return CU_get_error();

    suite = CU_add_suite("SONiC FRR Redis interface", NULL, NULL);
    if (!suite)
        goto fail;

    if (!CU_add_test(suite, "provider state and counters",
                     test_enable_disable_and_counters) ||
        !CU_add_test(suite, "ProducerStateTable key names",
                     test_key_names_use_swss_common_naming) ||
        !CU_add_test(suite, "bridge-port update publish",
                     test_publish_update_queues_split_horizon_and_df) ||
        !CU_add_test(suite, "bridge-port delete publish",
                     test_publish_delete_queues_split_horizon_and_df_deletes) ||
        !CU_add_test(suite, "dataplane provider pass-through",
                     test_provider_process_passes_contexts_to_next_provider))
        goto fail;

    CU_basic_set_mode(CU_BRM_SILENT);
    CU_basic_run_tests();
    reset_test_state();

    if (test_failure_count() != 0)
        goto fail;

    puts("sonic_frr_redis_interface_test: PASS");
    CU_cleanup_registry();
    return 0;

fail:
    CU_cleanup_registry();
    return 1;
}