//
// jWrite.c		version 1v2
//
// A *really* simple JSON writer in C
//
// see: jWrite.h for info
//
// TonyWilk, Mar 2015
//

#include "jwrite.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>  // definintion of uint32_t, int32_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // memset()

// the jWrite functions take the above jWriteControl structure pointer
// to maintain state while writing a JSON string.
//
// You can opt to use a single global instance of a jWriteControl structure
// which simplifies the function parameters or to supply your own structure
//

//------------------------------------------
// Internal functions
//
void jwPutch(jw_t *jwc, char c);
void jwPutstr(jw_t *jwc, const char *str);
void jwPutraw(jw_t *jwc, const char *str);
void jwPutstrf(jw_t *jwc, const char *fmt, va_list ap);
void jwPutstrf_raw(jw_t *jwc, const char *fmt, va_list ap);
void jwPretty(jw_t *jwc);
enum jwNodeType jwPop(jw_t *jwc);
void jwPush(jw_t *jwc, enum jwNodeType nodeType);

//------------------------------------------
// jwOpen
// - open writing of JSON starting with rootType = JW_OBJECT or JW_ARRAY
// - initialise with user string buffer of length buflen
// - isPretty=JW_PRETTY adds \n and spaces to prettify output (else JW_COMPACT)
//
jw_t *jwOpen(jw_t *jwc, size_t buflen, enum jwNodeType rootType, int isPretty) {
    jwc->buffer = malloc(buflen);
    jwc->buflen = buflen;
    jwc->bufp = jwc->buffer;
    jwc->nodeStack[0].nodeType = rootType;
    jwc->nodeStack[0].elementNo = 0;
    jwc->stackpos = 0;
    jwc->error = (jwc->buffer != NULL) ? JWRITE_OK : JWRITE_OUT_OF_MEM;
    jwc->callNo = 1;
    jwc->isPretty = isPretty;
    jwc->inStr = false;
    jwc->inOpt = false;
    jwPutch(jwc, (rootType == JW_OBJECT) ? '{' : '[');
    return jwc;
}

//------------------------------------------
// jwClose
// - closes the root JSON object started by jwOpen()
// - returns error code
//
int jwClose(jw_t *jwc) {
    if (jwc->error == JWRITE_OK) {
        if (jwc->stackpos == 0) {
            enum jwNodeType node = jwc->nodeStack[0].nodeType;
            if (jwc->isPretty) jwPutch(jwc, '\n');
            jwPutch(jwc, (node == JW_OBJECT) ? '}' : ']');
        } else {
            jwc->error = JWRITE_NEST_ERROR;  // nesting error, not all objects closed when jwClose() called
        }
    }
    return jwc->error;
}

void jwFree(jw_t *jwc) {
    free(jwc->buffer);
    jwc->buffer = NULL;
    jwc->bufp = NULL;
}

//------------------------------------------
// End the current array/object
//
int jwEnd(jw_t *jwc) {
    if (jwc->error == JWRITE_OK) {
        enum jwNodeType node;
        int lastElemNo = jwc->nodeStack[jwc->stackpos].elementNo;
        node = jwPop(jwc);
        if (lastElemNo > 0) jwPretty(jwc);
        jwPutch(jwc, (node == JW_OBJECT) ? '}' : ']');
    }
    return jwc->error;
}

//------------------------------------------
// jwErrorPos
// - Returns position of error: the nth call to a jWrite function
//
int jwErrorPos(jw_t *jwc) {
    return jwc->callNo;
}

//------------------------------------------
// Object insert functions
//
int jwObj(jw_t *jwc, const char *key);

// put raw string to object (i.e. contents of rawtext without quotes)
//
void jwObj_raw(jw_t *jwc, const char *key, const char *rawtext) {
    if (jwObj(jwc, key) == JWRITE_OK) jwPutraw(jwc, rawtext);
}

// put "quoted" string to object
//
void jwObj_string(jw_t *jwc, const char *key, const char *value) {
    if (jwc->inOpt) {
        jwObj_null(jwc, key);
        return;
    }
    if (jwObj(jwc, key) == JWRITE_OK) jwPutstr(jwc, value);
}

void jwObj_number(jw_t *jwc, const char *key, const char *fmt, ...) {
    if (jwc->inOpt) {
        jwObj_null(jwc, key);
        return;
    }
    if (jwObj(jwc, key) == JWRITE_OK) {
        va_list ap;
        va_start(ap, fmt);
        jwPutstrf_raw(jwc, fmt, ap);
        va_end(ap);
    }
}

