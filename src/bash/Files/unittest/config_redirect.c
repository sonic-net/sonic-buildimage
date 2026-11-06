#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fopen(const char *path, const char *mode)
{
    FILE *(*real_fopen)(const char *, const char *);
    const char *test_config;

    real_fopen = dlsym(RTLD_NEXT, "fopen");
    test_config = getenv("BASH_PLUGIN_TEST_CONFIG");
    if (test_config && strcmp(path, "/etc/bash_plugins.conf") == 0)
        path = test_config;
    return real_fopen(path, mode);
}
