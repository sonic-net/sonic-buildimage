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

TEMPERATURE_REGEX= {"FS032GMSI-AC" : ['194 Temperature_Celsius\s*(.+?)\n', 7]}
HEALTH_REGEX = {"FS032GMSI-AC" : ['169 Unknown_Attribute\s*(.+?)\n', -1]}

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
        if self.vendor_ssd_info == NOT_AVAILABLE:
            self.vendor_ssd_info = self.ssd_info

        self.temperature = self.__handle_attribute(self.temperature, TEMPERATURE_REGEX)
        self.health = self.__handle_attribute(self.health, HEALTH_REGEX)

    def __is_number(self, value):
        try:
            float(value)
            return True
        except ValueError:
            return False

    def __handle_attribute(self, attribute, regex_offset):
        try:
            regex, offset = regex_offset.get(self.model, [None, None])
            if regex is None or offset is None:
                return attribute

            attribute = NOT_AVAILABLE

            attribute_raw = self._parse_re(regex, self.ssd_info)
            if attribute_raw == NOT_AVAILABLE:
                return attribute

            split_raw = attribute_raw.split()
            if self.__is_number(split_raw[offset]):
                attribute = split_raw[offset]
        except Exception as e:
            log.log_warning(f"Warning : {e}")

        return attribute
