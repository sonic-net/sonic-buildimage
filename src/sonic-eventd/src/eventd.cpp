#include <thread>
#include <memory>
#include <swss/dbconnector.h>
#include "eventd.h"
#include "zmq.h"

/*
 * There are 5 threads, including the main
 *
 * (0) main thread -- Runs eventd service that accepts commands event_req_type_t
 *  This can be used to control caching events and a no-op echo service.
 *
 * (1) capture/cache service
 *      Saves all the events between cache start & stop.
 *      Update missed cached counter in memory.
 *
 * (2) Main proxy service that runs XSUB/XPUB ends
 *
 * (3) Get stats for total published counter in memory. This thread also sends
 *     heartbeat message. It accomplishes by counting upon receive missed due
 *     to event receive timeout.
 *
 * (4) Thread to update counters from memory to redis periodically.
 *
 */

using namespace std;
using namespace swss;

#define MB(N) ((N) * 1024 * 1024)
#define EVT_SIZE_AVG 150

#define MAX_CACHE_SIZE (MB(100) / (EVT_SIZE_AVG))

/* Count of elements returned in each read */
#define READ_SET_SIZE 100

#define VEC_SIZE(p) ((int)p.size())

/* Sock read timeout in milliseconds, to enable look for control signals */
#define CAPTURE_SOCK_TIMEOUT 800

#define HEARTBEAT_INTERVAL_SECS 2  /* Default: 2 seconds */

/* Source & tag for heartbeat events */
#define EVENTD_PUBLISHER_SOURCE "sonic-events-eventd"
#define EVENTD_HEARTBEAT_TAG "heartbeat"


const char *counter_keys[COUNTERS_EVENTS_TOTAL] = {
    COUNTERS_EVENTS_PUBLISHED,
    COUNTERS_EVENTS_MISSED_CACHE
};

static bool s_unit_testing = false;

int
eventd_proxy::init()
{
	SWSS_LOG_INFO("Start xpub/xsub proxy");

	m_thr = thread(&eventd_proxy::run, this);

    while (!m_init_done) {
        this_thread::sleep_for(chrono::milliseconds(10));
    }

	return m_init_result;
}

void
eventd_proxy::run()
{
	int rc = 0;
    SWSS_LOG_INFO("Running xpub/xsub proxy");

    m_frontend = zmq_socket(m_ctx, ZMQ_XSUB);
    RET_ON_ERR(m_frontend != NULL, "failing to get ZMQ_XSUB socket");

    rc = zmq_bind(m_frontend, get_config(string(XSUB_END_KEY)).c_str());
    RET_ON_ERR(rc == 0, "Failing to bind XSUB to %s", get_config(string(XSUB_END_KEY)).c_str());

    m_backend = zmq_socket(m_ctx, ZMQ_XPUB);
    RET_ON_ERR(m_backend != NULL, "failing to get ZMQ_XPUB socket");

    rc = zmq_bind(m_backend, get_config(string(XPUB_END_KEY)).c_str());
    RET_ON_ERR(rc == 0, "Failing to bind XPUB to %s", get_config(string(XPUB_END_KEY)).c_str());

    m_capture = zmq_socket(m_ctx, ZMQ_PUB);
    RET_ON_ERR(m_capture != NULL, "failing to get ZMQ_PUB socket for capture");

    rc = zmq_bind(m_capture, get_config(string(CAPTURE_END_KEY)).c_str());
    RET_ON_ERR(rc == 0, "Failing to bind capture PUB to %s", get_config(string(CAPTURE_END_KEY)).c_str());

    /* Signal successful initialization to init() */
    m_init_result = 0;
    m_init_done = true;

    /* runs forever until zmq context is terminated */
    zmq_proxy(m_frontend, m_backend, m_capture);

out:
    /* Signal failure if we got here before successful init */
    if (!m_init_done) {
        m_init_result = 1;
        m_init_done = true;
    }

    if (m_frontend != NULL) {
        zmq_close(m_frontend);
        m_frontend = NULL;
    }
    if (m_backend != NULL) {
        zmq_close(m_backend);
        m_backend = NULL;
    }
    if (m_capture != NULL) {
        zmq_close(m_capture);
        m_capture = NULL;
    }

    SWSS_LOG_INFO("Stopped xpub/xsub proxy");
}


