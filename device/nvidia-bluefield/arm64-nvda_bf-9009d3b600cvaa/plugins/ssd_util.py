
import os

from sonic_platform_base.sonic_storage.emmc import EmmcUtil
from sonic_platform_base.sonic_storage.ssd import SsdUtil as SsdUtilDefault

def SsdUtil(diskdev):
   if os.path.basename(diskdev).startswith('mmcblk'):
      return EmmcUtil(diskdev)
   return SsdUtilDefault(diskdev)
