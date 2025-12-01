#
# SPDX-FileCopyrightText: NVIDIA CORPORATION & AFFILIATES
# Copyright (c) 2021-2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import redis
import threading
from sonic_platform_base.module_base import ModuleBase
from sonic_platform_base.chassis_base import ChassisBase
from sonic_py_common.syslogger import SysLogger
from .dpuctlplat import DpuCtlPlat, BootProgEnum
import subprocess

from . import utils
from .device_data import DeviceDataManager, DpuInterfaceEnum
from .vpd_parser import VpdParser
from .dpu_vpd_parser import DpuVpdParser
from swsscommon.swsscommon import ConfigDBConnector, SonicV2Connector

# Global logger class instance
logger = SysLogger()


class Module(ModuleBase):
    STATE_ACTIVATED = 1
    STATE_DEACTIVATED = 0

    STATE_DB = 6
    STATE_MODULAR_CHASSIS_SLOT_TABLE = 'MODULAR_CHASSIS_SLOT|{}'
    FIELD_SEQ_NO = 'seq_no'
    redis_client = redis.Redis(db = STATE_DB)

    def __init__(self, slot_id):
        super(Module, self).__init__()
        self.slot_id = slot_id
        self.seq_no = 0
        self.current_state = Module.STATE_DEACTIVATED
        self.lock = threading.Lock()

        self.sfp_initialized_count = 0
        self.sfp_count = 0
        self.vpd_parser = VpdParser('/run/hw-management/lc{}/eeprom/vpd_parsed')

    def get_name(self):
        return 'LINE-CARD{}'.format(self.slot_id)

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.vpd_parser.get_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return self.vpd_parser.get_serial()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.vpd_parser.get_revision()

    def get_type(self):
        return ModuleBase.MODULE_TYPE_LINE

    def get_slot(self):
        return self.slot_id

    def get_presence(self):
        return utils.read_int_from_file('/run/hw-management/system/lc{}_present'.format(self.slot_id)) == 1

    def get_position_in_parent(self):
        return self.slot_id

    def is_replaceable(self):
        return True

    def get_oper_status(self):
        if utils.read_int_from_file('/run/hw-management/system/lc{}_active'.format(self.slot_id)) == 1:
            return ModuleBase.MODULE_STATUS_ONLINE
        elif utils.read_int_from_file('/run/hw-management/system/lc{}_present'.format(self.slot_id)) == 1:
            return ModuleBase.MODULE_STATUS_PRESENT
        elif utils.read_int_from_file('/run/hw-management/system/lc{}_present'.format(self.slot_id)) == 0:
            return ModuleBase.MODULE_STATUS_EMPTY
        else:
            return ModuleBase.MODULE_STATUS_FAULT

    def _check_state(self):
        """Check Module status change:
            1. If status sysfs file value has been changed
            2. If sequence NO has been changed which means line card has been removed and inserted again.
        """
        seq_no = self._get_seq_no()
        state = utils.read_int_from_file('/run/hw-management/system/lc{}_powered'.format(self.slot_id), log_func=None)
        if state != self.current_state:
            self._re_init()
        elif seq_no != self.seq_no:
            if state == Module.STATE_ACTIVATED: # LC has been replaced, need re-initialize
                self._re_init()
        self.current_state = state
        self.seq_no = seq_no

    def _get_seq_no(self):
        try:
            seq_no = Module.redis_client.hget(Module.STATE_MODULAR_CHASSIS_SLOT_TABLE.format(self.slot_id), Module.FIELD_SEQ_NO)
            seq_no = seq_no.decode().strip()
        except Exception as e:
            seq_no = 0
        return seq_no

    def _re_init(self):
        self._thermal_list = []
        self._sfp_list = []
        self._sfp_count = 0


    ##############################################
    # THERMAL methods
    ##############################################

    def initialize_thermals(self):
        self._check_state()
        if self.current_state == Module.STATE_ACTIVATED and not self._thermal_list:
            from .thermal import initialize_linecard_thermals
            self._thermal_list = initialize_linecard_thermals(self.get_name(), self.slot_id)

    def get_num_thermals(self):
        """
        Retrieves the number of thermals available on this module

        Returns:
            An integer, the number of thermals available on this module
        """
        return DeviceDataManager.get_gearbox_count('/run/hw-management/lc{}/config'.format(self.slot_id))

    def get_all_thermals(self):
        """
        Retrieves all thermals available on this module

        Returns:
            A list of objects derived from ThermalBase representing all thermals
            available on this module
        """
        with self.lock:
            self.initialize_thermals()
            return self._thermal_list

    def get_thermal(self, index):
        """
        Retrieves thermal unit represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the thermal to
            retrieve

        Returns:
            An object dervied from ThermalBase representing the specified thermal
        """
        with self.lock:
            self.initialize_thermals()
            return super(Module, self).get_thermal(index)

    ##############################################
    # SFP methods
    ##############################################
    def _create_sfp_object(self, index):
        from .sfp import SFP
        return SFP(index, slot_id=self.slot_id, linecard_port_count=self.sfp_count, lc_name=self.get_name())

    def initialize_single_sfp(self, index):
        self._check_state()
        if self.current_state == Module.STATE_ACTIVATED:
            sfp_count = self.get_num_sfps()
            if index < sfp_count:
                if not self._sfp_list:
                    self._sfp_list = [None] * sfp_count

                if not self._sfp_list[index]:
                    self._sfp_list[index] = self._create_sfp_object(index)
                    self.sfp_initialized_count += 1

    def initialize_sfps(self):
        self._check_state()
        if self.current_state == Module.STATE_ACTIVATED:
            if not self._sfp_list:
                sfp_count = self.get_num_sfps()
                for index in range(sfp_count):
                    self._sfp_list.append(self._create_sfp_object(index))
                self.sfp_initialized_count = sfp_count
            elif self.sfp_initialized_count != len(self._sfp_list):
                for index in range(len(self._sfp_list)):
                    if self._sfp_list[index] is None:
                        self._sfp_list[index] = self._create_sfp_object(index)
                self.sfp_initialized_count = len(self._sfp_list)

    def get_num_sfps(self):
        """
        Retrieves the number of sfps available on this module

        Returns:
            An integer, the number of sfps available on this module
        """
        if self.sfp_count == 0:
            self.sfp_count = DeviceDataManager.get_linecard_sfp_count(self.slot_id)
        return self.sfp_count

    def get_all_sfps(self):
        """
        Retrieves all sfps available on this module

        Returns:
            A list of objects derived from PsuBase representing all sfps
            available on this module
        """
        with self.lock:
            self.initialize_sfps()
            return self._sfp_list

    def get_sfp(self, index):
        """
        Retrieves sfp represented by (0-based) index <index>

        Args:
            index: An integer, the index (0-based) of the sfp to retrieve

        Returns:
            An object dervied from SfpBase representing the specified sfp
        """
        with self.lock:
            self.initialize_single_sfp(index)
            return super(Module, self).get_sfp(index)


