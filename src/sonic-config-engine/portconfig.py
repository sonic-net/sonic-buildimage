try:
    import ast
    import json
    import os
    import re
    import sys

    from swsscommon import swsscommon
    from sonic_py_common import device_info
    from sonic_py_common.multi_asic import get_asic_id_from_name
except ImportError as e:
    raise ImportError("%s - required module not found" % str(e))

try:
    if os.environ["CFGGEN_UNIT_TESTING"] == "2":
        modules_path = os.path.join(os.path.dirname(__file__), ".")
        tests_path = os.path.join(modules_path, "tests")
        sys.path.insert(0, modules_path)
        sys.path.insert(0, tests_path)
        import mock_tables.dbconnector
        mock_tables.dbconnector.load_namespace_config()

except KeyError:
    pass

# Global Variable
PLATFORM_ROOT_PATH = '/usr/share/sonic/device'
PLATFORM_ROOT_PATH_DOCKER = '/usr/share/sonic/platform'
SONIC_ROOT_PATH = '/usr/share/sonic'
HWSKU_ROOT_PATH = '/usr/share/sonic/hwsku'

PLATFORM_JSON = 'platform.json'
PORT_CONFIG_INI = 'port_config.ini'
HWSKU_JSON = 'hwsku.json'

PORT_STR = "Ethernet"
BRKOUT_MODE = "default_brkout_mode"
CUR_BRKOUT_MODE = "brkout_mode"
INTF_KEY = "interfaces"
OPTIONAL_HWSKU_ATTRIBUTES = ["fec", "autoneg", "role", "subport"]

BRKOUT_PATTERN = r'(\d{1,6})x(\d{1,6}G?)(\[(\d{1,6}G?,?)*\])?(\((\d{1,6})\))?'
BRKOUT_PATTERN_GROUPS = 6

#
# Helper Functions
#

# For python2 compatibility
def py2JsonStrHook(j):
    if isinstance(j, unicode):
        return j.encode('utf-8', 'backslashreplace')
    if isinstance(j, list):
        return [py2JsonStrHook(item) for item in j]
    if isinstance(j, dict):
        return {py2JsonStrHook(key): py2JsonStrHook(value)
            for key, value in j.iteritems()}
    return j

def readJson(filename):
    # Read 'platform.json' or 'hwsku.json' file
    try:
        with open(filename) as fp:
            if sys.version_info.major == 2:
                return json.load(fp, object_hook=py2JsonStrHook)
            return json.load(fp)
    except Exception as e:
        print("error occurred while parsing json: {}".format(sys.exc_info()[1]))
        return None

def db_connect_configdb(namespace=None):
    """
    Connect to configdb
    """
    config_db = swsscommon.ConfigDBConnector(use_unix_socket_path=True, namespace=namespace)
    if config_db is None:
        return None
    try:
        """
        This could be blocking during the config load_minigraph phase,
        as the CONFIG_DB_INITIALIZED is not yet set in the configDB.
        We can ignore the check by using config_db.db_connect('CONFIG_DB') instead
        """
        # Connect only if available & initialized
        config_db.connect(wait_for_init=False)
    except Exception as e:
        config_db = None
    return config_db

def get_hwsku_file_name(hwsku=None, platform=None):
    hwsku_candidates_Json = []
    hwsku_candidates_Json.append(os.path.join(HWSKU_ROOT_PATH, HWSKU_JSON))
    if hwsku:
        if platform:
            hwsku_candidates_Json.append(os.path.join(PLATFORM_ROOT_PATH, platform, hwsku, HWSKU_JSON))
        hwsku_candidates_Json.append(os.path.join(PLATFORM_ROOT_PATH_DOCKER, hwsku, HWSKU_JSON))
        hwsku_candidates_Json.append(os.path.join(SONIC_ROOT_PATH, hwsku, HWSKU_JSON))
    for candidate in hwsku_candidates_Json:
        if os.path.isfile(candidate):
            return candidate
    return None

def get_fabric_monitor_config(hwsku=None, asic_name=None):
    config_db = db_connect_configdb(asic_name)
    if config_db is not None:
       fabric_monitor_global = config_db.get_table("FABRIC_MONITOR")
       if bool( fabric_monitor_global ):
          return fabric_monitor_global

    fabric_monitor_global = {}
    if asic_name is not None:
        asic_id = str(get_asic_id_from_name(asic_name))
    else:
        asic_id = None
    fabric_monitor_config_file = device_info.get_path_to_fabric_monitor_config_file(hwsku, asic_id)
    if not fabric_monitor_config_file:
       return fabric_monitor_global
    with open( fabric_monitor_config_file, "r" ) as openfile:
       fabric_monitor_global = json.load( openfile )
       openfile.close()

    return fabric_monitor_global


