#include "common/datacap.h"

#include "sdk.h"

#include <string.h>

const DataCaptureCommand_t* datacap_find_command(const DataCaptureCommand_t commands[], const char* command_name) {
    for (const DataCaptureCommand_t* command = commands; command->name != NULL; command++) {
        if (strcmp(command->name, command_name) == 0) {
            return command;
        }
    }
    return NULL;
}

static const DataCaptureArgMap_t* find_arg_map(DataCaptureArgMap_t map[], const char* name) {
    for (const DataCaptureArgMap_t* map_arg = map; map_arg->name != NULL; map_arg++) {
        if (strcmp(map_arg->name, name) == 0) {
            return map_arg;
        }
    }
    return NULL;
}

size_t datacap_extract_args(const CredoDataCaptureArg_t argv[], size_t argc, DataCaptureArgMap_t map[]) {
    // user passed no options, leave as default
    if (argv == NULL) {
        return argc;
    }
    for (size_t i = 0; i < argc; i++) {
        CredoDataCaptureArg_t arg = argv[i];
        const DataCaptureArgMap_t* map_arg = find_arg_map(map, arg.name);
        if (map_arg == NULL) {
            return i;
        }
        *map_arg->value = arg.value;
    }
    return argc;
}
