#ifndef __HAMD_GECOS_H__
#define __HAMD_GECOS_H__

#include <string>               // std::string
#include <vector>               // std::vector

class gecos_c;
class roles_c
{
public:
    const char * c_str() const { return roles_pm; }
    std::vector< std::string > members() const;

protected:
    friend class gecos_c;
    void init(const std::string & roles_r) { roles_pm = roles_r.c_str(); }

private:
    const char * roles_pm = "roles=";
};

/**
 * @brief The GECOS is one of the fields of each entry in /etc/passwd.
 *
 *        A typical entry in /etc/passwd looks like this:
 *
 *           Login:pw:uid:gid:GECOS:HomeDir:Shell
 *
 *        The GECOS field is a list of comma-separated sub-fields as follows:
 *
 *           FullName,RoomAddress,WorkPhone,HomePhone,Other1,Other2,Other3,...
 *
 *        User "roles" are specified as an "other" sub-field of the GECOS field.
 *        Roles are semi-colon separated to avoid confusion with the colon
 *        and comma already used as separators elsewhere in the /etc/passwd
 *        entry.
 *
 *           roles=netadmin;secadmin;lord-of-the-vlans
 *
 *        Example of a complete /etc/passwd entry:
 *
 *           homer:x:1012:1012:Homer J. Simpson,,,,roles=secadmin;lord-of-the-vlans:/home/homer:/bin/bash
 *
 *        The gecos_c class is a tool to make it easy to access the fields and
 *        sub-fields of the GECOS.
 *
 * @see
 *  -Wiki GECOS:  https://en.wikipedia.org/wiki/Gecos_field
 *  -chfn(1):     https://man7.org/linux/man-pages/man1/chfn.1@@shadow-utils.html
 *  -useradd(8):  https://man7.org/linux/man-pages/man8/useradd.8.html
 *  -getpwnam(3): https://man7.org/linux/man-pages/man3/getpwnam.3.html
 *
 * @note We use the "RoomAddress" to identify where the user is located.
 *       E.g. "local", "tacacs+", "radius".
 */
class gecos_c
{
public:
    gecos_c();
    gecos_c(const char * pw_gecos);
    void init(const char * pw_gecos);

    const char    * full_name()  const { return members_m[0].c_str(); }
    const char    * room_addr()  const { return members_m[1].c_str(); }
    const char    * home_phone() const { return members_m[2].c_str(); }
    const char    * work_phone() const { return members_m[3].c_str(); }

    const roles_c & roles()      const { return roles_m; }

    const char    * c_str()      const { return text_m.c_str(); }
    std::string     text()       const { return text_m; }

    void set(const char * roles_r=nullptr,       /* nullptr means leave unchanged */
             const char * full_name_p=nullptr,   /* nullptr means leave unchanged */
             const char * room_addr_p=nullptr,   /* nullptr means leave unchanged */
             const char * home_phone_p=nullptr,  /* nullptr means leave unchanged */
             const char * work_phone_p=nullptr); /* nullptr means leave unchanged */

private:
    std::vector< std::string >  members_m;
    std::string                 text_m = ",,,";
    roles_c                     roles_m;
    size_t                      roles_index_m = 4;
};

std::string roles_as_string(const std::vector< std::string > & roles);


#endif // __HAMD_GECOS_H__
