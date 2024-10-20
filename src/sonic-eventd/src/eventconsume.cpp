
#include <math.h>
#include <syslog.h>
#include <limits>
#include <queue>
#include <unistd.h>
#include "eventconsume.h"
#include "loghandler.h"
#include "eventutils.h"
#include "events_common.h"


using namespace std::chrono;

// map to store sequence-id for alarms
unordered_map<string, uint64_t> cal_lookup_map;

// temporary map to hold merge of default map of events and any event profile
EventMap static_event_table;

bool g_run = true;
uint64_t seq_id = 0;
uint64_t PURGE_SECONDS = 86400;

typedef pair<uint64_t, uint64_t> pi;
priority_queue<pi, vector<pi>, greater<pi> > event_history_list;

map<string, int> SYSLOG_SEVERITY = {
    {EVENT_SEVERITY_CRITICAL_STR,      LOG_ALERT},
    {EVENT_SEVERITY_MAJOR_STR,         LOG_CRIT},
    {EVENT_SEVERITY_MINOR_STR,         LOG_ERR},
    {EVENT_SEVERITY_WARNING_STR,       LOG_WARNING},
    {EVENT_SEVERITY_INFORMATIONAL_STR, LOG_NOTICE}
};

map <int, string> SYSLOG_SEVERITY_STR = {
    {LOG_ALERT   , EVENT_SEVERITY_CRITICAL_STR},
    {LOG_CRIT    , EVENT_SEVERITY_MAJOR_STR},
    {LOG_ERR     , EVENT_SEVERITY_MINOR_STR},
    {LOG_WARNING , EVENT_SEVERITY_WARNING_STR},
    {LOG_NOTICE  , EVENT_SEVERITY_INFORMATIONAL_STR}
};

static string flood_ev_id;
static string flood_ev_action;
static string flood_ev_resource;
static string flood_ev_msg;

EventConsume::EventConsume(DBConnector* dbConn,
                           string evProfile,
                           string dbProfile):
    m_eventTable(dbConn, EVENT_HISTORY_TABLE_NAME),
    m_alarmTable(dbConn, EVENT_CURRENT_ALARM_TABLE_NAME),
    m_eventStatsTable(dbConn, EVENT_STATS_TABLE_NAME),
    m_alarmStatsTable(dbConn, EVENT_ALARM_STATS_TABLE_NAME),
    m_evProfile(evProfile),
    m_dbProfile(dbProfile) {

    // open syslog connection
    openSyslog();

    // init stats
    initStats();

    // populate local queue of event histor table
    read_events();

    // read and apply eventd configuration files 
    // read eventd.json and apply it on history table. 
    // read default and custom profiles, build static_event_table
    read_eventd_config();

    SWSS_LOG_NOTICE("DONE WITH EventConsume constructor");
}

EventConsume::~EventConsume() {
}

void EventConsume::run()
{

    SWSS_LOG_ENTER();
    event_handle_t hsub;
    hsub = events_init_subscriber();
    
    while (g_run) {
        event_receive_op_t evt;
        map_str_str_t evtOp;
        int rc = event_receive(hsub, evt); 
        if (rc != 0) {
            SWSS_LOG_ERROR("Failed to receive rc=%d", rc);
             continue;
        }
        handle_notification(evt);
    }
    events_deinit_subscriber(hsub);
}

void EventConsume::read_eventd_config(bool read_all) {
    // read manifest file for config options
    if (read_all) {
        read_config_and_purge();
    }

    // read from default map
    static_event_table.clear();
    if (!parse(m_evProfile.c_str(), static_event_table)) {
        SWSS_LOG_ERROR("Can not initialize event map");
        closeSyslog();
        exit(0);
    }

    SWSS_LOG_NOTICE("Event map is built as follows:");
    for (auto& x: static_event_table) {
        SWSS_LOG_NOTICE("    %s (%s %s %s)", x.first.c_str(), x.second.severity.c_str(), x.second.enable.c_str(), x.second.static_event_msg.c_str());
    }
}


