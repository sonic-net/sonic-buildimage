"""
Bridge/Port mapping utility library.
"""
from swsscommon import swsscommon
import re


SONIC_ETHERNET_RE_PATTERN = "^Ethernet(\d+)$"
"""
Ethernet-BP refers to BackPlane interfaces
in multi-asic platform.
"""
SONIC_ETHERNET_BP_RE_PATTERN = "^Ethernet-BP(\d+)$"
SONIC_VLAN_RE_PATTERN = "^Vlan(\d+)$"
SONIC_PORTCHANNEL_RE_PATTERN = "^PortChannel(\d+)$"
SONIC_MGMT_PORT_RE_PATTERN = "^eth(\d+)$"
SONIC_ETHERNET_IB_RE_PATTERN = "^Ethernet-IB(\d+)$"
SONIC_ETHERNET_REC_RE_PATTERN = "^Ethernet-Rec(\d+)$"

class BaseIdx:
    ethernet_base_idx = 1
    vlan_interface_base_idx = 2000
    ethernet_bp_base_idx = 9000
    portchannel_base_idx = 1000
    mgmt_port_base_idx = 10000
    ethernet_ib_base_idx = 11000
    ethernet_rec_base_idx = 12000

def get_index(if_name):
    """
    OIDs are 1-based, interfaces are 0-based, return the 1-based index
    Ethernet N = N + 1
    Vlan N = N + 2000
    Ethernet_BP N = N + 9000
    PortChannel N = N + 1000
    eth N = N + 10000
    Ethernet_IB N = N + 11000
    Ethernet_Rec N = N + 12000
    """
    return get_index_from_str(if_name.decode())


def get_index_from_str(if_name):
    """
    OIDs are 1-based, interfaces are 0-based, return the 1-based index
    Ethernet N = N + 1
    Vlan N = N + 2000
    Ethernet_BP N = N + 9000
    PortChannel N = N + 1000
    eth N = N + 10000
    Ethernet_IB N = N + 11000
    Ethernet_Rec N = N + 12000
    """
    patterns = {
        SONIC_ETHERNET_RE_PATTERN: BaseIdx.ethernet_base_idx,
        SONIC_ETHERNET_BP_RE_PATTERN: BaseIdx.ethernet_bp_base_idx,
        SONIC_VLAN_RE_PATTERN: BaseIdx.vlan_interface_base_idx,
        SONIC_PORTCHANNEL_RE_PATTERN: BaseIdx.portchannel_base_idx,
        SONIC_MGMT_PORT_RE_PATTERN: BaseIdx.mgmt_port_base_idx,
        SONIC_ETHERNET_IB_RE_PATTERN: BaseIdx.ethernet_ib_base_idx,
        SONIC_ETHERNET_REC_RE_PATTERN: BaseIdx.ethernet_rec_base_idx
    }

    for pattern, baseidx in patterns.items():
        match = re.match(pattern, if_name)
        if match:
            return int(match.group(1)) + baseidx

def get_interface_oid_map(db, blocking=True):
    """
        Get the Interface names from Counters DB
    """
    db.connect('COUNTERS_DB')
    if_name_map = db.get_all('COUNTERS_DB', 'COUNTERS_PORT_NAME_MAP', blocking=blocking)
    if_lag_name_map = db.get_all('COUNTERS_DB', 'COUNTERS_LAG_NAME_MAP', blocking=blocking)
    if_name_map.update(if_lag_name_map)

    if not if_name_map:
        return {}, {}

    oid_pfx = len("oid:0x")
    if_name_map = {if_name: sai_oid[oid_pfx:] for if_name, sai_oid in if_name_map.items()}

    # TODO: remove the first branch after all SonicV2Connector are migrated to decode_responses
    if isinstance(db, swsscommon.SonicV2Connector) == False and db.dbintf.redis_kwargs.get('decode_responses', False) == False:
        get_index_func = get_index
    else:
        get_index_func = get_index_from_str

    if_id_map = {sai_oid: if_name for if_name, sai_oid in if_name_map.items()
                 # only map the interface if it's a style understood to be a SONiC interface.
                 if get_index_func(if_name) is not None}

    return if_name_map, if_id_map

