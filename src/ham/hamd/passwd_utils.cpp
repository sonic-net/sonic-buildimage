#include "passwd_utils.h"
#include "../shared/utils.h"    // streq()

/**
 * @brief Scan "/etc/passwd" looking for user. If found, return a pointer
 *        to a "struct passwd" containing all the data related to user.
 *
 *        The reason for creating this function instead of using the
 *        stadard POSIX getpwnam(), is that this function doesn't use the
 *        underlying NSS infrastructure. Instead, it access /etc/passwd
 *        directly, which is what we need.
 *
 * @param user The user we're looking for
 *
 * @return If user found, return a pointer to a struct passwd.
 */
struct passwd * fgetpwnam(const char * user)
{
    struct passwd * pwd = NULL;
    FILE          * f   = fopen("/etc/passwd", "re");
    if (f)
    {
        struct passwd * ent;
        while (NULL != (ent = fgetpwent(f)))
        {
            if (streq(ent->pw_name, user))
            {
                pwd = ent;
                break;
            }
        }
        fclose(f);
    }

    return pwd;
}
