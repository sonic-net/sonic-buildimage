import re
import socket
import ipaddress
import uuid
import base64
import sys
import json
import binascii
import subprocess
import ast

from ipaddress import ip_address as IP

import dash_api.eni_pb2 as eni_pb2
import dash_api.route_type_pb2 as route_type_pb2
import dash_api.types_pb2 as types_pb2
import dash_api.ha_scope_config_pb2 as ha_scope_config_pb2
import dash_api.ha_set_config_pb2 as ha_set_config_pb2

from dash_api.eni_pb2 import State
from dash_api.route_type_pb2 import RoutingType, ActionType, RouteType, RouteTypeItem, EncapType
from dash_api.types_pb2 import IpVersion, Range, ValueOrRange, IpPrefix, IpAddress, HaScope, HaOwner
from dash_api.ha_scope_config_pb2 import DesiredHaState

from google.protobuf.descriptor import FieldDescriptor
from google.protobuf.json_format import ParseDict, MessageToDict
from google.protobuf.internal.enum_type_wrapper import EnumTypeWrapper

ENABLE_PROTO = True

PB_INT_TYPES = set([
    FieldDescriptor.TYPE_INT32,
    FieldDescriptor.TYPE_INT64,
    FieldDescriptor.TYPE_UINT32,
    FieldDescriptor.TYPE_UINT64,
    FieldDescriptor.TYPE_FIXED64,
    FieldDescriptor.TYPE_FIXED32,
    FieldDescriptor.TYPE_SFIXED32,
    FieldDescriptor.TYPE_SFIXED64,
    FieldDescriptor.TYPE_SINT32,
    FieldDescriptor.TYPE_SINT64
])

PROTO_MODULES = [
    eni_pb2,
    route_type_pb2,
    types_pb2,
    ha_scope_config_pb2,
    ha_set_config_pb2
]

ENUM_TYPE_MAP = {
    "ActionType": route_type_pb2.ActionType,
    "EncapType": route_type_pb2.EncapType,
    "RoutingType": route_type_pb2.RoutingType,
    "State": eni_pb2.State,
    "IpVersion": types_pb2.IpVersion,
    "HaScope": types_pb2.HaScope,
    "HaOwner": types_pb2.HaOwner,
    "DesiredHaState": ha_scope_config_pb2.DesiredHaState,
}

def _get_enum_wrapper(enum_type_str):
    enum_wrapper = ENUM_TYPE_MAP.get(enum_type_str)
    if enum_wrapper is not None:
        return enum_wrapper

    for module in PROTO_MODULES:
        if module and hasattr(module, enum_type_str):
            candidate = getattr(module, enum_type_str)
            if isinstance(candidate, EnumTypeWrapper):
                return candidate
    return None

def _get_message_class_by_name(message_name):
    for module in PROTO_MODULES:
        if module and hasattr(module, message_name):
            return getattr(module, message_name)
    raise Exception(f"Cannot find message type {message_name}")

def get_enum_type_from_str(enum_type_str, enum_name_str):

    # 4_to_6 uses small cap so cannot use dynamic naming
    if enum_name_str == "4_to_6":
        return ActionType.ACTION_TYPE_4_to_6

    my_enum_type_parts = re.findall(r'[A-Z][^A-Z]*', enum_type_str)
    my_enum_type_concatenated = '_'.join(my_enum_type_parts)
    enum_name = f"{my_enum_type_concatenated.upper()}_{enum_name_str.upper()}"
    enum_wrapper = _get_enum_wrapper(enum_type_str)
    if enum_wrapper is not None:
        return enum_wrapper.Value(enum_name)
    raise Exception(f"Cannot find enum type {enum_type_str}")

def routing_type_from_json(json_obj):
    pb = RouteType()
    if isinstance(json_obj, list):
        for item in json_obj:
            pbi = RouteTypeItem()
            pbi.action_name = item["action_name"]
            pbi.action_type = get_enum_type_from_str('ActionType', item.get("action_type"))
            if item.get("encap_type") is not None:
                pbi.encap_type = get_enum_type_from_str('EncapType', item.get("encap_type"))
            if item.get("vni") is not None:
                pbi.vni = int(item["vni"])
            pb.items.append(pbi)
    else:
        pbi = RouteTypeItem()
        pbi.action_name = json_obj["action_name"]
        pbi.action_type = get_enum_type_from_str('ActionType', json_obj.get("action_type"))
        if json_obj.get("encap_type") is not None:
            pbi.encap_type = get_enum_type_from_str('EncapType', json_obj.get("encap_type"))
        if json_obj.get("vni") is not None:
            pbi.vni = int(json_obj["vni"])
        pb.items.append(pbi)
    return pb

