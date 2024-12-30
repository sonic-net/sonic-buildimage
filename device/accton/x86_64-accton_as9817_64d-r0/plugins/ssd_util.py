# ssd_util.py
#
# Platform-specific SSD interface for SONiC
##

try:
    from sonic_platform_base.sonic_ssd.ssd_generic import SsdUtil as MainSsdUtil
    from sonic_platform_base.sonic_ssd.ssd_generic import NOT_AVAILABLE
    from sonic_py_common import logger
except ImportError as e:
    raise ImportError (str(e) + "- required module not found")

SYSLOG_IDENTIFIER = "ssd_util.py"

# Global logger instance
log = logger.Logger(SYSLOG_IDENTIFIER)

class SsdUtil(MainSsdUtil):
    """Platform-specific SsdUtil class"""

    def __init__(self, diskdev):
        super(SsdUtil, self).__init__(diskdev)

        # If it has no vendor tool to read SSD information,
        # ssd_util.py will use generic SSD information
        # for vendor SSD information.
        if self.vendor_ssd_info == NOT_AVAILABLE or \
                len(self.vendor_ssd_info.strip()) == 0:
            self.vendor_ssd_info = self.ssd_info
