/*
 * Copyright(C) 2018. All rights reserved.
 */
/*
 * debug.h
 *
 * Debug control for showing version
 *
 */
 
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "debug.h"

/* 
 * show_version_debug
 * Function: Handle the debug switch
 * Parses the file /var/tmp/.show_version_debug and returns the corresponding debug information 
 * Return value: Returns DEBUG_OFF if debug is off, DEBUG_ON if it is on, and DEBUG_IGNORE for other cases
 */
int show_version_debug(void)
{
    int size;
    FILE *fp;
    char debug_info[DEBUG_INFO_LEN];

    fp = fopen(DEBUG_FILE, "r");
    if (fp == NULL) {
        return DEBUG_IGNORE;
    }
    
    memset(debug_info, 0, DEBUG_INFO_LEN);
    size = fread(debug_info, DEBUG_INFO_LEN - 1, 1, fp);
    if (size < 0) {
        fclose(fp);
        return DEBUG_IGNORE;
    }

    if (strncmp(debug_info, DEBUG_ON_INFO, 1) == 0) {
        fclose(fp);
        return DEBUG_ON;
    }

    if (strncmp(debug_info, DEBUG_OFF_INFO, 1) == 0) {
        fclose(fp);
        return DEBUG_OFF;
    }

    fclose(fp);
    return DEBUG_IGNORE;
}

