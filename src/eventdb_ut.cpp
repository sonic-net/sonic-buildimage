#include<string>
#include "gtest/gtest.h"
#include "events_common.h"
#include "events.h"
#include "../src/eventd.h"
#include "../src/eventconsume.h"

using namespace std;
using namespace swss;

typedef struct {
    int key;
    string type-id;
    string resource;
    string action;
    string text;
} event_data_t;

static const event_data_t events[] = {
    {1, "SYSTEM_STATE", "System", "", "System Ready"},
    {2, "USER_LOGIN", "ssh", "", "user testuser logged in"},
};

static const event_data_t alarms[] = {
    {1, "TEMP_THRESHOLD_HIGH", "CPU_SENSOR", "RAISE", "Temperature cross threshold 50C"},
    {2, "INTERFACE_OPER_STATE", "eth1", "RAISE", "Operational Down"},
    {3, "INTERFACE_OPER_STATE", "eth1", "CLEAR", "Operational Down"},
    {4, "TEMP_THRESHOLD_HIGH", "CPU_SENSOR", "CLEAR", "Temperature cross threshold 50C"},    

};

const string event_profile = "default_event.json";
const string event_db_profile = "event_db.json";

//typedef unordered_map<string, string> params;

//params events_lst[] = [("id":" 1", "type-id": SYSTEM_STATE, "resource": "System", "text": "System Ready", "severity" : "INFORMATIONAL"},
//                       {"id":"2", "type-id": USER_LOGIN, "resource": "SSH", "text": "User login", "severity": "WARNING"}];
/*
 1) "time-created"
 2) "1706507049241598976"
 3) "type-id"
 4) "SYSTEM_STATUS"
 5) "text"
 6) "System is ready"
 7) "resource"
 8) "system_status"
 9) "severity"
10) "INFORMATIONAL"
11) "id"
12) "4"
*/

class EventDbFixture : public ::testing::Environment { 
    protected:
    void SetUp() override {
        zctx = zmq_ctx_new();
        EXPECT_TRUE(NULL != zctx);

        /* Run proxy to enable receive as capture test needs to receive */
        eventd_proxy *pxy = new eventd_proxy(zctx);
        EXPECT_TRUE(NULL != pxy);

        /* Starting proxy */
        EXPECT_EQ(0, pxy->init());

        /* Start Eventdb in a separate thread*/
        swss::DBConnector eventDb("EVENT_DB", 0);

        EventConsume evtd(&eventDb, event_profile, event_db_profile);
        thread thr(&EventConsume::run, &evtd);
    }
    
    void TearDown() override {

    }
    void *zctx;
}

void *init_publish(void *zctx)
{
    void *mock_pub = zmq_socket (zctx, ZMQ_PUB);
    EXPECT_TRUE(NULL != mock_pub);
    EXPECT_EQ(0, zmq_connect(mock_pub, get_config(XSUB_END_KEY).c_str()));

    /* Provide time for async connect to complete */
    this_thread::sleep_for(chrono::milliseconds(200));

    return mock_pub;
}


TEST_F(EventDbFixture, validate_events)
{
    DBConnector db("EVENT_DB", 0, true);
    void* pub_handle = init_publish(zctx);
//    pub_handle = events_init_publisher("test_db");    
    for(int i = 0; i < ARRAY_SIZE(events); i++) {
        string tag = string("test_db_tag_") + to_string(i);
        const event_params_t params = {{"type-id", events[i].type-id},
                                 { "resource", events[i].resource},
                                 { "action", events[i].action},
                                 { "text", events[i].text}};

        event_publish(pub_handle, tag, &params);
    }
}