void jwObj_stringf(jw_t *jwc, const char *key, const char *fmt, ...) {
    if (jwc->inOpt) {
        jwObj_null(jwc, key);
        return;
    }
    if (jwObj(jwc, key) == JWRITE_OK) {
        va_list ap;
        va_start(ap, fmt);
        jwPutstrf(jwc, fmt, ap);
        va_end(ap);
    }
}

void jwObj_bool(jw_t *jwc, const char *key, bool oneOrZero) {
    if (jwc->inOpt) {
        jwObj_null(jwc, key);
        return;
    }
    jwObj_raw(jwc, key, (oneOrZero) ? "true" : "false");
}

void jwObj_null(jw_t *jwc, const char *key) {
    jwObj_raw(jwc, key, "null");
}

// put Object in Object
//
void jwObj_object(jw_t *jwc, const char *key) {
    if (jwObj(jwc, key) == JWRITE_OK) {
        jwPutch(jwc, '{');
        jwPush(jwc, JW_OBJECT);
    }
}

// put Array in Object
//
void jwObj_array(jw_t *jwc, const char *key) {
    if (jwObj(jwc, key) == JWRITE_OK) {
        jwPutch(jwc, '[');
        jwPush(jwc, JW_ARRAY);
    }
}

//------------------------------------------
// Array insert functions
//
int jwArr(jw_t *jwc);

// put raw string to array (i.e. contents of rawtext without quotes)
//
void jwArr_raw(jw_t *jwc, const char *rawtext) {
    if (jwArr(jwc) == JWRITE_OK) jwPutraw(jwc, rawtext);
}

// put "quoted" string to array
//
void jwArr_string(jw_t *jwc, const char *value) {
    if (jwArr(jwc) == JWRITE_OK) jwPutstr(jwc, value);
}

void jwArr_number(jw_t *jwc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (jwArr(jwc) == JWRITE_OK) jwPutstrf_raw(jwc, fmt, ap);
    va_end(ap);
}

void jwArr_stringf(jw_t *jwc, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    if (jwArr(jwc) == JWRITE_OK) jwPutstrf(jwc, fmt, ap);
    va_end(ap);
}

void jwArr_bool(jw_t *jwc, int oneOrZero) {
    jwArr_raw(jwc, (oneOrZero) ? "true" : "false");
}

void jwArr_null(jw_t *jwc) {
    jwArr_raw(jwc, "null");
}

void jwArr_object(jw_t *jwc) {
    if (jwArr(jwc) == JWRITE_OK) {
        jwPutch(jwc, '{');
        jwPush(jwc, JW_OBJECT);
    }
}

void jwArr_array(jw_t *jwc) {
    if (jwArr(jwc) == JWRITE_OK) {
        jwPutch(jwc, '[');
        jwPush(jwc, JW_ARRAY);
    }
}

//------------------------------------------
// jwErrorToString
// - returns string describing error code
//
const char *jwErrorToString(int err) {
    switch (err) {
        case JWRITE_OK:
            return "OK";
        case JWRITE_OUT_OF_MEM:
            return "out of memory to build json";
        case JWRITE_NOT_ARRAY:
            return "tried to write Array value into Object";
        case JWRITE_NOT_OBJECT:
            return "tried to write Object key/value into Array";
        case JWRITE_STACK_FULL:
            return "array/object nesting > JWRITE_STACK_DEPTH";
        case JWRITE_STACK_EMPTY:
            return "stack underflow error (too many 'end's)";
        case JWRITE_NEST_ERROR:
            return "nesting error, not all objects closed when jwClose() called";
        case JWRITE_BAD_FORMAT:
            return "bad formatting for value";
        default:
            return "Unknown error";
    }
}

//============================================================================
// Internal functions
//
void jwPretty(jw_t *jwc) {
    int i;
    if (jwc->isPretty) {
        jwPutch(jwc, '\n');
        for (i = 0; i < jwc->stackpos + 1; i++) jwPutraw(jwc, "    ");
    }
}

// Push / Pop node stack
//
void jwPush(jw_t *jwc, enum jwNodeType nodeType) {
    if ((jwc->stackpos + 1) >= JWRITE_STACK_DEPTH)
        jwc->error = JWRITE_STACK_FULL;  // array/object nesting > JWRITE_STACK_DEPTH
    else {
        jwc->nodeStack[++jwc->stackpos].nodeType = nodeType;
        jwc->nodeStack[jwc->stackpos].elementNo = 0;
    }
}

enum jwNodeType jwPop(jw_t *jwc) {
    enum jwNodeType retval = jwc->nodeStack[jwc->stackpos].nodeType;
    if (jwc->stackpos == 0)
        jwc->error = JWRITE_STACK_EMPTY;  // stack underflow error (too many 'end's)
    else
        jwc->stackpos--;
    return retval;
}

