/*! \file sal_libc.h
 *
 * STandard C functions.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifndef SAL_LIBC_H
#define SAL_LIBC_H

#include <sal/sal_types.h>

/* System MUST supply stdarg.h */
#ifndef SAL_LINUX
#include <stdarg.h>
#endif

/*!
 * \cond SAL_STDC
 *
 * Standard C library functions.
 */

#ifndef sal_memcmp
extern int sal_memcmp(const void *s1, const void *s2, size_t cnt);
#endif

#ifndef sal_memcpy
extern void *sal_memcpy(void *dest, const void *src, size_t cnt);
#endif

#ifndef sal_memset
extern void *sal_memset(void *dest, int c, size_t cnt);
#endif

#ifndef sal_memmove
extern void *sal_memmove(void *dest, const void *src, size_t cnt);
#endif

#ifndef sal_strcpy
extern char *sal_strcpy(char *dest, const char *src);
#endif

#ifndef sal_strncpy
extern char *sal_strncpy(char *dest, const char *src, size_t cnt);
#endif

#ifndef sal_strlen
extern size_t sal_strlen(const char *str);
#endif

#ifndef sal_strcmp
extern int sal_strcmp(const char *dest, const char *src);
#endif

#ifndef sal_strncmp
extern int sal_strncmp(const char *dest, const char *src, size_t cnt);
#endif

#ifndef sal_strchr
extern char *sal_strchr(const char *dest, int c);
#endif

#ifndef sal_strrchr
extern char *sal_strrchr(const char *dest, int c);
#endif

#ifndef sal_strstr
extern char *sal_strstr(const char *dest, const char *src);
#endif

#ifndef sal_strcat
extern char *sal_strcat(char *dest, const char *src);
#endif

#ifndef sal_strncat
extern char *sal_strncat(char *dest, const char *src, size_t cnt);
#endif

/* ANSI/ISO ctype.h */

#ifndef sal_tolower
extern char sal_tolower(char c);
#endif

#ifndef sal_toupper
extern char sal_toupper(char c);
#endif

#ifndef sal_isspace
extern int sal_isspace(int c);
#endif

#ifndef sal_isupper
extern int sal_isupper(int c);
#endif

#ifndef sal_islower
extern int sal_islower(int c);
#endif

#ifndef sal_isalpha
extern int sal_isalpha(int c);
#endif

#ifndef sal_isdigit
extern int sal_isdigit(int c);
#endif

#ifndef sal_isalnum
extern int sal_isalnum(int c);
#endif

#ifndef sal_isxdigit
extern int sal_isxdigit(int c);
#endif

/* ANSI/ISO stdlib.h */

#ifndef sal_strtol
extern long sal_strtol(const char *s, char **end, int base);
#endif

#ifndef sal_strtoul
extern unsigned long sal_strtoul(const char *s, char **end, int base);
#endif

#ifndef sal_strtoll
extern long long sal_strtoll(const char *s, char **end, int base);
#endif

#ifndef sal_strtoull
extern unsigned long long sal_strtoull(const char *s, char **end, int base);
#endif

#ifndef sal_atoi
extern int sal_atoi(const char *s);
#endif

#ifndef sal_abs
extern int sal_abs(int j);
#endif

#ifndef RAND_MAX
#define RAND_MAX 32767
#endif

#ifndef sal_rand
extern int sal_rand(void);
#endif

#ifndef sal_srand
extern void sal_srand(unsigned seed);
#endif

#ifndef sal_qsort
extern void sal_qsort(void *arr, size_t numel,
                      size_t elsz, int (*cmpfn)(const void *, const void *));
#endif

#ifndef sal_bsearch
extern void *sal_bsearch(const void *el, const void *arr, size_t numel,
                         size_t elsz, int (*cmpfn)(const void *, const void *));
#endif

/* ANSI/ISO stdio.h */

#ifndef sal_vsnprintf
extern int sal_vsnprintf(char *buf, size_t bufsz, const char *fmt, va_list ap);
#endif

#ifndef sal_vsprintf
extern int sal_vsprintf(char *buf, const char *fmt, va_list ap);
#endif

#ifndef sal_snprintf
extern int sal_snprintf(char *buf, size_t bufsz, const char *fmt, ...);
#endif

#ifndef sal_sprintf
extern int sal_sprintf(char *buf, const char *fmt, ...);
#endif


/* Non-standard ANSI/ISO functions */

#ifndef sal_strcasecmp
extern int sal_strcasecmp(const char *dest, const char *src);
#endif

#ifndef sal_strncasecmp
extern int sal_strncasecmp(const char *dest, const char *src, size_t cnt);
#endif

#ifndef sal_strlcpy
extern size_t sal_strlcpy(char *dest, const char *src, size_t cnt);
#endif

#ifndef sal_strupr
extern void sal_strupr(char *s);
#endif

#ifndef sal_strlwr
extern void sal_strlwr(char *s);
#endif

#ifndef sal_strnchr
extern char *sal_strnchr(const char *dest, int c, size_t cnt);
#endif

#ifndef sal_strtok_r
extern char *sal_strtok_r(char *s1, const char *delim, char **s2);
#endif

#ifndef sal_strcasestr
extern char *sal_strcasestr(const char *dest, const char *src);
#endif

/*!
 * End of standard C library functions.
 *
 * \endcond
 */

/* Special SAL library functions */

#ifndef sal_ctoi
/*!
 * \brief Convert a string to an int type.
 *
 * Similar to atoi, but in addition to 0x it also reconizes prefix 0b
 * for binary numbers (e.g. 0b10010) and 0 for octal numbers
 * (e.g. 0347).
 *
 * If not NULL, the \c end pointer will be updated with the address of
 * the first invalid character in the string. The functionality of \c
 * end is similar to that of standard C \c strtol.
 *
 * \param [in] s Input string,
 * \param [out] end Pointer to first invalid character in \c s.
 *
 * \return Parsed integer value of input string.
 */
extern int sal_ctoi(const char *s, char **end);
#endif


/*! Internal marker used by sal_vsnprintf. */
#ifndef SAL_VSNPRINTF_X_INF
#define SAL_VSNPRINTF_X_INF     0x7ff0
#endif

#endif /* SAL_LIBC_H */