def get_fabric_port_config(hwsku=None, platform=None, fabric_port_config_file=None, hwsku_config_file=None, asic_name=None):
    config_db = db_connect_configdb(asic_name)
    if config_db is not None and fabric_port_config_file is None:
       port_data = config_db.get_table("FABRIC_PORT")
       if bool(port_data):
          ports = ast.literal_eval(json.dumps(port_data))
          return ports

    if asic_name is not None:
        asic_id = str(get_asic_id_from_name(asic_name))
    else:
        asic_id = None

    if not fabric_port_config_file:
        fabric_port_config_file = device_info.get_path_to_fabric_port_config_file(hwsku, asic_id)
        if not fabric_port_config_file:
            return {}
    # else  parse fabric_port_config.ini
    ports = {}

    # Default column definition
    # ../../device/arista/x86_64-arista_7800r3_48cq2_lc/Arista-7800R3-48CQ2-C48/fabric_port_config.ini
    # # name              lanes     isolateStatus
    # Fabric0             0         False

    titles = ['name', 'lanes', 'isolateStatus']
    with open(fabric_port_config_file) as data:
        for line in data:
            if line.startswith('#'):
                if "name" in line:
                    titles = line.strip('#').split()
                continue;
            tokens = line.split()
            if len(tokens) < 2:
                continue
            name_index = titles.index('name')
            name = tokens[name_index]
            data = {}
            for i, item in enumerate(tokens):
                if i == name_index:
                    continue
                data[titles[i]] = item
            data.setdefault('alias', name)
            ports[name] = data
    return ports

def get_port_config(hwsku=None, platform=None, port_config_file=None, hwsku_config_file=None, asic_name=None):
    config_db = db_connect_configdb(asic_name)
    
    config_db_hwsku = device_info.get_localhost_info('hwsku', config_db=config_db)
    # If available, Read from CONFIG DB first
    if config_db is not None and port_config_file is None and (hwsku is None or config_db_hwsku == hwsku):
        port_data = config_db.get_table("PORT")
        if bool(port_data):
            ports = ast.literal_eval(json.dumps(port_data))
            port_alias_map = {}
            port_alias_asic_map = {}
            for intf_name in ports.keys():
                if "alias" in ports[intf_name]:
                    port_alias_map[ports[intf_name]["alias"]] = intf_name
            return (ports, port_alias_map, port_alias_asic_map)

    if asic_name is not None:
        asic_id = str(get_asic_id_from_name(asic_name))
    else:
        asic_id = None

    if not port_config_file:
        port_config_file = device_info.get_path_to_port_config_file(hwsku, asic_id)

        if not port_config_file:
            return ({}, {}, {})


    # Read from 'platform.json' file
    if port_config_file.endswith('.json'):
        if not hwsku_config_file:
             hwsku_json_file = get_hwsku_file_name(hwsku, platform)
             if not hwsku_json_file:
                return ({}, {}, {})
        else:
            hwsku_json_file = hwsku_config_file

        return parse_platform_json_file(hwsku_json_file, port_config_file)

    # If 'platform.json' file is not available, read from 'port_config.ini'
    else:
        return parse_port_config_file(port_config_file)

def parse_port_config_file(port_config_file):
    ports = {}
    port_alias_map = {}
    port_alias_asic_map = {}
    # Default column definition
    titles = ['name', 'lanes', 'alias', 'index']
    with open(port_config_file) as data:
        for line in data:
            if line.startswith('#'):
                if "name" in line:
                    titles = line.strip('#').split()
                continue;
            tokens = line.split()
            if len(tokens) < 2:
                continue
            name_index = titles.index('name')
            name = tokens[name_index]
            data = {}
            for i, item in enumerate(tokens):
                if i == name_index:
                    continue
                data[titles[i]] = item
            data.setdefault('alias', name)
            ports[name] = data
            port_alias_map[data['alias']] = name
            # asic_port_name to sonic_name mapping also included in
            # port_alias_map
            if (('asic_port_name' in data) and
                (data['asic_port_name'] != name)):
                port_alias_map[data['asic_port_name']] = name
            # alias to asic_port_name mapping
            if 'asic_port_name' in data:
                port_alias_asic_map[data['alias']] = data['asic_port_name'].strip()
    return (ports, port_alias_map, port_alias_asic_map)