void jwPutchb(jw_t *jwc, char c) {
    if (jwc->error != JWRITE_OK) {
        return;
    }
    size_t bufsize = (jwc->bufp - jwc->buffer);
    if (bufsize >= jwc->buflen - 1) {
        char *newbuf = realloc(jwc->buffer, jwc->buflen * 2);
        if (newbuf == NULL) {
            jwc->error = JWRITE_OUT_OF_MEM;
            return;
        }
        jwc->bufp = newbuf + (jwc->bufp - jwc->buffer);
        jwc->buffer = newbuf;
        jwc->buflen = jwc->buflen * 2;
    }
    *jwc->bufp++ = c;
    *jwc->bufp = '\0';
}

char escape_map[256] = {
    ['\\'] = '\\', ['\"'] = '\"', ['\b'] = 'b', ['\f'] = 'f', ['\n'] = 'n', ['\r'] = 'r', ['\t'] = 't'};

void jwPutch(jw_t *jwc, char c) {
    // escape special characters
    if (jwc->inStr && escape_map[(unsigned char)c] != 0) {
        jwPutchb(jwc, '\\');
        jwPutchb(jwc, escape_map[(unsigned char)c]);
        return;
    }
    jwPutchb(jwc, c);
}

#define TMPBUF_SIZE 256

void jwPutstrf(jw_t *jwc, const char *fmt, va_list ap) {
    jwPutch(jwc, '\"');
    jwc->inStr = true;
    jwPutstrf_raw(jwc, fmt, ap);
    jwc->inStr = false;
    jwPutch(jwc, '\"');
}

void jwPutstrf_raw(jw_t *jwc, const char *fmt, va_list ap) {
    if (jwc->error != JWRITE_OK) {
        return;
    }
    char tmpbuf[TMPBUF_SIZE];
    char *outbuf = tmpbuf;
    char *heapbuf = NULL;
    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(tmpbuf, TMPBUF_SIZE, fmt, ap);
    if (n < 0) {
        jwc->error = JWRITE_BAD_FORMAT;
        va_end(ap2);
        return;
    }
    if (n >= TMPBUF_SIZE) {
        heapbuf = malloc(n + 1);
        if (heapbuf == NULL) {
            jwc->error = JWRITE_OUT_OF_MEM;
            va_end(ap2);
            return;
        }
        int n2 = vsnprintf(heapbuf, n + 1, fmt, ap);
        if (n2 < 0 || n2 >= n + 1) {
            jwc->error = JWRITE_BAD_FORMAT;
            va_end(ap2);
            free(heapbuf);
            return;
        }
        outbuf = heapbuf;
    }
    jwPutraw(jwc, outbuf);
    free(heapbuf);
    va_end(ap2);
}

// put string enclosed in quotes
//
void jwPutstr(jw_t *jwc, const char *str) {
    jwPutch(jwc, '\"');
    jwc->inStr = true;
    while (*str != '\0') jwPutch(jwc, *str++);
    jwc->inStr = false;
    jwPutch(jwc, '\"');
}

// put raw string
//
void jwPutraw(jw_t *jwc, const char *str) {
    while (*str != '\0') jwPutch(jwc, *str++);
}

// *common Object function*
// - checks error
// - checks current node is OBJECT
// - adds comma if reqd
// - adds "key" :
//
int jwObj(jw_t *jwc, const char *key) {
    if (jwc->error == JWRITE_OK) {
        jwc->callNo++;
        if (jwc->nodeStack[jwc->stackpos].nodeType != JW_OBJECT)
            jwc->error = JWRITE_NOT_OBJECT;  // tried to write Object key/value into Array
        else if (jwc->nodeStack[jwc->stackpos].elementNo++ > 0)
            jwPutch(jwc, ',');
        jwPretty(jwc);
        jwPutstr(jwc, key);
        jwPutch(jwc, ':');
        if (jwc->isPretty) jwPutch(jwc, ' ');
    }
    return jwc->error;
}

// *common Array function*
// - checks error
// - checks current node is ARRAY
// - adds comma if reqd
//
int jwArr(jw_t *jwc) {
    if (jwc->error == JWRITE_OK) {
        jwc->callNo++;
        if (jwc->nodeStack[jwc->stackpos].nodeType != JW_ARRAY)
            jwc->error = JWRITE_NOT_ARRAY;  // tried to write array value into Object
        else if (jwc->nodeStack[jwc->stackpos].elementNo++ > 0)
            jwPutch(jwc, ',');
        jwPretty(jwc);
    }
    return jwc->error;
}

/* end of jWrite.c */
