#ifndef COMMON_DISPLAY_H
#define COMMON_DISPLAY_H

#include "argparse.h"

#include "sdk.h"

#define USAGE_COUNT 8

typedef struct DisplayCommand DisplayCommand_t;

typedef struct {
    CredoDisplayWriter_t CR_PRINTF_ATTRIBUTE_FORMAT(2, 3) writer;
    void* ud;
    const DisplayCommand_t* command;
} DisplayState_t;

struct DisplayCommand {
    const char* name;
    const char* description;
    const char* usages[16];
    const char* epilog;
    CredoError_t (*cb)(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
};

CredoError_t display_info(CredoSlice_t* slice, size_t argc, const char* argv[], DisplayState_t* D,
                          const DisplayCommand_t commands[], size_t command_len);

enum argparse_exit display_parse_args(CredoSlice_t* slice, int argc, const char* argv[],
                                      struct argparse_option* options, const DisplayState_t* D);

#define DISPLAY_PARSE_ARGS(slice, argc, argv, options, D)                 \
    do {                                                                  \
        int arg_exit = display_parse_args(slice, argc, argv, options, D); \
        if (arg_exit == ARGPARSE_EXIT_ERROR) {                            \
            return CR_INVALID_ARGS;                                       \
        }                                                                 \
        if (arg_exit == ARGPARSE_EXIT_SKIP) {                             \
            return CR_OK;                                                 \
        }                                                                 \
        (argc) = arg_exit;                                                \
    } while (0)

#define DISPF(D, ...) (D)->writer((D)->ud, __VA_ARGS__)

#define DISPLAY_CMD_DOC(arg_str, doc) arg_str "  # " doc
#define ADD_DISPLAY_CMD(cmd_name, func, ...)                    \
    {                                                           \
        .name = cmd_name, .cb = func, .usages = { __VA_ARGS__ } \
    }

#define DISPLAY_ARGP_CMD(cmd_name, func, desc, uses, eplg) \
    { .name = (cmd_name), .cb = (func), .description = (desc), .usages = uses, .epilog = (eplg) }

#define DISPLAY_ARGP_USES(...) \
    { __VA_ARGS__ }

#define ARGCOUNT_CHECK(cond)    \
    if (!(cond)) {              \
        return CR_INVALID_ARGS; \
    }

// assumes argc and argv are defined
#define ARGPARSE_STRTOL(arg_idx, val)                \
    do {                                             \
        if (argc <= arg_idx) return CR_INVALID_ARGS; \
        char* endptr;                                \
        val = strtol(argv[arg_idx], &endptr, 10);    \
        if (*endptr != '\0') return CR_INVALID_ARGS; \
    } while (0);

// assumes argc and argv are defined
#define ARGPARSE_STRTOUL(arg_idx, val)               \
    do {                                             \
        if (argc <= arg_idx) return CR_INVALID_ARGS; \
        char* endptr;                                \
        val = strtoul(argv[arg_idx], &endptr, 10);   \
        if (*endptr != '\0') return CR_INVALID_ARGS; \
    } while (0);

typedef enum {
    NON_SPLIT_DISPLAY,
    SPLIT_DISPLAY,
} split_display_t;

CredoError_t common_rx_monitor(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
CredoError_t common_slice_info(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);

typedef CredoError_t (*serdes_param_func_t)(CredoSlice_t* slice, const int* lane_list, split_display_t split_display,
                                            const DisplayState_t* D);
CredoError_t common_serdes_param_parser(CredoSlice_t* slice, serdes_param_func_t func, int argc, const char* argv[],
                                        const DisplayState_t* D);
CredoError_t common_mac_show_statistics(CredoSlice_t* slice, int port_num, int port_list[][2],
                                        CredoMACStatistics_t stats[][2], const DisplayState_t* D);
CredoError_t common_dump_fw_reg(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
CredoError_t common_fw_dump_exit_codes(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
CredoError_t common_fw_dump_error_codes(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);
CredoError_t common_fw_serdes_control(CredoSlice_t* slice, int argc, const char* argv[], const DisplayState_t* D);

#endif  // COMMON_DISPLAY_H
