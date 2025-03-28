/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "util.h"

/*********************************************************************
* @purpose  To display the date in the format hr:mn:sec dd/mm/yyyy
* @param rawtime -time in seconds **argv
* @param buf - contains the date
* @author
*
* @cmddescript
*
* @end
*
*********************************************************************/
void getUtcTimeInMonthsAndDays(uint32 utcTime,  char8 *buf)
{
  time_t l_utcTime = utcTime;
  struct tm date, *datep;
   char8 mon_name[12][4] = {"Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0",
                              "Jun\0", "Jul\0", "Aug\0", "Sep\0", "Oct\0",
                              "Nov\0", "Dec\0" };

  datep = localtime_r(&l_utcTime, &date);
  sprintf(buf, "%s %.2d %d %.2d:%.2d:%.2d", mon_name[datep->tm_mon], datep->tm_mday, datep->tm_year + 1900,
          datep->tm_hour, datep->tm_min, datep->tm_sec);
}
