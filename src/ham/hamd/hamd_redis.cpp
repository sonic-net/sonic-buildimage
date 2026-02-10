#ifndef _GNU_SOURCE
#   define _GNU_SOURCE
#endif

#include <sys/mman.h>                       /* memfd_create() */
#include <pwd.h>                            // fgetpwent()
#include <hiredis/hiredis.h>                // redisAsyncContext
#include <hiredis/async.h>                  // redisAsyncContext
#include <hiredis/adapters/glib.h>          // redis_source_new()
#include <glib.h>                           // GMainContext, gpointer
#include <system_error>                     // std::system_error

#include "../shared/utils.h"                // join()
#include "../shared/missing-memfd_create.h" // memfd_create() if missing from sys/mman.h
#include "hamd_redis.h"                     // prototypes
#include "hamd_config.h"                    // hamd_config_c
#include "file-utils.h"                     // get_file_contents()
#include "hamd_gecos.h"                     // gecos_c()

#include "swss/table.h"                     // swss::Table_async
#include "swss/dbconnector.h"               // swss::DBConnector_async

static inline std::string int_to_string(long long integer)
{
    return "int(" + std::to_string(integer) + ')';
}

static inline std::string string_to_string(const char * str_p)
{
    return "str(\"" + std::string(str_p) + "\")";
}

static inline std::string status_to_string(const char * str_p)
{
    return "status(" + std::string(str_p) + ')';
}

static inline std::string unknown_to_string(int type, const char * str_p)
{
    return "unknown(type=" + std::to_string(type) + ", \"" + std::string(str_p) + "\")";
}

/**
 * @brief Convert a reply (%redisReply) of type array to a printable string
 *
 * @param reply The reply to convert
 *
 * @return std::string The printable string
 */
static std::string array_to_string(redisReply * reply_p)
{
    std::vector<std::string>  tokens;
    std::string               token;

    for (size_t i = 0; i < reply_p->elements; i++)
    {
        switch (reply_p->element[i]->type)
        {
        case REDIS_REPLY_ERROR:   token = "err(" + std::string(reply_p->element[i]->str) + ')'; break;
        case REDIS_REPLY_INTEGER: token = int_to_string(reply_p->element[i]->integer); break;
        case REDIS_REPLY_STRING:  token = string_to_string(reply_p->element[i]->str); break;
        case REDIS_REPLY_STATUS:  token = status_to_string(reply_p->element[i]->str); break;
        case REDIS_REPLY_ARRAY:   token = array_to_string(reply_p->element[i]); break;
        default:                  token = unknown_to_string(reply_p->element[i]->type, reply_p->element[i]->str);
        }

        tokens.push_back(token);
    }

    if (tokens.empty())
        return "array[]";

    return "array[" + join(tokens.cbegin(), tokens.cend(), ", ") + ']';
}

static std::string reply_as_text(redisAsyncContext * ac_p, redisReply * reply_p)
{
    std::string text;

    switch (reply_p->type)
    {
    case REDIS_REPLY_ERROR:   text = "err(" + std::to_string(ac_p->err) + '-' + ac_p->errstr + ')'; break;
    case REDIS_REPLY_INTEGER: text = int_to_string(reply_p->integer); break;
    case REDIS_REPLY_STRING:  text = string_to_string(reply_p->str); break;
    case REDIS_REPLY_STATUS:  text = status_to_string(reply_p->str); break;
    case REDIS_REPLY_ARRAY:   text = array_to_string(reply_p); break;
    default:                  text = unknown_to_string(reply_p->type, reply_p->str);
    }

    return text;
}

//##############################################################################
redis_c::redis_c(const hamd_config_c  & hamd_config_r, GMainContext * g_main_ctx_p) :
    hamd_config_rm(hamd_config_r),
    g_main_ctx_pm(g_main_ctx_p),
    inloop_init_timer_m(0.01, redis_c::on_inloop_init_timeout_cb, this)
{
    inloop_init_timer_m.start();
}

bool redis_c::is_tron() const
{
    return hamd_config_rm.is_tron();
}

/**
 * @brief This is called when the inloop_init_timer_m expires.
 *
 * @param user_data_p Pointer to user data. In this case this point to the
 *                    hamd_c object.
 * @return bool
 */
bool redis_c::on_inloop_init_timeout_cb(gpointer user_data_p)
{
    static_cast<redis_c *>(user_data_p)->on_inloop_init_timeout();
    return false; // Return false for one-shot timer
}

/**
 * @brief This is called at the expiration of the inloop_init_timer_m.
 *        We use a timer to force the execution of this code as soon as
 *        the main event loop is started. It allows us to delay this
 *        initialization code until we're inside the event loop, as opposed
 *        to running it before the main loop in entered.
 */
void redis_c::on_inloop_init_timeout()
{
    DBConnector_async_glib  db(g_main_ctx_pm, is_tron());
    if (db.valid())
    {
        swss::Table_async  user_table(db.obj(), "USER");
        roles_db_upload(user_table);
    }
}

/**
 * @brief Set user's roles in the REDIS DB
 *
 * @param login_r User's login name
 * @param roles_r List of roles
 */
void redis_c::set_roles(const std::string  & login_r, const std::vector< std::string > & roles_r)
{
    DBConnector_async_glib  db(g_main_ctx_pm, is_tron());
    if (db.valid())
    {
        swss::Table_async  user_table(db.obj(), "USER");
        set_roles(user_table, login_r, roles_r);
    }
}

/**
 * @brief Remove user's roles from the REDIS DB
 *
 * @param login_r User's login name
 */
