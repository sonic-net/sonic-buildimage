try:
    import os
    import re
    import time
    from sonic_sfp.sfputilbase import SfpUtilBase
except ImportError as e:
    raise ImportError(str(e) + "- required module not found")


class SfpUtil(SfpUtilBase):
    """Platform specific sfputil class"""

    port_start = 0
    port_end = 53
    ports_in_block = 54
    cplda_sfp_num = 24
    cpldb_sfp_num = 12
    cpldc_sfp_num = 18

    port_to_eeprom_mapping = {}
    port_to_i2c_mapping = {}
    sfp_ports = list(range(0, ports_in_block))
    qsfp_ports = list(range(ports_in_block - 6, ports_in_block))

    def __init__(self):
        for x in range(self.port_start, self.port_end + 1):
            if x < self.cpldb_sfp_num:
                self.port_to_i2c_mapping.update({x: 7})
            elif x < self.cplda_sfp_num + self.cpldb_sfp_num:
                self.port_to_i2c_mapping.update({x: 6})
            else:
                self.port_to_i2c_mapping.update({x: 8})

        for x in range(self.port_start, self.port_end+1):
            eeprom_path = '/sys/bus/i2c/devices/{0}-0050/sfp'+str(x+1)+'_eeprom'
            port_eeprom_path = eeprom_path.format(self.port_to_i2c_mapping[x])
            self.port_to_eeprom_mapping[x] = port_eeprom_path
        SfpUtilBase.__init__(self)

    def get_presence(self, port_num):
        if port_num < self.port_start or port_num > self.port_end:
            return False

        if port_num < self.cpldb_sfp_num:
            presence_path = '/sys/bus/i2c/devices/7-0075/sfp'+str(port_num+1)+'_present'
        elif port_num < self.cpldb_sfp_num + self.cplda_sfp_num:
            presence_path = '/sys/bus/i2c/devices/6-0074/sfp'+str(port_num+1)+'_present'
        else:
            presence_path = '/sys/bus/i2c/devices/8-0076/sfp'+str(port_num+1)+'_present'

        try:
            file = open(presence_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        value = int(file.readline().rstrip())

        file.close()
        if value == 0:
            return True

        return False

    def get_low_power_mode(self, port_num):
        if port_num not in self.qsfp_ports:
            return False

        lowpower_path = '/sys/bus/i2c/devices/8-0076/sfp'+str(port_num+1)+'_lowpower'

        try:
            file = open(lowpower_path)
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        value = int(file.readline().rstrip())

        file.close()
        if value == 1:
            return True

        return False

    def set_low_power_mode(self, port_num, lpmode):
        if port_num not in self.qsfp_ports:
            return False

        lowpower_path = '/sys/bus/i2c/devices/8-0076/sfp'+str(port_num+1)+'_lowpower'

        # LPMode is active high; set or clear the bit accordingly
        if lpmode is True:
            value = 1
        else:
            value = 0

        try:
            file = open(lowpower_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(value))
        file.close()

        return True

    def reset(self, port_num):
        if port_num not in self.qsfp_ports:
            return False
        reset_path = '/sys/bus/i2c/devices/8-0076/sfp'+str(port_num+1)+'_reset'

        try:
            file = open(reset_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(2))
        file.close()

        # Sleep 1 second to allow it to settle
        time.sleep(1)

        try:
            file = open(reset_path, "r+")
        except IOError as e:
            print("Error: unable to open file: %s" % str(e))
            return False

        file.seek(0)
        file.write(str(1))
        file.close()

        return True

    def read_porttab_mappings(self, porttabfile):
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
        # XXX: move the porttab
        # parsing stuff to a separate module, or reuse
        # if something already exists
        for line in f:
            line.strip()
            if re.search("^#", line) is not None:
                continue

            # Parsing logic for 'port_config.ini' file
            if (parse_fmt_port_config_ini):
                # bcm_port is not explicitly listed in port_config.ini format
                # Currently we assume ports are listed in numerical order according to bcm_port
                # so we use the port's position in the file (zero-based) as bcm_port
                portname = line.split()[0]

                bcm_port = str(port_pos_in_file)

                if len(line.split()) >= 4:
                    fp_port_index = int(line.split()[3])
                else:
                    fp_port_index = portname.split("Ethernet").pop()
                    fp_port_index = int(fp_port_index.split("s").pop(0))/4
            else:  # Parsing logic for older 'portmap.ini' file
                (portname, bcm_port) = line.split("=")[1].split(",")[:2]

                fp_port_index = portname.split("Ethernet").pop()
                fp_port_index = int(fp_port_index.split("s").pop(0))/4

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

        """
        print "logical: " +  self.logical
        print "logical to bcm: " + self.logical_to_bcm
        print "logical to physical: " + self.logical_to_physical
        print "physical to logical: " + self.physical_to_logical
        """
