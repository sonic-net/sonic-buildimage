#ifndef __EVENTCONSUME_H__
#define __EVENTCONSUME_H__

#include <string>
#include <map>
#include "events.h"
#include "eventutils.h"
#include "dbconnector.h"
#include "subscriberstatetable.h"

using namespace swss;
using namespace std;

extern std::atomic<bool> reload_config_flag;

class EventConsume
{
public:
    EventConsume(DBConnector *dbConn,
                 string evProfile =EVENTD_DEFAULT_MAP_FILE,
                 string dbProfile =EVENTD_CONF_FILE);
    ~EventConsume();
    void read_eventd_config(bool read_all=true);
    void run();

private:
    Table m_eventTable;
    Table m_alarmTable;
    Table m_eventStatsTable;
    Table m_alarmStatsTable;
    u_int32_t m_days, m_count;
    string m_evProfile;
    string m_dbProfile;

    void handle_notification(const event_receive_op_t& evt);
    void read_events();
    void updateAlarmStatistics(string ev_sev, string ev_act);
    void updateEventStatistics(bool is_add, bool is_alarm, bool is_ack, bool is_clear);
    void read_config_and_purge();
    void update_events(string seq_id, string ts, vector<FieldValueTuple> vec);
    void purge_events();
    void modifyEventStats(string seq_id);
    void clearAckAlarmStatistic();
    void resetAlarmStats(int, int, int, int, int, int);
    void fetchFieldValues(const event_receive_op_t& evt , vector<FieldValueTuple> &, string &, string &, string &, string &, string &);
    bool isFloodedEvent(string, string, string, string);
    bool staticInfoExists(string &, string &, string &, string &, vector<FieldValueTuple> &);
    bool udpateLocalCacheAndAlarmTable(string, bool &);
    void initStats();
    void updateAckInfo(bool, string, string, string, string);
    bool fetchRaiseInfo(vector<FieldValueTuple> &, string, string &, string &, string &, string &, string &);
};


#endif /* __EVENTCONSUME_H__ */

