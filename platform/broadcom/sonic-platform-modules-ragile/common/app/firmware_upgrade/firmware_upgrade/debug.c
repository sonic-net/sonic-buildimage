/*
 * debug.c
 * firmware upgrade debug switch control
 */

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <debug.h>

int is_debug_on = DEBUG_IGNORE;

/*
 * firmware_upgrade_debug
 * function: Debug switch
 * Parses the file "/var/tmp/.firmware_upgrade_debug" and returns the corresponding debug level
 * return:off--DEBUG_OFF,  app debug on---DEBUG_APP_ON, kernel debug on--DEBUG_KERN_ON,
 *        all debug on--DEBUG_ALL_ON, other--DEBUG_IGNORE
 */
int firmware_upgrade_debug(void)
{
    int value;
    FILE *fp;

    fp = fopen(DEBUG_FILE, "r");
    if (fp == NULL) {
        return DEBUG_IGNORE;
    }

    value = fgetc(fp);
    fclose(fp);

    switch (value) {
    case '1':
        return DEBUG_APP_ON;
    case '3':
        return DEBUG_ALL_ON;
    case '0':
        return DEBUG_OFF;
    default:
        return DEBUG_IGNORE;
    }
}
