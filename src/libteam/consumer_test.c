/* Public library lifecycle and log priority contract, documented in team.h,
 * teamdctl.h and the corresponding upstream API documentation comments. */
#include <stddef.h>
#include <syslog.h>
#include <team.h>
#include <teamdctl.h>

int main(void)
{
    struct team_handle *team = team_alloc();
    struct teamdctl *control = teamdctl_alloc();
    if (!team || !control) return 1;
    team_set_log_priority(team, LOG_DEBUG);
    if (team_get_log_priority(team) != LOG_DEBUG) return 2;
    team_set_log_priority(team, LOG_WARNING);
    if (team_get_log_priority(team) != LOG_WARNING) return 2;
    teamdctl_set_log_priority(control, LOG_INFO);
    if (teamdctl_get_log_priority(control) != LOG_INFO) return 2;
    teamdctl_set_log_priority(control, LOG_ERR);
    if (teamdctl_get_log_priority(control) != LOG_ERR) return 2;
    teamdctl_free(control);
    team_free(team);
    return 0;
}
