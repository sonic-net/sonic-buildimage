# pylint: disable=unused-import
try:
   from arista.utils.sonic_ssd import SsdUtil
except ImportError:
   from sonic_platform_base.sonic_storage.ssd import SsdUtil
