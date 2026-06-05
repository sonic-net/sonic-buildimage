
#include "dii.h"

#include "crintl/display.h"

#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#define MAX_DISPLAY_ARGC 32

CredoError_t cr_slice_display_info(CredoSlice_t* slice, const char* command) {
    LOGS_API("command: %s", command);
    if (command == NULL) return CR_INVALID_ARGS;

    // duplicate command to not break const contract when using strtok_r
    char command_buffer[CR_CMDLINE_SIZE] = {0};
    char* command_parse = strncpy(command_buffer, command, CR_CMDLINE_SIZE - 1);

    char* argv[MAX_DISPLAY_ARGC] = {NULL};
    int argc = 0;

    for (char *tok, *saveptr = NULL;; command_parse = NULL) {
        tok = strtok_r(command_parse, " \t\n", &saveptr);
        if (tok == NULL) break;
        argv[argc++] = tok;
        if (argc >= MAX_DISPLAY_ARGC) {
            LOGS_ERROR("only support %d argument number", MAX_DISPLAY_ARGC);
            return CR_FAIL;
        }
    }
    if (slice->hal->hal_display_slice_info != NULL) {
        CALL_HAL(slice, hal_display_slice_info(slice, argc, (const char**)argv));
    }
    return cr_display_info_log(slice, (const char**)argv, argc);
}

CredoError_t cr_firmware_display_info(CredoSlice_t* slice, const char* command) {
    return CR_NOTIMPLEMENTED;
}

CredoError_t cr_port_display_info(CredoSlice_t* slice, const char* command, uint32_t port_id) {
    return CR_NOTIMPLEMENTED;
}

CredoError_t cr_lane_display_info(CredoSlice_t* slice, const char* command, int lane) {
    return CR_NOTIMPLEMENTED;
}

CredoError_t cr_display_info_log(CredoSlice_t* slice, const char* argv[], size_t argc) {
    char* buffer = NULL;
    CredoError_t err = cr_display_info_buffer(slice, argv, argc, &buffer);
    if (buffer != NULL && buffer[0] != '\0') {
        cr_llog_slice(slice, CR_LOG_INFO, buffer);
    }
    free(buffer);
    return err;
}

CredoError_t cr_display_info(CredoSlice_t* slice, const char* argv[], size_t argc, CredoDisplayWriter_t writer,
                             void* ud) {
    CALL_HAL(slice, hal_display_info(slice, argv, argc, writer, ud))
}

typedef struct {
    char* output;
    size_t len;
    size_t size;
} CaptureStr_t;

static void cr_display_buffer(CaptureStr_t* capture, const char* fmt, ...) {
    if (capture->output == NULL) return;  // skip if it has already failed
    va_list ap;
    size_t available = capture->size - capture->len;
    // have non-error path only compute once
    va_start(ap, fmt);
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

#define DISPLAY_BUFFER_START_SIZE 8192

CredoError_t cr_display_info_buffer(CredoSlice_t* slice, const char* argv[], size_t argc, char** output) {
    CaptureStr_t capture = {.output = malloc(DISPLAY_BUFFER_START_SIZE), .size = DISPLAY_BUFFER_START_SIZE, .len = 0};
    if (capture.output == NULL) {
        *output = NULL;
        return CR_OUT_OF_MEMORY;
    }
    capture.output[0] = '\0';  // make it empty on start
    CredoError_t err = cr_display_info(slice, argv, argc, (CredoDisplayWriter_t)cr_display_buffer, (void*)&capture);
    // not enough memory
    if (capture.output == NULL) {
        *output = NULL;
        return CR_OUT_OF_MEMORY;
    }
    *output = capture.output;
    return err;
}

typedef struct {
    FILE* fp;
    int err;
} CredoCaptureFile_t;

static void cr_display_file(CredoCaptureFile_t* cap, const char* fmt, ...) {
    if (cap->err != 0) return;
    va_list ap;
    va_start(ap, fmt);
    int rc = vfprintf(cap->fp, fmt, ap);
    va_end(ap);
    if (rc < 0) {
        cap->err = errno;
    }
}

CredoError_t cr_display_info_file(CredoSlice_t* slice, const char* argv[], size_t argc, const char* file) {
    CredoCaptureFile_t cap = {.err = 0};
    cap.fp = fopen(file, "a");
    if (cap.fp == NULL) {
        return CR_INVALID_ARGS;
    }
    CredoError_t err = cr_display_info(slice, argv, argc, (CredoDisplayWriter_t)cr_display_file, (void*)&cap);
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

bool cr_display_info_implemented(CredoSlice_t* slice) {
    return slice->hal->hal_display_info != NULL;
}
