#!/usr/bin/env python

# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

#############################################################################
# PDDF
# Module contains an implementation of SONiC Chassis API
#
#############################################################################

import os
import re
import sys
import time

from sonic_platform.thermal import NexthopFpgaAsicThermal
from sonic_platform.watchdog import Watchdog

try:
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

# Xcvr presence states used by xcvrd
XCVR_INSERTED = "1"
XCVR_REMOVED = "0"

# Sleep duration waiting for change events
CHANGE_EVENT_SLEEP_SECONDS = 1


class Chassis(PddfChassis):
    """
    PDDF Platform-specific Chassis class
    """

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)

        # {'port': 'presence'}
        self._xcvr_presence = {}
        self._watchdog: Watchdog | None = None
        self._pddf_data = pddf_data

        if self._pddf_data:
            num_asic_thermals = self._pddf_data.data.get("PLATFORM", {}).get("num_nexthop_fpga_asic_temp_sensors", 0)
        else:
            num_asic_thermals = 0
        for index in range(num_asic_thermals):
            thermal = NexthopFpgaAsicThermal(index, pddf_data)
            self._thermal_list.append(thermal)

    # Provide the functions/variables below for which implementation is to be overwritten

    def get_model(self):
        """
        Retrieves the model number (or part number) of the chassis
        Returns:
            string: Model/part number of chassis
        """
        return self._eeprom.modelstr()

    def get_revision(self):
        """
        Retrieves the hardware revision of the device
        Returns:
            string: Label Revision value of device
        """
        return self._eeprom.label_revision_str()

    def get_position_in_parent(self):
        """
        Retrieves 1-based relative physical position in parent device. If the agent cannot determine the parent-relative position
        for some reason, or if the associated value of entPhysicalContainedIn is '0', then the value '-1' is returned
        Returns:
            integer: The 1-based relative physical position in parent device or -1 if cannot determine the position
        """
        return -1

    def is_replaceable(self):
        """
        Indicate whether this device is replaceable
        Returns:
            bool: True if it is replaceable
        """
        return False

    def get_sfp(self, index):
        sfp = None

        try:
            # The index starts from 1
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(index, len(self._sfp_list)))
        return sfp

    def _get_xcvr_change_event(self):
        """
        Returns a dictionary containing the change events for all xcvrs.
        """
        change_dict = {}
        for xcvr in self.get_all_sfps():
            presence = XCVR_INSERTED if xcvr.get_presence() else XCVR_REMOVED
            port = str(xcvr.get_position_in_parent())
            if presence != self._xcvr_presence.get(port):
                change_dict[port] = presence
                self._xcvr_presence[port] = presence
        return change_dict

    def get_change_event(self, timeout=0):
        """
        Returns a nested dictionary containing all devices which have
        experienced a change at chassis level

        Args:
            timeout: Timeout in milliseconds (optional). If timeout == 0,
                this method will block until a change is detected.

        Returns:
            (bool, dict):
                - True if the call was successful, False if not;
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
        """
        end_time = time.monotonic() + timeout / 1000 if timeout > 0 else None
        change_events = {}
        while True:
            change_events["sfp"] = self._get_xcvr_change_event()
            if bool(change_events["sfp"]):
                break
            if end_time is not None and time.monotonic() > end_time:
                break
            time.sleep(min(timeout / 1000, CHANGE_EVENT_SLEEP_SECONDS))
        return True, change_events

    # sonic-utilities/show/system_health.py calls this
    def initizalize_system_led(self):
        return True

    def set_status_led(self, color):
        return self.set_system_led("SYS_LED", color)

    def get_status_led(self):
        return self.get_system_led("SYS_LED")

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

        reboot_cause_path = self.plugin_data['REBOOT_CAUSE']['reboot_cause_file']

        try:
            with open(reboot_cause_path, 'r', errors='replace') as fd:
                data = fd.read()
                sw_reboot_cause = data.strip()
        except IOError:
            sw_reboot_cause = "Unknown"

        return ('REBOOT_CAUSE_NON_HARDWARE', sw_reboot_cause)

    def get_watchdog(self) -> Watchdog | None:
        """
        Retrieves hardware watchdog device on this chassis
        Returns:
            An object derived from WatchdogBase representing the hardware
            watchdog device. None if no watchdog is present as defined in pddf_data.
        """
        if self._watchdog is None:
            if not self._pddf_data:
                return None
            watchdog_pddf_obj_data = self._pddf_data.data.get("WATCHDOG")
            if watchdog_pddf_obj_data is None:
                return None
            device_parent_name = watchdog_pddf_obj_data["dev_info"]["device_parent"]
            fpga_pci_addr = self._pddf_data.data[device_parent_name]["dev_info"]["device_bdf"]
            watchdog_dev_attr = watchdog_pddf_obj_data["dev_attr"]
            event_driven_power_cycle_control_reg_offset = int(
                watchdog_dev_attr["event_driven_power_cycle_control_reg_offset"], 16
            )
            watchdog_counter_reg_offset = int(watchdog_dev_attr["watchdog_counter_reg_offset"], 16)
            self._watchdog = Watchdog(
                fpga_pci_addr=fpga_pci_addr,
                event_driven_power_cycle_control_reg_offset=event_driven_power_cycle_control_reg_offset,
                watchdog_counter_reg_offset=watchdog_counter_reg_offset,
            )

        return self._watchdog

    ##############################################################
    ################## ThermalManager methods ####################
    ##############################################################

    def get_thermal_manager(self):
        from .thermal_manager import ThermalManager
        return ThermalManager
