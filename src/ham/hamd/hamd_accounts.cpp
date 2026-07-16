#include <pwd.h>                // fgetpwent()
#include <algorithm>            // std::is_permutation()

#include "hamd.h"               // hamd_c
#include "hamd_gecos.h"         // gecos_c()
#include "subprocess.h"         // run()
#include "file-utils.h"         // sorted_filelist(), read_key_csv()
#include "hamd_redis.h"         // redis_c
#include "passwd_utils.h"       // fgetpwnam()
#include "../shared/utils.h"    // streq(), LOG_CONDITIONAL()

/**
 * @brief Create a new user
 *
 * @param login     User's login name
 * @param roles     List of roles
 * @param hashed_pw Hashed password. Must follow useradd's --password
 *                  syntax.
 *
 * @return Empty string on success, Error message otherwise.
 */
std::string  hamd_c::create_user(const std::string                & login,
                                 const std::vector< std::string > & roles,
                                 const std::string                & hashed_pw)
{
    gecos_c  gecos;
    gecos.set(roles_as_string(roles).c_str(), "", "local");

    std::string cmd = "/usr/sbin/useradd"
                      " --create-home"
                      " --user-group"
                      " --password '" + hashed_pw + "'"
                      " --comment \"" + gecos.text() + '"';

    if (!config_rm.shell().empty())
        cmd += " --shell " + config_rm.shell();

    std::string groups = get_groups_as_string(roles);
    if (!groups.empty())
        cmd += " --groups \"" + groups + '"';

    cmd += ' ' + login;

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::create_user() - Create user \"%s\" [%s]", login.c_str(), cmd.c_str());

    int          rc;
    std::string  std_err;
    std::string  std_out;
    std::tie(rc, std_out, std_err) = run(cmd);

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::create_user() - Create user \"%s\" rc=%d, stdout=%s, stderr=%s",
                    login.c_str(), rc, std_out.c_str(), std_err.c_str());

    if (rc != 0)
    {
        if (std_err.empty())
            return "/usr/sbin/useradd failed with rc=" + std::to_string(rc);

        return "/usr/sbin/useradd failed with " + std_err;
    }

    std::string errmsg = post_create_scripts(login);
    if (!errmsg.empty()) // The errmsg will be empty on success
    {
        // Since we failed to run the port-create
        // scripts, we now need to delete the user.
        delete_user(login);
    }
    else
    {
        // Save roles to REDIS DB (a SONiC requirement)
        redis_rm.set_roles(login, roles);
    }

    return errmsg;
}

/**
 * @brief Delete a user
 *
 * @param login     User's login name
 *
 * @return Empty string on success, Error message otherwise.
 */
std::string hamd_c::delete_user(const std::string & login)
{
    if (::getpwnam(login.c_str()) == nullptr)
    {
        // User doesn't exist...so success!
        return "";
    }

    std::string  pre_delete_msg = pre_delete_scripts(login);

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::delete_user() - pre_delete_scripts() %s",
                    pre_delete_msg.empty() ? "success" : pre_delete_msg.c_str());

    std::string  cmd = "/usr/sbin/userdel --force --remove " + login;

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::delete_user() - executing command \"%s\"", cmd.c_str());

    int          rc;
    std::string  std_err;
    std::string  std_out;
    std::tie(rc, std_out, std_err) = run(cmd);

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::delete_user() - command returned rc=%d, stdout=%s, stderr=%s",
                    rc, std_out.c_str(), std_err.c_str());

    if (rc == 0)
        return "";

    if (std_err.empty())
        return "/usr/sbin/userdel failed with rc=" + std::to_string(rc);

    return "/usr/sbin/userdel failed with " + std_err;
}

/**
 * @brief Create or Modify a user account
 *
 * @param login     User's login name
 * @param roles     List of roles
 * @param hashed_pw Hashed password. Must follow useradd's --password
 *                  syntax.
 */
