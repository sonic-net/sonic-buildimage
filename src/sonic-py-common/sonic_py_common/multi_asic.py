import glob
import os
import subprocess

from natsort import natsorted
from swsscommon import swsscommon

from .device_info import get_asic_conf_file_path
from .device_info import is_supervisor, is_chassis

ASIC_NAME_PREFIX = 'asic'
NAMESPACE_PATH_GLOB = '/run/netns/*'
ASIC_CONF_FILENAME = 'asic.conf'
FRONTEND_ASIC_SUB_ROLE = 'FrontEnd'
BACKEND_ASIC_SUB_ROLE = 'BackEnd'
FABRIC_ASIC_SUB_ROLE = 'Fabric'
EXTERNAL_PORT = 'Ext'
INTERNAL_PORT = 'Int'
INBAND_PORT = 'Inb'
RECIRC_PORT ='Rec'
PORT_CHANNEL_MEMBER_CFG_DB_TABLE = 'PORTCHANNEL_MEMBER'
PORT_CFG_DB_TABLE = 'PORT'
BGP_NEIGH_CFG_DB_TABLE = 'BGP_NEIGHBOR'
BGP_INTERNAL_NEIGH_CFG_DB_TABLE = 'BGP_INTERNAL_NEIGHBOR'
NEIGH_DEVICE_METADATA_CFG_DB_TABLE = 'DEVICE_NEIGHBOR_METADATA'
DEFAULT_NAMESPACE = ''
PORT_ROLE = 'role'
CHASSIS_STATE_DB='CHASSIS_STATE_DB'
CHASSIS_FABRIC_ASIC_INFO_TABLE='CHASSIS_FABRIC_ASIC_TABLE'

# Dictionary to cache config_db connection handle per namespace
# to prevent duplicate connections from being opened
config_db_handle = {}

def connect_config_db_for_ns(namespace=DEFAULT_NAMESPACE):
    """
    The function connects to the config DB for a given namespace and
    returns the handle
    If no namespace is provided, it will connect to the db in the
    default namespace.
    In case of multi ASIC, the default namespace is the database
    instance running the on the host

    Returns:
      handle to the config_db for a namespace
    """
    config_db = swsscommon.ConfigDBConnector(namespace=namespace)
    config_db.connect()
    return config_db


def connect_to_all_dbs_for_ns(namespace=DEFAULT_NAMESPACE):
    """
    The function connects to the DBs for a given namespace and
    returns the handle

    For voq chassis systems, the db list includes databases from
    supervisor card. Avoid connecting to these databases from linecards

    If no namespace is provided, it will connect to the db in the
    default namespace.
    In case of multi ASIC, the default namespace is the
    database instance running the on the host
    In case of single ASIC, the namespace has to be DEFAULT_NAMESPACE

    Returns:
        handle to all the dbs for a namespaces
    """
    db = swsscommon.SonicV2Connector(namespace=namespace)
    db_list = list(db.get_db_list())
    if not is_supervisor():
        try:
            db_list.remove('CHASSIS_APP_DB')
            db_list.remove('CHASSIS_STATE_DB')
        except Exception:
            pass

    for db_id in db_list:
        db.connect(db_id)
    return db

def get_num_asics():
    """
    Retrieves the num of asics present in the multi ASIC platform

    Returns:
        Num of asics
    """
    asic_conf_file_path = get_asic_conf_file_path()

    if asic_conf_file_path is None:
        return 1

    with open(asic_conf_file_path) as asic_conf_file:
        for line in asic_conf_file:
            tokens = line.split('=')
            if len(tokens) < 2:
                continue
            if tokens[0].lower() == 'num_asic':
                num_asics = tokens[1].strip()
        return int(num_asics)


def is_multi_asic():
    """
    Checks if the device is multi asic or not

    Returns:
        True: if the num of asic is more than 1
    """
    num_asics = get_num_asics()

    return (num_asics > 1)


