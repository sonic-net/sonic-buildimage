import ipaddress
from lxml import etree as ET
from lxml.etree import QName


UNICODE_TYPE = type(u'')


ns = "Microsoft.Search.Autopilot.Evolution"
ns2 = "Microsoft.Search.Autopilot.NetMux"
ns3 = "http://www.w3.org/2001/XMLSchema-instance"


def parse_device(device):
    lo_prefix = None
    lo_prefix_v6 = None
    mgmt_prefix = None
    mgmt_prefix_v6 = None
    d_type = None   # don't shadow type()
    hwsku = None
    name = None
    deployment_id = None
    cluster = None
    d_subtype = None
    slice_type = None

    for node in device:
        if node.tag == str(QName(ns, "Address")):
            lo_prefix = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "AddressV6")):
            lo_prefix_v6 = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "ManagementAddress")):
            mgmt_prefix = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "ManagementAddressV6")):
            mgmt_prefix_v6 = node.find(str(QName(ns2, "IPPrefix"))).text
        elif node.tag == str(QName(ns, "Hostname")):
            name = node.text
        elif node.tag == str(QName(ns, "HwSku")):
            hwsku = node.text
        elif node.tag == str(QName(ns, "DeploymentId")):
            deployment_id = node.text
        elif node.tag == str(QName(ns, "ElementType")):
            d_type = node.text
        elif node.tag == str(QName(ns, "ClusterName")):
            cluster = node.text
        elif node.tag == str(QName(ns, "SubType")):
            d_subtype = node.text
        elif node.tag == str(QName(ns, "AssociatedSliceStr")) and node.text and "AZNG_Production" in node.text:
            slice_type = "AZNG_Production"

    if d_type is None and str(QName(ns3, "type")) in device.attrib:
        d_type = device.attrib[str(QName(ns3, "type"))]

    return (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, name, hwsku, d_type, deployment_id, cluster, d_subtype, slice_type)


def parse_device_desc_xml(filename):
    root = ET.parse(filename).getroot()
    (lo_prefix, lo_prefix_v6, mgmt_prefix, mgmt_prefix_v6, hostname, hwsku, d_type, _, _, _, _) = parse_device(root)

    results = {}
    results['DEVICE_METADATA'] = {'localhost': {
        'hostname': hostname,
        'hwsku': hwsku,
        }}
    if d_type:
        if d_type in ('Linecard', 'Supervisor'):
            results['DEVICE_METADATA']['localhost']['type'] = 'SpineRouter'
        else:
            results['DEVICE_METADATA']['localhost']['type'] = d_type

    results['LOOPBACK_INTERFACE'] = {'lo': {}, ('lo', lo_prefix): {}}
    if lo_prefix_v6:
        results['LOOPBACK_INTERFACE'] = {'lo_v6': {}, ('lo_v6', lo_prefix_v6): {}}

    results['MGMT_INTERFACE'] = {}
    if mgmt_prefix:
        mgmtipn = ipaddress.ip_network(UNICODE_TYPE(mgmt_prefix), False)
        if mgmtipn != ipaddress.ip_network(u'0.0.0.0/0', False):
            gwaddr = ipaddress.ip_address((next(mgmtipn.hosts())))
            results['MGMT_INTERFACE'].update({('eth0', mgmt_prefix): {'gwaddr': gwaddr}})

    if mgmt_prefix_v6:
        mgmtipn_v6 = ipaddress.ip_network(UNICODE_TYPE(mgmt_prefix_v6), False)
        if mgmtipn != ipaddress.ip_network(u'::/0', False):
            gwaddr_v6 = ipaddress.ip_address((next(mgmtipn_v6.hosts())))
            results['MGMT_INTERFACE'].update({('eth0', mgmt_prefix_v6): {'gwaddr': gwaddr_v6}})

    return results
