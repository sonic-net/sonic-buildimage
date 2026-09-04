#!/usr/bin/env python

#############################################################################
# Ciena Chassis
#
#
# Each LED supports: brightness (0/1), blink (0/1), max_brightness
#
# System LED names recognised by set_system_led / get_system_led:
#   STATUS  — front-panel status indicator (green / green_blink / off)
#   ALARM   — front-panel alarm  indicator (amber / amber_blink / off)
#   PSA     — PSU-A power-good (read-only, green / off)
#   PSB     — PSU-B power-good (read-only, green / off)
#   SYNC    — timing-sync (green / red / yellow / *_blink / off)
#############################################################################

import json
import logging
import os
import select
import sys
import re
import time

try:
    from sonic_platform_pddf_base.pddf_chassis import PddfChassis
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = logging.getLogger(__name__)

# SFP event status values (must match xcvrd/sfp_status_helper.py)
SFP_STATUS_INSERTED = '1'
SFP_STATUS_REMOVED  = '0'

# Max warnings logged for change-event poll failures before going silent
CHG_EVT_MAX_WARNINGS = 5


def _get_pddf_json(pddf_obj):
    """Return parsed PDDF JSON dict from PDDF object (or dict input)."""
    if pddf_obj is None:
        return {}
    pddf_json = getattr(pddf_obj, "data", pddf_obj)
    return pddf_json if isinstance(pddf_json, dict) else {}