void EventConsume::handle_notification(const event_receive_op_t& evt)
{
    SWSS_LOG_ENTER();

    string ev_id, ev_msg, ev_src, ev_act, ev_timestamp, ev_type("EVENT"),
           ev_static_msg(""), ev_reckey;
    string ev_sev = string(EVENT_SEVERITY_INFORMATIONAL_STR);
    bool is_raise = false;
    bool is_clear = false;
    bool is_ack = false;
    vector<FieldValueTuple> vec;

    fetchFieldValues(evt, vec, ev_id, ev_msg, ev_src, ev_act, ev_timestamp);

    // flood protection. If a rogue application sends same event repeatedly, throttle repeated instances of that event
    if (isFloodedEvent(ev_src, ev_act, ev_id, ev_msg)) {
        return;
    }

    // get static info
    if (!staticInfoExists(ev_id, ev_act, ev_sev, ev_static_msg, vec)) {
        return;
    }

    // increment save seq-id for the newly received event
    uint64_t new_seq_id = seq_id + 1;

    FieldValueTuple seqfv("id", to_string(new_seq_id));
    vec.push_back(seqfv);

    if (ev_act.length() > 0) {
        SWSS_LOG_DEBUG("ev_act %s", ev_act.c_str());
        ev_type = "ALARM";
        string almkey = ev_id;
        if (!ev_src.empty()) {
            almkey += "|" + ev_src;
        }

        if (ev_act.compare(EVENT_ACTION_RAISE_STR) == 0) {
            is_raise = true;
            // add entry to the lookup map
            cal_lookup_map.insert(make_pair(almkey, new_seq_id));

            // add acknowledged field intializing it to false
            FieldValueTuple seqfv1("acknowledged", "false");
            vec.push_back(seqfv1);
            m_alarmTable.set(to_string(new_seq_id), vec);

            // update alarm counters
            updateAlarmStatistics(ev_sev, ev_act);
        } else if (ev_act.compare(EVENT_ACTION_CLEAR_STR) == 0) {
            is_clear = true;
            SWSS_LOG_DEBUG(" Received clear alarm for %s", almkey.c_str());

            bool ack_flag = false;
            // remove entry from local cache, alarm table
            if (!udpateLocalCacheAndAlarmTable(almkey, ack_flag)) {
                SWSS_LOG_ERROR("Received clear for non-existent alarm %s", almkey.c_str());
                return;
            }

            // update alarm counters ONLY if it has not been ack'd before.
            // This is because when alarm is ack'd, alarms/severity counter is reduced already.
            if (!ack_flag) {
                updateAlarmStatistics(ev_sev, ev_act);
            } else {
                // if it has been ack'd before, ack counter would have been incremented for this alrm.
                // Now is the time reduce it.
                clearAckAlarmStatistic();
            }
        } else {
            // ack/unack events comes with seq-id of raised alarm as resource field.
            // fetch details of "raised" alarm record
            string raise_act;
            string raise_ack_flag;
            string raise_ts;

            // fetch information from raised event record
            if (!fetchRaiseInfo(vec, ev_src, ev_id, ev_sev, raise_act, raise_ack_flag, raise_ts))
            {
                SWSS_LOG_ERROR("Action %s on a non-existent Alarm id %s", ev_act.c_str(), ev_src.c_str());
                return;
            }

            if (ev_act.compare(EVENT_ACTION_ACK_STR) == 0) {
                if (raise_ack_flag.compare("true") == 0) {
                    SWSS_LOG_INFO("%s/%s is already acknowledged", ev_id.c_str(), ev_src.c_str());
                    return;
                }
                if (raise_act.compare(EVENT_ACTION_RAISE_STR) == 0) {
                    is_ack = true;
                    SWSS_LOG_DEBUG("Received acknowledge event - %s/%s", ev_id.c_str(), ev_src.c_str());

                    // update the record with ack flag and ack-time and stats
                    updateAckInfo(is_ack, ev_timestamp, ev_sev, ev_act, ev_src);
                } else {
                    SWSS_LOG_ERROR("Alarm %s/%s not in RAISE state", ev_id.c_str(), ev_src.c_str());
                    return;
                }
            } else if (ev_act.compare(EVENT_ACTION_UNACK_STR) == 0) {
                if (raise_ack_flag.compare("true") == 0) {
                    SWSS_LOG_DEBUG(" received un-ACKnowledge event - %s/%s", ev_id.c_str(), ev_src.c_str());

                    // update the record with ack flag and ack-time and stats
                    updateAckInfo(is_ack, ev_timestamp, ev_sev, ev_act, ev_src);
                } else {
                    SWSS_LOG_INFO(" %s/%s is already un-acknowledged", ev_id.c_str(), ev_src.c_str());
                    return;
                }
            }
        }
    }
    // verify the size of history table; delete older entry; add new entry
    seq_id = new_seq_id;
    update_events(to_string(seq_id), ev_timestamp, vec);

    updateEventStatistics(true, is_raise, is_ack, is_clear);

    // raise a syslog message
    writeToSyslog(ev_id, (int)(SYSLOG_SEVERITY.find(ev_sev)->second), ev_type, ev_act, ev_msg, ev_static_msg);

    return;
}

