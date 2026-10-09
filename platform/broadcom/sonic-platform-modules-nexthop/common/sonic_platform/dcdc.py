# Copyright 2025 Nexthop Systems Inc. All rights reserved.
# SPDX-License-Identifier: Apache-2.0

"""Chip specific DCDC structures

Provides custom logic overtop of the existing base class
"""

import os

from dataclasses import dataclass
import glob
from sonic_platform.blackbox_record_base import BlackBoxRecordBase
from sonic_platform_pddf_base.pddf_dcdc import PddfDcdc


# PMBus status-register bit definitions, keyed by register name:
STATUS_BITFIELDS: dict[str, dict[int, str]] = {
    "STATUS_WORD": {
        15: "VOUT_FAULT",
        14: "IOUT_FAULT",
        13: "INPUT_FAULT",
        12: "MFR_SPECIFIC_FAULT",
        11: "POWER_BAD", # POWER_GOOD# (active low)
        10: "FAN_FAULT",
        9:  "OTHER_FAULT",
        8:  "UNKNOWN",
        7:  "DEVICE_BUSY",
        6:  "OUTPUT_OFF",
        5:  "VOUT_OV_FAULT",
        4:  "IOUT_OC_FAULT",
        3:  "VIN_UV_FAULT",
        2:  "TEMPERATURE_FAULT",
        1:  "CML_FAULT",
        0:  "OTHER_STATUS_CHANGE", # NONE_OF_THE_ABOVE
    },
    "STATUS_VOUT": {
        7: "VOUT_OV_FAULT",
        6: "VOUT_OV_WARNING",
        5: "VOUT_UV_WARNING",
        4: "VOUT_UV_FAULT",
        3: "VOUT_MAX_MIN_WARNING",
        2: "TON_MAX_FAULT",
        1: "TOFF_MAX_WARNING",
        0: "VOUT_TRACKING_ERROR",
    },
    "STATUS_IOUT": {
        7: "IOUT_OC_FAULT",
        6: "IOUT_OC_LV_FAULT",
        5: "IOUT_OC_WARNING",
        4: "IOUT_UC_FAULT",
        3: "CURRENT_SHARE_FAULT",
        2: "POWER_LIMITING_MODE",
        1: "POUT_OP_FAULT",
        0: "POUT_OP_WARNING",
    },
    "STATUS_INPUT": {
        7: "VIN_OV_FAULT",
        6: "VIN_OV_WARNING",
        5: "VIN_UV_WARNING",
        4: "VIN_UV_FAULT",
        3: "VIN_OFF",
        2: "IIN_OC_FAULT",
        1: "IIN_OC_WARNING",
        0: "PIN_OP_WARNING",
    },
    "STATUS_TEMPERATURE": {
        7: "OT_FAULT",
        6: "OT_WARNING",
        5: "UT_WARNING",
        4: "UT_FAULT",
        # bits 3-0 reserved
    },
    "STATUS_CML": {
        7: "INVALID_OR_UNSUPPORTED_COMMAND_RECEIVED",
        6: "INVALID_OR_UNSUPPORTED_DATA_RECEIVED",
        5: "PACKET_ERROR_CHECK_FAILED",
        4: "MEMORY_FAULT_DETECTED",
        3: "PROCESSOR_FAULT_DETECTED",
        # bit 2 reserved
        1: "OTHER_COMMUNICATION_FAULT",
        0: "OTHER_MEMORY_OR_LOGIC_FAULT",
    },
}



def resolve_i2c_hwmon_paths(bus: int, addr: int) -> list[str]:
    """Returns the hwmon directories registered for the i2c device <bus>-<addr>."""
    return glob.glob(f"/sys/bus/i2c/devices/{bus}-{addr:04x}/hwmon/hwmon*")

@dataclass
class DcdcStatusRecord:
    """PMBus status register values read from a DCDC device.

    Contains both raw binary values and decoded PMBus statuses
    (register values expanded into active bitfield names).
    """
    name: str
    dev_type: str
    global_statuses: dict[str, int] # {reg_name: reg_value, ...}
    rail_statuses: list[dict[str, int]]
    global_statuses_decoded: dict[str, list[str]] # {reg_name: [active_bitfield1, active_bitfield2, ...], ...}
    rail_statuses_decoded: list[dict[str, list[str]]]


@dataclass
class StatusRegister:
    """Defines a PMBUS status register for a DCDC device."""
    name: str
    bit_width: int
    mask: int
    bitfields: dict[int, str]
    debugfs_attribute: str # Path to attribute in /sys/kernel/debug/pmbus/<hwmon>/
    is_global: bool # Statuses may be global or have registers per-rail.

    def __post_init__(self):
        """Filter 'bitfields' to contain only the bits enabled by 'mask'."""
        supported_bitfields: dict[int, str] = {}
        for bit_position, bit_name in self.bitfields.items():
            if self.mask & (1 << bit_position):
                supported_bitfields[bit_position] = bit_name
        self.bitfields = supported_bitfields


