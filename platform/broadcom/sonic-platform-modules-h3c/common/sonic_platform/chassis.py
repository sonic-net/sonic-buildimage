#!/usr/bin/env python
"""
Version: 1.0

H3C B40X0
Module contains an implementation of SONiC Platform Base API and
provides the Chassis information
"""

try:
    import os
    import time
    from sonic_platform_base.chassis_byted import ChassisByted
    from sonic_platform.sfp import Sfp
    from sonic_platform.eeprom import Eeprom
    from sonic_platform.fan import Fan
    from sonic_platform.psu import Psu
    from sonic_platform.thermal import Thermal
    from sonic_platform.component import Component
    from sonic_platform.watchdog import Watchdog
    from sonic_platform.sensor import Sensor
    from vendor_sonic_platform.devcfg import Devcfg
except ImportError as import_error:
    raise ImportError(str(import_error) + "- required module not found")


class Chassis(ChassisByted):
    """
    Platform-specific Chassis class
    """
    sfp_control = ""
    _port_base_path = {}

    def __init__(self, dev_list):
        super(Chassis, self).__init__()
        fan_list = list()
        thermal_list = list()
        psu_list = list()
        sfp_list = list()
        component_list = list()
        module_list = list()
        sensor_list = list()

        if 'syseeprom' in dev_list:
            syseeprom = Eeprom()
            self.log_debug('syseeprom inited.')
        else:
            syseeprom = None

        if 'watchdog' in dev_list:
            watchdog = Watchdog()
            self.log_debug('watchdog inited.')
        else:
            watchdog = None

        if 'fan' in dev_list:
            for index in range(0, Devcfg.FAN_NUM):
                fan_list.append(Fan(index))
                self.log_debug('fan{} inited.'.format(index))

        if 'thermal' in dev_list:
            for index in range(0, Devcfg.THERMAL_NUM):
                thermal_list.append(Thermal(index))
                self.log_debug('thermal{} inited.'.format(index))

        if 'psu' in dev_list:
            for index in range(0, Devcfg.PSU_NUM):
                psu_list.append(Psu(index))
                self.log_debug('psu{} inited.'.format(index))

        if 'sfp' in dev_list:
            if hasattr(Devcfg, 'SFP_START'):
                SFP_PORT_LIST = range(Devcfg.SFP_START, Devcfg.SFP_END)
                for index in SFP_PORT_LIST:
                    sfp_list.append(Sfp(index, self, 'DSFP'))
                    self.log_debug('sfp{} inited.'.format(index))
            if hasattr(Devcfg, 'QSFP_START'):
                QSFP_PORT_LIST = range(Devcfg.QSFP_START, Devcfg.QSFP_END)
                for index in QSFP_PORT_LIST:
                    sfp_list.append(Sfp(index, self, 'QSFP-DD'))
                    self.log_debug('sfp{} inited.'.format(index))

        if 'component' in dev_list:
            for index in range(0, Devcfg.COMPONENT_NUM):
                component_list.append(Component(index))
                self.log_debug('component{} inited.'.format(index))

        if 'sensor' in dev_list:
            for index in range(0, Devcfg.SENSOR_NUM):
                sensor_list.append(Sensor(index))
                self.log_debug('sensor{} inited.'.format(index))

        self.device_init(component_list, fan_list, psu_list, thermal_list,
                         sfp_list, watchdog, syseeprom, module_list, sensor_list)

    def clr_history_reboot_cause(self):
        with open(Devcfg.HW_CLR_RST, 'w') as clear:
            self.log_debug('Clear the hardware reboot recode')
            clear.write("1")

    def get_reboot_cause(self):
        """
        Retrieves the cause of the previous reboot
        """
        retry = 60
        board_cpld_path = Devcfg.DEBUG_CPLD_DIR + 'board_cpld'
        while retry:
            if os.path.exists(Devcfg.CPU_CPLD_PATH):
                break
            else:
                retry -= 1
                time.sleep(1)
        reboot_cause = self.REBOOT_CAUSE_NON_HARDWARE
        reset_reboot_cause = 'Reset Button Shutdown'
        # add cpucpld_reboot_cause_mask : cpu thermal overload not supportted in current cpu cpld version,
        #                                 mask it.
        cpucpld_reboot_cause_mask = 0x7f
        try:
            reboot_reg_value = int(self.read_attr(Devcfg.REBOOT_CAUSE_CPLD_PATH)[2:], 16) & cpucpld_reboot_cause_mask
            reboot_his_reg_value = int(self.read_attr(Devcfg.LAST_REBOOT_CAUSE_CPLD_PATH)[2:], 16) \
                & cpucpld_reboot_cause_mask
            reset_reg_value = int(self.read_attr(board_cpld_path).split()[-1], 16) & cpucpld_reboot_cause_mask
            self.log_debug(
                'Reboot reg: {}, Last reboot reg: {}, reset_reg_value:{}'.format(
                    reboot_reg_value,
                    reboot_his_reg_value,
                    reset_reg_value))

            if reset_reg_value & 0x01:
                os.system("echo 0xff:0x00 > {path}; echo 0xff:0x02 > {path}".format(path=board_cpld_path))
                self.clr_history_reboot_cause()
                return (reset_reboot_cause, None)

            if reboot_his_reg_value == 0x00:
                self.clr_history_reboot_cause()
                return (reboot_cause, None)

            if reboot_reg_value & 0x08:
                self.log_error("BIOS flash chip master&slave switchover happened! Before switchover,\
                    reboot cause register 0x21 = {}".format(reboot_his_reg_value))
                reboot_reg_value = (reboot_his_reg_value & 0xf7)

            reg_map = {
                0x02: self.REBOOT_CAUSE_WATCHDOG,
                # 0x08: self.REBOOT_CAUSE_SW,
                0x04: self.REBOOT_CAUSE_THERMAL_OVERLOAD_ASIC,
                0x40: self.REBOOT_CAUSE_POWER_LOSS,
                0x80: self.REBOOT_CAUSE_THERMAL_OVERLOAD_CPU
            }
            for reg, cause in reg_map.items():
                if reboot_reg_value & reg:
                    reboot_cause = cause
        except Exception as error:
            self.log_error('Get hardware reboot cause error: {}'.format(error))

        self.log_debug('Reboot cause: {}'.format(reboot_cause))
        self.clr_history_reboot_cause()
        return (reboot_cause, None)

    def get_thermal_manager(self):
        """
        Retrieves the class of thermal manager
        Returns:
            An Class of ThermalManager
        """

        return None
