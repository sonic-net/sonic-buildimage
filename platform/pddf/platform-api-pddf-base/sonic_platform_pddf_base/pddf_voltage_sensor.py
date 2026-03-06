
import glob
import os

try:
    from sonic_platform_base.sensor_base import VoltageSensorBase, SensorBase
    from subprocess import getstatusoutput
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

try:
    import smbus2
    HAS_SMBUS2 = True
except ImportError:
    smbus2 = None
    HAS_SMBUS2 = False

# PMBus register addresses (PMBus Specification Part II, Rev 1.3)
PMBUS_VOUT_MODE = 0x20
PMBUS_READ_VOUT = 0x8B

# PMBus VOUT_MODE encoding (bits [7:5])
PMBUS_VOUT_MODE_LINEAR = 0
PMBUS_VOUT_MODE_VID = 1
PMBUS_VOUT_MODE_SHIFT = 5        # mode field starts at bit 5
PMBUS_VOUT_MODE_MASK = 0x07       # 3-bit mode field in bits [7:5]
PMBUS_VOUT_EXP_MASK = 0x1F       # 5-bit exponent/parameter field in bits [4:0]
PMBUS_VOUT_EXP_SIGN_BIT = 15     # exponent values > 15 are negative (sign-extend)
PMBUS_VOUT_EXP_OFFSET = 32       # subtracted to sign-extend 5-bit exponent

# PMBus VID table IDs (VOUT_MODE bits [4:0] when mode == VID)
PMBUS_VID_TABLE_VR12 = 0         # VR12
PMBUS_VID_TABLE_VR12_MAX = 2     # VR12 (table 0,1) and VR12.5 (table 2)
PMBUS_VID_TABLE_VR13 = 3         # VR13

# VID voltage lookup parameters (millivolts)
VID_VR12_BASE_MV = 250           # VR12/VR12.5 base voltage
VID_VR12_STEP_MV = 5             # VR12/VR12.5 step per code
VID_VR13_BASE_MV = 500           # VR13 base voltage
VID_VR13_STEP_MV = 10            # VR13 step per code

# Millivolts conversion factor
MV_PER_VOLT = 1000

# PCA9548 MUX deselect value (all channels off)
MUX_DESELECT = 0x00

# Track device-binding attempts per (bus, addr) so we only try once.
_bind_attempted = set()