class Dcdc(PddfDcdc):
    """Generic PMBus DCDC chip. Overridden by chip-specific subclasses to provide chip-specific behavior."""

    NUM_RAILS = 2
    NVMEM_CELL_IDX = 0

    _device_registry: dict[str, type["Dcdc"]] = {}

    @classmethod
    def register_device(cls, *dev_types: str):
        """Class decorator registering a DCDC subclass for one or more dev_types."""
        def _decorator(device_cls):
            for dev_type in dev_types:
                cls._device_registry[dev_type] = device_cls
            return device_cls
        return _decorator

    def __new__(cls, index, pddf_data=None, pddf_plugin_data=None):
        if cls is Dcdc and pddf_data is not None:
            dev_type = pddf_data.data["DCDC{0}".format(index)]["i2c"]["topo_info"]["dev_type"].removeprefix("nh_")
            cls = Dcdc._device_registry.get(dev_type, Dcdc)
        return super().__new__(cls)

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        PddfDcdc.__init__(self, index, pddf_data, pddf_plugin_data)
        self.name = self.get_name()
        self.dev_type = self.dcdc_obj['i2c']['topo_info']["dev_type"]
        self.registers = self._build_registers()
        self._calculate_paths()

    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1111_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1111_1111,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1111_1111,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1111_1111,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1111_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1011,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
        }

    def _resolve_debugfs_dir(self, bus, addr) -> str | None:
        """Locate the PMBus debugfs directory for this DCDC. Returns None if missing/ambiguous."""
        hwmon_paths = resolve_i2c_hwmon_paths(bus, addr)
        if len(hwmon_paths) != 1: # PMBus drivers register exactly one hwmon per chip
            return None
        hwmon_name = os.path.basename(hwmon_paths[0])
        debugfs_dir = f"/sys/kernel/debug/pmbus/{hwmon_name}"
        return debugfs_dir if os.path.isdir(debugfs_dir) else None

    def _resolve_hwmon_dir(self, bus, addr) -> str | None:
        """Locate the HW monitor directory for this DCDC. Returns None if missing/ambiguous."""
        hwmon_paths = resolve_i2c_hwmon_paths(bus, addr)
        return hwmon_paths[0] if len(hwmon_paths) == 1 else None

    def _calculate_paths(self) -> None:
        """Calculate sysfs paths from the DCDC device's pddf-device.json i2c topology.

        Raises:
            ValueError: If parent_bus/dev_addr are missing from pddf-device.json
        """
        topo_info = self.dcdc_obj['i2c']['topo_info']
        parent_bus = topo_info.get("parent_bus")
        dev_addr = topo_info.get("dev_addr")

        if not parent_bus or not dev_addr:
            raise ValueError(
                f"Missing parent_bus or dev_addr in pddf-device.json for DCDC '{self.name}'"
            )

        parent_bus_int = int(parent_bus, 16) if isinstance(parent_bus, str) else parent_bus
        dev_addr_int = int(dev_addr, 16) if isinstance(dev_addr, str) else dev_addr

        self._nvmem_path = f"/sys/bus/nvmem/devices/{parent_bus_int}-{dev_addr_int:04x}{self.NVMEM_CELL_IDX}/nvmem"
        self._hwmon_dir = self._resolve_hwmon_dir(parent_bus_int, dev_addr_int)
        self._debugfs_dir = self._resolve_debugfs_dir(parent_bus_int, dev_addr_int)

    def get_model(self):
        """Retrieves the device type of the DCDC."""
        return self.dev_type.removeprefix("nh_")

    def read_debugfs_file(self, path: str) -> int:
        """Read a PMBus debugfs status file. Returns -1 on any error."""
        try:
            with open(path, "rb") as f:
                data = f.read()
            return int(data.strip(), 0)
        except (FileNotFoundError, PermissionError, OSError, ValueError):
            return -1

    def get_register_width(self, reg_name: str) -> int:
        """Return the bit width of `reg_name`."""
        reg = self.registers.get(reg_name)
        return reg.bit_width if reg else 0

    def read_statuses_via_debugfs(self) -> DcdcStatusRecord:
        """Reads DCDC statuses from `self._debugfs_dir`.

        Only includes asserted bitfields included in the bit mask of the chip's supported
        registers. If the debugfs directory was notresolved, all status dicts are empty.

        Returns:
            DcdcStatusRecord containing the raw status register values and decoded status
            values stored in key value pairs with the register name.
        """
        status_record = DcdcStatusRecord(
            name=self.name,
            dev_type=self.dev_type,
            global_statuses={},
            rail_statuses=[],
            global_statuses_decoded={},
            rail_statuses_decoded=[],
        )

        if self._debugfs_dir is None:
            return status_record

        # Read global status registers
        for reg_name, reg_info in self.registers.items():
            if reg_info.is_global:
                path = os.path.join(self._debugfs_dir, reg_info.debugfs_attribute)
                status_val = self.read_debugfs_file(path)
                if status_val != -1:
                    status_record.global_statuses[reg_name] = status_val

                    # Decode active bitfields
                    status_record.global_statuses_decoded[reg_name] = []
                    for bit_position, bit_name in reg_info.bitfields.items():
                        if status_val & (1 << bit_position):
                            status_record.global_statuses_decoded[reg_name].append(bit_name)

        # Read per-rail status registers
        for rail in range(self.NUM_RAILS):
            rail_statuses = {}
            rail_statuses_decoded = {}
            for reg_name, reg_info in self.registers.items():
                if reg_info.is_global:
                    continue
                # Encode rail number into debugfs file path
                path = os.path.join(self._debugfs_dir, reg_info.debugfs_attribute.format(rail=rail))
                status_val = self.read_debugfs_file(path)
                if status_val != -1:
                    rail_statuses[reg_name] = status_val

                    # Decode active bitfields
                    rail_statuses_decoded[reg_name] = []
                    for bit_position, bit_name in reg_info.bitfields.items():
                        if status_val & (1 << bit_position):
                            rail_statuses_decoded[reg_name].append(bit_name)

            status_record.rail_statuses.append(rail_statuses)
            status_record.rail_statuses_decoded.append(rail_statuses_decoded)

        return status_record

    def get_blackbox_raw(self, from_ram=True) -> bytes:
        """Read the blackbox data from the DCDC memory.

        Returns:
            List of bytes, or empty if unsupported.
        """
        return b""

    def clear_blackbox(self, from_ram=True):
        """Clears the blackbox data from the DCDC memory."""
        return

    def decode_blackbox_records(self, raw: bytes) -> list[BlackBoxRecordBase]:
        """Decode blackbox data into records

        By default, no blackbox access is provided at the base Dcdc level; subclasses for chips
        that support blackbox parsing override this to return their own BlackBoxRecordBase subclass.

        Returns:
            List of records, specific to the DCDC device
        """
        return []


