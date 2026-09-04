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
    import sys
    import select
    import json
    import time
    import logging
    import re
    import struct
    import tempfile
    import bz2
    import gzip
    import zlib
    import shlex
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


# ------------------------------------------------------------------
# FPGA reconfig "breadcrumb" -- activation handshake with platform_reboot
# ------------------------------------------------------------------
# The fwutil install/upgrade FPGA flashes the new load, now it needs to be
# reloaded from flash by issuing a reconfig command. This is done on the 
# next COLD reboot. The reboot hook (platform_reboot) consumes to trigger the reconfig.
FPGA_RECONFIG_PENDING = "/host/fpga_reconfig_pending"


def _arm_fpga_reconfig(image_name):
    """Drop a persistent breadcrumb so the next COLD reboot activates the
    freshly-flashed FPGA image via an FPGA reconfig.

    Written atomically (tmp + fsync + rename, plus a directory fsync) to a
    persistent partition so it survives the reboot.  Returns True on success.
    """
    payload = {
        "image": os.path.basename(image_name or ""),
        "time": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "note": "FPGA user bank flashed; FPGA reconfig required to activate",
    }
    tmp = FPGA_RECONFIG_PENDING + ".tmp"
    try:
        with open(tmp, "w") as f:
            json.dump(payload, f)
            f.flush()
            os.fsync(f.fileno())
        os.replace(tmp, FPGA_RECONFIG_PENDING)
        try:
            dfd = os.open(os.path.dirname(FPGA_RECONFIG_PENDING), os.O_RDONLY)
            try:
                os.fsync(dfd)
            finally:
                os.close(dfd)
        except OSError:
            pass
        return True
    except Exception as e:
        logger.error("Failed to arm FPGA reconfig breadcrumb %s: %s",
                     FPGA_RECONFIG_PENDING, e)
        try:
            if os.path.exists(tmp):
                os.unlink(tmp)
        except OSError:
            pass
        return False


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

    def get_firmware_update_notification(self, image_path):
        """Return a pre-install notification message for fwutil.

        This override documents how the new firmware is activated
        (a reboot is required for the FPGA).
        """
        return ("{} firmware will be updated. A reboot is required to "
                "activate the new image.".format(self.get_name()))

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

        FPGA upgrades are supported; they are written to NOR flash
        via flashcp. BMC firmware upgrades are not supported on this
        platform (see notes below).

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

    def update_firmware(self, image_path):
        """Update component firmware (used by ``fwutil update``).

        The PDDF base ``update_firmware`` runs a PDDF "update" command from
        the device JSON, which the Ciena model does not define. Route through
        our ``install_firmware`` (flashcp for FPGA) instead. Activation still
        requires a reboot; that is not performed automatically here so the
        operator stays in control.

        Args:
            image_path: A string, path to the firmware image file.
        Returns:
            False if the image file does not exist; True on success.
        Raises:
            RuntimeError: if the install fails.
        """
        if not os.path.isfile(image_path):
            return False
        if not self.install_firmware(image_path):
            raise RuntimeError(
                "{} firmware update failed".format(self.get_name()))
        return True

    # U-Boot legacy image (uImage) constants
    __UIMAGE_MAGIC = 0x27051956
    __UIMAGE_HDR_LEN = 64
    # ih_comp values
    __IH_COMP_NONE = 0
    __IH_COMP_GZIP = 1
    __IH_COMP_BZIP2 = 2

    # FPGA .rbf/.rpd bitstreams are stored bit-reversed relative to what the
    # FPGA configuration port expects. SAOS runs the payload through its "tib"
    # (byte-wise bit reversal) tool before writing it to flash; we reproduce
    # that here with a 256-entry translation table.
    __BITREV_TABLE = bytes(
        int('{:08b}'.format(_b)[::-1], 2) for _b in range(256))

    @staticmethod
    def __maybe_decompress(raw, name):
        """Transparently decompress an outer bzip2/gzip container.

        SAOS stores firmware images bzip2-compressed (e.g.
        ``rudra40.rbf.img.bz2``); ``img_extract`` decompresses by extension
        before parsing. We do the same in-process using the Python stdlib
        (bz2/gzip), so no bzip2 CLI is required on the box. Detection is by
        file extension with a magic-byte fallback. A plain/uncompressed
        image is returned unchanged.
        """
        lname = name.lower()
        if lname.endswith((".bz2", ".bzip2")) or raw[:3] == b"BZh":
            return bz2.decompress(raw)
        if lname.endswith((".gz", ".gzip")) or raw[:2] == b"\x1f\x8b":
            return gzip.decompress(raw)
        return raw

    @staticmethod
    def __crc32_cksum(data):
        """POSIX cksum CRC-32 (CRC-32/CKSUM).

        SAOS accepts either this or the zlib/ISO-HDLC CRC-32 for the uImage
        header, depending on the tool that built the image. Reproduced here
        for parity with SAOS "img_extract_header".
        """
        crc = 0
        for byte in data:
            crc ^= byte << 24
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ 0x04C11DB7) & 0xffffffff
                else:
                    crc = (crc << 1) & 0xffffffff
        n = len(data)
        while n > 0:
            crc ^= (n & 0xff) << 24
            for _ in range(8):
                if crc & 0x80000000:
                    crc = ((crc << 1) ^ 0x04C11DB7) & 0xffffffff
                else:
                    crc = (crc << 1) & 0xffffffff
            n >>= 8
        return (~crc) & 0xffffffff

    def __extract_fpga_payload(self, image_path):
        """Extract the flashable FPGA bitstream from a Ciena firmware image.

        FPGA images (e.g. rudra40.rbf.img) are wrapped in a U-Boot
        image (uImage) header: a 64-byte header followed by the
        (optionally compressed) bitstream payload, and the whole thing may
        be stored bzip2-compressed (``.bz2``). To match what
        SAOS writes to the NOR user bank (see fpga_utils.sh
        "img_extract_payload"), we:

          0. Transparently decompress an outer bzip2/gzip container.
          1. Validate the uImage header (magic + header CRC + length) and,
             for content integrity, the data CRC — before touching flash.
          2. Strip the 64-byte header and decompress the payload.
          3. For .rbf/.rpd bitstreams, apply the byte-wise bit reversal.

        Writing the wrapped container, or the payload without the bit
        reversal, leaves an unbootable user bank and the FPGA falls back to
        its golden bank. A corrupt or truncated image is rejected before the
        NOR is erased/programmed.
        """
        name = os.path.basename(image_path)
        with open(image_path, "rb") as f:
            raw = f.read()

        # Images stored as bzip2-compressed; decompress before parsing.
        raw = self.__maybe_decompress(raw, name)

        has_uimage = len(raw) >= self.__UIMAGE_HDR_LEN and \
            struct.unpack(">I", raw[:4])[0] == self.__UIMAGE_MAGIC

        if has_uimage:
            header = raw[:self.__UIMAGE_HDR_LEN]
            stored_hcrc = struct.unpack(">I", header[4:8])[0]
            ih_size = struct.unpack(">I", header[12:16])[0]
            stored_dcrc = struct.unpack(">I", header[24:28])[0]
            ih_comp = header[31]
            ih_name = header[32:64].split(b"\0")[0].decode("latin1", "ignore")

            # Header CRC: CRC32 of the 64-byte header with the hcrc field
            # zeroed. SAOS accepts either the zlib/ISO-HDLC or the POSIX
            # cksum variant; validate before trusting any other field.
            hdr_for_crc = bytearray(header)
            hdr_for_crc[4:8] = b"\x00\x00\x00\x00"
            calc_hdlc = zlib.crc32(bytes(hdr_for_crc)) & 0xffffffff
            if stored_hcrc != calc_hdlc and \
                    stored_hcrc != self.__crc32_cksum(bytes(hdr_for_crc)):
                raise RuntimeError(
                    "uImage header CRC mismatch: header 0x%08x, computed "
                    "0x%08x" % (stored_hcrc, calc_hdlc))

            data = raw[self.__UIMAGE_HDR_LEN:self.__UIMAGE_HDR_LEN + ih_size]
            if len(data) != ih_size:
                raise RuntimeError(
                    "uImage payload truncated: got %d bytes, header says %d"
                    % (len(data), ih_size))

            # Data CRC: CRC32 over the stored (possibly compressed) payload.
            calc_dcrc = zlib.crc32(data) & 0xffffffff
            if calc_dcrc != stored_dcrc:
                raise RuntimeError(
                    "uImage data CRC mismatch: header 0x%08x, computed 0x%08x"
                    % (stored_dcrc, calc_dcrc))

            if ih_comp == self.__IH_COMP_NONE:
                payload = data
            elif ih_comp == self.__IH_COMP_GZIP:
                payload = gzip.decompress(data)
            elif ih_comp == self.__IH_COMP_BZIP2:
                payload = bz2.decompress(data)
            else:
                raise RuntimeError(
                    "Unsupported uImage compression type: %d" % ih_comp)
            logger.info("Validated uImage '%s' (%d bytes, comp=%d, CRC OK)",
                        ih_name, len(payload), ih_comp)
        else:
            # No uImage header (e.g. a bare .rbf) — use the file as-is.
            logger.info("FPGA image has no uImage header; treating as raw")
            payload = raw

        # Bit-reverses .rbf/.rpd bitstreams before flashing (similar to "tib"
        # tool); .bin images are written verbatim.
        if ".rbf" in name or ".rpd" in name:
            payload = payload.translate(self.__BITREV_TABLE)

        return payload

    @staticmethod
    def __run_streaming(cmd, timeout=None):
        """Run *cmd*, echoing its output to the terminal in real time.

        Long flash operations (``flashcp -v``) emit a live erase/write/
        verify progress counter using carriage returns. ``subprocess.run``
        with ``capture_output=True`` swallows that, so ``fwutil update``
        appears to hang silently. This runs the command with its output
        streamed straight through to our stdout (preserving the ``\\r``
        progress line) while still capturing it so it can be logged on
        failure.

        Args:
            cmd: command argument list.
            timeout: overall timeout in seconds (None = no limit).
        Returns:
            (returncode, combined_output_str) — stdout+stderr merged.
        Raises:
            subprocess.TimeoutExpired: if *timeout* elapses.
        """
        proc = subprocess.Popen(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, bufsize=0)
        captured = []
        deadline = (time.time() + timeout) if timeout else None
        try:
            while True:
                remaining = None
                if deadline is not None:
                    remaining = deadline - time.time()
                    if remaining <= 0:
                        proc.kill()
                        proc.wait()
                        raise subprocess.TimeoutExpired(cmd, timeout)
                ready, _, _ = select.select([proc.stdout], [], [], remaining)
                if not ready:
                    continue
                chunk = os.read(proc.stdout.fileno(), 4096)
                if not chunk:
                    break
                captured.append(chunk)
                try:
                    sys.stdout.buffer.write(chunk)
                    sys.stdout.buffer.flush()
                except Exception:
                    try:
                        sys.stdout.write(chunk.decode("utf-8", "replace"))
                        sys.stdout.flush()
                    except Exception:
                        pass
            proc.wait()
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait()
        return proc.returncode, b"".join(captured).decode("utf-8", "replace")

    def __install_fpga_firmware(self, image_path):
        """Flash FPGA firmware to the FPGA user partition via flashcp.

        The target MTD partition is taken from PLATFORM.FPGA_MTD_DEVICE in
        pddf-device.json (e.g. the "rudra40-user" NOR bank).
        The image is a uImage-wrapped bitstream; its payload is
        extracted (header stripped, decompressed) before being written. A
        reboot or FPGA reload is required to activate the new firmware. The
        read-only golden/recovery bank is never touched.
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

        payload_path = None
        try:
            # Extract the raw bitstream from the uImage container. Writing
            # the wrapped file directly leaves an unbootable user bank.
            payload = self.__extract_fpga_payload(image_path)

            # FPGA user bank is 216 erase blocks x 64 KiB = 0xD80000 bytes
            # (matches europa-user on the 8112 and rudra40-user on the 8140)
            max_size = 0xD80000
            if len(payload) > max_size:
                logger.error("FPGA payload too large: %d bytes (max %d)",
                             len(payload), max_size)
                return False

            with tempfile.NamedTemporaryFile(
                    prefix="fpga_payload_", suffix=".bin", delete=False) as tf:
                tf.write(payload)
                payload_path = tf.name

            logger.info("Flashing FPGA firmware: %s → %s (%d bytes payload)",
                        image_path, fpga_mtd_device, len(payload))

            print("Erasing and writing FPGA flash ({:,} bytes) — "
                  "this can take several minutes:".format(len(payload)),
                  flush=True)

            rc, output = self.__run_streaming(
                ["flashcp", "-v", payload_path, fpga_mtd_device], timeout=300)
            # Ensure the shell prompt starts on a fresh line after flashcp's
            # carriage-return progress counter.
            print(flush=True)

            if rc != 0:
                logger.error("flashcp failed (rc=%d): %s", rc, output.strip())
                return False

            logger.info("FPGA firmware flashed successfully. Reboot required to activate.")
            # Stage automatic activation: the next COLD reboot will trigger
            # an FPGA reconfig (see platform_reboot) to load the new image.
            if _arm_fpga_reconfig(image_path):
                print("FPGA image staged. It will be ACTIVATED automatically on the "
                      "next COLD reboot (run: sudo reboot).", flush=True)
                logger.info("Armed FPGA reconfig breadcrumb %s", FPGA_RECONFIG_PENDING)
            else:
                print("WARNING: could not stage automatic activation; after reboot "
                      "activate manually with: sudo plreg write "
                      "RUDRA40_BASE_FPGA_RECONFIG 0x5a5a", flush=True)
            return True

        except subprocess.TimeoutExpired:
            logger.error("FPGA flash timed out after 300 seconds")
            return False
        except Exception as e:
            logger.error("FPGA flash failed: %s", e)
            return False
        finally:
            if payload_path and os.path.exists(payload_path):
                try:
                    os.unlink(payload_path)
                except OSError:
                    pass

    def get_available_firmware_version(self, image_path):
        """Get version from a firmware image file — not implemented."""
        return "N/A"