stats_collector::stats_collector() :
    m_shutdown(false), m_pause_heartbeat(false), m_heartbeats_published(0),
    m_heartbeats_interval_cnt(0)
{
    set_heartbeat_interval(HEARTBEAT_INTERVAL_SECS);
    for (int i=0; i < COUNTERS_EVENTS_TOTAL; ++i) {
        m_lst_counters[i] = 0;
    }
    m_updated = false;
}


void
stats_collector::set_heartbeat_interval(int val)
{
    int interval_count_to_set = 0;
    if (val > 0) {
        /* Round to highest possible multiples of MIN */
        interval_count_to_set =
            (((val * 1000) + STATS_HEARTBEAT_MIN - 1) / STATS_HEARTBEAT_MIN);
    }
    else if (val == 0) {
        /* Least possible */
        interval_count_to_set = 1;
    }
    else if (val == -1) {
        /* Turn off heartbeat */
        interval_count_to_set = 0;
        SWSS_LOG_INFO("Heartbeat turned OFF");
    }
    /* Any other value is ignored as invalid */
    m_heartbeats_interval_cnt = interval_count_to_set;

    SWSS_LOG_INFO("Set heartbeat: val=%d secs cnt=%d min=%d ms final=%d secs",
            val, interval_count_to_set, STATS_HEARTBEAT_MIN,
            (interval_count_to_set * STATS_HEARTBEAT_MIN / 1000));
}


int
stats_collector::get_heartbeat_interval()
{
    return m_heartbeats_interval_cnt * STATS_HEARTBEAT_MIN / 1000;
}

int
stats_collector::start()
{
    int rc = -1;

    if (!s_unit_testing) {
        try {
            m_counters_db = make_shared<swss::DBConnector>("COUNTERS_DB", 0, false);
        }
        catch (exception &e)
        {
            SWSS_LOG_ERROR("Unable to get DB Connector, e=(%s)\n", e.what());
        }
        RET_ON_ERR(m_counters_db != NULL, "Failed to get COUNTERS_DB");

        m_stats_table = make_shared<swss::Table>(
                m_counters_db.get(), COUNTERS_EVENTS_TABLE);
        RET_ON_ERR(m_stats_table != NULL, "Failed to get events table");

        m_thr_writer = thread(&stats_collector::run_writer, this);
    }
    m_thr_collector = thread(&stats_collector::run_collector, this);
    rc = 0;
out:
    return rc;
}

