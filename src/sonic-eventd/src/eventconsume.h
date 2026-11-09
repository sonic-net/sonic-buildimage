#ifndef __EVENTCONSUME_H__
#define __EVENTCONSUME_H__

#include <string>
#include <map>
#include <swss/events.h>
#include <swss/dbconnector.h>
#include <swss/subscriberstatetable.h>
#include "eventutils.h"

extern std::atomic<bool> reload_config_flag;
extern volatile bool g_run;

class EventConsume
{
public:
    EventConsume(swss::DBConnector *dbConn,
                 std::string evProfile =EVENTD_DEFAULT_MAP_FILE,
                 std::string dbProfile =EVENTD_CONF_FILE);
    ~EventConsume();
    void read_eventd_config(bool read_all=true);
    void run();

private:
    swss::Table m_eventTable;
    swss::Table m_alarmTable;
    swss::Table m_eventStatsTable;
    swss::Table m_alarmStatsTable;
    u_int32_t m_days, m_count;
    std::string m_evProfile;
    std::string m_dbProfile;

    void handle_notification(const event_receive_op_t& evt);
    void read_events();
    void updateAlarmStatistics(std::string ev_sev, std::string ev_act);
    void updateEventStatistics(bool is_add, bool is_alarm, bool is_ack, bool is_clear);
    void read_config_and_purge();
    void update_events(std::string seq_id, std::string ts, std::vector<swss::FieldValueTuple> vec);
    void purge_events();
    void modifyEventStats(std::string seq_id);
    void clearAckAlarmStatistic();
    void resetAlarmStats(int, int, int, int, int, int);
    void fetchFieldValues(const event_receive_op_t& evt , std::vector<swss::FieldValueTuple> &, std::string &, std::string &, std::string &, std::string &, std::string &);
    bool isFloodedEvent(std::string, std::string, std::string, std::string);
    bool staticInfoExists(std::string &, std::string &, std::string &, std::string &, std::vector<swss::FieldValueTuple> &);
    bool udpateLocalCacheAndAlarmTable(std::string, bool &);
    void initStats();
    void updateAckInfo(bool, std::string, std::string, std::string, std::string);
    bool fetchRaiseInfo(std::vector<swss::FieldValueTuple> &, std::string, std::string &, std::string &, std::string &, std::string &, std::string &);
};


#endif /* __EVENTCONSUME_H__ */