void EventConsume::read_events() {
    vector<KeyOpFieldsValuesTuple> tuples;
    m_eventTable.getContent(tuples);

    SWSS_LOG_ENTER();
    // find out last sequence-id; build local history list
    for (auto tuple: tuples) {
        for (auto fv: kfvFieldsValues(tuple)) {
            if (fvField(fv) == "time-created") {
                char* end;
                uint64_t seq = strtoull(kfvKey(tuple).c_str(), &end,10);
                if (seq > seq_id) {
                    seq_id = seq;
                }
                uint64_t val = strtoull(fvValue(fv).c_str(), &end,10);
                event_history_list.push(make_pair( seq, val ));
            }
        }
    }
    SWSS_LOG_NOTICE("eventd sequence-id intialized to %lu", seq_id);
}

void EventConsume::updateAlarmStatistics(string ev_sev, string ev_act) {
    vector<FieldValueTuple> vec;
    vector<FieldValueTuple> temp;

    // severity counter names are of lower case
    transform(ev_sev.begin(), ev_sev.end(), ev_sev.begin(), ::tolower);

    if (m_alarmStatsTable.get("state", vec)) {
        for (auto fv: vec) {
            if (!fv.first.compare("alarms")) {
                if ((ev_act.compare(EVENT_ACTION_RAISE_STR) == 0) || (ev_act.compare(EVENT_ACTION_UNACK_STR) == 0)) {
                    fv.second = to_string(stoi(fv.second.c_str())+1);
                } else {
                    fv.second = to_string(stoi(fv.second.c_str())-1);
                }
                temp.push_back(fv);
            } else if (!fv.first.compare(ev_sev)) {
                if ((ev_act.compare(EVENT_ACTION_RAISE_STR) == 0) || (ev_act.compare(EVENT_ACTION_UNACK_STR) == 0)) {
                    fv.second = to_string(stoi(fv.second.c_str())+1);
                } else {
                    fv.second = to_string(stoi(fv.second.c_str())-1);
                }
                temp.push_back(fv);
            } else if (!fv.first.compare("acknowledged")) {
                if (ev_act.compare(EVENT_ACTION_ACK_STR) == 0)  {
                    fv.second = to_string(stoi(fv.second.c_str())+1);
                } else if (ev_act.compare(EVENT_ACTION_UNACK_STR) == 0) { 
                    fv.second = to_string(stoi(fv.second.c_str())-1);
                }
                temp.push_back(fv);
            }
        }
        m_alarmStatsTable.set("state", temp);
    } else {
        SWSS_LOG_ERROR("Can not update alarm statistics (table does not exist)");
    }
}

void EventConsume::updateEventStatistics(bool is_add, bool is_raise, bool is_ack, bool is_clear) {
    vector<FieldValueTuple> vec;
    vector<FieldValueTuple> temp;

    if (m_eventStatsTable.get("state", vec)) {
        for (auto fv: vec) {
            if (!fv.first.compare("events")) {
                if (is_add) {
                    fv.second = to_string(stoi(fv.second.c_str())+1);
                } else {
                    fv.second = to_string(stoi(fv.second.c_str())-1);
                }
                temp.push_back(fv);
            } else if (!fv.first.compare("raised")) {
                if (is_raise) {
                    if (is_add) {
                        fv.second = to_string(stoi(fv.second.c_str())+1);
                    } else {
                        fv.second = to_string(stoi(fv.second.c_str())-1);
                    }
                    temp.push_back(fv);
                }
            } else if (!fv.first.compare("cleared")) {
                if (is_clear) {
                    if (is_add) {
                        fv.second = to_string(stoi(fv.second.c_str())+1);
                    } else {
                        fv.second = to_string(stoi(fv.second.c_str())-1);
                    }
                    temp.push_back(fv);
                }
            } else if (!fv.first.compare("acked")) {
                if (is_ack) {
                    if (is_add) {
                        fv.second = to_string(stoi(fv.second.c_str())+1);
                    } else {
                        fv.second = to_string(stoi(fv.second.c_str())-1);
                    }
                    temp.push_back(fv);
                }
            }
        }

        m_eventStatsTable.set("state", temp);
    } else {
        SWSS_LOG_ERROR("Can not update event statistics (table does not exist)");
    }
}