@dataclass
@BlackBoxRecordBase.register_record("isl68225")
class RaaDmpvr2BlackBoxRecord(BlackBoxRecordBase):
    """Renesas digital multiphase Gen 2 blackbox record. Refer to the 
    Renesas Gen 2 Black Box Guide section 5 for field information.

    Represents a single fault record from the Gen2 blackbox.
    """
    # [NVM] Reserved 2 bytes [3:2]
    # [NVM] 2 bytes, [1:0]
    nvm_written_status: int

    # 3 rail slots in total, only the first num_rails are decoded
    # Per rail, 4 bytes each [11:0]
    rail_uptime_counter: list[int]
    # 4 bytes [15:12]
    controller_first_fault_detect: int
    # Per rail, 4 bytes each [27:16]
    rail_first_fault_detect: list[int]
    # 4 bytes [31:28]
    phase_first_fault_detect_a: int
    # 4 bytes [35:32]
    phase_first_fault_detect_b: int
    # Reserved 4 bytes [39:36]
    # 2 bytes [43:42]
    controller_adc_fault_detect: int
    # Per rail, 2 bytes each [41:40], [47:44]
    status_word: list[int]
    # 1 byte [50:50]
    status_mfr_specific: int
    # 1 byte [51:51]
    status_cml: int
    # Per rail, 1 byte each [54:52]
    status_iout: list[int]
    # Per rail, 1 byte each [49:48], [55:55]
    status_vout: list[int]
    # Per rail, 1 byte each [59:57]
    status_temperature: list[int]
    # Per rail, 1 byte each [56:56], [63:62]
    status_input: list[int]
    # Per rail, 2 bytes each [61:60], [67:64]
    read_vin: list[int]
    # Per rail, 2 bytes each [71:68], [75:74]
    read_vout: list[int]
    # Per rail, 2 bytes each [73:72], [79:76]
    read_iin: list[int]
    # Per rail, 2 bytes each [83:80], [87:86]
    read_iout: list[int]
    # Reserved 2 bytes [89:88]
    # 1 byte [90:90]
    controller_read_temperature: int
    # Per rail, 1 byte each [85:84], [91:91]
    rail_read_temperature: list[int]
    # Reserved 2 bytes [93:92]
    # 2 bytes [95:94]
    vmon_voltage: int
    # Reserved 2 bytes [97:96]
    # Per rail, 2 bytes each [99:98]
    vinsen_voltage: list[int]
    # 12 phases in total
    # Per phase, 1 byte each [111:100]
    phase_temperature: list[int]
    # Per phase, 2 bytes each [135:112]
    phase_current: list[int]
    # [RAM] Reserved 16 bytes [151:136]
    # [NVM] Reserved 20 bytes [159:140]

    NVM_RECORD_SIZE = 160
    RAM_RECORD_SIZE = 152
    MAX_NUM_RAILS = 3
    DEFAULT_NUM_RAILS = 2

    @classmethod
    def from_bytes(cls, data: bytes, dev_name: str, num_rails: int = DEFAULT_NUM_RAILS):
        """Creates RaaDmpvr2BlackBoxRecord from raw bytes.

        Args:
            data: 152 (RAM) or 160 (NVM) byte data representing a black box record
            dev_name: Name of the device for this black box (for display purposes)
            num_rails: Rails monitored by DCDC. Records contain per-rail stats up
                       to 3 rails, with unused rail stats set to 0.

        Returns:
            Parsed BlackBoxRecordBase
        """

        raw = data
        if(len(data) == cls.NVM_RECORD_SIZE):
            nvm_written_status = int.from_bytes(data[0:2], byteorder='little')
            data = data[4:] # strip NVM prefix from data
        elif(len(data) == cls.RAM_RECORD_SIZE):
            nvm_written_status = 0
        else:
            raise ValueError(f"Invalid data length for RaaDmpvr2BlackBoxRecord: {len(data)} bytes")

        if not 1 <= num_rails <= cls.MAX_NUM_RAILS:
            raise ValueError(f"Invalid rail count for RaaDmpvr2BlackBoxRecord: {num_rails}")

        return cls(
            name=dev_name,
            raw=raw,
            nvm_written_status=nvm_written_status,
            rail_uptime_counter=[int.from_bytes(data[i:i+4], byteorder='little') for i in range(0, 12, 4)[:num_rails]],
            controller_first_fault_detect=int.from_bytes(data[12:16], byteorder='little'),
            rail_first_fault_detect=[int.from_bytes(data[i:i+4], byteorder='little') for i in range(16, 28, 4)[:num_rails]],
            phase_first_fault_detect_a=int.from_bytes(data[28:32], byteorder='little'),
            phase_first_fault_detect_b=int.from_bytes(data[32:36], byteorder='little'),
            controller_adc_fault_detect=int.from_bytes(data[42:44], byteorder='little'),
            # Rail fields are inverted within the blackbox words: rail N sits at a
            # higher offset than rail N+1 when both share a word.
            status_word=[int.from_bytes(data[i:i+2], byteorder='little') for i in (40, 46, 44)[:num_rails]],
            status_mfr_specific=data[50],
            status_cml=data[51],
            status_iout=[data[i] for i in (54, 53, 52)[:num_rails]],
            status_vout=[data[i] for i in (49, 48, 55)[:num_rails]],
            status_temperature=[data[i] for i in (59, 58, 57)[:num_rails]],
            status_input=[data[i] for i in (56, 63, 62)[:num_rails]],
            read_vin=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in (60, 66, 64)[:num_rails]],
            read_vout=[int.from_bytes(data[i:i+2], byteorder='little') for i in (70, 68, 74)[:num_rails]],
            read_iin=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in (72, 78, 76)[:num_rails]],
            read_iout=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in (82, 80, 86)[:num_rails]],
            controller_read_temperature=int.from_bytes(data[90:91], byteorder='little', signed=True),
            rail_read_temperature=[int.from_bytes(data[i:i+1], byteorder='little', signed=True) for i in (85, 84, 91)[:num_rails]],
            vmon_voltage=int.from_bytes(data[94:96], byteorder='little', signed=True),
            vinsen_voltage=int.from_bytes(data[98:100], byteorder='little', signed=True),
            # Phase statuses are inverted within the blackbox words
            phase_temperature=[
                int.from_bytes(data[chunk + 3 - i:chunk + 4 - i], byteorder='little', signed=True)
                for chunk in range(100, 112, 4)
                for i in range(4)
            ],
            phase_current=[
                int.from_bytes(data[chunk + 2 - i*2:chunk + 4 - i*2], byteorder='little', signed=True)
                for chunk in range(112, 136, 4)
                for i in range(2)
            ],
        )

    def is_valid(self) -> bool:
        """Returns True if the record is valid to be used, False otherwise."""
        if(len(self.raw) == self.NVM_RECORD_SIZE):
            if(self.nvm_written_status == 0):
                return True
        elif(len(self.raw) == self.RAM_RECORD_SIZE):
            for byte in self.raw:
                if byte != 0:
                    return True
        return False

    def as_dict(self) -> dict[str, str]:
        """Returns a dictionary representation of this record.

        The returned dictionary is helpful for rendering the record in nh_reboot_cause.
        For simplicity, only a subset of the fields are included in the dictionary representation.
        """
        def raw_pretty(data: bytes) -> str:
            """Returns multi-line hex dump (rows of 16 bytes each)."""
            rows = []
            for i in range(0, len(data), 16):
                chunk = data[i : i + 16]
                rows.append(" ".join(f"{b:02x}" for b in chunk))
            return "\n".join(rows)

        def multi_line_format(phase_values: list[str], width_offset: int) -> str:
            """Formats phase values as a multi-line string with one phase per line."""
            line_width = 65 + width_offset
            lines = []
            line = ""
            for i, val in enumerate(phase_values):
                if len(line) == 0:
                    line = val
                elif len(line) + len(val) <= line_width:
                    line += f", {val}"
                else:
                    line += ","
                    lines.append(line)
                    line = val
            if line:
                lines.append(line)
            return "\n".join(lines)

        num_rails = len(self.rail_uptime_counter)

        return {
            "name": self.name,
            f"rail_uptime[{num_rails}]": ", ".join([f"{counter / 10:.2f}s" for counter in self.rail_uptime_counter]),
            "controller_first_fault_detect": f"0b{self.controller_first_fault_detect:032b}",
            f"rail_first_fault_detect[{num_rails}]": multi_line_format(
                [f"0b{val:032b}" for val in self.rail_first_fault_detect],
                -len(f"rail_first_fault_detect[{num_rails}]: ")
            ),
            "phase_first_fault_detect_a": f"0b{self.phase_first_fault_detect_a:032b}",
            "phase_first_fault_detect_b": f"0b{self.phase_first_fault_detect_b:032b}",
            "controller_adc_fault_detect": f"0b{self.controller_adc_fault_detect:016b}",
            f"status_word[{num_rails}]": multi_line_format(
                [f"0b{val:016b}" for val in self.status_word],
                -len(f"status_word[{num_rails}]: ")
            ),
            "status_mfr_specific": f"0b{self.status_mfr_specific:08b}",
            "status_cml": f"0b{self.status_cml:08b}",
            f"status_iout[{num_rails}]": ", ".join([f"0b{val:08b}" for val in self.status_iout]),
            f"status_vout[{num_rails}]": ", ".join([f"0b{val:08b}" for val in self.status_vout]),
            f"status_temperature[{num_rails}]": ", ".join([f"0b{val:08b}" for val in self.status_temperature]),
            f"status_input[{num_rails}]": ", ".join([f"0b{val:08b}" for val in self.status_input]),
            f"read_vin[{num_rails}]": ", ".join([f"{val / 100:.2f}V" for val in self.read_vin]),
            f"read_vout[{num_rails}]": ", ".join([f"{val / 1000:.2f}V" for val in self.read_vout]),
            f"read_iin[{num_rails}]": ", ".join([f"{val / 100:.2f}A" for val in self.read_iin]),
            f"read_iout[{num_rails}]": ", ".join([f"{val / 10:.2f}A" for val in self.read_iout]),
            "controller_read_temperature": f"{self.controller_read_temperature * 2:.2f}°C",
            f"rail_read_temperature[{num_rails}]": ", ".join([f"{val * 2:.2f}°C" for val in self.rail_read_temperature]),
            "vmon_voltage": f"{self.vmon_voltage / 100:.2f}V",
            "vinsen_voltage": f"{self.vinsen_voltage / 100:.2f}V",
            "phase_temperature[12]": multi_line_format(
                [f"{val * 2:.2f}°C" for val in self.phase_temperature],
                -len("phase_temperature[12]: ")
            ),
            "phase_current[12]": multi_line_format(
                [f"{val / 10:.2f}A" for val in self.phase_current],
                -len("phase_current[12]: ")
            ),
            "raw": raw_pretty(self.raw),
        }