class BreakoutCfg(object):

    class BreakoutModeEntry:
        def __init__(self, num_ports, default_speed, supported_speed, num_assigned_lanes=None):
            self.num_ports = int(num_ports)
            self.default_speed = self._speed_to_int(default_speed)
            self.supported_speed = set((self.default_speed, ))
            self._parse_supported_speed(supported_speed)
            self.num_assigned_lanes = self._parse_num_assigned_lanes(num_assigned_lanes)

        @classmethod
        def _speed_to_int(cls, speed):
            try:
                if speed.endswith('G'):
                    return int(speed.replace('G', '')) * 1000

                return int(speed)
            except ValueError:
                raise RuntimeError("Unsupported speed format '{}'".format(speed))

        def _parse_supported_speed(self, speed):
            if not speed:
                return

            if not speed.startswith('[') and not speed.endswith(']'):
                raise RuntimeError("Unsupported port breakout format!")

            for s in speed[1:-1].split(','):
                self.supported_speed.add(self._speed_to_int(s.strip()))

        def _parse_num_assigned_lanes(self, num_assigned_lanes):
            if not num_assigned_lanes:
                return

            if isinstance(num_assigned_lanes, int):
                return num_assigned_lanes

            if not num_assigned_lanes.startswith('(') and not num_assigned_lanes.endswith(')'):
                raise RuntimeError("Unsupported port breakout format!")

            return int(num_assigned_lanes[1:-1])

        def __eq__(self, other):
            if isinstance(other, BreakoutCfg.BreakoutModeEntry):
                if self.num_ports != other.num_ports:
                    return False
                if self.supported_speed != other.supported_speed:
                    return False
                if self.num_assigned_lanes != other.num_assigned_lanes:
                    return False
                return True
            else:
                return False

        def __ne__(self, other):
            return not self == other

        def __hash__(self):
            return hash((self.num_ports, tuple(self.supported_speed), self.num_assigned_lanes))

    def __init__(self, name, bmode, properties):
        self._interface_base_id = int(name.replace(PORT_STR, ''))
        self._properties = properties
        self._lanes = properties ['lanes'].split(',')
        self._indexes = properties ['index'].split(',')
        self._breakout_mode_entry = self._str_to_entries(bmode)
        self._breakout_capabilities = None

        # Find specified breakout mode in port breakout mode capabilities
        for supported_mode in self._properties['breakout_modes']:
            if self._breakout_mode_entry == self._str_to_entries(supported_mode):
                self._breakout_capabilities = self._properties['breakout_modes'][supported_mode]
                break

        if not self._breakout_capabilities:
            raise RuntimeError("Unsupported breakout mode {}!".format(bmode))

    def _re_group_to_entry(self, group):
        if len(group) != BRKOUT_PATTERN_GROUPS:
            raise RuntimeError("Unsupported breakout mode format!")

        num_ports, default_speed, supported_speed, _, num_assigned_lanes, _ = group
        if not num_assigned_lanes:
            num_assigned_lanes = len(self._lanes)

        return BreakoutCfg.BreakoutModeEntry(num_ports, default_speed, supported_speed, num_assigned_lanes)

    def _str_to_entries(self, bmode):
        """
        Example of match_list for some breakout_mode using regex
            Breakout Mode -------> Match_list
            -----------------------------
            2x25G(2)+1x50G(2) ---> [('2', '25G', None, '(2)', '2'), ('1', '50G', None, '(2)', '2')]
            1x50G(2)+2x25G(2) ---> [('1', '50G', None, '(2)', '2'), ('2', '25G', None, '(2)', '2')]
            1x100G[40G] ---------> [('1', '100G', '[40G]', None, None)]
            2x50G ---------------> [('2', '50G', None, None, None)]
        """

        try:
            groups_list = [re.match(BRKOUT_PATTERN, i).groups() for i in bmode.split("+")]
        except Exception:
            raise RuntimeError('Breakout mode "{}" validation failed!'.format(bmode))

        return [self._re_group_to_entry(group) for group in groups_list]

    def get_config(self):
        # Ensure that we have corret number of configured lanes
        lanes_used = 0
        total_num_ports = 0
        for entry in self._breakout_mode_entry:
            lanes_used += entry.num_assigned_lanes
            total_num_ports += entry.num_ports

        if lanes_used > len(self._lanes):
            raise RuntimeError("Assigned lines count is more that available!")

        ports = {}

        lane_id = 0
        alias_id = 0

        for entry in self._breakout_mode_entry:
            lanes_per_port = entry.num_assigned_lanes // entry.num_ports

            for port in range(entry.num_ports):
                interface_name = PORT_STR + str(self._interface_base_id + lane_id)

                lanes = self._lanes[lane_id:lane_id + lanes_per_port]

                alias = self._breakout_capabilities[alias_id]
                # If alias follows new SONiC port naming convention (e.g. et[sX]pY[abcd]),
                # we can derive subport directly based on the breakout mode. Otherwise,
                # fallback to the old method.
                if m := re.match(r"et(s\d+)?p\d+([a-l])?", alias):
                    breakout = m.groups()[-1]
                    subport = "0" if not breakout else str(ord(breakout) - ord('a') + 1)
                else:
                    subport = "0" if total_num_ports == 1 else str(alias_id + 1)

                port_config = {
                    'alias': alias,
                    'lanes': ','.join(lanes),
                    'speed': str(entry.default_speed),
                    'index': self._indexes[lane_id],
                    'subport': subport
                }
                
                # If the lane speed is greater than 50G, enable FEC
                if entry.default_speed // lanes_per_port >= 50000:
                    port_config['fec'] = 'rs'

                ports[interface_name] = port_config

                lane_id += lanes_per_port
                alias_id += 1

        return ports


