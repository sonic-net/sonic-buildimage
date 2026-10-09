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
OPTIONAL_HWSKU_ATTRIBUTES = ["fec", "autoneg", "role"]

BRKOUT_PATTERN = r'^(\d{1,6})x(\d{1,6}G?)(.*)$'
BRKOUT_OPTION_PATTERN = r'\[[^\[\]]+\]|\(\d{1,6}\)'

try:
    STRING_TYPES = (basestring,)
except NameError:
    STRING_TYPES = (str,)

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
        hwsku_json_file = hwsku_config_file
        if not hwsku_json_file:
            hwsku_json_file = get_hwsku_file_name(hwsku, platform)

        return parse_platform_json_file(hwsku_json_file, port_config_file)

    # If 'platform.json' file is not available, read from 'port_config.ini'
    else:
        return parse_port_config_file(port_config_file)

def get_system_port_config(hwsku=None, hostname=None, asic_name=None, port_config_file=None):
    config_db = db_connect_configdb(asic_name)
    config_db_hwsku = device_info.get_localhost_info('hwsku', config_db=config_db)
    # Skip CONFIG_DB when caller requested a specific hwsku that doesn't match
    # the running one — otherwise stale SYSTEM_PORT rows from the live SKU
    # leak into the dump for an unrelated SKU.
    if config_db is not None and port_config_file is None and (hwsku is None or config_db_hwsku == hwsku):
        port_data = config_db.get_table("SYSTEM_PORT")
        if bool(port_data):
            port_data_str_keys = {
                f"{k[0]}|{k[1]}|{k[2]}" if isinstance(k, tuple) else k: v
                for k, v in port_data.items()
            }
            sys_ports = ast.literal_eval(json.dumps(port_data_str_keys))
            return sys_ports

    asic_id = None
    if asic_name is not None:
        asic_id = str(get_asic_id_from_name(asic_name))


    if not port_config_file:
        port_config_file = device_info.get_path_to_system_port_config_file(hwsku, asic_id)

        if not port_config_file:
            return {}

    if asic_name is None:
        asic_name = "Asic0"
        asic_id = "0"

    system_ports = parse_system_port_config_file(port_config_file, hostname, asic_name, asic_id)

    return system_ports

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

# Columns a system port config file must provide for VOQ SYSTEM_PORT
# generation. Also the assumed column order when the file has no header.
SYSTEM_PORT_CONFIG_COLUMNS = ['name', 'speed', 'index', 'core_id',
                              'core_port_id', 'num_voq']

def parse_system_port_config_file(port_config_file, hostname, asic_name, asic_id):
    ports = {}
    files = [port_config_file]
    for file in files:
        with open(file) as data:
            titles = list(SYSTEM_PORT_CONFIG_COLUMNS)
            min_tokens = len(titles)
            for line in data:
                if line.startswith('#'):
                    if "name" in line:
                        titles = line.strip('#').split()
                        missing = [c for c in SYSTEM_PORT_CONFIG_COLUMNS if c not in titles]
                        if missing:
                            raise RuntimeError(
                                "'{}' is missing column(s) {} required to generate the "
                                "SYSTEM_PORT config. A plain port_config.ini cannot be "
                                "used as a system port config file.".format(
                                    file, ", ".join(missing)))
                        # Rows may drop trailing optional columns, but must carry
                        # every required one.
                        min_tokens = max(titles.index(c)
                                         for c in SYSTEM_PORT_CONFIG_COLUMNS) + 1
                    continue

                tokens = line.split()
                if len(tokens) < 2:
                    continue
                if len(tokens) < min_tokens:
                    raise RuntimeError(
                        "'{}' row '{}' has {} value(s), expected at least {} to cover "
                        "the columns required for the SYSTEM_PORT config.".format(
                            file, line.strip(), len(tokens), min_tokens))

                name_index = titles.index('name')
                name = tokens[name_index]
                # Build a filtered and renamed dictionary
                field_map = {
                    'speed': 'speed',
                    'core_id': 'core_index',
                    'core_port_id': 'core_port_index',
                    'num_voq': 'num_voq',
                }

                system_port_data = {}
                system_port_data['switch_id'] = asic_id
                for original_key, new_key in field_map.items():
                    system_port_data[new_key] = tokens[titles.index(original_key)]
                index = tokens[titles.index('index')]
                system_port_data['system_port_id'] = index
                system_port_name = hostname + "|" + asic_name + "|" + name
                ports[system_port_name] = system_port_data

    # add system port for CPU. hardcode values for now. But will need
    # read these from config files for cpu port also
    ports[hostname + "|" + asic_name + "|Cpu0"] = {
            "core_index": "0",
            "core_port_index": "0",
            "num_voq": "8",
            "speed": "10000",
            "switch_id": asic_id,
            "system_port_id": "0"
    }

    return ports

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

    def _str_to_entries(self, bmode):
        """
        Parse each segment of a breakout mode. Supported-speed groups and the
        assigned-lane group may appear in either order.
        """
        entries = []
        for segment in bmode.split("+"):
            match = re.match(BRKOUT_PATTERN, segment)
            if not match:
                raise RuntimeError('Breakout mode "{}" validation failed!'.format(bmode))

            num_ports, default_speed, options_text = match.groups()
            options = re.findall(BRKOUT_OPTION_PATTERN, options_text)
            if re.sub(r'\s+', '', ''.join(options)) != re.sub(r'\s+', '', options_text):
                raise RuntimeError('Breakout mode "{}" validation failed!'.format(bmode))

            supported_speeds = []
            num_assigned_lanes = None
            for option in options:
                if option.startswith('['):
                    supported_speeds.extend(speed.strip() for speed in option[1:-1].split(','))
                elif num_assigned_lanes is None:
                    num_assigned_lanes = option
                else:
                    raise RuntimeError('Breakout mode "{}" has multiple lane counts!'.format(bmode))

            supported_speed = None
            if supported_speeds:
                supported_speed = '[{}]'.format(','.join(supported_speeds))
            if num_assigned_lanes is None:
                num_assigned_lanes = len(self._lanes)

            entries.append(BreakoutCfg.BreakoutModeEntry(
                num_ports, default_speed, supported_speed, num_assigned_lanes
            ))

        return entries

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

                port_config = {
                    'alias': self._breakout_capabilities[alias_id],
                    'lanes': ','.join(lanes),
                    'speed': str(entry.default_speed),
                    'index': self._indexes[lane_id],
                    'subport': "0" if total_num_ports == 1 else str(alias_id + 1)
                }
                
                # If the lane speed is greater than 50G, enable FEC
                if entry.default_speed // lanes_per_port >= 50000 and \
                        (device_info.get_sonic_version_info() or {}).get('asic_type') != 'mellanox':
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


