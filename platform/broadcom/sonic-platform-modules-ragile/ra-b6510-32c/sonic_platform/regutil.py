# -*- coding: utf-8 -*-
from glob import glob
from plat_hal.osutil import osutil

try:
    from sonic_platform.config import DecodeFormat, DecodeMethod

    DECODE_FORMAT = DecodeFormat
    DECODE_METHOD = DecodeMethod
except ImportError:
    raise ImportError(str(e) + "- required module not found")

ERR_CODE = "ERR"


class Reg(object):
    """
    "e2loc": {"bus": 3, "addr": 0x53, "way": "i2c"}
        "value": {
            "loc": "/sys/bus/i2c/devices/2-0048/hwmon/hwmon*/temp1_input",
            "way": "sysfs",

        "InputsStatus": {
            "bus": 8,
            "addr": 0x5B,
            "offset": 0x79,
            "way": "i2cword",
            "mask": 0x0200,
        },
    """

    def __new__(cls, *args):
        if args[0] is None or not isinstance(args[0], dict):
            return None
        return super(Reg, cls).__new__(cls)

    def __init__(self, data):

        self.loc = None
        self.way = DECODE_METHOD.SYSFS
        self.addr = None
        self.bus = None
        self.offset = None
        self.size = 1
        self.bit = None
        self.mask = None
        self.digit = None
        self.sdk_type = None
        self.sep = None
        self.format = DECODE_FORMAT.TEXT
        self.__dict__.update(data)

    def _read_reg_val(self):
        ret = None
        try:
            if self.way == DECODE_METHOD.SYSFS:
                ret = self.get_sysfs()
            elif self.way == DECODE_METHOD.I2C:
                ret = self.get_i2c()
            elif self.way == DECODE_METHOD.I2C_WORD:
                ret = self.get_i2cword()
            elif self.way == DECODE_METHOD.DEVMEM:
                ret = self.get_devmem()
            elif self.way == DECODE_METHOD.SDK:
                # TODO
                pass
            else:
                pass
        except Exception as e:
            raise e

        return ret

    def _write_reg_val(self, val):
        try:
            if self.way == DECODE_METHOD.SYSFS:
                return self._write_sysfs(val)
        except Exception as e:
            raise e

        return False

    def _write_sysfs(self, val):
        try:
            with open(glob(self.loc)[0], "w") as f:
                f.write(val)
                f.flush()
            return True
        except Exception as e:
            raise e

    def _format_val(self, val):
        try:
            if isinstance(val, str):
                val = val.strip()
                if self.format == DECODE_FORMAT.THOUSANDTH:
                    return float("%.1f" % (float(val) / 1000))
                elif self.format == DECODE_FORMAT.HUNDREDTH:
                    return float("%.1f" % (float(val) / 100))
                elif self.format == DECODE_FORMAT.ONE_BIT_HEX:
                    return (int(val, 16) & (1 << self.bit)) >> self.bit
                elif self.format == DECODE_FORMAT.DECIMAL:
                    return int(val, 10)
                elif self.format == DECODE_FORMAT.MILLIONTH:
                    return float("%.1f" % (float(val) / 1000 / 1000))
                elif self.format == DECODE_FORMAT.AND:
                    return (int(val, 16)) & self.mask
            elif isinstance(val, list):
                if self.format == DECODE_FORMAT.JOIN:
                    return self.sep.join(val)
        except Exception as e:
            raise e
        else:
            return val

    def decode(self):
        """
        get value by config way
            way  i2c/sysfs/lpc
        """
        if self.way is None:
            raise ValueError("cannot found way to deal")

        ret = self._read_reg_val()

        ret = self._format_val(ret)
        return ret

    def encode(self, val):
        if self.way is None:
            raise ValueError("cannot found way to deal")

        return self._write_reg_val(val)

    def get_sdk(self):
        # TODO
        pass

    def get_sysfs(self):
        if self.loc is None:
            raise ValueError("Not Enough Attr: loc: {}".format(self.loc))

        ret, val = osutil.readsysfs(self.loc)

        if not ret:
            raise IOError(val)

        return val

    def get_devmem(self):
        if self.addr is None or self.digit is None or self.mask is None:
            raise ValueError(
                "Not Enough Attr: addr: {}, digit: {}, mask: {}".format(
                    self.addr, self.digit, self.mask
                )
            )

        ret, val = osutil.getdevmem(self.addr, self.digit, self.mask)

        if not ret:
            raise IOError(val)

        return val

    def get_i2cword(self):
        if self.bus is None or self.addr is None or self.offset is None:
            raise ValueError(
                "Not Enough Attr: bus: {}, addr: {}, offset: {}".format(
                    self.bus, self.addr, self.offset
                )
            )

        ret, val = osutil.geti2cword(self.bus, self.addr, self.offset)

        if not ret:
            raise IOError(val)

        return val

    def get_i2c(self):
        if (
            self.bus is None
            or self.addr is None
            or self.offset is None
            or self.size is None
        ):
            raise ValueError(
                "Not Enough Attr: bus: {}, addr: {}, offset: {}".format(
                    self.bus, self.addr, self.offset
                )
            )

        value = []
        for i in range(self.size):
            ofs = self.offset + i
            ret, val = osutil.rji2cget(self.bus, self.addr, ofs)

            if not ret:
                raise IOError(val)
            else:
                value.append(repr(chr(val)).translate(None, r"\\x").replace("'", ""))

        return value

    def set_i2cword(self, bus, addr, offset, byte):
        return self.seti2cword(bus, addr, offset, byte)

    def seti2cword(self, bus, addr, offset, byte):
        return osutil.seti2cword(bus, addr, offset, byte)

    def set_i2c(self, bus, addr, offset, byte):
        return self.seti2c(bus, addr, offset, byte)

    def seti2c(self, bus, addr, offset, byte):
        ret, val = osutil.rji2cset(bus, addr, offset, byte)
        return ret, val

    def getbcmtemp(self):
        try:
            sta, ret = osutil.getmactemp()
            if sta == True:
                mac_aver = float(ret.get("average", self.__error_ret))
                #mac_max = float(ret.get("maximum", self.__error_ret))
                mac_aver = mac_aver * 1000
                #mac_max = mac_max * 1000
            else:
                return False, ret
        except AttributeError as e:
            return False, str(e)
        return True, mac_aver

    def getbcmreg(self, reg):
        ret, val = osutil.getsdkreg(reg)
        return ret, val

    def logger_debug(self, msg):
        baseutil.logger_debug(msg)

    def command(self, cmd):
        ret, output = osutil.command(cmd)
        return ret, output

    def set_val(self, val):
        # TODO
        pass
