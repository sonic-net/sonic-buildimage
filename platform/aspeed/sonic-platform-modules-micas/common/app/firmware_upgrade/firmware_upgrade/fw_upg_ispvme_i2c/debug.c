#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "debug.h"

int g_cpld_i2c_debug = 0;

void cpld_i2c_debug_init(void)
{
    FILE *fp;
    char *stopstring;
    char buf[32];

    memset(buf, 0, sizeof(buf));
    fp = fopen(DEBUG_FILE, "r");
    if (fp != NULL) {
        if (fgets(buf, sizeof(buf), fp) != NULL) {
            g_cpld_i2c_debug = strtol(buf, &stopstring, 16);
        }

        fclose(fp);
    }

    return;
}