def get_bridge_port_map(db):
    """
        Get the Bridge port mapping from ASIC DB
    """
    db.connect('ASIC_DB')
    br_port_str = db.keys('ASIC_DB', "ASIC_STATE:SAI_OBJECT_TYPE_BRIDGE_PORT:*")
    if not br_port_str:
        return {}

    if_br_oid_map = {}
    offset = len("ASIC_STATE:SAI_OBJECT_TYPE_BRIDGE_PORT:")
    oid_pfx = len("oid:0x")
    for br_s in br_port_str:
        # Example output: ASIC_STATE:SAI_OBJECT_TYPE_BRIDGE_PORT:oid:0x3a000000000616
        br_port_id = br_s[(offset + oid_pfx):]
        ent = db.get_all('ASIC_DB', br_s, blocking=True)
        # TODO: remove the first branch after all SonicV2Connector are migrated to decode_responses
        if isinstance(db, swsscommon.SonicV2Connector) == False and db.dbintf.redis_kwargs.get('decode_responses', False) == False:
            if b"SAI_BRIDGE_PORT_ATTR_PORT_ID" in ent:
                port_id = ent[b"SAI_BRIDGE_PORT_ATTR_PORT_ID"][oid_pfx:]
                if_br_oid_map[br_port_id] = port_id
        else:
            if "SAI_BRIDGE_PORT_ATTR_PORT_ID" in ent:
                port_id = ent["SAI_BRIDGE_PORT_ATTR_PORT_ID"][oid_pfx:]
                if_br_oid_map[br_port_id] = port_id

    return if_br_oid_map

def get_vlan_id_from_bvid(db, bvid):
    """
        Get the Vlan Id from Bridge Vlan Object
    """
    db.connect('ASIC_DB')
    vlan_obj = db.keys('ASIC_DB', str("ASIC_STATE:SAI_OBJECT_TYPE_VLAN:" + bvid))
    vlan_entry = db.get_all('ASIC_DB', vlan_obj[0], blocking=True)
    vlan_id = None
    # TODO: remove the first branch after all SonicV2Connector are migrated to decode_responses
    if isinstance(db, swsscommon.SonicV2Connector) == False and db.dbintf.redis_kwargs.get('decode_responses', False) == False:
        if b"SAI_VLAN_ATTR_VLAN_ID" in vlan_entry:
            vlan_id = vlan_entry[b"SAI_VLAN_ATTR_VLAN_ID"]
    else:
        if "SAI_VLAN_ATTR_VLAN_ID" in vlan_entry:
            vlan_id = vlan_entry["SAI_VLAN_ATTR_VLAN_ID"]

    return vlan_id

def get_rif_port_map(db):
    """
        Get the RIF port mapping from ASIC DB
    """
    db.connect('ASIC_DB')
    rif_keys_str = db.keys('ASIC_DB', "ASIC_STATE:SAI_OBJECT_TYPE_ROUTER_INTERFACE:*")
    if not rif_keys_str:
        return {}

    rif_port_oid_map = {}
    for rif_s in rif_keys_str:
        rif_id = rif_s[len("ASIC_STATE:SAI_OBJECT_TYPE_ROUTER_INTERFACE:oid:0x"):]
        ent = db.get_all('ASIC_DB', rif_s, blocking=True)
        # TODO: remove the first branch after all SonicV2Connector are migrated to decode_responses
        if isinstance(db, swsscommon.SonicV2Connector) == False and db.dbintf.redis_kwargs.get('decode_responses', False) == False:
            if b"SAI_ROUTER_INTERFACE_ATTR_PORT_ID" in ent:
                port_id = ent[b"SAI_ROUTER_INTERFACE_ATTR_PORT_ID"].lstrip(b"oid:0x")
                rif_port_oid_map[rif_id] = port_id
        else:
            if "SAI_ROUTER_INTERFACE_ATTR_PORT_ID" in ent:
                port_id = ent["SAI_ROUTER_INTERFACE_ATTR_PORT_ID"].lstrip("oid:0x")
                rif_port_oid_map[rif_id] = port_id

    return rif_port_oid_map

def get_vlan_interface_oid_map(db, blocking=True):
    """
        Get Vlan Interface names and sai oids
    """
    db.connect('COUNTERS_DB')

    rif_name_map = db.get_all('COUNTERS_DB', 'COUNTERS_RIF_NAME_MAP', blocking=blocking)
    rif_type_name_map = db.get_all('COUNTERS_DB', 'COUNTERS_RIF_TYPE_MAP', blocking=blocking)

    if not rif_name_map or not rif_type_name_map:
        return {}

    oid_pfx = len("oid:0x")
    vlan_if_name_map = {}

    # TODO: remove the first branch after all SonicV2Connector are migrated to decode_responses
    if isinstance(db, swsscommon.SonicV2Connector) == False and db.dbintf.redis_kwargs.get('decode_responses', False) == False:
        get_index_func = get_index
    else:
        get_index_func = get_index_from_str

    for if_name, sai_oid in rif_name_map.items():
        # Check if RIF is l3 vlan interface
        # TODO: remove the first candidate after all SonicV2Connector are migrated to decode_responses
        if rif_type_name_map[sai_oid] in (b'SAI_ROUTER_INTERFACE_TYPE_VLAN', 'SAI_ROUTER_INTERFACE_TYPE_VLAN'):
            # Check if interface name is in style understood to be a SONiC interface
            if get_index_func(if_name):
                vlan_if_name_map[sai_oid[oid_pfx:]] = if_name

    return vlan_if_name_map
