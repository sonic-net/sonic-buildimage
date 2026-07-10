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