class RaaDmpvr2Dcdc(Dcdc):
    """Renesas Digital Multiphase Voltage Regulator Devices. Generation 2
    
    Implements blackbox interface common between all Gen2 devices.
    """

    NVM_NUM_RECORDS = 10

    def get_blackbox_raw(self, from_ram=True) -> bytes:
        """Reads blackbox bytes from the RAA chip's sysfs interface.

        Reads from either the RAM or NVM depending on the configuration
        of the value of the `from_ram` parameter.

        Returns:
            List of bytes, raw blackbox data
        """
        if from_ram:
            if self._debugfs_dir:
                mfr_status_file = self.registers.get("STATUS_MFR_SPECIFIC").debugfs_attribute
                mfr_status_val = self.read_debugfs_file(os.path.join(self._debugfs_dir, mfr_status_file))
                if mfr_status_val != -1 and (mfr_status_val & (1 << 3) == 0): # BLACKBOX_EVENT bit not set
                    return b""

            # Check for blackbox_ram in hwmon first, then debugfs if not found
            blackbox_path = os.path.join(self._hwmon_dir or "", "blackbox_ram")
            if not os.path.exists(blackbox_path):
                blackbox_path = os.path.join(self._debugfs_dir or "", self.dev_type, "blackbox_ram")
                if not os.path.exists(blackbox_path):
                    raise FileNotFoundError(f"Blackbox RAM file not found for {self.name} at {blackbox_path}")
        else:
            blackbox_path = self._nvmem_path

        raw_bytes = b""
        with open(blackbox_path, "rb") as f:
            raw_bytes = f.read()

        return raw_bytes

    def clear_blackbox(self, from_ram=True):
        """Clears the blackbox data from the DCDC memory."""
        return # No mechanism to clear blackbox

    def decode_blackbox_records(self, raw: bytes) -> list[BlackBoxRecordBase]:
        """Decodes raw blackbox data into list of records

        Empty records and non-valid records are discarded from the output.
        Accepts both NVM and RAM blackbox data

        Returns:
            List of non-empty blackbox records. Max 10 for NVM and 1 for RAM.
        """

        record_size = 0
        if(len(raw) % RaaDmpvr2BlackBoxRecord.RAM_RECORD_SIZE == 0):
            record_size = RaaDmpvr2BlackBoxRecord.RAM_RECORD_SIZE
        elif(len(raw) % RaaDmpvr2BlackBoxRecord.NVM_RECORD_SIZE == 0):
            record_size = RaaDmpvr2BlackBoxRecord.NVM_RECORD_SIZE
        else:
            return []

        records = []
        for r in range(len(raw) // record_size):
            raw_record = raw[r * record_size : (r + 1) * record_size]
            record = RaaDmpvr2BlackBoxRecord.from_bytes(raw_record, f"{self.name}:{self.dev_type}", self.NUM_RAILS)
            if record.is_valid():
                records.append(record)

        return records


@Dcdc.register_device("isl68225")
class Isl68225Dcdc(RaaDmpvr2Dcdc):
    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1001_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1111_1110,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1101_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1011,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
            "STATUS_MFR_SPECIFIC": StatusRegister("STATUS_MFR_SPECIFIC", 8,  0b1011_1110, {
                7: "ADC_UNLOCK",
                # bit 6 reserved
                5: "CFP_FAULT",
                4: "INTERNAL_TEMP_FAULT",
                3: "BLACKBOX_EVENT",
                2: "LMS_EVENT",
                1: "SPS_FAULT",
                # bit 0 reserved
            }, "status0_mfr", True),
        }