::DBus::Struct< bool, std::string > hamd_c::useradd(const std::string                & login,
                                                    const std::vector< std::string > & roles,
                                                    const std::string                & hashed_pw)
{
    if (fgetpwnam(login.c_str()) == nullptr)           // User does not exist
    {                                                  // Let's create it.
        ::DBus::Struct< bool, std::string > ret;
        ret._2 = create_user(login, roles, hashed_pw);
        ret._1 = ret._2.empty();
        return ret;
    }

    // User exists, so update password and roles
    ::DBus::Struct< bool,       /* success */
                    std::string /* errmsg  */ > ret;
    ret = passwd(login, hashed_pw);
    return ret._1/*success*/ ? set_roles(login, roles) : ret;
}

/**
 * @brief Delete a user account
 */
::DBus::Struct< bool, std::string > hamd_c::userdel(const std::string& login)
{
    ::DBus::Struct< bool,       /* success */
                    std::string /* errmsg  */ > ret;

    redis_rm.del_roles(login);

    ret._2 = delete_user(login);
    ret._1 = ret._2.empty();

    return ret;
}

/**
 * @brief Change user password
 */
::DBus::Struct< bool, std::string > hamd_c::passwd(const std::string& login, const std::string& hashed_pw)
{
    std::string  cmd = "/usr/sbin/usermod --password '" + hashed_pw + "' " + login;

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::passwd() - executing command \"%s\"", cmd.c_str());

    int          rc;
    std::string  std_err;
    std::string  std_out;
    std::tie(rc, std_out, std_err) = run(cmd);

    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::passwd() - command returned rc=%d, stdout=%s, stderr=%s",
                    rc, std_out.c_str(), std_err.c_str());

    ::DBus::Struct< bool,       /* success */
                    std::string /* errmsg  */ > ret;

    ret._1 = rc == 0;
    ret._2 = rc == 0 ? "" : std_err;

    return ret;
}

/**
 * @brief Set user's roles (supplementary groups)
 */
::DBus::Struct< bool, std::string > hamd_c::set_roles(const std::string& login, const std::vector< std::string >& roles)
{
    LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::set_roles() - login=\"%s\", roles=[ \"%s\" ]",
                    login.c_str(), join(roles.cbegin(), roles.cend(), "\", \"").c_str());

    ::DBus::Struct< bool,       /* success */
                    std::string /* errmsg  */ > ret;

    // First let's get the current roles configured in /etc/passwd
    struct passwd * pwd = fgetpwnam(login.c_str());
    if ((NULL == pwd) || (NULL == pwd->pw_name) || (login != pwd->pw_name))
    {
        // User does not exist
        ret._2 = strfmt("No such user in /etc/passwd: %s", login.c_str());
        ret._1 = false;
    }
    else
    {
        std::string cmd;

        // Extract the current roles from the GECOS field.
        gecos_c gecos(pwd->pw_gecos);
        std::vector<std::string> cur_roles = gecos.roles().members();

        // Now let's check if the roles have changed.
        if (std::is_permutation(roles.begin(), roles.end(), cur_roles.begin(), cur_roles.end()))
        {
            // The roles haven't changed compared to /etc/passwd
            // We still need to run usermod for the --groups option (below),
            // but we don't need to change the GECOS field.
            cmd = "/usr/sbin/usermod";
        }
        else
        {
            gecos.set(roles_as_string(roles).c_str());
            cmd = "/usr/sbin/usermod --comment \"" + gecos.text() + '"';
        }

        // No need to check if the groups have changed.
        // usermod does that for us.
        std::string groups = get_groups_as_string(roles);
        cmd += " --groups \"" + groups + "\" " + login;

        LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::set_roles() - executing command \"%s\"", cmd.c_str());

        int          rc;
        std::string  std_err;
        std::string  std_out;
        std::tie(rc, std_out, std_err) = run(cmd);

        LOG_CONDITIONAL(is_tron(), LOG_DEBUG, "hamd_c::set_roles() - command returned rc=%d, stdout=%s, stderr=%s",
                        rc, std_out.c_str(), std_err.c_str());

        if (rc == 0)
        {
            redis_rm.set_roles(login, roles);
            ret._2 = "";
            ret._1 = true;
        }
        else
        {
            ret._1 = false;
            ret._2 = std_err;
        }
    }

    return ret;
}

