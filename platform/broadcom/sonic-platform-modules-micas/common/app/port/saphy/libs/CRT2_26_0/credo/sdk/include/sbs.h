#ifndef __SBS_H
#define __SBS_H

#include <stdarg.h>
#include <stdint.h>
#include <sys/types.h>

typedef struct {
    char *str;
    size_t len;
    size_t size;
} sbs;

// Properties
static inline char *sbsstr(sbs *s) {
    return s->str;
}
static inline size_t sbslen(const sbs *s) {
    return s->len;
}

static inline size_t sbssize(const sbs *s) {
    return s->size;
}
static inline size_t sbsavail(const sbs *s) {
    return s->size - s->len;
}
static inline char *sbsend(const sbs *s) {
    return s->str + s->len;
}

// Initialization
sbs *sbsnewlen(sbs *s, const void *init, size_t initlen, char buffer[], size_t buffer_size);
#define SBSNEWLEN(init, initlen, size) sbsnewlen(&(sbs){0}, init, initlen, (char[size]){0}, size)
sbs *sbsnew(sbs *s, const char *init, char buffer[], size_t buffer_size);
#define SBSNEW(init, size) sbsnew(&(sbs){0}, init, (char[size]){0}, size)
sbs *sbsempty(sbs *s, char buffer[], size_t buffer_size);
#define SBSEMPTY(size) sbsempty(&(sbs){0}, (char[size]){0}, size)
sbs *sbsdup(sbs *s, sbs *d, char buffer[], size_t buffer_size);
#define SBSDUP(s, size) sbsdup(s, &(sbs){0}, (char[size]){0}, size)
sbs *sbsfromlonglong(sbs *s, char buffer[64], long long value);
#define SBSFROMLL(value) sbsfromlonglong(&(sbs){0}, (char[64]){0}, value)

// add some convenience creators
#define SBS64(init)   SBSNEW(init, 128)
#define SBS128(init)  SBSNEW(init, 128)
#define SBS256(init)  SBSNEW(init, 256)
#define SBS512(init)  SBSNEW(init, 512)
#define SBS1024(init) SBSNEW(init, 1024)
#define SBS2048(init) SBSNEW(init, 2048)

// fill
int sbscatlen(sbs *s, const void *t, size_t len);
int sbscat(sbs *s, const char *t);
int sbscatsbs(sbs *s, const sbs *t);
int sbscpylen(sbs *s, const char *t, size_t len);
int sbscpy(sbs *s, const char *t);

// format
int sbscatvprintf(sbs *s, const char *fmt, va_list ap);
#ifdef __GNUC__
int sbscatprintf(sbs *s, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
#else
int sbscatprintf(sbs *s, const char *fmt, ...);
#endif
int sbscatfmt(sbs *s, char const *fmt, ...);
int sbscatrepr(sbs *s, const char *p, size_t len);

// modify
void sbstrim(sbs *s, const char *cset);
void sbsrange(sbs *s, ssize_t start, ssize_t end);
void sbsclear(sbs *s);
int sbscmp(const sbs *s1, const sbs *s2);
void sbstolower(sbs *s);
void sbstoupper(sbs *s);
void sbsmapchars(sbs *s, const char *from, const char *to, size_t setlen);

// join
int sbsjoin(sbs *s, const char **argv, int argc, const char *sep);
int sbsjoinsbs(sbs *s, const sbs argv[], int argc, const char *sep, size_t seplen);

// escape hatch
int sbsresize(sbs *s, char buffer[], size_t buffer_size);
#define SBSRESIZE(s, size) sbsresize(s, (char[size]){}, size)
static inline void sbssetlen(sbs *s, size_t len) {
    if (s->len >= s->size) return;
    s->len = len;
    s->str[s->len] = '\0';  // null terminate in case
}
void sbsupdatelen(sbs *s);

#endif