"""
Given a port and breakout mode, this method returns
the list of child ports using platform_json file
"""
def get_child_ports(interface, breakout_mode, platform_json_file):
    port_dict = readJson(platform_json_file)

    mode_handler = BreakoutCfg(interface, breakout_mode, port_dict[INTF_KEY][interface])

    return mode_handler.get_config()

def parse_platform_json_file(hwsku_json_file, platform_json_file):
    ports = {}
    port_alias_map = {}
    port_alias_asic_map = {}

    port_dict = readJson(platform_json_file)
    hwsku_dict = readJson(hwsku_json_file)

    if port_dict is None:
        raise Exception("port_dict is none")
    if hwsku_dict is None:
        raise Exception("hwsku_dict is none")

    if INTF_KEY not in port_dict or INTF_KEY not in  hwsku_dict:
        raise Exception("INTF_KEY is not present in appropriate file")

    for intf in port_dict[INTF_KEY]:
        if intf not in hwsku_dict[INTF_KEY]:
            continue

        # take default_brkout_mode from hwsku.json
        brkout_mode = hwsku_dict[INTF_KEY][intf][BRKOUT_MODE]

        child_ports = get_child_ports(intf, brkout_mode, platform_json_file)

        # take optional fields from hwsku.json
        hwsku_entry = hwsku_dict[INTF_KEY]
        for child_port in child_ports:
            if child_port in hwsku_entry:
                for key, item in hwsku_entry[child_port].items():
                    if key in OPTIONAL_HWSKU_ATTRIBUTES:
                        for child in child_ports:
                            child_ports.get(child)[key] = item

        ports.update(child_ports)

    if ports is None:
        raise Exception("Ports dictionary is None")

    for i in ports.keys():
        port_alias_map[ports[i]["alias"]]= i
    return (ports, port_alias_map, port_alias_asic_map)


def get_breakout_mode(hwsku=None, platform=None, port_config_file=None):
    if not port_config_file:
        port_config_file = device_info.get_path_to_port_config_file(hwsku)
        if not port_config_file:
            return None
    if port_config_file.endswith('.json'):
        hwsku_json_file = get_hwsku_file_name(hwsku, platform)
        if not hwsku_json_file:
            raise Exception("'hwsku_json' file does not exist!!! This file is necessary to proceed forward.")

        return parse_breakout_mode(hwsku_json_file)
    else:
        return None

def parse_breakout_mode(hwsku_json_file):
    brkout_table = {}
    hwsku_dict = readJson(hwsku_json_file)
    if not hwsku_dict:
        raise Exception("hwsku_dict is empty")
    if INTF_KEY not in  hwsku_dict:
        raise Exception("INTF_KEY is not present in hwsku_dict")

    for intf in hwsku_dict[INTF_KEY]:
        brkout_table[intf] = {}
        brkout_table[intf][CUR_BRKOUT_MODE] = hwsku_dict[INTF_KEY][intf][BRKOUT_MODE]
    return brkout_table
