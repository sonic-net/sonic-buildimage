######################################################################
# Asterfusion CX-N Devices Transceiver                               #
#                                                                    #
# Sfp contains an implementation of SONiC Platform Base API and      #
# provides the sfp device status which are available in the platform #
#                                                                    #
######################################################################

try:
    import subprocess
    import time

    from .constants import *
    from .helper import Helper
    from .logger import Logger
    from .thermal import Thermal

    from sonic_platform_base.sonic_xcvr.sfp_optoe_base import SfpOptoeBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Sfp(SfpOptoeBase):
    """Platform-specific Sfp class"""

    def __init__(self, sfp_index, hwsku, asic):
        # type: (int, str, str) -> None
        self._helper = Helper()
        self._logger = Logger()
        SfpOptoeBase.__init__(self)
        self._sfp_index = sfp_index
        self._hwsku = hwsku
        self._asic = asic
        self._init_sfp_info()

    def _init_sfp_info(self):
        # type: () -> None
        sfp_info = SFP_INFO.get(self._hwsku, {}).get(self._asic, None)
        if sfp_info is None:
            self._logger.log_fatal("Failed in initializing sfp info")
            raise RuntimeError("failed in initializing sfp info")
        if self._sfp_index >= len(sfp_info):
            self._logger.log_fatal("Failed in initializing sfp info")
            raise RuntimeError("failed in initializing sfp info")
        self._sfp_info = sfp_info[self._sfp_index]
        # Static information
        self.index = self._sfp_index
        self._sfp_name = self._sfp_info.get("name", NOT_AVAILABLE)
        self._logger.log_info("Initialized info for <{}>".format(self._sfp_name))

    def get_num_thermals(self):
        # type: () -> int
        """
        Retrieves the number of thermals available on this SFP

        Returns:
            An integer, the number of thermals available on this SFP
        """
        return len(self._thermal_list)

    def get_all_thermals(self):
        # type: () -> list[Thermal]
        """
        Retrieves all thermals available on this SFP

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this SFP
        """
        return self._thermal_list

    def get_thermal(self, index):
        # type: () -> Thermal
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object derived from ThermalBase representing the specified thermal
        """
        thermal = None

        try:
            thermal = self._thermal_list[index]
        except IndexError:
            self._logger.log_error(
                "THERMAL index <{}> out of range (<0-{}>)".format(
                    index, len(self._thermal_list) - 1
                )
            )

        return thermal

    def get_eeprom_path(self):
        # type: () -> str
        """
        Retrieves the eeprom path of SFP

        Returns:
            string: The path to the eeprom file of SFP
        """
        eeprom_path = self._sfp_info.get("eeprom", {}).get("path", NOT_AVAILABLE)
        if eeprom_path == NOT_AVAILABLE:
            self._logger.log_fatal(
                "Failed in getting eeprom path for <{}>".format(self._sfp_name)
            )
            raise RuntimeError(
                "failed in getting eeprom path for <{}>".format(self._sfp_name)
            )
        return eeprom_path

    def get_reset_status(self):
        # type: () -> bool
        """
        Retrieves the reset status of SFP

        Returns:
            A Boolean, True if reset enabled, False if disabled
        """
        if self._sfp_info.get("rststat", None) is None:
            self._logger.log_info(
                "Current device does not support reset status on <{}>".format(
                    self._sfp_name
                )
            )
            return False
        return self._helper.get_sysfs_content(self._sfp_info, "rststat")

    def get_error_description(self):
        """
        Retrives the error descriptions of the SFP module

        Returns:
            String that represents the current error descriptions of vendor specific errors
            In case there are multiple errors, they should be joined by '|',
            like: "Bad EEPROM|Unsupported cable"
        """
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return ERROR_DESCRIPTION_UNPLUGGED
        api = self.get_xcvr_api()
        handler = getattr(api, "get_error_description", None)
        if handler is not None:
            try:
                return handler()
            except NotImplementedError:
                self._logger.log_info(
                    "Failed in getting error description for module on <{}> due to operation unsupported".format(
                        self._sfp_name
                    )
                )
        return ERROR_DESCRIPTION_OK

    def reset(self):
        # type: () -> bool
        """
        Reset SFP and return all user module settings to their default state.

        Returns:
            A boolean, True if successful, False if not
        """
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return False
        # SW reset via API
        api = self.get_xcvr_api()
        handler = getattr(api, "reset", None)
        if handler is not None:
            try:
                return handler()
            except NotImplementedError:
                self._logger.log_info(
                    "Failed in software resetting module on <{}> due to method not implemented".format(
                        self._sfp_name
                    )
                )
        # HW reset via CPLD pin
        if "reset" not in self._sfp_info:
            self._logger.log_info(
                "Failed in hardware resetting module on <{}> due to operation unsupported".format(
                    self._sfp_name
                )
            )
            return False
        success = self._helper.trigger_via_sysfs(self._sfp_info, "reset", True)
        time.sleep(2)
        success &= self._helper.trigger_via_sysfs(self._sfp_info, "reset", False)
        time.sleep(1)
        if success:
            self._logger.log_info(
                "Succeeded in hardware resetting module on <{}>".format(self._sfp_name)
            )
        else:
            self._logger.log_info(
                "Failed in hardware resetting module on <{}> due to write failure".format(
                    self._sfp_name
                )
            )
        return success

    def assert_soft_reset(self):
        # type: () -> bool
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return False
        # Assert SW reset via API
        api = self.get_xcvr_api()
        handler = getattr(api, "reset", None)
        if handler is not None:
            try:
                return handler()
            except NotImplementedError:
                self._logger.log_info(
                    "Failed in asserting software reset on module on <{}> due to method not implemented".format(
                        self._sfp_name
                    )
                )
                return False

    def deassert_soft_reset(self):
        # type: () -> bool
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return False
        # No need to deassert SW reset manually.
        self._logger.log_info(
            "Succeeded in deasserting software reset on module on <{}> due to method not implemented".format(
                self._sfp_name
            )
        )
        return True

    def assert_hard_reset(self):
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return False
        # Assert HW reset via CPLD pin
        if "reset" not in self._sfp_info:
            self._logger.log_info(
                "Failed in asserting hardware reset on module on <{}> due to operation unsupported".format(
                    self._sfp_name
                )
            )
            return False
        success = self._helper.trigger_via_sysfs(self._sfp_info, "reset", True)
        if success:
            self._logger.log_info(
                "Succeeded in asserting hardware reset on module on <{}>".format(
                    self._sfp_name
                )
            )
        else:
            self._logger.log_info(
                "Failed in asserting hardware reset on module on <{}> due to write failure".format(
                    self._sfp_name
                )
            )
        return success

    def deassert_hard_reset(self):
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return False
        # HW reset via CPLD pin
        if "reset" not in self._sfp_info:
            self._logger.log_info(
                "Failed in deasserting hardware reset on module on <{}> due to operation unsupported".format(
                    self._sfp_name
                )
            )
            return False
        success = self._helper.trigger_via_sysfs(self._sfp_info, "reset", False)
        if success:
            self._logger.log_info(
                "Succeeded in deasserting hardware reset on module on <{}>".format(
                    self._sfp_name
                )
            )
        else:
            self._logger.log_info(
                "Failed in deasserting hardware reset on module on <{}> due to write failure".format(
                    self._sfp_name
                )
            )
        return success

    def set_status_led(self):
        # type: () -> None
        if self._hwsku != HWSKU_CX532PN:
            return
        if self._asic not in (ASIC_FL00E02, ASIC_FL00E03):
            return
        if self._sfp_index < 32:
            return
        # Update X1/X2 led status for CX532P-N FL00E02/FL00E03
        if not hasattr(self, "_db"):
            from swsscommon.swsscommon import SonicV2Connector

            self._db = SonicV2Connector(host="127.0.0.1")
            self._db.connect(self._db.APPL_DB)
            self._db.connect(self._db.COUNTERS_DB)
        if not hasattr(self, "_oper_status"):
            self._oper_status = 0

        port_oid = self._db.get(
            self._db.COUNTERS_DB, "COUNTERS_PORT_NAME_MAP", "{}".format(self._sfp_name)
        )
        if port_oid is None:
            self._logger.log_error(
                "Failed in setting LED for <{}> due to invalid oid".format(
                    self._sfp_name
                )
            )
            return
        port_oper_status = self._db.get(
            self._db.APPL_DB, "PORT_TABLE:{}".format(self._sfp_name), "oper_status"
        )
        if port_oper_status is None:
            port_oper_status = "down"
        port_rx_rate = self._db.get(
            self._db.COUNTERS_DB, "RATES:{}".format(port_oid), "RX_BPS"
        )
        port_tx_rate = self._db.get(
            self._db.COUNTERS_DB, "RATES:{}".format(port_oid), "TX_BPS"
        )
        port_rx_rate = float(port_rx_rate) if port_rx_rate is not None else 0.0
        port_tx_rate = float(port_tx_rate) if port_tx_rate is not None else 0.0
        port_traffic_rate = port_rx_rate + port_tx_rate
        aux_sfp_index = self._sfp_index - CX532PN_PORT_SFP_START
        reg_offset = 0xF0 >> (aux_sfp_index * 4)
        led_get_cmd = "i2cget -f -y 2 0x40 0x19"
        led_set_cmd = "i2cset -f -y 2 0x40 0x19 {}"

        try:
            if port_oper_status == "up":
                if (
                    port_traffic_rate != 0
                    and ((self._oper_status >> (aux_sfp_index * 4)) & 0xF) != 3
                ):
                    led_status = int(
                        subprocess.check_output(led_get_cmd, shell=True)
                        .strip()
                        .decode(),
                        0,
                    )
                    subprocess.check_output(
                        led_set_cmd.format(
                            str(2 << (aux_sfp_index * 4) | (reg_offset & led_status))
                        ),
                        shell=True,
                    )
                    self._oper_status = (self._oper_status & reg_offset) | (
                        3 << (aux_sfp_index * 4)
                    )
                    self._logger.log_info(
                        "Succeeded in setting LED to blinking for <{}>".format(
                            self._sfp_name
                        )
                    )
                elif (
                    port_traffic_rate == 0
                    and ((self._oper_status >> (aux_sfp_index * 4)) & 0xF) != 1
                ):
                    led_status = int(
                        subprocess.check_output(led_get_cmd, shell=True)
                        .strip()
                        .decode(),
                        0,
                    )
                    subprocess.check_output(
                        led_set_cmd.format(
                            str(1 << (aux_sfp_index * 4) | (reg_offset & led_status))
                        ),
                        shell=True,
                    )
                    self._oper_status = (self._oper_status & reg_offset) | (
                        1 << (aux_sfp_index * 4)
                    )
                    self._logger.log_info(
                        "Succeeded in setting LED to on for <{}>".format(self._sfp_name)
                    )
            else:
                if ((self._oper_status >> (aux_sfp_index * 4)) & 0xF) != 2:
                    led_status = int(
                        subprocess.check_output(led_get_cmd, shell=True)
                        .strip()
                        .decode(),
                        0,
                    )
                    subprocess.check_output(
                        led_set_cmd.format(str(reg_offset & led_status)), shell=True
                    )
                    self._oper_status = (self._oper_status & reg_offset) | (
                        2 << (aux_sfp_index * 4)
                    )
                    self._logger.log_info(
                        "Succeeded in setting LED to off for <{}>".format(
                            self._sfp_name
                        )
                    )
        except Exception as err:
            self._logger.log_error(
                "Failed in setting LED status for <{}> due to <{}>".format(
                    self._sfp_name, err
                )
            )

    def set_lpmode(self, lpmode):
        # type: (bool) -> int
        """
        Sets the lpmode (low power mode) of SFP
        Args:
            lpmode: A Boolean, True to enable lpmode, False to disable it
            Note  : lpmode can be overridden by set_power_override
        Returns:
            A boolean, True if lpmode is set successfully, False if not
        """
        if not self._helper.get_sysfs_content(self._sfp_info, "presence"):
            return LPMODE_UNSUPPORTED
        # SW low power mode via API
        api = self.get_xcvr_api()
        handler = getattr(api, "set_lpmode", None)
        if handler is not None:
            try:
                if handler(lpmode):
                    return LPMODE_SUCCESS
            except NotImplementedError:
                self._logger.log_info(
                    "Failed in software setting module to low power mode on <{}> due to method not implemented".format(
                        self._sfp_name
                    )
                )
        # HW low power mode via CPLD pin
        if "lpmode" not in self._sfp_info:
            self._logger.log_info(
                "Failed in hardware setting module to low power mode on <{}> due to operation unsupported".format(
                    self._sfp_name
                )
            )
            return LPMODE_UNSUPPORTED
        if self._helper.trigger_via_sysfs(self._sfp_info, "lpmode", lpmode):
            return LPMODE_SUCCESS
        return LPMODE_FAILURE

    def get_lpmode(self):
        # type: () -> int
        """
        This common API is applicable only for CMIS as Low Power mode can be verified
        using EEPROM registers.For other media types like QSFP28/QSFP+ etc., platform
        vendors has to implement accordingly.
        """
        api = self.get_xcvr_api()
        if api is None:
            return LPMODE_UNSUPPORTED
        if api.get_lpmode():
            return LPMODE_ON
        return LPMODE_OFF

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self._sfp_name

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        presence = self._helper.get_sysfs_content(self._sfp_info, "presence")
        self.set_status_led()
        return presence

    def get_revision(self):
        # type: () -> str
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        transceiver_info = self.get_transceiver_info()
        if transceiver_info is None:
            return NOT_AVAILABLE
        return transceiver_info.get("vendor_rev", NOT_AVAILABLE)

    def get_status(self):
        # type: () -> bool
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return self._helper.get_sysfs_content(self._sfp_info, "presence")

    def get_position_in_parent(self):
        # type: () -> int
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return self._sfp_index + 1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable.
        Returns:
            bool: True if it is replaceable.
        """
        return True
