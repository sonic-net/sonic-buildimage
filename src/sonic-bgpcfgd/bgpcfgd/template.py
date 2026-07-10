from collections import OrderedDict
from functools import partial

import jinja2
import netaddr
import os

from .log import log_err

class TemplateFabric(object):
    """ Fabric for rendering jinja2 templates """
    def __init__(self, template_path = '/usr/share/sonic/templates'):
        j2_template_paths = [template_path]
        j2_loader = jinja2.FileSystemLoader(j2_template_paths)
        j2_env = jinja2.Environment(loader=j2_loader, trim_blocks=False)
        j2_env.filters['ipv4'] = self.is_ipv4
        j2_env.filters['ipv6'] = self.is_ipv6
        j2_env.filters['pfx_filter'] = self.pfx_filter
        j2_env.filters['file_exists'] = self.file_exists_filter
        for attr in ['ip', 'network', 'prefixlen', 'netmask']:
            j2_env.filters[attr] = partial(self.prefix_attr, attr)
        self.env = j2_env

    def from_file(self, filename):
        """
        Read a template from a file
        :param filename: filename of the file. Type String
        :return: Jinja2 template object
        """
        return self.env.get_template(filename)

    def from_string(self, tmpl):
        """
        Read a template from a string
        :param tmpl: Text representation of Jinja2 template
        :return: Jinja2 template object
        """
        return self.env.from_string(tmpl)

    @staticmethod
    def is_ipv4(value):
        """ Return True if the value is an ipv4 address """
        if not value:
            return False
        if isinstance(value, netaddr.IPNetwork):
            addr = value
        else:
            try:
                addr = netaddr.IPNetwork(str(value))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                return False
        return addr.version == 4

    @staticmethod
    def is_ipv6(value):
        """ Return True if the value is an ipv6 address """
        if not value:
            return False
        if isinstance(value, netaddr.IPNetwork):
            addr = value
        else:
            try:
                addr = netaddr.IPNetwork(str(value))
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                return False
        return addr.version == 6

    @staticmethod
    def prefix_attr(attr, value):
        """
        Extract attribute from IPNetwork object
        :param attr: attribute to extract
        :param value: the string representation of ip prefix which will be converted to IPNetwork.
        :return: the value of the extracted attribute
        """
        if not value:
            return None
        else:
            try:
                prefix = netaddr.IPNetwork(str(value).strip())
            except (netaddr.NotRegisteredError, netaddr.AddrFormatError, netaddr.AddrConversionError):
                return None
        return str(getattr(prefix, attr))

    @staticmethod
    def pfx_filter(value):
        """INTERFACE Table can have keys in one of the two formats:
           string or tuple - This filter skips the string keys and only
           take into account the tuple.
           For eg - VLAN_INTERFACE|Vlan1000 vs VLAN_INTERFACE|Vlan1000|192.168.0.1/21
        """
        table = OrderedDict()

        if not value:
            return table

        for key, val in value.items():
            if not isinstance(key, tuple):
                continue
            intf, ip_address = key
            if '/' not in ip_address:
                if TemplateFabric.is_ipv4(ip_address):
                    table[(intf, "%s/32" % ip_address)] = val
                elif TemplateFabric.is_ipv6(ip_address):
                    table[(intf, "%s/128" % ip_address)] = val
                else:
                    log_err("'%s' is invalid ip address" % ip_address)
            else:
                table[key] = val
        return table

    @staticmethod
    def file_exists_filter(path):
        return os.path.isfile(path)
