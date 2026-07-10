"""
SONiC interface types and access functions.
"""

"""
 Dictionary of SONIC interface name prefixes. Each entry in the format
 "Human readable interface string":"Sonic interface prefix"
 Currently this is a static mapping, but in future could be stored as metadata in DB.
"""

SONIC_INTERFACE_PREFIXES = {
    "Ethernet-FrontPanel": "Ethernet",
    "PortChannel": "PortChannel",
    "Vlan": "Vlan",
    "Loopback": "Loopback",
    "Ethernet-Backplane": "Ethernet-BP",
    "Ethernet-Inband": "Ethernet-IB",
    "Ethernet-Recirc": "Ethernet-Rec",
    "Ethernet-SubPort": "Eth",
    "PortChannel-SubPort": "Po"
}

VLAN_SUB_INTERFACE_SEPARATOR = '.'

def front_panel_prefix():
    """
    Retrieves the SONIC front panel interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Ethernet-FrontPanel"]

def backplane_prefix():
    """
    Retrieves the SONIC backplane interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Ethernet-Backplane"]

def portchannel_prefix():
    """
    Retrieves the SONIC PortChannel interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["PortChannel"]

def vlan_prefix():
    """
    Retrieves the SONIC Vlan interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Vlan"]

def loopback_prefix():
    """
    Retrieves the SONIC Loopback interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Loopback"]

def inband_prefix():
    """
    Retrieves the SONIC recycle port inband interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Ethernet-Inband"]

def recirc_prefix():
    """
    Retrieves the SONIC recirculation port interface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Ethernet-Recirc"]

def physical_subinterface_prefix():
    """
    Retrieves the SONIC Subinterface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["Ethernet-SubPort"]

def portchannel_subinterface_prefix():
    """
    Retrieves the SONIC Subinterface name prefix.
    """
    return SONIC_INTERFACE_PREFIXES["PortChannel-SubPort"]

def get_interface_table_name(interface_name):
    """Get table name by interface_name prefix
    """
    if interface_name.startswith(front_panel_prefix()):
        if VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
            return "VLAN_SUB_INTERFACE"
        return "INTERFACE"
    elif interface_name.startswith(portchannel_prefix()):
        if VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
            return "VLAN_SUB_INTERFACE"
        return "PORTCHANNEL_INTERFACE"
    elif interface_name.startswith(vlan_prefix()):
        return "VLAN_INTERFACE"
    elif interface_name.startswith(loopback_prefix()):
        return "LOOPBACK_INTERFACE"
    elif VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
        if interface_name.startswith(physical_subinterface_prefix()) or interface_name.startswith(portchannel_subinterface_prefix()):
            return "VLAN_SUB_INTERFACE"
    else:
        return ""

def get_port_table_name(interface_name):
    """Get table name by port_name prefix
    """
    if interface_name.startswith(front_panel_prefix()):
        if VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
            return "VLAN_SUB_INTERFACE"
        return "PORT"
    elif interface_name.startswith(portchannel_prefix()):
        if VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
            return "VLAN_SUB_INTERFACE"
        return "PORTCHANNEL"
    elif interface_name.startswith(vlan_prefix()):
        return "VLAN_INTERFACE"
    elif interface_name.startswith(loopback_prefix()):
        return "LOOPBACK_INTERFACE"
    elif VLAN_SUB_INTERFACE_SEPARATOR in interface_name:
        if interface_name.startswith(physical_subinterface_prefix()) or interface_name.startswith(portchannel_subinterface_prefix()):
            return "VLAN_SUB_INTERFACE"
    else:
        return ""

def get_subintf_longname(intf):
    if intf is None:
       return None
    sub_intf_sep_idx = intf.find(VLAN_SUB_INTERFACE_SEPARATOR)
    if sub_intf_sep_idx == -1:
        return str(intf)
    parent_intf = intf[:sub_intf_sep_idx]
    sub_intf_idx = intf[(sub_intf_sep_idx+1):]
    if intf.startswith("Eth"):
        intf_index=intf[len("Eth"):sub_intf_sep_idx]
        return "Ethernet"+intf_index+VLAN_SUB_INTERFACE_SEPARATOR+sub_intf_idx
    elif intf.startswith("Po"):
        intf_index=intf[len("Po"):sub_intf_sep_idx]
        return "PortChannel"+intf_index+VLAN_SUB_INTERFACE_SEPARATOR+sub_intf_idx
    else:
        return str(intf)

def get_intf_longname(intf):
    if intf is None:
       return None

    if VLAN_SUB_INTERFACE_SEPARATOR in intf:
       return get_subintf_longname(intf)
    else:
        if intf.startswith("Eth"):
           if intf.startswith("Ethernet"):
              return intf
           intf_index=intf[len("Eth"):len(intf)]
           return "Ethernet"+intf_index
        elif intf.startswith("Po"):
             if intf.startswith("PortChannel"):
                return intf
             intf_index=intf[len("Po"):len(intf)]
             return "PortChannel"+intf_index
        else:
             return intf


