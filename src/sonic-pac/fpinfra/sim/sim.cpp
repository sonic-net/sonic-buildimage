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

#include <time.h>
#include "fpnim.h"

extern "C" {
#include "datatypes.h"
#include "system_exports.h"
#include "simapi.h"
#include "osapi.h"
}

/*********************************************************************
* @purpose  Get the Unit's System Mac Address Type
*
* @param    none
*
* @returns  sysMacType   System Mac Address Type
*
* @comments
*
* @end
*********************************************************************/
uint32 simGetSystemIPMacType(void)
{
  return( SYSMAC_BIA);
}

/*********************************************************************
* @purpose  Get the Unit's System Burned in Mac Address
*
* @param    *sysBIA   @b{(output)} pointer to system burned in mac address
*                              Length  MAC_ADDR_LEN
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
void simGetSystemIPBurnedInMac( uchar8 *sysBIA)
{
  FpNim * nim = FpNim::getInstance();
  if(nim)
  {
    nim->getSystemMac(sysBIA);
  }
}


/*********************************************************************
* @purpose  Get the Unit's System Locally Administered Mac Address
*
* @param    *sysLAA   @b{(output)} pointer to system local admin mac address
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
void simGetSystemIPLocalAdminMac( uchar8 *sysLAA)
{
  simGetSystemIPBurnedInMac(sysLAA);
}

/*********************************************************************
*
* @purpose  Get the stack up time
*
* @param    none
*
* @returns  stack up time in seconds
*
* @comments  Stack up time is the time since the stack performed a cold
*            restart. Stack up time does not reset on a warm restart.
*
* @end
*
*********************************************************************/
uint32 simSystemUpTimeGet(void)
{
  return osapiUpTimeRaw();
}

/**********************************************************************
* @purpose  Adjusts current time for timezone and summer time
*
* @returns  Adjusted time
*
*
* @end
*********************************************************************/
uint32 simAdjustedTimeGet()
{
  struct timespec tp;
  int rc;

  rc = clock_gettime(CLOCK_REALTIME, &tp);
  if (rc < 0)
  {
      return 0;
  }
  return(tp.tv_sec);
}



