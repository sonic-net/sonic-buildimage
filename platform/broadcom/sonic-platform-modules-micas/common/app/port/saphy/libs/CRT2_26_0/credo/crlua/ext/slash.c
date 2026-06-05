#include "crlua.h"

#include "credo.h"
#include "sdk.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

CredoError_t cr_slash_command(CredoSlice_t *slices[], size_t slice_count, const char *command,
                              CredoShellWriter_t writer, void *ud) {
    if (slice_count == 0) return CR_INVALID_ARGS;

    if (slice_count > 1) {
        // ensure slices are of the same family
        CredoFamily_t shared_family;
        CredoError_t err = cr_slice_get_family(slices[0], &shared_family);
        if (err != CR_OK) {
            return err;
        }
        for (size_t i = 1; i < slice_count; i++) {
            slices[i]->in_shell = true;
            CredoFamily_t slice_family;
            err = cr_slice_get_family(slices[i], &slice_family);
            if (err != CR_OK) {
                return err;
            }
            // must be the same family
            if (slice_family != shared_family) {
                return CR_INVALID_ARGS;
            }
        }
    }

    // we are using the slice for a command so we can capture
    for (size_t i = 0; i < slice_count; i++) {
        slices[i]->in_shell = true;
    }
    CredoError_t err = crlua_run_command(slices, slice_count, command, writer, ud);
    for (size_t i = 0; i < slice_count; i++) {
        slices[i]->in_shell = false;
    }
    return (err == 0) ? CR_OK : CR_FAIL;
}

typedef struct {
    FILE *fp;
    int err;
} CredoCaptureFile_t;

static void cr_capture_file(CredoCaptureFile_t *cap, CredoShellStream_t stream, const char *message) {
    (void)(stream);
    if (cap->err != 0) return;
    int rc = fprintf(cap->fp, "%s", message);
    if (rc < 0) {
        cap->err = errno;
    }
}

CRLUAAPI CredoError_t cr_slash_command_file(CredoSlice_t *slices[], size_t slice_count, const char *command,
                                            const char *file) {
    CredoCaptureFile_t cap = {.err = 0};
    cap.fp = fopen(file, "a");
    if (cap.fp == NULL) {
        return CR_INVALID_ARGS;
    }
    CredoError_t err =
        cr_slash_command(slices, slice_count, command, (CredoShellWriter_t)cr_capture_file, (void *)&cap);
    fclose(cap.fp);
    if (err != CR_OK) {
        return err;
    }
    if (cap.err != 0) {
        return CR_FAIL;
    }
    return err;
}

typedef struct {
    char *output;
    size_t len;
    size_t size;
} CredoCaptureStr_t;

static void cr_capture_buffer(CredoCaptureStr_t *capture, CredoShellStream_t stream, const char *message) {
    (void)(stream);
    if (capture->output == NULL) return;  // skip if it has already failed
    size_t available = capture->size - capture->len;
    // have non-error path only compute once
    int size = snprintf(capture->output + capture->len, available, "%s", message);
    if (size < 0) {
        goto failed;
    }
    // if not enough space, realloc and rewrite the data
    if ((size_t)size >= available) {
        // double buffer size needed
        size_t new_buff_size = (capture->len + size) << 1;
        char *new_output = realloc(capture->output, new_buff_size);

        if (new_output == NULL) {
            goto failed;
        }
        capture->output = new_output;
        capture->size = new_buff_size;
        available = capture->size - capture->len;
        size = snprintf(capture->output + capture->len, available, "%s", message);
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

CredoError_t cr_slash_command_buffer(CredoSlice_t *slices[], size_t slice_count, const char *command, char **output) {
    CredoCaptureStr_t capture = {
        .output = malloc(CAPUTRE_BUFFER_START_SIZE), .size = CAPUTRE_BUFFER_START_SIZE, .len = 0};

    if (capture.output == NULL) {
        return CR_OUT_OF_MEMORY;
    }
    capture.output[0] = '\0';
    CredoError_t err =
        cr_slash_command(slices, slice_count, command, (CredoShellWriter_t)cr_capture_buffer, (void *)&capture);
    // not enough memory
    if (capture.output == NULL) {
        *output = NULL;
        return CR_OUT_OF_MEMORY;
    }
    *output = capture.output;
    return err;
}

CredoError_t cr_slash_command_log(CredoSlice_t *slices[], size_t slice_count, const char *command) {
    char *output = NULL;
    CredoError_t err = cr_slash_command_buffer(slices, slice_count, command, &output);

    if (output != NULL && output[0] != '\0') {
        cr_llog("lua", CR_LOG_INFO, output);
    }
    free(output);
    return err;
}
