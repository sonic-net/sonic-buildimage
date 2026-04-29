#include <iostream>
#include <memory>
#include <thread>
#include <algorithm>
#include <deque>
#include <regex>
#include <chrono>
#include <atomic>
#include <swss/events_common.h>
#include <swss/events.h>
#include "gtest/gtest.h"
#include "../src/eventd.h"

using namespace std;
using namespace swss;

extern bool g_is_redis_available;
extern const char *counter_keys[];

typedef struct {
    int id;
    string source;
    string tag;
    string rid;
    string seq;
    event_params_t params;
    int missed_cnt;
} test_data_t;

internal_event_t create_ev(const test_data_t &data)
{
    internal_event_t event_data;

    event_data[EVENT_STR_DATA] = convert_to_json(
            data.source + ":" + data.tag, data.params);
    event_data[EVENT_RUNTIME_ID] = data.rid;
    event_data[EVENT_SEQUENCE] = data.seq;

    return event_data;
}

/* Mock test data with event parameters and expected missed count */
static const test_data_t ldata[] = {
    {
        0,
        "source0",
        "tag0",
        "guid-0",
        "1",
        {{"ip", "10.10.10.10"}, {"state", "up"}},
        0
    },
    {
        1,
        "source0",
        "tag1",
        "guid-1",
        "100",
        {{"ip", "10.10.27.10"}, {"state", "down"}},
        0
    },
    {
        2,
        "source1",
        "tag2",
        "guid-2",
        "101",
        {{"ip", "10.10.24.10"}, {"state", "down"}},
        0
    },
    {
        3,
        "source0",
        "tag3",
        "guid-1",
        "105",
        {{"ip", "10.10.10.10"}, {"state", "up"}},
        4
    },
    {
        4,
        "source0",
        "tag4",
        "guid-0",
        "2",
        {{"ip", "10.10.20.10"}, {"state", "down"}},
        0
    },
    {
        5,
        "source1",
        "tag5",
        "guid-2",
        "110",
        {{"ip", "10.10.24.10"}, {"state", "down"}},
        8
    },
    {
        6,
        "source0",
        "tag0",
        "guid-0",
        "5",
        {{"ip", "10.10.10.10"}, {"state", "up"}},
        2
    },
    {
        7,
        "source0",
        "tag1",
        "guid-1",
        "106",
        {{"ip", "10.10.27.10"}, {"state", "down"}},
        0
    },
    {
        8,
        "source1",
        "tag2",
        "guid-2",
        "111",
        {{"ip", "10.10.24.10"}, {"state", "down"}},
        0
    },
    {
        9,
        "source0",
        "tag3",
        "guid-1",
        "109",
        {{"ip", "10.10.10.10"}, {"state", "up"}},
        2
    },
    {
        10,
        "source0",
        "tag4",
        "guid-0",
        "6",
        {{"ip", "10.10.20.10"}, {"state", "down"}},
        0
    },
    {
        11,
        "source1",
        "tag5",
        "guid-2",
        "119",
        {{"ip", "10.10.24.10"}, {"state", "down"}},
        7
    },
};

void run_cap(void *zctx, atomic<bool> &term, atomic<int> &cnt)
{
    void *mock_cap = zmq_socket (zctx, ZMQ_SUB);
    string source;
    internal_event_t ev_int;
    int block_ms = 200;
    int i=0;

    EXPECT_TRUE(NULL != mock_cap);
    EXPECT_EQ(0, zmq_connect(mock_cap, get_config(CAPTURE_END_KEY).c_str()));
    EXPECT_EQ(0, zmq_setsockopt(mock_cap, ZMQ_SUBSCRIBE, "", 0));
    EXPECT_EQ(0, zmq_setsockopt(mock_cap, ZMQ_RCVTIMEO, &block_ms, sizeof (block_ms)));


    while(!term) {
        string source;
        internal_event_t ev_int;
        int rc = zmq_message_read(mock_cap, 0, source, ev_int);
        if (0 == rc && !ev_int.empty()) { // ignore control character empty event
            cnt = ++i;
        }
    }
    zmq_close(mock_cap);
}

