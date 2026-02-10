// Host Account Management
#ifndef HAMD_CONFIG_H
#define HAMD_CONFIG_H

#include <sys/types.h>      // uid_t
#include <stdint.h>         // uint64_t
#include <glib.h>           // gint, gchar
#include <string>           // std::string
#include <ostream>          // std::ostream

class hamd_config_c
{
public:
    hamd_config_c(int argc, char **argv);

    /**
     * @brief Read configuration and update hamd_config_c object
     */
    void reload();

    uid_t uid_fit_into_range(uint64_t hash) const
    {
        return (uid_t)((hash % sac_uid_range_m) + sac_uid_min_m);
    }

    /**
     * @brief return the shell program to assign to new users.
     */
    const std::string & shell() const {return shell_m;}

    std::string to_string() const;
    bool        is_tron()   const { return tron_m; }

private:
    // PLEASE UPDATE etc/sonic/hamd/config
    // WHEN MAKING CHANGES TO DEFAULTS
    static const  gint  poll_period_sec_default_m  = 30;
    static const  gint  sac_uid_min_default_m      = 5000;  // System-Assigned IDs will be in the
    static const  gint  sac_uid_max_default_m      = 59999; // range [sac_uid_min_m..sac_uid_max_m]
    static const  bool  tron_default_m             = false;
    const gchar       * conf_file_default_pm       = "/etc/sonic/hamd/config";
    std::string         shell_default_m            = "/usr/bin/sonic-launch-shell";

public:
    bool                tron_m                     = tron_default_m;
    gint                poll_period_sec_m          = poll_period_sec_default_m;

    gint                sac_uid_min_m              = sac_uid_min_default_m;  // System-Assigned IDs will be in the
    gint                sac_uid_max_m              = sac_uid_max_default_m;  // range [sac_uid_min_m..sac_uid_max_m]

private:
    const gchar       * conf_file_pm               = conf_file_default_pm;
    std::string         shell_m                    = shell_default_m;
    gint                sac_uid_range_m            = 1 + (sac_uid_max_m - sac_uid_min_m);
};

std::ostream & operator<<(std::ostream  & stream_r, const hamd_config_c  & obj_r);

#endif // HAMD_CONFIG_H
