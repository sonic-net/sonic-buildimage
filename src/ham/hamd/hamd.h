// Host Account Management
#ifndef HAMD_H
#define HAMD_H

#include <dbus-c++/dbus.h>          // DBus::Connection
#include <glib.h>                   // gpointer
#include <string>                   // std::string
#include <vector>                   // std::vector
#include <set>                      // std::set
#include "timer.h"                  // gtimer_c
#include "hamd_config.h"            // hamd_config_c

#include "../shared/dbus-address.h" // DBUS_BUS_NAME_BASE
#include "../shared/org.SONiC.HostAccountManagement.dbus-adaptor.h" // Generated file

struct redis_c; // forward declaration

class hamd_c : public DBus::ObjectAdaptor,
               public DBus::IntrospectableAdaptor,
               public ham::accounts_adaptor,
               public ham::name_service_adaptor,
               public ham::sac_adaptor,
               public ham::debug_adaptor
{
public:
    hamd_c(hamd_config_c & config_r, DBus::Connection  & conn_r, redis_c & redis_r);
    virtual ~hamd_c() {}

    // DBus "accounts" interface
    virtual ::DBus::Struct< bool, std::string > useradd(const std::string& login, const std::vector< std::string >& roles, const std::string& hashed_pw);
    virtual ::DBus::Struct< bool, std::string > userdel(const std::string& login);
    virtual ::DBus::Struct< bool, std::string > passwd(const std::string& login, const std::string& hashed_pw);
    virtual ::DBus::Struct< bool, std::string > set_roles(const std::string& login, const std::vector< std::string >& roles);
    virtual ::DBus::Struct< bool, std::string > groupadd(const std::string& group);
    virtual ::DBus::Struct< bool, std::string > groupdel(const std::string& group);

    // DBus "nss" interface
    virtual ::DBus::Struct< bool, std::string, std::string, uint32_t, uint32_t, std::string, std::string, std::string > getpwnam(const std::string& name);
    virtual ::DBus::Struct< bool, std::string, std::string, uint32_t, uint32_t, std::string, std::string, std::string > getpwuid(const uint32_t& uid);
    virtual std::string getpwcontents();
    virtual ::DBus::Struct< bool, std::string, std::string, uint32_t, std::vector< std::string > > getgrnam(const std::string& name);
    virtual ::DBus::Struct< bool, std::string, std::string, uint32_t, std::vector< std::string > > getgrgid(const uint32_t& gid);
    virtual std::string getgrcontents();
    virtual ::DBus::Struct< bool, std::string, std::string, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, uint32_t > getspnam(const std::string& name);

    // DBus "sac" interface
    virtual std::string add_unconfirmed_user(const std::string & login, const uint32_t & pid);
    virtual std::string user_confirm(const std::string & login, const std::vector<std::string> & roles, const std::string & auth_method);

    // DBus "debug" interface
    virtual std::string  tron();
    virtual std::string  troff();
    virtual std::string  show();

    bool                 is_tron() const { return config_rm.tron_m; }
    virtual void         cleanup();
    void                 reload();
    void                 apply_config();

private:
    hamd_config_c      & config_rm;
    gtimer_c             poll_timer_m;
    static bool          on_poll_timeout_cb(gpointer user_data_p); // This callback functions must follow GSourceFunc signature.
    void                 on_poll_timeout();
    void                 rm_unconfirmed_users() const;
    std::string          post_create_scripts(const std::string  & login) const;
    std::string          pre_delete_scripts (const std::string  & login) const;

    std::string          create_user(const std::string& login, const std::vector< std::string >& roles, const std::string& hashed_pw);
    std::string          delete_user(const std::string& login);

    static std::set<std::string> get_groups(const std::vector< std::string > & roles);
    static std::string get_groups_as_string(const std::vector< std::string > & roles);

    redis_c            & redis_rm;
};

#endif /* HAMD_H */