void run_sub(void *zctx, atomic<bool> &term, internal_events_lst_t &lst,
        atomic<int> &cnt)
{
    void *mock_sub = zmq_socket (zctx, ZMQ_SUB);
    string source;
    internal_event_t ev_int;
    int block_ms = 200;

    EXPECT_TRUE(NULL != mock_sub);
    EXPECT_EQ(0, zmq_connect(mock_sub, get_config(XPUB_END_KEY).c_str()));
    EXPECT_EQ(0, zmq_setsockopt(mock_sub, ZMQ_SUBSCRIBE, "", 0));
    EXPECT_EQ(0, zmq_setsockopt(mock_sub, ZMQ_RCVTIMEO, &block_ms, sizeof (block_ms)));

    while(!term) {
        if (0 == zmq_message_read(mock_sub, 0, source, ev_int)) {
            lst.push_back(ev_int);
            cnt = (int)lst.size();
        }
    }

    zmq_close(mock_sub);
}

void *init_pub(void *zctx)
{
    void *mock_pub = zmq_socket (zctx, ZMQ_PUB);
    EXPECT_TRUE(NULL != mock_pub);
    EXPECT_EQ(0, zmq_connect(mock_pub, get_config(XSUB_END_KEY).c_str()));

    /* Provide time for async connect to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    return mock_pub;
}

void run_pub(void *mock_pub, const string wr_source, internal_events_lst_t &lst)
{
    for(internal_events_lst_t::const_iterator itc = lst.begin(); itc != lst.end(); ++itc) {
        EXPECT_EQ(0, zmq_message_send(mock_pub, wr_source, *itc));
    }
}

TEST(eventd, proxy)
{
    printf("Proxy TEST started\n");
    atomic<bool> term_sub = false;
    atomic<bool> term_cap = false;
    string wr_source("hello");
    internal_events_lst_t rd_evts, wr_evts;
    atomic<int> rd_evts_sz = 0, rd_cevts_sz = 0;
    int wr_sz;

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    eventd_proxy *pxy = new eventd_proxy(zctx);
    EXPECT_TRUE(NULL != pxy);

    /* Starting proxy */
    EXPECT_EQ(0, pxy->init());

    /* capture in a thread */
    thread thrc(&run_cap, zctx, ref(term_cap), ref(rd_cevts_sz));

    /* subscriber in a thread */
    thread thr(&run_sub, zctx, ref(term_sub), ref(rd_evts), ref(rd_evts_sz));

    /* Init pub connection */
    void *mock_pub = init_pub(zctx);

    EXPECT_TRUE(5 < ARRAY_SIZE(ldata));

    for(int i=0; i<5; ++i) {
        wr_evts.push_back(create_ev(ldata[i]));
    }

    EXPECT_EQ(rd_evts_sz, 0);
    EXPECT_EQ(rd_cevts_sz, 0);

    /* Publish events. */
    run_pub(mock_pub, wr_source, wr_evts);

    wr_sz = (int)wr_evts.size();
    for(int i=0; (wr_sz != rd_evts_sz) && (i < 100); ++i) {
        /* Loop & wait for atmost a second */
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    this_thread::sleep_for(chrono::milliseconds(1000));

    term_sub = true;
    term_cap = true;

    thr.join();
    thrc.join();
    EXPECT_EQ(rd_evts.size(), wr_evts.size());
    EXPECT_EQ(rd_cevts_sz,  wr_evts.size());

    zmq_close(mock_pub);

    zmq_ctx_term(zctx);

    delete pxy;
    pxy = NULL;

    /* Provide time for async proxy removal to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    printf("eventd_proxy is tested GOOD\n");
}

TEST(eventd, capture)
{
    printf("Capture TEST started\n");

    atomic<bool> term_sub = false;
    atomic<int> sub_evts_sz = 0;
    internal_events_lst_t sub_evts;
    stats_collector stats_instance;

    /* run_pub details */
    string wr_source("hello");
    internal_events_lst_t wr_evts;

    /* capture related */
    int init_cache = 3;     /* provided along with start capture */
    int cache_max = init_cache + 3; /* capture service cache max */

    /* startup strings; expected list & read list from capture */
    event_serialized_lst_t evts_start, evts_expect, evts_read;
    last_events_t last_evts_exp, last_evts_read;
    counters_t overflow, overflow_exp = 0;

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    /* Run the proxy; Capture service reads from proxy */
    eventd_proxy *pxy = new eventd_proxy(zctx);
    EXPECT_TRUE(NULL != pxy);

    /* Starting proxy */
    EXPECT_EQ(0, pxy->init());

    /* Create capture service */
    capture_service *pcap = new capture_service(zctx, cache_max, &stats_instance);

    /* Expect START_CAPTURE */
    EXPECT_EQ(-1, pcap->set_control(STOP_CAPTURE));

    /* Initialize the capture */
    EXPECT_EQ(0, pcap->set_control(INIT_CAPTURE));

    /* Run subscriber; Else publisher will drop events on floor, with no subscriber. */
    thread thr_sub(&run_sub, zctx, ref(term_sub), ref(sub_evts), ref(sub_evts_sz));

    EXPECT_TRUE(init_cache > 1);
    EXPECT_TRUE((cache_max+3) < (int)ARRAY_SIZE(ldata));

    /* Collect few serailized strings of events for startup cache */
    for(int i=0; i < init_cache; ++i) {
        internal_event_t ev(create_ev(ldata[i]));
        string evt_str;
        serialize(ev, evt_str);
        evts_start.push_back(evt_str);
        evts_expect.push_back(evt_str);
    }

    /*
     * Collect events to publish for capture to cache
     * re-publishing some events sent in cache.
     * Hence i=1, when first init_cache events are already
     * in crash.
     */
    for(int i=1; i < (int)ARRAY_SIZE(ldata); ++i) {
        internal_event_t ev(create_ev(ldata[i]));
        string evt_str;

        serialize(ev, evt_str);

        wr_evts.push_back(ev);

        if (i < cache_max) {
            if (i >= init_cache) {
                /* for i < init_cache, evts_expect is already populated */
                evts_expect.push_back(evt_str);
            }
        } else {
            /* collect last entries for overflow */
            last_evts_exp[ldata[i].rid] = evt_str;
            overflow_exp++;
        }
    }
    overflow_exp -= (int)last_evts_exp.size();

    EXPECT_EQ(0, pcap->set_control(START_CAPTURE, &evts_start));

    /* Init pub connection */
    void *mock_pub = init_pub(zctx);

    /* Publish events from 1 to all. */
    run_pub(mock_pub, wr_source, wr_evts);

    /* Provide time for async message receive. */
    this_thread::sleep_for(chrono::milliseconds(200));

    /* Stop capture, closes socket & terminates the thread */
    EXPECT_EQ(0, pcap->set_control(STOP_CAPTURE));

    /* terminate subs thread */
    term_sub = true;

    /* Read the cache */
    EXPECT_EQ(0, pcap->read_cache(evts_read, last_evts_read, overflow));

#ifdef DEBUG_TEST
    if ((evts_read.size() != evts_expect.size()) ||
            (last_evts_read.size() != last_evts_exp.size())) {
        printf("size: sub_evts_sz=%d sub_evts=%d\n", sub_evts_sz, (int)sub_evts.size());
        printf("init_cache=%d cache_max=%d\n", init_cache, cache_max);
        printf("overflow=%ul overflow_exp=%ul\n", overflow, overflow_exp);
        printf("evts_start=%d evts_expect=%d evts_read=%d\n",
                (int)evts_start.size(), (int)evts_expect.size(), (int)evts_read.size());
        printf("last_evts_exp=%d last_evts_read=%d\n", (int)last_evts_exp.size(),
                (int)last_evts_read.size());
    }
#endif

    EXPECT_EQ(evts_read.size(), evts_expect.size());
    EXPECT_EQ(evts_read, evts_expect);
    EXPECT_EQ(last_evts_read.size(), last_evts_exp.size());
    EXPECT_EQ(last_evts_read, last_evts_exp);
    EXPECT_EQ(overflow, overflow_exp);

    delete pcap;
    pcap = NULL;

    thr_sub.join();

    zmq_close(mock_pub);
    zmq_ctx_term(zctx);

    delete pxy;
    pxy = NULL;

    /* Provide time for async proxy removal to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    printf("Capture TEST completed\n");
}

TEST(eventd, captureCacheMax)
{
    printf("Capture TEST with matchinhg cache-max started\n");

    /*
     * Need to run subscriber; Else publisher would skip publishing
     * in the absence of any subscriber.
     */
    atomic<bool> term_sub = false;
    atomic<int> sub_evts_sz = 0;
    internal_events_lst_t sub_evts;
    stats_collector stats_instance;

    /* run_pub details */
    string wr_source("hello");
    internal_events_lst_t wr_evts;

    /* capture related */
    int init_cache = 4;     /* provided along with start capture */
    int cache_max = ARRAY_SIZE(ldata); /* capture service cache max */

    /* startup strings; expected list & read list from capture */
    event_serialized_lst_t evts_start, evts_expect, evts_read;
    last_events_t last_evts_read;
    counters_t overflow;

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    /* Run the proxy; Capture service reads from proxy */
    eventd_proxy *pxy = new eventd_proxy(zctx);
    EXPECT_TRUE(NULL != pxy);

    /* Starting proxy */
    EXPECT_EQ(0, pxy->init());

    /* Create capture service */
    capture_service *pcap = new capture_service(zctx, cache_max, &stats_instance);

    /* Expect START_CAPTURE */
    EXPECT_EQ(-1, pcap->set_control(STOP_CAPTURE));

    EXPECT_TRUE(init_cache > 1);

    /* Run subscriber; Else publisher will drop events on floor, with no subscriber. */
    thread thr_sub(&run_sub, zctx, ref(term_sub), ref(sub_evts), ref(sub_evts_sz));

    /* Collect few serailized strings of events for startup cache */
    for(int i=0; i < init_cache; ++i) {
        internal_event_t ev(create_ev(ldata[i]));
        string evt_str;
        serialize(ev, evt_str);
        evts_start.push_back(evt_str);
        evts_expect.push_back(evt_str);
    }

    /*
     * Collect events to publish for capture to cache
     * re-publishing some events sent in cache.
     */
    for(int i=1; i < (int)ARRAY_SIZE(ldata); ++i) {
        internal_event_t ev(create_ev(ldata[i]));
        string evt_str;

        serialize(ev, evt_str);

        wr_evts.push_back(ev);

        if (i >= init_cache) {
            /* for i < init_cache, evts_expect is already populated */
            evts_expect.push_back(evt_str);
        }
    }

    EXPECT_EQ(0, pcap->set_control(INIT_CAPTURE));
    EXPECT_EQ(0, pcap->set_control(START_CAPTURE, &evts_start));

    /* Init pub connection */
    void *mock_pub = init_pub(zctx);

    /* Publish events from 1 to all. */
    run_pub(mock_pub, wr_source, wr_evts);

    /* Provide time for async message receive. */
    this_thread::sleep_for(chrono::milliseconds(100));

    /* Stop capture, closes socket & terminates the thread */
    EXPECT_EQ(0, pcap->set_control(STOP_CAPTURE));

    /* terminate subs thread */
    term_sub = true;

    /* Read the cache */
    EXPECT_EQ(0, pcap->read_cache(evts_read, last_evts_read, overflow));

#ifdef DEBUG_TEST
    if ((evts_read.size() != evts_expect.size()) ||
            !last_evts_read.empty()) {
        printf("size: sub_evts_sz=%d sub_evts=%d\n", sub_evts_sz, (int)sub_evts.size());
        printf("init_cache=%d cache_max=%d\n", init_cache, cache_max);
        printf("evts_start=%d evts_expect=%d evts_read=%d\n",
                (int)evts_start.size(), (int)evts_expect.size(), (int)evts_read.size());
        printf("last_evts_read=%d\n", (int)last_evts_read.size());
        printf("overflow=%ul overflow_exp=%ul\n", overflow, overflow_exp);
    }
#endif

    EXPECT_EQ(evts_read, evts_expect);
    EXPECT_TRUE(last_evts_read.empty());
    EXPECT_EQ(overflow, 0);

    delete pcap;
    pcap = NULL;

    thr_sub.join();

    zmq_close(mock_pub);
    zmq_ctx_term(zctx);

    delete pxy;
    pxy = NULL;

    /* Provide time for async proxy removal to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    printf("Capture TEST with matchinhg cache-max completed\n");
}

TEST(eventd, service)
{
    /*
     * Don't PUB/SUB events as main run_eventd_service itself
     * is using zmq_message_read. Any PUB/SUB will cause
     * eventd's do_capture running in another thread to call
     * zmq_message_read, which will crash as boost:archive is
     * not thread safe.
     * TEST(eventd, capture) has already tested caching.
     */
    printf("Service TEST started\n");

    /* startup strings; expected list & read list from capture */
    event_service service;

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    /*
     * Start the eventd server side service
     * It runs proxy & capture service
     * It uses its own zmq context
     * It starts to capture too.
     */

    if (!g_is_redis_available) {
        set_unit_testing(true);
    }

    thread thread_service(&run_eventd_service);
    this_thread::sleep_for(chrono::milliseconds(CAPTURE_SERVICE_POLLING_DURATION * CAPTURE_SERVICE_POLLING_RETRIES));

    /* Need client side service to interact with server side */
    EXPECT_EQ(0, service.init_client(zctx));

    {
        /* eventd_service starts cache too; Test this caching */
        /* Init pub connection */
        void *mock_pub = init_pub(zctx);
        EXPECT_TRUE(NULL != mock_pub);

        internal_events_lst_t wr_evts;
        int wr_sz = 2;
        string wr_source("hello");

        /* Test service startup caching */
        event_serialized_lst_t evts_start, evts_read, polled_events;

        for(int i=0; i<wr_sz; ++i) {
            string evt_str;
            internal_event_t ev(create_ev(ldata[i]));

            wr_evts.push_back(ev);
            serialize(ev, evt_str);
            evts_start.push_back(evt_str);
        }

        /* Publish events. */
        run_pub(mock_pub, wr_source, wr_evts);

        int max_polling_duration = 2000;
        int polling_interval = 100;
        auto poll_start_ts = chrono::steady_clock::now();

        while(true) {
            auto current_ts = chrono::steady_clock::now();
            if(chrono::duration_cast<chrono::milliseconds>(current_ts - poll_start_ts).count() >= max_polling_duration) {
                break;
            }
            event_serialized_lst_t read_events;
            service.cache_read(read_events);
            polled_events.insert(polled_events.end(), read_events.begin(), read_events.end());
            if (!read_events.empty()) {
                break;
            }
            this_thread::sleep_for(chrono::milliseconds(polling_interval));	    
        }

        EXPECT_EQ(0, service.cache_stop());

        /* Read remaining events in cache, if any */
        EXPECT_EQ(0, service.cache_read(evts_read));

        polled_events.insert(polled_events.end(), evts_read.begin(), evts_read.end());

        EXPECT_EQ(polled_events, evts_start);

        zmq_close(mock_pub);
    }

    {
        /* Test normal cache op; init, start & stop via event_service APIs */
        int init_cache = 4;     /* provided along with start capture */
        event_serialized_lst_t evts_start, evts_read;
        vector<internal_event_t> evts_start_int;

        EXPECT_TRUE(init_cache > 1);

        /* Collect few serailized strings of events for startup cache */
        for(int i=0; i < init_cache; ++i) {
            internal_event_t ev(create_ev(ldata[i]));
            string evt_str;
            serialize(ev, evt_str);
            evts_start.push_back(evt_str);
            evts_start_int.push_back(ev);
        }


        EXPECT_EQ(0, service.cache_init());
        EXPECT_EQ(0, service.cache_start(evts_start));

        this_thread::sleep_for(chrono::milliseconds(200));

        /* Stop capture, closes socket & terminates the thread */
        EXPECT_EQ(0, service.cache_stop());

        /* Read the cache */
        EXPECT_EQ(0, service.cache_read(evts_read));

        if (evts_read != evts_start) {
            vector<internal_event_t> evts_read_int;

            for (event_serialized_lst_t::const_iterator itc = evts_read.begin();
                    itc != evts_read.end(); ++itc) {
                internal_event_t event;

                if (deserialize(*itc, event) == 0) {
                    evts_read_int.push_back(event);
                }
            }
            EXPECT_EQ(evts_read_int, evts_start_int);
        }
    }

    {
        string set_opt_bad("{\"HEARTBEAT_INTERVAL\": 2000, \"OFFLINE_CACHE_SIZE\": 500}");
        string set_opt_good("{\"HEARTBEAT_INTERVAL\":5}");
        char buff[100];
        buff[0] = 0;

        EXPECT_EQ(-1, service.global_options_set(set_opt_bad.c_str()));
        EXPECT_EQ(0, service.global_options_set(set_opt_good.c_str()));
        EXPECT_LT(0, service.global_options_get(buff, sizeof(buff)));

        EXPECT_EQ(set_opt_good, string(buff));
    }

    EXPECT_EQ(0, service.send_recv(EVENT_EXIT));

    service.close_service();

    thread_service.join();

    zmq_ctx_term(zctx);
    printf("Service TEST completed\n");
}


void
wait_for_heartbeat(stats_collector &stats_instance, long unsigned int cnt,
        int wait_ms = 3000)
{
    auto st = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    while (stats_instance.heartbeats_published() == cnt) {
        auto en = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        auto diff = en - st;
        if (diff > wait_ms) {
            EXPECT_LE(diff, wait_ms);
            EXPECT_EQ(cnt, stats_instance.heartbeats_published());
            break;
        }
        else {
            stringstream ss;
            ss << (en -st);
        }
        this_thread::sleep_for(chrono::milliseconds(300));
    }
}

TEST(eventd, heartbeat)
{
    printf("heartbeat TEST started\n");

    int rc;
    long unsigned int cnt;
    stats_collector stats_instance;

    if (!g_is_redis_available) {
        set_unit_testing(true);
    }

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    eventd_proxy *pxy = new eventd_proxy(zctx);
    EXPECT_TRUE(NULL != pxy);

    /* Starting proxy */
    EXPECT_EQ(0, pxy->init());

    rc = stats_instance.start();
    EXPECT_EQ(rc, 0);

    /* Wait for any non-zero heartbeat */
    wait_for_heartbeat(stats_instance, 0);

    /* Pause heartbeat */
    stats_instance.heartbeat_ctrl(true);

    /* Sleep to ensure the other thread noticed the pause request. */
    this_thread::sleep_for(chrono::milliseconds(200));

    /* Get current count */
    cnt = stats_instance.heartbeats_published();

    /* Wait for 3 seconds with no new neartbeat */
    this_thread::sleep_for(chrono::seconds(3));

    EXPECT_EQ(stats_instance.heartbeats_published(), cnt);

    /* Set interval as 1 second */
    stats_instance.set_heartbeat_interval(1);

    /* Turn on heartbeat */
    stats_instance.heartbeat_ctrl();

    /* Wait for heartbeat count to change from last count */
    wait_for_heartbeat(stats_instance, cnt, 2000);

    stats_instance.stop();

    zmq_ctx_term(zctx);

    delete pxy;

    printf("heartbeat TEST completed\n");
}


TEST(eventd, testDB)
{
    printf("DB TEST started\n");

    /* consts used */
    const int pub_count = 7;
    const int cache_max = 3;

    stats_collector stats_instance;
    event_handle_t pub_handle;
    event_serialized_lst_t evts_read;
    last_events_t last_evts_read;
    counters_t overflow;
    string tag;

    if (!g_is_redis_available) {
        printf("redis not available; Hence DB TEST skipped\n");
        return;
    }

    EXPECT_LT(cache_max, pub_count);
    DBConnector db("COUNTERS_DB", 0, true);


    /* Not testing heartbeat; Hence set high val as 10 seconds */
    stats_instance.set_heartbeat_interval(10000);

    /* Start instance to capture published count & as well writes to DB */
    EXPECT_EQ(0, stats_instance.start());

    void *zctx = zmq_ctx_new();
    EXPECT_TRUE(NULL != zctx);

    /* Run proxy to enable receive as capture test needs to receive */
    eventd_proxy *pxy = new eventd_proxy(zctx);
    EXPECT_TRUE(NULL != pxy);

    /* Starting proxy */
    EXPECT_EQ(0, pxy->init());

    /* Create capture service */
    capture_service *pcap = new capture_service(zctx, cache_max, &stats_instance);

    /* Initialize the capture */
    EXPECT_EQ(0, pcap->set_control(INIT_CAPTURE));

    /* Kick off capture */
    EXPECT_EQ(0, pcap->set_control(START_CAPTURE));

    pub_handle = events_init_publisher("test_db");

    for(int i=0; i < pub_count; ++i) {
        tag = string("test_db_tag_") + to_string(i);
        event_publish(pub_handle, tag);
    }

    /* Pause to ensure all publisghed events did reach capture service */
    this_thread::sleep_for(chrono::milliseconds(200));

    EXPECT_EQ(0, pcap->set_control(STOP_CAPTURE));

    /* Read the cache */
    EXPECT_EQ(0, pcap->read_cache(evts_read, last_evts_read, overflow));

    /*
     * Sent pub_count messages of different tags.
     * Upon cache max, only event per sender/runtime-id is saved. Hence
     * expected last_evts_read is one.
     * expected overflow = pub_count - cache_max - 1
     */

    EXPECT_EQ(cache_max, (int)evts_read.size());
    EXPECT_EQ(1, (int)last_evts_read.size());
    EXPECT_EQ((pub_count - cache_max - 1), overflow);

    EXPECT_EQ(pub_count, stats_instance.read_counter(
                INDEX_COUNTERS_EVENTS_PUBLISHED));
    EXPECT_EQ((pub_count - cache_max - 1), stats_instance.read_counter(
                INDEX_COUNTERS_EVENTS_MISSED_CACHE));

    events_deinit_publisher(pub_handle);

    for (int i=0; i < COUNTERS_EVENTS_TOTAL; ++i) {
        string key = string("COUNTERS_EVENTS:") + counter_keys[i];
        unordered_map<string, string> m;
        bool key_found = false, val_found=false, val_match=false;

        if (db.exists(key)) {
            try {
                m = db.hgetall(key);
                unordered_map<string, string>::const_iterator itc =
                    m.find(string(EVENTS_STATS_FIELD_NAME));
                if (itc != m.end()) {
                    int expect =  (counter_keys[i] == string(COUNTERS_EVENTS_PUBLISHED) ?
                            pub_count : (pub_count - cache_max - 1));
                    val_match = (expect == stoi(itc->second) ? true : false);
                    val_found = true;
                }
            }
            catch (exception &e)
            {
                printf("Failed to get key=(%s) err=(%s)", key.c_str(), e.what());
                EXPECT_TRUE(false);
            }
            key_found = true;
        }

        if (!val_match) {
            printf("key=%s key_found=%d val_found=%d fields=%d",
                    key.c_str(), key_found, val_found, (int)m.size());

            printf("hgetall BEGIN key=%s", key.c_str());
            for(unordered_map<string, string>::const_iterator itc = m.begin();
                    itc != m.end(); ++itc) {
                printf("val[%s] = (%s)", itc->first.c_str(), itc->second.c_str());
            }
            printf("hgetall END\n");
            EXPECT_TRUE(false);
        }
    }

    stats_instance.stop();

    delete pcap;

    zmq_ctx_term(zctx);

    delete pxy;

    printf("DB TEST completed\n");
}


// TODO -- Add unit tests for stats
