// Host Account Management
#ifndef __HAM_LIB_H
#define __HAM_LIB_H

#include <stdbool.h>    /* bool */
#include <grp.h>        /* gid_t, struct group */
#include <pwd.h>        /* uid_t, struct passwd */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Add user to the host's /etc/passwd, /etc/group, /etc/shadow DBs.
 *
 * @param login     Login name
 * @param role      List of comma-separated roles. E.g.
 *                  "secadmin,netoperator".
 * @param hashed_pw Hashed password. Must follow useradd's --password
 *                  syntax.
 *
 * @return true on success, false otherwise.
 */
bool ham_useradd(const char * login, const char * roles, const char * hashed_pw);

/**
 * @brief Delete user from the host's /etc/passwd, /etc/group, /etc/shadow
 *        DBs.
 *
 * @param login     Login name
 *
 * @return true on success, false otherwise.
 */
bool ham_userdel(const char * login);

/**
 * @brief Add a new group to the host's /etc/group DB.
 *
 * @param group     Group name
 * @param options
 *
 * @return true on success, false otherwise.
 */
bool ham_groupadd(const char * group);

/**
 * @brief Remove a group from the host's /etc/group DB.
 *
 * @param group     Group name
 *
 * @return true on success, false otherwise.
 */
bool ham_groupdel(const char * group);

/**
 * @brief This is to be invoked by the PAM RADIUS and/or TACACS+ modules
 *        after a remote user has been authenticated. This tells the system
 *        that the System-assigned credentials (SAC) can be persisted in
 *        the local DB (/etc/passwd).
 *
 * @param login_p       User's login name
 * @param roles_p       List of comma-separated roles for that user.
 * @param auth_method_p Authentication method. E.g. "tacacs+", "radius".
 *
 * @return bool    true if successful, false otherwise.
 */
bool sac_user_confirm(const char * login_p, const char * roles_p, const char * auth_method_p);

#ifdef __cplusplus
}
#endif

#endif // __HAM_LIB_H

