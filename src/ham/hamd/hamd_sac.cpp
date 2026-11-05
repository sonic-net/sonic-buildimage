#include "hamd.h"               // hamd_c
#include "hamd_gecos.h"         // gecos_c()
#include "siphash24.h"          // siphash24()
#include "subprocess.h"         // run()
#include "hamd_redis.h"         // redis_c
#include "passwd_utils.h"       // fgetpwnam()
#include "../shared/utils.h"    // LOG_CONDITIONAL(), strfmt(), join()

#include <sys/types.h>          // getpwnam()
#include <pwd.h>                // getpwnam(), getpwuid()
#include <grp.h>                // getgrnam(), getgrouplist()
#include <syslog.h>             // syslog()
#include <set>                  // std::set
#include <glib/gstdio.h>        // g_chdir()
#include <algorithm>            // std::is_permutation()

#define UNCONFIRMED_USER_MAGIC_STRING       "Unconfirmed SAC user"
#define UNCONFIRMED_USER_MAGIC_STRING_LEN   (sizeof(UNCONFIRMED_USER_MAGIC_STRING)-1)


/**
 * @brief This is a DBus interface used by remote programs to add an
 *        unconfirmed user.
 *
 * @param login User's login name
 * @param pid   PID of the caller.
 *
 * @return Empty string on success, Error message otherwise.
 */
std::string hamd_c::add_unconfirmed_user(const std::string & login,
                                         const uint32_t    & pid)
{
    // First, let's check if there are any
    // unconfirmed users that could be removed.
    rm_unconfirmed_users();

    // Second check if user already exists
    if (NULL != fgetpwnam(login.c_str()))
    {
        LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::add_unconfirmed_user: User \"%s\" already exist", login.c_str());
        return "";
    }

    // Next, add <login> as an unconfirmed user.

    unsigned     attempt_cnt;
    uid_t        candidate;
    std::string  name(login);
    std::string  full_cmd;
    std::string  base_cmd = "/usr/sbin/useradd"
                            " --create-home"
                            " --user-group"
                            " --comment \"" UNCONFIRMED_USER_MAGIC_STRING " " + std::to_string(pid) + '"';

    if (!config_rm.shell().empty())
        base_cmd += " --shell " + config_rm.shell();

    for (attempt_cnt = 0; attempt_cnt < 100; attempt_cnt++) /* Give up retrying eventually */
    {
        // Find a unique UID in the range sac_uid_min_m..sac_uid_max_m.
        // We use a hash function to always get the same ID for a given user
        // name. Hash collisions (i.e. two user names with the same hash) will
        // be handled by trying with a slightly different username.
        candidate = config_rm.uid_fit_into_range(siphash24(name.c_str(), name.length()));

        LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "User \"%s\": attempt %d using name \"%s\", candidate UID=%lu",
                        login.c_str(), attempt_cnt, name.c_str(), (unsigned long)candidate);

        // Note: The range 60000-64999 is reserved on Debian platforms
        //       and should be avoided and the value 65535 is traditionally
        //       reserved as an "error" code as well as 65534 is reserved
        //       as the "nobody" user.
        if (!((candidate >= 60000) && (candidate <= 64999)) &&
             (candidate != 65535) &&
             (candidate != 65534) &&
             !::getpwuid(candidate)) /* make sure not already allocated */
        {
            full_cmd = base_cmd + " --uid " + std::to_string(candidate) + ' ' + login;

            LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "User \"%s\": executing \"%s\"", login.c_str(), full_cmd.c_str());

            int          rc;
            std::string  std_err;
            std::string  std_out;
            std::tie(rc, std_out, std_err) = run(full_cmd);

            LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "SAC - User \"%s\": useradd returned rc=%d, stdout=%s, stderr=%s",
                            login.c_str(), rc, std_out.c_str(), std_err.c_str());

            return rc == 0 ? "" : strfmt("SAC - User \"%s\": useradd failed rc=%d, stdout=%s, stderr=%s",
                                         login.c_str(), rc, std_out.c_str(), std_err.c_str());
        }
        else
        {
            // Try with a slightly different name
            name = login + std::to_string(attempt_cnt);
            LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "SAC - User \"%s\": candidate UID=%lu already in use. Retry with name = \"%s\"",
                            login.c_str(), (unsigned long)candidate, name.c_str());
        }
    }

    std::string errmsg = strfmt("SAC - User \"%s\": Unable to create user after %d attempts",
                                login.c_str(), attempt_cnt);

    syslog(LOG_ERR, "%s", errmsg.c_str());

    return errmsg;
}

/**
 * @brief This is a DBus interface used by remote programs to confirm a
 *        user.
 *
 * @param login        User's login name
 * @param roles        List of roles
 * @param auth_method  One of "tacacs+" or "radius"
 *
 * @return Empty string on success, Error message otherwise.
 */
