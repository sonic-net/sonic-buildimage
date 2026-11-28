/* Exercise the installed C header names and the shared ABI used by FRR. */
#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <nexthopgroup/c_nexthopgroupfull.h>
#include <nexthopgroup/c-api/nexthopgroup_capi.h>

int main(void)
{
    if (strcmp(nexthopgroup_version(), "1.0.0") != 0)
        return 1;
    fib_frr_set_log_level(3);
    if (fib_frr_get_log_level() != 3)
        return 2;
    fib_frr_set_log_level(0);
    if (fib_frr_get_log_level() != 0)
        return 3;

    /* Cross the generated C-structure boundary used by the FRR plugin. */
    struct C_NextHopGroupFull group = {0};
    group.id = 100;
    group.key = 1234567;
    group.nhg_flags = 1024;
    group.nh_grp_full_list[0].id = 200;
    group.nh_grp_full_list[0].weight = 1;
    group.depends[0] = 200;
    group.dependents[0] = 300;
    char *json = nexthopgroupfull_json_from_c_nhg_multi(&group, 1, 1, 1, false);
    if (json == NULL)
        return 4;
    int valid = strstr(json, "\"id\":100") != NULL &&
                strstr(json, "\"nhg_flags\":1024") != NULL &&
                strstr(json, "\"depends\":[200]") != NULL &&
                strstr(json, "\"dependents\":[300]") != NULL;
    free(json);
    return valid ? 0 : 5;
}
