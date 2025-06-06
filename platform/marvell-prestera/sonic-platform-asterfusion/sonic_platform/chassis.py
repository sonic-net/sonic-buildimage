########################################################################
# Asterfusion CX-N Devices Chassis API                                 #
#                                                                      #
# Module contains an implementation of SONiC Platform Base API and     #
# provides the Chassis information which are available in the platform #
#                                                                      #
########################################################################

try:
    import copy
    import time

    from functools import cached_property
    from itertools import product

    from .constants import *
    from .helper import Helper
    from .logger import Logger

    from .eeprom import Eeprom
    from .component import Component
    from .fan import Fan
    from .fan_drawer import FanDrawer
    from .psu import Psu
    from .thermal import Thermal

    from .sensor import VoltageSensor
    from .sfp import Sfp

    from sonic_platform_base.chassis_base import ChassisBase
except ImportError as err:
    raise ImportError(str(err) + "- required module not found")


class Chassis(ChassisBase):
    """Platform-specific Chassis class"""

    def __init__(self):
        # type: () -> None
        self._helper = Helper()
        self._logger = Logger()
        ChassisBase.__init__(self)
        self._hwsku = self._helper.get_hwsku()
        self._asic = self._helper.get_asic()
        (
            self._num_component,
            self._num_fan_drawer,
            self._num_fan,
            self._num_psu,
            self._num_thermal,
            self._num_voltage_sensor,
            self._num_sfp,
        ) = self._helper.get_peripheral_num()
        if self._num_fan_drawer:
            self._num_fan_per_drawer = self._num_fan // self._num_fan_drawer
        else:
            self._num_fan_per_drawer = self._num_fan
        # fmt: off
        self._logger.log_info("Initialized chassis information:")
        self._logger.log_info("HWSKU                  : <{}>".format(self._hwsku))
        self._logger.log_info("ASIC                   : <{}>".format(self._asic))
        self._logger.log_info("Component count        : <{}>".format(self._num_component))
        self._logger.log_info("Fan drawer count       : <{}>".format(self._num_fan_drawer))
        self._logger.log_info("Fan count per drawer   : <{}>".format(self._num_fan_per_drawer))
        self._logger.log_info("Fan count in total     : <{}>".format(self._num_fan))
        self._logger.log_info("Power supply unit count: <{}>".format(self._num_psu))
        self._logger.log_info("Thermal sensor count   : <{}>".format(self._num_thermal))
        self._logger.log_info("Voltage sensor count   : <{}>".format(self._num_voltage_sensor))
        self._logger.log_info("Front panel port count : <{}>".format(self._num_sfp))
        # fmt: on

    ##############################################
    # LAZY attributes
    ##############################################

    @cached_property
    def __eeprom(self):
        self._eeprom = Eeprom()
        self._logger.log_info("EEPROM has been initialized")
        return self._eeprom

    @cached_property
    def __component_list(self):
        self._component_list = list(
            map(
                lambda component_index: Component(
                    component_index, self._hwsku, self._asic
                ),
                range(self._num_component),
            )
        )
        self._logger.log_info("Component list has been initialized")
        return self._component_list

    @cached_property
    def __fan_list(self):
        self._fan_list = list(
            map(
                lambda fand_fan_index: Fan(
                    fand_fan_index[0],
                    fand_fan_index[1],
                    self._num_fan_per_drawer,
                    self._hwsku,
                    self._asic,
                ),
                product(range(self._num_fan_drawer), range(self._num_fan_per_drawer)),
            )
        )
        self._logger.log_info("Fan list has been initialized")
        return self._fan_list

    @cached_property
    def __fan_drawer_list(self):
        self._fan_drawer_list = list(
            map(
                lambda fand_index: FanDrawer(
                    fand_index, self._num_fan_per_drawer, self._hwsku, self._asic
                ),
                range(self._num_fan_drawer),
            )
        )
        self._logger.log_info("Fan drawer list has been initialized")
        return self._fan_drawer_list

    @cached_property
    def __psu_list(self):
        self._psu_list = list(
            map(
                lambda psu_index: Psu(psu_index, self._hwsku, self._asic),
                range(self._num_psu),
            )
        )
        self._logger.log_info("PSU list has been initialized")
        return self._psu_list

    @cached_property
    def __thermal_list(self):
        self._thermal_list = list(
            map(
                lambda thermal_index: Thermal(thermal_index, self._hwsku, self._asic),
                range(self._num_thermal),
            )
        )
        self._logger.log_info("Thermal list has been initialized")
        return self._thermal_list

    @cached_property
    def __voltage_sensor_list(self):
        self._voltage_sensor_list = list(
            map(
                lambda voltage_sensor_index: VoltageSensor(
                    voltage_sensor_index, self._hwsku, self._asic
                ),
                range(self._num_voltage_sensor),
            )
        )
        self._logger.log_info("Sensor list has been initialized")
        return self._voltage_sensor_list

    @cached_property
    def __sfp_list(self):
        self._sfp_list = list(
            map(
                lambda sfp_index: Sfp(sfp_index, self._hwsku, self._asic),
                range(self._num_sfp),
            )
        )
        self._logger.log_info("SFP list has been initialized")
        return self._sfp_list

    @property
    def __device_presence_dict(self):
        if not hasattr(self, "_device_presence_dict"):
            self._device_presence_dict = {}
            self._device_presence_dict.setdefault(
                "fan_drawer",
                dict.fromkeys(map(str, range(len(self.__fan_drawer_list))), "0"),
            )
            self._device_presence_dict.setdefault(
                "fan", dict.fromkeys(map(str, range(len(self.__fan_list))), "0")
            )
            self._device_presence_dict.setdefault(
                "psu", dict.fromkeys(map(str, range(len(self.__psu_list))), "0")
            )
            self._device_presence_dict.setdefault(
                "sfp", dict.fromkeys(map(str, range(len(self.__sfp_list))), "0")
            )
            self._logger.log_info("Device presence dict has been initialized")
        return self._device_presence_dict

    @__device_presence_dict.setter
    def __device_presence_dict(self, device_presence_dict):
        self._device_presence_dict = device_presence_dict

    @__device_presence_dict.deleter
    def __device_presence_dict(self):
        del self._device_presence_dict

    ##############################################
    # Chassis methods
    ##############################################

    def get_base_mac(self):
        # type: () -> str
        """
        Retrieves the base MAC address for the chassis

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self.get_eeprom().get_base_mac_addr()

    def get_system_eeprom_info(self):
        # type: () -> dict[str, str]
        """
        Retrieves the full content of system EEPROM information for the chassis

        Returns:
            A dictionary where keys are the type code defined in
            OCP ONIE TlvInfo EEPROM format and values are their corresponding
            values.
            Ex. { '0x21':'AG9064', '0x22':'V1.0', '0x23':'AG9064-0109867821',
                  '0x24':'001c0f000fcd0a', '0x25':'02/03/2018 16:22:00',
                  '0x26':'01', '0x27':'REV01', '0x28':'AG9064-C2358-16G'}
        """
        return self.get_eeprom().get_system_eeprom_info()

    def get_reboot_cause(self):
        # type: () -> tuple[str, str]
        """
        Retrieves the cause of the previous reboot

        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must be
            one of the predefined strings in this class. If the first string
            is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be used
            to pass a description of the reboot cause.
        """
        return (self.REBOOT_CAUSE_NON_HARDWARE, "")

    ##############################################
    # Component methods
    ##############################################

    def get_num_components(self):
        # type: () -> int
        """
        Retrieves the number of components available on this chassis

        Returns:
            An integer, the number of components available on this chassis
        """
        return len(self.__component_list)

    def get_all_components(self):
        # type: () -> list[Component]
        """
        Retrieves all components available on this chassis

        Returns:
            A list of objects derived from ComponentBase representing all components
            available on this chassis
        """
        return self.__component_list

    def get_component(self, index):
        # type: (int) -> Component | None
        """
        Retrieves component represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the component to retrieve

        Returns:
            An object dervied from ComponentBase representing the specified component
        """
        component = None

        try:
            component = self.__component_list[index]
        except IndexError:
            self._logger.log_error(
                "Component index <{}> out of range (<0-{}>)".format(
                    index, len(self.__component_list) - 1
                )
            )

        return component

    ##############################################
    # Fan methods
    ##############################################

    def get_num_fans(self):
        # type: () -> int
        """
        Retrieves the number of fans available on this chassis

        Returns:
            An integer, the number of fan modules available on this chassis
        """
        return len(self.__fan_list)

    def get_all_fans(self):
        # type: () -> list[Fan]
        """
        Retrieves all fan modules available on this chassis

        Returns:
            A list of objects derived from FanBase representing all fan
            modules available on this chassis
        """
        return self.__fan_list

    def get_fan(self, index):
        # type: (int) -> Fan | None
        """
        Retrieves fan module represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan module to
            retrieve

        Returns:
            An object dervied from FanBase representing the specified fan
            module
        """
        fan = None

        try:
            fan = self.__fan_list[index]
        except IndexError:
            self._logger.log_error(
                "Fan index <{}> out of range (<0-{}>)".format(
                    index, len(self.__fan_list) - 1
                )
            )

        return fan

    ##############################################
    # Fan drawer methods
    ##############################################

    def get_num_fan_drawers(self):
        # type: () -> int
        """
        Retrieves the number of fan drawers available on this chassis

        Returns:
            An integer, the number of fan drawers available on this chassis
        """
        return len(self.__fan_drawer_list)

    def get_all_fan_drawers(self):
        # type: () -> list[FanDrawer]
        """
        Retrieves all fan drawers available on this chassis

        Returns:
            A list of objects derived from FanDrawerBase representing all fan
            drawers available on this chassis
        """
        return self.__fan_drawer_list

    def get_fan_drawer(self, index):
        # type: (int) -> FanDrawer | None
        """
        Retrieves fan drawers represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the fan drawer to
            retrieve

        Returns:
            An object dervied from FanDrawerBase representing the specified fan
            drawer
        """
        fan_drawer = None

        try:
            fan_drawer = self.__fan_drawer_list[index]
        except IndexError:
            self._logger.log_error(
                "Fan drawer index <{}> out of range (<0-{}>)".format(
                    index, len(self.__fan_drawer_list) - 1
                )
            )

        return fan_drawer

    ##############################################
    # PSU methods
    ##############################################

    def get_num_psus(self):
        # type: () -> int
        """
        Retrieves the number of power supply units available on this chassis

        Returns:
            An integer, the number of power supply units available on this
            chassis
        """
        return len(self.__psu_list)

    def get_all_psus(self):
        # type: () -> list[Psu]
        """
        Retrieves all power supply units available on this chassis

        Returns:
            A list of objects derived from PsuBase representing all power
            supply units available on this chassis
        """
        return self.__psu_list

    def get_psu(self, index):
        # type: (int) -> Psu | None
        """
        Retrieves power supply unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the power supply unit to
            retrieve

        Returns:
            An object dervied from PsuBase representing the specified power
            supply unit
        """
        psu = None

        try:
            psu = self.__psu_list[index]
        except IndexError:
            self._logger.log_error(
                "PSU index <{}> out of range (<0-{}>)".format(
                    index, len(self.__psu_list) - 1
                )
            )

        return psu

    ##############################################
    # THERMAL methods
    ##############################################

    def get_num_thermal(self):
        # type: () -> int
        """
        Retrieves the number of thermals available on this chassis

        Returns:
            An integer, the number of thermals available on this chassis
        """
        return len(self.__thermal_list)

    def get_num_thermals(self):
        # type: () -> int
        """
        Retrieves the number of thermals available on this chassis

        Returns:
            An integer, the number of thermals available on this chassis
        """
        return len(self.__thermal_list)

    def get_all_thermals(self):
        # type: () -> list[Thermal]
        """
        Retrieves all thermals available on this chassis

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this chassis
        """
        return self.__thermal_list

    def get_thermal(self, index):
        # type: (int) -> Thermal | None
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        thermal = None

        try:
            thermal = self.__thermal_list[index]
        except IndexError:
            self._logger.log_error(
                "THERMAL index <{}> out of range (<0-{}>)".format(
                    index, len(self.__thermal_list) - 1
                )
            )

        return thermal

    ##############################################
    # Voltage Sensor Methods
    ##############################################

    def get_num_voltage_sensors(self):
        # type: () -> int
        """
        Retrieves the number of voltage sensors available on this chassis

        Returns:
            An integer, the number of voltage sensors available on this chassis
        """
        return len(self.__voltage_sensor_list)

    def get_all_voltage_sensors(self):
        # type: () -> list[Sensor]
        """
        Retrieves all voltage sensors available on this chassis

        Returns:
            A list of objects derived from VoltageSensorBase representing all voltage
            sensors available on this chassis
        """
        return self.__voltage_sensor_list

    def get_voltage_sensor(self, index):
        # type: (int) -> Sensor | None
        """
        Retrieves voltage sensor unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the voltage sensor to
            retrieve

        Returns:
            An object derived from VoltageSensorBase representing the specified voltage sensor
        """
        voltage_sensor = None

        try:
            voltage_sensor = self.__voltage_sensor_list[index]
        except IndexError:
            self._logger.log_error(
                "Voltage sensor index <{}> out of range (<0-{}>)".format(
                    index, len(self.__voltage_sensor_list) - 1
                )
            )

        return voltage_sensor

    ##############################################
    # SFP methods
    ##############################################

    def get_num_sfps(self):
        # type: () -> int
        """
        Retrieves the number of sfps available on this chassis

        Returns:
            An integer, the number of sfps available on this chassis
        """
        return len(self.__sfp_list)

    def get_all_sfps(self):
        # type: () -> list[Sfp]
        """
        Retrieves all sfps available on this chassis

        Returns:
            A list of objects derived from SfpBase representing all sfps
            available on this chassis
        """
        return [sfp for sfp in self.__sfp_list if sfp is not None]

    def get_sfp(self, index):
        # type: (int) -> Sfp | None
        """
        Retrieves sfp corresponding to physical port <index>

        Args:
            index: An integer (>=0), the index of the sfp to retrieve.
                   The index should correspond to the physical port in a chassis.
                   For example:-
                   1 for Ethernet0, 2 for Ethernet4 and so on for one platform.
                   0 for Ethernet0, 1 for Ethernet4 and so on for another platform.

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        sfp = None

        try:
            sfp = self.__sfp_list[index]
        except IndexError:
            self._logger.log_error(
                "SFP index <{}> out of range (<0-{}>)".format(
                    index, len(self.__sfp_list) - 1
                )
            )

        return sfp

    def reset_all_sfps(self):
        # type: () -> bool
        """
        Reset sfps on all ports

        Returns:
            bool: True if all reset operations are successfule, False if some fails
        """
        status = True
        # SW reset assert & deassert
        for sfp in self.__sfp_list:
            if not sfp.assert_soft_reset():
                status = False
        time.sleep(1.0)
        for sfp in self.__sfp_list:
            if not sfp.deassert_soft_reset():
                status = False
        # HW reset assert & deassert
        for sfp in self.__sfp_list:
            if not sfp.assert_hard_reset():
                status = False
        time.sleep(2.0)
        for sfp in self.__sfp_list:
            if not sfp.deassert_hard_reset():
                status = False
        return status

    ##############################################
    # Other methods
    ##############################################

    def get_eeprom(self):
        # type: () -> Eeprom
        """
        Retreives eeprom device on this chassis

        Returns:
            An object derived from WatchdogBase representing the hardware
            eeprom device
        """
        return self.__eeprom

    def get_change_event(self, timeout=0):
        # type: (int) -> dict[str, dict[str, str]]
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if call successful, False if not;
                - A nested dictionary where key is a device type,
                  value is a dictionary with key:value pairs in the format of
                  {'device_id':'device_event'},
                  where device_id is the device ID for this device and
                        device_event,
                             status='1' represents device inserted,
                             status='0' represents device removed.
                  Ex. {'fan':{'0':'0', '2':'1'}, 'sfp':{'11':'0'}}
                      indicates that fan 0 has been removed, fan 2
                      has been inserted and sfp 11 has been removed.
                  Specifically for SFP event, besides SFP plug in and plug out,
                  there are some other error event could be raised from SFP, when
                  these error happened, SFP eeprom will not be avalaible, XCVRD shall
                  stop to read eeprom before SFP recovered from error status.
                      status='2' I2C bus stuck,
                      status='3' Bad eeprom,
                      status='4' Unsupported cable,
                      status='5' High Temperature,
                      status='6' Bad cable.
        """
        # Create a back-up device presence dict to avoid detection failure
        device_presence_dict = copy.deepcopy(self.__device_presence_dict)
        change_event_dict = {"fan_drawer": {}, "fan": {}, "psu": {}, "sfp": {}}
        start = time.time()
        while True:
            timediff = time.time()
            succeeded = True
            detected = False
            presence = False
            reason = None
            try:
                # Walk throught all fan drawers and get current fan drawer presence
                for index, fan_drawer in enumerate(self.__fan_drawer_list):
                    fan_drawer_index = str(index)
                    presence = fan_drawer.get_presence()
                    if (
                        str(int(presence))
                        != device_presence_dict["fan_drawer"][fan_drawer_index]
                    ):
                        device_presence_dict["fan_drawer"][fan_drawer_index] = str(
                            int(presence)
                        )
                        change_event_dict["fan_drawer"][fan_drawer_index] = str(
                            int(presence)
                        )
                        detected = True
                        self._logger.log_info(
                            "Detected <{}> <{}> event".format(
                                fan_drawer.get_name(),
                                "inserted" if presence else "removed",
                            )
                        )
                # Walk throught all fans and get current fan presence
                for index, fan in enumerate(self.__fan_list):
                    fan_index = str(index)
                    presence = fan.get_presence()
                    if str(int(presence)) != device_presence_dict["fan"][fan_index]:
                        device_presence_dict["fan"][fan_index] = str(int(presence))
                        change_event_dict["fan"][fan_index] = str(int(presence))
                        detected = True
                        self._logger.log_info(
                            "Detected <{}> <{}> event".format(
                                fan.get_name(), "inserted" if presence else "removed"
                            )
                        )
                # Walk throught all psus and get current psu presence
                for index, psu in enumerate(self.__psu_list):
                    psu_index = str(index)
                    presence = psu.get_presence()
                    if str(int(presence)) != device_presence_dict["psu"][psu_index]:
                        device_presence_dict["psu"][psu_index] = str(int(presence))
                        change_event_dict["psu"][psu_index] = str(int(presence))
                        detected = True
                        self._logger.log_info(
                            "Detected <{}> <{}> event".format(
                                psu.get_name(), "inserted" if presence else "removed"
                            )
                        )
                # Walk throught all sfps and get current sfp presence
                for index, sfp in enumerate(self.__sfp_list):
                    sfp_index = str(index)
                    presence = sfp.get_presence()
                    if str(int(presence)) != device_presence_dict["sfp"][sfp_index]:
                        device_presence_dict["sfp"][sfp_index] = str(int(presence))
                        change_event_dict["sfp"][sfp_index] = str(int(presence))
                        detected = True
                        self._logger.log_info(
                            "Detected <{}> <{}> event".format(
                                sfp.get_name(), "inserted" if presence else "removed"
                            )
                        )
            except Exception as err:
                succeeded = False
                reason = err
            # Four conditions(OR gate) are concerned here:
            # 1) When succeeded is False. In other words, this method cannot run well. So we shouldn`t let the loop go on.
            if succeeded:  # If call successful, update the device presence dict
                self.__device_presence_dict = copy.deepcopy(device_presence_dict)
                self._logger.log_debug("Succeeded in updating device presence dict")
            else:  # Otherwise break the loop immediately
                self._logger.log_debug(
                    "Failed in updating device presence dict: <{}>".format(reason)
                )
                return succeeded, change_event_dict
            # 2) When a change event is detected. Always break the loop as long as a change event is detected.
            # 3) When timeout is 0. Under this condition we should never break the loop until a change event is detected.
            if detected or (timeout == 0 and detected):
                self._logger.log_debug("Succeeded in updating device presence dict")
                return succeeded, change_event_dict
            # 4) When time elapsed is longer than timeout. It doesn`t matter if there`s any change event detected,
            #    As long as it has taken long enough on running this method, we just break the loop.
            if timeout > 0 and time.time() - start > timeout / 1000:
                self._logger.log_debug(
                    "Failed in updating device presence dict due to time out"
                )
                return succeeded, change_event_dict
            time.sleep(max(0, 1 + timediff - time.time()))

    def get_name(self):
        # type: () -> str
        """
        Retrieves the name of the device

        Returns:
            string: The name of the device
        """
        return self.get_eeprom().get_model_str()

    def get_presence(self):
        # type: () -> bool
        """
        Retrieves the presence of the device

        Returns:
            bool: True if device is present, False if not
        """
        return True

    def get_model(self):
        # type: () -> str
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.get_eeprom().get_part_number_str()

    def get_serial(self):
        # type: () -> str
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return self.get_eeprom().get_serial_number_str()

    def get_revision(self):
        # type: () -> str
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.get_eeprom().get_label_revision_str()

    def get_status(self):
        # type: () -> bool
        """
        Retrieves the operational status of the device

        Returns:
            A boolean value, True if device is operating properly, False if not
        """
        return True
