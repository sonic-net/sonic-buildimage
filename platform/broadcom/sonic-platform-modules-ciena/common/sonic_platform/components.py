#!/usr/bin/env python

#############################################################################
# Ciena Components
#
# Firmware component management for BIOS, FPGA, BMC, CPLD.
#
# FPGA version: read via plreg — EUROPA_BASE_MJR.EUROPA_BASE_MNR.EUROPA_BASE_BLD
# BIOS version: read via ipmitool mc getsysinfo system_fw_version
# FPGA upgrade: flash via /dev/mtd2 (europa-user NOR flash, 13.5 MiB)
#
# Firmware versions are cached in /tmp/.ciena_fw_cache.json to avoid
# repeated slow hardware queries (ipmitool ~500ms, plreg ~200ms each).
# Cache TTL defaults to 300 s; invalidated automatically after install.
#############################################################################

try:
    from sonic_platform_pddf_base.pddf_component import PddfComponent
    import subprocess
    import os
    import json
    import time
    import logging
    import re
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

logger = logging.getLogger(__name__)

# Firmware version cache — persists across CLI invocations
FW_CACHE_FILE = "/tmp/.ciena_fw_cache.json"
FW_CACHE_TTL  = 300   # seconds (5 min); versions only change after flash + reboot

# ------------------------------------------------------------------
# Module-level firmware version cache helpers
# ------------------------------------------------------------------

def _load_fw_cache():
    """Load the firmware version cache from disk.
    Returns dict  {"<name>": {"version": str, "ts": float}, ...}
    """
    try:
        with open(FW_CACHE_FILE, 'r') as f:
            return json.load(f)
    except Exception:
        return {}


def _save_fw_cache(cache):
    """Atomically write the firmware version cache to disk."""
    tmp = FW_CACHE_FILE + ".tmp"
    try:
        with open(tmp, 'w') as f:
            json.dump(cache, f)
        os.replace(tmp, FW_CACHE_FILE)
    except Exception as e:
        logger.debug("Failed to write fw cache: %s", e)


def _get_cached_version(name):
    """Return cached version string if still valid, else None."""
    cache = _load_fw_cache()
    entry = cache.get(name)
    if entry and (time.time() - entry.get("ts", 0)) < FW_CACHE_TTL:
        return entry["version"]
    return None


def _set_cached_version(name, version):
    """Store a firmware version in the persistent cache."""
    cache = _load_fw_cache()
    cache[name] = {"version": version, "ts": time.time()}
    _save_fw_cache(cache)


def _invalidate_cached_version(name):
    """Remove a single component from the cache (e.g. after install)."""
    cache = _load_fw_cache()
    if name in cache:
        del cache[name]
        _save_fw_cache(cache)


