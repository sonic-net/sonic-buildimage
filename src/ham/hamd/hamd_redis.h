#ifndef __REDIS_H__
#define __REDIS_H__

#include <glib.h>               // GMainContext

#include <string>
#include <vector>

#include "timer.h"              // gtimer_c

// Forward declarations
namespace swss
{
class Table_async;
class DBConnector_async;
}

class hamd_config_c;
struct redisReply;
typedef struct redisReply redisReply;
typedef struct redisAsyncContext redisAsyncContext;

//##############################################################################
class redis_c
{
public:
    redis_c(const hamd_config_c  & hamd_config_r, GMainContext  * main_ctx_p);

    void cleanup() {}
    bool is_tron() const;

    void set_roles(const std::string  & login_r, const std::vector< std::string > & roles_r);
    void del_roles(const std::string  & login_r);

private:
    const hamd_config_c  & hamd_config_rm;
    GMainContext         * g_main_ctx_pm = nullptr; // GLib main loop context

    gtimer_c               inloop_init_timer_m;
    static bool            on_inloop_init_timeout_cb(gpointer user_data_p); // This callback functions must follow GSourceFunc signature.
    void                   on_inloop_init_timeout();

    void roles_db_upload(swss::Table_async & user_table_r);

    void set_roles(swss::Table_async & user_table_r, const std::string  & login_r, const std::vector< std::string > & roles_r);
    void del_roles(swss::Table_async & user_table_r, const std::string  & login_r);

    static void on_set_roles_cb(redisAsyncContext * ac_p, void * reply_p, void * user_data);
    void        on_set_roles(redisAsyncContext * ac_p, redisReply * reply_p);

    static void on_del_roles_cb(redisAsyncContext * ac_p, void * reply_p, void * user_data);
    void        on_del_roles(redisAsyncContext * ac_p, redisReply * reply_p);
};


class DBConnector_async_glib
{
public:
    DBConnector_async_glib(GMainContext  * g_main_ctx_p, bool verbose=false);
    ~DBConnector_async_glib();

    bool                      valid() const { return db_pm != nullptr; }
    swss::DBConnector_async & obj()   const { return *db_pm; }

private:
    swss::DBConnector_async * db_pm = nullptr;
};

#endif // __REDIS_H__