def get_message_from_table_name(table_name):
    table_name_lis = table_name.lower().split("_")
    table_name_lis2 = [item.capitalize() for item in table_name_lis]
    message_name = ''.join(table_name_lis2)
    message_class = _get_message_class_by_name(message_name)
    return message_class()

def parse_ip_address(ip_str):
    ip_addr = IP(ip_str)
    if ip_addr.version == 4:
        encoded_val = int(ip_addr)
    else:
        encoded_val = base64.b64encode(ip_addr.packed)

    return {f"ipv{ip_addr.version}": encoded_val}

def prefix_to_ipv4(prefix_length):
    mask = 2**32 - 2**(32-int(prefix_length))
    s = str(hex(mask))
    s = s[2:]
    hex_groups = [s[i:i+2] for i in range(0, len(s), 2)]
    ipv4_address_str = '.'.join(hex_groups)
    return ipv4_address_str

def prefix_to_ipv6(prefix_length):
    mask = 2**128 - 2**(128-int(prefix_length))
    s = str(hex(mask))
    s = s[2:]
    hex_groups = [s[i:i+4] for i in range(0, len(s), 4)]
    ipv6_address_str = ':'.join(hex_groups)
    return ipv6_address_str


def parse_ip_prefix(ip_prefix_str):
    ip_addr_str, mask = ip_prefix_str.split("/")
    if mask.isdigit():
        ip_addr = IP(ip_addr_str)
        if ip_addr.version == 4:
            mask_str = prefix_to_ipv4(mask)
        else:
            mask_str = prefix_to_ipv6(mask)
    else:
        mask_str = mask
    return {"ip": parse_ip_address(ip_addr_str), "mask": parse_ip_address(mask_str)}


def parse_byte_field(orig_val):
    return base64.b64encode(bytes.fromhex(orig_val.replace(":", "")))


def parse_guid(guid_str):
    return {"value": parse_byte_field(uuid.UUID(guid_str).hex)}


def json_to_proto(key: str, proto_dict: dict):
    """
    Custom parser for DASH configs to allow writing configs
    in a more human-readable format
    """
    table_name = re.search(r"DASH_(\w+)_TABLE", key).group(1)
    if table_name == "ROUTING_TYPE":
        pb = routing_type_from_json(proto_dict)
        return pb.SerializeToString()

    message = get_message_from_table_name(table_name)
    field_map = message.DESCRIPTOR.fields_by_name
    new_dict = {}
    for key, value in proto_dict.items():
        if field_map[key].type == field_map[key].TYPE_MESSAGE:
            if isinstance(value, list):
                # Handle repeated message fields
                new_dict[key] = []
                for item in value:
                    if field_map[key].message_type.name == "Guid":
                        new_dict[key].append(parse_guid(item))
                    elif field_map[key].message_type.name == "IpAddress":
                        new_dict[key].append(parse_ip_address(item))
                    elif field_map[key].message_type.name == "IpPrefix":
                        new_dict[key].append(parse_ip_prefix(item))
                    else:
                        new_dict[key].append(item)
            else:
                if field_map[key].message_type.name == "IpAddress":
                    new_dict[key] = parse_ip_address(value)
                elif field_map[key].message_type.name == "IpPrefix":
                    new_dict[key] = parse_ip_prefix(value)
                elif field_map[key].message_type.name == "Guid":
                    new_dict[key] = parse_guid(value)
                else:
                    new_dict[key] = value
        elif field_map[key].type == field_map[key].TYPE_ENUM:
            new_dict[key] = get_enum_type_from_str(field_map[key].enum_type.name, value)
        elif field_map[key].type == field_map[key].TYPE_BOOL:
            new_dict[key] = value

        elif field_map[key].type == field_map[key].TYPE_BYTES:
            new_dict[key] = parse_byte_field(value)

        elif field_map[key].type in PB_INT_TYPES:
            new_dict[key] = int(value)

        if key not in new_dict:
            new_dict[key] = value

    pb =  ParseDict(new_dict, message)
    return pb.SerializeToString()


def tbl_name_to_type(tbl_name):
    dash_name = re.search(r"DASH_(\w+)_TABLE", tbl_name).group(1)
    # Split the string by underscores
    words = dash_name.split('_')
    # Capitalize the first character of each word
    words = [word.capitalize() for word in words]
    return ''.join(words)

def from_pb(tbl_name, byte_array):
    type_name = tbl_name_to_type(tbl_name)
    obj_type = _get_message_class_by_name(type_name)
    obj = obj_type()
    obj.ParseFromString(byte_array)
    return obj

def decode_pb_hex(tbl_name, hex_string):
    """
    Decode a hex-encoded protobuf payload into a python dict with proto field names.
    """
    byte_array = binascii.unhexlify(hex_string)
    obj = from_pb(tbl_name, byte_array)
    decoded = MessageToDict(obj, preserving_proto_field_name=True)
    return _normalize_ip_fields(decoded)

