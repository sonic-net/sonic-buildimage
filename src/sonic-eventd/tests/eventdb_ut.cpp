#include<string>
#include "gtest/gtest.h"
#include "events_common.h"
#include "events.h"

#include<sstream>
#include "../src/eventd.h"
#include "../src/eventconsume.h"

using namespace std;
using namespace swss;

extern volatile bool g_run;
extern uint64_t seq_id;
extern uint64_t PURGE_SECONDS;
extern unordered_map<string, uint64_t> cal_lookup_map;
typedef pair<uint64_t, uint64_t> pi;
extern priority_queue<pi, vector<pi>, greater<pi> > event_history_list;
extern EventMap static_event_table;

extern void run_pub(void *mock_pub, const string wr_source, internal_events_lst_t &lst);

typedef struct {
    map<string, string> ev_data;
    string severity;
} ev_data_struct;
map<pair<string, string>, ev_data_struct> event_data_t = {
    {{"SYSTEM_STATE","NOTIFY"}, 
      {{{"type-id","SYSTEM_STATE"}, {"resource", "system-state"}, {"text", "System Ready"}},
      "INFORMATIONAL"}},
    {{"INTERFACE_OPER_STATE", "NOTIFY"}, 
      {{{"type-id", "INTERFACE_OPER_STATE"}, {"resource", "Ethernet1"}, {"text", "Operational Down"}, {"state", "up"}},
      "INFORMATIONAL"}},  
    {{"SENSOR_TEMP_HIGH", "RAISE"},
        {{{"type-id", "SENSOR_TEMP_HIGH"}, {"resource", "cpu_sensor"}, {"action", "RAISE"}, {"text", "sensor temp 55C, threshold temp 52C"}},
        "WARNING"}},
    {{"SENSOR_TEMP_HIGH", "CLEAR"},
        {{{"type-id", "SENSOR_TEMP_HIGH"}, {"resource", "cpu_sensor"}, {"action", "CLEAR"}, {"text", "sensor temp 50C, threshold temp 52C"}},
        "WARNING"}}
};


typedef struct {
    int id;
    string source;
    string tag;
    string rid;
    string seq;
} test_data_t;

const string event_profile = "tests/default.json";
const string event_db_profile = "tests/eventd.json";

void delete_evdb(DBConnector& dbConn)
{
    auto keys = dbConn.keys("*");
    for (const auto& key : keys)
    {
        dbConn.del(key);
    }
}

void clear_eventdb_data()
{
    g_run = true;
    seq_id =0;
    cal_lookup_map.clear();
    PURGE_SECONDS = 86400;
    event_history_list = priority_queue<pi, vector<pi>, greater<pi> >();
    static_event_table.clear();
}


class EventDbFixture : public ::testing::Test { 
    protected:
    void SetUp() override {
        zctx = zmq_ctx_new();
        EXPECT_TRUE(NULL != zctx);

        /* Run proxy to enable receive as capture test needs to receive */
        pxy = new eventd_proxy(zctx);
        EXPECT_TRUE(NULL != pxy);

        /* Starting proxy */
        EXPECT_EQ(0, pxy->init());
        DBConnector eventDb("EVENT_DB", 0, true);
        //delete any entries in the EVENT_DB        
        delete_evdb(eventDb);
        try
        {
            /* Start Eventdb in a separate thread*/
            evtConsume= new EventConsume(&eventDb, event_profile, event_db_profile);
            thread thr(&EventConsume::run, evtConsume);
            thr.detach();
        }
        catch (exception &e)
        {
            printf("EventDbFixture::SetUP: Unable to get DB Connector, e=(%s)\n", e.what());
       }
    }
    
    void TearDown() override {
        delete evtConsume;
        evtConsume = NULL;
        delete pxy;
        pxy= NULL;
        zmq_ctx_term(zctx);
        zctx = NULL;
        clear_eventdb_data();
    }
    EventConsume *evtConsume;
    void *zctx;
    eventd_proxy *pxy;
};


