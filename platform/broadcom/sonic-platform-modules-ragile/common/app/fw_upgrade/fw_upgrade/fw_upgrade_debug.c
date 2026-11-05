#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "fw_upgrade_debug.h"

int fw_upgrade_debug(void)
{
    int size;
    FILE *fp;
    char debug_info[DEBUG_INFO_LEN];

    fp = fopen(DEBUG_FILE, "r");
    if (fp == NULL) {
        return DEBUG_IGNORE;
    }

    mem_clear(debug_info, DEBUG_INFO_LEN);
    size = fread(debug_info, DEBUG_INFO_LEN - 1, 1, fp);
    if (size < 0) {
        fclose(fp);
        return DEBUG_IGNORE;
    }

    if (strncmp(debug_info, DEBUG_ON_INFO, 1) == 0) {
        fclose(fp);
        return DEBUG_APP_ON;
    }

    if (strncmp(debug_info, DEBUG_ON_KERN, 1) == 0) {
        fclose(fp);
        return DEBUG_KERN_ON;
    }

    if (strncmp(debug_info, DEBUG_ON_ALL, 1) == 0) {
        fclose(fp);
        return DEBUG_ALL_ON;
    }

    if (strncmp(debug_info, DEBUG_OFF_INFO, 1) == 0) {
        fclose(fp);
        return DEBUG_OFF;
    }

    fclose(fp);
    return DEBUG_IGNORE;
}
