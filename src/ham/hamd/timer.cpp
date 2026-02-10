// Host Account Management
#include <glib.h>
#include <math.h>       // rintf()
#include <ostream>      // std::ostream
#include <syslog.h>     // syslog()

#include "timer.h"

/**
 * @brief See timer.h for description.
 */
gtimer_c::gtimer_c(double           interval_sec,
                   bool          (* user_cback_p)(void * user_data_p),
                   void           * user_data_p,
                   int              priority):
    source_pm(NULL),
    tid_m(0),
    interval_sec_m(-1.0),
    user_cback_pm(user_cback_p),
    user_data_pm(user_data_p),
    priority_m(priority),
    g_timeout_source_new_funct_pm(g_timeout_source_new_seconds)
{
    //syslog(LOG_INFO, "gtimer_c::gtimer_c() - interval_sec=%f s", interval_sec);
    set_timeout(interval_sec);
}

/**
 * @brief See timer.h for description.
 */
gtimer_c::~gtimer_c()
{
    stop();
}

/**
 * @brief See timer.h for description.
 */
gboolean gtimer_c::callback(gpointer user_data_p)
{
    gtimer_c * p = reinterpret_cast<gtimer_c *>(user_data_p);
    bool       r = p->user_cback_pm(p->user_data_pm);
    gboolean   ret;

    if (r)
    {
        ret = TRUE;
    }
    else
    {
        p->source_pm = NULL;
        p->tid_m     = 0;
        ret = FALSE;
    }

    return ret;
}

/**
 * @brief See timer.h for description.
 */
void gtimer_c::stop()
{
    //syslog(LOG_INFO, "gtimer_c::stop() - ENTER tid_m=%d", tid_m);
    if (0 != tid_m)
    {
        g_source_remove(tid_m);
        source_pm = NULL;
        tid_m     = 0;
    }
    //syslog(LOG_INFO, "gtimer_c::stop() - EXIT");
}

/**
 * @brief See timer.h for description.
 */
void gtimer_c::start(double new_interval_sec, void * user_data_p)
{
    //syslog(LOG_INFO, "gtimer_c::start() - ENTER tid_m=%d", tid_m);

    if (new_interval_sec > 0)
    {
        set_timeout(new_interval_sec);
    }

    if (NULL != user_data_p)
    {
        user_data_pm = user_data_p;
    }

    if (active())
    {
        //syslog(LOG_INFO, "gtimer_c::start() - Restarting interval_sec_m=%f s");
        g_source_set_ready_time(source_pm, g_source_get_time(source_pm) + (gint64)(interval_sec_m * 1000000));
    }
    else
    {
        //syslog(LOG_INFO, "gtimer_c::start() - Starting new");
        source_pm = g_timeout_source_new_funct_pm(interval_m);

        if (G_PRIORITY_DEFAULT != priority_m)
          g_source_set_priority(source_pm, priority_m);

        g_source_set_callback(source_pm, gtimer_c::callback, this, NULL);
        tid_m = g_source_attach(source_pm, NULL);
        g_source_unref(source_pm);
    }

    //syslog(LOG_INFO, "gtimer_c::start() - EXIT tid_m=%d", tid_m);
}

/**
 * @brief See timer.h for description.
 */
void gtimer_c::clear()
{
    if (active())
    {
        g_source_set_ready_time(source_pm, 0);
    }
}

/**
 * @brief See timer.h for description.
 */
void gtimer_c::set_timeout(double new_interval_sec)
{
    //syslog(LOG_INFO, "gtimer_c::set_timeout() - ENTER new_interval_sec=%f", new_interval_sec);
    if ((new_interval_sec >= 0) && (interval_sec_m != new_interval_sec))
    {
        interval_sec_m = new_interval_sec;
        if (rintf(new_interval_sec) == new_interval_sec)
        {
            //syslog(LOG_INFO, "gtimer_c::set_timeout() - use sec");
            g_timeout_source_new_funct_pm = g_timeout_source_new_seconds;
            interval_m = (guint)new_interval_sec;
        }
        else
        {
            //syslog(LOG_INFO, "gtimer_c::set_timeout() - use msec");
            g_timeout_source_new_funct_pm = g_timeout_source_new;
            interval_m = (guint)((new_interval_sec * 1000) + 0.5);
        }
    }
    //syslog(LOG_INFO, "gtimer_c::set_timeout() - EXIT - interval_m=%d", interval_m);
}

/**
 * @brief See timer.h for description.
 */
void  gtimer_c::set_user_data(void * user_data_p)
{
    if (NULL != user_data_p)
    {
        user_data_pm = user_data_p;
    }
}

/**
 * @brief See timer.h for description.
 */
double gtimer_c::time_remaining() const
{
    if (active())
    {
        gint64 delta_us = g_source_get_ready_time(source_pm) - g_source_get_time(source_pm);
        if (delta_us > 0)
        {
            return (double)delta_us / 1000000.0;
        }
    }

    return 0;
}

/**
 * @brief See timer.h for description.
 */
std::ostream& operator<<(std::ostream  & stream_r, const gtimer_c  & timer_r)
{
    if (timer_r.active())
    {
        stream_r << timer_r.time_remaining() << "s";
    }
    else
    {
        stream_r << "off";
    }
    return stream_r;
}
