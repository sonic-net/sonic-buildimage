#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import sys
    import time
    from . import component
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
    from sonic_platform_base.sfp_base import SfpBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class _CopperSfpStub(SfpBase):
    """Stub SFP object for copper/non-transceiver ports.
    Returns False/None for all queries so xcvrd handles them gracefully."""
    def get_presence(self):
        return False
    def get_transceiver_info(self):
        return None
    def get_transceiver_bulk_status(self):
        return None
    def is_replaceable(self):
        return False
    def get_error_description(self):
        return None


_COPPER_SFP_STUB = _CopperSfpStub()

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """
    sfp_status = {}

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

        for port in range(1, self.platform_inventory['num_ports'] + 1):
            self.sfp_status[port] = self.get_sfp(port).get_presence()

        for index in range(self.platform_inventory['num_component']):
            component_obj = component.Component(index)
            self._component_list.append(component_obj)

    # Provide the functions/variables below for which implementation is to be overwritten

    @staticmethod
    def initizalize_system_led():
        return True

    def get_status_led(self):
        # SYS LED is driven by GPIO (MPP25, active-low), not via I2C/CPLD.
        # No software-readable register is available; return green as default.
        return self.STATUS_LED_COLOR_GREEN

    def set_status_led(self, color):
        # SYS LED is hardware-controlled; software writes are not supported.
        return False

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>

        Physical SFP ports on ln3848g are 49-54; PDDF numbers them 1-6.
        Copper ports (indices 1-48) return a stub that reports not-present
        so xcvrd handles them without crashing.
        """
        # Translate physical port index (49-54) to PDDF SFP list index (1-6)
        SFP_PORT_START = 49
        if index >= SFP_PORT_START:
            index = index - SFP_PORT_START + 1
            try:
                return self._sfp_list[index - 1]
            except IndexError:
                return _COPPER_SFP_STUB

        # Copper port — return stub so callers get get_presence()=False
        return _COPPER_SFP_STUB

    def get_change_event(self, timeout=0):
        sfp_dict = {}
        start_time = time.time()
        time_period = timeout / float(1000)

        while time.time() < (start_time + time_period) or timeout == 0:
            for port in range(1, self.platform_inventory['num_ports'] + 1):
                exist = self.get_sfp(port).get_presence()
                if self.sfp_status[port] != exist:
                    self.sfp_status[port] = exist
                    sfp_dict[port] = '1' if exist else '0'

            if sfp_dict:
                return True, {'sfp': sfp_dict}

            time.sleep(1)

        return True, {'sfp': {}}  # Timeout

    def get_reboot_cause(self):
        # ln3848g does not have a hardware reboot cause register accessible
        # at this time. Return NON_HARDWARE as default.
        return (self.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_watchdog(self):
        """
        Retrieves hardware watchdog device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device
        """
        try:
            if self._watchdog is None:
                from sonic_platform.watchdog import WatchdogImplBase
                watchdog_device_path = "/dev/watchdog0"
                self._watchdog = WatchdogImplBase(watchdog_device_path)
        except Exception as e:
            from sonic_py_common import logger
            logger.Logger("chassis").log_warning(
                " Fail to load watchdog {}".format(repr(e)))

        return self._watchdog