class PddfVoltageSensor(VoltageSensorBase):
    """PDDF generic Voltage Sensor class.

    Supports three sensor read methods based on sensor_type in dev_attr:
      - hwmon (default): reads hwmon sysfs via standard PDDF path
      - iio: reads IIO sysfs (iio:deviceX/in_voltageN_raw)
      - smbus: reads raw I2C PMBus registers (VOUT_MODE + READ_VOUT)

    Scaling (multi_factor / decimate_factor) is applied to all reads.
    Thresholds are read from JSON dev_attr if present, falling back to sysfs.
    """
    pddf_obj = {}
    plugin_data = {}

    def __init__(self, index, pddf_data=None, pddf_plugin_data=None):
        if not pddf_data or not pddf_plugin_data:
            raise ValueError('PDDF JSON data error')

        self.pddf_obj = pddf_data
        self.plugin_data = pddf_plugin_data
        self.sensor_index = index + 1
        
        self.sensor_obj_name = f"VOLTAGE{self.sensor_index}"
        if self.sensor_obj_name not in self.pddf_obj.data:
            raise ValueError(f"Voltage sensor {self.sensor_obj_name} not found in PDDF data")

        self.sensor_obj = self.pddf_obj.data[self.sensor_obj_name]

        # dev_attr may be at top level or under i2c
        self._top_dev_attr = self.sensor_obj.get('dev_attr', {})
        self._i2c_dev_attr = self.sensor_obj.get('i2c', {}).get('dev_attr', {})

        self.display_name = (self._top_dev_attr.get('display_name') or
                             self._i2c_dev_attr.get('display_name', self.sensor_obj_name))

        # Sensor type: 'iio', 'smbus', or '' (hwmon default)
        self._sensor_type = self._i2c_dev_attr.get('sensor_type', '')

        # Scaling factors (default 1:1 if missing or invalid in JSON)
        try:
            self._multi_factor = float(
                self._top_dev_attr.get('multi_factor',
                    self._i2c_dev_attr.get('multi_factor', '1')))
        except (ValueError, TypeError):
            self._multi_factor = 1.0
        try:
            self._decimate_factor = float(
                self._top_dev_attr.get('decimate_factor',
                    self._i2c_dev_attr.get('decimate_factor', '1')))
        except (ValueError, TypeError):
            self._decimate_factor = 1.0

        # IIO channel ID (for sensor_type='iio')
        try:
            self._channel_id = int(self._i2c_dev_attr.get('channel_id', '0'))
        except (ValueError, TypeError):
            self._channel_id = 0

        # Cached IIO sysfs path (populated on first read)
        self._cached_iio_path = None

    def get_name(self):
        return self.display_name

    # ------------------------------------------------------------------ #
    #  Value read — dispatches by sensor_type
    # ------------------------------------------------------------------ #
    def get_value(self):
        """Read voltage value in millivolts.  Returns float or None."""
        if self._sensor_type == 'iio':
            return self._read_iio()
        elif self._sensor_type == 'smbus':
            return self._read_smbus()
        else:
            return self._read_hwmon()

    def _read_hwmon(self):
        """Read via standard PDDF hwmon sysfs path."""
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, "volt1_input")
        if not output:
            return None

        if output['status'].isalpha():
            return None

        raw = float(output['status'])
        return self._scale(raw)

    # ------------------------------------------------------------------ #
    #  IIO read path
    # ------------------------------------------------------------------ #
    def _read_iio(self):
        """Read voltage via IIO sysfs (in_voltageN_raw).

        Discovers the IIO device under the I2C device path and caches
        the sysfs path for subsequent reads.  Creates the I2C device
        if it doesn't exist (PDDF doesn't auto-create for VOLTAGE_SENSOR).
        """
        # Fast path: use cached sysfs path
        if self._cached_iio_path:
            try:
                with open(self._cached_iio_path, 'r') as f:
                    raw = float(f.read().strip())
                    return self._scale(raw)
            except (IOError, ValueError):
                self._cached_iio_path = None  # Re-discover

        i2c_path = self._find_i2c_device_path()
        if not i2c_path:
            return None

        iio_dirs = glob.glob(os.path.join(i2c_path, 'iio:device*'))
        if not iio_dirs:
            return None

        raw_file = os.path.join(iio_dirs[0],
                                'in_voltage{}_raw'.format(self._channel_id))
        if not os.path.isfile(raw_file):
            return None

        self._cached_iio_path = raw_file
        try:
            with open(raw_file, 'r') as f:
                raw = float(f.read().strip())
                return self._scale(raw)
        except (IOError, ValueError):
            return None

    # ------------------------------------------------------------------ #
    #  SMBus read path — raw PMBus register reads
    # ------------------------------------------------------------------ #
    def _read_smbus(self):
        """Read voltage via raw SMBus PMBus registers.

        Used for PMBus chips that lack a STATUS register (e.g. max20796),
        preventing the kernel pmbus driver from binding properly.
        Reads VOUT_MODE (0x20) and READ_VOUT (0x8b) directly.

        For sensors behind a 2-level MUX cascade (mux2_addr set in dev_attr),
        uses the parent bus with manual MUX channel selection.

        Returns millivolts (float) or None.
        """
        if not HAS_SMBUS2:
            return None

        dev_attr = self._i2c_dev_attr
        mux2_addr = dev_attr.get('mux2_addr', '')
        mux2_chan = dev_attr.get('mux2_chan', '')

        if mux2_addr and mux2_chan:
            return self._read_smbus_via_parent()

        bus, addr = self._resolve_bus_addr()
        if bus is None:
            return None

        try:
            with smbus2.SMBus(bus) as smb:
                vout_mode = smb.read_byte_data(addr, PMBUS_VOUT_MODE)
                raw_vout = smb.read_word_data(addr, PMBUS_READ_VOUT)
        except (OSError, IOError):
            return None

        mv = self._decode_pmbus_vout(vout_mode, raw_vout)
        return self._scale(mv) if mv is not None else None

    def _read_smbus_via_parent(self):
        """Read via parent bus with manual MUX channel selection.

        For sensors behind cascaded MUXes where the kernel path is
        unreliable.  Opens the first-level MUX bus and manually selects
        the second MUX channel before reading PMBus registers.
        """
        topo = self.sensor_obj.get('i2c', {}).get('topo_info', {})
        dev_attr = self._i2c_dev_attr
        addr = int(topo.get('dev_addr', '0'), 0)

        adapter_name = dev_attr.get('adapter_name', '')
        mux_chan = dev_attr.get('mux_chan', '')
        mux2_addr = int(dev_attr.get('mux2_addr', '0'), 0)
        mux2_chan = int(dev_attr.get('mux2_chan', '0'))

        parent_bus = self._find_i2c_bus_by_adapter(adapter_name, mux_chan)
        if parent_bus is None:
            return None

        mux_select = 1 << mux2_chan  # PCA9548: one-hot channel bitmask
        try:
            # force=True is required here because the kernel i2c-mux driver
            # owns the MUX address; we need to bypass that to do manual
            # channel selection for chips the kernel cannot probe.
            with smbus2.SMBus(parent_bus, force=True) as smb:
                smb.write_byte(mux2_addr, mux_select)
                try:
                    vout_mode = smb.read_byte_data(addr, PMBUS_VOUT_MODE)
                    raw_vout = smb.read_word_data(addr, PMBUS_READ_VOUT)
                finally:
                    try:
                        smb.write_byte(mux2_addr, MUX_DESELECT)
                    except (OSError, IOError):
                        pass
        except (OSError, IOError):
            return None

        mv = self._decode_pmbus_vout(vout_mode, raw_vout)
        return self._scale(mv) if mv is not None else None

    @staticmethod
    def _decode_pmbus_vout(vout_mode, raw_vout):
        """Decode PMBus VOUT to millivolts.

        Supports Linear (mode 0) and VID (mode 1) formats per
        PMBus Specification Part II, Section 8.3.2.
        Returns millivolts (int) or None if format is unsupported.
        """
        mode = (vout_mode >> PMBUS_VOUT_MODE_SHIFT) & PMBUS_VOUT_MODE_MASK
        if mode == PMBUS_VOUT_MODE_LINEAR:
            exp = vout_mode & PMBUS_VOUT_EXP_MASK
            if exp > PMBUS_VOUT_EXP_SIGN_BIT:
                exp -= PMBUS_VOUT_EXP_OFFSET  # sign-extend 5-bit exponent
            return int(raw_vout * (2 ** exp) * MV_PER_VOLT)
        if mode == PMBUS_VOUT_MODE_VID:
            if raw_vout < 1:
                return 0
            vid_table = vout_mode & PMBUS_VOUT_EXP_MASK
            if vid_table <= PMBUS_VID_TABLE_VR12_MAX:
                return VID_VR12_BASE_MV + (raw_vout - 1) * VID_VR12_STEP_MV
            if vid_table == PMBUS_VID_TABLE_VR13:
                return VID_VR13_BASE_MV + (raw_vout - 1) * VID_VR13_STEP_MV
            # Unsupported VID table (e.g. VR13.5, IMVP9)
            return None
        # Unsupported VOUT_MODE (e.g. Direct mode=2)
        return None

    # ------------------------------------------------------------------ #
    #  Scaling
    # ------------------------------------------------------------------ #
    def _scale(self, raw_value):
        """Apply multi/decimate scaling.  Returns millivolts (whole number)."""
        if raw_value is None or self._decimate_factor == 0:
            return None
        # Truncate to integer millivolts, return as float for API consistency
        return float(int(raw_value * self._multi_factor / self._decimate_factor))

    # ------------------------------------------------------------------ #
    #  Thresholds — sysfs first (chip hw), JSON dev_attr fallback
    # ------------------------------------------------------------------ #
    def _get_threshold_from_sysfs(self, attr_name):
        """Read threshold from hwmon sysfs via PDDF attr_list mapping.

        Works when the chip supports the PMBus warn/fault registers
        (e.g. inN_max, inN_min, inN_crit, inN_lcrit) and the JSON
        attr_list maps them via drv_attr_name.
        """
        output = self.pddf_obj.get_attr_name_output(self.sensor_obj_name, attr_name)
        if not output:
            return None
        if output['status'].isalpha():
            return None
        return float(output['status'])

    def _get_threshold_from_json(self, key):
        """Fallback: look up static threshold from dev_attr in JSON.

        Used for chips that don't support warn limit registers
        (e.g. TPS53659/TPS53622 lack VOUT_OV_WARN / VOUT_UV_WARN).
        """
        val = self._top_dev_attr.get(key)
        if val is None:
            val = self._i2c_dev_attr.get(key)
        if val is None:
            return None
        try:
            return float(val)
        except (ValueError, TypeError):
            return None

    def get_low_threshold(self):
        # Try sysfs first (chip hw warn limit)
        val = self._get_threshold_from_sysfs("volt1_low_threshold")
        if val is not None:
            return val
        # Fallback: static JSON threshold
        return self._get_threshold_from_json('low_threshold')

    def get_high_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_high_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('high_threshold')

    def get_high_critical_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_crit_high_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('crit_high_threshold')

    def get_low_critical_threshold(self):
        val = self._get_threshold_from_sysfs("volt1_crit_low_threshold")
        if val is not None:
            return val
        return self._get_threshold_from_json('crit_low_threshold')

    # ------------------------------------------------------------------ #
    #  I2C device path helpers
    # ------------------------------------------------------------------ #
    def _find_i2c_device_path(self):
        """Locate /sys/bus/i2c/devices/{bus}-{addr:04x} from JSON topo_info.

        If the device doesn't exist, attempts to create it via new_device.
        """
        if "virt_parent" in self.sensor_obj.get('dev_info', {}):
            parent_name = self.sensor_obj['dev_info']['virt_parent']
            topo_dev = self.pddf_obj.data.get(parent_name, self.sensor_obj)
        else:
            topo_dev = self.sensor_obj

        topo = topo_dev.get('i2c', {}).get('topo_info', {})
        dev_attr = self.sensor_obj.get('i2c', {}).get('dev_attr', {})

        addr = int(topo.get('dev_addr', '0'), 0)
        dev_type = topo.get('dev_type', '')

        # Discover bus dynamically if adapter_name is set
        adapter_name = dev_attr.get('adapter_name', '')
        if adapter_name:
            bus = self._find_i2c_bus_by_adapter(adapter_name,
                                                 dev_attr.get('mux_chan', ''))
            if bus is None:
                return None
        else:
            bus = int(topo.get('parent_bus', '0'), 0)

        path = '/sys/bus/i2c/devices/{}-{:04x}'.format(bus, addr)
        if os.path.isdir(path):
            return path

        # Device doesn't exist — try to bind the driver
        self._ensure_i2c_device(bus, addr, dev_type)
        if os.path.isdir(path):
            return path
        return None

    @staticmethod
    def _ensure_i2c_device(bus, addr, dev_type):
        """Bind an I2C driver by writing to new_device if not present."""
        key = (bus, addr)
        if key in _bind_attempted:
            return
        _bind_attempted.add(key)

        if not dev_type:
            return

        dev_path = '/sys/bus/i2c/devices/{}-{:04x}'.format(bus, addr)
        new_device = '/sys/bus/i2c/devices/i2c-{}/new_device'.format(bus)

        if os.path.isdir(dev_path):
            return
        if not os.path.exists(new_device):
            return
        try:
            with open(new_device, 'w') as f:
                f.write('{} 0x{:02x}\n'.format(dev_type, addr))
        except OSError:
            pass

    def _resolve_bus_addr(self):
        """Resolve I2C bus number and address from JSON topo_info."""
        topo = self.sensor_obj.get('i2c', {}).get('topo_info', {})
        dev_attr = self._i2c_dev_attr
        addr = int(topo.get('dev_addr', '0'), 0)

        adapter_name = dev_attr.get('adapter_name', '')
        if adapter_name:
            bus = self._find_i2c_bus_by_adapter(adapter_name,
                                                 dev_attr.get('mux_chan', ''))
        else:
            bus = int(topo.get('parent_bus', '0'), 0)
        return (bus, addr) if bus is not None else (None, None)

    @staticmethod
    def _find_i2c_bus_by_adapter(adapter_name, mux_chan=''):
        """Find I2C bus number by scanning adapter names in sysfs.

        Looks for /sys/bus/i2c/devices/i2c-*/name matching adapter_name.
        If mux_chan is specified, finds the child bus with that channel suffix.

        Returns bus number (int) or None.
        """
        if not adapter_name:
            return None
        # Search for parent adapter
        for entry in glob.glob('/sys/bus/i2c/devices/i2c-*/name'):
            try:
                with open(entry, 'r') as f:
                    name = f.read().strip()
                if adapter_name in name:
                    bus = int(entry.split('/i2c-')[1].split('/')[0])
                    if not mux_chan:
                        return bus
                    # Look for MUX child channel
                    chan_glob = '/sys/bus/i2c/devices/i2c-{}/{}-*/channel-{}'.format(
                        bus, bus, mux_chan)
                    chan_dirs = glob.glob(chan_glob)
                    if chan_dirs:
                        # Resolve symlink to get child bus number
                        link = os.readlink(chan_dirs[0])
                        child_bus = int(link.split('i2c-')[-1])
                        return child_bus
            except (IOError, ValueError, IndexError):
                continue
        return None