@dataclass
@BlackBoxRecordBase.register_record("raa228234", "raa228236", "raa228244")
class RaaDmpvr3BlackBoxRecord(BlackBoxRecordBase):
    """Renesas digital multiphase Gen 3 blackbox record. Refer to the 
    Renesas Gen 3 Black Box Guide section 6 for field information.

    Represents a single fault record from the Gen3 blackbox.
    """
    # Header only exists on NVM blackbox records
    # records in RAM start with the first rail uptime counter
    # 4 bytes
    header: int
    # Per rail, 4 bytes each [7:0]
    rail_uptime_counter: list[int]
    # 4 bytes [11:8]
    controller_first_fault_detect: int
    # Per rail, 4 bytes each [19:12]
    rail_first_fault_detect: list[int]
    # 4 bytes [23:20]
    phase_uc_fault_detect: int
    # 4 bytes [27:24]
    phase_oc_fault_detect: int
    # 4 bytes [31:28]
    adc_uc_fault_detect: int
    # 4 bytes [35:32]
    adc_oc_fault_detect: int
    # Per rail, 2 bytes each [51:48]
    status_word: list[int]
    # 1 byte [52:52]
    status_mfr_specific: int
    # 1 byte [53:53]
    status_cml: int
    # Per rail, 1 byte each [57:56]
    status_iout: list[int]
    # Per rail, 1 byte each [59:58]
    status_vout: list[int]
    # Per rail, 1 byte each [61:60]
    status_input: list[int]
    # Per rail, 1 byte each [63:62]
    status_temperature: list[int]
    # Per rail, 2 bytes each [67:64]
    read_vin: list[int]
    # Per rail, 2 bytes each [71:68]
    read_vout: list[int]
    # Per rail, 2 bytes each [75:72]
    read_iin: list[int]
    # Per rail, 2 bytes each [79:76]
    read_iout: list[int]
    # Per rail, 1 byte each [81:80]
    rail_read_temperature: list[int]
    # 1 byte [82:82]
    controller_read_temperature: int
    # Per rail, 2 bytes each [91:88]
    vmon_iinsen: list[int]
    # 20 phases in total
    # Per phase, 1 byte each [111:92]
    phase_temperature: list[int]
    # Per phase, 2 bytes each [151:112]
    phase_current: list[int]

    NVM_RECORD_SIZE = 184
    RAM_RECORD_SIZE = 176

    @classmethod
    def from_bytes(cls, data: bytes, dev_name: str):
        """Creates RaaDmpvr3BlackBoxRecord from raw bytes.

        Args:
            data: 176 (RAM) or 184 (NVM) byte data representing a black box record
            dev_name: Name of the device for this black box (for display purposes)

        Returns:
            Parsed BlackBoxRecordBase
        """

        raw = data
        if(len(data) == cls.NVM_RECORD_SIZE):
            header = int.from_bytes(data[0:4], byteorder='little')
            data = data[4:] # strip header from data
        elif(len(data) == cls.RAM_RECORD_SIZE):
            header = 0
        else:
            raise ValueError(f"Invalid data length for RaaDmpvr3BlackBoxRecord: {len(data)} bytes")

        return cls(
            name=dev_name,
            raw=raw,
            header=header,
            rail_uptime_counter=[int.from_bytes(data[i:i+4], byteorder='little') for i in range(0, 8, 4)],
            controller_first_fault_detect=int.from_bytes(data[8:12], byteorder='little'),
            rail_first_fault_detect=[int.from_bytes(data[i:i+4], byteorder='little') for i in range(12, 20, 4)],
            phase_uc_fault_detect=int.from_bytes(data[20:24], byteorder='little'),
            phase_oc_fault_detect=int.from_bytes(data[24:28], byteorder='little'),
            adc_uc_fault_detect=int.from_bytes(data[28:32], byteorder='little'),
            adc_oc_fault_detect=int.from_bytes(data[32:36], byteorder='little'),
            status_word=[int.from_bytes(data[i:i+2], byteorder='little') for i in range(48, 52, 2)],
            status_mfr_specific=data[52],
            status_cml=data[53],
            status_iout=[data[i] for i in range(56, 58)],
            status_vout=[data[i] for i in range(58, 60)],
            status_input=[data[i] for i in range(60, 62)],
            status_temperature=[data[i] for i in range(62, 64)],
            read_vin=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in range(64, 68, 2)],
            read_vout=[int.from_bytes(data[i:i+2], byteorder='little') for i in range(68, 72, 2)],
            read_iin=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in range(72, 76, 2)],
            read_iout=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in range(76, 80, 2)],
            rail_read_temperature=[int.from_bytes(data[i:i+1], byteorder='little', signed=True) for i in range(80, 82)],
            controller_read_temperature=int.from_bytes(data[82:83], byteorder='little', signed=True),
            vmon_iinsen=[int.from_bytes(data[i:i+2], byteorder='little', signed=True) for i in range(88, 92, 2)],
            # Phase statuses are inverted within the blackbox words
            # Decoding the fields using big-endian order...
            phase_temperature=[
                int.from_bytes(data[chunk + 3 - i:chunk + 4 - i], byteorder='little', signed=True)
                for chunk in range(92, 112, 4)
                for i in range(4)
            ],
            phase_current=[
                int.from_bytes(data[chunk + 2 - i*2 : chunk + 4 - i*2], byteorder='little', signed=True)
                for chunk in range(112, 152, 4)
                for i in range(2)
            ],
        )

    def is_valid(self) -> bool:
        """Returns True if the record is valid to be used, False otherwise."""
        if(len(self.raw) == self.NVM_RECORD_SIZE):
            if(self.header != 0):
                return True
        elif(len(self.raw) == self.RAM_RECORD_SIZE):
            for byte in self.raw:
                if byte != 0:
                    return True
        return False

    def as_dict(self) -> dict[str, str]:
        """Returns a dictionary representation of this record.

        The returned dictionary is helpful for rendering the record in nh_reboot_cause.
        For simplicity, only a subset of the fields are included in the dictionary representation.
        """
        def raw_pretty(data: bytes) -> str:
            """Returns multi-line hex dump (rows of 16 bytes each)."""
            rows = []
            for i in range(0, len(data), 16):
                chunk = data[i : i + 16]
                rows.append(" ".join(f"{b:02x}" for b in chunk))
            return "\n".join(rows)

        def multi_line_format(phase_values: list[str], width_offset: int) -> str:
            """Formats phase values as a multi-line string with one phase per line."""
            line_width = 65 + width_offset
            lines = []
            line = ""
            for i, val in enumerate(phase_values):
                if len(line) == 0:
                    line = val
                elif len(line) + len(val) <= line_width:
                    line += f", {val}"
                else:
                    line += ","
                    lines.append(line)
                    line = val
            if line:
                lines.append(line)
            return "\n".join(lines)

        return {
            "name": self.name,
            "rail_uptime[2]": ", ".join([f"{counter / 10:.2f}s" for counter in self.rail_uptime_counter]),
            "controller_first_fault_detect": f"0b{self.controller_first_fault_detect:032b}",
            "rail_first_fault_detect[2]": multi_line_format([f"0b{val:032b}" for val in self.rail_first_fault_detect], -len("rail_first_fault_detect[2]: ")),
            "phase_uc_fault_detect": f"0b{self.phase_uc_fault_detect:032b}",
            "phase_oc_fault_detect": f"0b{self.phase_oc_fault_detect:032b}",
            "adc_uc_fault_detect": f"0b{self.adc_uc_fault_detect:032b}",
            "adc_oc_fault_detect": f"0b{self.adc_oc_fault_detect:032b}",
            "status_word[2]": ", ".join([f"0b{val:016b}" for val in self.status_word]),
            "status_mfr_specific": f"0b{self.status_mfr_specific:08b}",
            "status_cml": f"0b{self.status_cml:08b}",
            "status_iout[2]": ", ".join([f"0b{val:08b}" for val in self.status_iout]),
            "status_vout[2]": ", ".join([f"0b{val:08b}" for val in self.status_vout]),
            "status_input[2]": ", ".join([f"0b{val:08b}" for val in self.status_input]),
            "status_temperature[2]": ", ".join([f"0b{val:08b}" for val in self.status_temperature]),
            "read_vin[2]": ", ".join([f"{val / 100:.2f}V" for val in self.read_vin]),
            "read_vout[2]": ", ".join([f"{val / 1000:.2f}V" for val in self.read_vout]),
            "read_iin[2]": ", ".join([f"{val / 100:.2f}A" for val in self.read_iin]),
            "read_iout[2]": ", ".join([f"{val / 10:.2f}A" for val in self.read_iout]),
            "controller_read_temperature": f"{self.controller_read_temperature * 2:.2f}°C",
            "rail_read_temperature[2]": ", ".join([f"{val * 2:.2f}°C" for val in self.rail_read_temperature]),
            "vmon_iinsen[2]": ", ".join([f"{val / 100:.2f}A" for val in self.vmon_iinsen]),
            "phase_temperature[20]": multi_line_format([f"{val * 2:.2f}°C" for val in self.phase_temperature], -len("phase_temperature[20]: ")),
            "phase_current[20]": multi_line_format([f"{val / 10:.2f}A" for val in self.phase_current], -len("phase_current[20]: ")),
            "raw": raw_pretty(self.raw),
        }