void EventConsume::modifyEventStats(string seq_id) { 
    vector<FieldValueTuple> rec;
    m_eventTable.get(seq_id, rec);
    bool is_raise = false;
    bool is_clear = false;
    bool is_ack = false;
    for (auto fvr: rec) {
        if (!fvr.first.compare("action")) {
            if (!fvr.second.compare(EVENT_ACTION_RAISE_STR)) {
                is_raise = true;
            } else if (!fvr.second.compare(EVENT_ACTION_CLEAR_STR)) {
                is_clear = true;
            }
        }
        if (!fvr.first.compare("acknowledged")) {
            if (!fvr.second.compare("true")) {
                is_ack = true;
            }
        }
    }
    updateEventStatistics(false, is_raise, is_ack, is_clear);
}

void EventConsume::purge_events() {
    SWSS_LOG_ENTER();
    uint32_t size = event_history_list.size();

    while (size >= m_count) {
        pair <uint64_t,uint64_t> oldest_entry = event_history_list.top();
        SWSS_LOG_NOTICE("Rollover based on count(%d/%d). Deleting %lu", size, m_count, oldest_entry.first);
        m_eventTable.del(to_string(oldest_entry.first));
        modifyEventStats(to_string(oldest_entry.first));
        event_history_list.pop();
        --size;
    }

    const auto p1 = system_clock::now();
    unsigned tnow_seconds = duration_cast<seconds>(p1.time_since_epoch()).count();

    while (!event_history_list.empty()) {
        pair <uint64_t,uint64_t> oldest_entry = event_history_list.top();
	    unsigned old_seconds = oldest_entry.second / 1000000000ULL;

        if ((tnow_seconds - old_seconds) > PURGE_SECONDS) {
            SWSS_LOG_NOTICE("Rollover based on time (%lu days). Deleting %lu.. now %u old %u", (PURGE_SECONDS/m_days), oldest_entry.second, tnow_seconds, old_seconds);
            m_eventTable.del(to_string(oldest_entry.first));
            modifyEventStats(to_string(oldest_entry.first));
            event_history_list.pop();
        } else {
            return;
        }
    }
    return;
}

void EventConsume::read_config_and_purge() {
    m_days = 0;
    m_count = 0;
    // read from the manifest file
    parse_config(m_dbProfile.c_str(), m_days, m_count);
    SWSS_LOG_NOTICE("max-days %d max-records %d", m_days, m_count);

    // update the nanosecond limit
    PURGE_SECONDS *= m_days; 

    // purge events based on # of days
    purge_events();
}

void EventConsume::update_events(string seq_id, string ts, vector<FieldValueTuple> vec) {
    // purge events based on # of days
    purge_events();

    // now add the event to the event table
    m_eventTable.set(seq_id, vec);

    // store it into the event history list
    char* end;
    uint64_t seq = strtoull(seq_id.c_str(), &end, 10);
    uint64_t val = strtoull(ts.c_str(), &end, 10);
    event_history_list.push(make_pair( seq, val ));
}

void EventConsume::resetAlarmStats(int alarms, int critical, int major, int minor, int warning, int acknowledged) {
    vector<FieldValueTuple> temp;
    FieldValueTuple fv;
    map<int, string>::iterator it;
    for (it = SYSLOG_SEVERITY_STR.begin(); it != SYSLOG_SEVERITY_STR.end(); it++) {
        // there wont be any informational alarms
        if (it->second.compare(EVENT_SEVERITY_CRITICAL_STR) == 0) {
            fv = FieldValueTuple("critical", to_string(critical));
            temp.push_back(fv);
        } else if (it->second.compare(EVENT_SEVERITY_MAJOR_STR) == 0) {
            fv = FieldValueTuple("major", to_string(major));
            temp.push_back(fv);
        } else if (it->second.compare(EVENT_SEVERITY_MINOR_STR) == 0) {
            fv = FieldValueTuple("minor", to_string(minor));
            temp.push_back(fv);
        } else if (it->second.compare(EVENT_SEVERITY_WARNING_STR) == 0) {
            fv = FieldValueTuple("warning", to_string(warning));
            temp.push_back(fv);
        }
    }
    fv = FieldValueTuple("alarms", to_string(alarms));
    temp.push_back(fv);
    fv = FieldValueTuple("acknowledged", to_string(acknowledged));
    temp.push_back(fv);
    m_alarmStatsTable.set("state", temp);
}

