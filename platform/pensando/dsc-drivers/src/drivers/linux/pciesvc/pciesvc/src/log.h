/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021-2022, Pensando Systems Inc.
 */

#ifndef __LOG_H__
#define __LOG_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#ifdef PCIESVC_SYSTEM_EXTERN

void pciesvc_loglocal(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
void pciesvc_logdebug(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
void pciesvc_loginfo(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
void pciesvc_logwarn(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));
void pciesvc_logerror(const char *fmt, ...)
    __attribute__((format (printf, 1, 2)));

#endif

#ifdef __cplusplus
}
#endif

#endif /* __LOG_H__ */
