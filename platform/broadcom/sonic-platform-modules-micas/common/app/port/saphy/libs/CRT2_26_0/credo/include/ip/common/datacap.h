#ifndef DATACAP_H
#define DATACAP_H

#include "sdk.h"

#define DC_START_COMMA(D, state) \
    do {                         \
        if ((state)) {           \
            DC_PRINTF((D), ","); \
        }                        \
        (state) = true;          \
    } while (0)
typedef struct {
    CredoDataWriter_t CR_PRINTF_ATTRIBUTE_FORMAT(2, 3) writer;
    void* ud;
    unsigned optMode;
    bool prefixComma;
    bool inHeader;
} DataCaptureState_t;

typedef struct {
    const char* name;
    int default_value;
    const char* description;
} DataCaptureParam_t;

typedef CredoError_t (*DataCaptureRunner_t)(CredoSlice_t* slice, const CredoDataCaptureArg_t argv[], size_t argc,
                                            DataCaptureState_t* D);

typedef struct {
    const char* name;
    const char* description;
    DataCaptureRunner_t runner;
    const DataCaptureParam_t params[64];
} DataCaptureCommand_t;

typedef struct {
    const char* name;
    int* value;
} DataCaptureArgMap_t;

const DataCaptureCommand_t* datacap_find_command(const DataCaptureCommand_t commands[], const char* command_name);
/**
 * @brief Extract arguments from users
 *
 * @param[in] argv user arguments
 * @param[in] argc
 * @param[in] map mapping that stores user arguments into a table
 * @return position of invalid argument, if none then it will return argc
 */
size_t datacap_extract_args(const CredoDataCaptureArg_t argv[], size_t argc, DataCaptureArgMap_t map[]);

// Print function that

#define FIRST_ARG(N, ...) N

static inline void write_opt(const DataCaptureState_t* D, const char* fmt) {
    while (*fmt != '\0') {
        if (*fmt == ',') {
            D->writer(D->ud, ",");
        }
        fmt++;
    }
}

#define DC_CSV_INHEADER(D, condition) ((D)->inHeader = (condition))

#define DC_CSV_PRINTF_HEADER(D, ...)                         \
    do {                                                     \
        if ((D)->inHeader) {                                 \
            if ((D)->prefixComma) (D)->writer((D)->ud, ","); \
            DC_PRINTF(D, __VA_ARGS__);                       \
            (D)->prefixComma = true;                         \
        }                                                    \
    } while (0);

#define DC_CSV_PRINTF_CELL(D, ...)                           \
    do {                                                     \
        if ((D)->inHeader) {                                 \
        } else if ((D)->optMode) {                           \
            if ((D)->prefixComma) (D)->writer((D)->ud, ","); \
            write_opt(D, FIRST_ARG(__VA_ARGS__, 0));         \
            (D)->prefixComma = true;                         \
        } else {                                             \
            if ((D)->prefixComma) (D)->writer((D)->ud, ","); \
            (D)->writer((D)->ud, __VA_ARGS__);               \
            (D)->prefixComma = true;                         \
        }                                                    \
    } while (0)

#define DC_CSV_PRINTF(D, headers, ...)          \
    do {                                        \
        DC_CSV_PRINTF_HEADER(D, "%s", headers); \
        DC_CSV_PRINTF_CELL(D, __VA_ARGS__);     \
    } while (0);

#define DC_CSV_LN(D)                \
    do {                            \
        (D)->writer((D)->ud, "\n"); \
        (D)->prefixComma = false;   \
    } while (0)

#define DC_PRINTF(D, ...) (D)->writer((D)->ud, __VA_ARGS__)

#define DC_CSV_PRINTF_CELL_ARRAY(D, format, array_data, size) \
    do {                                                      \
        for (size_t i = 0; i < (size); i++) {                 \
            DC_CSV_PRINTF_CELL(D, format, (array_data)[i]);   \
        }                                                     \
    } while (0)

#define DC_CSV_PRINTF_HEADER_ARRAY(D, format, start, end) \
    do {                                                  \
        for (int i = start; i < (end); i++) {             \
            DC_CSV_PRINTF_HEADER(D, format, i);           \
        }                                                 \
    } while (0)

static inline void dc_opt(DataCaptureState_t* D, bool condition) {
    // if in opt mode, increment that we are in opt mode (or starting opt mode)
    (D)->optMode += (condition || D->optMode > 0) ? 1 : 0;
}

static inline void dc_optend(DataCaptureState_t* D) {
    // always decrement opt mode if possible
    (D)->optMode -= ((D)->optMode > 0) ? 1 : 0;
}

// allow for recursive opt without optend completely canceling it
#define DC_CSV_OPT(D, condition) dc_opt(D, condition)
#define DC_CSV_OPTEND(D)         dc_optend(D)

#endif