class RaaDmpvr3Dcdc(Dcdc):
    """Renesas Digital Multiphase Voltage Regulator Devices. Generation 3 and 3.5
    
    Implements blackbox interface common between all Gen3 and Gen3.5 devices.
    """

    NVM_NUM_RECORDS = 10

    def get_blackbox_raw(self, from_ram=True) -> bytes:
        """Reads blackbox bytes from the RAA chip's sysfs interface.

        Reads from either the RAM or NVM depending on the configuration
        of the value of the `from_ram` parameter.

        Returns:
            List of bytes, raw blackbox data
        """
        if from_ram:
            if self._debugfs_dir:
                mfr_status_file = self.registers.get("STATUS_MFR_SPECIFIC").debugfs_attribute
                mfr_status_val = self.read_debugfs_file(os.path.join(self._debugfs_dir, mfr_status_file))
                if mfr_status_val != -1 and (mfr_status_val & (1 << 3) == 0): # BLACKBOX_EVENT bit not set
                    return b""

            # Check for blackbox_ram in hwmon first, then debugfs if not found
            blackbox_path = os.path.join(self._hwmon_dir or "", "blackbox_ram")
            if not os.path.exists(blackbox_path):
                blackbox_path = os.path.join(self._debugfs_dir or "", self.dev_type, "blackbox_ram")
                if not os.path.exists(blackbox_path):
                    raise FileNotFoundError(f"Blackbox RAM file not found for {self.name} at {blackbox_path}")
        else:
            blackbox_path = self._nvmem_path

        raw_bytes = b""
        with open(blackbox_path, "rb") as f:
            raw_bytes = f.read()

        return raw_bytes

    def clear_blackbox(self, from_ram=True):
        """Clears the blackbox data from the DCDC memory."""
        return # No mechanism to clear blackbox

    def decode_blackbox_records(self, raw: bytes) -> list[BlackBoxRecordBase]:
        """Decodes raw blackbox data into list of records

        Empty records and non-valid records are discarded from the output.
        Accepts both NVM and RAM blackbox data

        Returns:
            List of non-empty blackbox records. Max 10 for NVM and 1 for RAM.
        """

        record_size = 0
        if(len(raw) % RaaDmpvr3BlackBoxRecord.RAM_RECORD_SIZE == 0):
            record_size = RaaDmpvr3BlackBoxRecord.RAM_RECORD_SIZE
        elif(len(raw) % RaaDmpvr3BlackBoxRecord.NVM_RECORD_SIZE == 0):
            record_size = RaaDmpvr3BlackBoxRecord.NVM_RECORD_SIZE
        else:
            return []

        records = []
        for r in range(len(raw) // record_size):
            raw_record = raw[r * record_size : (r + 1) * record_size]
            record = RaaDmpvr3BlackBoxRecord.from_bytes(raw_record, f"{self.name}:{self.dev_type}")
            if record.is_valid():
                records.append(record)

        return records


@Dcdc.register_device("raa228234")
class Raa228234Dcdc(RaaDmpvr3Dcdc):
    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1001_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1111_1110,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1101_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1011,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
            "STATUS_MFR_SPECIFIC": StatusRegister("STATUS_MFR_SPECIFIC", 8,  0b1011_1110, {
                7: "ADC_UNLOCK",
                # bit 6 reserved
                5: "CFP_FAULT",
                4: "INTERNAL_TEMP_FAULT",
                3: "BLACKBOX_EVENT",
                2: "LMS_EVENT",
                1: "SPS_FAULT",
                # bit 0 reserved
            }, "status0_mfr", True),
        }


