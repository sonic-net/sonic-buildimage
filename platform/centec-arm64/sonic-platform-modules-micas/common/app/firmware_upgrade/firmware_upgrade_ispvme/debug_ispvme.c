/*
 * debug.c
 *
 * firmware upgrade debug control
 *
 * Original Author : support <support>2013-10-25
 */
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <debug_ispvme.h>

/**
 * firmware_upgrade_debug:handle debug switch
 *
 * analyse file /tmp/firmware_upgrade_debug,return the information correspoding to debug
 *
 * return:return DEBUG_OFF when debug off,return DEBUG_ON when debug on,return DEBUG_IGNORE in other cases
 */
int firmware_upgrade_debug(void)
{
    int size;
    FILE *fp;
    char debug_info[DEBUG_INFO_LEN] = {0};

    fp = fopen(DEBUG_FILE, "r");
    if (fp == NULL) {
        return DEBUG_IGNORE;
    }

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