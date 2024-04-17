
import os

# pylint: disable=import-error
from sonic_platform_base.sonic_ssd.ssd_base import SsdBase
from sonic_platform_base.sonic_ssd.ssd_generic import SsdUtil as SsdUtilDefault

class EmmcUtil(SsdBase):
   def __init__(self, diskdev):
      self.diskdev = diskdev
      self.path = os.path.join('/sys/block', os.path.basename(diskdev))

   def _readDeviceEntry(self, entry, default=None):
      path = os.path.join(self.path, 'device', entry)
      try:
         with open(path, encoding='utf8') as f:
            return f.read().rstrip()
      except OSError:
         return default

   def _isSlc(self):
      return bool(self._readDeviceEntry('enhanced_area_offset'))

   def get_health(self):
      data = self._readDeviceEntry('life_time')
      if data is None:
         raise NotImplementedError
      value = int(data.split()[0 if self._isSlc() else 1], 0)
      return float(100 - (10 * (value - 1)))

   def get_temperature(self):
      return 'N/A'

   def get_model(self):
      return self._readDeviceEntry('name')

   def get_firmware(self):
      return self._readDeviceEntry('fwrev')

   def get_serial(self):
      return self._readDeviceEntry('serial')

   def get_vendor_output(self):
      return ''

def SsdUtil(diskdev):
   return EmmcUtil('/dev/mmcblk0')