void EventConsume::clearAckAlarmStatistic() {
    vector<FieldValueTuple> vec;
    vector<FieldValueTuple> temp;

    if (m_alarmStatsTable.get("state", vec)) {
        for (auto fv: vec) {
            if (!fv.first.compare("acknowledged")) {
                fv.second = to_string(stoi(fv.second.c_str())-1);
                temp.push_back(fv);
                m_alarmStatsTable.set("state", temp);
		return;
            }
        }
    } 
}


void EventConsume::fetchFieldValues(const event_receive_op_t& evt, 
                                    vector<FieldValueTuple>& vec, 
                                    string& ev_id, 
                                    string& ev_msg, 
                                    string& ev_src, 
                                    string& ev_act, 
                                    string &ev_timestamp) {

    ev_timestamp = to_string(evt.publish_epoch_ms);                          
    vec.push_back(FieldValueTuple("time-created", ev_timestamp));
    for (const auto& idx : evt.params) {
        if (idx.first == "type-id") {
            ev_id = idx.second;
            vec.push_back(FieldValueTuple("type-id", ev_id));
            SWSS_LOG_DEBUG("type-id: <%s> ", ev_id.c_str());
        } else if (idx.first == "text") {
            ev_msg = idx.second;
            vec.push_back(FieldValueTuple("text", ev_msg));
            SWSS_LOG_DEBUG("text: <%s> ", ev_msg.c_str());
        } else if (idx.first == "resource") {
            ev_src = idx.second;
            vec.push_back(idx);
            SWSS_LOG_DEBUG("resource: <%s> ", ev_src.c_str());
        } else if (idx.first == "action") {
            ev_act = idx.second;
            // for events, action is empty
            if (!ev_act.empty()) {
                vec.push_back(FieldValueTuple("action", ev_act));
            }
        }
    }
}

bool EventConsume::isFloodedEvent(string ev_src, string ev_act, string ev_id, string ev_msg) {
    // flood protection. If a rogue application sends same event repeatedly, throttle repeated instances of that event
    if (!flood_ev_resource.compare(ev_src) &&
        !flood_ev_action.compare(ev_act) &&
        !flood_ev_id.compare(ev_id) &&
        !(flood_ev_msg.compare(ev_msg))) {
            SWSS_LOG_INFO("Ignoring the event %s from %s action %s msg %s as it is repeated", ev_id.c_str(), ev_src.c_str(), ev_act.c_str(), ev_msg.c_str());
            return true;
    }

    flood_ev_resource = ev_src;
    flood_ev_action = ev_act;
    flood_ev_id = ev_id;
    flood_ev_msg = ev_msg;
    return false;
}

bool EventConsume::staticInfoExists(string &ev_id, string &ev_act, string &ev_sev, string &ev_static_msg, vector<FieldValueTuple> &vec) {
    auto it = static_event_table.find(ev_id);
    if (it != static_event_table.end()) {
        EventInfo tmp = (EventInfo) (it->second);
        // discard the event as event_static_map shows enable is false for this event
        if (tmp.enable == EVENT_ENABLE_FALSE_STR) {
            SWSS_LOG_NOTICE("Discarding event <%s> as it is set to disabled", ev_id.c_str());
            return false;;
        }

        // get severity in the map and store it in the db
        ev_sev = tmp.severity;
        ev_static_msg = tmp.static_event_msg;
        SWSS_LOG_DEBUG("static info: <%s> <%s> ", tmp.severity.c_str(), tmp.static_event_msg.c_str());

        FieldValueTuple seqfv1("severity", tmp.severity);
        vec.push_back(seqfv1);
        return true;
    } else {
        // dont process the incoming alarms if action is neither raise nor clear
        // for ack/unack, no need for this check
        if ((ev_act.compare(EVENT_ACTION_ACK_STR) && ev_act.compare(EVENT_ACTION_UNACK_STR))) {
            // TODO currently, applications may raise events but default evprofile doesnt contain 
            // ID info. This is planned for later. 
            // Change it back to SWSS_LOG_ERROR once event profile contains event-ids
            SWSS_LOG_DEBUG("static info NOT FOUND for <%s> ", ev_id.c_str());
            return false;
        }
    }
    return true;
}

