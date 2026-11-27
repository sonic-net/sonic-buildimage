#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <sys/timerfd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "libobmc_ipmi.h"

/* 15 billion Julian years is still a mighty long time. */
static const time_t obmc_15_billion_years = 15000000000ULL * 1461 * 6 * 3600;
static const char rtc_ts_trigger_file[]  = "/run/rtc-timestamp-trigger";

static int obmc_sync()
{
   int rc;

   rc = obmc_ipmi_sel_time_set_now();

   if (rc) fprintf(stderr, "obmc_ipmi_sel_time_set_now failed (%d/%s)\n",
                   rc, strerror(errno));

   return rc;
}

static void rtc_ts_update()
{
   FILE *fp;
   if (!(fp = fopen(rtc_ts_trigger_file, "w")))
      fprintf(stderr, "%s failed to open trigger file (%s)\n",
                   __func__, strerror(errno));
   else
      fclose(fp);
}

int main()
{
   int tflags = TFD_TIMER_CANCEL_ON_SET | TFD_TIMER_ABSTIME;
   int tfd;
   int rc;

   /* Start off with one gratuitous system time synch. */
   rc = obmc_sync();
   if (rc) {
      /* If the initial time synch fails, the BMC is adverse to time
       * updates. Just exit gracefully. */
      fprintf(stderr, "initial BMC time synch failed, bailing out\n");
      return 0;
   }

   tfd = timerfd_create(CLOCK_REALTIME, 0);
   if (0 > tfd) {
      rc = -errno;
      fprintf(stderr, "timerfd_create failed (%s)\n",
              strerror(errno));
      return rc;
   }

   while (1) {
      struct itimerspec  itspec = {};
      unsigned long long rbuf   = 0;

      itspec.it_value.tv_sec = time(NULL) + obmc_15_billion_years;

      rc = timerfd_settime(tfd, tflags, &itspec, NULL);
      if (rc) {
         rc = -errno;
         fprintf(stderr, "timerfd_settime failed (%s)\n",
                 strerror(errno));
         break;
      }

      rc = TEMP_FAILURE_RETRY(read(tfd, &rbuf, sizeof(rbuf)));

      if ((0 > rc) && (ECANCELED == errno)) {
         fprintf(stdout, "system clock change detected: informing BMC\n");
         rc = obmc_sync();
         if (rc) break;
         rtc_ts_update();
      }
      else {
         fprintf(stderr, "unexpected timer event (%llu/%s), bailing out\n",
                 (0 <= rc) ? rbuf : errno, strerror(errno));
         rc = -EINVAL;
         break;
      }
   }

   TEMP_FAILURE_RETRY(close(tfd));

   return rc;
}
