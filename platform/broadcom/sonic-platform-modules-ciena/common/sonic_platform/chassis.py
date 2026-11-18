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

    # -------------------------------------------------------------------
    # Reboot cause
    #
    # A genuine cold power-cycle must be reported as POWER_LOSS while an
    # ordinary software `reboot` must defer to the software-recorded cause
    # (e.g. "User issued 'reboot'").  The RUDRA40 FPGA "reset history" register
    # ring is documented "Reserved for future use" in the vendor regmap and was
    # found unreliable in practice (its entry 0 is a stuck first-power-on
    # marker), so it is NOT used here.  The standard IPMI chassis power-event
    # primitives ("Last Power Event", restart_cause) are unimplemented on this
    # BMC, so they are not used either.
    #
    # Discriminator: the BMC is standby/battery powered and logs supply-rail
    # threshold crossings in its System Event Log.  A true loss of INPUT power
    # collapses the BMC's own standby rail (P3V3_BMC_BATT) toward 0V; a software
    # reboot leaves the BMC powered and logs NO such event.  So a standby-rail
    # collapse in the SEL, correlated in time with THIS boot, means power loss.
    #
    # get_reboot_cause() decision order:
    #   0. Warm/fast boot (per /proc/cmdline) -> NON_HARDWARE (kexec: no HW
    #      reset; defer to the software/cmdline cause).
    #   1. A BMC-SEL standby-rail collapse whose recovery correlates with this
    #      boot (and has not already been consumed by an earlier boot) ->
    #      POWER_LOSS, annotated with the power-loss time in the "[Time: ...]"
    #      form that determine-reboot-cause surfaces into the Time column.
    #   2. A latched BMC watchdog-timer expiration flag -> WATCHDOG (the flag is
    #      then cleared so it is reported for one boot only).
    #   3. Otherwise -> NON_HARDWARE (defer to the software cause).
    # -------------------------------------------------------------------

    # A power-loss-induced boot occurs shortly after the BMC observes the rails
    # recovering; anything older belongs to an earlier boot that has since been
    # soft-rebooted past (soft reboots add no SEL power events).  The negative
    # slack absorbs small BMC/host clock skew; the upper bound covers a slow
    # BIOS/POST while still excluding stale events.
    _POWER_LOSS_CLOCK_SLACK_S = 60
    _POWER_LOSS_BOOT_WINDOW_S = 900  # 15 min

    # Persistent (survives a power cycle) marker recording the last power event
    # that was attributed to a boot, so a power-cycle boot followed shortly by a
    # software reboot is not re-classified as power loss, and so the result is
    # stable if get_reboot_cause() is called more than once per boot.
    _POWER_EVENT_MARKER = "/host/reboot-cause/platform/ciena_power_event"

    @staticmethod
    def _is_warmfast_reboot():
        """True if this boot was a warm/fast reboot (kexec, no HW reset).

        Warm/fast reboots perform no hardware reset and no cold power-on, so a
        power-loss classification must never be attributed to them; defer to the
        software cause.  Matches the SONIC_BOOT_TYPE values set by the
        sonic-utilities warm/fast-reboot scripts (warm|fast).
        """
        try:
            with open("/proc/cmdline") as f:
                cmdline = f.read()
        except OSError:
            return False
        return re.search(r"SONIC_BOOT_TYPE=(warm|fast)", cmdline) is not None

    @staticmethod
    def _boot_epoch():
        """Kernel boot time as a UTC epoch int (from /proc/stat btime), or None."""
        try:
            with open("/proc/stat") as f:
                for line in f:
                    if line.startswith("btime"):
                        return int(line.split()[1])
        except (OSError, ValueError, IndexError):
            pass
        return None

    def _read_power_marker(self):
        """Return the persisted power-event marker dict, or None. Never raises."""
        try:
            with open(self._POWER_EVENT_MARKER) as f:
                data = json.load(f)
            if isinstance(data, dict):
                return data
        except (OSError, ValueError):
            pass
        return None

    def _write_power_marker(self, marker):
        """Atomically persist the power-event marker. Return True on success.

        Writes a sibling temp file and renames it into place so a reader never
        observes a partial marker.  Never raises.
        """
        path = self._POWER_EVENT_MARKER
        tmp = path + ".tmp"
        try:
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(tmp, "w") as f:
                json.dump(marker, f)
                f.flush()
                os.fsync(f.fileno())
            os.replace(tmp, path)
            return True
        except OSError:
            try:
                os.remove(tmp)
            except OSError:
                pass
            return False

    def get_reboot_cause(self):
        # A warm/fast reboot is a kexec with no hardware reset or power event.
        if self._is_warmfast_reboot():
            return (PddfChassis.REBOOT_CAUSE_NON_HARDWARE, None)

        # 1. A genuine cold power-cycle (BMC standby-rail collapse).
        result = self._check_power_loss()
        if result is not None:
            return result

        # 2. A BMC watchdog-timer expiration (latched expiration flags).
        result = self._check_watchdog()
        if result is not None:
            return result

        # 3. Nothing decisive: defer to the software-recorded cause.
        return (PddfChassis.REBOOT_CAUSE_NON_HARDWARE, None)

    def _check_power_loss(self):
        """Return a POWER_LOSS (cause, description) tuple for a BMC-observed cold
        power-cycle correlated with this boot, or None otherwise."""
        ev = self._last_power_event()
        # No BMC/SEL evidence of a standby-rail collapse, or its time could not
        # be validated -> not a power loss.
        if not ev or ev.get("restore_epoch") is None:
            return None

        restore_epoch = ev["restore_epoch"]
        boot_epoch = self._boot_epoch()
        marker = self._read_power_marker() or {}

        # Idempotent replay: if this exact power event was already evaluated for
        # this boot, reproduce that decision regardless of how many times
        # get_reboot_cause() is called.
        if (marker.get("restore_epoch") == restore_epoch
                and boot_epoch is not None
                and marker.get("boot_epoch") is not None
                and abs(boot_epoch - marker["boot_epoch"])
                <= self._POWER_LOSS_CLOCK_SLACK_S):
            return (self._power_loss_result(ev)
                    if marker.get("classified") == "POWER_LOSS" else None)

        # A previously-unseen power event that correlates with this boot is a
        # real cold power-cycle.  "Unseen" (restore_epoch strictly newer than
        # the last consumed one) prevents a soft reboot shortly after a
        # power-loss boot from re-triggering on the same, already-consumed event.
        prev_epoch = marker.get("restore_epoch")
        is_new_event = prev_epoch is None or restore_epoch > prev_epoch
        correlates = (
            boot_epoch is not None
            and -self._POWER_LOSS_CLOCK_SLACK_S
            <= (boot_epoch - restore_epoch)
            <= self._POWER_LOSS_BOOT_WINDOW_S)
        is_power_loss = is_new_event and correlates

        # Persist the decision.  A POWER_LOSS result must be durably recorded
        # first -- otherwise the same event could re-fire on the next soft
        # reboot -- so defer to the software cause if the marker cannot be
        # written.  Merge into any existing marker to preserve watchdog state.
        if is_new_event:
            marker.update({
                "restore_epoch": restore_epoch,
                "loss_epoch": ev.get("loss_epoch"),
                "boot_epoch": boot_epoch,
                "classified": "POWER_LOSS" if is_power_loss else "NON_HARDWARE",
            })
            written = self._write_power_marker(marker)
            if is_power_loss and not written:
                return None

        return self._power_loss_result(ev) if is_power_loss else None

    def _check_watchdog(self):
        """Return a WATCHDOG (cause, description) tuple if the previous reset was
        a BMC watchdog-timer expiration, or None otherwise.

        The BMC records a persistent System Event Log entry when its watchdog
        expires and resets the host (e.g. "Watchdog2 watchdog_host0 | Hard
        reset | Asserted").  Unlike the volatile IPMI expiration flags -- which
        this platform's early-init re-arm clears before reboot-cause runs -- the
        SEL event survives the reset and the boot-time re-arm.  A watchdog event
        that correlates in time with this boot therefore means a watchdog reset
        caused it.  A persistent marker records the last-consumed event so the
        same SEL entry cannot re-fire on subsequent (software) reboots.
        """
        ev = self._last_watchdog_event()
        if not ev or ev.get("event_epoch") is None:
            return None

        event_epoch = ev["event_epoch"]
        boot_epoch = self._boot_epoch()
        marker = self._read_power_marker() or {}
        prev_epoch = marker.get("wd_event_epoch")

        # Already-consumed event (this or older): replay the prior decision only
        # if it was classified WATCHDOG for this same boot; never re-fire an old
        # SEL entry on a later reboot.
        if prev_epoch is not None and event_epoch <= prev_epoch:
            if (marker.get("wd_classified") == "WATCHDOG"
                    and event_epoch == prev_epoch
                    and boot_epoch is not None
                    and marker.get("wd_boot_epoch") is not None
                    and abs(boot_epoch - marker["wd_boot_epoch"])
                    <= self._POWER_LOSS_CLOCK_SLACK_S):
                return self._watchdog_result(ev)
            return None

        # A previously-unseen watchdog event: it caused this boot if it lands in
        # the boot-correlation window just before the kernel came up.
        correlates = (
            boot_epoch is not None
            and -self._POWER_LOSS_CLOCK_SLACK_S
            <= (boot_epoch - event_epoch)
            <= self._POWER_LOSS_BOOT_WINDOW_S)
        is_watchdog = correlates

        logger.info(
            "BMC watchdog SEL event at epoch=%s (%s); boot_epoch=%s -> %s",
            event_epoch, ev.get("action_str"), boot_epoch,
            "WATCHDOG" if is_watchdog else "NON_HARDWARE")

        # Persist the decision first so the same SEL entry cannot re-fire on a
        # later reboot; defer to the software cause if the marker cannot be
        # written.  Merge to preserve any power-loss marker state.
        marker.update({
            "wd_event_epoch": event_epoch,
            "wd_boot_epoch": boot_epoch,
            "wd_classified": "WATCHDOG" if is_watchdog else "NON_HARDWARE",
        })
        written = self._write_power_marker(marker)
        if is_watchdog and not written:
            return None
        return self._watchdog_result(ev) if is_watchdog else None

    def _last_watchdog_event(self):
        """Best-effort most-recent BMC watchdog reset event dict, or None.
        Never raises."""
        try:
            bmc = self.get_bmc()
            return bmc.get_last_watchdog_event() if bmc else None
        except Exception:
            return None

    @staticmethod
    def _watchdog_result(ev):
        """Build the WATCHDOG (cause, description) tuple for a watchdog event."""
        description = "Watchdog timer expired (BMC hard reset)"
        # Surface the SEL event time in the "[Time: ...]" form that
        # determine-reboot-cause extracts into the reboot-history Time column.
        ts = Chassis._format_bmc_time(ev.get("event_epoch")) or ev.get("raw_str")
        if ts:
            description += " [Time: {} (BMC SEL)]".format(ts)
        return (PddfChassis.REBOOT_CAUSE_WATCHDOG, description)

    @staticmethod
    def _format_bmc_time(epoch):
        """Format a UTC epoch like the reboot-history Time column
        ('Tue Sep  1 01:42:42 PM UTC 2026'), or None."""
        if epoch is None:
            return None
        try:
            return time.strftime(
                "%a %b %e %I:%M:%S %p UTC %Y", time.gmtime(epoch))
        except (ValueError, OSError, OverflowError):
            return None

    @staticmethod
    def _power_loss_result(ev):
        """Build the POWER_LOSS (cause, description) tuple for a power event."""
        description = "Cold power-on / power-cycle detected"
        # Prefer a normalised timestamp that matches the reboot-history Time
        # column format; fall back to the raw BMC string if the epoch is absent.
        ts = Chassis._format_bmc_time(ev.get("loss_epoch")) or ev.get("loss_str")
        if ts:
            # Encode the BMC-SEL power-loss time in the "[Time: ...]" form that
            # determine-reboot-cause surfaces into the reboot-history Time
            # column, instead of leaving it in the cause text.
            description += " [Time: {} (BMC SEL)]".format(ts)
        return (PddfChassis.REBOOT_CAUSE_POWER_LOSS, description)

    def _last_power_event(self):
        """Best-effort most-recent BMC power event dict, or None. Never raises."""
        try:
            bmc = self.get_bmc()
            if bmc is None:
                return None
            return bmc.get_last_power_event()
        except Exception:
            return None

    def get_bmc(self):
        """Return the 8140 BMC accessor (IPMI-over-KCS), or None.

        Used by bmc_techsupport.py (invoked from "show techsupport") to
        collect BMC diagnostics into the techsupport bundle.
        """
        if getattr(self, "_bmc", None) is None:
            try:
                from sonic_platform.bmc import CienaBmc
                self._bmc = CienaBmc()
            except Exception as e:
                logger.error("Failed to create BMC accessor: %s", e)
                self._bmc = None
        return self._bmc

    def get_position_in_parent(self):
        return -1

    def is_replaceable(self):
        return False