def _get_platform_attr(pddf_obj, attr_name):
    """Get PLATFORM-level attribute from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    platform_data = pddf_json.get("PLATFORM", {})
    if isinstance(platform_data, dict):
        return platform_data.get(attr_name)
    return None


def _get_led_attr(pddf_obj, attr_name):
    """Get LED.dev_attr value from PDDF JSON."""
    pddf_json = _get_pddf_json(pddf_obj)
    led_data = pddf_json.get("LED", {}).get("dev_attr", {})
    if isinstance(led_data, dict):
        return led_data.get(attr_name)
    return None

class Chassis(PddfChassis):
    """
    Ciena Platform-specific Chassis class.

    Inherits from PddfChassis to get PDDF-based SFP management
    (EEPROM, presence via GPIO, change events).  Overrides fans,
    PSUs, thermals, EEPROM, and components with custom Ciena
    implementations that talk to the FPGA / BMC / IPMI directly.
    """

    def __init__(self, pddf_data=None, pddf_plugin_data=None):
        # Let PddfChassis.__init__() load pddf-device.json, create
        # the SFP list (40 ports), and populate pddf_obj/plugin_data.
        # It will also try to create FanDrawer/Psu/Thermal/Eeprom
        # objects — our custom classes accept (and ignore) the PDDF
        # arguments so this works without error.
        PddfChassis.__init__(self, pddf_data, pddf_plugin_data)
        # Ensure we reference the PDDF objects actually loaded by PddfChassis
        # (PddfChassis.__init__ will load them if None was passed in).
        self._pddf_data = self.pddf_obj
        self._pddf_plugin_data = self.plugin_data
        self._gpio_sysfs_base = _get_platform_attr(
            self._pddf_data, "gpio_sysfs_base"
        )

        # Cache LED paths on this chassis instance.
        self._led_master = _get_led_attr(self._pddf_data, "LED_MASTER")
        self._led_status_grn = _get_led_attr(self._pddf_data, "LED_STATUS_GRN")
        self._led_alarm_ylw = _get_led_attr(self._pddf_data, "LED_ALARM_YLW")
        self._led_sync_grn = _get_led_attr(self._pddf_data, "LED_SYNC_GRN")
        self._led_sync_red = _get_led_attr(self._pddf_data, "LED_SYNC_RED")
        self._led_sync_ylw = _get_led_attr(self._pddf_data, "LED_SYNC_YLW")
        self._led_psa = _get_led_attr(self._pddf_data, "LED_PSA")
        self._led_psb = _get_led_attr(self._pddf_data, "LED_PSB")

        # Now replace the PDDF-created fan/PSU/thermal/eeprom/component
        # objects with our custom Ciena implementations.
        # SFPs are kept as-is (PDDF-managed).
        self._fan_list = []
        self._fan_drawer_list = []
        self.__initialize_fan()

        self.__initialize_eeprom()

        self._psu_list = []
        self.__initialize_psu()

        self._thermal_list = []
        self.__initialize_thermals()

        self._component_list = []
        self.__initialize_components()

        self._voltage_sensor_list = []
        self.__initialize_voltage_sensors()

        self._current_sensor_list = []
        self.__initialize_current_sensors()

        self.__initialize_watchdog()

    
    def get_sfp(self, index):
        """
        Retrieve SFP object by (1-based) physical port index.

        The platform SFP utility for this device uses 1-based physical
        port numbering (1..N). Translate to the internal 0-based
        `_sfp_list` index to return the correct SFP object.
        """
        sfp = None

        try:
            if index == 0:
                raise IndexError
            sfp = self._sfp_list[index - 1]
        except IndexError:
            sys.stderr.write("SFP index {} out of range (1-{})\n".format(
                index, len(self._sfp_list)))

        return sfp

    # ------------------------------------------------------------------
    # Change-event detection via ISR_PWR_GD_CHG GPIOs
    # ------------------------------------------------------------------

    def _build_isr_gpio_map(self):
        """
        Build a dict mapping 1-based port index → ISR GPIO sysfs edge path.

        The Ciena FPGA driver exports named GPIO nodes:
          Ports  1-36 : SFP_ISR_PWR_GD_CHG_0  … SFP_ISR_PWR_GD_CHG_35
          Ports 37-40 : QSFP_ISR_PWR_GD_CHG_0 … QSFP_ISR_PWR_GD_CHG_3

        We read the gpio_isr_name field from pddf-device.json dev_attr.
        """
        isr_map = {}
        if self.pddf_obj is None:
            return isr_map

        pddf_data_dict = getattr(self.pddf_obj, 'data', self.pddf_obj)
        if not isinstance(pddf_data_dict, dict):
            return isr_map

        num_ports = len(self._sfp_list)
        for port_idx in range(1, num_ports + 1):
            port_key = "PORT{}".format(port_idx)
            try:
                port_data = pddf_data_dict.get(port_key, {})
                isr_name = port_data.get("dev_attr", {}).get("gpio_isr_name", None)
                if isr_name and self._gpio_sysfs_base is not None:
                    edge_path = os.path.join(self._gpio_sysfs_base, isr_name, "edge")
                    value_path = os.path.join(self._gpio_sysfs_base, isr_name, "value")
                    if os.path.exists(value_path):
                        isr_map[port_idx] = value_path
                        # Ensure edge is set to "both" for poll()-based detection
                        try:
                            with open(edge_path, 'w') as f:
                                f.write("both")
                        except (IOError, OSError):
                            pass
            except Exception:
                pass
        return isr_map

    def get_change_event(self, timeout=0):
        """
        Detect SFP insert/remove events by polling presence GPIOs.

        Cache the last-known presence state per port and
        compare against the current state.  Only genuine transitions
        (present→absent or absent→present) are reported as events.

        Args:
            timeout: milliseconds to wait for an event.
                     0 = block indefinitely until at least one event fires.

        Returns:
            (True, {'sfp': {port_str: status_str}})
            where status_str = '1' (inserted) or '0' (removed).
        """
        # Build ISR map on first call
        if not hasattr(self, '_isr_gpio_map'):
            self._isr_gpio_map = self._build_isr_gpio_map()

        isr_map = self._isr_gpio_map
        # If no ISR GPIO sysfs paths exist, fall back to polling all ports
        # directly.  Raising NotImplementedError here would cause xcvrd to
        # fall through to the legacy platform_sfputil path, which is None on
        # new-style platform-API chassis and results in an AttributeError that
        # kills xcvrd (SIGKILL loop → FATAL state).
        if isr_map:
            poll_ports = list(isr_map.keys())
        else:
            poll_ports = list(range(1, len(self._sfp_list) + 1))

        # Initialise the presence cache on first call by reading all ports
        if not hasattr(self, '_prev_presence'):
            self._prev_presence = {}
            for port_idx in poll_ports:
                sfp = self.get_sfp(port_idx)
                if sfp is not None:
                    try:
                        self._prev_presence[port_idx] = sfp.get_presence()
                    except Exception:
                        self._prev_presence[port_idx] = False
                else:
                    self._prev_presence[port_idx] = False

        sfp_events = {}
        deadline = None
        if timeout > 0:
            deadline = time.monotonic() + timeout / 1000.0

        while True:
            # --- Poll actual presence state and compare to cache ---
            for port_idx in poll_ports:
                try:
                    sfp = self.get_sfp(port_idx)
                    if sfp is not None:
                        current = sfp.get_presence()
                    else:
                        current = False

                    prev = self._prev_presence.get(port_idx, False)
                    if current != prev:
                        self._prev_presence[port_idx] = current
                        sfp_events[str(port_idx)] = (
                            SFP_STATUS_INSERTED if current else SFP_STATUS_REMOVED
                        )
                except Exception as e:
                    if not hasattr(self, '_chg_evt_err_rem'):
                        self._chg_evt_err_rem = CHG_EVT_MAX_WARNINGS
                    if self._chg_evt_err_rem:
                        self._chg_evt_err_rem -= 1
                        logger.warning("get_change_event: port %d presence "
                                       "poll failed: %s", port_idx, e)

            if sfp_events:
                return True, {'sfp': sfp_events}

            # No events yet — check timeout
            if deadline is not None:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    return True, {'sfp': {}}
                time.sleep(min(0.5, remaining))
            else:
                # Block mode: sleep and re-poll
                time.sleep(0.5)

    def __initialize_components(self):
        from sonic_platform.components import Component
        num_components = self.platform_inventory.get('num_components', 0)
        for idx in range(num_components):
            comp = Component(idx, self._pddf_data, self._pddf_plugin_data)
            self._component_list.append(comp)

    def __initialize_fan(self):
        from sonic_platform.fan_drawer import FanDrawer
        num_fan_tray = self.platform_inventory.get('num_fantrays', 0)
        for fant_index in range(num_fan_tray):
            fandrawer = FanDrawer(fant_index, self._pddf_data, self._pddf_plugin_data)
            self._fan_drawer_list.append(fandrawer)
            self._fan_list.extend(fandrawer._fan_list)

    def __initialize_eeprom(self):
        from sonic_platform.eeprom import Eeprom
        self._eeprom = Eeprom(self._pddf_data, self._pddf_plugin_data)



    def __initialize_psu(self):
        from sonic_platform.psu import Psu
        num_psus = self.platform_inventory.get('num_psus', 0)
        for index in range(0, num_psus):
            psu = Psu(index, self._pddf_data, self._pddf_plugin_data)
            self._psu_list.append(psu)



    def __initialize_thermals(self):
        from sonic_platform.thermal import Thermal
        num_thermal = self.platform_inventory.get('num_temps', 0)
        for index in range(num_thermal):
            thermal = Thermal(index, self._pddf_data, self._pddf_plugin_data, False, 0)
            self._thermal_list.append(thermal)

    def __initialize_voltage_sensors(self):
        """Initialize all voltage sensors from pddf-device.json.

        Sources:
          1. MAX1139 ADC (IIO sysfs) — board-level 12 V rails (VOLTAGE1-3)
          2. FPGA PMBus VI_MON — VRM telemetry with direct FPGA reads (VOLTAGE4-12)

        Both types are now handled uniformly by VoltageSensor, which detects
        VRM entries via vrm_device_name metadata in pddf-device.json.
        """
        try:
            from sonic_platform.voltage_sensor import VoltageSensor
            # Initialize all voltage sensors from PDDF inventory
            num_sensors = self.platform_inventory.get('num_voltage_sensors', 0)
            for index in range(num_sensors):
                sensor = VoltageSensor(index, self._pddf_data, self._pddf_plugin_data)
                self._voltage_sensor_list.append(sensor)
        except Exception as e:
            logger.warning("Failed to initialize voltage sensors: %s", e)

    def __initialize_current_sensors(self):
        """Initialize current sensors from MAX11127 optics power ADC via FPGA.

        The MAX11127 is a 16-channel 12-bit SPI ADC accessed through FPGA
        registers.  Each channel measures optics module supply current.

        Scale factors convert ADC millivolts to milliamps:
          QSFPDD (CH0-3):  4.927 mA/mV
          SFP28  (CH4-12): 2.857 mA/mV
        """
        try:
            from sonic_platform.current_sensor import CurrentSensor
            num_sensors = self.platform_inventory.get('num_current_sensors', 0)
            for index in range(num_sensors):
                sensor = CurrentSensor(index, self._pddf_data, self._pddf_plugin_data)
                self._current_sensor_list.append(sensor)
        except Exception as e:
            logger.warning("Failed to initialize FPGA ADC current sensors: %s", e)

    def __initialize_watchdog(self):
        """Initialize the BMC IPMI watchdog."""
        try:
            from sonic_platform.watchdog import Watchdog
            self._watchdog = Watchdog(self.pddf_obj)
        except Exception as e:
            logger.warning("Failed to initialize watchdog: %s", e)

    # ------------------------------------------------------------------
    # LED helpers
    # ------------------------------------------------------------------

    def initialize_system_leds(self):
        """Ensure the front-panel all-on override is cleared so individual LEDs show.

        ``front::all`` maps to SUTRA_GLUE_LED_SYS_STATUS_0.enable_all_leds, which is
        an ALL-ON lamp-test override: brightness=1 forces every front-panel LED.
        Ensure this is off.
        This is a privileged write to root-owned LED sysfs nodes and is therefore
        intended to be called by the root ``ledd`` daemon (via ciena/led_control.py)
        at start-up -- deliberately NOT from __init__, so that unprivileged callers
        (e.g. the ``show`` CLI) do not attempt the write and emit EACCES
        permission-denied warnings.
        """
        return self._write_led(self._led_master, "brightness", "0")

    @staticmethod
    def _write_led(led_path, attr, value):
        """Write *value* to /sys/class/leds/<led>/<attr>."""
        if led_path is None:
            return False
        path = os.path.join(led_path, attr)
        try:
            with open(path, "w") as f:
                f.write(str(value))
            return True
        except OSError as e:
            logger.warning("Failed to write %s to %s: %s", value, path, e)
            return False

    @staticmethod
    def _read_led(led_path, attr):
        """Read /sys/class/leds/<led>/<attr>, return stripped string or None."""
        if led_path is None:
            return None
        path = os.path.join(led_path, attr)
        try:
            with open(path, "r") as f:
                return f.read().strip()
        except OSError:
            return None

    @staticmethod
    def _set_led_on(led_path, blink=False):
        """Turn a single LED sysfs node ON, optionally blinking."""
        ok  = Chassis._write_led(led_path, "brightness", "1")
        ok &= Chassis._write_led(led_path, "blink", "1" if blink else "0")
        return ok

    @staticmethod
    def _set_led_off(led_path):
        """Turn a single LED sysfs node OFF (brightness=0, blink=0)."""
        ok  = Chassis._write_led(led_path, "blink", "0")
        ok &= Chassis._write_led(led_path, "brightness", "0")
        return ok

    # ------------------------------------------------------------------
    # System status LED  (SONiC thermalctld / health-mon uses these)
    # ------------------------------------------------------------------

    def set_status_led(self, color):
        """Set the front-panel system status LED.

        This controls two physical LEDs (STATUS green + ALARM yellow)
        as a pair to represent overall system health.

        Supported colours / states:
            green        -> STATUS on,  ALARM off       (healthy)
            green_blink  -> STATUS blinking, ALARM off  (booting)
            amber        -> STATUS on,  ALARM on        (warning)
            amber_blink  -> STATUS on,  ALARM blinking  (warning)
            red          -> STATUS off, ALARM on        (critical)
            red_blink    -> STATUS off, ALARM blinking  (critical)
            off          -> STATUS off, ALARM off

        Args:
            color: A string -- one of the values above.
        Returns:
            bool: True on success.
        """
        color = (color or "off").lower()

        if color == self.STATUS_LED_COLOR_GREEN:
            ok  = self._set_led_on(self._led_status_grn, blink=False)
            ok &= self._set_led_off(self._led_alarm_ylw)
        elif color == "green_blink":
            ok  = self._set_led_on(self._led_status_grn, blink=True)
            ok &= self._set_led_off(self._led_alarm_ylw)
        elif color == self.STATUS_LED_COLOR_AMBER:
            # Warning: STATUS green stays on, ALARM yellow on
            ok  = self._set_led_on(self._led_status_grn, blink=False)
            ok &= self._set_led_on(self._led_alarm_ylw, blink=False)
        elif color == "amber_blink":
            # Warning blinking: STATUS green stays on, ALARM yellow blinking
            ok  = self._set_led_on(self._led_status_grn, blink=False)
            ok &= self._set_led_on(self._led_alarm_ylw, blink=True)
        elif color == self.STATUS_LED_COLOR_RED:
            # Critical: STATUS green off, ALARM yellow on
            ok  = self._set_led_off(self._led_status_grn)
            ok &= self._set_led_on(self._led_alarm_ylw, blink=False)
        elif color == "red_blink":
            # Critical blinking: STATUS green off, ALARM yellow blinking
            ok  = self._set_led_off(self._led_status_grn)
            ok &= self._set_led_on(self._led_alarm_ylw, blink=True)
        elif color == self.STATUS_LED_COLOR_OFF:
            ok  = self._set_led_off(self._led_status_grn)
            ok &= self._set_led_off(self._led_alarm_ylw)
        else:
            logger.warning("set_status_led: unsupported color '%s'", color)
            return False

        return ok

    def initizalize_system_led(self):
        """Called by show system-health CLI to initialise LEDs.

        Note: the method name is intentionally misspelled to match
        the SONiC show CLI expectation (upstream typo).
        """
        return

    def get_status_led(self):
        """Get the current front-panel system status LED colour.

        Returns:
            A string: 'green', 'green_blink', 'amber', 'amber_blink', or 'off'.
        """
        grn     = self._read_led(self._led_status_grn, "brightness")
        grn_blk = self._read_led(self._led_status_grn, "blink")
        ylw     = self._read_led(self._led_alarm_ylw,  "brightness")
        ylw_blk = self._read_led(self._led_alarm_ylw,  "blink")

        if grn == "1":
            if ylw == "1":
                # Both on = warning (amber)
                return "amber_blink" if ylw_blk == "1" else self.STATUS_LED_COLOR_AMBER
            return "green_blink" if grn_blk == "1" else self.STATUS_LED_COLOR_GREEN
        elif ylw == "1":
            # Yellow on without green = critical (red)
            return "red_blink" if ylw_blk == "1" else self.STATUS_LED_COLOR_RED
        else:
            return self.STATUS_LED_COLOR_OFF

    # ------------------------------------------------------------------
    # SYNC multi-colour LED
    # ------------------------------------------------------------------

    def _set_sync_led(self, color):
        """Set the SYNC multi-colour LED.

        The SYNC indicator has three sysfs nodes (green, red, yellow).
        They share the same FPGA register bits -- only one colour node
        should be active at a time.

        Supported:
            green / green_blink
            red   / red_blink
            yellow / amber / yellow_blink / amber_blink
            off
        """
        color = (color or "off").lower()
        blink = "_blink" in color
        base  = color.replace("_blink", "")

        # Turn all sync colour nodes off first
        self._set_led_off(self._led_sync_grn)
        self._set_led_off(self._led_sync_red)
        self._set_led_off(self._led_sync_ylw)

        if base == "green":
            return self._set_led_on(self._led_sync_grn, blink=blink)
        elif base == "red":
            return self._set_led_on(self._led_sync_red, blink=blink)
        elif base in ("yellow", "amber"):
            return self._set_led_on(self._led_sync_ylw, blink=blink)
        elif base == "off":
            return True   # already off
        else:
            logger.warning("_set_sync_led: unsupported color '%s'", color)
            return False

    def _get_sync_led(self):
        """Get the current SYNC LED colour/state."""
        for path, name in [(self._led_sync_grn, "green"),
                   (self._led_sync_red, "red"),
                   (self._led_sync_ylw, "yellow")]:
            if self._read_led(path, "brightness") == "1":
                blk = self._read_led(path, "blink")
                return "{}_blink".format(name) if blk == "1" else name
        return "off"

    # ------------------------------------------------------------------
    # Named system / port LED  (ledd / led_control.py calls these)
    # ------------------------------------------------------------------

    def set_system_led(self, led_device_name, color):
        """Set a named system or port LED.

        Recognised LED names (case-insensitive):
            STATUS -- front-panel status (green / green_blink / off)
            ALARM  -- front-panel alarm  (amber / amber_blink / off)
            PSA    -- PSU-A OK           (hardware-driven, read-only)
            PSB    -- PSU-B OK           (hardware-driven, read-only)
            SYNC   -- sync indicator     (green / red / yellow / *_blink / off)

        For port LEDs (PORT_LED_*), not yet implemented -- returns True.

        Args:
            led_device_name: LED identifier string.
            color: colour/state string.
        Returns:
            bool: True on success.
        """
        name  = (led_device_name or "").upper()
        color = (color or "off").lower()

        if name == "STATUS":
            # STATUS controls the green LED only (no alarm pairing)
            blink = "_blink" in color
            base  = color.replace("_blink", "")
            if base == "green":
                return self._set_led_on(self._led_status_grn, blink=blink)
            elif base == "off":
                return self._set_led_off(self._led_status_grn)
            else:
                logger.warning("set_system_led(STATUS): unsupported color '%s'", color)
                return False

        elif name == "ALARM":
            blink = "_blink" in color
            base  = color.replace("_blink", "")
            if base in ("amber", "yellow", "red"):
                return self._set_led_on(self._led_alarm_ylw, blink=blink)
            elif base == "off":
                return self._set_led_off(self._led_alarm_ylw)
            else:
                logger.warning("set_system_led(ALARM): unsupported color '%s'", color)
                return False

        elif name == "SYNC":
            return self._set_sync_led(color)

        elif name in ("PSA", "PSB"):
            # PSU LEDs are hardware-driven (read-only from software)
            logger.debug("set_system_led(%s): hardware-driven, ignoring", name)
            return True

        elif name.startswith("PORT_LED_"):
            # Port LEDs -- controlled via FPGA registers, not yet implemented
            logger.debug("set_system_led(%s, %s): port LED not yet implemented",
                          led_device_name, color)
            return True

        else:
            logger.warning("set_system_led: unknown LED '%s'", led_device_name)
            return False

    def get_system_led(self, led_device_name):
        """Get the current state of a named system LED.

        Args:
            led_device_name: LED identifier string.
        Returns:
            str: colour/state string, or "N/A" if unknown.
        """
        name = (led_device_name or "").upper()

        if name == "STATUS":
            br  = self._read_led(self._led_status_grn, "brightness")
            blk = self._read_led(self._led_status_grn, "blink")
            if br == "1":
                return "green_blink" if blk == "1" else "green"
            return "off"

        elif name == "ALARM":
            br  = self._read_led(self._led_alarm_ylw, "brightness")
            blk = self._read_led(self._led_alarm_ylw, "blink")
            if br == "1":
                return "amber_blink" if blk == "1" else "amber"
            return "off"

        elif name == "SYNC":
            return self._get_sync_led()

        elif name == "PSA":
            return "green" if self._read_led(self._led_psa, "brightness") == "1" else "off"

        elif name == "PSB":
            return "green" if self._read_led(self._led_psb, "brightness") == "1" else "off"

        else:
            return "N/A"

    def get_name(self):
        return self._eeprom.get_model() if self._eeprom else "N/A"

    def get_presence(self):
        return True

    def get_model(self):
        return self._eeprom.get_part_number() if self._eeprom else "N/A"

    def get_serial(self):
        return self._eeprom.get_serial() if self._eeprom else "N/A"

    def get_revision(self):
        return self._eeprom.get_revision() if self._eeprom else "N/A"

    def get_status(self):
        return True

    def get_base_mac(self):
        return self._eeprom.get_base_mac() if self._eeprom else "N/A"

    def get_system_eeprom_info(self):
        return self._eeprom.system_eeprom_info() if self._eeprom else {}

    def get_reboot_cause(self):
        return (PddfChassis.REBOOT_CAUSE_NON_HARDWARE, None)

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False