bool EventConsume::udpateLocalCacheAndAlarmTable(string almkey, bool &ack_flag) {
    // find and remove the raised alarm
    uint64_t lookup_seq_id = 0;
    auto it = cal_lookup_map.find(almkey);
    if (it != cal_lookup_map.end()) {
        lookup_seq_id = (uint64_t) (it->second);
        cal_lookup_map.erase(almkey);

        // get status of is_aknowledged flag so that we dont decrement counters twice
        vector<FieldValueTuple> alm_rec;
        m_alarmTable.get(to_string(lookup_seq_id), alm_rec);
        for (auto fvr: alm_rec) {
            if (!fvr.first.compare("acknowledged")) {
                ack_flag = (fvr.second.compare("true") == 0) ? true : false;
                break;
            }
        }

        // delete the record from alarm table
        m_alarmTable.del(to_string(lookup_seq_id));
    } else {
        // possible - when event profile removes alarms for which enable is false and application cleared them later.
        // ignore by logging a debug message..
        SWSS_LOG_INFO("Received alarm-clear for non-existing alarm %s", almkey.c_str());
        return false;
    }
    return true;
}

void EventConsume::initStats() {
    vector<FieldValueTuple> vec;
    // possible after a cold-boot or very first time
    if (! m_eventStatsTable.get("state", vec)) {
        FieldValueTuple fv;
        vector<FieldValueTuple> temp;

        SWSS_LOG_DEBUG("resetting Event Statistics table");
        fv = FieldValueTuple("events", to_string(0));
        temp.push_back(fv);
        fv = FieldValueTuple("raised", to_string(0));
        temp.push_back(fv);
        fv = FieldValueTuple("cleared", to_string(0));
        temp.push_back(fv);
        fv = FieldValueTuple("acked", to_string(0));
        temp.push_back(fv);
        m_eventStatsTable.set("state", temp);
    }
    if (! m_alarmStatsTable.get("state", vec)) {
        SWSS_LOG_DEBUG("resetting Alarm Statistics table");
        resetAlarmStats(0, 0, 0, 0, 0, 0);
    }
}

void EventConsume::updateAckInfo(bool is_ack, string ev_timestamp, string ev_sev, string ev_act, string ev_src) {
    vector<FieldValueTuple> ack_vec;

    FieldValueTuple seqfv1("acknowledged", (is_ack ? "true" : "false"));
    ack_vec.push_back(seqfv1);

    FieldValueTuple seqfv2("acknowledge-time", ev_timestamp);
    ack_vec.push_back(seqfv2);

    // update alarm stats
    updateAlarmStatistics(ev_sev, ev_act);

    // update alarm/event tables for the "raise" record with ack flag and ack timestamp
    // for ack/unack, ev_src contains the "seq-id"
    m_alarmTable.set(ev_src, ack_vec);
    m_eventTable.set(ev_src, ack_vec);
}


bool EventConsume::fetchRaiseInfo(vector<FieldValueTuple> &vec, string ev_src, string &ev_id, string &ev_sev, string &raise_act, 
                                  string &raise_ack_flag, string &raise_ts) {
    vector<FieldValueTuple> raise_vec;
    if (!m_alarmTable.get(ev_src, raise_vec)) {
        return false;
    }
    for (auto fv: raise_vec) {
        if (!fv.first.compare("type-id")) {
            ev_id = fv.second;
            vec.push_back(fv);
        }
        if (!fv.first.compare("severity")) {
            ev_sev = fv.second;
            vec.push_back(fv);
        }
        if (!fv.first.compare("action")) {
            raise_act = fv.second;
        }
        if (!fv.first.compare("acknowledged")) {
            raise_ack_flag = fv.second;
        }
        if (!fv.first.compare("time-created")) {
            raise_ts = fv.second;
        }
    }
    return true;
}


