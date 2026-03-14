// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2019,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "log.h"

#ifdef PCIESVC_SYSTEM_EXTERN

static void
logv(pciesvc_logpri_t pri, const char *fmt, va_list ap)
{
    pciesvc_eventdata_t evd;
    pciesvc_logmsg_t *logmsg;
    char buf[80];

    if (pri < pciesvc_log_level) {
        return;
    }

    pciesvc_vsnprintf(buf, sizeof(buf), fmt, ap);

    pciesvc_memset(&evd, 0, sizeof(evd));
    evd.evtype = PCIESVC_EV_LOGMSG;
    logmsg = &evd.logmsg;
    logmsg->pri = pri;
    pciesvc_memcpy(logmsg->msg, buf, sizeof(logmsg->msg));
    pciesvc_event_handler(&evd, sizeof(evd));
}

void
pciesvc_logdebug(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    logv(PCIESVC_LOGPRI_DEBUG, fmt, ap);
    va_end(ap);
}

void
pciesvc_loginfo(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    logv(PCIESVC_LOGPRI_INFO, fmt, ap);
    va_end(ap);
}

void
pciesvc_logwarn(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    logv(PCIESVC_LOGPRI_WARN, fmt, ap);
    va_end(ap);
}

void
pciesvc_logerror(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    logv(PCIESVC_LOGPRI_ERROR, fmt, ap);
    va_end(ap);
}

void
pciesvc_loglocal(const char *fmt, ...)
{
    char buf[80];
    va_list ap;

    va_start(ap, fmt);
    pciesvc_vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    pciesvc_log(buf);
}

#endif
