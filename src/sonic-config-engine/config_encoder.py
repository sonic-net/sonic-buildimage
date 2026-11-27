import ipaddress
import json


class minigraph_encoder(json.JSONEncoder):
    def default(self, obj):
        if isinstance(obj, (
            ipaddress.IPv4Network, ipaddress.IPv6Network,
            ipaddress.IPv4Address, ipaddress.IPv6Address
            )):
            return str(obj)
        return json.JSONEncoder.default(self, obj)
