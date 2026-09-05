try:
    import os
    import time
    import re
    import subprocess
    from sonic_sfp.sfputilbase import SfpUtilBase
    from sonic_py_common.general import getstatusoutput_noshell
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")

smbus_present = 1

try:
    import smbus
except ImportError as e:
    smbus_present = 0


class SfpUtil(SfpUtilBase):
    """Platform specific sfputil class"""
    _port_start = 1
    _port_end = 257
    ports_in_block = 257

    _port_to_eeprom_mapping = {}

    _qsfp_ports = list(range(_port_start, ports_in_block + 1))

    def __init__(self):
        subprocess.call(["modprobe", "i2c-dev"])
        if not os.path.exists("/sys/bus/i2c/devices/0-0050"):
            with open("/sys/bus/i2c/devices/i2c-0/new_device", 'w') as file:
                file.write("optoe2 0x50")

        eeprom_path = '/sys/bus/i2c/devices/0-0050/eeprom'
        # for x in range(self.port _start, self.port_end +1):
        x = self.port_start
        while(x < self.port_end+1):
            self.port_to_eeprom_mapping[x] = eeprom_path
            x = x + 1
        SfpUtilBase.__init__(self)

    def reset(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False

        port_ps = "/sys/bus/i2c/devices/0-0050/sfp_port_reset"

        try:
            reg_file = open(port_ps, 'w')
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        # toggle reset
        reg_file.seek(0)
        reg_file.write('1')
        time.sleep(1)
        reg_file.seek(0)
        reg_file.write('0')
        reg_file.close()
        return True

    def set_low_power_mode(self, port_nuM, lpmode):
        raise NotImplementedError

    def get_low_power_mode(self, port_num):
        raise NotImplementedError

    def i2c_get(self, device_addr, offset):
        status = 0
        if smbus_present == 0:
            x = ["i2cget", "-y", "0", hex(device_addr), hex(offset)]
            cmdstatus, status = getstatusoutput_noshell(x)
            if cmdstatus != 0:
                return cmdstatus
            status = int(status, 16)
        else:
            bus = smbus.SMBus(0)
            status = bus.read_byte_data(device_addr, offset)
        return status

    def i2c_set(self, device_addr, offset, value):
        if smbus_present == 0:
            cmd = ["i2cset", "-y", "0", hex(device_addr), hex(offset), hex(value)]
            subprocess.call(cmd)
        else:
            bus = smbus.SMBus(0)
            bus.write_byte_data(device_addr, offset, value)

    def get_presence(self, port_num):
        # Check for invalid port_num
        if port_num < self._port_start or port_num > self._port_end:
            return False
        else:
            self.i2c_set(0x70, 0, 0)
            self.i2c_set(0x71, 0, 0)
            self.i2c_set(0x72, 0, 0)
            self.i2c_set(0x73, 0, 0)
            self.i2c_set(0x74, 0, 0)
            reg = (port_num)/8
            offset = reg % 8
            if offset >= 4:
                offset = offset-4
            elif offset < 4:
                offset = offset+4
            bin_offset = 1 << offset

            if port_num >= 0 and port_num <= 63:
                device_reg = 0x70
            elif port_num >= 64 and port_num <= 127:
                device_reg = 0x71
            elif port_num >= 128 and port_num <= 191:
                device_reg = 0x72
            elif port_num >= 192 and port_num <= 255:
                device_reg = 0x73
            elif port_num >= 256 and port_num <= 257:
                device_reg = 0x74

            #print "i2c %d %x %x" % (port_num, device_reg, bin_offset)
            self.i2c_set(device_reg, 0, bin_offset)
            path = "/sys/bus/i2c/devices/0-0050/eeprom"
            try:
                reg_file = open(path)
                reg_file.seek(0o1)
                reg_file.read(0o2)
            except IOError as e:
                return False

            return True

    def read_porttab_mappings(self, porttabfile):
        #print("I am in porttab_mappings")
        logical = []
        logical_to_bcm = {}
        logical_to_physical = {}
        physical_to_logical = {}
        last_fp_port_index = 0
        last_portname = ""
        first = 1
        port_pos_in_file = 0
        parse_fmt_port_config_ini = False

        try:
            f = open(porttabfile)
        except:
            raise

        parse_fmt_port_config_ini = (os.path.basename(porttabfile) == "port_config.ini")
        # Read the porttab file and generate dicts
        # with mapping for future reference.
        #
        # TODO: Refactor this to use the portconfig.py module that now
        # exists as part of the sonic-config-engine package.
        title = []
        for line in f:
            line.strip()
            if re.search("^#", line) is not None:
                # The current format is: # name lanes alias index speed
                # Where the ordering of the columns can vary
                title = line.split()[1:]
                continue
            #print title

            # Parsing logic for 'port_config.ini' file
            if (parse_fmt_port_config_ini):
                # bcm_port is not explicitly listed in port_config.ini format
                # Currently we assume ports are listed in numerical order according to bcm_port
                # so we use the port's position in the file (zero-based) as bcm_port
                portname = line.split()[0]

                bcm_port = str(port_pos_in_file)

                if "index" in title:
                    fp_port_index = int(line.split()[title.index("index")])
                # Leave the old code for backward compatibility
                # if len(line.split()) >= 4:
                #    fp_port_index = (line.split()[3])
                #    print(fp_port_index)
                else:
                    fp_port_index = portname.split("Ethernet").pop()
                    fp_port_index = int(fp_port_index.split("s").pop(0))+1
            else:  # Parsing logic for older 'portmap.ini' file
                (portname, bcm_port) = line.split("=")[1].split(",")[:2]

                fp_port_index = portname.split("Ethernet").pop()
                fp_port_index = int(fp_port_index.split("s").pop(0))+1

            if ((len(self.sfp_ports) > 0) and (fp_port_index not in self.sfp_ports)):
                continue

            if first == 1:
                # Initialize last_[physical|logical]_port
                # to the first valid port
                last_fp_port_index = fp_port_index
                last_portname = portname
                first = 0

            logical.append(portname)

            logical_to_bcm[portname] = "xe" + bcm_port
            logical_to_physical[portname] = [fp_port_index]
            if physical_to_logical.get(fp_port_index) is None:
                physical_to_logical[fp_port_index] = [portname]
            else:
                physical_to_logical[fp_port_index].append(
                    portname)

            if (fp_port_index - last_fp_port_index) > 1:
                # last port was a gang port
                for p in range(last_fp_port_index+1, fp_port_index):
                    logical_to_physical[last_portname].append(p)
                    if physical_to_logical.get(p) is None:
                        physical_to_logical[p] = [last_portname]
                    else:
                        physical_to_logical[p].append(last_portname)

            last_fp_port_index = fp_port_index
            last_portname = portname

            port_pos_in_file += 1

        self.logical = logical
        self.logical_to_bcm = logical_to_bcm
        self.logical_to_physical = logical_to_physical
        self.physical_to_logical = physical_to_logical

        # print(self.logical_to_physical)
        '''print("logical: " + self.logical)
        print("logical to bcm: " + self.logical_to_bcm)
        print("logical to physical: " + self.logical_to_physical)
        print("physical to logical: " + self.physical_to_logical)'''
        #print("exiting port_tab_mappings")

    @property
    def port_start(self):
        return self._port_start

    @property
    def port_end(self):
        return self._port_end

    @property
    def qsfp_ports(self):
        return self._qsfp_ports

    @property
    def port_to_eeprom_mapping(self):
        return self._port_to_eeprom_mapping

    @property
    def get_transceiver_change_event(self):
        raise NotImplementedError