class DpuModule(ModuleBase):

    def __init__(self, dpu_id):
        super(DpuModule, self).__init__()
        self.dpu_id = dpu_id
        self._name = f"DPU{self.dpu_id}"
        self.dpuctl_obj = DpuCtlPlat(self._name.lower())
        self.dpuctl_obj.setup_logger(use_notice_level=True)
        self.dpuctl_obj.verbosity = True
        self.fault_state = False
        self.dpu_vpd_parser = DpuVpdParser('/var/run/hw-management/eeprom/vpd_data', self.dpuctl_obj._name.upper())
        self.CONFIG_DB_NAME = "CONFIG_DB"
        self.midplane_interface = None
        self.bus_info = None
        self.reboot_base_path = f"/var/run/hw-management/{self.dpuctl_obj._name}/system/"
        self.reboot_cause_map = {
            f'{self.reboot_base_path}reset_aux_pwr_or_reload':
                (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'power auxiliary outage or reload'),
            f'{self.reboot_base_path}reset_comex_pwr_fail':
                (ChassisBase.REBOOT_CAUSE_POWER_LOSS, 'Power failed to comex module'),
            f'{self.reboot_base_path}reset_from_main_board':
                (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Reset from Main board'),
            f'{self.reboot_base_path}reset_dpu_thermal':
                (ChassisBase.REBOOT_CAUSE_THERMAL_OVERLOAD_OTHER, 'Thermal shutdown of the DPU'),
            f'{self.reboot_base_path}reset_pwr_off':
                (ChassisBase.REBOOT_CAUSE_NON_HARDWARE, 'Reset due to Power off'),
        }
        self.chassis_state_db = SonicV2Connector(host="127.0.0.1")
        self.chassis_state_db.connect(self.chassis_state_db.CHASSIS_STATE_DB)

    def get_base_mac(self):
        """
        Retrieves the base MAC address for the module

        Returns:
            A string containing the MAC address in the format
            'XX:XX:XX:XX:XX:XX'
        """
        return self.dpu_vpd_parser.get_dpu_base_mac()

    def get_model(self):
        """
        Retrieves the model number (or part number) of the device

        Returns:
            string: Model/part number of device
        """
        return self.dpu_vpd_parser.get_dpu_model()

    def get_serial(self):
        """
        Retrieves the serial number of the device

        Returns:
            string: Serial number of device
        """
        return self.dpu_vpd_parser.get_dpu_serial()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device

        Returns:
            string: Revision value of device
        """
        return self.dpu_vpd_parser.get_dpu_revision()

    def reboot(self, reboot_type=ModuleBase.MODULE_REBOOT_DPU):
        """
        Request to reboot the module

        Args:
            reboot_type: A string, the type of reboot requested from one of the
            predefined reboot types: MODULE_REBOOT_DEFAULT, MODULE_REBOOT_CPU_COMPLEX,
            MODULE_REBOOT_FPGA_COMPLEX or MODULE_REBOOT_DPU

            MODULE_REBOOT_DPU is only applicable for smartswitch chassis.

        Returns:
            bool: True if the request has been issued successfully, False if not
        """
        logger.log_notice(f"Rebooting {self._name} with type {reboot_type}")
        # no_wait=True is not supported at this point, because of race conditions with other drivers
        return_value = self.dpuctl_obj.dpu_reboot(skip_pre_post=True)
        logger.log_notice(f"Rebooted {self._name} with type {reboot_type} and return value {return_value}")
        return return_value

    def set_admin_state(self, up):
        """
        Request to keep the card in administratively up/down state.
        The down state will power down the module and the status should show
        MODULE_STATUS_OFFLINE.
        The up state will take the module to MODULE_STATUS_FAULT or
        MODULE_STATUS_ONLINE states.

        Args:
            up: A boolean, True to set the admin-state to UP. False to set the
            admin-state to DOWN.

        Returns:
            bool: True if the request has been issued successfully, False if not
        """
        logger.log_notice(f"Setting the admin state for {self._name} to {up}")
        if up:
            if self.dpuctl_obj.dpu_power_on(skip_pre_post=True):
                logger.log_notice(f"Completed the admin state change for {self._name} to {up}")
                return True
            logger.log_error(f"Failed to set the admin state for {self._name}")
            return False
        return_value = self.dpuctl_obj.dpu_power_off(skip_pre_post=True)
        logger.log_notice(f"Completed the admin state change for {self._name} to {up}")
        return return_value

    def get_type(self):
        """
        Retrieves the type of the module.

        Returns:
            A string, the module-type from one of the predefined types:
            MODULE_TYPE_SUPERVISOR, MODULE_TYPE_LINE or MODULE_TYPE_FABRIC
            or MODULE_TYPE_DPU or MODULE_TYPE_SWITCH
        """
        return ModuleBase.MODULE_TYPE_DPU

    def get_name(self):
        """
        Retrieves the type of the module.

        Returns:
            A string, the module-type from one of the predefined types:
            MODULE_TYPE_SUPERVISOR, MODULE_TYPE_LINE or MODULE_TYPE_FABRIC
            or MODULE_TYPE_DPU
        """
        return self._name

    def get_description(self):
        """
        Retrieves the platform vendor's product description of the module

        Returns:
            A string, providing the vendor's product description of the module.
        """
        return "NVIDIA BlueField-3 DPU"

    def get_oper_status(self):
        boot_prog = self.dpuctl_obj.read_boot_prog()
        if boot_prog == BootProgEnum.OS_RUN.value:
            return ModuleBase.MODULE_STATUS_ONLINE
        return ModuleBase.MODULE_STATUS_OFFLINE

    ##############################################
    # SmartSwitch methods
    ##############################################

    def get_dpu_id(self):
        """
        Retrieves the DPU ID. Returns None for non-smartswitch chassis.

        Returns:
            An integer, indicating the DPU ID. DPU0 returns 0, DPUX returns X
            DPU ID can be greater than or equal to 0.
        """
        return self.dpu_id

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot of the DPU module
        Returns:
            A tuple (string, string) where the first element is a string
            containing the cause of the previous reboot. This string must
            be one of the predefined strings in this class. If the first
            string is "REBOOT_CAUSE_HARDWARE_OTHER", the second string can be
            used to pass a description of the reboot cause.
            Some more causes are appended to the existing list to handle other
            modules such as DPUs.
            Ex: REBOOT_CAUSE_POWER_LOSS, REBOOT_CAUSE_HOST_RESET_DPU,
            REBOOT_CAUSE_HOST_POWERCYCLED_DPU, REBOOT_CAUSE_SW_THERMAL,
            REBOOT_CAUSE_DPU_SELF_REBOOT
        """
        for f, rd in self.reboot_cause_map.items():
            if utils.read_int_from_file(f) == 1:
                return rd
        return ChassisBase.REBOOT_CAUSE_NON_HARDWARE, ''

    def get_midplane_ip(self):
        """
        Retrieves the midplane IP-address of the module in a modular chassis
        When called from the Supervisor, the module could represent the
        line-card and return the midplane IP-address of the line-card.
        When called from the line-card, the module will represent the
        Supervisor and return its midplane IP-address.
        When called from the DPU, returns the midplane IP-address of the dpu-card.
        When called from the Switch returns the midplane IP-address of Switch.
        Returns:
            A string, the IP-address of the module reachable over the midplane
        """
        return f"169.254.200.{int(self.dpu_id) + 1}"

    def is_midplane_reachable(self):
        """
        Retrieves the reachability status of the module from the Supervisor or
        of the Supervisor from the module via the midplane of the modular chassis
        Returns:
            A bool value, should return True if module is reachable via midplane
        """
        if not self._is_midplane_up():
            return False
        command = ['ping', '-c', '1', '-W', '1', self.get_midplane_ip()]
        try:
            return subprocess.call(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0
        except Exception as e:
            logger.log_error(f"Failed to check midplane reachability for {self.get_name()}! {e}")
            return False

    def get_midplane_interface(self):
        if not self.midplane_interface:
            # Cache the data to prevent multiple platform.json parsing
            self.midplane_interface = DeviceDataManager.get_dpu_interface(self.get_name().lower(), DpuInterfaceEnum.MIDPLANE_INT.value)
            # If we are unable to parse platform.json for midplane interface raise RunTimeError
            if not self.midplane_interface:
                raise RuntimeError(f"Unable to obtain midplane interface information from platform.json for {self.get_name()}")
        return self.midplane_interface

    def _is_midplane_up(self):
        # If rshim is not activated then the interface does not exist, do not log the error
        return utils.read_int_from_file(f'/sys/class/net/{self.get_midplane_interface()}/carrier', log_func=None) == 1

    def get_pci_bus_info(self):
        """
        Retrieves the bus information.

        Returns:
            Returns the PCI bus information in list of BDF format
        """
        if self.bus_info:
            return self.bus_info
        bus_paths = self.dpuctl_obj.get_pci_dev_path()
        # Convert full paths to BDF format
        self.bus_info = [path.split('/')[-1] for path in bus_paths]
        return self.bus_info

    def pci_detach(self):
        """
        Detaches the DPU PCI device specified by "module_name" on a SmartSwitch.

        Returns: True once the PCI is successfully detached.
        """
        return self.dpuctl_obj.dpu_pre_shutdown()

    def pci_reattach(self):
        """
        Rescans and reconnects the DPU PCI device specified by "module_name" on a SmartSwitch.

        Returns: True once the PCI is successfully reconnected.
        """
        return self.dpuctl_obj.dpu_post_startup()

    def get_hw_mgmt_id(self):
        """
        Obtains the id used by hw-mgmt API, so that we can use different APIs and access sysfs files provided
        """
        return self.dpu_id + 1

    def get_temperature_dict(self):
        """
        This function is used to obtain the TEMPERATURE INFO TABLE from CHASSIS_STATE_DB
        """
        chassis_state_db_name = "CHASSIS_STATE_DB"
        ddr = "DDR"
        nvme = "NVME"
        cpu = "CPU"
        dpu_ddr_temperature_info_table = f"TEMPERATURE_INFO_{self.dpu_id}|{ddr}"
        dpu_cpu_temperature_info_table = f"TEMPERATURE_INFO_{self.dpu_id}|{cpu}"
        dpu_drive_temperature_info_table = f"TEMPERATURE_INFO_{self.dpu_id}|{nvme}"
        return_dict = {}
        try:
            return_dict[ddr] = self.chassis_state_db.get_all(chassis_state_db_name, dpu_ddr_temperature_info_table)
            return_dict[cpu] = self.chassis_state_db.get_all(chassis_state_db_name, dpu_cpu_temperature_info_table)
            return_dict[nvme] = self.chassis_state_db.get_all(chassis_state_db_name, dpu_drive_temperature_info_table)
        except Exception as e:
            logger.log_error(f"Failed to check obtain DPU temperature informatoin for {self.get_name()}! {e}")
            return {}
        return return_dict
