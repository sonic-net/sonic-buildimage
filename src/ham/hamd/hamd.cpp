// Host Account Management
#include <unistd.h>             // getpid()
#include <string>               // std::string
#include <sstream>              // std::ostringstream

#include "hamd.h"               // hamd_c
#include "hamd_redis.h"         // redis_c
#include "../shared/utils.h"    // LOG_CONDITIONAL()


/**
 * @brief DBus adaptor class constructor
 *
 * @param config_r Structure containing configuration parameters
 * @param conn_r
 */
hamd_c::hamd_c(hamd_config_c & config_r, DBus::Connection & conn_r, redis_c & redis_r) :
    DBus::ObjectAdaptor(conn_r, DBUS_OBJ_PATH_BASE),
    config_rm(config_r),
    poll_timer_m((double)config_rm.poll_period_sec_m, hamd_c::on_poll_timeout_cb, this),
    redis_rm(redis_r)
{
    apply_config();
}

/**
 * @brief This is called when the poll_timer_m expires.
 *
 * @param user_data_p Pointer to user data. In this case this point to the
 *                    hamd_c object.
 * @return bool
 */
bool hamd_c::on_poll_timeout_cb(gpointer user_data_p)
{
    static_cast<hamd_c *>(user_data_p)->on_poll_timeout();
    return true; // Return true to repeat timer
}

void hamd_c::on_poll_timeout()
{
    rm_unconfirmed_users();
}

/**
 * @brief reload configuration and apply to running daemon.
 */
void hamd_c::reload()
{
    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::reload()");
    config_rm.reload();
    apply_config();
}

/**
 * @brief Apply the configuration to the running daemon
 */
void hamd_c::apply_config()
{
    if (config_rm.poll_period_sec_m > 0)
        poll_timer_m.start((double)config_rm.poll_period_sec_m);
    else
        poll_timer_m.stop();
}

/**
 * @brief This is called just before the destructor is called and is used
 *        to clean up all resources in use by the class instance.
 */
void hamd_c::cleanup()
{
    poll_timer_m.stop();
}


/**
 * @brief This is a DBus interface used to turn tracing on. This allows
 *        the daemon to run with verbosity turned on.
 *
 * @return std::string
 */
std::string hamd_c::tron()
{
    config_rm.tron_m = true;
    return "Tracing is now ON";
}

/**
 * @brief This is a DBus interface used to turn tracing off. This allows
 *        the daemon to run with verbosity turned off.
 *
 * @return std::string
 */
std::string hamd_c::troff()
{
    config_rm.tron_m = false;
    return "Tracing is now OFF";
}

/**
 * @brief This is a DBus interface used to retrieve daemon running info
 *
 * @return std::string
 */
std::string hamd_c::show()
{
    std::ostringstream  oss;
    oss << "Process data:\n"
        << "  PID                       = " << getpid() << '\n'
        << "  poll_timer_m              = " << poll_timer_m << '\n'
        << '\n'
        << config_rm << '\n';

    return oss.str();
}