def get_minimum_breakout_mode(interface, properties):
    candidates = []
    for breakout_mode, aliases in properties.get('breakout_modes', {}).items():
        if len(aliases) != 1:
            continue

        match = re.match(r'^1x(\d{1,6})(G?)', breakout_mode)
        if not match:
            continue

        speed = int(match.group(1))
        if match.group(2) == 'G':
            speed *= 1000
        candidates.append((speed, breakout_mode))

    if not candidates:
        raise RuntimeError(
            "No single-port breakout mode is defined for interface '{}'".format(interface)
        )

    return max(candidates, key=lambda candidate: (candidate[0], candidate[1]))[1]


def get_valid_breakout_modes(properties):
    return ", ".join(properties.get('breakout_modes', {}).keys())


def parse_platform_json_file(hwsku_json_file, platform_json_file):
    ports = {}
    port_alias_map = {}
    port_alias_asic_map = {}

    port_dict = readJson(platform_json_file)
    hwsku_dict = readJson(hwsku_json_file) if hwsku_json_file else None

    if port_dict is None:
        raise Exception("port_dict is none")
    if hwsku_json_file and hwsku_dict is None:
        raise Exception("hwsku_dict is none")

    if INTF_KEY not in port_dict:
        raise Exception("INTF_KEY is not present in platform file")
    if hwsku_dict is not None and INTF_KEY not in hwsku_dict:
        raise Exception("INTF_KEY is not present in hwsku file")

    for intf in port_dict[INTF_KEY]:
        properties = port_dict[INTF_KEY][intf]
        if hwsku_dict is not None:
            if intf not in hwsku_dict[INTF_KEY]:
                continue

            hwsku_entry = hwsku_dict[INTF_KEY][intf]
            if not isinstance(hwsku_entry, dict) or BRKOUT_MODE not in hwsku_entry:
                raise RuntimeError(
                    "Invalid breakout mode for interface '{}': missing '{}'. Valid modes: {}".format(
                        intf, BRKOUT_MODE, get_valid_breakout_modes(properties)
                    )
                )

            brkout_mode = hwsku_entry[BRKOUT_MODE]
            if not isinstance(brkout_mode, STRING_TYPES) or not brkout_mode:
                raise RuntimeError(
                    "Invalid breakout mode '{}' for interface '{}': expected a non-empty string. "
                    "Valid modes: {}".format(
                        brkout_mode, intf, get_valid_breakout_modes(properties)
                    )
                )
        else:
            brkout_mode = get_minimum_breakout_mode(intf, properties)

        try:
            child_ports = get_child_ports(intf, brkout_mode, platform_json_file)
        except RuntimeError as e:
            raise RuntimeError(
                "Invalid breakout mode '{}' for interface '{}': {} Valid modes: {}".format(
                    brkout_mode, intf, e, get_valid_breakout_modes(properties)
                )
            )

        # take optional fields from hwsku.json
        if hwsku_dict is not None:
            hwsku_interfaces = hwsku_dict[INTF_KEY]
            for child_port in child_ports:
                if child_port in hwsku_interfaces:
                    for key, item in hwsku_interfaces[child_port].items():
                        if key in OPTIONAL_HWSKU_ATTRIBUTES:
                            for child in child_ports:
                                child_ports.get(child)[key] = item

        ports.update(child_ports)

    if ports is None:
        raise Exception("Ports dictionary is None")

    for i in ports.keys():
        port_alias_map[ports[i]["alias"]]= i
    return (ports, port_alias_map, port_alias_asic_map)


def get_breakout_mode(hwsku=None, platform=None, port_config_file=None, hwsku_config_file=None):
    if not port_config_file:
        port_config_file = device_info.get_path_to_port_config_file(hwsku)
        if not port_config_file:
            return None
    if port_config_file.endswith('.json'):
        hwsku_json_file = hwsku_config_file or get_hwsku_file_name(hwsku, platform)
        if not hwsku_json_file:
            return None

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
