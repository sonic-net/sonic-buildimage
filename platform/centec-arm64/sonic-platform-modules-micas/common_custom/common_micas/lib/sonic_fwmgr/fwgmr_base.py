# fwgmr_base.py
#
# Base class for creating platform-specific firmware management interfaces for SONiC
#
try:
    import abc
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

class FwMgrUtilBase(object):

    """Base class for Platform-specific FwMgrUtil class"""
    __metaclass__ = abc.ABCMeta

    def __init__(self):
        """TODO: to be defined1. """
        pass

    @abc.abstractmethod
    def get_bmc_version(self):
        """Get BMC version from SONiC
        :returns: version string
        """
        return

    @abc.abstractmethod
    def get_cpld_version(self):
        """Get CPLD version from SONiC
        :returns: dict like {'CPLD_1': version_string, 'CPLD_2': version_string}
        """
        return

    @abc.abstractmethod
    def get_onie_version(self):
        """Get ONiE version from SONiC
        :returns: version string
        """
        return

    @abc.abstractmethod
    def get_uboot_version(self):
        """Get UBOOT version from SONiC
        :returns: version string
        """
        return

    # @fw_type MANDATORY, firmware type, should be one of the strings: 'cpld', 'fpga', 'bios', 'bmc'
    # @fw_path MANDATORY, target firmware file
    # @fw_extra OPTIONAL, extra information string,
    #           for fw_type 'cpld' and 'fpga': it can be used to indicate specific cpld, such as 'cpld1', 'cpld2', ...
    #               or 'cpld_fan_come_board', etc. For fw_type 'bios' and 'bmc', value should be one of 'master'
    #               or 'slave' or 'both'. For BMC, 'pingpong' stands for alternative upgrade policy.
    @abc.abstractmethod
    def firmware_upgrade(self, fw_type, fw_path, fw_extra):
        return

    # Get last firmware upgrade information, inlcudes:
    # 1) FwType: cpld/fpga/bios/bmc(passed by method 'firmware_upgrade'), string
    # 2) FwPath: path and file name of firmware(passed by method 'firmware_upgrade'), string
    # 3) FwExtra: designated string, econdings of this string is determined by vendor(passed by method 'firmware_upgrade')
    # 4) Result: indicates whether the upgrade action is performed and success/failure status if performed. Values should be one of: "DONE"/"FAILED"/"NOT_PERFORMED".
    # dict object:
    # {
    # "FwType": "cpld",
    # "FwExtra": "specific_encoded_string"
    # "Result": "DONE"/"FAILED"/"NOT_PERFORMED"
    # }
    @abc.abstractmethod
    def get_last_upgrade_result(self):
        return

    # Program FPGA and/or CPLD firmware only, but do not refresh them
    #
    # @param fw_type value can be: FPGA, CPLD
    # @param fw_path a string of firmware file path, seperated by ':'
    # @param fw_extra a string of firmware subtype, i.e CPU_CPLD, BOARD_CPLD,
    #                 FAN_CPLD, LC_CPLD, etc. Subtypes are seperated by ':',
    #                 the sequence should match the file nanmes in param @fw_path
    # @return True when all required firmware is program succefully,
    #         False otherwise.
    #
    # Example:
    #     self.firmware_program("CPLD", "/cpu_cpld.vme:/lc_cpld", \
    #                           "CPU_CPLD:LC_CPLD")
    #     or
    #     self.firmware_program("FPGA", "/fpga.bin", "FPGA")
    @abc.abstractmethod
    def firmware_program(self, fw_type, fw_path, fw_extra=None):
        return

    # Refresh firmware and take extra action when necessary.
    # @param fpga_list a list of FPGA names
    # @param cpld_list a list of CPLD names
    # @return True if refresh succefully
    #
    # @Note extra action:
    #       1) response OK to restful call
    #       2) shutdown eth0.4088(keep eth0 working)
    #       3) shutdown SFP power
    #       4) power off all(CPU, switch, gearbox)
    #       5) if power off fpga is supported:
    #              power off fpga
    #          else:
    #              refresh fpga
    #       6) refresh linecard cpld, fan cpld
    #       7) refresh cpu cpld, baseboard cpld
    #       8) power on all
    #       9) bring up eth0.4088
    #
    # Example:
    #    self.firmware_refresh(["FPGA"], ["CPU_CPLD", "LC_CPLD"], "/tmp/fw/refresh.vme")
    #    or
    #    self.firmware_refresh(["FPGA"], None, None)
    #    or
    #    self.firmware_refresh(None, ["FAN_CPLD", "LC1_CPLD", "BOARD_CPLD"], "/tmp/fw/refresh.vme")
    @abc.abstractmethod
    def firmware_refresh(self, fpga_list, cpld_list, fw_extra=None):
        return

    # Get booting flash of running BMC.
    # @return a string, "master" or "slave"
    @abc.abstractmethod
    def get_running_bmc(self):
        return

    # Set booting flash of BMC
    # @param flash should be "master" or "slave"
    @abc.abstractmethod
    def set_bmc_boot_flash(self, flash):
        return

    # Reboot BMC
    @abc.abstractmethod
    def reboot_bmc(self):
        return

    # Get booting uboot image of current running host OS
    # @return a string, "master" or "slave"
    @abc.abstractmethod
    def get_current_uboot(self):
        return