def get_asic_id_from_name(asic_name):
    """
    Get the asic id from the asic name for multi-asic platforms
    In single ASIC platforms, it would fail and throw an exception.

    Returns:
        asic id.
    """
    if asic_name.startswith(ASIC_NAME_PREFIX):
        return asic_name[len(ASIC_NAME_PREFIX):]
    else:
        raise ValueError('Unknown asic namespace name {}'.format(asic_name))

def get_asic_device_id(asic_id):
    # Get asic.conf file
    asic_conf_file_path = get_asic_conf_file_path()

    if asic_conf_file_path is None:
        return None

    # In a multi-asic device we need to have the file "asic.conf" updated with the asic instance
    # and the corresponding device id which could be pci_id. Below is an eg: for a 2 ASIC platform/sku.
    # DEV_ID_ASIC_0=03:00.0
    # DEV_ID_ASIC_1=04:00.0
    device_str = "DEV_ID_ASIC_{}".format(asic_id)

    with open(asic_conf_file_path) as asic_conf_file:
        for line in asic_conf_file:
            tokens = line.split('=')
            if len(tokens) < 2:
               continue
            if tokens[0] == device_str:
                device_id = tokens[1].strip()
                return device_id

    return None

def get_current_namespace(pid=None):
    """
    This API returns the network namespace in which it is
    invoked. In case of global namepace the API returns None
    """

    net_namespace = None
    command = ['/bin/ip', 'netns', 'identify', "{}".format(os.getpid() if not pid else pid)]
    proc = subprocess.Popen(command,
                            stdout=subprocess.PIPE,
                            universal_newlines=True,
                            stderr=subprocess.STDOUT)
    try:
        stdout, stderr = proc.communicate()
        if proc.returncode != 0:
            raise RuntimeError(
                "Command {} failed with stderr {}".format(command, stderr)
            )
        if stdout.rstrip('\n') != "":
            net_namespace = stdout.rstrip('\n')
        else:
            net_namespace = DEFAULT_NAMESPACE
    except OSError as e:
        raise OSError("Error running command {}".format(command))

    return net_namespace


def get_namespaces_from_linux():
    """
    In a multi asic platform, each ASIC is in a Linux Namespace.
    This method returns the asic namespace under which this is invoked,
    if namespace is None (global namespace) it returns list of all
    the Namespace present on the device

    Note: It is preferable to use this function only when config_db is not
    available. When configdb is available use get_all_namespaces()

    Returns:
        List of the namespaces present in the system
    """
    current_ns = get_current_namespace()
    if current_ns:
        return [current_ns]

    ns_list = []
    for path in glob.glob(NAMESPACE_PATH_GLOB):
        ns = os.path.basename(path)
        ns_list.append(ns)
    return natsorted(ns_list)


def get_all_namespaces():
    """
    In case of Multi-Asic platform, Each ASIC will have a linux network namespace created.
    So we loop through the databases in different namespaces and depending on the sub_role
    decide whether this is a front end ASIC/namespace or a back end one.
    """
    front_ns = []
    back_ns = []
    fabric_ns = []
    num_asics = get_num_asics()

    if is_multi_asic():
        for asic in range(num_asics):
            namespace = "{}{}".format(ASIC_NAME_PREFIX, asic)
            if namespace not in config_db_handle:
                config_db_handle[namespace] =  connect_config_db_for_ns(namespace)
            config_db = config_db_handle[namespace]

            metadata = config_db.get_table('DEVICE_METADATA')
            if metadata['localhost']['sub_role'] == FRONTEND_ASIC_SUB_ROLE:
                front_ns.append(namespace)
            elif metadata['localhost']['sub_role'] == BACKEND_ASIC_SUB_ROLE:
                back_ns.append(namespace)
            elif metadata['localhost']['sub_role'] == FABRIC_ASIC_SUB_ROLE:
                fabric_ns.append(namespace)

    return {'front_ns': front_ns, 'back_ns': back_ns, 'fabric_ns': fabric_ns}


