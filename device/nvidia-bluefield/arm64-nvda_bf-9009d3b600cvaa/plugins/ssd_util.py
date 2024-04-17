
import os

from sonic_platform_base.sonic_ssd.ssd_emmc import EmmcUtil
from sonic_platform_base.sonic_ssd.ssd_generic import SsdUtil as SsdUtilDefault

def SsdUtil(diskdev):
   if os.path.basename(diskdev).startswith('mmcblk'):
      return EmmcUtil(diskdev)
   return SsdUtilDefault(diskdev)