class Component(PddfComponent):
    """Platform-specific Component class"""

    def __init__(self, component_index, pddf_data=None, pddf_plugin_data=None):
        PddfComponent.__init__(self, component_index, pddf_data, pddf_plugin_data)

    def get_name(self):
        component_key = f"COMPONENT{self.index + 1}"
        try:
            name = self.__get_component_attr(component_key, "name")
            if not isinstance(name, str) or not name.strip():
                return f"COMPONENT{self.index + 1}"

            # fwutil maps components by name; disambiguate when there are
            # multiple components sharing the same base name (e.g. FPGA, CPLD).
            if self.__has_multiple_components_named(name.strip()):
                desc = self.__get_component_attr(component_key, "description")
                if isinstance(desc, str) and desc.strip():
                    return f"{name.strip()} ({desc.strip()})"
                return f"{name.strip()} ({component_key})"

            return name.strip()
        except Exception:
            return f"COMPONENT{self.index + 1}"
        
    def get_description(self):
        try:
            description = PddfComponent.get_description(self)
            if description:
                return description
        except Exception:
            pass
        return f"{self.get_name()} component"

    def __get_pddf_json(self):
        """Return parsed PDDF JSON dictionary from PDDF object."""
        pddf_json = getattr(self.pddf_obj, "data", {})
        return pddf_json if isinstance(pddf_json, dict) else {}

    def __get_component_entry(self, component_key):
        """Return component entry dict from PDDF JSON."""
        entry = self.__get_pddf_json().get(component_key, {})
        return entry if isinstance(entry, dict) else {}

    def __get_component_cmd(self, component_key, attr_name="version"):
        """Get command string from component attr_list for attr_name."""
        comp = self.__get_component_entry(component_key)
        attr_list = comp.get("attr_list", [])
        if not isinstance(attr_list, list):
            return None

        for attr in attr_list:
            if isinstance(attr, dict) and attr.get("attr_name") == attr_name:
                return attr.get("cmd") or attr.get("get_cmd")
        return None

    def __get_component_attr(self, component_key, attr_key):
        """Get value from COMPONENTx.comp_attr[attr_key], or None."""
        comp_attr = self.__get_component_entry(component_key).get("comp_attr", {})
        if not isinstance(comp_attr, dict):
            return None
        return comp_attr.get(attr_key)

    def __get_platform_attr(self, attr_key):
        """Get value from PLATFORM[attr_key], or None if unavailable."""
        platform_attr = self.__get_pddf_json().get("PLATFORM", {})
        if not isinstance(platform_attr, dict):
            return None
        return platform_attr.get(attr_key)

    def __get_component_type_name(self):
        """Get canonical component type from comp_attr.name."""
        component_key = f"COMPONENT{self.index + 1}"
        name = self.__get_component_attr(component_key, "name")
        return name.strip() if isinstance(name, str) else ""

    def __has_multiple_components_named(self, target_name):
        """Return True if the PDDF model contains more than one component with target_name."""
        pddf_json = self.__get_pddf_json()
        count = 0
        for key, value in pddf_json.items():
            if not (isinstance(key, str) and key.startswith("COMPONENT")):
                continue
            if not isinstance(value, dict):
                continue
            comp_attr = value.get("comp_attr", {})
            if not isinstance(comp_attr, dict):
                continue
            comp_name = comp_attr.get("name")
            if isinstance(comp_name, str) and comp_name.strip() == target_name:
                count += 1
                if count > 1:
                    return True
        return False

    # ------------------------------------------------------------------
    # Version retrieval helpers
    # ------------------------------------------------------------------

    def __get_bios_version(self):
        """Retrieve BIOS version via ipmitool, falling back to DMI sysfs."""
        try:
            result = subprocess.run(
                ["ipmitool", "mc", "getsysinfo", "system_fw_version"],
                capture_output=True, text=True, timeout=5)
            if result.returncode == 0 and result.stdout.strip():
                return result.stdout.strip()
        except Exception:
            pass

        # Fallback to PDDF COMPONENT2 version command from pddf-device.json
        output = PddfComponent.get_firmware_version(self)
        if output:
            return output

        return "N/A"

    def __get_fpga_version(self):
        """Retrieve FPGA version as major.minor.build from plreg registers.

        If a command is defined in the component JSON, executes it and returns
        the output as-is (command handles formatting).
        Otherwise, reads FPGA version registers via plreg and formats as "M.N.B".
        """
        component_key = f"COMPONENT{self.index + 1}"

        # Prefer explicit command override for this FPGA component.
        cmd = self.__get_component_cmd(component_key, "version")
        if cmd:
            try:
                result = subprocess.run(
                    cmd,
                    shell=True,
                    capture_output=True,
                    text=True,
                    timeout=5,
                    executable="/bin/bash")
                if result.returncode == 0 and result.stdout.strip():
                    return result.stdout.strip()
            except Exception as e:
                logger.warning("Failed to run FPGA version cmd for %s: %s", component_key, e)

        try:
            json_regs = self.__get_component_attr(component_key, "version_regs")
            regs = []
            if isinstance(json_regs, list):
                parsed_regs = [reg for reg in json_regs if isinstance(reg, str) and reg.strip()]
                if parsed_regs:
                    regs = parsed_regs
        except Exception:
            return "N/A"

        parts = []
        for reg in regs:
            try:
                result = subprocess.run(
                    ["sudo", "-n", "/usr/local/bin/plreg", "-c", "read", reg],
                    capture_output=True, text=True, timeout=5, check=True)
                # plreg outputs e.g. "0x29" — convert to decimal
                val = int(result.stdout.strip(), 16)
                parts.append(str(val))
            except Exception as e:
                logger.warning("Failed to read FPGA register %s: %s", reg, e)
                parts.append("?")
        if not parts:
            return "N/A"

        return ".".join(parts)

    def __get_bmc_version(self):
        """Retrieve BMC/IPMI firmware version."""
        # Fallback to PDDF COMPONENT4 version command from pddf-device.json
        output = PddfComponent.get_firmware_version(self)
        if output:
            return output

        # Fallback: ipmitool mc info
        try:
            result = subprocess.run(
                ["ipmitool", "mc", "info"],
                capture_output=True, text=True, timeout=5)
            if result.returncode == 0:
                for line in result.stdout.splitlines():
                    if "Firmware Revision" in line:
                        return line.split(":")[-1].strip()
        except Exception:
            pass
        return "N/A"

    def __get_cpld_version(self):
        """Retrieve CPLD version via FPGA SGPIO data register.

        The Europa CPLD communicates with the FPGA over an SGPIO interface.
        EUROPA_GLUE_CPLD_SGPIO_DATA0 (offset 0x0630) contains data
        received from the CPLD.  The low byte (bits [7:0]) encodes the
        CPLD firmware version.

        Returns the version as a hex string (e.g. "0x05"), or None.
        """
        # Optional override from pddf-device.json:
        try:
            json_reg = self.__get_component_attr("COMPONENT1", "version_reg")
            if isinstance(json_reg, str) and json_reg.strip():
                reg = json_reg.strip()

            result = subprocess.run(
                ["sudo", "-n", "/usr/local/bin/plreg", "-c", "read", reg],
                capture_output=True, text=True, timeout=5, check=True)
            raw = result.stdout.strip()
            if raw:
                val = int(raw, 16)
                version = val & 0xFF
                if version != 0:
                    return "0x{:02X}".format(version)
        except Exception as e:
            logger.debug("CPLD version read failed: %s", e)
        return "N/A"

    def get_firmware_version(self):
        """Retrieve the firmware version of the component.

        Checks a file-based cache first (TTL = FW_CACHE_TTL seconds).
        On miss, reads from hardware and populates the cache so that
        subsequent CLI invocations return instantly.
        """
        component_key = f"COMPONENT{self.index + 1}"
        cache_key = f"{component_key}:{self.name}"

        # Fast path — cached
        cached = _get_cached_version(cache_key)
        if cached is not None:
            return cached

        # Check for explicit command override in attr_list first.
        # This allows any component type to define a custom version command.
        cmd = self.__get_component_cmd(component_key, "version")
        if cmd:
            try:
                result = subprocess.run(
                    cmd, shell=True, capture_output=True, text=True,
                    timeout=5, executable="/bin/bash")
                if result.returncode == 0 and result.stdout.strip():
                    fw_version = result.stdout.strip()
                    _set_cached_version(cache_key, fw_version)
                    return fw_version
            except Exception as e:
                logger.warning("Failed to run version cmd for %s: %s", component_key, e)

        # Slow path — type-based hardware read
        fw_version = None
        component_type = self.__get_component_type_name()
        if component_type == "BIOS":
            fw_version = self.__get_bios_version()
        elif component_type == "FPGA":
            fw_version = self.__get_fpga_version()
        elif component_type == "BMC":
            fw_version = self.__get_bmc_version()
        elif "CPLD" in component_type:
            fw_version = self.__get_cpld_version()

        # Populate cache for next time
        if fw_version is not None:
            _set_cached_version(cache_key, fw_version)

        return fw_version

    # ------------------------------------------------------------------
    # Firmware install
    # ------------------------------------------------------------------

    def install_firmware(self, image_path):
        """Install firmware to the component.

        Currently only FPGA upgrade is supported, via flashcp to /dev/mtd2
        (europa-user NOR flash).

        Args:
            image_path: A string, path to the firmware image file.
        Returns:
            A boolean, True if installed successfully, False if not.
        """
        component_type = self.__get_component_type_name()
        if component_type == "FPGA":
            result = self.__install_fpga_firmware(image_path)
        else:
            logger.error("Firmware install not supported for component: %s", self.name)
            return False

        # Invalidate cache so next version read goes to hardware
        if result:
            component_key = f"COMPONENT{self.index + 1}"
            _invalidate_cached_version(f"{component_key}:{self.name}")
        return result

    def __install_fpga_firmware(self, image_path):
        """Flash FPGA firmware to europa-user bank via flashcp.

        The image should be a raw binary matching the europa-user MTD
        partition size (up to 13.5 MiB).  A reboot or FPGA reload is
        required to activate the new firmware.
        """
        if not os.path.isfile(image_path):
            logger.error("FPGA image file not found: %s", image_path)
            return False

        fpga_mtd_device = self.__get_platform_attr("FPGA_MTD_DEVICE")

        if not fpga_mtd_device:
            logger.error("FPGA_MTD_DEVICE is not set in PLATFORM")
            return False

        if not os.path.exists(fpga_mtd_device):
            logger.error("FPGA MTD device not found: %s", fpga_mtd_device)
            return False

        try:
            # Verify image size doesn't exceed partition
            img_size = os.path.getsize(image_path)
            # europa-user is 216 erase blocks × 64 KiB = 14155776 bytes
            max_size = 14155776
            if img_size > max_size:
                logger.error("FPGA image too large: %d bytes (max %d)", img_size, max_size)
                return False

            logger.info("Flashing FPGA firmware: %s → %s (%d bytes)",
                        image_path, fpga_mtd_device, img_size)

            result = subprocess.run(
                ["flashcp", "-v", image_path, fpga_mtd_device],
                capture_output=True, text=True, timeout=300)

            if result.returncode != 0:
                logger.error("flashcp failed: %s", result.stderr)
                return False

            logger.info("FPGA firmware flashed successfully. Reboot required to activate.")
            return True

        except subprocess.TimeoutExpired:
            logger.error("FPGA flash timed out after 300 seconds")
            return False
        except Exception as e:
            logger.error("FPGA flash failed: %s", e)
            return False

    def get_available_firmware_version(self, image_path):
        """Get version from a firmware image file — not implemented."""
        return "N/A"