def get_namespace_list(namespace=None):
    if not is_multi_asic():
        ns_list = [DEFAULT_NAMESPACE]
        return ns_list

    if namespace is None:
        # there are few commands that needs to work even if the
        # config db is not present. So get the namespaces
        # list from linux
        ns_list = get_namespaces_from_linux()
    else:
        ns_list = [namespace]

    return ns_list

def get_port_entry(port, namespace):
    """
    Retrieves the given port information

    Returns:
        a dict of given port entry
    """
    all_ports = {}
    ns_list = get_namespace_list(namespace)

    for ns in ns_list:
        ports = get_port_entry_for_asic(port, ns)
        if ports:
            return ports

    return all_ports


def get_port_table(namespace=None):
    """
    Retrieves the ports from all the asic present on the devices

    Returns:
        a dict of all the ports
    """
    all_ports = {}
    ns_list = get_namespace_list(namespace)

    for ns in ns_list:
        ports = get_port_table_for_asic(ns)
        all_ports.update(ports)

    return all_ports

def get_port_entry_for_asic(port, namespace):

    config_db = connect_config_db_for_ns(namespace)
    ports = config_db.get_entry(PORT_CFG_DB_TABLE, port)
    return ports


def get_port_table_for_asic(namespace):

    config_db = connect_config_db_for_ns(namespace)
    ports = config_db.get_table(PORT_CFG_DB_TABLE)
    return ports


def get_namespace_for_port(port_name):

    ns_list = get_namespace_list()
    port_namespace = None

    for ns in ns_list:
        ports = get_port_table_for_asic(ns)
        if port_name in ports:
            port_namespace = ns
            break

    if port_namespace is None:
        raise ValueError('Unknown port name {}'.format(port_name))

    return port_namespace


def get_port_role(port_name, namespace=None):

    ports_config = get_port_entry(port_name, namespace)
    if not ports_config:
        raise ValueError('Unknown port name {}'.format(port_name))

    if PORT_ROLE not in ports_config:
        return EXTERNAL_PORT

    role = ports_config[PORT_ROLE]
    return role


def is_port_internal(port_name, namespace=None):

    role = get_port_role(port_name, namespace)

    if role in [INTERNAL_PORT, INBAND_PORT, RECIRC_PORT]:
        return True

    return False


def get_external_ports(port_names, namespace=None):
    external_ports = set()
    ports_config = get_port_table(namespace)
    for port in port_names:
        if port in ports_config:
            if (PORT_ROLE not in ports_config[port] or
                    ports_config[port][PORT_ROLE] == EXTERNAL_PORT):
                external_ports.add(port)
    return external_ports


def is_port_channel_internal(port_channel, namespace=None):

    if not is_multi_asic():
        return False

    ns_list = get_namespace_list(namespace)

    for ns in ns_list:
        config_db = connect_config_db_for_ns(ns)
        port_channel_members = config_db.get_keys(PORT_CHANNEL_MEMBER_CFG_DB_TABLE)

        for port_channel_member in port_channel_members:
            if port_channel_member[0] != port_channel:
                continue
            return is_port_internal(port_channel_member[1], namespace)

    return False

# Allow user to get a set() of back-end interface and back-end LAG per namespace
# default is getting it for all name spaces if no namespace is specified
def get_back_end_interface_set(namespace=None):
    bk_end_intf_list =[]
    if not is_multi_asic():
        return None

    port_table = get_port_table(namespace)
    for port, info in port_table.items():
        if PORT_ROLE in info and info[PORT_ROLE] == INTERNAL_PORT:
            bk_end_intf_list.append(port)

    if len(bk_end_intf_list):
        ns_list = get_namespace_list(namespace)
        for ns in ns_list:
            config_db = connect_config_db_for_ns(ns)
            port_channel_members = config_db.get_keys(PORT_CHANNEL_MEMBER_CFG_DB_TABLE)
            # a back-end LAG must be configured with all of its member from back-end interfaces.
            # mixing back-end and front-end interfaces is miss configuration and not allowed.
            # To determine if a LAG is back-end LAG, just need to check its first member is back-end or not
            # is sufficient. Note that a user defined LAG may have empty members so the list expansion logic
            # need to ensure there are members before inspecting member[0].
            bk_end_intf_list.extend(set([port_channel_member[0] for port_channel_member in port_channel_members\
                                if port_channel_member[1] in bk_end_intf_list]))
    a = set()
    a.update(bk_end_intf_list)
    return a

