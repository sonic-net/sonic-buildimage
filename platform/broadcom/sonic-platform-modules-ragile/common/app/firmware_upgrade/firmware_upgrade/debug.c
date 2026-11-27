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
 * Reads the first byte of DEBUG_FILE ("/tmp/.firmware_upgrade_debug").
 * Returns DEBUG_OFF for '0', DEBUG_APP_ON for '1', DEBUG_ALL_ON for '3',
 * and DEBUG_IGNORE if the file cannot be opened, is empty, or starts with another byte.
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
