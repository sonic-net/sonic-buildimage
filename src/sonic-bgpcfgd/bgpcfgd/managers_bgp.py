import json
from swsscommon import swsscommon

import jinja2
import netaddr
import os

from .log import log_warn, log_err, log_info, log_debug, log_crit
from .manager import Manager
from .template import TemplateFabric
from .utils import run_command
from .managers_device_global import DeviceGlobalCfgMgr


class BGPPeerGroupMgr(object):
    """ This class represents peer-group and routing policy for the peer_type """
    def __init__(self, common_objs, base_template):
        """
        Construct the object
        :param common_objs: common objects
        :param base_template: path to the directory with Jinja2 templates
        """
        self.cfg_mgr = common_objs['cfg_mgr']
        self.constants = common_objs['constants']
        tf = common_objs['tf']
        self.policy_template = tf.from_file(base_template + "policies.conf.j2")
        self.peergroup_template = tf.from_file(base_template + "peer-group.conf.j2")
        self.device_global_cfgmgr = DeviceGlobalCfgMgr(common_objs, "CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME)

    def update(self, name, **kwargs):
        """
        Update peer-group and routing policy for the peer with the name
        :param name: name of the peer. Used for logging only
        :param kwargs: dictionary with parameters for rendering
        """
        rc_policy = self.update_policy(name, **kwargs)
        rc_pg = self.update_pg(name, **kwargs)
        return rc_policy and rc_pg

    def update_policy(self, name, **kwargs):
        """
        Update routing policy for the peer
        :param name: name of the peer. Used for logging only
        :param kwargs: dictionary with parameters for rendering
        """
        try:
            policy = self.policy_template.render(**kwargs)
        except jinja2.TemplateError as e:
            log_err("Can't render policy template name: '%s': %s" % (name, str(e)))
            return False
        self.update_entity(policy, "Routing policy for peer '%s'" % name)
        return True

    def update_pg(self, name, **kwargs):
        """
        Update peer-group for the peer
        :param name: name of the peer. Used for logging only
        :param kwargs: dictionary with parameters for rendering
        """
        try:
            pg = self.peergroup_template.render(**kwargs)
            tsa_rm = self.device_global_cfgmgr.check_state_and_get_tsa_routemaps(pg)
            idf_isolation_rm = self.device_global_cfgmgr.check_state_and_get_idf_isolation_routemaps()
        except jinja2.TemplateError as e:
            log_err("Can't render peer-group template: '%s': %s" % (name, str(e)))
            return False

        if kwargs['vrf'] == 'default':
            cmd = ('router bgp %s\n' % kwargs['bgp_asn']) + pg + tsa_rm + idf_isolation_rm + "\nexit"
        else:
            cmd = ('router bgp %s vrf %s\n' % (kwargs['bgp_asn'], kwargs['vrf'])) + pg + tsa_rm + idf_isolation_rm + "\nexit"
        self.update_entity(cmd, "Peer-group for peer '%s'" % name)
        return True

    def update_entity(self, cmd, txt):
        """
        Send commands to FRR
        :param cmd: commands to send in a raw form
        :param txt: text for the syslog output
        :return:
        """
        self.cfg_mgr.push(cmd)
        log_info("%s has been scheduled to be updated" % txt)
        return True


class BGPPeerMgrBase(Manager):
    """ Manager of BGP peers """
    def __init__(self, common_objs, db_name, table_name, peer_type, check_neig_meta):
        """
        Initialize the object
        :param common_objs: common objects
        :param table_name: name of the table with peers
        :param peer_type: type of the peers. It is used to find right templates
        """
        self.common_objs = common_objs
        self.constants = self.common_objs["constants"]
        self.fabric = common_objs['tf']
        self.peer_type = peer_type

        base_template = "bgpd/templates/" + self.constants["bgp"]["peers"][peer_type]["template_dir"] + "/"
        self.templates = {
            "add":         self.fabric.from_file(base_template + "instance.conf.j2"),
            "delete":      self.fabric.from_string('no neighbor {{ neighbor_addr }}'),
            "shutdown":    self.fabric.from_string('neighbor {{ neighbor_addr }} shutdown'),
            "no shutdown": self.fabric.from_string('no neighbor {{ neighbor_addr }} shutdown'),
            "no listen range": self.fabric.from_string('no bgp listen range {{ ip_range }} peer-group {{peer_group}}'),
        }

        if (os.path.exists(self.fabric.env.loader.searchpath[0] + "/" + base_template + "update.conf.j2")):
            self.templates["update"] = self.fabric.from_file(base_template + "update.conf.j2")
        if os.path.exists(self.fabric.env.loader.searchpath[0] + "/" + base_template + "delete.conf.j2"):
            self.templates["delete"] = self.fabric.from_file(base_template + "delete.conf.j2")
            log_info("Using delete template found at %s" % (base_template + "delete.conf.j2"))

        deps = [
            ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/bgp_asn"),
            ("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/type"),
            ("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback0"),
            ("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "tsa_enabled"),
            ("CONFIG_DB", swsscommon.CFG_BGP_DEVICE_GLOBAL_TABLE_NAME, "idf_isolation_state"),
            ("LOCAL", "local_addresses", ""),
            ("LOCAL", "interfaces", ""),
        ]

        if check_neig_meta:
            self.check_neig_meta = 'bgp' in self.constants \
                               and 'use_neighbors_meta' in self.constants['bgp'] \
                               and self.constants['bgp']['use_neighbors_meta']
        else:
            self.check_neig_meta = False

        self.check_deployment_id = 'bgp' in self.constants \
                               and 'use_deployment_id' in self.constants['bgp'] \
                               and self.constants['bgp']['use_deployment_id']

        if self.check_neig_meta:
            deps.append(("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME, ""))

        if self.check_deployment_id:
            deps.append(("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME, "localhost/deployment_id"))

        if self.peer_type == 'internal':
            deps.append(("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME, "Loopback4096"))

        super(BGPPeerMgrBase, self).__init__(
            common_objs,
            deps,
            db_name,
            table_name,
        )

        self.peers = self.load_peers()
        self.peer_group_mgr = BGPPeerGroupMgr(self.common_objs, base_template)
        return

    def set_handler(self, key, data):
        """
         It runs on 'SET' command
        :param key: key of the changed table
        :param data: the data associated with the change
        """
        vrf, nbr = self.split_key(key)
        peer_key = (vrf, nbr)
        if peer_key not in self.peers:
            return self.add_peer(vrf, nbr, data)
        else:
            return self.update_peer(vrf, nbr, data)

    def add_peer(self, vrf, nbr, data):
        """
        Add a peer into FRR. This is used if the peer is not existed in FRR yet
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param data: associated data
        :return: True if this adding was successful, False otherwise
        """
        print_data = vrf, nbr, data
        bgp_asn = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        #
        lo0_ipv4 = self.get_lo_ipv4("Loopback0|")
        if (lo0_ipv4 is None and "bgp_router_id"
            not in self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]):
            log_warn("Loopback0 ipv4 address is not presented yet and bgp_router_id not configured")
            return False

        #
        if self.peer_type == 'internal':
            lo4096_ipv4 = self.get_lo_ipv4("Loopback4096|")
            if (lo4096_ipv4 is None and "bgp_router_id"
                not in self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]):
                log_warn("Loopback4096 ipv4 address is not presented yet and bgp_router_id not configured")
                return False

        if "local_addr" not in data:
            log_warn("Peer %s. Missing attribute 'local_addr'" % nbr)
        else:
            data["local_addr"] = str(netaddr.IPNetwork(str(data["local_addr"])).ip)
            interface = self.get_local_interface(data["local_addr"])
            if not interface:
                print_data = nbr, data["local_addr"]
                log_debug("Peer '%s' with local address '%s' wait for the corresponding interface to be set" % print_data)
                return False

        kwargs = {
            'CONFIG_DB__DEVICE_METADATA': self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME),
            'CONFIG_DB__BGP_BBR': self.directory.get_slot('CONFIG_DB', 'BGP_BBR'),
            'constants': self.constants,
            'bgp_asn': bgp_asn,
            'vrf': vrf,
            'neighbor_addr': nbr,
            'bgp_session': data,
            'CONFIG_DB__LOOPBACK_INTERFACE':{ tuple(key.split('|')) : {} for key in self.directory.get_slot("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME)
                                                                         if '|' in key }
        }
        if lo0_ipv4 is not None:
            kwargs['loopback0_ipv4'] = lo0_ipv4
        if self.check_neig_meta:
            neigmeta = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_NEIGHBOR_METADATA_TABLE_NAME)
            if 'name' in data and data["name"] not in neigmeta:
                log_info("DEVICE_NEIGHBOR_METADATA is not ready for neighbor '%s' - '%s'" % (nbr, data['name']))
                return False
            kwargs['CONFIG_DB__DEVICE_NEIGHBOR_METADATA'] = neigmeta

        tag = data['name'] if 'name' in data else nbr
        self.peer_group_mgr.update(tag, **kwargs)

        try:
            cmd = self.templates["add"].render(**kwargs)
        except jinja2.TemplateError as e:
            msg = "Peer '(%s|%s)'. Error in rendering the template for 'SET' command '%s'" % print_data
            log_err("%s: %s" % (msg, str(e)))
            return True
        if cmd is not None:
            self.apply_op(cmd, vrf)
            key = (vrf, nbr)
            self.peers.add(key)
            self.update_state_db(vrf, nbr, data, "SET")
            log_info("Peer '(%s|%s)' has been scheduled to be added with attributes '%s'" % print_data)

        self.directory.put(self.db_name, self.table_name, vrf + '|' + nbr, data)
        return True

    def update_state_db(self, vrf, nbr, data, op):
        """
        Update the database with the new data
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param data: associated data (will be empty for deletion case)
        :param op: operation type. It can be "SET" or "DEL"
        :return: True if this adding was successful, False otherwise
        """
        if (vrf == "default"):
            key = nbr
        else:
            key = vrf + "|" + nbr
        # Update the peer in the STATE_DB table
        try:
            state_db = swsscommon.DBConnector("STATE_DB", 0)
            state_peer_table = swsscommon.Table(state_db, swsscommon.STATE_BGP_PEER_CONFIGURED_TABLE_NAME)
            if (op == "SET"):
                state_peer_table.set(key, list(sorted(data.items())))
                log_info("Peer '(%s)' has been added to BGP_PEER_CONFIGURED_TABLE with attributes '%s'" % (key, data))
            elif (op == "DEL"):
                (status, fvs) = state_peer_table.get(key)
                if status == True:
                    state_peer_table.delete(key)
                    log_info("Peer '(%s)' has been deleted from BGP_PEER_CONFIGURED_TABLE" % (key))
                else:
                    log_warn("Peer '(%s)' not found in BGP_PEER_CONFIGURED_TABLE" % (key))
            else:
                log_err("Update state DB called for Peer '(%s)' with unkown operation '%s'" % (key, op))
                return False
            return True
        except Exception as e:
            log_err("Update of state db failed for peer '(%s)' with error: %s" % (key, str(e)))
            return False

    def update_peer(self, vrf, nbr, data):
        """
        Update a peer. This is used when the peer is already in the FRR
        Update support only "admin_status" for now
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param data: associated data
        :return: True if this adding was successful, False otherwise
        """
        if "admin_status" in data:
            self.change_admin_status(vrf, nbr, data)
        elif "update" in self.templates and "ip_range" in data and self.peer_type == 'dynamic':
            self.change_ip_range(vrf, nbr, data)
        else:
            log_err("Peer '(%s|%s)': Can't update the peer. Only 'admin_status' attribute is supported" % (vrf, nbr))

        self.directory.put(self.db_name, self.table_name, vrf + '|' + nbr, data)
        return True

    def change_admin_status(self, vrf, nbr, data):
        """
        Change admin status of a peer
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param data: associated data
        :return: True if this adding was successful, False otherwise
        """
        if data['admin_status'] == 'up':
            self.apply_admin_status(vrf, nbr, "no shutdown", "up", data)
        elif data['admin_status'] == 'down':
            self.apply_admin_status(vrf, nbr, "shutdown", "down", data)
        else:
            print_data = vrf, nbr, data['admin_status']
            log_err("Peer '%s|%s': Can't update the peer. It has wrong attribute value attr['admin_status'] = '%s'" % print_data)

    def apply_admin_status(self, vrf, nbr, template_name, admin_state, data):
        """
        Render admin state template and apply the command to the FRR
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param template_name: name of the template to render
        :param admin_state: desired admin state
        :return: True if this adding was successful, False otherwise
        """
        print_data = vrf, nbr, admin_state
        ret_code = self.apply_op(self.templates[template_name].render(neighbor_addr=nbr), vrf)
        if ret_code:
            self.update_state_db(vrf, nbr, data, "SET")
            log_info("Peer '%s|%s' admin state is set to '%s'" % print_data)
        else:
            log_err("Can't set peer '%s|%s' admin state to '%s'." % print_data)

    def change_ip_range(self, vrf, nbr, data):
        """
        Change ip range of a peer
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param data: associated data
        :return: True if this adding was successful, False otherwise
        """
        if data['ip_range']:
            log_info("Peer '(%s|%s)' ip range is going to be updated with range: %s" % (vrf, nbr, data['ip_range']))
            new_ip_range = data["ip_range"].split(",")
            ip_ranges_to_add = new_ip_range
            ip_ranges_to_del = []
            existing_ipv4_range, existing_ipv6_range = self.get_existing_ip_ranges(vrf, nbr)
            if existing_ipv4_range:
                for ipv4_range in existing_ipv4_range:
                    if ipv4_range not in new_ip_range:
                        ip_ranges_to_del.append(ipv4_range)
                    else:
                        ip_ranges_to_add.remove(ipv4_range)
            if existing_ipv6_range:
                for ipv6_range in existing_ipv6_range:
                    if ipv6_range not in new_ip_range:
                        ip_ranges_to_del.append(ipv6_range)
                    else:
                        ip_ranges_to_add.remove(ipv6_range)
            if ip_ranges_to_del or ip_ranges_to_add:
                log_info("Peer '(%s|%s)' ip range is going to be updated. Ranges to delete: %s Ranges to add: %s" % (vrf, nbr, ip_ranges_to_del, ip_ranges_to_add))
                self.apply_range_changes(vrf, nbr, ip_ranges_to_add, ip_ranges_to_del, data)

    def get_existing_ip_ranges(self, vrf, nbr):
        """
        Get existing ip range of a peer
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :return: existing ipv4 and ipv6 ranges of a peer if they exist.
        """
        ipv4_ranges = []
        ipv6_ranges = []
        if vrf == 'default':
            command = ["vtysh", "-c", "show bgp peer-group %s json" % (nbr)]
        else:
            command = ["vtysh", "-c", "show bgp vrf %s peer-group %s json" % (vrf, nbr)]
        try:
            ret_code, out, err = run_command(command)
            if ret_code == 0:
                js_bgp = json.loads(out)
                if nbr in js_bgp and 'dynamicRanges' in js_bgp[nbr] and 'IPv4' in js_bgp[nbr]['dynamicRanges'] and 'ranges' in js_bgp[nbr]['dynamicRanges']['IPv4']:
                    ipv4_ranges = js_bgp[nbr]['dynamicRanges']['IPv4']['ranges']
                    log_info("Peer '(%s|%s)' already has ipV4 range: %s" % (vrf, nbr, ipv4_ranges))
                if nbr in js_bgp and 'dynamicRanges' in js_bgp[nbr] and 'IPv6' in js_bgp[nbr]['dynamicRanges'] and 'ranges' in js_bgp[nbr]['dynamicRanges']['IPv6']:
                    ipv6_ranges = js_bgp[nbr]['dynamicRanges']['IPv6']['ranges']
                    log_info("Peer '(%s|%s)' already has ipV6 range: %s" % (vrf, nbr, ipv6_ranges))
                return ipv4_ranges, ipv6_ranges
            else:
                log_err("Can't read ip range of peer '%s|%s': %s" % (vrf, nbr, str(err)))
                return ipv4_ranges, ipv6_ranges
        except Exception as e:
            log_err("Error in parsing ip range: %s" % str(e))
            return ipv4_ranges, ipv6_ranges

    def apply_range_changes(self, vrf, nbr, new_ip_range, ip_ranges_to_del, data):
        """
        Apply changes of ip range of a peer
        :param vrf: vrf name. Name is equal "default" for the global vrf
        :param nbr: neighbor ip address (name for dynamic peer type)
        :param new_ip_range: new ip range
        :param ip_ranges_to_del: ip ranges to delete
        """
        print_data = vrf, nbr, data
        kwargs = {
            'CONFIG_DB__DEVICE_METADATA': self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME),
            'vrf': vrf,
            'bgp_session': data,
            'delete_ranges': ip_ranges_to_del,
            'add_ranges': new_ip_range
        }
        try:
            cmd = self.templates["update"].render(**kwargs)
        except jinja2.TemplateError as e:
            msg = "Peer '(%s|%s)'. Error in rendering the template for 'SET' command '%s'" % print_data
            log_err("%s: %s" % (msg, str(e)))
            return True
        if cmd is not None:
            self.apply_op(cmd, vrf)
            self.update_state_db(vrf, nbr, data, "SET")
            log_info("Peer '(%s|%s)' ip range has been scheduled to be updated with range '%s'" % (vrf, nbr, data['ip_range']))

    def del_handler(self, key):
        """
        'DEL' handler for the BGP PEER tables
        :param key: key of the neighbor
        """
        vrf, nbr = self.split_key(key)
        peer_key = (vrf, nbr)
        if peer_key not in self.peers:
            log_warn("Peer '(%s|%s)' has not been found" % (vrf, nbr))
            return
        # Starting with FRR 10.1, if a peer group is attached to a "listen range",
        # the range must be removed before the peer group can be deleted.
        # To comply with this requirement, we first run the command "no bgp listen range ..." to
        # remove the "listen range" associated with the peer group, and only then proceed
        # with deleting the peer group.
        if self.peer_type == 'dynamic' or self.peer_type == 'sentinels':
            ip_ranges = self.directory.get(self.db_name, self.table_name, vrf + '|' + nbr).get("ip_range")
            if ip_ranges is not None:
                ip_ranges = ip_ranges.split(',')
                for ip_range in ip_ranges:
                    log_debug("Deleting listen range for peer-group {}, ip_range {}".format(ip_range, nbr))
                    cmd = self.templates["no listen range"].render(ip_range=ip_range, peer_group=nbr)
                    ret_code = self.apply_op(cmd, vrf)
                    if ret_code:
                        log_info("Listen range '%s' for peer '(%s|%s)' has been disabled" % (ip_range, vrf, nbr))
                    else:
                        log_err("Listen range '%s' for peer '(%s|%s)' hasn't been disabled" % (ip_range, vrf, nbr))
        
        kwargs = {
            'CONFIG_DB__DEVICE_METADATA': self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME),
            'neighbor_addr': nbr,
            'vrf': vrf
        }

        try:
            cmd = self.templates["delete"].render(**kwargs)
        except jinja2.TemplateError as e:
            log_err("Peer '(%s|%s)'. Error in rendering the template for 'DEL' command '%s'" % (vrf, nbr, str(e)))

        ret_code = self.apply_op(cmd, vrf)
        if ret_code:
            self.update_state_db(vrf, nbr, {}, "DEL")
            log_info("Peer '(%s|%s)' has been removed" % (vrf, nbr))
            self.peers.remove(peer_key)
        else:
            log_err("Peer '(%s|%s)' hasn't been removed" % (vrf, nbr))
        self.directory.remove(self.db_name, self.table_name, vrf + '|' + nbr)

    def apply_op(self, cmd, vrf):
        """
        Push commands cmd into FRR
        :param cmd: commands in raw format
        :param vrf: vrf where the commands should be applied
        :return: True if no errors, False if there are errors
        """
        bgp_asn = self.directory.get_slot("CONFIG_DB", swsscommon.CFG_DEVICE_METADATA_TABLE_NAME)["localhost"]["bgp_asn"]
        enable_bgp_suppress_fib_pending_cmd = 'bgp suppress-fib-pending'
        if vrf == 'default':
            cmd = ('router bgp %s\n %s\n' % (bgp_asn, enable_bgp_suppress_fib_pending_cmd)) + cmd + "\nexit"
        else:
            cmd = ('router bgp %s vrf %s\n %s\n' % (bgp_asn, vrf, enable_bgp_suppress_fib_pending_cmd)) + cmd + "\nexit"
        self.cfg_mgr.push(cmd)
        return True

    def get_lo_ipv4(self, loopback_str):
        """
        Extract Loopback0 ipv4 address from the Directory
        :return: ipv4 address for Loopback0, None if nothing found
        """
        loopback0_ipv4 = None
        for loopback in self.directory.get_slot("CONFIG_DB", swsscommon.CFG_LOOPBACK_INTERFACE_TABLE_NAME).keys():
            if loopback.startswith(loopback_str):
                loopback0_prefix_str = loopback.replace(loopback_str, "")
                loopback0_ip_str = loopback0_prefix_str[:loopback0_prefix_str.find('/')]
                if TemplateFabric.is_ipv4(loopback0_ip_str):
                    loopback0_ipv4 = loopback0_ip_str
                    break

        return loopback0_ipv4

    def get_local_interface(self, local_addr):
        """
        Get interface according to the local address from the directory
        :param: directory: Directory object that stored metadata of interfaces
        :param: local_addr: Local address of the interface
        :return: Return the metadata of the interface with the local address
                 If the interface has not been set, return None
        """
        local_addresses = self.directory.get_slot("LOCAL", "local_addresses")
        # Check if the local address of this bgp session has been set
        if local_addr not in local_addresses:
            return None
        local_address = local_addresses[local_addr]
        interfaces = self.directory.get_slot("LOCAL", "interfaces")
        # Check if the information for the interface of this local address has been set
        if "interface" in local_address and local_address["interface"] in interfaces:
            return interfaces[local_address["interface"]]
        else:
            return None

    @staticmethod
    def get_vnet(interface):
        """
        Get the VNet name of the interface
        :param: interface: The metadata of the interface
        :return: Return the vnet name of the interface if this interface belongs to a vnet,
                 Otherwise return None
        """
        if "vnet_name" in interface and interface["vnet_name"]:
            return interface["vnet_name"]
        else:
            return None

    @staticmethod
    def split_key(key):
        """
        Split key into ip address and vrf name. If there is no vrf, "default" would be return for vrf
        :param key: key to split
        :return: vrf name extracted from the key, peer ip address extracted from the key
        """
        if '|' not in key:
            return 'default', key
        else:
            return tuple(key.split('|', 1))

    @staticmethod
    def load_peers():
        """
        Load peers from FRR.
        :return: set of peers, which are already installed in FRR
        """
        command = ["vtysh", "-H", "/dev/null", "-c", "show bgp vrfs json"]
        ret_code, out, err = run_command(command)
        if ret_code == 0:
            js_vrf = json.loads(out)
            vrfs = js_vrf['vrfs'].keys()
        else:
            log_crit("Can't read bgp vrfs: %s" % err)
            raise Exception("Can't read bgp vrfs: %s" % err)
        peers = set()
        for vrf in vrfs:
            command = ["vtysh", "-c", 'show bgp vrf %s neighbors json' % str(vrf)]
            ret_code, out, err = run_command(command)
            if ret_code == 0:
                js_bgp = json.loads(out)
                for nbr in js_bgp.keys():
                    peers.add((vrf, nbr))
            else:
                log_crit("Can't read vrf '%s' neighbors: %s" % (vrf, str(err)))
                raise Exception("Can't read vrf '%s' neighbors: %s" % (vrf, str(err)))

        return peers
