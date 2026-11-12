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
        bus = smbus.SMBus(i2c_bus)
        return bus.read_byte_data(i2c_addr, reg) if reg is not None else bus.read_byte(i2c_addr)
    
    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        bus = smbus.SMBus(i2c_bus)
        return bus.write_byte_data(i2c_addr, reg, data) if reg is not None else bus.write_byte(i2c_addr, data)

    def i2c_read_word_data(i2c_bus, i2c_addr, reg):
        return smbus.SMBus(i2c_bus).read_word_data(i2c_addr, reg)

    def i2c_write_word_data(i2c_bus, i2c_addr, reg, data):
        return smbus.SMBus(i2c_bus).write_word_data(i2c_addr, reg, data)
else:
    def i2c_read_byte_data(i2c_bus, i2c_addr, reg=None):
        cmd = ["sudo", "i2cget", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None: cmd.append(hex(reg))
        cmdstatus, status = getstatusoutput_noshell(cmd)
        return int(status, 16) if cmdstatus == 0 else -1

    def i2c_write_byte_data(i2c_bus, i2c_addr, reg, data):
        cmd = ["sudo", "i2cset", "-y", str(i2c_bus), hex(i2c_addr)]
        if reg is not None: cmd.append(hex(reg))
        cmd.append(hex(data))
        cmdstatus, _ = getstatusoutput_noshell(cmd)
        return cmdstatus

    def i2c_read_word_data(i2c_bus, i2c_addr, reg):
        # Режим 'w' для читання 16-бітного слова
        cmd = ["sudo", "i2cget", "-y", str(i2c_bus), hex(i2c_addr), hex(reg), "w"]
        cmdstatus, status = getstatusoutput_noshell(cmd)
        return int(status, 16) if cmdstatus == 0 else -1

    def i2c_write_word_data(i2c_bus, i2c_addr, reg, data):
        # Тип 'w' для запису 16-бітного слова
        cmd = ["sudo", "i2cset", "-y", str(i2c_bus), hex(i2c_addr), hex(reg), hex(data), "w"]
        cmdstatus, _ = getstatusoutput_noshell(cmd)
        return cmdstatus
