/*
 * Header file for eventd daemon
 */
#include <atomic>
#include <swss/table.h>
#include <swss/events_service.h>
#include <swss/events.h>
#include <swss/events_wrap.h>

#define ARRAY_SIZE(l) (sizeof(l)/sizeof((l)[0]))

typedef map<runtime_id_t, event_serialized_t> last_events_t;

/* stat counters */
typedef uint64_t counters_t;

typedef enum {
    INDEX_COUNTERS_EVENTS_PUBLISHED,
    INDEX_COUNTERS_EVENTS_MISSED_CACHE,
    COUNTERS_EVENTS_TOTAL
} stats_counter_index_t;

#define EVENTS_STATS_FIELD_NAME "value"
#define STATS_HEARTBEAT_MIN 300
#define CAPTURE_SERVICE_POLLING_DURATION 10
#define CAPTURE_SERVICE_POLLING_INCREMENT 10
#define CAPTURE_SERVICE_POLLING_MAX_DURATION 100
#define CAPTURE_SERVICE_POLLING_RETRIES 100

/*
 *  Started by eventd_service.
 *  Creates XPUB & XSUB end points.
 *  Bind the same
 *  Create a PUB socket end point for capture and bind.
 *  Call run_proxy method with sockets in a dedicated thread.
 *  Thread runs forever until the zmq context is terminated.
 */
class eventd_proxy
{
    public:
        eventd_proxy(void *ctx) : m_ctx(ctx), m_frontend(NULL), m_backend(NULL),
            m_capture(NULL), m_init_done(false), m_init_result(0) {};

        ~eventd_proxy() {
            if (m_thr.joinable())
                m_thr.join();
        }

        int init();

    private:
        void run();

        void *m_ctx;
        void *m_frontend;
        void *m_backend;
        void *m_capture;
        thread m_thr;
        std::atomic<bool> m_init_done;
        std::atomic<int> m_init_result;
};


class stats_collector
{
    public:
        stats_collector();

        ~stats_collector() { stop(); }

        int start();

        void stop() {

            m_shutdown = true;

            if (m_thr_collector.joinable()) {
                m_thr_collector.join();
            }

            if (m_thr_writer.joinable()) {
                m_thr_writer.join();
            }
        }

        void increment_published(counters_t val) {
            _update_stats(INDEX_COUNTERS_EVENTS_PUBLISHED, val);
        }

        void increment_missed_cache(counters_t val) {
            _update_stats(INDEX_COUNTERS_EVENTS_MISSED_CACHE, val);
        }

        counters_t read_counter(stats_counter_index_t index) {
            if (index != COUNTERS_EVENTS_TOTAL) {
                return m_lst_counters[index];
            }
            else {
                return 0;
            }
        }

        /* Sets heartbeat interval in milliseconds */
        void set_heartbeat_interval(int val_in_ms);

        /*
         * Get heartbeat interval in milliseconds
         * NOTE: Set & get value may not match as the value is rounded
         *       to a multiple of smallest possible interval.
         */
        int get_heartbeat_interval();

        /* A way to pause heartbeat */
        void heartbeat_ctrl(bool pause = false) {
            m_pause_heartbeat = pause;
            SWSS_LOG_INFO("Set heartbeat_ctrl pause=%d", pause);
        }

        uint64_t heartbeats_published() const {
            return m_heartbeats_published;
        }

        bool is_running()
        {
            return !m_shutdown;
        }

    private:
        void _update_stats(stats_counter_index_t index, counters_t val) {
            if (index != COUNTERS_EVENTS_TOTAL) {
                m_lst_counters[index] += val;
                m_updated = true;
            }
            else {
                SWSS_LOG_ERROR("Internal code error. Invalid index=%d", index);
            }
        }

        void run_collector();

        void run_writer();

        std::atomic<bool> m_updated;

        std::atomic<counters_t> m_lst_counters[COUNTERS_EVENTS_TOTAL];

        std::atomic<bool> m_shutdown;

        thread m_thr_collector;
        thread m_thr_writer;

        shared_ptr<swss::DBConnector> m_counters_db;
        shared_ptr<swss::Table> m_stats_table;

        std::atomic<bool> m_pause_heartbeat;

        std::atomic<uint64_t> m_heartbeats_published;

        std::atomic<int> m_heartbeats_interval_cnt;
};

/*
 *  Capture/Cache service
 *
 *  The service started in a dedicted thread upon demand.
 *  It is controlled by the caller.
 *  On cache init, the thread is created.
 *      Upon create, it creates a SUB socket to PUB end point of capture.
 *      PUB end point is maintained by zproxy service.
 *
 *  On Cache start, the thread is signalled to start reading.
 *
 *  On cache stop, it is signalled to stop reading and exit. Caller waits
 *  for thread to exit, before starting to read cached data, to ensure
 *  that the data is not handled by two threads concurrently.
 *
 *  This thread maintains its own copy of cache. Reader, does a swap
 *  after thread exits.
 *  This thread ensures the cache is empty at the init.
 *
 *  Upon cache start, the thread is blocked in receive call with timeout.
 *  Only upon receive/timeout, it would notice stop signal. Hence stop
 *  is not synchronous. The caller may wait for thread to terminate
 *  via thread.join().
 *
 *  Each event is 2 parts. It drops the first part, which is
 *  more for filtering events. It creates string from second part
 *  and saves it.
 *
 *  The string is the serialized version of internal_event_ref
 *
 *  It keeps two sets of data
 *      1) List of all events received in vector in same order as received
 *      2) Map of last event from each runtime id upon list overflow max size.
 *
 *  We add to the vector as much as allowed by vector and max limit,
 *  whichever comes first.
 *
 *  The sequence number in internal event will help assess the missed count
 *  by the consumer of the cache data.
 *
 */
typedef enum {
    NEED_INIT = 0,
    INIT_CAPTURE,
    START_CAPTURE,
    STOP_CAPTURE
} capture_control_t;


class capture_service
{
    public:
        capture_service(void *ctx, int cache_max, stats_collector *stats) :
            m_ctx(ctx), m_stats_instance(stats), m_cap_run(false),
            m_ctrl(NEED_INIT), m_cache_max(cache_max),
            m_last_events_init(false), m_total_missed_cache(0)
        {}

        ~capture_service();

        int set_control(capture_control_t ctrl, event_serialized_lst_t *p=NULL);

        int read_cache(event_serialized_lst_t &lst_fifo,
                last_events_t &lst_last, counters_t &overflow_cnt);

    private:
        void init_capture_cache(const event_serialized_lst_t &lst);
        void do_capture();

        void stop_capture();

        void *m_ctx;
        stats_collector *m_stats_instance;

        std::atomic<bool> m_cap_run;
        std::atomic<capture_control_t> m_ctrl;
        thread m_thr;

        int m_cache_max;

        event_serialized_lst_t m_events;

        last_events_t m_last_events;
        bool m_last_events_init;

        typedef map<runtime_id_t, sequence_t> pre_exist_id_t;
        pre_exist_id_t m_pre_exist_id;

        counters_t m_total_missed_cache;

};


/*
 * Main server, that starts the zproxy service and honor
 * eventd service requests event_req_type_t
 *
 *  For echo, it just echoes
 *
 *  FOr cache start, create the SUB end of capture and kick off
 *  capture_events thread. Upon cache stop command, close the handle
 *  which will stop the caching thread with read failure.
 *
 *  for cache read, returns the collected events in chunks.
 *
 */
void run_eventd_service();

/* To help skip redis access during unit testing */
void set_unit_testing(bool b);