def is_bgp_session_internal(bgp_neigh_ip, namespace=None):

    if not is_multi_asic() and not is_chassis():
        return False

    ns_list = get_namespace_list(namespace)

    for ns in ns_list:

        config_db = connect_config_db_for_ns(ns)
        bgp_sessions = config_db.get_entry(
            BGP_INTERNAL_NEIGH_CFG_DB_TABLE, bgp_neigh_ip
        )
        if bgp_sessions:
            return True

        bgp_sessions = config_db.get_entry(
            'BGP_VOQ_CHASSIS_NEIGHBOR', bgp_neigh_ip
        )
        if bgp_sessions:
            return True

    return False

def get_front_end_namespaces():
    """
    Get the namespaces in the platform. For multi-asic devices we get the namespaces
    mapped to asic which have front-panel interfaces. For single ASIC device it is the
    DEFAULT_NAMESPACE which maps to the linux host.

    Returns:
        a list of namespaces
    """
    namespaces = [DEFAULT_NAMESPACE]
    if is_multi_asic():
        ns_list = get_all_namespaces()
        namespaces = ns_list['front_ns']

    return namespaces


def get_asic_index_from_namespace(namespace):
    """
    Get asic index from the namespace name.
    With single ASIC platform, return asic_index 0, which is mapped to the only asic present.

    Returns:
        asic_index as an integer.
    """
    if is_multi_asic():
        return int(get_asic_id_from_name(namespace))

    return 0

# Validate whether a given namespace name is valid in the device.
# This API is significant in multi-asic platforms.
def validate_namespace(namespace):
    if not is_multi_asic():
        return True

    namespaces = get_all_namespaces()
    if namespace in namespaces['front_ns'] + namespaces['back_ns']:
        return True
    else:
        return False

def get_asic_presence_list():
    """
    @summary: This function will get the asic presence list. On Supervisor, the list includes only the asics
              for inserted and detected fabric cards. For non-supervisor cards, e.g. line card, the list should
              contain all supported asics by the card. The function gets the asic list from CHASSIS_ASIC_TABLE from
              CHASSIS_STATE_DB. The function assumes that the first N asic ids (asic0 to asic(N-1)) in
              CHASSIS_ASIC_TABLE belongs to the supervisor, where N is the max number of asics supported by the Chassis
    @return:  List of asics present
    """
    asics_list = []
    if is_multi_asic():
        if not is_supervisor():
            # This is not supervisor, all asics should be present. Assuming that asics
            # are not removable entity on Line Cards. Add all asics, 0 - num_asics to the list.
            asics_list = list(range(0, get_num_asics()))
        else:
            # This is supervisor card. Some fabric cards may not be inserted.
            # Get asic list from CHASSIS_ASIC_TABLE which lists only the asics
            # present based on Fabric card detection by the platform.
            db = swsscommon.DBConnector(CHASSIS_STATE_DB, 0, True)
            asic_table = swsscommon.Table(db, CHASSIS_FABRIC_ASIC_INFO_TABLE)
            if asic_table:
                asics_presence_list = list(asic_table.getKeys())
                for asic in asics_presence_list:
                    # asic is asid id: asic0, asic1.... asicN. Get the numeric value.
                    asics_list.append(int(get_asic_id_from_name(asic)))
    return asics_list
