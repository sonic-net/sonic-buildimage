// Prior to running this, the hamd daemon must be running
// and the NSS ham module need to be installed in /lib/x86_64-linux-gnu/
// as follows:
//              sudo cp libnss_ham.so.2 /lib/x86_64-linux-gnu/.


#include <nss.h>
#include <pwd.h>        /* getpwnam() */
#include <grp.h>        /* getgrnam(), getgrouplist() */
#include <syslog.h>     /* syslog(), LOG_ERR */
#include <unistd.h>     /* sysconf() */
#include <stdbool.h>    /* bool */
#include <stdlib.h>     /* malloc(), exit() */
#include <stdio.h>      /* printf() */

/**
 * @brief Test whether a user is a member of a role.
 *
 * @details The role_member() function tests whether a user has been
 *          assigned to a particular role.
 *
 * @param[in] user - The user name
 * @param[in] role - The role
 *
 * @return bool true on success, false otherwise.
 */
bool role_member(const char * user, const char * role)
{
    struct passwd * pwd = getpwnam(user);
    if (pwd == NULL)
    {
        syslog(LOG_ERR, "role_member() - Unknown user %s", user);
        return false;
    }
    gid_t   primary_group  = pwd->pw_gid;

    struct group * grp = getgrnam(role);
    if (grp == NULL)
    {
        syslog(LOG_ERR, "role_member() - Unknown role (i.e. group) %s", role);
        return false;
    }
    gid_t   role_gid = grp->gr_gid;

    // Check primary Group ID
    if (primary_group == role_gid)
        return true;

    // Check supplementary Group IDs
    int     ngroups  = (int)sysconf(_SC_NGROUPS_MAX) + 1;
    size_t  memsize  = ngroups * sizeof(gid_t);
    gid_t * groups   = (gid_t *)malloc(memsize);
    if (groups == NULL)
    {
        syslog(LOG_ERR, "role_member() - malloc() error. size = %lu", memsize);
        return false;
    }

    bool is_member = false;
    if (getgrouplist(user, primary_group, groups, &ngroups) == -1)
    {
       syslog(LOG_ERR, "role_member() - getgrouplist() failed; ngroups = %d\n", ngroups);
    }
    else
    {
        for (int i = 0; i < ngroups; i++)
        {
            if (role_gid == groups[i])
            {
                is_member = true;
                break;
            }
        }
    }

    free(groups);

    return is_member;
}

int main(int argc, char *argv[])
{
    __nss_configure_lookup("passwd", "ham");
    __nss_configure_lookup("group",  "ham");
    __nss_configure_lookup("shadow", "ham");

    const char * users[] =
    {
        "mbelanger",
        "syslog",
        "pipi",
        NULL
    };

    const char * roles[] =
    {
        "sudo",
        "adm",
        "caca",
        NULL
    };

    const char * role;
    const char * user;
    unsigned     i, j;
    for (i = 0, user = users[0]; user != NULL; user = users[++i])
    {
        for (j = 0, role = roles[0]; role != NULL; role = roles[++j])
        {
            printf("%-9s %s member of %s\n", user, role_member(user, role) ? "\033[32;1mis\033[0m    " : "\033[31;1mis not\033[0m", role);
        }
    }

    exit(EXIT_SUCCESS);
}


