#ifndef COMMON_FW_DUMP_H
#define COMMON_FW_DUMP_H

#include "common/common_display.h"

#include "sdk.h"

typedef struct dump_info {
    char* name;
    int section;
    int index;
    int len;
    int type;
    int type_arg1;
    int fmt;
    int fmt_arg1;
    int fmt_arg2;
    const char** timer_name;
} dump_info_t;

enum dump_format {
    DUMP_FORMAT_UNSIGNED,
    DUMP_FORMAT_UNSIGNED_HEX,
    DUMP_FORMAT_UNSIGNED_SPLIT,
    DUMP_FORMAT_SIGNED,
    DUMP_FORMAT_SIGNED_DIV,
    DUMP_FORMAT_FLOAT,
    DUMP_FORMAT_MAX,
};

enum dump_type {
    DUMP_TYPE_LIST,
    DUMP_TYPE_MATRIX,
    DUMP_TYPE_TIMER,
    DUMP_TYPE_MAX,
};

#define DUMP_BASIC(NAME, SECTION, CODE, LEN, TYPE, FORMAT)                                             \
    .name = NAME, .section = SECTION, .index = SECTION##_##CODE, .len = LEN, .type = DUMP_TYPE_##TYPE, \
    .fmt = DUMP_FORMAT_##FORMAT

#define DUMP(FORMAT, NAME, SECTION, CODE, LEN) \
    { DUMP_BASIC(NAME, SECTION, CODE, LEN, LIST, FORMAT) }

#define DUMP1(FORMAT, NAME, SECTION, CODE, LEN, ARG1) \
    { DUMP_BASIC(NAME, SECTION, CODE, LEN, LIST, FORMAT), .fmt_arg1 = ARG1 }

#define DUMP2(FORMAT, NAME, SECTION, CODE, LEN, ARG1, ARG2) \
    { DUMP_BASIC(NAME, SECTION, CODE, LEN, LIST, FORMAT), .fmt_arg1 = ARG1, .fmt_arg2 = ARG2 }

#define DUMP_MATRIX(FORMAT, NAME, SECTION, CODE, LEN, ROW) \
    { DUMP_BASIC(NAME, SECTION, CODE, LEN, MATRIX, FORMAT), .type_arg1 = ROW }

#define DUMP_TIMERS(NAME, SECTION, CODE, LEN, TIMER_NAME) \
    { DUMP_BASIC(NAME, SECTION, CODE, LEN, TIMER, MAX), .timer_name = TIMER_NAME }

#define DUMP_UNSIGNED(NAME, SECTION, CODE, LEN) DUMP(UNSIGNED, NAME, SECTION, CODE, LEN)

#define DUMP_UNSIGNED_HEX(NAME, SECTION, CODE, LEN) DUMP(UNSIGNED_HEX, NAME, SECTION, CODE, LEN)

#define DUMP_UNSIGNED_SPLIT(NAME, SECTION, CODE, LEN, ARG1, ARG2) \
    DUMP2(UNSIGNED_SPLIT, NAME, SECTION, CODE, LEN, ARG1, ARG2)

#define DUMP_SIGNED(NAME, SECTION, CODE, LEN) DUMP(SIGNED, NAME, SECTION, CODE, LEN)

#define DUMP_SIGNED_DIV(NAME, SECTION, CODE, LEN, ARG1, ARG2) DUMP2(SIGNED_DIV, NAME, SECTION, CODE, LEN, ARG1, ARG2)

#define DUMP_FLOAT(NAME, SECTION, CODE, LEN, ARG1) DUMP1(FLOAT, NAME, SECTION, CODE, LEN, ARG1)

CredoError_t common_fw_dump_debug(CredoSlice_t* slice, int lane, dump_info_t* dump_list, unsigned count,
                                  const DisplayState_t* D);

#endif