void redis_c::del_roles(const std::string  & login_r)
{
    DBConnector_async_glib  db(g_main_ctx_pm, is_tron());
    if (db.valid())
    {
        swss::Table_async  user_table(db.obj(), "USER");
        del_roles(user_table, login_r);
    }
}

/**
 * @brief This function retrieves the users and their roles from
 *        /etc/passwd and uploads them to the REDIS DB. This is
 *        done every time hamd restarts.
 */
void redis_c::roles_db_upload(swss::Table_async & user_table_r)
{
    LOG_CONDITIONAL(is_tron(), LOG_INFO, "redis_c::roles_db_upload()");

    // To limit the amount of time we spend accessing /etc/passwd,
    // we retrieve the whole contents of /etc/passwd in one shot.
    // Then we can parse the local copy.
    std::string   contents = get_file_contents("/etc/passwd");
    if (!contents.empty())
    {
        // It is easier to parse each passwd entry with fgetpwent(). However,
        // this API operates on a FILE *. So we create a temporary
        // in-memory file to which we copy the contents of the real
        // /etc/passwd file.
        int fd = memfd_create("passwd", MFD_CLOEXEC);
        if (fd == -1)
        {
            syslog(LOG_ERR, "redis_c::roles_db_upload() - Failed to create temp passwd file. errno=%d (%s)\n", errno, strerror(errno));
            return;
        }

        // Convert File Descriptor to a "FILE *". This is needed for fgetpwent()
        FILE  * passwd_fp = fdopen(fd, "w+");
        if (passwd_fp == nullptr)
        {
            syslog(LOG_ERR, "redis_c::roles_db_upload() - fdopen() failed. errno=%d (%s)\n", errno, strerror(errno));
            return;
        }

        fwrite(contents.c_str(), contents.length(), 1, passwd_fp);
        rewind(passwd_fp);

        struct passwd * ent;
        gecos_c         gecos;
        while (nullptr != (ent = fgetpwent(passwd_fp)))
        {
            if (nullptr != strstr(ent->pw_gecos, "roles=")) // Does the GECOS field contain roles?
            {
                gecos.init(ent->pw_gecos);
                set_roles(user_table_r, ent->pw_name, gecos.roles().members());
            }
        }

        fclose(passwd_fp); // This closes both passwd_fp and fd.
    }
}


/**
 * @brief Set user's roles in the REDIS DB
 *
 * @param login_r User's login name
 * @param roles_r List of roles
 */
void redis_c::set_roles(swss::Table_async & user_table_r, const std::string  & login_r, const std::vector< std::string > & roles_r)
{
    std::string  cs_roles = join(roles_r.cbegin(), roles_r.cend(), ","); // Comma-separated roles

    LOG_CONDITIONAL(is_tron(), LOG_INFO, "redis_c::set_roles() - login_r=\"%s\", roles_r=\"%s\"", login_r.c_str(), cs_roles.c_str());

    int rv = user_table_r.hset(redis_c::on_set_roles_cb, this, login_r, "roles@", cs_roles);
    if (rv != 0)
    {
        syslog(LOG_ERR, "redis_c::set_roles() - user_table_m.hset() returned %d\n", rv);
    }
}

void redis_c::on_set_roles_cb(redisAsyncContext * ac_p, void * reply_p, void * user_data_p)
{
    static_cast<redis_c *>(user_data_p)->on_set_roles(ac_p, static_cast<redisReply *>(reply_p));
}

void redis_c::on_set_roles(redisAsyncContext * ac_p, redisReply * reply_p)
{
    LOG_CONDITIONAL(is_tron(), LOG_INFO, "redis_c::on_set_roles() - %s", reply_as_text(ac_p, reply_p).c_str());
}

/**
 * @brief Remove user's roles from the REDIS DB
 *
 * @param login_r User's login name
 */
void redis_c::del_roles(swss::Table_async & user_table_r, const std::string  & login_r)
{
    LOG_CONDITIONAL(is_tron(), LOG_INFO, "redis_c::del_roles() - login_r=\"%s\"", login_r.c_str());

    int rv = user_table_r.hdel(redis_c::on_del_roles_cb, this, login_r, "roles@");
    if (rv != 0)
    {
        syslog(LOG_ERR, "redis_c::del_roles() - user_table_m.hdel() returned %d\n", rv);
    }
}

void redis_c::on_del_roles_cb(redisAsyncContext * ac_p, void * reply_p, void * user_data_p)
{
    static_cast<redis_c *>(user_data_p)->on_del_roles(ac_p, static_cast<redisReply *>(reply_p));
}

void redis_c::on_del_roles(redisAsyncContext * ac_p, redisReply * reply_p)
{
    LOG_CONDITIONAL(is_tron(), LOG_INFO, "redis_c::on_set_roles() - %s", reply_as_text(ac_p, reply_p).c_str());
}




DBConnector_async_glib::DBConnector_async_glib(GMainContext  * g_main_ctx_p, bool verbose)
{
    try
    {
        db_pm = new swss::DBConnector_async("APPL_DB", nullptr);
    }
    catch (std::system_error & ex)
    {
        syslog(LOG_ERR, "DBConnector_async_glib::DBConnector_async_glib() - %s", ex.what());
        db_pm = nullptr;
        return;
    }

    LOG_CONDITIONAL(verbose, LOG_INFO,
                    "DBConnector_async_glib::DBConnector_async_glib() - Connected to -> %s (%d) socket:%s",
                    db_pm->getDbName(), db_pm->getDbId(), db_pm->getSockAddr());

    g_source_attach(redis_source_new(db_pm->context()), g_main_ctx_p);
}

DBConnector_async_glib::~DBConnector_async_glib()
{
    if (db_pm != nullptr)
    {
        delete db_pm;
        db_pm = nullptr;
    }
}