/**
 * @brief Create a group
 */
::DBus::Struct< bool, std::string > hamd_c::groupadd(const std::string& group)
{
    ::DBus::Struct< bool, std::string > ret;
    ret._1 = false;
    ret._2 = "Not implemented";
    return ret;
}

/**
 * @brief Delete a group
 */
::DBus::Struct< bool, std::string > hamd_c::groupdel(const std::string& group)
{
    ::DBus::Struct< bool, std::string > ret;
    ret._1 = false;
    ret._2 = "Not implemented";
    return ret;
}

/**
 * @brief Return the Linux groups associated with the @roles specified.
 *
 * @param roles  List of roles
 *
 * @return A std::set of Linux groups.
 */
//#define MAP_ROLES_TO_GROUPS_AS_WELL
std::set<std::string> hamd_c::get_groups(const std::vector< std::string > & roles)
{
    std::set<std::string>  groups; // Use std::set to eliminate duplicates
    std::map<std::string/*role or priv-lvl*/,
             std::vector<std::string>/*groups*/>  group_mapping = read_key_csv("/etc/sonic/hamd/group-mapping");

    for (auto & role : roles)
    {
        std::copy(group_mapping[role].cbegin(), group_mapping[role].cend(), std::inserter(groups, groups.end()));
#ifdef MAP_ROLES_TO_GROUPS_AS_WELL
        // Currently the roles are saved as Linux groups in /etc/group.
        // In the next iteration the roles will be saved to the REDIS DB
        // and this won't be needed.
        groups.insert(role);
#endif
    }

    return groups;
}

/**
 * @brief Return the Linux groups associated with the provided @roles.
 *
 * @param roles  List of roles
 *
 * @return A string of comma-separated Linux groups (e.g. "sudo,docker").
 */
std::string hamd_c::get_groups_as_string(const std::vector< std::string > & roles)
{
    std::set<std::string>  groups = get_groups(roles);
    return join(groups.cbegin(), groups.cend(), ",");
}

/**
 * @brief Run the post-create scripts located in
 *        /etc/sonic/hamd/scripts/post-create/.
 *
 * @param login User's login name.
 *
 * @return Empty string on success, error message otherwise.
 */
std::string hamd_c::post_create_scripts(const std::string  & login) const
{
    int          rc;
    std::string  cmd;
    std::string  std_err;
    std::string  std_out;

    for (auto & file : sorted_filelist("/etc/sonic/hamd/scripts/post-create"))
    {
        if (file != "/etc/sonic/hamd/scripts/post-create/README")
        {
            if (g_file_test(file.c_str(), G_FILE_TEST_IS_EXECUTABLE))
            {
                cmd = file + ' ' + login;
                std::tie(rc, std_out, std_err) = run(cmd);

                if (rc != 0)
                    return "Failed to execute " + cmd + ". " + std_err;
            }
            else
            {
                syslog(LOG_WARNING, "\"%s\" is not executable", file.c_str());
            }
        }
    }

    return "";
}

/**
 * @brief Run the post-create scripts located in
 *        /etc/sonic/hamd/scripts/pre-delete/.
 *
 * @param login User's login name.
 *
 * @return Empty string on success, error message otherwise.
 */
std::string hamd_c::pre_delete_scripts(const std::string  & login) const
{
    int                      rc;
    std::string              cmd;
    std::string              std_err;
    std::string              std_out;
    std::vector<std::string> errmsgs;

    for (auto & file : sorted_filelist("/etc/sonic/hamd/scripts/pre-delete"))
    {
        if (file != "/etc/sonic/hamd/scripts/pre-delete/README")
        {
            if (g_file_test(file.c_str(), G_FILE_TEST_IS_EXECUTABLE))
            {
                cmd = file + ' ' + login;
                std::tie(rc, std_out, std_err) = run(cmd);

                if (rc != 0)
                    errmsgs.push_back("Failed to execute " + cmd + ". " + std_err);
            }
            else
            {
                syslog(LOG_WARNING, "\"%s\" is not executable", file.c_str());
            }
        }

    }

    return join(errmsgs.cbegin(), errmsgs.cend(), ";");
}
