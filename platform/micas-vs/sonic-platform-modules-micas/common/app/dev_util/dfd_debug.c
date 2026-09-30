/*
 * An dfd_debug driver for dfd debug function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include "dfd_utest.h"

int g_dfd_debug_sw = 0;
int g_dfd_debugpp_sw = 0;

void dfd_debug_set_init(void)
{
    FILE *fp;
    char buf[10];

    mem_clear(buf, sizeof(buf));
    fp = fopen(DFD_DEBUGP_DEBUG_FILE, "r");
    if (fp != NULL) {

        g_dfd_debug_sw = 1;
        fclose(fp);
    }

    fp = fopen(DFD_DEBUGPP_DEBUG_FILE, "r");
    if (fp != NULL) {

        g_dfd_debugpp_sw = 1;
        fclose(fp);
    }

    return;
}

int main(int argc, char* argv[])
{
    dfd_debug_set_init();
    dfd_utest_cmd_main(argc, argv);

    return 0;
}