@Dcdc.register_device("raa228236")
class Raa228236Dcdc(RaaDmpvr3Dcdc):
    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1001_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1111_1110,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1101_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1011,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
            "STATUS_MFR_SPECIFIC": StatusRegister("STATUS_MFR_SPECIFIC", 8,  0b1011_1110, {
                7: "ADC_UNLOCK",
                # bit 6 reserved
                5: "CFP_FAULT",
                4: "INTERNAL_TEMP_FAULT",
                3: "BLACKBOX_EVENT",
                2: "LMS_EVENT",
                1: "SPS_FAULT",
                # bit 0 reserved
            }, "status0_mfr", True),
        }


@Dcdc.register_device("raa228244")
class Raa228244Dcdc(RaaDmpvr3Dcdc):
    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1001_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1001_1000,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1111_1000,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1111_1110,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1101_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1011,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
            "STATUS_MFR_SPECIFIC": StatusRegister("STATUS_MFR_SPECIFIC", 8,  0b1111_1111, {
                7: "ADC_UNLOCK",
                6: "PSYS_OR_IIN_SENSE_WARNING",
                5: "CFP_FAULT",
                4: "INTERNAL_TEMP_FAULT",
                3: "BLACKBOX_EVENT",
                2: "LMS_EVENT",
                1: "SPS_FAULT",
                0: "SVI3_ERROR"
            }, "status0_mfr", True),
        }


