// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018, Pensando Systems Inc.
 */

#include "pciesvc_system.h"

#ifdef PCIESVC_SYSTEM_EXTERN
struct ostr_s {
    void (*cb)(int c, void *arg);
    void *arg;
};

static inline void
prf_putc(const struct ostr_s *o, int c)
{
    o->cb(c, o->arg);
}

static void
prf_emit_u64(const struct ostr_s *o, uint64_t n,
        int base, int zeroes, int ptr, int neg, int width, int ljust)
{
    char buf[32];
    int i = 0;
    if (n == 0) {
        buf[i++] = '0';
    } else {
        while (n) {
            buf[i++] = "0123456789abcdef"[n % base];
            n /= base;
        }
    }
    if (ljust) {
        int rpad;
        if (neg) {
            prf_putc(o, '-');
            --width;
        } else if (ptr) {
            prf_putc(o, '0');
            prf_putc(o, 'x');
            width -= 2;
        }
        rpad = width - i;
        while (i > 0) {
            prf_putc(o, buf[--i]);
        }
        while (rpad-- > 0) {
            prf_putc(o, ' ');
        }
    } else {
        if (ptr) {
            width -= 2;
        } else if (neg) {
            --width;
            if (zeroes) {
               prf_putc(o, '-');
            }
        }
        while (i < width) {
            prf_putc(o, zeroes ? '0' : ' ');
            --width;
        }
        if (ptr) {
            prf_putc(o, '0');
            prf_putc(o, 'x');
        } else if (neg && !zeroes) {
            prf_putc(o, '-');
        }
        while (i > 0) {
            prf_putc(o, buf[--i]);
        }
    }
}

#define strlen _strlen
static size_t
strlen(const char *s)
{
    const char *e;
    for (e = s; *e; e++) {
        ;
    }
    return (size_t)(e - s);
}

static void
prf_emit_str(const struct ostr_s *o, const char *s, int width, int ljust)
{
    int c, nspc;

    if (s == NULL) {
        s = "<null>";
    }
    nspc = width - strlen(s);
    if (width > 0 && !ljust) {
        while (nspc-- > 0) {
            prf_putc(o, ' ');
        }
    }
    while ((c = *s++)) {
        prf_putc(o, c);
    }
    if (width > 0 && ljust) {
        while (nspc-- > 0) {
            prf_putc(o, ' ');
        }
    }
}

static void
subr_prf(const struct ostr_s *o, const char *s, va_list ap)
{
    int base, done, pop, is_long, zeroes, ptr, neg, sign, width, str, ljust;
    uint64_t n;
    char c;

    while ((c = *s++)) {
        switch (c) {
        case '%':
            done = pop = is_long = ptr = neg = sign = zeroes = str = ljust = 0;
            width = -1;
            base = 10;
            do {
                c = *s++;
                switch (c) {
                case '\0':
                    return;
                case '-':
                    ljust = 1;
                    break;
                case '0':
                    if (width < 0) {
                        zeroes = 1;
                        break;
                    }
                    width = (((width < 0) ? 0 : width) * 10) + c - '0';
                    break;
                case '1' ... '9':
                    width = (((width < 0) ? 0 : width) * 10) + c - '0';
                    break;
                case '%':
                    prf_putc(o, '%');
                    done = 1;
                    break;
                case 'l':
                    is_long = 1;
                    break;
                case 'p':
                    ptr = 1;
                    is_long = 1;
                    base = 16;
                    pop = 1;
                    done = 1;
                    break;
                case 'x':
                    base = 16;
                    pop = 1;
                    done = 1;
                    break;
                case 'd':
                    sign = 1;
                    pop = 1;
                    done = 1;
                    break;
                case 'u':
                    pop = 1;
                    done = 1;
                    break;
                case 's':
                    pop = 1;
                    done = 1;
                    str = 1;
                    break;
                case 'c':
                    prf_putc(o, va_arg(ap, int));
                    done = 1;
                    break;
                default:
                    prf_putc(o, c);
                    done = 1;
                }
            } while (!done);
            if (pop) {
                if (str) {
                    prf_emit_str(o, va_arg(ap, char *), width, ljust);
                } else {
                    n = is_long ? va_arg(ap, uint64_t) : va_arg(ap, uint32_t);
                    if (sign) {
                        if (is_long) {
                            neg = ((int64_t)n < 0);
                            if (neg) {
                                n = -n;
                            }
                        } else {
                            neg = ((int32_t)n < 0);
                            if (neg) {
                                n = -(int64_t)(int32_t)n;
                            }
                        }
                    }
                    prf_emit_u64(o, n, base, zeroes, ptr, neg, width, ljust);
                }
            }
            break;
        default:
            prf_putc(o, c);
            break;
        }
    }
}

struct snprintf_ctx {
    char *pos;
    size_t remain;
};

static void
snprintf_outchar(int c, void *arg)
{
    struct snprintf_ctx *ctx = arg;
    if (ctx->remain) {
        *ctx->pos++ = c;
        --ctx->remain;
    }
}

int
pciesvc_vsnprintf(char *buf, size_t len, const char *fmt, va_list ap)
{
#ifdef PCIESVC_SYSTEM_EXTERN
/*
 * Oracle environment wants runtime init of these structs to
 * use pc-relative offsets that are within the module and need
 * no relocation required when running in different environments.
 */
#define RUNTIME_INIT
#endif
#ifdef RUNTIME_INIT
    struct snprintf_ctx ctx;
    struct ostr_s o;

    ctx.pos = buf;
    ctx.remain = len;
    o.cb = snprintf_outchar;
    o.arg = &ctx;
#else
    struct snprintf_ctx ctx = {
        .pos = buf,
        .remain = len,
    };
    struct ostr_s o = {
        .cb = snprintf_outchar,
        .arg = &ctx
    };
#endif

    subr_prf(&o, fmt, ap);
    if (ctx.remain) {
        *ctx.pos = '\0';
    } else {
        *(ctx.pos - 1) = '\0';
    }
    return len - ctx.remain;
}

int
pciesvc_snprintf(char *buf, size_t len, const char *fmt, ...)
{
    int r;
    va_list ap;
    va_start(ap, fmt);
    r = pciesvc_vsnprintf(buf, len, fmt, ap);
    va_end(ap);
    return r;
}
#endif

#ifdef CONFIG_PRINTF
static void
printf_outchar(int c, void *arg)
{
    putchar(c);
}

void
pciesvc_vprintf(const char *fmt, va_list ap)
{
#ifdef RUNTIME_INIT
    const struct ostr_s o;
    o.cb = printf_outchar;
    o.arg = NULL;
#else
    const struct ostr_s o = {
        .cb = printf_outchar,
        .arg = NULL,
    };
#endif
    subr_prf(&o, fmt, ap);
}

void
printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    pciesvc_vprintf(fmt, ap);
    va_end(ap);
}
#endif