void
stats_collector::run_writer()
{
    while (true) {
        if (m_updated.exchange(false)) {
            /* Update if there had been any update */

            for (int i = 0; i < COUNTERS_EVENTS_TOTAL; ++i) {
                vector<FieldValueTuple> fv;

                fv.emplace_back(EVENTS_STATS_FIELD_NAME, to_string(m_lst_counters[i]));

                m_stats_table->set(counter_keys[i], fv);
            }
        }
        if (m_shutdown) {
            break;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
        /*
         * After sleep always do an update if needed before checking
         * shutdown flag, as any counters collected during sleep
         * needs to be updated.
         */
    }

    m_stats_table.reset();
    m_counters_db.reset();
}

void
stats_collector::run_collector()
{
    int hb_cntr = 0;
    string hb_key = string(EVENTD_PUBLISHER_SOURCE) + ":" + EVENTD_HEARTBEAT_TAG;
    event_handle_t pub_handle = NULL;
    event_handle_t subs_handle = NULL;

    /*
     * A subscriber is required to set a subscription. Else all published
     * events will be dropped at the point of publishing itself.
     */
    pub_handle = events_init_publisher(EVENTD_PUBLISHER_SOURCE);
    RET_ON_ERR(pub_handle != NULL,
            "failed to create publisher handle for heartbeats");

    subs_handle = events_init_subscriber(false, STATS_HEARTBEAT_MIN);
    RET_ON_ERR(subs_handle != NULL, "failed to subscribe to all");

    /*
     * Though we can count off of capture socket, then we need to duplicate
     * code in event_receive which has the logic to count all missed per
     * runtime id. It also has logic to retire closed runtime IDs.
     *
     * So use regular subscriber API w/o cache but timeout to enable
     * exit, upon shutdown.
     */
    /*
     * The collector service runs until shutdown.
     * The only task is to update total_published & total_missed_internal.
     * The write of these counters into redis is done by another thread.
     */

    while(!m_shutdown) {
        event_receive_op_t op;
        int rc = 0;

        try {
            rc = event_receive(subs_handle, op);
        }
        catch (exception& e)
        {
            rc = -1;
            stringstream ss;
            ss << e.what();
            SWSS_LOG_ERROR("Receive event failed with %s", ss.str().c_str());
        }

        if ((rc == 0) && (op.key != hb_key)) {
            /* TODO: Discount EVENT_STR_CTRL_DEINIT messages too */
            increment_published(1+op.missed_cnt);

            /* reset counter on receive to restart. */
            hb_cntr = 0;
        }
        else {
            if (rc < 0) {
                SWSS_LOG_ERROR(
                        "event_receive failed with rc=%d; stats:published(%lu)", rc,
                        m_lst_counters[INDEX_COUNTERS_EVENTS_PUBLISHED].load());
            }
            if (!m_pause_heartbeat && (m_heartbeats_interval_cnt > 0) &&
                    ++hb_cntr >= m_heartbeats_interval_cnt) {
                rc = event_publish(pub_handle, EVENTD_HEARTBEAT_TAG);
                if (rc != 0) {
                    SWSS_LOG_ERROR("Failed to publish heartbeat rc=%d", rc);
                }
                hb_cntr = 0;
                ++m_heartbeats_published;
            }
        }
    }

out:
    /*
     * NOTE: A shutdown could lose messages in cache.
     * But consider, that eventd shutdown is a critical shutdown as it would
     * bring down all other features. Hence done only at system level shutdown,
     * hence losing few messages in flight is acceptable. Any more complex code
     * to handle is unwanted.
     */

    events_deinit_subscriber(subs_handle);
    events_deinit_publisher(pub_handle);
    m_shutdown = true;
}

capture_service::~capture_service()
{
    stop_capture();
}

void
capture_service::stop_capture()
{
    m_ctrl = STOP_CAPTURE;

    if (m_thr.joinable()) {
        m_thr.join();
    }
}

static bool
validate_event(const internal_event_t &event, runtime_id_t &rid, sequence_t &seq)
{
    bool ret = false;
    if(event.empty()) {
        return ret;
    }
    internal_event_t::const_iterator itc_r, itc_s, itc_e;
    itc_r = event.find(EVENT_RUNTIME_ID);
    itc_s = event.find(EVENT_SEQUENCE);
    itc_e = event.find(EVENT_STR_DATA);

    if ((itc_r != event.end()) && (itc_s != event.end()) && (itc_e != event.end())) {
        ret = true;
        rid = itc_r->second;
        seq = str_to_seq(itc_s->second);
    }
    else {
        SWSS_LOG_ERROR("Invalid evt: %s", map_to_str(event).c_str());
    }

    return ret;
}


/*
 * Initialize cache with set of events provided.
 * Events read by cache service will be appended
 */
void
capture_service::init_capture_cache(const event_serialized_lst_t &lst)
{
    /* Cache given events as initial stock.
     * Save runtime ID with last seen seq to avoid duplicates, while reading
     * from capture socket.
     * No check for max cache size here, as most likely not needed.
     */
    for (event_serialized_lst_t::const_iterator itc = lst.begin(); itc != lst.end(); ++itc) {
        internal_event_t event;

        if (deserialize(*itc, event) == 0) {
            runtime_id_t rid;
            sequence_t seq;

            if (validate_event(event, rid, seq)) {
                m_pre_exist_id[rid] = seq;
                m_events.push_back(*itc);
            }
        }
    }
}


void
capture_service::do_capture()
{
    int rc;
    int block_ms=CAPTURE_SOCK_TIMEOUT;
    int init_cnt;
    void *cap_sub_sock = NULL;
    counters_t total_overflow = 0;

    typedef enum {
        /*
         * In this state every event read is compared with init cache given
         * Only new events are saved.
         */
        CAP_STATE_INIT = 0,

        /* In this state, all events read are cached until max limit */
        CAP_STATE_ACTIVE,

        /* Cache has hit max. Hence only save last event for each runime ID */
        CAP_STATE_LAST
    } cap_state_t;

    cap_state_t cap_state = CAP_STATE_INIT;

    /*
     * Need subscription for publishers to publish.
     * The stats collector service already has active subscriber for all.
     */

    cap_sub_sock = zmq_socket(m_ctx, ZMQ_SUB);
    RET_ON_ERR(cap_sub_sock != NULL, "failing to get ZMQ_SUB socket");

    rc = zmq_connect(cap_sub_sock, get_config(string(CAPTURE_END_KEY)).c_str());
    RET_ON_ERR(rc == 0, "Failing to bind capture SUB to %s", get_config(string(CAPTURE_END_KEY)).c_str());

    rc = zmq_setsockopt(cap_sub_sock, ZMQ_SUBSCRIBE, "", 0);
    RET_ON_ERR(rc == 0, "Failing to ZMQ_SUBSCRIBE");

    rc = zmq_setsockopt(cap_sub_sock, ZMQ_RCVTIMEO, &block_ms, sizeof (block_ms));
    RET_ON_ERR(rc == 0, "Failed to ZMQ_RCVTIMEO to %d", block_ms);

    m_cap_run = true;

    while (m_ctrl != START_CAPTURE) {
        /* Wait for capture start */
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    /*
     * The cache service connects but defers any reading until caller provides
     * the startup cache. But all events that arrived since connect, though not read
     * will be held by ZMQ in its local cache.
     *
     * When cache service starts reading, check against the initial stock for duplicates.
     * m_pre_exist_id caches the last seq number in initial stock for each runtime id.
     * So only allow sequence number greater than cached number.
     *
     * Theoretically all the events provided via initial stock could be duplicates.
     * Hence until as many events as in initial stock or until the cached id map
     * is empty, do this check.
     */
    init_cnt = (int)m_events.size();

    /* Read until STOP_CAPTURE */
    while(m_ctrl == START_CAPTURE) {
        runtime_id_t rid;
        sequence_t seq;
        internal_event_t event;
        string source, evt_str;

        if ((rc = zmq_message_read(cap_sub_sock, 0, source, event)) != 0) {
            /*
             * The capture socket captures SUBSCRIBE requests too.
             * The messge could contain subscribe filter strings and binary code.
             * Empty string with binary code will fail to deserialize.
             * Else would fail event validation.
             */
            RET_ON_ERR((rc == EAGAIN) || (rc == ERR_MESSAGE_INVALID),
                "0:Failed to read from capture socket");
            continue;
        }
        if (!validate_event(event, rid, seq)) {
            continue;
        }
        serialize(event, evt_str);

        switch(cap_state) {
        case CAP_STATE_INIT:
            /*
             * In this state check against cache, if duplicate
             * When duplicate or new one seen, remove the entry from pre-exist map
             * Stay in this state, until the pre-exist cache is empty or as many
             * messages as in cache are seen, as in worst case even if you see
             * duplicate of each, it will end with first m_events.size()
             */
            {
                bool add = true;
                init_cnt--;
                pre_exist_id_t::iterator it = m_pre_exist_id.find(rid);

                if (it != m_pre_exist_id.end()) {
                    if (seq <= it->second) {
                        /* Duplicate; Later/same seq in cache. */
                        add = false;
                    }
                    if (seq >= it->second) {
                        /* new one; This runtime ID need not be checked again */
                        m_pre_exist_id.erase(it);
                    }
                }
                if (add) {
                    m_events.push_back(evt_str);
                }
            }
            if(m_pre_exist_id.empty() || (init_cnt <= 0)) {
                /* Init check is no more needed. */
                pre_exist_id_t().swap(m_pre_exist_id);
                cap_state = CAP_STATE_ACTIVE;
            }
            break;

        case CAP_STATE_ACTIVE:
            /* Save until max allowed */
            try
            {
                m_events.push_back(evt_str);
                if (VEC_SIZE(m_events) >= m_cache_max) {
                    cap_state = CAP_STATE_LAST;
                    /* Clear the map, created to ensure memory space available */
                    m_last_events.clear();
                    m_last_events_init = true;
                }
                break;
            }
            catch (bad_alloc& e)
            {
                stringstream ss;
                ss << e.what();
                SWSS_LOG_ERROR("Cache save event failed with %s events:size=%d",
                        ss.str().c_str(), VEC_SIZE(m_events));
                cap_state = CAP_STATE_LAST;
                // fall through to save this event in last set.
            }

        case CAP_STATE_LAST:
            total_overflow++;
            m_last_events[rid] = evt_str;
            if (total_overflow > m_last_events.size()) {
                m_total_missed_cache++;
                m_stats_instance->increment_missed_cache(1);
            }
            break;
        }
    }

out:
    /*
     * Capture stop will close the socket which fail the read
     * and hence bail out.
     */
    zmq_close(cap_sub_sock);
    m_cap_run = false;
    return;
}


int
capture_service::set_control(capture_control_t ctrl, event_serialized_lst_t *lst)
{
    int ret = -1;
    int duration = CAPTURE_SERVICE_POLLING_DURATION;

    /* Can go in single step only. */
    RET_ON_ERR((ctrl - m_ctrl) == 1, "m_ctrl(%d)+1 < ctrl(%d)", m_ctrl.load(), ctrl);

    switch(ctrl) {
        case INIT_CAPTURE:
            m_thr = thread(&capture_service::do_capture, this);
            for(int i=0; !m_cap_run && (i < CAPTURE_SERVICE_POLLING_RETRIES); ++i) {
                /* Poll to see if thread has been init, if so exit early. Add delay on every attempt */
                this_thread::sleep_for(chrono::milliseconds(duration));
                duration = min(duration + CAPTURE_SERVICE_POLLING_INCREMENT, CAPTURE_SERVICE_POLLING_MAX_DURATION);
            }
            RET_ON_ERR(m_cap_run, "Failed to init capture");
            m_ctrl = ctrl;
            ret = 0;
            break;

        case START_CAPTURE:

            /*
             * Reserve a MAX_PUBLISHERS_COUNT entries for last events, as we use it only
             * upon m_events/vector overflow, which might block adding new entries in map
             * if overall mem consumption is too high. Clearing the map just before use
             * is likely to help.
             */
            for (int i=0; i<MAX_PUBLISHERS_COUNT; ++i) {
                m_last_events[to_string(i)] = "";
            }

            if ((lst != NULL) && (!lst->empty())) {
                init_capture_cache(*lst);
            }
            m_ctrl = ctrl;
            ret = 0;
            break;


        case STOP_CAPTURE:
            /*
             * Caller would have initiated SUBS channel.
             * Read for CACHE_DRAIN_IN_MILLISECS to drain off cache
             * before stopping.
             */
            this_thread::sleep_for(chrono::milliseconds(CACHE_DRAIN_IN_MILLISECS));
            stop_capture();
            ret = 0;
            break;

        default:
            SWSS_LOG_ERROR("Unexpected code=%d", ctrl);
            break;
    }
out:
    return ret;
}

int
capture_service::read_cache(event_serialized_lst_t &lst_fifo,
        last_events_t &lst_last, counters_t &overflow_cnt)
{
    lst_fifo.swap(m_events);
    if (m_last_events_init) {
        lst_last.swap(m_last_events);
    } else {
        last_events_t().swap(lst_last);
    }
    last_events_t().swap(m_last_events);
    event_serialized_lst_t().swap(m_events);
    overflow_cnt = m_total_missed_cache;
    return 0;
}

static int
process_options(stats_collector *stats, const event_serialized_lst_t &req_data,
        event_serialized_lst_t &resp_data)
{
    int ret = -1;
    if (!req_data.empty()) {
        RET_ON_ERR(req_data.size() == 1, "Expect only one options string %d",
                (int)req_data.size());
        const auto &data = nlohmann::json::parse(*(req_data.begin()));
        RET_ON_ERR(data.size() == 1, "Only one supported option. Expect 1. size=%d",
                (int)data.size());
        const auto it = data.find(GLOBAL_OPTION_HEARTBEAT);
        RET_ON_ERR(it != data.end(), "Expect HEARTBEAT_INTERVAL; got %s",
                data.begin().key().c_str());
        stats->set_heartbeat_interval(it.value());
        ret = 0;
    }
    else {
        nlohmann::json msg = nlohmann::json::object();
        msg[GLOBAL_OPTION_HEARTBEAT] = stats->get_heartbeat_interval();
        resp_data.push_back(msg.dump());
        ret = 0;
    }
out:
    return ret;
}


void
run_eventd_service()
{
    int code = 0;
    int cache_max;
    event_service service;
    stats_collector stats_instance;
    eventd_proxy *proxy = NULL;
    unique_ptr<capture_service> capture;
    bool skip_caching = false;

    event_serialized_lst_t capture_fifo_events;
    last_events_t capture_last_events;

    SWSS_LOG_INFO("Eventd service starting\n");

    void *zctx = zmq_ctx_new();
    RET_ON_ERR(zctx != NULL, "Failed to get zmq ctx");

    cache_max = get_config_data(string(CACHE_MAX_CNT), (int)MAX_CACHE_SIZE);
    RET_ON_ERR(cache_max > 0, "Failed to get CACHE_MAX_CNT");

    proxy = new eventd_proxy(zctx);
    RET_ON_ERR(proxy != NULL, "Failed to create proxy");

    RET_ON_ERR(proxy->init() == 0, "Failed to init proxy");

    RET_ON_ERR(service.init_server(zctx) == 0, "Failed to init service");

    RET_ON_ERR(stats_instance.start() == 0, "Failed to start stats collector");

    /* Pause heartbeat during caching */
    stats_instance.heartbeat_ctrl(true);

    /*
     * Start cache service, right upon eventd starts so as not to lose
     * events until telemetry starts.
     * Telemetry will send a stop & collect cache upon startup
     */
    capture = make_unique<capture_service>(zctx, cache_max, &stats_instance);
    if (capture->set_control(INIT_CAPTURE) != 0) {
        SWSS_LOG_WARN("Failed to initialize capture service, so we skip caching");
        skip_caching = true;
        capture.reset(); // Capture service will not be available
    } else {
        RET_ON_ERR(capture->set_control(START_CAPTURE) == 0, "Failed to start capture");
    }

    this_thread::sleep_for(chrono::milliseconds(200));
    RET_ON_ERR(stats_instance.is_running(), "Failed to start stats instance");

    while(code != EVENT_EXIT) {
        int resp = -1;
        event_serialized_lst_t req_data, resp_data;

        RET_ON_ERR(service.channel_read(code, req_data) == 0,
                "Failed to read request");

        switch(code) {
            case EVENT_CACHE_INIT:
                /* connect only*/
                if (capture != NULL) {
                    capture.reset();
                }
                event_serialized_lst_t().swap(capture_fifo_events);
                last_events_t().swap(capture_last_events);

                capture = make_unique<capture_service>(zctx, cache_max, &stats_instance);
                if (capture != NULL) {
                    resp = capture->set_control(INIT_CAPTURE);
                }
                break;


            case EVENT_CACHE_START:
                if (capture == NULL) {
                    SWSS_LOG_WARN("Cache is not initialized to start");
                    resp = -1;
                    break;
                }
                /* Pause heartbeat during caching */
                stats_instance.heartbeat_ctrl(true);

                resp = capture->set_control(START_CAPTURE, &req_data);
                break;


            case EVENT_CACHE_STOP:
                if (capture == NULL) {
                    SWSS_LOG_WARN("Cache is not initialized to stop");
                    resp = -1;
                    break;
                }
                resp = capture->set_control(STOP_CAPTURE);
                if (resp == 0) {
                    counters_t overflow;
                    resp = capture->read_cache(capture_fifo_events, capture_last_events,
                            overflow);
                }
                capture.reset();

                /* Unpause heartbeat upon stop caching */
                stats_instance.heartbeat_ctrl();
                break;


            case EVENT_CACHE_READ:
                if (skip_caching) {
                    SWSS_LOG_WARN("Capture service is unavailable, skipping cache read");
                    resp = -1;
                    break;
                }
                if (capture != NULL) {
                    SWSS_LOG_ERROR("Cache is not stopped yet.");
                    resp = -1;
                    break;
                }
                resp = 0;

                if (capture_fifo_events.empty()) {
                    for (last_events_t::iterator it = capture_last_events.begin();
                            it != capture_last_events.end(); ++it) {
                        capture_fifo_events.push_back(it->second);
                    }
                    last_events_t().swap(capture_last_events);
                }

                {
                    int sz = VEC_SIZE(capture_fifo_events) < READ_SET_SIZE ?
                        VEC_SIZE(capture_fifo_events) : READ_SET_SIZE;

                    if (sz != 0) {
                        auto it = std::next(capture_fifo_events.begin(), sz);
                        move(capture_fifo_events.begin(), capture_fifo_events.end(),
                                back_inserter(resp_data));

                        if (sz == VEC_SIZE(capture_fifo_events)) {
                            event_serialized_lst_t().swap(capture_fifo_events);
                        } else {
                            capture_fifo_events.erase(capture_fifo_events.begin(), it);
                        }
                    }
                }
                break;


            case EVENT_ECHO:
                resp = 0;
                resp_data.swap(req_data);
                break;

            case EVENT_OPTIONS:
                resp = process_options(&stats_instance, req_data, resp_data);
                break;

            case EVENT_EXIT:
                resp = 0;
                break;

            default:
                SWSS_LOG_ERROR("Unexpected request: %d", code);
                assert(false);
                break;
        }
        RET_ON_ERR(service.channel_write(resp, resp_data) == 0,
                "Failed to write response back");
    }
out:
    service.close_service();
    stats_instance.stop();

    if (zctx != NULL) {
        zmq_ctx_term(zctx);
    }
    if (proxy != NULL) {
        delete proxy;
    }
    SWSS_LOG_INFO("Eventd service exiting\n");
}

void set_unit_testing(bool b)
{
    s_unit_testing = b;
}