@Dcdc.register_device("xdpe1a2g5b")
class Xdpe1a2g5bDcdc(Dcdc):
    def _build_registers(self) -> dict[str, StatusRegister]:
        return {
            "STATUS_WORD":         StatusRegister("STATUS_WORD",         16, 0b1111_1111_1111_1111, STATUS_BITFIELDS["STATUS_WORD"],        "status{rail}",       False),
            "STATUS_VOUT":         StatusRegister("STATUS_VOUT",         8,  0b1111_1110,           STATUS_BITFIELDS["STATUS_VOUT"],        "status{rail}_vout",  False),
            "STATUS_IOUT":         StatusRegister("STATUS_IOUT",         8,  0b1110_0001,           STATUS_BITFIELDS["STATUS_IOUT"],        "status{rail}_iout",  False),
            "STATUS_INPUT":        StatusRegister("STATUS_INPUT",        8,  0b1110_1111,           STATUS_BITFIELDS["STATUS_INPUT"],       "status{rail}_input", False),
            "STATUS_TEMPERATURE":  StatusRegister("STATUS_TEMPERATURE",  8,  0b1100_0000,           STATUS_BITFIELDS["STATUS_TEMPERATURE"], "status{rail}_temp",  False),
            "STATUS_CML":          StatusRegister("STATUS_CML",          8,  0b1111_1010,           STATUS_BITFIELDS["STATUS_CML"],         "status0_cml",        True),
            "STATUS_OTHER":        StatusRegister("STATUS_OTHER",        8,  0b0000_0001, {
                # bits 7-1 reserved
                0: "ASSERTED_SMBALERT#",
            }, "status0_other",  True),
            "STATUS_MFR_SPECIFIC": StatusRegister("STATUS_MFR_SPECIFIC", 8,  0b0010_0010, {
                # bits 7-6 reserved
                5: "COMMON_FAULT",
                # bits 4-2 reserved
                1: "TSEN_HIGH_FAULT",
                # bit 0 reserved
            }, "status0_mfr", True),
        }
