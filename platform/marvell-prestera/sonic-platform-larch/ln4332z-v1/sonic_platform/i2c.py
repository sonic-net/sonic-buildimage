# common i2c logic

try:
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = True
try:
    import smbus
except ImportError as e:
    smbus_present = False


if smbus_present:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg=None):
        if reg is not None:
            return smbus.SMBus(i2c_bus).read_byte_data(i2c_addr, reg)
        else:
            return smbus.SMBus(i2c_bus).read_byte(i2c_addr)
    
    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        if reg is not None:
            return smbus.SMBus(i2c_bus).write_byte_data(i2c_addr, reg, data)
        else:
            return smbus.SMBus(i2c_bus).write_byte(i2c_addr, data)
else:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg=None):
        cmd = ["sudo", "i2cget", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None:
            cmd.append(hex(reg))

        cmdstatus, status = getstatusoutput_noshell(cmd)
        if cmdstatus != 0:
            return cmdstatus

        return int(status, 16)

    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        cmd = ["sudo", "i2cset", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None:
            cmd.append(hex(reg))
        cmd.append(hex(data))

        cmdstatus, status = getstatusoutput_noshell(cmd)
        if cmdstatus != 0:
            return cmdstatus

        return status


def i2c_try_read_byte(i2c_bus, i2c_addr):
    """Read one byte, returning None instead of raising when nothing answers.

    i2c_read_byte_data() reports a failed i2cget by returning the exit status,
    which is indistinguishable from a register that happens to hold that
    value, so it cannot be used to decide whether a device is there.
    """
    if smbus_present:
        try:
            return smbus.SMBus(i2c_bus).read_byte(i2c_addr)
        except Exception:
            return None

    cmdstatus, status = getstatusoutput_noshell(
        ["sudo", "i2cget", "-y", str(i2c_bus), hex(i2c_addr)])
    if cmdstatus != 0:
        return None
    try:
        return int(status, 16)
    except ValueError:
        return None
