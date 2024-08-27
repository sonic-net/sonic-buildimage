#!/usr/bin/env python

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

try:
    import syslog
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

    # Provide the functions/variables below for which implementation is to be overwritten
    def get_sfp(self, index):
        """
        Retrieves sfp represented by (1-based) index <index>
        Args:
            index: An integer, the index (1-based) of the sfp to retrieve.
            The index should be the sequence of a physical port in a chassis,
            starting from 0.
            For example, 0 for Ethernet0, 1 for Ethernet4 and so on.
        Returns:
            An object derived from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            # The 'index' starts from 1 for this platform
            sfp = self._sfp_list[index-1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                             index, len(self._sfp_list)))
        return sfp

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        watchdog_dir = '/sys/class/watchdog'
        description = 'Hardware Watchdog Reset'
        hw_reboot_cause = ""
        import os
        if not os.path.exists(watchdog_dir):
            syslog.syslog(syslog.LOG_INFO, "Watchdog is not supported on this platform")
            return (self.REBOOT_CAUSE_NON_HARDWARE, 'Unknown reason')

        for item in os.listdir(watchdog_dir):
            path = os.path.join(watchdog_dir, item)
            if os.path.isdir(path):
                bootstatus_file = os.path.join(path, "bootstatus")
                if os.path.exists(bootstatus_file):
                    try:
                        with open(bootstatus_file, "r") as f:
                            hw_reboot_cause = f.read().strip('\n')
                        if hw_reboot_cause == "32":
                            return (self.REBOOT_CAUSE_WATCHDOG, description)
                    except Exception as e:
                        raise Exception('Error while trying to find the HW reboot cause - {}'.format(str(e)))

                reboot_reason_file = os.path.join(path, "reboot_reason")
                if os.path.exists(reboot_reason_file):
                    try:
                        with open(reboot_reason_file, "r") as f:
                            hw_reboot_cause = f.read().strip('\n')
                        if hw_reboot_cause == "2":
                            return (self.REBOOT_CAUSE_WATCHDOG, description)
                    except Exception as e:
                        raise Exception('Error while trying to find the HW reboot cause - {}'.format(str(e)))

        syslog.syslog(syslog.LOG_INFO, "No watchdog reset detected")
        return (self.REBOOT_CAUSE_NON_HARDWARE, 'Unknown reason')

    def initizalize_system_led(self):
        return True

    def set_status_led(self, *args):
        if len(args) == 1:
            color = args[0]
        else:
            return False

        return super().set_status_led("SYS_LED", color)

    def get_status_led(self, *args):
        return super().get_status_led("SYS_LED")
