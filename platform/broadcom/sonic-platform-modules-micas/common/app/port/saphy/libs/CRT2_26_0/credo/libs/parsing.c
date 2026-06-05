#include <stdio.h>
#include <string.h>

static int strtoint(const char* str, int* num) {
    char tmp[32];
    return *str == '-' || sscanf(str, "%d", num) != 1 ? 0 : sprintf(tmp, "%d", *num);
}

void parsing_string_to_lane_list(const char* str, int* lane_list, int max_lane) {
    int start_lane, end_lane, idx = 0;

    if (!str || *str == '\0' || !lane_list) return;

    if (strcmp(str, "all") == 0) {
        for (idx = 0; idx < max_lane; idx++) lane_list[idx] = idx;
        return;
    }

    while ((*str != '\0') && (idx < max_lane)) {
        int bytes = strtoint(str, &start_lane);
        if (bytes == 0) {
            ++str;
            continue;
        }
        str += bytes;
        lane_list[idx++] = start_lane;
        if ((*str == '-') && ((bytes = strtoint(++str, &end_lane)) != 0)) {
            int diff = (end_lane > start_lane) ? 1 : -1;
            str += bytes;
            while ((start_lane != end_lane) && (idx < max_lane)) {
                start_lane += diff;
                lane_list[idx++] = start_lane;
            }
        }
    }
    if (idx < max_lane) lane_list[idx++] = max_lane;
}