std::string hamd_c::user_confirm(const std::string              & login,
                                 const std::vector<std::string> & roles,
                                 const std::string              & auth_method)
{
    std::string errmsg;
    std::string dbgstr;
    if (is_tron())
    {
        dbgstr = strfmt("SAC - hamd_c::user_confirm(login=\"%s\", roles=\"[%s]\", auth_method=\"%s\")",
                         login.c_str(), join(roles.cbegin(), roles.cend(), ", ").c_str(), auth_method.c_str());
        syslog(LOG_DEBUG, "%s", dbgstr.c_str());
    }

    // Check whether user already exists in /etc/passwd
    struct passwd * pwd = fgetpwnam(login.c_str());
    if ((NULL == pwd) || (NULL == pwd->pw_name) || (login != pwd->pw_name))
    {
        errmsg = strfmt("No such user in /etc/passwd: %s", login.c_str());
        LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "%s: %s", dbgstr.c_str(), errmsg.c_str());
        return errmsg;
    }

    // Check whether it is an unconfirmed user
    bool unconfirmed_user = strneq(pwd->pw_gecos, UNCONFIRMED_USER_MAGIC_STRING, UNCONFIRMED_USER_MAGIC_STRING_LEN);
    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "%s: unconfirmed_user=%s", dbgstr.c_str(), true_false(unconfirmed_user));


    gecos_c  gecos(pwd->pw_gecos);
    bool must_update_roles = true;
    if (!unconfirmed_user)
    {
        // Check whether the roles need to be changed by extracting
        // the current roles from the GECOS.
        std::vector<std::string> cur_roles = gecos.roles().members();

        if (is_tron())
        {
            syslog(LOG_DEBUG, "%s: cur roles=[%s]", dbgstr.c_str(), join(cur_roles.cbegin(), cur_roles.cend(), ", ").c_str());
            syslog(LOG_DEBUG, "%s: new roles=[%s]", dbgstr.c_str(), join(roles.cbegin(), roles.cend(), ", ").c_str());
        }

        must_update_roles = !std::is_permutation(roles.begin(), roles.end(), cur_roles.begin(), cur_roles.end());
    }

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "%s: must_update_roles=%s",
                    dbgstr.c_str(), true_false(must_update_roles));

    if (!must_update_roles)
        return ""; // We're all good. Nothing has changed.

    gecos.set(roles_as_string(roles).c_str(), nullptr, auth_method.c_str());
    std::string cmd = "/usr/sbin/usermod"
                      " --groups \"" + get_groups_as_string(roles) + "\""
                      " --comment \"" + gecos.text() + "\" " + login;

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "%s: executing \"%s\"", dbgstr.c_str(), cmd.c_str());

    int          rc;
    std::string  std_err;
    std::string  std_out;
    std::tie(rc, std_out, std_err) = run(cmd);

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "%s: usermod returned rc=%d, stdout=%s, stderr=%s",
                    dbgstr.c_str(), rc, std_out.c_str(), std_err.c_str());

    if (rc != 0)
    {
        return strfmt("/usr/sbin/usermod returned rc=%d, stdout=%s, stderr=%s",
                      rc, std_out.c_str(), std_err.c_str());
    }

    if (unconfirmed_user)
    {
        // Only run port-create scripts for newly authenticated users.
        errmsg = post_create_scripts(login);
        if (!errmsg.empty()) // The errmsg will be empty on success
        {
            // Since we failed to run the post-create
            // scripts, we now need to delete the user.
            delete_user(login);
        }
    }

    if (!errmsg.empty()) // The errmsg will be empty on success
    {
        redis_rm.set_roles(login, roles);
    }

    return errmsg;
}

/**
 * @brief Remove unconfirmed users from /etc/passwd. Unconfirmed users have
 *        the string "Unconfirmed sac user [PID]" in their GECOS string and
 *        the PID does not exist anymore.
 */
void hamd_c::rm_unconfirmed_users() const
{
    FILE  * f = fopen("/etc/passwd", "re");
    if (f)
    {
        struct passwd * ent;
        std::string     base_cmd("/usr/sbin/userdel --remove ");
        std::string     full_cmd;
        g_chdir("/proc");
        while (NULL != (ent = fgetpwent(f)))
        {
            const char * pid_p = startswith(ent->pw_gecos, UNCONFIRMED_USER_MAGIC_STRING);
            if (NULL != pid_p)
            {
                if (!g_file_test(pid_p, G_FILE_TEST_EXISTS))
                {
                    // Directory does not exist, which means process does not
                    // exist either. Let's remove this user which was never
                    // confirmed by PAM authentification.
                    full_cmd = base_cmd + ent->pw_name;

                    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::rm_unconfirmed_users() - executing command \"%s\"", full_cmd.c_str());

                    int          rc;
                    std::string  std_err;
                    std::string  std_out;
                    std::tie(rc, std_out, std_err) = run(full_cmd);

                    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::rm_unconfirmed_users() - command returned rc=%d, stdout=%s, stderr=%s",
                                    rc, std_out.c_str(), std_err.c_str());

                    if (rc != 0)
                    {
                        syslog(LOG_ERR, "User \"%s\": Failed to removed unconfirmed user UID=%d. %s",
                               ent->pw_name, ent->pw_uid, std_err.c_str());
                    }
                }
            }
        }
        fclose(f);
    }
}
