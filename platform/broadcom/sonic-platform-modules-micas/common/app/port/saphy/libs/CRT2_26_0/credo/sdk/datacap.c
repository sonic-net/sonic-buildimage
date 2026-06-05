#include "dii.h"

#include "crintl/datacap.h"
#include "credo.h"
#include "sdk.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
CredoError_t cr_data_capture(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[], size_t argc,
                             CredoDataWriter_t writer, void* ud) {
    CALL_HAL(slice, hal_data_capture(slice, command, argv, argc, writer, ud));
}

typedef struct {
    FILE* fp;
    int err;
} CredoCaptureFile_t;

static void cr_capture_file(CredoCaptureFile_t* cap, const char* fmt, ...) {
    if (cap->err != 0) return;
    va_list ap;
    va_start(ap, fmt);
    int rc = vfprintf(cap->fp, fmt, ap);
    va_end(ap);
    if (rc < 0) {
        cap->err = errno;
    }
}

CredoError_t cr_data_capture_file(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[],
                                  size_t argc, const char* file) {
    CredoCaptureFile_t cap = {.err = 0};
    cap.fp = fopen(file, "a");
    if (cap.fp == NULL) {
        return CR_INVALID_ARGS;
    }
    CredoError_t err = cr_data_capture(slice, command, argv, argc, (CredoDataWriter_t)cr_capture_file, (void*)&cap);
    fclose(cap.fp);
    if (err != CR_OK) {
        return err;
    }
    if (cap.err != 0) {
        LOGS_ERROR("%s", strerror(cap.err));
        return CR_FAIL;
    }
    return err;
}

typedef struct {
    char* output;
    size_t len;
    size_t size;
} CredoCaptureStr_t;

static void cr_capture_buffer(CredoCaptureStr_t* capture, const char* fmt, ...) {
    if (capture->output == NULL) return;  // skip if it has already failed
    va_list ap;
    va_start(ap, fmt);
    size_t available = capture->size - capture->len;
    // have non-error path only compute once
    int size = vsnprintf(capture->output + capture->len, available, fmt, ap);
    va_end(ap);
    if (size < 0) {
        goto failed;
    }
    // if not enough space, realloc and rewrite the data
    if ((size_t)size >= available) {
        // double buffer size needed
        size_t new_buff_size = (capture->len + size) << 1;
        char* new_output = realloc(capture->output, new_buff_size);

        if (new_output == NULL) {
            goto failed;
        }
        capture->output = new_output;
        capture->size = new_buff_size;
        available = capture->size - capture->len;
        va_start(ap, fmt);
        size = vsnprintf(capture->output + capture->len, available, fmt, ap);
        va_end(ap);
        if (size < 0) {
            goto failed;
        }
    }
    capture->len += size;
    return;
failed:
    free(capture->output);
    capture->output = NULL;
    return;
}
#define CAPUTRE_BUFFER_START_SIZE 8192
CredoError_t cr_data_capture_buffer(CredoSlice_t* slice, const char* command, const CredoDataCaptureArg_t argv[],
                                    size_t argc, char** output) {
    CredoCaptureStr_t capture = {
        .output = malloc(CAPUTRE_BUFFER_START_SIZE), .size = CAPUTRE_BUFFER_START_SIZE, .len = 0};

    if (capture.output == NULL) {
        return CR_OUT_OF_MEMORY;
    }
    capture.output[0] = '\0';  // start as empty string
    CredoError_t err =
        cr_data_capture(slice, command, argv, argc, (CredoDataWriter_t)cr_capture_buffer, (void*)&capture);
    if (err != CR_OK) {
        free(capture.output);
        *output = NULL;
        return err;
    }
    // not enough memory
    if (capture.output == NULL) {
        *output = NULL;
        return CR_OUT_OF_MEMORY;
    }
    *output = capture.output;
    return CR_OK;
}

void* cri_data_get_commands(CredoSlice_t* slice, size_t* command_count) {
    if (slice->hal->hal_data_get_commands == NULL) return NULL;
    return slice->hal->hal_data_get_commands(slice, command_count);
}

void cri_data_free_buffer(char** output) {
    free(*output);
    *output = NULL;
}