void *init_publish(void *zctx)
{
    void *mock_pub = zmq_socket (zctx, ZMQ_PUB);
    EXPECT_TRUE(NULL != mock_pub);
    EXPECT_EQ(0, zmq_connect(mock_pub, get_config(XSUB_END_KEY).c_str()));

    /* Provide time for async connect to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    return mock_pub;
}

internal_event_t create_ev(const int id, const int ev_id, const string& event,
                           const string& action,
                           map<string, unordered_map<string, string>> &verify_data)
{
    internal_event_t event_data;
    stringstream ss;
    //new data
    test_data_t data = {id, "source" +id, "tag" + id, "guid-" + id, to_string(id)};

    event_data[EVENT_STR_DATA] = convert_to_json(
            data.source + ":" + data.tag, map(event_data_t[make_pair(event, action)].ev_data));
    event_data[EVENT_RUNTIME_ID] = data.rid;
    event_data[EVENT_SEQUENCE] = data.seq;
    auto timepoint = system_clock::now();
    ss << duration_cast<nanoseconds>(timepoint.time_since_epoch()).count();

    event_data[EVENT_EPOCH] = ss.str();
    unordered_map<string, string> ev_val(event_data_t[make_pair(event, action)].ev_data.begin(),
                                         event_data_t[make_pair(event, action)].ev_data.end());
    ev_val.insert({"id", to_string(ev_id)});
    ev_val.insert({"time-created", ss.str()});
    ev_val.insert({"severity", event_data_t[make_pair(event, action)].severity});
 
    if (action == "RAISE") {
        ev_val.insert({"acknowledged", "false"});
        ev_val.insert({"action", action});
    }    
    verify_data.insert({to_string(ev_id), ev_val});

    return event_data;
}

#if 0
void run_pub(void *mock_pub, const string wr_source, internal_events_lst_t &lst)
{
    for(internal_events_lst_t::const_iterator itc = lst.begin(); itc != lst.end(); ++itc) {
        EXPECT_EQ(0, zmq_message_send(mock_pub, wr_source, *itc));
    }
}
#endif

void verify_events(map<string, unordered_map<string, string>> verifyData) 
{
    DBConnector eventDb("EVENT_DB", 0, true);
    auto dbKeys = eventDb.keys("EVENT:*");
    EXPECT_EQ(verifyData.size(), dbKeys.size());
    for (const auto& vKey : verifyData)
    {
        string evtKey = "EVENT:" + vKey.first;
        EXPECT_TRUE(count(dbKeys.begin(), dbKeys.end(), evtKey) == 1);
        auto ev = eventDb.hgetall(evtKey);
        EXPECT_TRUE(ev == verifyData[vKey.first]);
    }
}


void verify_alarms_clear(map<string, unordered_map<string, string>> verifyData)
{
    DBConnector eventDb("EVENT_DB", 0, true);
    auto dbKeys = eventDb.keys("ALARM:*");
    EXPECT_EQ(0, dbKeys.size());
}

void verify_alarms_raise(map<string, unordered_map<string, string>> verifyData) 
{
    DBConnector eventDb("EVENT_DB", 0, true);
    auto dbKeys = eventDb.keys("ALARM:*");
    EXPECT_EQ(verifyData.size(), dbKeys.size());

    for (const auto& vKey : verifyData)
    {
        string almKey = "ALARM:" + vKey.first;
        EXPECT_TRUE(count(dbKeys.begin(), dbKeys.end(), almKey) == 1);
        auto ev = eventDb.hgetall(almKey);
        EXPECT_TRUE(ev == verifyData[vKey.first]);
    }
}

TEST_F(EventDbFixture, validate_events)
{  
    internal_events_lst_t wr_evts;
    string wr_source("eventd-test");        

    void *mock_pub = init_publish(zctx);

    map<string, unordered_map<string, string>> verify_data;

    wr_evts.push_back(create_ev(1, 1, "SENSOR_TEMP_HIGH", "RAISE", verify_data));
    wr_evts.push_back(create_ev(2, 2, "SYSTEM_STATE", "NOTIFY", verify_data));

    run_pub(mock_pub, wr_source, wr_evts);

    this_thread::sleep_for(chrono::milliseconds(2000));
  
    // verify events logged in DB.
    verify_events(verify_data);

    //send events to close eventdb task
    g_run = false;
        wr_evts.clear();
    wr_evts.push_back(create_ev(301, 3, "SYSTEM_STATE", "NOTIFY", verify_data));
    run_pub(mock_pub, wr_source, wr_evts);
    this_thread::sleep_for(chrono::milliseconds(2000));
    zmq_close(mock_pub);  
 
}


TEST_F(EventDbFixture, validate_alarms)
{  
    internal_events_lst_t wr_evts;
    string wr_source("eventd-test");
    void *mock_pub = init_publish(zctx);

    map<string, unordered_map<string, string>> verify_data;
    wr_evts.push_back(create_ev(3, 1, "SENSOR_TEMP_HIGH", "RAISE", verify_data));
 
    run_pub(mock_pub, wr_source, wr_evts);

    this_thread::sleep_for(chrono::milliseconds(2000));

    // verify events logged in DB.
    verify_events(verify_data);
    verify_alarms_raise(verify_data);

    wr_evts.clear();
    wr_evts.push_back(create_ev(4, 2, "SENSOR_TEMP_HIGH", "CLEAR", verify_data));

    run_pub(mock_pub, wr_source, wr_evts);
    this_thread::sleep_for(chrono::milliseconds(2000));
    verify_events(verify_data);
    verify_alarms_clear(verify_data);
    g_run = false;
        wr_evts.clear();
    wr_evts.push_back(create_ev(302, 3, "SYSTEM_STATE", "NOTIFY", verify_data));
    run_pub(mock_pub, wr_source, wr_evts); 
    this_thread::sleep_for(chrono::milliseconds(2000));
    zmq_close(mock_pub);    
}


TEST_F(EventDbFixture, expiry_purge)
{
    internal_events_lst_t wr_evts;
    string wr_source("eventd-test");
    void *mock_pub = init_publish(zctx);
    map<string, unordered_map<string, string>> verify_data;
    //set epoch time back to 31 days
    auto timepoint = system_clock::now();
    auto epochTimeNs = duration_cast<nanoseconds>(timepoint.time_since_epoch()).count();
    epochTimeNs = epochTimeNs - (32UL * 24 * 60 * 60 * 1000 * 1000 * 1000);
    auto ev_data = create_ev(5, 1, "SENSOR_TEMP_HIGH", "RAISE", verify_data);

    ev_data[EVENT_EPOCH] = to_string(epochTimeNs);
    verify_data["1"]["time-created"] = ev_data[EVENT_EPOCH];
    wr_evts.push_back(ev_data);

    run_pub(mock_pub, wr_source, wr_evts);
    this_thread::sleep_for(chrono::milliseconds(2000));

    // verify events logged in DB.
    verify_events(verify_data);
    verify_alarms_raise(verify_data);

    wr_evts.clear();
    verify_data.clear();
    wr_evts.push_back(create_ev(6, 2, "SENSOR_TEMP_HIGH", "CLEAR", verify_data));
    run_pub(mock_pub, wr_source, wr_evts);
    this_thread::sleep_for(chrono::milliseconds(2000));

    verify_events(verify_data);
    verify_alarms_clear(verify_data);
    wr_evts.clear();
    g_run = false;
    wr_evts.push_back(create_ev(303, 3, "SYSTEM_STATE", "NOTIFY", verify_data));
    run_pub(mock_pub, wr_source, wr_evts); 
    this_thread::sleep_for(chrono::milliseconds(2000));    
    zmq_close(mock_pub);
}



TEST_F(EventDbFixture, overflow_purge)
{
    internal_events_lst_t wr_evts;
    string wr_source("eventd-test");
    void *mock_pub = init_publish(zctx);
    map<string, unordered_map<string, string>> verify_data;
    int i=0,j=6;

    for (; i <= 198; i+=2, j+=2)
    {
        wr_evts.push_back(create_ev(j+1, i+1, "SENSOR_TEMP_HIGH", "RAISE", verify_data));
        wr_evts.push_back(create_ev(j+2, i+2, "SENSOR_TEMP_HIGH", "CLEAR", verify_data));
    }

    run_pub(mock_pub, wr_source, wr_evts);

    this_thread::sleep_for(chrono::milliseconds(5000));

    // verify events logged in DB.
    verify_events(verify_data);
    // This will make it out of limit
    wr_evts.push_back(create_ev(j+1, i+1, "SENSOR_TEMP_HIGH", "RAISE", verify_data));
    run_pub(mock_pub, wr_source, wr_evts);

    this_thread::sleep_for(chrono::milliseconds(2000));

    DBConnector eventDb("EVENT_DB", 0, true);
    auto dbKeys = eventDb.keys("EVENT:*");
    EXPECT_EQ(200, dbKeys.size());
    EXPECT_TRUE(count(dbKeys.begin(), dbKeys.end(), "EVENT:1") == 0);
    EXPECT_TRUE(count(dbKeys.begin(), dbKeys.end(), "EVENT:2") == 1);
    EXPECT_TRUE(count(dbKeys.begin(), dbKeys.end(), "EVENT:" + to_string(i+1)) == 1);
    g_run = false;
    wr_evts.push_back(create_ev(303, 3, "SYSTEM_STATE", "NOTIFY", verify_data));
    run_pub(mock_pub, wr_source, wr_evts); 
    this_thread::sleep_for(chrono::milliseconds(2000));        
    zmq_close(mock_pub);
}