def _normalize_ip_fields(val):
    """
    Recursively convert protobuf IpAddress dicts into human-readable strings.
    """
    if isinstance(val, dict):
        if "ipv4" in val:
            ipv4_raw = val["ipv4"]
            try:
                val["ipv4"] = str(ipaddress.IPv4Address(int(ipv4_raw)))
            except Exception:
                pass
        if "ipv6" in val:
            ipv6_raw = val["ipv6"]
            try:
                ipv6_bytes = base64.b64decode(ipv6_raw)
                val["ipv6"] = str(ipaddress.IPv6Address(ipv6_bytes))
            except Exception:
                pass
        for k, v in val.items():
            val[k] = _normalize_ip_fields(v)
        return val
    if isinstance(val, list):
        return [_normalize_ip_fields(item) for item in val]
    return val

def parse_hgetall_output(raw_output):
    """
    Accept both dict-like one-line output (e.g. "{'field': 'val'}")
    and alternating-line output from sonic-db-cli hgetall.
    """
    lines = [line for line in raw_output.splitlines() if line.strip()]
    if not lines:
        return {}

    if len(lines) == 1 and lines[0].lstrip().startswith("{") and lines[0].rstrip().endswith("}"):
        try:
            obj = ast.literal_eval(lines[0])
            if isinstance(obj, dict):
                return {str(k): str(v) for k, v in obj.items()}
        except Exception:
            pass

    if len(lines) % 2 == 0:
        kv = {}
        for i in range(0, len(lines), 2):
            field = lines[i].strip()
            value = lines[i+1].strip()
            kv[field] = value
        return kv

    raise ValueError("Unexpected hgetall output format")

def parse_value(value_str):
    """
    Parse the value string. If it looks like a JSON array, parse it as a list.
    Otherwise, treat it as a single string or number.
    """
    value_str = value_str.strip()

    # Handle quoted strings explicitly
    if (value_str.startswith('"') and value_str.endswith('"')) or (value_str.startswith("'") and value_str.endswith("'")):
        return value_str[1:-1]

    # Try to parse as JSON to detect arrays or numbers
    try:
        parsed = json.loads(value_str)
        if isinstance(parsed, (list, int, float)):
            return parsed
        return value_str
    except json.JSONDecodeError:
        return value_str

def main():
    if len(sys.argv) < 3:
        print("Usage: python script.py hset key name1 value1 [name2 value2 ...] | hgetall key")
        sys.exit(1)

    action = sys.argv[1]
    if action not in ["hset", "hgetall"]:
        print("Error: action must be 'hset' or 'hgetall'")
        sys.exit(1)

    if action == "hset" and len(sys.argv) < 5:
        print("Usage: python script.py hset key name1 value1 [name2 value2 ...]")
        sys.exit(1)
    if action == "hgetall" and len(sys.argv) != 3:
        print("Usage: python script.py hgetall key")
        sys.exit(1)

    key = sys.argv[2]
    table_name = key.split(':')[0]

    if action == "hgetall":
        res = subprocess.run(["/usr/bin/sonic-db-cli", "APPL_DB", "hgetall", key], capture_output=True, text=True)
        if res.returncode != 0:
            print(f"Error executing hgetall: {res.stderr.strip()}")
            sys.exit(res.returncode)

        try:
            kv = parse_hgetall_output(res.stdout)
        except Exception as e:
            print(f"Unexpected hgetall output format. Raw output:\n{res.stdout}")
            print(f"Error: {e}")
            sys.exit(1)

        print(f"raw: {kv}")

        if "pb" in kv:
            try:
                decoded = decode_pb_hex(table_name, kv["pb"])
                print("decoded:")
                print(json.dumps(decoded, indent=2))
            except Exception as e:
                print(f"Failed to decode protobuf: {e}")
        else:
            print("No 'pb' field found to decode.")
        return

    args = sys.argv[3:]
    if len(args) % 2 != 0:
        print("Error: name and value arguments must be in pairs")
        sys.exit(1)

    proto_dict = {}
    for i in range(0, len(args), 2):
        name = args[i]
        value_str = args[i+1]
        value = parse_value(value_str)
        proto_dict[name] = value
    print(f"input: {proto_dict}")
    proto_encoded = json_to_proto(table_name, proto_dict)
    hex_string = binascii.hexlify(proto_encoded).decode('utf-8')
    print(f"proto encoded: {hex_string}")

    subprocess.run(["/usr/bin/sonic-db-cli", "APPL_DB", "hset", key, "pb", hex_string])
if __name__ == "__main__":
    main()
